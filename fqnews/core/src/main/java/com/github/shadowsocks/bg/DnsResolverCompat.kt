/*******************************************************************************
 *                                                                             *
 *  Copyright (C) 2019 by Max Lv <max.c.lv@gmail.com>                          *
 *  Copyright (C) 2019 by Mygod Studio <contact-shadowsocks-android@mygod.be>  *
 *                                                                             *
 *  This program is free software: you can redistribute it and/or modify       *
 *  it under the terms of the GNU General Public License as published by       *
 *  the Free Software Foundation, either version 3 of the License, or          *
 *  (at your option) any later version.                                        *
 *                                                                             *
 *  This program is distributed in the hope that it will be useful,            *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 *  GNU General Public License for more details.                               *
 *                                                                             *
 *  You should have received a copy of the GNU General Public License          *
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

package com.github.shadowsocks.bg

import android.annotation.SuppressLint
import android.annotation.TargetApi
import android.net.DnsResolver
import android.net.Network
import android.os.Build
import android.os.CancellationSignal
import android.os.Looper
import android.system.ErrnoException
import android.system.Os
import android.system.OsConstants
import com.github.shadowsocks.Core
import com.github.shadowsocks.utils.closeQuietly
import com.github.shadowsocks.utils.int
import com.github.shadowsocks.utils.parseNumericAddress
import com.github.shadowsocks.utils.printLog
import kotlinx.coroutines.*
import java.io.FileDescriptor
import java.io.IOException
import java.net.InetAddress
import java.util.concurrent.Executor
import java.util.concurrent.Executors
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException

sealed class DnsResolverCompat {
    companion object : DnsResolverCompat() {
        private val instance by lazy {
            when (Build.VERSION.SDK_INT) {
                in 29..Int.MAX_VALUE -> DnsResolverCompat29
                in 23 until 29 -> DnsResolverCompat23
                in 21 until 23 -> DnsResolverCompat21()
                else -> error("Unsupported API level")
            }
        }

        /**
         * Based on: https://android.googlesource.com/platform/frameworks/base/+/9f97f97/core/java/android/net/util/DnsUtils.java#341
         */
        private val address4 = "8.8.8.8".parseNumericAddress()!!
        private val address6 = "2000::".parseNumericAddress()!!
        suspend fun haveIpv4(network: Network) = checkConnectivity(network, OsConstants.AF_INET, address4)
        suspend fun haveIpv6(network: Network) = checkConnectivity(network, OsConstants.AF_INET6, address6)
        private suspend fun checkConnectivity(network: Network, domain: Int, addr: InetAddress) = try {
            val socket = Os.socket(domain, OsConstants.SOCK_DGRAM, OsConstants.IPPROTO_UDP)
            try {
                instance.bindSocket(network, socket)
                instance.connectUdp(socket, addr)
            } finally {
                socket.closeQuietly()
            }
            true
        } catch (e: IOException) {
            if ((e.cause as? ErrnoException)?.errno == OsConstants.EPERM) checkConnectivity(network, addr) else false
        } catch (_: ErrnoException) {
            false
        } catch (e: ReflectiveOperationException) {
            check(Build.VERSION.SDK_INT < 23)
            printLog(e)
            checkConnectivity(network, addr)
        }
        private fun checkConnectivity(network: Network, addr: InetAddress): Boolean {
            return Core.connectivity.getLinkProperties(network)?.routes?.any {
                try {
                    it.matches(addr)
                } catch (e: RuntimeException) {
                    printLog(e)
                    false
                }
            } == true
        }

        override fun bindSocket(network: Network, socket: FileDescriptor) = instance.bindSocket(network, socket)
        override suspend fun resolve(network: Network, host: String) = instance.resolve(network, host)
        override suspend fun resolveOnActiveNetwork(host: String) = instance.resolveOnActiveNetwork(host)
    }

    @Throws(IOException::class)
    abstract fun bindSocket(network: Network, socket: FileDescriptor)
    internal open suspend fun connectUdp(fd: FileDescriptor, address: InetAddress, port: Int = 0) =
            Os.connect(fd, address, port)
    abstract suspend fun resolve(network: Network, host: String): Array<InetAddress>
    abstract suspend fun resolveOnActiveNetwork(host: String): Array<InetAddress>

    @SuppressLint("PrivateApi")
    private open class DnsResolverCompat21 : DnsResolverCompat() {
        private val bindSocketToNetwork by lazy {
            Class.forName("android.net.NetworkUtils").getDeclaredMethod(
                    "bindSocketToNetwork", Int::class.java, Int::class.java)
        }
        private val netId by lazy { Network::class.java.getDeclaredField("netId") }
        override fun bindSocket(network: Network, socket: FileDescriptor) {
            val netId = netId.get(network)!!
            val err = bindSocketToNetwork.invoke(null, socket.int, netId) as Int
            if (err == 0) return
            val message = "Binding socket to network $netId"
            throw IOException(message, ErrnoException(message, -err))
        }

        override suspend fun connectUdp(fd: FileDescriptor, address: InetAddress, port: Int) {
            if (Looper.getMainLooper().thread == Thread.currentThread()) withContext(Dispatchers.IO) {  // #2405
                super.connectUdp(fd, address, port)
            } else super.connectUdp(fd, address, port)
        }

        /**
         * This dispatcher is used for noncancellable possibly-forever-blocking operations in network IO.
         *
         * See also: https://issuetracker.google.com/issues/133874590
         */
        private val unboundedIO by lazy {
            if (Core.activity.isLowRamDevice) Dispatchers.IO
            else Executors.newCachedThreadPool().asCoroutineDispatcher()
        }

        override suspend fun resolve(network: Network, host: String) =
                GlobalScope.async(unboundedIO) { network.getAllByName(host) }.await()
        override suspend fun resolveOnActiveNetwork(host: String) =
                GlobalScope.async(unboundedIO) { InetAddress.getAllByName(host) }.await()
    }

    @TargetApi(23)
    private object DnsResolverCompat23 : DnsResolverCompat21() {
        override fun bindSocket(network: Network, socket: FileDescriptor) = network.bindSocket(socket)
    }

    @TargetApi(29)
    private object DnsResolverCompat29 : DnsResolverCompat(), Executor {
        /**
         * This executor will run on its caller directly. On Q beta 3 thru 4, this results in calling in main thread.
         */
        override fun execute(command: Runnable) = command.run()

        override fun bindSocket(network: Network, socket: FileDescriptor) = network.bindSocket(socket)

        override suspend fun resolve(network: Network, host: String): Array<InetAddress> {
            return suspendCancellableCoroutine { cont ->
                val signal = CancellationSignal()
                cont.invokeOnCancellation { signal.cancel() }
                // retry should be handled by client instead
                DnsResolver.getInstance().query(network, host, DnsResolver.FLAG_NO_RETRY, this,
                        signal, object : DnsResolver.Callback<Collection<InetAddress>> {
                    override fun onAnswer(answer: Collection<InetAddress>, rcode: Int) =
                            cont.resume(answer.toTypedArray())
                    override fun onError(error: DnsResolver.DnsException) = cont.resumeWithException(IOException(error))
                })
            }
        }

        override suspend fun resolveOnActiveNetwork(host: String): Array<InetAddress> {
            return resolve(Core.connectivity.activeNetwork ?: return emptyArray(), host)
        }
    }
}

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

package com.github.shadowsocks.net

import android.util.Log
import com.crashlytics.android.Crashlytics
import com.github.shadowsocks.acl.AclMatcher
import com.github.shadowsocks.bg.BaseService
import com.github.shadowsocks.utils.printLog
import kotlinx.coroutines.*
import org.xbill.DNS.*
import java.io.IOException
import java.net.*
import java.nio.ByteBuffer
import java.nio.channels.DatagramChannel
import java.nio.channels.SelectionKey
import java.nio.channels.SocketChannel

/**
 * A simple DNS conditional forwarder.
 *
 * No cache is provided as localResolver may change from time to time. We expect DNS clients to do cache themselves.
 *
 * Based on:
 *   https://github.com/bitcoinj/httpseed/blob/809dd7ad9280f4bc98a356c1ffb3d627bf6c7ec5/src/main/kotlin/dns.kt
 *   https://github.com/shadowsocks/overture/tree/874f22613c334a3b78e40155a55479b7b69fee04
 */
class LocalDnsServer(private val localResolver: suspend (String) -> Array<InetAddress>,
                     private val remoteDns: Socks5Endpoint,
                     private val proxy: SocketAddress,
                     private val hosts: HostsFile,
                     /**
                      * Forward UDP queries to TCP.
                      */
                     private val tcp: Boolean = false,
                     aclSpawn: (suspend () -> AclMatcher)? = null) : CoroutineScope {
    companion object {
        private const val TAG = "LocalDnsServer"
        private const val TIMEOUT = 10_000L
        /**
         * TTL returned from localResolver is set to 120. Android API does not provide TTL,
         * so we suppose Android apps should not care about TTL either.
         */
        private const val TTL = 120L
        private const val UDP_PACKET_SIZE = 512

        private fun prepareDnsResponse(request: Message) = Message(request.header.id).apply {
            header.setFlag(Flags.QR.toInt())    // this is a response
            if (request.header.getFlag(Flags.RD.toInt())) header.setFlag(Flags.RD.toInt())
            request.question?.also { addRecord(it, Section.QUESTION) }
        }

        private fun cookDnsResponse(request: Message, results: Iterable<InetAddress>) =
                ByteBuffer.wrap(prepareDnsResponse(request).apply {
                    header.setFlag(Flags.RA.toInt())   // recursion available
                    for (address in results) addRecord(when (address) {
                        is Inet4Address -> ARecord(question.name, DClass.IN, TTL, address)
                        is Inet6Address -> AAAARecord(question.name, DClass.IN, TTL, address)
                        else -> error("Unsupported address $address")
                    }, Section.ANSWER)
                }.toWire())
    }

    private val monitor = ChannelMonitor()

    override val coroutineContext = SupervisorJob() + CoroutineExceptionHandler { _, t ->
        if (t is IOException) Crashlytics.log(Log.WARN, TAG, t.message) else printLog(t)
    }
    private val acl = aclSpawn?.let { async { it() } }

    suspend fun start(listen: SocketAddress) = DatagramChannel.open().run {
        configureBlocking(false)
        try {
            socket().bind(listen)
        } catch (e: BindException) {
            throw BaseService.ExpectedExceptionWrapper(e)
        }
        monitor.register(this, SelectionKey.OP_READ) { handlePacket(this) }
    }

    private fun handlePacket(channel: DatagramChannel) {
        val buffer = ByteBuffer.allocateDirect(UDP_PACKET_SIZE)
        val source = channel.receive(buffer)!!
        buffer.flip()
        launch {
            val reply = resolve(buffer)
            while (channel.send(reply, source) <= 0) monitor.wait(channel, SelectionKey.OP_WRITE)
        }
    }

    private suspend fun resolve(packet: ByteBuffer): ByteBuffer {
        val request = try {
            Message(packet)
        } catch (e: IOException) {  // we cannot parse the message, do not attempt to handle it at all
            Crashlytics.log(Log.WARN, TAG, e.message)
            return forward(packet)
        }
        return supervisorScope {
            val remote = async { withTimeout(TIMEOUT) { forward(packet) } }
            try {
                if (request.header.opcode != Opcode.QUERY) return@supervisorScope remote.await()
                val question = request.question
                val isIpv6 = when (question?.type) {
                    Type.A -> false
                    Type.AAAA -> true
                    else -> return@supervisorScope remote.await()
                }
                val host = question.name.canonicalize().toString(true)
                val hostsResults = hosts.resolve(host)
                if (hostsResults.isNotEmpty()) {
                    remote.cancel()
                    return@supervisorScope cookDnsResponse(request, hostsResults.run {
                        if (isIpv6) filterIsInstance<Inet6Address>() else filterIsInstance<Inet4Address>()
                    })
                }
                val acl = acl?.await() ?: return@supervisorScope remote.await()
                val useLocal = when (acl.shouldBypass(host)) {
                    true -> true.also { remote.cancel() }
                    false -> return@supervisorScope remote.await()
                    null -> false
                }
                val localResults = try {
                    withTimeout(TIMEOUT) { localResolver(host) }
                } catch (_: TimeoutCancellationException) {
                    Crashlytics.log(Log.WARN, TAG, "Local resolving timed out, falling back to remote resolving")
                    return@supervisorScope remote.await()
                } catch (_: UnknownHostException) {
                    return@supervisorScope remote.await()
                }
                if (isIpv6) {
                    val filtered = localResults.filterIsInstance<Inet6Address>()
                    if (useLocal) return@supervisorScope cookDnsResponse(request, filtered)
                    if (filtered.any { acl.shouldBypassIpv6(it.address) }) {
                        remote.cancel()
                        cookDnsResponse(request, filtered)
                    } else remote.await()
                } else {
                    val filtered = localResults.filterIsInstance<Inet4Address>()
                    if (useLocal) return@supervisorScope cookDnsResponse(request, filtered)
                    if (filtered.any { acl.shouldBypassIpv4(it.address) }) {
                        remote.cancel()
                        cookDnsResponse(request, filtered)
                    } else remote.await()
                }
            } catch (e: Exception) {
                remote.cancel()
                when (e) {
                    is TimeoutCancellationException -> Crashlytics.log(Log.WARN, TAG, "Remote resolving timed out")
                    is CancellationException -> { } // ignore
                    is IOException -> Crashlytics.log(Log.WARN, TAG, e.message)
                    else -> printLog(e)
                }
                ByteBuffer.wrap(prepareDnsResponse(request).apply {
                    header.rcode = Rcode.SERVFAIL
                }.toWire())
            }
        }
    }

    private suspend fun forward(packet: ByteBuffer): ByteBuffer {
        packet.position(0)  // the packet might have been parsed, reset to beginning
        return if (tcp) SocketChannel.open().use { channel ->
            channel.configureBlocking(false)
            channel.connect(proxy)
            val wrapped = remoteDns.tcpWrap(packet)
            while (!channel.finishConnect()) monitor.wait(channel, SelectionKey.OP_CONNECT)
            while (channel.write(wrapped) >= 0 && wrapped.hasRemaining()) monitor.wait(channel, SelectionKey.OP_WRITE)
            val result = remoteDns.tcpReceiveBuffer(UDP_PACKET_SIZE)
            remoteDns.tcpUnwrap(result, channel::read) { monitor.wait(channel, SelectionKey.OP_READ) }
            result
        } else DatagramChannel.open().use { channel ->
            channel.configureBlocking(false)
            monitor.wait(channel, SelectionKey.OP_WRITE)
            check(channel.send(remoteDns.udpWrap(packet), proxy) > 0)
            val result = remoteDns.udpReceiveBuffer(UDP_PACKET_SIZE)
            while (isActive) {
                monitor.wait(channel, SelectionKey.OP_READ)
                if (channel.receive(result) == proxy) break
                result.clear()
            }
            result.flip()
            remoteDns.udpUnwrap(result)
            result
        }
    }

    fun shutdown(scope: CoroutineScope) {
        cancel()
        monitor.close(scope)
        scope.launch {
            this@LocalDnsServer.coroutineContext[Job]!!.join()
            acl?.also { it.await().close() }
        }
    }
}

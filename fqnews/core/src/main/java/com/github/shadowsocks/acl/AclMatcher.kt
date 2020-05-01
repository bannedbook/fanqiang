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

package com.github.shadowsocks.acl

import com.github.shadowsocks.Core
import com.github.shadowsocks.net.Subnet
import java.io.Reader
import java.net.Inet4Address
import java.net.Inet6Address

class AclMatcher : AutoCloseable {
    companion object {
        init {
            System.loadLibrary("jni-helper")
        }

        @JvmStatic private external fun init(): Long
        @JvmStatic private external fun close(handle: Long)

        @JvmStatic private external fun addBypassDomain(handle: Long, regex: String): Boolean
        @JvmStatic private external fun addProxyDomain(handle: Long, regex: String): Boolean
        @JvmStatic private external fun build(handle: Long, memoryLimit: Long): String?
        @JvmStatic private external fun matchHost(handle: Long, host: String): Int
    }

    class Re2Exception(message: String) : IllegalArgumentException(message)

    private var handle = 0L
    override fun close() {
        if (handle != 0L) {
            close(handle)
            handle = 0L
        }
    }

    private var subnetsIpv4 = emptyList<Subnet.Immutable>()
    private var subnetsIpv6 = emptyList<Subnet.Immutable>()
    private var bypass = false

    suspend fun init(id: String) = init(Acl.getFile(id).bufferedReader())
    suspend fun init(reader: Reader) {
        fun Sequence<Subnet>.dedup() = sequence {
            val iterator = map { it.toImmutable() }.sortedWith(Subnet.Immutable).iterator()
            var current: Subnet.Immutable? = null
            while (iterator.hasNext()) {
                val next = iterator.next()
                if (current?.matches(next) == true) continue
                yield(next)
                current = next
            }
        }.toList()
        check(handle == 0L)
        handle = init()
        try {
            val (bypass, subnets) = Acl.parse(reader, {
                check(addBypassDomain(handle, it))
            }, {
                check(addProxyDomain(handle, it))
            })
            build(handle, if (Core.activity.isLowRamDevice) 8 shl 20 else 64 shl 20)?.also { throw Re2Exception(it) }
            subnetsIpv4 = subnets.asSequence().filter { it.address is Inet4Address }.dedup()
            subnetsIpv6 = subnets.asSequence().filter { it.address is Inet6Address }.dedup()
            this.bypass = bypass
        } catch (e: Exception) {
            close()
            throw e
        }
    }

    private fun quickMatches(subnets: List<Subnet.Immutable>, ip: ByteArray): Boolean {
        val i = subnets.binarySearch(Subnet.Immutable(ip), Subnet.Immutable)
        return i >= 0 || i < -1 && subnets[-i - 2].matches(ip)
    }

    fun shouldBypassIpv4(ip: ByteArray) = bypass xor quickMatches(subnetsIpv4, ip)
    fun shouldBypassIpv6(ip: ByteArray) = bypass xor quickMatches(subnetsIpv6, ip)
    fun shouldBypass(host: String) = when (val e = matchHost(handle, host)) {
        0 -> null
        1 -> true
        2 -> false
        else -> error("matchHost -> $e")
    }
}

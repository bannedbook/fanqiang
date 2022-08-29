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

import com.github.shadowsocks.utils.parseNumericAddress
import net.sourceforge.jsocks.Socks4Message
import net.sourceforge.jsocks.Socks5Message
import java.io.EOFException
import java.io.IOException
import java.net.Inet4Address
import java.net.Inet6Address
import java.nio.ByteBuffer
import kotlin.math.max

class Socks5Endpoint(host: String, port: Int) {
    private val dest = host.parseNumericAddress().let { numeric ->
        val bytes = numeric?.address ?: host.toByteArray().apply { check(size < 256) { "Hostname too long" } }
        val type = when (numeric) {
            null -> Socks5Message.SOCKS_ATYP_DOMAINNAME
            is Inet4Address -> Socks5Message.SOCKS_ATYP_IPV4
            is Inet6Address -> Socks5Message.SOCKS_ATYP_IPV6
            else -> error("Unsupported address type $numeric")
        }
        ByteBuffer.allocate(bytes.size + (if (numeric == null) 1 else 0) + 3).apply {
            put(type.toByte())
            if (numeric == null) put(bytes.size.toByte())
            put(bytes)
            putShort(port.toShort())
        }
    }.array()
    private val headerReserved = max(3 + 3 + 16, 3 + dest.size)

    fun tcpWrap(message: ByteBuffer): ByteBuffer {
        check(message.remaining() < 65536) { "TCP message too large" }
        return ByteBuffer.allocateDirect(8 + dest.size + message.remaining()).apply {
            put(Socks5Message.SOCKS_VERSION.toByte())
            put(1)  // nmethods
            put(0)  // no authentication required
            // header
            put(Socks5Message.SOCKS_VERSION.toByte())
            put(Socks4Message.REQUEST_CONNECT.toByte())
            put(0)  // reserved
            put(dest)
            // data
            putShort(message.remaining().toShort())
            put(message)
            flip()
        }
    }
    fun tcpReceiveBuffer(size: Int) = ByteBuffer.allocateDirect(headerReserved + 4 + size)
    suspend fun tcpUnwrap(buffer: ByteBuffer, reader: (ByteBuffer) -> Int, wait: suspend () -> Unit) {
        suspend fun readBytes(till: Int) {
            if (buffer.position() >= till) return
            while (reader(buffer) >= 0 && buffer.position() < till) wait()
            if (buffer.position() < till) throw EOFException("${buffer.position()} < $till")
        }
        suspend fun read(index: Int): Byte {
            readBytes(index + 1)
            return buffer[index]
        }
        if (read(0) != Socks5Message.SOCKS_VERSION.toByte()) throw IOException("Unsupported SOCKS version ${buffer[0]}")
        if (read(1) != 0.toByte()) throw IOException("Unsupported authentication ${buffer[1]}")
        if (read(2) != Socks5Message.SOCKS_VERSION.toByte()) throw IOException("Unsupported SOCKS version ${buffer[2]}")
        if (read(3) != 0.toByte()) throw IOException("SOCKS5 server returned error ${buffer[3]}")
        val dataOffset = when (val type = read(5)) {
            Socks5Message.SOCKS_ATYP_IPV4.toByte() -> 4
            Socks5Message.SOCKS_ATYP_DOMAINNAME.toByte() -> 1 + read(6)
            Socks5Message.SOCKS_ATYP_IPV6.toByte() -> 16
            else -> throw IOException("Unsupported address type $type")
        } + 8
        readBytes(dataOffset + 2)
        buffer.limit(buffer.position()) // store old position to update mark
        buffer.position(dataOffset)
        val dataLength = buffer.short.toUShort().toInt()
        val end = buffer.position() + dataLength
        if (end > buffer.capacity()) throw IOException(
                "Buffer too small to contain the message: $dataLength > ${buffer.capacity() - buffer.position()}")
        buffer.mark()
        buffer.position(buffer.limit()) // restore old position
        buffer.limit(end)
        readBytes(buffer.limit())
        buffer.reset()
    }

    private fun ByteBuffer.tryPosition(newPosition: Int) {
        if (limit() < newPosition) throw EOFException("${limit()} < $newPosition")
        position(newPosition)
    }

    fun udpWrap(packet: ByteBuffer) = ByteBuffer.allocateDirect(3 + dest.size + packet.remaining()).apply {
        // header
        putShort(0) // reserved
        put(0)      // fragment number
        put(dest)
        // data
        put(packet)
        flip()
    }
    fun udpReceiveBuffer(size: Int) = ByteBuffer.allocateDirect(headerReserved + size)
    fun udpUnwrap(packet: ByteBuffer) {
        packet.tryPosition(3)
        packet.tryPosition(6 + when (val type = packet.get()) {
            Socks5Message.SOCKS_ATYP_IPV4.toByte() -> 4
            Socks5Message.SOCKS_ATYP_DOMAINNAME.toByte() -> 1 + packet.get()
            Socks5Message.SOCKS_ATYP_IPV6.toByte() -> 16
            else -> throw IOException("Unsupported address type $type")
        })
        packet.mark()
    }
}

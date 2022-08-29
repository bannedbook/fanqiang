/*******************************************************************************
 *                                                                             *
 *  Copyright (C) 2017 by Max Lv <max.c.lv@gmail.com>                          *
 *  Copyright (C) 2017 by Mygod Studio <contact-shadowsocks-android@mygod.be>  *
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
import java.net.InetAddress
import java.util.*

class Subnet(val address: InetAddress, val prefixSize: Int) : Comparable<Subnet> {
    companion object {
        fun fromString(value: String, lengthCheck: Int = -1): Subnet? {
            val parts = value.split('/', limit = 2)
            val addr = parts[0].parseNumericAddress() ?: return null
            check(lengthCheck < 0 || addr.address.size == lengthCheck)
            return if (parts.size == 2) try {
                val prefixSize = parts[1].toInt()
                if (prefixSize < 0 || prefixSize > addr.address.size shl 3) null else Subnet(addr, prefixSize)
            } catch (_: NumberFormatException) {
                null
            } else Subnet(addr, addr.address.size shl 3)
        }
    }

    private val addressLength get() = address.address.size shl 3

    init {
        require(prefixSize in 0..addressLength) { "prefixSize $prefixSize not in 0..$addressLength" }
    }

    class Immutable(private val a: ByteArray, private val prefixSize: Int = 0) {
        companion object : Comparator<Immutable> {
            override fun compare(a: Immutable, b: Immutable): Int {
                check(a.a.size == b.a.size)
                for (i in a.a.indices) {
                    val result = a.a[i].compareTo(b.a[i])
                    if (result != 0) return result
                }
                return 0
            }
        }

        fun matches(b: Immutable) = matches(b.a)
        fun matches(b: ByteArray): Boolean {
            if (a.size != b.size) return false
            var i = 0
            while (i * 8 < prefixSize && i * 8 + 8 <= prefixSize) {
                if (a[i] != b[i]) return false
                ++i
            }
            return i * 8 == prefixSize || a[i] == (b[i].toInt() and -(1 shl i * 8 + 8 - prefixSize)).toByte()
        }
    }
    fun toImmutable() = Immutable(address.address.also {
        var i = prefixSize / 8
        if (prefixSize % 8 > 0) {
            it[i] = (it[i].toInt() and -(1 shl i * 8 + 8 - prefixSize)).toByte()
            ++i
        }
        while (i < it.size) it[i++] = 0
    }, prefixSize)

    override fun toString(): String =
            if (prefixSize == addressLength) address.hostAddress else address.hostAddress + '/' + prefixSize

    private fun Byte.unsigned() = toInt() and 0xFF
    override fun compareTo(other: Subnet): Int {
        val addrThis = address.address
        val addrThat = other.address.address
        var result = addrThis.size.compareTo(addrThat.size)                 // IPv4 address goes first
        if (result != 0) return result
        for (i in addrThis.indices) {
            result = addrThis[i].unsigned().compareTo(addrThat[i].unsigned())   // undo sign extension of signed byte
            if (result != 0) return result
        }
        return prefixSize.compareTo(other.prefixSize)
    }

    override fun equals(other: Any?): Boolean {
        val that = other as? Subnet
        return address == that?.address && prefixSize == that.prefixSize
    }
    override fun hashCode(): Int = Objects.hash(address, prefixSize)
}

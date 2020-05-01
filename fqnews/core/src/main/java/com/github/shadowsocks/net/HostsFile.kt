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
import java.net.InetAddress

class HostsFile(input: String = "") {
    private val map = mutableMapOf<String, MutableSet<InetAddress>>()
    init {
        for (line in input.lineSequence()) {
            val entries = line.substringBefore('#').splitToSequence(' ', '\t').filter { it.isNotEmpty() }
            val address = entries.firstOrNull()?.parseNumericAddress() ?: continue
            for (hostname in entries.drop(1)) map.computeIfAbsent(hostname) { LinkedHashSet(1) }.add(address)
        }
    }

    val configuredHostnames get() = map.size
    fun resolve(hostname: String) = map[hostname]?.shuffled() ?: emptyList()
}

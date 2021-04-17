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

package com.github.shadowsocks.bg

import android.net.LocalSocket
import android.os.SystemClock
import com.github.shadowsocks.aidl.TrafficStats
import com.github.shadowsocks.database.ProfileManager
import com.github.shadowsocks.net.LocalSocketListener
import com.github.shadowsocks.preference.DataStore
import com.github.shadowsocks.utils.DirectBoot
import java.io.File
import java.io.IOException
import java.nio.ByteBuffer
import java.nio.ByteOrder

open class TrafficMonitor(statFile: File) {
    val thread = object : LocalSocketListener("TrafficMonitor-" + statFile.name, statFile) {
        private val buffer = ByteArray(16)
        private val stat = ByteBuffer.wrap(buffer).order(ByteOrder.LITTLE_ENDIAN)
        override fun acceptInternal(socket: LocalSocket) {
            if (socket.inputStream.read(buffer) != 16) throw IOException("Unexpected traffic stat length")
            val tx = stat.getLong(0)
            val rx = stat.getLong(8)
            if (current.txTotal != tx) {
                current.txTotal = tx
                dirty = true
            }
            if (current.rxTotal != rx) {
                current.rxTotal = rx
                dirty = true
            }
        }
    }.apply { start() }

    val current = TrafficStats()
    var out = TrafficStats()
    private var timestampLast = 0L
    private var dirty = false
    private var persisted: TrafficStats? = null

    open fun requestUpdate(): Pair<TrafficStats, Boolean> {
        val now = SystemClock.elapsedRealtime()
        val delta = now - timestampLast
        timestampLast = now
        var updated = false
        if (delta != 0L) {
            if (dirty) {
                out = current.copy().apply {
                    txRate = (txTotal - out.txTotal) * 1000 / delta
                    rxRate = (rxTotal - out.rxTotal) * 1000 / delta
                }
                dirty = false
                updated = true
            } else {
                if (out.txRate != 0L) {
                    out.txRate = 0
                    updated = true
                }
                if (out.rxRate != 0L) {
                    out.rxRate = 0
                    updated = true
                }
            }
        }
        return Pair(out, updated)
    }

    fun persistStats(id: Long) {
        val current = current
        check(persisted == null || persisted == current) { "Data loss occurred" }
        persisted = current
        try {
            // profile may have host, etc. modified and thus a re-fetch is necessary (possible race condition)
            val profile = ProfileManager.getProfile(id) ?: return
            profile.tx += current.txTotal
            profile.rx += current.rxTotal
            ProfileManager.updateProfile(profile)
        } catch (e: IOException) {
            if (!DataStore.directBootAware) throw e // we should only reach here because we're in direct boot
            val profile = DirectBoot.getDeviceProfile()!!.toList().filterNotNull().single { it.id == id }
            profile.tx += current.txTotal
            profile.rx += current.rxTotal
            profile.dirty = true
            DirectBoot.update(profile)
            DirectBoot.listenForUnlock()
        }
    }
}

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

import android.net.LocalServerSocket
import android.net.LocalSocket
import android.net.LocalSocketAddress
import android.system.ErrnoException
import android.system.Os
import android.system.OsConstants
import com.github.shadowsocks.utils.printLog
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.channels.sendBlocking
import kotlinx.coroutines.launch
import java.io.File
import java.io.IOException

abstract class LocalSocketListener(name: String, socketFile: File) : Thread(name) {
    private val localSocket = LocalSocket().apply {
        socketFile.delete() // It's a must-have to close and reuse previous local socket.
        bind(LocalSocketAddress(socketFile.absolutePath, LocalSocketAddress.Namespace.FILESYSTEM))
    }
    private val serverSocket = LocalServerSocket(localSocket.fileDescriptor)
    private val closeChannel = Channel<Unit>(1)
    @Volatile
    protected var running = true

    /**
     * Inherited class do not need to close input/output streams as they will be closed automatically.
     */
    protected open fun accept(socket: LocalSocket) = socket.use { acceptInternal(socket) }
    protected abstract fun acceptInternal(socket: LocalSocket)
    final override fun run() {
        localSocket.use {
            while (running) {
                try {
                    accept(serverSocket.accept())
                } catch (e: IOException) {
                    if (running) printLog(e)
                    continue
                }
            }
        }
        closeChannel.sendBlocking(Unit)
    }

    open fun shutdown(scope: CoroutineScope) {
        running = false
        localSocket.fileDescriptor?.apply {
            // see also: https://issuetracker.google.com/issues/36945762#comment15
            if (valid()) try {
                Os.shutdown(this, OsConstants.SHUT_RDWR)
            } catch (e: ErrnoException) {
                // suppress fd inactive or already closed
                if (e.errno != OsConstants.EBADF && e.errno != OsConstants.ENOTCONN) throw IOException(e)
            }
        }
        scope.launch { closeChannel.receive() }
    }
}

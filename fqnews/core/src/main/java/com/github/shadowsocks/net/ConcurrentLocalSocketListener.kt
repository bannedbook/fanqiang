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

import android.net.LocalSocket
import com.github.shadowsocks.utils.printLog
import kotlinx.coroutines.*
import java.io.File

abstract class ConcurrentLocalSocketListener(name: String, socketFile: File) : LocalSocketListener(name, socketFile),
        CoroutineScope {
    override val coroutineContext = Dispatchers.IO + SupervisorJob() + CoroutineExceptionHandler { _, t -> printLog(t) }

    override fun accept(socket: LocalSocket) {
        launch { super.accept(socket) }
    }

    override fun shutdown(scope: CoroutineScope) {
        running = false
        cancel()
        super.shutdown(scope)
        coroutineContext[Job]!!.also { job -> scope.launch { job.join() } }
    }
}

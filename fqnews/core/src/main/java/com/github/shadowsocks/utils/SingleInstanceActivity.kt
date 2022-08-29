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

package com.github.shadowsocks.utils

import androidx.activity.ComponentActivity
import androidx.annotation.MainThread
import androidx.lifecycle.DefaultLifecycleObserver
import androidx.lifecycle.LifecycleOwner

/**
 * See also: https://stackoverflow.com/a/30821062/2245107
 */
object SingleInstanceActivity : DefaultLifecycleObserver {
    private val active = mutableSetOf<Class<LifecycleOwner>>()

    @MainThread
    fun register(activity: ComponentActivity) = if (active.add(activity.javaClass)) apply {
        activity.lifecycle.addObserver(this)
    } else {
        activity.finish()
        null
    }

    override fun onDestroy(owner: LifecycleOwner) {
        check(active.remove(owner.javaClass)) { "Double destroy?" }
    }
}

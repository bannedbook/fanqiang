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

package com.github.shadowsocks.plugin

import android.util.Log
import com.crashlytics.android.Crashlytics
import com.github.shadowsocks.utils.Commandline
import java.util.*

class PluginConfiguration(val pluginsOptions: Map<String, PluginOptions>, val selected: String) {
    private constructor(plugins: List<PluginOptions>) : this(
            plugins.filter { it.id.isNotEmpty() }.associateBy { it.id },
            if (plugins.isEmpty()) "" else plugins[0].id)
    constructor(plugin: String) : this(plugin.split('\n').map { line ->
        if (line.startsWith("kcptun ")) {
            val opt = PluginOptions()
            opt.id = "kcptun"
            try {
                val iterator = Commandline.translateCommandline(line).drop(1).iterator()
                while (iterator.hasNext()) {
                    val option = iterator.next()
                    when {
                        option == "--nocomp" -> opt["nocomp"] = null
                        option.startsWith("--") -> opt[option.substring(2)] = iterator.next()
                        else -> throw IllegalArgumentException("Unknown kcptun parameter: $option")
                    }
                }
            } catch (exc: Exception) {
                Crashlytics.log(Log.WARN, "PluginConfiguration", exc.message)
            }
            opt
        } else PluginOptions(line)
    })

    fun getOptions(
            id: String = selected,
            defaultConfig: () -> String? = { PluginManager.fetchPlugins().lookup[id]?.defaultConfig }
    ) = if (id.isEmpty()) PluginOptions() else pluginsOptions[id] ?: PluginOptions(id, defaultConfig())

    override fun toString(): String {
        val result = LinkedList<PluginOptions>()
        for ((id, opt) in pluginsOptions) if (id == this.selected) result.addFirst(opt) else result.addLast(opt)
        if (!pluginsOptions.contains(selected)) result.addFirst(getOptions())
        return result.joinToString("\n") { it.toString(false) }
    }
}

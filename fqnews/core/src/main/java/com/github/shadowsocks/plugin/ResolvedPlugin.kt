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

import android.content.pm.ComponentInfo
import android.content.pm.ResolveInfo
import android.graphics.drawable.Drawable
import android.os.Build
import com.github.shadowsocks.Core
import com.github.shadowsocks.Core.app
import com.github.shadowsocks.plugin.PluginManager.loadString
import com.github.shadowsocks.utils.signaturesCompat

abstract class ResolvedPlugin(protected val resolveInfo: ResolveInfo) : Plugin() {
    protected abstract val componentInfo: ComponentInfo

    override val id by lazy { componentInfo.loadString(PluginContract.METADATA_KEY_ID)!! }
    override val idAliases: Array<String> by lazy {
        when (val value = componentInfo.metaData.get(PluginContract.METADATA_KEY_ID_ALIASES)) {
            is String -> arrayOf(value)
            is Int -> app.packageManager.getResourcesForApplication(componentInfo.applicationInfo).run {
                when (getResourceTypeName(value)) {
                    "string" -> arrayOf(getString(value))
                    else -> getStringArray(value)
                }
            }
            null -> emptyArray()
            else -> error("unknown type for plugin meta-data idAliases")
        }
    }
    override val label: CharSequence get() = resolveInfo.loadLabel(app.packageManager)
    override val icon: Drawable get() = resolveInfo.loadIcon(app.packageManager)
    override val defaultConfig by lazy { componentInfo.loadString(PluginContract.METADATA_KEY_DEFAULT_CONFIG) }
    override val packageName: String get() = componentInfo.packageName
    override val trusted by lazy {
        Core.getPackageInfo(packageName).signaturesCompat.any(PluginManager.trustedSignatures::contains)
    }
    override val directBootAware get() = Build.VERSION.SDK_INT < 24 || componentInfo.directBootAware
}

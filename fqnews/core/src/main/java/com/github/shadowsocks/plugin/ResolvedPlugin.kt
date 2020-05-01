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

import android.content.pm.ResolveInfo
import android.graphics.drawable.Drawable
import android.os.Bundle
import com.github.shadowsocks.Core
import com.github.shadowsocks.Core.app
import com.github.shadowsocks.utils.signaturesCompat

abstract class ResolvedPlugin(protected val resolveInfo: ResolveInfo) : Plugin() {
    protected abstract val metaData: Bundle

    override val id: String by lazy { metaData.getString(PluginContract.METADATA_KEY_ID)!! }
    override val label: CharSequence by lazy { resolveInfo.loadLabel(app.packageManager) }
    override val icon: Drawable by lazy { resolveInfo.loadIcon(app.packageManager) }
    override val defaultConfig: String? by lazy { metaData.getString(PluginContract.METADATA_KEY_DEFAULT_CONFIG) }
    override val packageName: String get() = resolveInfo.resolvePackageName
    override val trusted by lazy {
        Core.getPackageInfo(packageName).signaturesCompat.any(PluginManager.trustedSignatures::contains)
    }
}

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

import android.content.Intent
import android.os.Bundle
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity

/**
 * Activity that's capable of getting EXTRA_OPTIONS input.
 */
abstract class OptionsCapableActivity : AppCompatActivity() {
    protected fun pluginOptions(intent: Intent = this.intent) = try {
        PluginOptions("", intent.getStringExtra(PluginContract.EXTRA_OPTIONS))
    } catch (exc: IllegalArgumentException) {
        Toast.makeText(this, exc.message, Toast.LENGTH_SHORT).show()
        PluginOptions()
    }

    /**
     * Populate args to your user interface.
     *
     * @param options PluginOptions parsed.
     */
    protected abstract fun onInitializePluginOptions(options: PluginOptions = pluginOptions())

    override fun onPostCreate(savedInstanceState: Bundle?) {
        super.onPostCreate(savedInstanceState)
        if (savedInstanceState == null) onInitializePluginOptions()
    }
}

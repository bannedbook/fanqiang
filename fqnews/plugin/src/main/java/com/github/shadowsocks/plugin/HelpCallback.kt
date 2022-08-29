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

/**
 * HelpCallback is an HelpActivity but you just need to produce a CharSequence help message instead of having to
 * provide UI. To create a help callback, just extend this class, implement abstract methods, and add it to your
 * manifest following the same procedure as adding a HelpActivity.
 */
abstract class HelpCallback : HelpActivity() {
    abstract fun produceHelpMessage(options: PluginOptions): CharSequence

    override fun onInitializePluginOptions(options: PluginOptions) {
        setResult(RESULT_OK, Intent().putExtra(PluginContract.EXTRA_HELP_MESSAGE, produceHelpMessage(options)))
        finish()
    }
}

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

/**
 * Base class for a help activity. A help activity is started when user taps help when configuring options for your
 * plugin. To create a help activity, just extend this class, and add it to your manifest like this:
 *
 * <pre class="prettyprint">&lt;manifest&gt;
 *    ...
 *    &lt;application&gt;
 *        ...
 *        &lt;activity android:name=".HelpActivity"&gt;
 *            &lt;intent-filter&gt;
 *                &lt;action android:name="com.github.shadowsocks.plugin.ACTION_HELP"/&gt;
 *                &lt;category android:name="android.intent.category.DEFAULT"/&gt;
 *                &lt;data android:scheme="plugin"
 *                         android:host="com.github.shadowsocks"
 *                         android:path="/$PLUGIN_ID"/&gt;
 *            &lt;/intent-filter&gt;
 *        &lt;/activity&gt;
 *        ...
 *    &lt;/application&gt;
 *&lt;/manifest&gt;</pre>
 */
abstract class HelpActivity : OptionsCapableActivity() {
    override fun onInitializePluginOptions(options: PluginOptions) { }
}

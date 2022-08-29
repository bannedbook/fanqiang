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

import android.app.Activity
import android.content.Intent

/**
 * Base class for configuration activity. A configuration activity is started when user wishes to configure the
 * selected plugin. To create a configuration activity, extend this class, implement abstract methods, invoke
 * `saveChanges(options)` and `discardChanges()` when appropriate, and add it to your manifest like this:
 *
 * <pre class="prettyprint">&lt;manifest&gt;
 *    ...
 *    &lt;application&gt;
 *        ...
 *        &lt;activity android:name=".ConfigureActivity"&gt;
 *            &lt;intent-filter&gt;
 *                &lt;action android:name="com.github.shadowsocks.plugin.ACTION_CONFIGURE"/&gt;
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
abstract class ConfigurationActivity : OptionsCapableActivity() {
    /**
     * Equivalent to setResult(RESULT_CANCELED).
     */
    fun discardChanges() = setResult(Activity.RESULT_CANCELED)

    /**
     * Equivalent to setResult(RESULT_OK, args_with_correct_format).
     *
     * @param options PluginOptions to save.
     */
    fun saveChanges(options: PluginOptions) =
            setResult(Activity.RESULT_OK, Intent().putExtra(PluginContract.EXTRA_OPTIONS, options.toString()))

    /**
     * Finish this activity and request manual editor to pop up instead.
     */
    fun fallbackToManualEditor() {
        setResult(PluginContract.RESULT_FALLBACK)
        finish()
    }
}

/*
 * Copyright (c) 2012-2018 Frederic Julian
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http:></http:>//www.gnu.org/licenses/>.
 */

package net.frju.flym

import SpeedUpVPN.VpnEncrypt
import android.annotation.SuppressLint
import android.app.Application
import android.content.Context
import android.util.Log
import com.github.shadowsocks.Core
import info.guardianproject.netcipher.webkit.WebkitProxy
import net.frju.flym.data.AppDatabase
import net.frju.flym.data.utils.PrefConstants
import net.frju.flym.ui.main.MainActivity
import net.frju.flym.utils.putPrefBoolean


class App : Application() {

    companion object {
        @SuppressLint("StaticFieldLeak")
        @JvmStatic
        lateinit var context: Context
            private set

        @JvmStatic
        lateinit var db: AppDatabase
            private set

        lateinit var instance: App
    }

    override fun onCreate() {
        super.onCreate()
        initializeProxy()
        instance = this
        context = applicationContext
        db = AppDatabase.createDatabase(context)

        context.putPrefBoolean(PrefConstants.IS_REFRESHING, false) // init
        Core.init(this, MainActivity::class)
    }

    fun initializeProxy() {
        val host = "127.0.0.1"
        val port: Int = VpnEncrypt.HTTP_PROXY_PORT
        try {
            WebkitProxy.setProxy(this::class.java.name, this, null, host, port)
        } catch (e: java.lang.Exception) {
            Log.e("initializeProxy", "error enabling web proxying", e)
            //Core.alertMessage("error enabling web proxying:"+e.message,context)
        }
    }
}

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

import android.annotation.SuppressLint
import android.app.Application
import android.content.Context
import net.frju.flym.data.AppDatabase
import net.frju.flym.data.utils.PrefConstants
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
    }

    fun initializeProxy() {
    }
}

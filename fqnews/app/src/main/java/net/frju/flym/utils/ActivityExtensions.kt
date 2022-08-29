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

package net.frju.flym.utils

import android.app.Activity
import net.fred.feedex.R
import net.frju.flym.data.utils.PrefConstants
import org.jetbrains.anko.inputMethodManager

fun Activity.closeKeyboard() {
	currentFocus?.let {
		inputMethodManager.hideSoftInputFromWindow(it.windowToken, 0)
	}
}

fun Activity.setupTheme() {
	setTheme(when (getPrefString(PrefConstants.THEME, "DARK")) {
		"LIGHT" -> R.style.AppThemeLight
		"BLACK" -> R.style.AppThemeBlack
		else -> R.style.AppTheme
	})
}

fun Activity.setupNoActionBarTheme() {
	setTheme(when (getPrefString(PrefConstants.THEME, "DARK")) {
		"LIGHT" -> R.style.AppThemeLight_NoActionBar
		"BLACK" -> R.style.AppThemeBlack_NoActionBar
		else -> R.style.AppTheme_NoActionBar
	})
}
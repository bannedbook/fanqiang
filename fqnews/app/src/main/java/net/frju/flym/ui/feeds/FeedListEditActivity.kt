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

package net.frju.flym.ui.feeds

import android.os.Bundle
import android.view.MenuItem
import androidx.appcompat.app.AppCompatActivity
import net.fred.feedex.R
import net.frju.flym.utils.setupTheme

class FeedListEditActivity : AppCompatActivity() {

	override fun onCreate(savedInstanceState: Bundle?) {
		setupTheme()

		super.onCreate(savedInstanceState)

		supportActionBar?.setDisplayHomeAsUpEnabled(true)

		setContentView(R.layout.activity_feed_list_edit)
	}
}

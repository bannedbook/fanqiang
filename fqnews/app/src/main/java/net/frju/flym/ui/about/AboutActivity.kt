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

package net.frju.flym.ui.about

import android.os.Bundle
import android.view.MenuItem
import androidx.appcompat.app.AppCompatActivity
import com.vansuita.materialabout.builder.AboutBuilder
import net.fred.feedex.R
import net.frju.flym.utils.setupTheme


class AboutActivity : AppCompatActivity() {

	override fun onCreate(savedInstanceState: Bundle?) {
		setupTheme()

		super.onCreate(savedInstanceState)

		supportActionBar?.setDisplayHomeAsUpEnabled(true)

		val view = AboutBuilder.with(this)
				//.setPhoto(R.mipmap.profile_picture)
				//.setCover(R.mipmap.profile_cover)
				.setName("bannedbook.org")
				.setBrief(R.string.about_info)
				//.setAppIcon(R.mipmap.ic_launcher_foreground)
				.setAppName(R.string.app_name)
				.setVersionNameAsAppSubTitle()
				.addLink(R.mipmap.github,"fanqiang","https://github.com/bannedbook/fanqiang/wiki")
				.addLink(R.mipmap.github,R.string.app_name,"https://github.com/bannedbook/fanqiang/blob/master/fqnews/readme.md")
				//.addTwitterLink("bannedebook")
				//.addFacebookLink("bannedbook.org")

				.addFiveStarsAction()
				.addShareAction(R.string.app_name)
				.setWrapScrollView(true)
				.setLinksAnimated(true)
				.setShowAsCard(true)
				//.addLink(R.mipmap.github,"Flym","https://github.com/FredJul/Flym")
				.build()
		setContentView(view)
	}
}
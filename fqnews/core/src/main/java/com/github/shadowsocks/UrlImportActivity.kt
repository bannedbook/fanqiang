/*******************************************************************************
 *                                                                             *
 *  Copyright (C) 2019 by Max Lv <max.c.lv@gmail.com>                          *
 *  Copyright (C) 2019 by Mygod Studio <contact-shadowsocks-android@mygod.be>  *
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

package com.github.shadowsocks

import android.content.DialogInterface
import android.os.Bundle
import android.os.Parcelable
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import com.github.shadowsocks.core.R
import com.github.shadowsocks.database.Profile
import com.github.shadowsocks.database.ProfileManager
import com.github.shadowsocks.plugin.AlertDialogFragment
import com.github.shadowsocks.plugin.Empty
import com.github.shadowsocks.plugin.showAllowingStateLoss
import kotlinx.android.parcel.Parcelize

class UrlImportActivity : AppCompatActivity() {
    @Parcelize
    data class ProfilesArg(val profiles: List<Profile>) : Parcelable
    class ImportProfilesDialogFragment : AlertDialogFragment<ProfilesArg, Empty>() {
        override fun AlertDialog.Builder.prepare(listener: DialogInterface.OnClickListener) {
            setTitle(R.string.add_profile_dialog)
            setPositiveButton(R.string.yes, listener)
            setNegativeButton(R.string.no, listener)
            setMessage(arg.profiles.joinToString("\n"))
        }

        override fun onClick(dialog: DialogInterface?, which: Int) {
            if (which == DialogInterface.BUTTON_POSITIVE) arg.profiles.forEach { ProfileManager.createProfile(it) }
            requireActivity().finish()
        }

        override fun onDismiss(dialog: DialogInterface) {
            requireActivity().finish()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        when (val dialog = handleShareIntent()) {
            null -> {
                Toast.makeText(this, R.string.profile_invalid_input, Toast.LENGTH_SHORT).show()
                finish()
            }
            else -> dialog.showAllowingStateLoss(supportFragmentManager)
        }
    }

    private fun handleShareIntent() = intent.data?.toString()?.let { sharedStr ->
        val profiles = Profile.findAllUrls(sharedStr, Core.currentProfile?.first).toList()
        if (profiles.isEmpty()) null else ImportProfilesDialogFragment().withArg(ProfilesArg(profiles))
    }
}

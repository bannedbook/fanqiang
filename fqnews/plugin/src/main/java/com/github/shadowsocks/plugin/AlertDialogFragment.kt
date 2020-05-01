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

package com.github.shadowsocks.plugin

import android.app.Activity
import android.content.DialogInterface
import android.content.Intent
import android.os.Bundle
import android.os.Parcelable
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatDialogFragment
import androidx.fragment.app.Fragment

/**
 * Based on: https://android.googlesource.com/platform/packages/apps/ExactCalculator/+/8c43f06/src/com/android/calculator2/AlertDialogFragment.java
 */
abstract class AlertDialogFragment<Arg : Parcelable, Ret : Parcelable> :
        AppCompatDialogFragment(), DialogInterface.OnClickListener {
    companion object {
        private const val KEY_ARG = "arg"
        private const val KEY_RET = "ret"
        fun <T : Parcelable> getRet(data: Intent) = data.extras!!.getParcelable<T>(KEY_RET)!!
    }
    protected abstract fun AlertDialog.Builder.prepare(listener: DialogInterface.OnClickListener)

    protected val arg by lazy { arguments!!.getParcelable<Arg>(KEY_ARG)!! }
    protected open fun ret(which: Int): Ret? = null
    fun withArg(arg: Arg) = apply { arguments = Bundle().apply { putParcelable(KEY_ARG, arg) } }

    override fun onCreateDialog(savedInstanceState: Bundle?): AlertDialog =
            AlertDialog.Builder(requireContext()).also { it.prepare(this) }.create()

    override fun onClick(dialog: DialogInterface?, which: Int) {
        targetFragment?.onActivityResult(targetRequestCode, which, ret(which)?.let {
            Intent().replaceExtras(Bundle().apply { putParcelable(KEY_RET, it) })
        })
    }

    override fun onDismiss(dialog: DialogInterface) {
        super.onDismiss(dialog)
        onClick(dialog, Activity.RESULT_CANCELED)
    }

    fun show(target: Fragment, requestCode: Int = 0, tag: String = javaClass.simpleName) {
        setTargetFragment(target, requestCode)
        showAllowingStateLoss(target.fragmentManager ?: return, tag)
    }
}

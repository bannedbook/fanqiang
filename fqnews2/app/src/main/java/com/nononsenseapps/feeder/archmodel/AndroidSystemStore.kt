package com.nononsenseapps.feeder.archmodel

import android.app.Application
import android.content.Context
import android.content.pm.ShortcutManager
import android.os.Build
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.instance

/**
 * A store to serve as a wrapper between the repository and system services.
 *
 * Useful because otherwise repository needs to run instrumented tests.
 */
class AndroidSystemStore(override val di: DI) : DIAware {
    private val application: Application by instance()

    fun removeDynamicShortcuts(feedIds: List<Long>) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N_MR1) {
            val shortcutManager = application.getSystemService(Context.SHORTCUT_SERVICE) as ShortcutManager
            shortcutManager.removeDynamicShortcuts(feedIds.map { it.toString() })
        }
    }
}

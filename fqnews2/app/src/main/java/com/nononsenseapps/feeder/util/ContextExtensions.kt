package com.nononsenseapps.feeder.util

import android.content.Context
import android.content.Intent
import android.content.pm.ShortcutInfo
import android.content.pm.ShortcutManager
import android.graphics.drawable.Icon
import android.os.Build
import android.util.Log
import androidx.annotation.StringRes
import androidx.core.app.NotificationManagerCompat
import androidx.core.net.toUri
import androidx.core.text.BidiFormatter
import com.nononsenseapps.feeder.ui.MainActivity
import java.util.Locale

interface ToastMaker {
    suspend fun makeToast(text: String)

    suspend fun makeToast(
        @StringRes resId: Int,
    )
}

val Context.notificationManager: NotificationManagerCompat
    get() = NotificationManagerCompat.from(this)

/**
 * If feed already has a shortcut then it is updated and bumped to the top of the list.
 * Ensures that a maximum number of shortcuts is available at any time with the last used being bumped out of the list
 * first.
 */
fun Context.addDynamicShortcutToFeed(
    label: String,
    id: Long,
    icon: Icon? = null,
) {
    try {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N_MR1) {
            val shortcutManager = getSystemService(ShortcutManager::class.java) ?: return

            val intent =
                Intent(
                    Intent.ACTION_VIEW,
                    "$DEEP_LINK_BASE_URI/feed?id=$id".toUri(),
                    this,
                    MainActivity::class.java,
                ).apply {
                    addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                }

            val current = shortcutManager.dynamicShortcuts.toMutableList()

            // Update shortcuts
            val shortcut: ShortcutInfo =
                ShortcutInfo.Builder(this, "$id")
                    .setShortLabel(label)
                    .setLongLabel(label)
                    .setIcon(
                        icon
                            ?: Icon.createWithBitmap(
                                getLetterIcon(
                                    label,
                                    id,
                                    radius = shortcutManager.iconMaxHeight,
                                ),
                            ),
                    )
                    .setIntent(intent)
                    .setDisabledMessage("Feed deleted")
                    .setRank(0)
                    .build()

            if (current.map { it.id }.contains(shortcut.id)) {
                // Just update existing one
                shortcutManager.updateShortcuts(listOf(shortcut))
            } else {
                // Ensure we do not exceed max limits
                if (current.size >= shortcutManager.maxShortcutCountPerActivity.coerceAtMost(3)) {
                    current.sortBy { it.rank }
                    current.lastOrNull()?.let {
                        shortcutManager.removeDynamicShortcuts(listOf(it.id))
                    }
                }

                // It's new!
                shortcutManager.addDynamicShortcuts(listOf(shortcut))
            }
        }
    } catch (error: Throwable) {
        Log.d("FeederDynamicShortcut", "Error during add of shortcut: ${error.message}")
    }
}

/**
 * Typically, launcher apps use this information to build a prediction model so that they can promote the shortcuts
 * that are likely to be used at the moment.
 */
fun Context.reportShortcutToFeedUsed(id: Any) {
    try {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N_MR1) {
            val shortcutManager = getSystemService(ShortcutManager::class.java) ?: return
            shortcutManager.reportShortcutUsed("$id")
        }
    } catch (error: Throwable) {
        Log.d("FeederDynamicShortcut", "Error during report use of shortcut: ${error.message}")
    }
}

fun Context.unicodeWrap(text: String): String = BidiFormatter.getInstance(getLocale()).unicodeWrap(text)

fun Context.getLocale(): Locale =
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
        resources.configuration.locales[0]
    } else {
        @Suppress("DEPRECATION")
        resources.configuration.locale
    }

const val DEEP_LINK_HOST = "feederapp.nononsenseapps.com"
const val DEEP_LINK_BASE_URI = "https://$DEEP_LINK_HOST"

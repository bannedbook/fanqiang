package com.nononsenseapps.feeder.util

import android.app.Activity
import android.content.ActivityNotFoundException
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.provider.Browser
import android.util.Log
import android.widget.Toast
import androidx.annotation.ColorInt
import androidx.browser.customtabs.CustomTabsIntent
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.archmodel.LinkOpener
import com.nononsenseapps.feeder.archmodel.Repository

class ActivityLauncher(
    private val activity: Activity,
    private val repository: Repository,
) {
    private val configuration by lazy { activity.resources.configuration }

    private fun Intent.openAdjacentIfSuitable(openAdjacentIfSuitable: Boolean): Intent {
        return if (openAdjacentIfSuitable &&
            Build.VERSION.SDK_INT >= Build.VERSION_CODES.N &&
            configuration.smallestScreenWidthDp >= 600 &&
            repository.isOpenAdjacent.value
        ) {
            addFlags(
                Intent.FLAG_ACTIVITY_LAUNCH_ADJACENT or Intent.FLAG_ACTIVITY_NEW_TASK,
            )
        } else {
            this
        }
    }

    /**
     * Returns true if activity was launched, false if no such activity
     */
    fun startActivity(
        openAdjacentIfSuitable: Boolean,
        intent: Intent,
    ): Boolean {
        return try {
            activity.startActivity(intent.openAdjacentIfSuitable(openAdjacentIfSuitable))
            true
        } catch (e: ActivityNotFoundException) {
            Log.e(LOG_TAG, "Failed to launch", e)
            Toast.makeText(activity, R.string.no_activity_for_link, Toast.LENGTH_SHORT).show()
            false
        }
    }

    /**
     * Returns true if activity was launched, false if no such activity
     *
     * Launches in browser or custom tab depending on settings
     */
    fun openLink(
        link: String,
        @ColorInt toolbarColor: Int,
        openAdjacentIfSuitable: Boolean = true,
    ): Boolean {
        if (link.isBlank()) {
            return false
        }
        return if (link.startsWith("mailto:")) {
            openEmailClient(link)
        } else {
            when (repository.linkOpener.value) {
                LinkOpener.CUSTOM_TAB -> openLinkInCustomTab(link, toolbarColor, openAdjacentIfSuitable)
                LinkOpener.DEFAULT_BROWSER -> openLinkInBrowser(link, openAdjacentIfSuitable)
            }
        }
    }

    private fun openEmailClient(link: String): Boolean {
        // example link: mailto:email@exampl.com?subject=subject
        val intent = Intent(Intent.ACTION_SENDTO, Uri.parse(link))

        return startActivity(openAdjacentIfSuitable = true, intent = intent)
    }

    /**
     * Returns true if activity was launched, false if no such activity
     */
    fun openLinkInBrowser(
        link: String,
        openAdjacentIfSuitable: Boolean = true,
    ): Boolean {
        return startActivity(
            openAdjacentIfSuitable = openAdjacentIfSuitable,
            intent =
                Intent(Intent.ACTION_VIEW, Uri.parse(link)).also {
                    it.putExtra(Browser.EXTRA_CREATE_NEW_TAB, true)
                },
        )
    }

    /**
     * Returns true if activity was launched, false if no such activity
     */
    fun openLinkInCustomTab(
        link: String,
        @ColorInt toolbarColor: Int,
        openAdjacentIfSuitable: Boolean = true,
    ): Boolean {
        return try {
            val uri = Uri.parse(link)
            val intent =
                CustomTabsIntent.Builder().apply {
                    setToolbarColor(toolbarColor)
                    addDefaultShareMenuItem()
                }.build().intent.apply {
                    data = uri
                }
            return startActivity(openAdjacentIfSuitable, intent)
        } catch (e: ActivityNotFoundException) {
            Log.e(LOG_TAG, "Failed to custom tab", e)
            Toast.makeText(activity, R.string.no_activity_for_link, Toast.LENGTH_SHORT).show()
            false
        } catch (e: Exception) {
            Log.e(LOG_TAG, "Maybe bad link?", e)
            false
        }
    }

    companion object {
        private const val LOG_TAG = "FEEDER_ALAUNCH"
    }
}

package com.nononsenseapps.feeder.ui

import android.content.Intent
import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.lifecycle.lifecycleScope
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.base.DIAwareComponentActivity
import com.nononsenseapps.feeder.db.COL_LINK
import com.nononsenseapps.feeder.db.room.ID_UNSET
import com.nononsenseapps.feeder.model.cancelNotification
import com.nononsenseapps.feeder.util.ActivityLauncher
import com.nononsenseapps.feeder.util.DEEP_LINK_HOST
import kotlinx.coroutines.launch
import org.kodein.di.instance

/**
 * Proxy activity to mark item as read and notified in database as well as cancelling the
 * notification before performing a notification action such as opening in the browser.
 *
 * If link is null, then item is only marked as read and notified.
 */
class OpenLinkInDefaultActivity : DIAwareComponentActivity() {
    private val viewModel: OpenLinkInDefaultActivityViewModel by instance(arg = this)
    private val activityLauncher: ActivityLauncher by instance()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        installExceptionHandler()

        intent?.let { intent ->
            val uri = intent.data

            if (uri?.host == DEEP_LINK_HOST && uri.lastPathSegment == "feed") {
                val feedItemIds =
                    intent.getLongArrayExtra(EXTRA_FEEDITEMS_TO_MARK_AS_NOTIFIED)
                        ?: longArrayOf()

                viewModel.markAsNotifiedInBackground(feedItemIds.toList())

                activityLauncher.startActivity(
                    openAdjacentIfSuitable = false,
                    intent =
                        Intent(
                            Intent.ACTION_VIEW,
                            uri,
                            this,
                            MainActivity::class.java,
                        ),
                )
            } else {
                handleNotificationActions(intent)
            }
        }

        // Terminate activity immediately
        finish()
    }

    private fun handleNotificationActions(intent: Intent) {
        val id: Long = intent.data?.lastPathSegment?.toLong() ?: ID_UNSET
        val link: String? = intent.data?.getQueryParameter(COL_LINK)

        lifecycleScope.launch {
            viewModel.markAsReadAndNotified(id)
            cancelNotification(this@OpenLinkInDefaultActivity, id)
        }

        if (link != null) {
            try {
                activityLauncher.openLinkInBrowser(
                    link,
                    openAdjacentIfSuitable = false,
                )
            } catch (e: Throwable) {
                e.printStackTrace()
                Toast.makeText(this, R.string.no_activity_for_link, Toast.LENGTH_SHORT).show()
                Log.e("FEEDEROpenInWebBrowser", "Failed to start browser", e)
            }
        }
    }
}

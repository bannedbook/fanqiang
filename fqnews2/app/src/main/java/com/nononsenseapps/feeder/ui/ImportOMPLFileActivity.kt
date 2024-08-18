package com.nononsenseapps.feeder.ui

import android.content.Intent
import android.os.Bundle
import androidx.activity.compose.setContent
import androidx.compose.animation.ExperimentalAnimationApi
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.core.net.toUri
import androidx.core.view.WindowCompat
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.nononsenseapps.feeder.base.DIAwareComponentActivity
import com.nononsenseapps.feeder.db.room.ID_ALL_FEEDS
import com.nononsenseapps.feeder.ui.compose.ompl.OpmlImportScreen
import com.nononsenseapps.feeder.ui.compose.utils.withAllProviders
import com.nononsenseapps.feeder.util.DEEP_LINK_BASE_URI
import com.nononsenseapps.feeder.util.logDebug

/**
 * This activity should only be started via a Open File Intent.
 */
class ImportOMPLFileActivity : DIAwareComponentActivity() {
    @OptIn(ExperimentalAnimationApi::class)
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        installExceptionHandler()

        WindowCompat.setDecorFitsSystemWindows(window, false)

        val uri = intent.data
        logDebug(LOG_TAG, "Uri: $uri")

        setContent {
            withAllProviders {
                val navController = rememberNavController()
                NavHost(navController, startDestination = "import") {
                    composable(
                        "import",
                        enterTransition = { fadeIn() },
                        exitTransition = { fadeOut() },
                        popEnterTransition = { fadeIn() },
                        popExitTransition = { fadeOut() },
                    ) {
                        OpmlImportScreen(
                            onNavigateUp = {
                                onNavigateUpFromIntentActivities()
                            },
                            uri = uri,
                            onDismiss = {
                                finish()
                            },
                        ) {
                            val deepLinkUri =
                                "$DEEP_LINK_BASE_URI/feed?id=$ID_ALL_FEEDS"

                            val intent =
                                Intent(
                                    Intent.ACTION_VIEW,
                                    deepLinkUri.toUri(),
                                    this@ImportOMPLFileActivity,
                                    MainActivity::class.java,
                                ).apply {
                                    addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                                }

                            startActivity(intent)
                            finish()
                        }
                    }
                }
            }
        }
    }

    companion object {
        private const val LOG_TAG = "FEEDER_OPMLIMPORT"
    }
}

package com.nononsenseapps.feeder.ui

import android.content.Intent
import android.net.Uri
import android.os.Bundle
import androidx.activity.compose.setContent
import androidx.core.view.WindowCompat
import com.nononsenseapps.feeder.base.DIAwareComponentActivity
import com.nononsenseapps.feeder.base.diAwareViewModel
import com.nononsenseapps.feeder.ui.compose.navigation.SyncScreenDestination
import com.nononsenseapps.feeder.ui.compose.settings.SettingsScreen
import com.nononsenseapps.feeder.ui.compose.utils.withAllProviders

/**
 * Should only be opened from the MANAGE SETTINGS INTENT
 */
class ManageSettingsActivity : DIAwareComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        installExceptionHandler()

        WindowCompat.setDecorFitsSystemWindows(window, false)

        setContent {
            withAllProviders {
                SettingsScreen(
                    onNavigateUp = {
                        onNavigateUpFromIntentActivities()
                    },
                    onNavigateToSyncScreen = {
                        startActivity(
                            Intent(
                                Intent.ACTION_VIEW,
                                Uri.parse(SyncScreenDestination.deepLinks.first().uriPattern),
                                this,
                                MainActivity::class.java,
                            ),
                        )
                        finish()
                    },
                    settingsViewModel = diAwareViewModel(),
                )
            }
        }
    }
}

package com.nononsenseapps.feeder.ui.compose

import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.rememberNavController
import com.nononsenseapps.feeder.ui.MainActivity
import com.nononsenseapps.feeder.ui.compose.navigation.SyncScreenDestination
import com.nononsenseapps.feeder.ui.compose.theme.FeederTheme
import com.nononsenseapps.feeder.ui.robots.feedScreen
import org.junit.Before
import org.junit.Ignore
import org.junit.Rule
import org.junit.Test
import org.kodein.di.compose.withDI

@Ignore
class SyncSetupTest : BaseComposeTest {
    @get:Rule
    override val composeTestRule = createAndroidComposeRule<MainActivity>()

    @Before
    fun setup() {
        composeTestRule.setContent {
            FeederTheme {
                withDI {
                    val navController = rememberNavController()
                    val navDrawerListState = rememberLazyListState()

                    NavHost(navController, startDestination = SyncScreenDestination.route) {
                        SyncScreenDestination.register(this, navController, navDrawerListState)
                    }
                }
            }
        }
    }

    @Test
    fun addFeed() {
        feedScreen {
        } openOverflowMenu {
        } pressAddFeed {
            assertSearchButtonIsNotEnabled()
            enterText("cowboyprogrammer.org")
            assertSearchButtonIsEnabled()
            pressSearchButton()
        } pressFirstResult {
            scrollToBottom()
        } pressOKButton {
            assertAppBarTitleIs("Cowboy Programmer")
        }
    }
}

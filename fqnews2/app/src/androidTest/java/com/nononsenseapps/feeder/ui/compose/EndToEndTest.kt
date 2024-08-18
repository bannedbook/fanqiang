package com.nononsenseapps.feeder.ui.compose

import androidx.compose.ui.test.junit4.createAndroidComposeRule
import com.nononsenseapps.feeder.ui.MainActivity
import com.nononsenseapps.feeder.ui.compose.theme.FeederTheme
import com.nononsenseapps.feeder.ui.robots.feedScreen
import org.junit.Before
import org.junit.Ignore
import org.junit.Rule
import org.junit.Test
import org.kodein.di.compose.withDI

@Ignore
class EndToEndTest : BaseComposeTest {
    @get:Rule
    override val composeTestRule = createAndroidComposeRule<MainActivity>()

    @Before
    fun setup() {
        composeTestRule.setContent {
            FeederTheme {
                withDI {
                    composeTestRule.activity.AppContent()
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

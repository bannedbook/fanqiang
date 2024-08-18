package com.nononsenseapps.feeder.ui.robots

import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.junit4.ComposeTestRule
import androidx.compose.ui.test.onNodeWithTag
import androidx.compose.ui.test.performClick

class FeedScreenMenuRobot(
    private val testRule: ComposeTestRule,
) {
    infix fun pressAddFeed(block: SearchFeedScreenRobot.() -> Unit): SearchFeedScreenRobot {
        testRule
            .onNodeWithTag("menuAddFeed", useUnmergedTree = true)
            .assertIsDisplayed()
            .performClick()

        return SearchFeedScreenRobot(testRule).apply { block() }
    }
}

package com.nononsenseapps.feeder.ui.robots

import androidx.compose.ui.test.assert
import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.hasText
import androidx.compose.ui.test.junit4.ComposeTestRule
import androidx.compose.ui.test.onNodeWithTag
import androidx.compose.ui.test.performClick
import com.nononsenseapps.feeder.ui.compose.BaseComposeTest

fun BaseComposeTest.feedScreen(block: FeedScreenRobot.() -> Unit) = FeedScreenRobot(composeTestRule).apply { block() }

class FeedScreenRobot(
    private val testRule: ComposeTestRule,
) : AndroidRobot() {
    fun assertAppBarTitleIs(title: String) {
        testRule.onNodeWithTag("appBarTitle")
            .assertIsDisplayed()
            .assert(hasText(title))
    }

    infix fun openOverflowMenu(block: FeedScreenMenuRobot.() -> Unit): FeedScreenMenuRobot {
        testRule.onNodeWithTag("menuButton")
            .assertIsDisplayed()
            .performClick()

        return FeedScreenMenuRobot(testRule).apply { block() }
    }
}

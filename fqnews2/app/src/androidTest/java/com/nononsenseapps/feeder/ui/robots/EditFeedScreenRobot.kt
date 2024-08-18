package com.nononsenseapps.feeder.ui.robots

import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.hasScrollAction
import androidx.compose.ui.test.junit4.ComposeTestRule
import androidx.compose.ui.test.onNodeWithText
import androidx.compose.ui.test.performClick
import androidx.compose.ui.test.performTouchInput
import androidx.compose.ui.test.swipeUp

class EditFeedScreenRobot(
    private val testRule: ComposeTestRule,
) : AndroidRobot() {
    fun scrollToBottom() {
        testRule
            .onNode(hasScrollAction())
            .performTouchInput {
                swipeUp()
            }
    }

    infix fun pressOKButton(block: FeedScreenRobot.() -> Unit): FeedScreenRobot {
        testRule
            .onNodeWithText("OK")
            .assertIsDisplayed()
            .performClick()

        return FeedScreenRobot(testRule).apply { block() }
    }
}

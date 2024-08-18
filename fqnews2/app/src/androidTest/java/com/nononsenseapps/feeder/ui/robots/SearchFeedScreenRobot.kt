package com.nononsenseapps.feeder.ui.robots

import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.assertIsEnabled
import androidx.compose.ui.test.assertIsNotEnabled
import androidx.compose.ui.test.junit4.ComposeTestRule
import androidx.compose.ui.test.onAllNodesWithTag
import androidx.compose.ui.test.onNodeWithTag
import androidx.compose.ui.test.onNodeWithText
import androidx.compose.ui.test.performClick
import androidx.compose.ui.test.performTextInput

class SearchFeedScreenRobot(
    private val testRule: ComposeTestRule,
) : AndroidRobot() {
    fun enterText(text: String) {
        testRule
            .onNodeWithTag("urlField")
            .assertIsDisplayed()
            .performTextInput(text)
    }

    fun assertSearchButtonIsNotEnabled() {
        testRule
            .onNodeWithText("Search")
            .assertIsDisplayed()
            .assertIsNotEnabled()
    }

    fun assertSearchButtonIsEnabled() {
        testRule
            .onNodeWithText("Search")
            .assertIsDisplayed()
            .assertIsEnabled()
    }

    fun pressSearchButton() {
        testRule
            .onNodeWithText("Search")
            .assertIsEnabled()
            .performClick()
    }

    infix fun pressFirstResult(block: EditFeedScreenRobot.() -> Unit): EditFeedScreenRobot {
        testRule.waitUntil(5_000) {
            try {
                testRule.onNodeWithTag("searchingIndicator")
                    .assertIsDisplayed()
                false
            } catch (_: AssertionError) {
                true
            }
        }

        testRule.onAllNodesWithTag("searchResult")[0]
            .performClick()

        return EditFeedScreenRobot(testRule).apply { block() }
    }
}

package com.nononsenseapps.feeder.ui.compose.navigation

import androidx.navigation.NavController
import io.mockk.MockKAnnotations
import io.mockk.impl.annotations.MockK
import io.mockk.verify
import org.junit.Before
import org.junit.Ignore
import org.junit.Test
import kotlin.test.assertEquals

@Ignore
class SearchFeedDestinationTest {
    @MockK
    private lateinit var navController: NavController

    @Before
    fun setup() {
        MockKAnnotations.init(this, relaxed = true, relaxUnitFun = true)
    }

    @Test
    fun searchFeedHasCorrectRoute() {
        assertEquals(
            "search/feed?feedUrl={feedUrl}",
            SearchFeedDestination.route,
        )
    }

    @Test
    fun searchFeedNavigateDefaults() {
        SearchFeedDestination.navigate(
            navController,
        )

        verify {
            navController.navigate("search/feed")
        }
    }

    @Test
    fun searchFeedNavigateFeed() {
        SearchFeedDestination.navigate(
            navController,
            "https://cowboyprogrammer.org",
        )

        verify {
            navController.navigate("search/feed?feedUrl=https%3A%2F%2Fcowboyprogrammer.org")
        }
    }
}

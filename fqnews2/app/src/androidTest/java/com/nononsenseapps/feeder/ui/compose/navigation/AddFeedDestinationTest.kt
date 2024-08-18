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
class AddFeedDestinationTest {
    @MockK
    private lateinit var navController: NavController

    @Before
    fun setup() {
        MockKAnnotations.init(this, relaxed = true, relaxUnitFun = true)
    }

    @Test
    fun addFeedHasCorrectRoute() {
        assertEquals(
            "add/feed/{feedUrl}?feedTitle={feedTitle}",
            AddFeedDestination.route,
        )
    }

    @Test
    fun addFeedNavigateNoTitle() {
        AddFeedDestination.navigate(
            navController,
            "https://cowboyprogrammer.org",
        )

        verify {
            navController.navigate("add/feed/https%3A%2F%2Fcowboyprogrammer.org")
        }
    }

    @Test
    fun addFeedNavigateWithEmptyTitle() {
        AddFeedDestination.navigate(
            navController,
            "https://cowboyprogrammer.org",
            "",
        )

        verify {
            navController.navigate("add/feed/https%3A%2F%2Fcowboyprogrammer.org")
        }
    }

    @Test
    fun addFeedNavigateWithBlankTitle() {
        AddFeedDestination.navigate(
            navController,
            "https://cowboyprogrammer.org",
            " ",
        )

        verify {
            navController.navigate("add/feed/https%3A%2F%2Fcowboyprogrammer.org?feedTitle=+")
        }
    }

    @Test
    fun addFeedNavigateWithTitle() {
        AddFeedDestination.navigate(
            navController,
            "https://cowboyprogrammer.org",
            "A feed",
        )

        verify {
            navController.navigate("add/feed/https%3A%2F%2Fcowboyprogrammer.org?feedTitle=A+feed")
        }
    }
}

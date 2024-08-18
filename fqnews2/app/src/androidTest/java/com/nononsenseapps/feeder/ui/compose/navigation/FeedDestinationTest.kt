package com.nononsenseapps.feeder.ui.compose.navigation

import androidx.navigation.NavController
import com.nononsenseapps.feeder.db.room.ID_ALL_FEEDS
import com.nononsenseapps.feeder.util.DEEP_LINK_BASE_URI
import io.mockk.MockKAnnotations
import io.mockk.impl.annotations.MockK
import io.mockk.verify
import org.junit.Before
import org.junit.Ignore
import org.junit.Test
import kotlin.test.assertEquals

@Ignore
class FeedDestinationTest {
    @MockK
    private lateinit var navController: NavController

    @Before
    fun setup() {
        MockKAnnotations.init(this, relaxed = true, relaxUnitFun = true)
    }

    @Test
    fun feedHasCorrectRoute() {
        assertEquals(
            "feed?id={id}&tag={tag}",
            FeedDestination.route,
        )
    }

    @Test
    fun feedHasCorrectDeeplinks() {
        assertEquals(
            listOf(
                "$DEEP_LINK_BASE_URI/feed?id={id}&tag={tag}",
            ),
            FeedDestination.deepLinks.map { it.uriPattern },
        )
    }

    @Test
    fun feedNavigateDefaults() {
        FeedDestination.navigate(
            navController,
        )

        verify {
            navController.navigate("feed")
        }
    }

    @Test
    fun feedNavigateId() {
        FeedDestination.navigate(
            navController,
            feedId = 6L,
        )

        verify {
            navController.navigate("feed?id=6")
        }
    }

    @Test
    fun feedNavigateTag() {
        FeedDestination.navigate(
            navController,
            tag = "foo bar+cop",
        )

        verify {
            navController.navigate("feed?tag=foo+bar%2Bcop")
        }
    }

    @Test
    fun feedNavigateIdAndTag() {
        FeedDestination.navigate(
            navController,
            feedId = ID_ALL_FEEDS,
            tag = "foo bar+cop",
        )

        verify {
            navController.navigate("feed?id=-10&tag=foo+bar%2Bcop")
        }
    }
}

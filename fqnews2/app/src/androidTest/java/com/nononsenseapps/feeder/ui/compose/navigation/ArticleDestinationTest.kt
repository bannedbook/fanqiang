package com.nononsenseapps.feeder.ui.compose.navigation

import androidx.navigation.NavController
import com.nononsenseapps.feeder.util.DEEP_LINK_BASE_URI
import io.mockk.MockKAnnotations
import io.mockk.impl.annotations.MockK
import io.mockk.verify
import org.junit.Before
import org.junit.Ignore
import org.junit.Test
import kotlin.test.assertEquals

@Ignore
class ArticleDestinationTest {
    @MockK
    private lateinit var navController: NavController

    @Before
    fun setup() {
        MockKAnnotations.init(this, relaxed = true, relaxUnitFun = true)
    }

    @Test
    fun readerHasCorrectRoute() {
        assertEquals(
            "reader/{itemId}",
            ArticleDestination.route,
        )
    }

    @Test
    fun readerHasCorrectDeeplinks() {
        assertEquals(
            listOf(
                "$DEEP_LINK_BASE_URI/article/{itemId}",
            ),
            ArticleDestination.deepLinks.map { it.uriPattern },
        )
    }

    @Test
    fun readerNavigate() {
        ArticleDestination.navigate(
            navController,
            55L,
        )

        verify {
            navController.navigate("reader/55")
        }
    }
}

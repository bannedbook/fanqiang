package com.nononsenseapps.feeder.ui.compose

import androidx.compose.ui.test.ExperimentalTestApi
import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.hasTestTag
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onNodeWithTag
import com.nononsenseapps.feeder.archmodel.FeedItemStyle
import com.nononsenseapps.feeder.archmodel.Repository
import com.nononsenseapps.feeder.archmodel.SyncFrequency
import com.nononsenseapps.feeder.db.room.Feed
import com.nononsenseapps.feeder.db.room.FeedItem
import com.nononsenseapps.feeder.model.ImageFromHTML
import com.nononsenseapps.feeder.ui.MainActivity
import kotlinx.coroutines.runBlocking
import org.junit.Rule
import org.junit.Test
import org.kodein.di.instance
import java.net.URL
import java.time.Instant
import java.time.ZoneOffset
import java.time.ZonedDateTime

class ThumbnailsAreDisplayedTest : BaseComposeTest {
    @get:Rule
    override val composeTestRule = createAndroidComposeRule<MainActivity>()

    @OptIn(ExperimentalTestApi::class)
    @Test
    fun thumbnailsAreShown() {
        val repository by (composeTestRule.activity).di.instance<Repository>()

        repository.setSyncFrequency(SyncFrequency.MANUAL)
        repository.setFeedItemStyle(FeedItemStyle.CARD)
        repository.setShowThumbnails(true)

        // Ensure we have feeds and items
        runBlocking {
            val feedId =
                repository.saveFeed(
                    Feed(
                        title = "Ampersands are & the worst",
                        url = URL("https://example.com/ampersands"),
                    ),
                )

            repository.setCurrentFeedAndTag(feedId, "")

            repository.upsertFeedItems(
                listOf(
                    FeedItem(
                        guid = "guid anime2you",
                        plainTitle = "Item with image",
                        plainSnippet = "Snippet with image",
                        feedId = feedId,
                        pubDate = ZonedDateTime.now(ZoneOffset.UTC),
                        primarySortTime = Instant.now(),
                        thumbnailImage =
                            ImageFromHTML(
                                url = "https://img.anime2you.de/2023/12/jujutsu-kaisen-6.jpg",
                                width = 700,
                                height = 350,
                            ),
                    ) to "",
                ),
            ) { _, _ -> }
        }

        composeTestRule.waitUntilExactlyOneExists(
            hasTestTag("feed_list"),
            5_000L,
        )

        composeTestRule.onNodeWithTag("card_image", useUnmergedTree = true)
            .assertIsDisplayed()
    }
}

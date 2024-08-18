package com.nononsenseapps.feeder.ui.compose

import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.test.ExperimentalTestApi
import androidx.compose.ui.test.click
import androidx.compose.ui.test.hasTestTag
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onNodeWithTag
import androidx.compose.ui.test.performTouchInput
import androidx.compose.ui.test.swipe
import com.nononsenseapps.feeder.archmodel.Repository
import com.nononsenseapps.feeder.archmodel.SyncFrequency
import com.nononsenseapps.feeder.db.room.Feed
import com.nononsenseapps.feeder.db.room.FeedItem
import com.nononsenseapps.feeder.ui.MainActivity
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withTimeout
import org.junit.Ignore
import org.junit.Rule
import org.junit.Test
import org.kodein.di.instance
import java.net.URL
import java.time.Instant
import java.time.ZoneOffset
import java.time.ZonedDateTime
import kotlin.test.assertEquals
import kotlin.test.assertTrue

@Ignore("Flaky")
class FeedScreenMarkAsReadOnScrollTest : BaseComposeTest {
    @get:Rule
    override val composeTestRule = createAndroidComposeRule<MainActivity>()

    @OptIn(ExperimentalTestApi::class)
    @Test
    fun scrollingWillMarkAsRead() {
        val repository by (composeTestRule.activity).di.instance<Repository>()

        repository.setIsMarkAsReadOnScroll(true)
        repository.setSyncFrequency(SyncFrequency.MANUAL)

        // Ensure we have feeds and items
        val feedId =
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
                    (1..50).map { i ->
                        FeedItem(
                            guid = "guid$i",
                            title = "Item $i",
                            feedId = feedId,
                            pubDate = ZonedDateTime.now(ZoneOffset.UTC).plusDays(i.toLong()),
                            primarySortTime = Instant.now().plusSeconds(i.toLong()),
                        ) to ""
                    },
                ) { _, _ -> }

                feedId
            }

        composeTestRule.waitUntilExactlyOneExists(
            hasTestTag("feed_list"),
            10_000L,
        )

        val unreadCountBefore: Int =
            runBlocking {
                repository.getUnreadCount(feedId).first()
            }

        assertTrue("There should be unread items before test: $unreadCountBefore") {
            unreadCountBefore > 0
        }

        composeTestRule.waitForIdle()

        composeTestRule
            .onNodeWithTag("feed_list")
            .performTouchInput {
                val start = Offset(centerX, top)
                val end = Offset(centerX, -2000f)
                swipe(start, end, durationMillis = 10_000)
            }

        composeTestRule.waitForIdle()

        runBlocking {
            withTimeout(5_000L) {
                while (repository.getUnreadCount(feedId).first() == unreadCountBefore) {
                    composeTestRule.waitForIdle()
                    delay(100L)
                }
            }
        }
    }

    @OptIn(ExperimentalTestApi::class)
    @Test
    fun markAsReadOnScrollOnlyHappensOnScroll() {
        val repository by (composeTestRule.activity).di.instance<Repository>()

        repository.setIsMarkAsReadOnScroll(true)
        repository.setSyncFrequency(SyncFrequency.MANUAL)

        // Ensure we have feeds and items
        val feedId =
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
                    (1..50).map { i ->
                        FeedItem(
                            guid = "guid$i",
                            title = "Item $i",
                            feedId = feedId,
                            pubDate = ZonedDateTime.now(ZoneOffset.UTC).plusDays(i.toLong()),
                            primarySortTime = Instant.now().plusSeconds(i.toLong()),
                        ) to ""
                    },
                ) { _, _ -> }

                feedId
            }

        composeTestRule.waitUntilExactlyOneExists(
            hasTestTag("feed_list"),
            10_000L,
        )

        val unreadCountBefore: Int =
            runBlocking {
                repository.getUnreadCount(feedId).first()
            }

        assertTrue("There should be unread items before test: $unreadCountBefore") {
            unreadCountBefore > 0
        }

        // Make sure items are on screen a while before click. We do that with a slow swipe to the side
        composeTestRule
            .onNodeWithTag("feed_list")
            .performTouchInput {
                val start = Offset(centerX, top)
                val end = Offset(-50f, top)
                swipe(start, end, durationMillis = 10_000)
            }

        composeTestRule.waitForIdle()

        // Click first item
        composeTestRule
            .onNodeWithTag("feed_list")
            .performTouchInput {
                click(Offset(centerX, top + 100f))
            }

        composeTestRule.waitUntilExactlyOneExists(
            hasTestTag("readerColumn"),
            10_000L,
        )

        composeTestRule.waitForIdle()

        runBlocking {
            // Delay to ensure background tasks - if any - complete
            delay(500L)
            assertEquals(
                unreadCountBefore - 1,
                repository.getUnreadCount(feedId).first(),
                "Should have marked as read on click but no more",
            )
        }
    }
}

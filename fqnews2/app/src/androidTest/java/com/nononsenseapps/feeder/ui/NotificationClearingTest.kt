package com.nononsenseapps.feeder.ui

import androidx.test.core.app.ApplicationProvider.getApplicationContext
import androidx.test.ext.junit.runners.AndroidJUnit4
import com.nononsenseapps.feeder.db.room.Feed
import com.nononsenseapps.feeder.db.room.FeedItem
import com.nononsenseapps.feeder.db.room.FeedItemWithFeed
import com.nononsenseapps.feeder.model.RssNotificationBroadcastReceiver
import com.nononsenseapps.feeder.model.getDeleteIntent
import com.nononsenseapps.feeder.model.notify
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withContext
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Ignore
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import java.net.URL

// This can be flaky
@RunWith(AndroidJUnit4::class)
@Ignore
class NotificationClearingTest {
    private val receiver: RssNotificationBroadcastReceiver = RssNotificationBroadcastReceiver()

    @get:Rule
    val testDb = TestDatabaseRule(getApplicationContext())

    @Test
    fun clearingNotificationMarksAsNotified() =
        runBlocking {
            val feedId =
                testDb.db.feedDao().insertFeed(
                    Feed(
                        title = "testFeed",
                        url = URL("http://testfeed"),
                        tag = "testTag",
                    ),
                )

            val item1Id =
                testDb.db.feedItemDao().insertFeedItem(
                    FeedItem(
                        feedId = feedId,
                        guid = "item1",
                        title = "item1",
                        notified = false,
                    ),
                )

            val di =
                getDeleteIntent(
                    getApplicationContext(),
                    FeedItemWithFeed(
                        id = item1Id,
                        feedId = feedId,
                        guid = "item1",
                        title = "item1",
                    ),
                )

            runBlocking {
                // Receiver runs on main thread
                withContext(Dispatchers.Main) {
                    receiver.onReceive(getApplicationContext(), di)
                }

                delay(50)

                val item = testDb.db.feedItemDao().loadFeedItem(guid = "item1", feedId = feedId)
                assertTrue(item!!.notified)
            }
        }

    @Test
    fun notifyWorksOnMainThread() =
        runBlocking {
            val feedId =
                testDb.db.feedDao().insertFeed(
                    Feed(
                        title = "testFeed",
                        url = URL("http://testfeed"),
                        tag = "testTag",
                    ),
                )

            testDb.db.feedItemDao().insertFeedItem(
                FeedItem(
                    feedId = feedId,
                    guid = "item1",
                    title = "item1",
                    notified = false,
                ),
            )

            runBlocking {
                // Try to notify on main thread
                withContext(Dispatchers.Main) {
                    notify(getApplicationContext())
                }

                delay(50)

                // Only care that the above call didn't crash because we ran on the main thread
                val item = testDb.db.feedItemDao().loadFeedItem(guid = "item1", feedId = feedId)
                assertFalse(item!!.notified)
            }
        }
}

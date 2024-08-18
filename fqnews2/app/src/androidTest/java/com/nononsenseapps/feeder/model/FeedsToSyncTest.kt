package com.nononsenseapps.feeder.model

import android.content.Context
import androidx.test.core.app.ApplicationProvider.getApplicationContext
import androidx.test.ext.junit.runners.AndroidJUnit4
import com.nononsenseapps.feeder.archmodel.FeedStore
import com.nononsenseapps.feeder.archmodel.Repository
import com.nononsenseapps.feeder.db.room.AppDatabase
import com.nononsenseapps.feeder.db.room.Feed
import com.nononsenseapps.feeder.db.room.FeedDao
import com.nononsenseapps.feeder.db.room.ID_UNSET
import com.nononsenseapps.feeder.ui.TestDatabaseRule
import com.nononsenseapps.feeder.util.minusMinutes
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertEquals
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.kodein.di.DIAware
import org.kodein.di.android.closestDI
import org.kodein.di.android.subDI
import org.kodein.di.bind
import org.kodein.di.instance
import org.kodein.di.singleton
import java.net.URL
import java.time.Instant

@RunWith(AndroidJUnit4::class)
class FeedsToSyncTest : DIAware {
    @get:Rule
    val testDb = TestDatabaseRule(getApplicationContext())

    override val di by subDI(closestDI(getApplicationContext() as Context)) {
        bind<AppDatabase>(overrides = true) with instance(testDb.db)
        bind<FeedDao>(overrides = true) with singleton { testDb.db.feedDao() }
        bind<FeedStore>(overrides = true) with singleton { FeedStore(di) }
        bind<Repository>(overrides = true) with singleton { Repository(di) }
        bind<RssLocalSync>(overrides = true) with singleton { RssLocalSync(di) }
    }

    private val rssLocalSync: RssLocalSync by instance()

    @Test
    fun returnsStaleFeed() =
        runBlocking {
            // with stale feed
            val feed = withFeed()

            // when
            val result = rssLocalSync.feedsToSync(feedId = feed.id, tag = "")

            // then
            assertEquals(listOf(feed), result)
        }

    @Test
    fun doesNotReturnFreshFeed() =
        runBlocking {
            val now = Instant.now()
            val feed = withFeed(lastSync = now.minusMinutes(1))

            // when
            val result =
                rssLocalSync.feedsToSync(
                    feedId = feed.id,
                    tag = "",
                    staleTime = now.minusMinutes(2).toEpochMilli(),
                )

            // then
            assertEquals(emptyList<Feed>(), result)
        }

    @Test
    fun returnsAllStaleFeeds() =
        runBlocking {
            val items =
                listOf(
                    withFeed(url = URL("http://one")),
                    withFeed(url = URL("http://two")),
                )

            val result = rssLocalSync.feedsToSync(feedId = ID_UNSET, tag = "")

            assertEquals(items, result)
        }

    @Test
    fun doesNotReturnAllFreshFeeds() =
        runBlocking {
            val now = Instant.now()
            val items =
                listOf(
                    withFeed(url = URL("http://one"), lastSync = now.minusMinutes(1)),
                    withFeed(url = URL("http://two"), lastSync = now.minusMinutes(3)),
                )

            val result =
                rssLocalSync.feedsToSync(
                    feedId = ID_UNSET,
                    tag = "",
                    staleTime = now.minusMinutes(2).toEpochMilli(),
                )

            assertEquals(listOf(items[1]), result)
        }

    @Test
    fun returnsTaggedStaleFeeds() =
        runBlocking {
            val items =
                listOf(
                    withFeed(url = URL("http://one"), tag = "tag"),
                    withFeed(url = URL("http://two"), tag = "tag"),
                )

            val result = rssLocalSync.feedsToSync(feedId = ID_UNSET, tag = "")

            assertEquals(items, result)
        }

    @Test
    fun doesNotReturnTaggedFreshFeeds() =
        runBlocking {
            val now = Instant.now()
            val items =
                listOf(
                    withFeed(url = URL("http://one"), lastSync = now.minusMinutes(1), tag = "tag"),
                    withFeed(url = URL("http://two"), lastSync = now.minusMinutes(3), tag = "tag"),
                )

            val result =
                rssLocalSync.feedsToSync(
                    feedId = ID_UNSET,
                    tag = "tag",
                    staleTime = now.minusMinutes(2).toEpochMilli(),
                )

            assertEquals(listOf(items[1]), result)
        }

    private suspend fun withFeed(
        lastSync: Instant = Instant.ofEpochMilli(0),
        url: URL = URL("http://url"),
        tag: String = "",
    ): Feed {
        val feed =
            Feed(
                lastSync = lastSync,
                url = url,
                tag = tag,
            )

        val id = testDb.db.feedDao().insertFeed(feed)

        return feed.copy(id = id)
    }
}

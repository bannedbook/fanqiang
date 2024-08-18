package com.nononsenseapps.feeder.model

import android.content.Context
import android.content.SharedPreferences
import androidx.test.core.app.ApplicationProvider.getApplicationContext
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.MediumTest
import androidx.work.WorkManager
import com.nononsenseapps.feeder.ApplicationCoroutineScope
import jww.app.FeederApplication
import com.nononsenseapps.feeder.archmodel.FeedItemStore
import com.nononsenseapps.feeder.archmodel.FeedStore
import com.nononsenseapps.feeder.archmodel.Repository
import com.nononsenseapps.feeder.archmodel.SessionStore
import com.nononsenseapps.feeder.archmodel.SettingsStore
import com.nononsenseapps.feeder.archmodel.SyncRemoteStore
import com.nononsenseapps.feeder.db.room.AppDatabase
import com.nononsenseapps.feeder.db.room.BlocklistDao
import com.nononsenseapps.feeder.db.room.Feed
import com.nononsenseapps.feeder.db.room.FeedDao
import com.nononsenseapps.feeder.db.room.FeedItemDao
import com.nononsenseapps.feeder.db.room.ID_UNSET
import com.nononsenseapps.feeder.db.room.ReadStatusSyncedDao
import com.nononsenseapps.feeder.db.room.RemoteReadMarkDao
import com.nononsenseapps.feeder.db.room.SyncRemoteDao
import com.nononsenseapps.feeder.di.networkModule
import com.nononsenseapps.feeder.model.Feeds.Companion.RSS_WITH_DUPLICATE_GUIDS
import com.nononsenseapps.feeder.model.Feeds.Companion.cowboyAtom
import com.nononsenseapps.feeder.model.Feeds.Companion.cowboyJson
import com.nononsenseapps.feeder.model.Feeds.Companion.nixosRss
import com.nononsenseapps.feeder.ui.TestDatabaseRule
import com.nononsenseapps.feeder.util.DoNotUseInProd
import com.nononsenseapps.feeder.util.FilePathProvider
import com.nononsenseapps.feeder.util.filePathProvider
import com.nononsenseapps.feeder.util.minusMinutes
import com.nononsenseapps.jsonfeed.cachingHttpClient
import kotlinx.coroutines.runBlocking
import okhttp3.OkHttpClient
import okhttp3.mockwebserver.Dispatcher
import okhttp3.mockwebserver.MockResponse
import okhttp3.mockwebserver.MockWebServer
import okhttp3.mockwebserver.RecordedRequest
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Ignore
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.bind
import org.kodein.di.instance
import org.kodein.di.singleton
import java.net.URL
import java.time.Instant
import java.util.concurrent.TimeUnit
import kotlin.test.assertTrue

@OptIn(DoNotUseInProd::class)
@RunWith(AndroidJUnit4::class)
@MediumTest
class RssLocalSyncKtTest : DIAware {
    @get:Rule
    val testDb = TestDatabaseRule(getApplicationContext())

    override val di by DI.lazy {
        bind<AppDatabase>() with instance(testDb.db)
        bind<FeedDao>() with singleton { testDb.db.feedDao() }
        bind<FeedItemDao>() with singleton { testDb.db.feedItemDao() }
        bind<BlocklistDao>() with singleton { testDb.db.blocklistDao() }
        bind<RemoteReadMarkDao>() with singleton { testDb.db.remoteReadMarkDao() }
        bind<ReadStatusSyncedDao>() with singleton { testDb.db.readStatusSyncedDao() }
        bind<SyncRemoteDao>() with singleton { testDb.db.syncRemoteDao() }
        bind<FeedStore>() with singleton { FeedStore(di) }
        bind<FeedItemStore>() with singleton { FeedItemStore(di) }
        bind<SettingsStore>() with
            singleton {
                SettingsStore(di).also {
                    it.setAddedFeederNews(true)
                }
            }
        bind<SessionStore>() with singleton { SessionStore() }
        bind<SyncRemoteStore>() with singleton { SyncRemoteStore(di) }
        bind<OkHttpClient>() with singleton { cachingHttpClient() }
        import(networkModule)
        bind<WorkManager>() with singleton { WorkManager.getInstance(getApplicationContext()) }
        bind<SharedPreferences>() with singleton { getApplicationContext<FeederApplication>().getSharedPreferences("test", Context.MODE_PRIVATE) }
        bind<ApplicationCoroutineScope>() with singleton { ApplicationCoroutineScope() }
        bind<Repository>() with singleton { Repository(di) }
        bind<FilePathProvider>() with
            singleton {
                filePathProvider(
                    cacheDir = getApplicationContext<FeederApplication>().cacheDir,
                    filesDir = getApplicationContext<FeederApplication>().filesDir,
                )
            }
    }

    private val server = MockWebServer()

    val responses = mutableMapOf<URL?, MockResponse>()

    private val rssLocalSync: RssLocalSync by instance()

    @After
    fun stopServer() {
        server.shutdown()
    }

    @Before
    fun setup() {
        server.start()
    }

    private suspend fun insertFeed(
        title: String,
        url: URL,
        raw: String,
        isJson: Boolean = true,
        useAlternateId: Boolean = false,
        skipDuplicates: Boolean = false,
    ): Long {
        val id =
            testDb.db.feedDao().insertFeed(
                Feed(
                    title = title,
                    url = url,
                    tag = "",
                    alternateId = useAlternateId,
                    skipDuplicates = skipDuplicates,
                ),
            )

        server.dispatcher =
            object : Dispatcher() {
                override fun dispatch(request: RecordedRequest): MockResponse {
                    return responses.getOrDefault(
                        request.requestUrl?.toUrl(),
                        MockResponse().setResponseCode(404),
                    )
                }
            }

        responses[url] =
            MockResponse().apply {
                setResponseCode(200)
                if (isJson) {
                    setHeader("Content-Type", "application/json")
                } else {
                    setHeader("Content-Type", "application/xml")
                }
                setBody(raw)
            }

        return id
    }

    @Test
    fun syncCowboyJsonWorks() =
        runBlocking {
            val cowboyJsonId =
                insertFeed(
                    "cowboyjson",
                    server.url("/feed.json").toUrl(),
                    cowboyJson,
                )

            runBlocking {
                assertTrue("Should have synced OK") {
                    rssLocalSync.syncFeeds(
                        feedId = cowboyJsonId,
                    )
                }
            }

            assertEquals(
                "Unexpected number of items in feed",
                10,
                testDb.db.feedItemDao().loadFeedItemsInFeedDesc(cowboyJsonId).size,
            )
        }

    @Test
    fun syncCowboyAtomWorks() =
        runBlocking {
            val cowboyAtomId =
                insertFeed(
                    "cowboyatom",
                    server.url("/atom.xml").toUrl(),
                    cowboyAtom,
                    isJson = false,
                )

            runBlocking {
                rssLocalSync.syncFeeds(
                    feedId = cowboyAtomId,
                )
            }

            assertEquals(
                "Unexpected number of items in feed",
                15,
                testDb.db.feedItemDao().loadFeedItemsInFeedDesc(cowboyAtomId).size,
            )
        }

    @Test
    fun alternateIdMitigatesRssFeedsWithNonUniqueGuids() =
        runBlocking {
            val duplicateIdRss =
                insertFeed(
                    "aussieWeather",
                    server.url("/IDZ00059.warnings_vic.xml").toUrl(),
                    RSS_WITH_DUPLICATE_GUIDS,
                    isJson = false,
                    useAlternateId = true,
                )

            runBlocking {
                rssLocalSync.syncFeeds(
                    feedId = duplicateIdRss,
                )
            }

            assertEquals(
                "Expected duplicate guids to be mitigated by alternate id",
                13,
                testDb.db.feedItemDao().loadFeedItemsInFeedDesc(duplicateIdRss).size,
            )
        }

    @Test
    fun syncAllWorks() =
        runBlocking {
            val cowboyJsonId =
                insertFeed(
                    "cowboyjson",
                    server.url("/feed.json").toUrl(),
                    cowboyJson,
                )
            val cowboyAtomId =
                insertFeed(
                    "cowboyatom",
                    server.url("/atom.xml").toUrl(),
                    cowboyAtom,
                    isJson = false,
                )

            runBlocking {
                rssLocalSync.syncFeeds(
                    feedId = ID_UNSET,
                )
            }

            assertEquals(
                "Unexpected number of items in feed",
                10,
                testDb.db.feedItemDao().loadFeedItemsInFeedDesc(cowboyJsonId).size,
            )

            assertEquals(
                "Unexpected number of items in feed",
                15,
                testDb.db.feedItemDao().loadFeedItemsInFeedDesc(cowboyAtomId).size,
            )
        }

    @Test
    fun responsesAreNotParsedUnlessFeedHashHasChanged() =
        runBlocking {
            val cowboyJsonId =
                insertFeed(
                    "cowboyjson",
                    server.url("/feed.json").toUrl(),
                    cowboyJson,
                )

            runBlocking {
                rssLocalSync.syncFeeds(feedId = cowboyJsonId, forceNetwork = true)
                testDb.db.feedDao().getFeed(cowboyJsonId)!!.let { feed ->
                    assertTrue("Feed should have been synced", feed.lastSync.toEpochMilli() > 0)
//                    assertTrue("Feed should have a valid response hash", feed.responseHash > 0)
                    // "Long time" ago, but not unset
                    testDb.db.feedDao().updateFeed(feed.copy(lastSync = Instant.ofEpochMilli(999L)))
                }
                rssLocalSync.syncFeeds(feedId = cowboyJsonId, forceNetwork = true)
            }

            assertEquals("Feed should have been fetched twice", 2, server.requestCount)

            assertNotEquals(
                "Cached response should still have updated feed last sync",
                999L,
                testDb.db.feedDao().getFeed(cowboyJsonId)!!.lastSync.toEpochMilli(),
            )
        }

    @Test
    fun feedsSyncedWithin15MinAreIgnored() =
        runBlocking {
            val cowboyJsonId =
                insertFeed(
                    "cowboyjson",
                    server.url("/feed.json").toUrl(),
                    cowboyJson,
                )

            val fourteenMinsAgo = Instant.now().minusMinutes(14)
            runBlocking {
                rssLocalSync.syncFeeds(feedId = cowboyJsonId, forceNetwork = true)
                testDb.db.feedDao().getFeed(cowboyJsonId)!!.let { feed ->
                    assertTrue("Feed should have been synced", feed.lastSync.toEpochMilli() > 0)
//                    assertTrue("Feed should have a valid response hash", feed.responseHash > 0)

                    testDb.db.feedDao().updateFeed(feed.copy(lastSync = fourteenMinsAgo))
                }
                rssLocalSync.syncFeeds(
                    feedId = cowboyJsonId,
                    forceNetwork = false,
                    minFeedAgeMinutes = 15,
                )
            }

            assertEquals(
                "Recently synced feed should not get a second network request",
                1,
                server.requestCount,
            )

            assertEquals(
                "Last sync should not have changed",
                fourteenMinsAgo,
                testDb.db.feedDao().getFeed(cowboyJsonId)!!.lastSync,
            )
        }

    @Test
    fun feedsGetRetryAfterSetAndWillNotSyncLater() =
        runBlocking {
            val cowboyJsonId =
                testDb.db.feedDao().insertFeed(
                    Feed(
                        title = "feed1",
                        url = server.url("/feed.json").toUrl(),
                        tag = "",
                    ),
                )

            val someOtherFeedId =
                testDb.db.feedDao().insertFeed(
                    Feed(
                        title = "feed2",
                        url = server.url("/feed2.json").toUrl(),
                        tag = "",
                    ),
                )

            val response =
                MockResponse().also {
                    it.setResponseCode(429)
                    it.setHeader("Retry-After", "30")
                }
            server.enqueue(response)

            rssLocalSync.syncFeeds(feedId = cowboyJsonId)
            assertEquals("Request should have been sent ", 1, server.requestCount)

            // Get the feed and check value
            val feed = testDb.db.feedDao().getFeed(cowboyJsonId)!!
            assertTrue("Feed should have a retry after value in the future") {
                feed.retryAfter > Instant.now()
            }

            val feed2 = testDb.db.feedDao().getFeed(someOtherFeedId)!!
            assertTrue("Feed should have a retry after value in the future") {
                feed2.retryAfter > Instant.now()
            }

            rssLocalSync.syncFeeds(feedId = cowboyJsonId)
            assertEquals("Request should not have been sent due to retry-after", 1, server.requestCount)

            rssLocalSync.syncFeeds(feedId = someOtherFeedId)
            assertEquals("Request should not have been sent due to retry-after", 1, server.requestCount)
        }

    @Test
    fun feedsSyncedWithin15MinAreNotIgnoredWhenForcingNetwork() =
        runBlocking {
            val cowboyJsonId =
                insertFeed(
                    "cowboyjson",
                    server.url("/feed.json").toUrl(),
                    cowboyJson,
                )

            val fourteenMinsAgo = Instant.now().minusMinutes(14)
            runBlocking {
                rssLocalSync.syncFeeds(feedId = cowboyJsonId, forceNetwork = true)
                testDb.db.feedDao().getFeed(cowboyJsonId)!!.let { feed ->
                    assertTrue("Feed should have been synced", feed.lastSync.toEpochMilli() > 0)
//                    assertTrue("Feed should have a valid response hash", feed.responseHash > 0)

                    testDb.db.feedDao().updateFeed(feed.copy(lastSync = fourteenMinsAgo))
                }
                rssLocalSync.syncFeeds(
                    feedId = cowboyJsonId,
                    forceNetwork = true,
                    minFeedAgeMinutes = 15,
                )
            }

            assertEquals("Request should have been sent due to forced network", 2, server.requestCount)

            assertNotEquals(
                "Last sync should have changed",
                fourteenMinsAgo,
                testDb.db.feedDao().getFeed(cowboyJsonId)!!.lastSync,
            )
        }

    @Test
    fun feedShouldNotBeUpdatedIfRequestFails() =
        runBlocking {
            val response =
                MockResponse().also {
                    it.setResponseCode(500)
                }
            server.enqueue(response)

            val url = server.url("/feed.json")

            val failingJsonId =
                testDb.db.feedDao().insertFeed(
                    Feed(
                        title = "failJson",
                        url = URL("$url"),
                        tag = "",
                    ),
                )

            runBlocking {
                rssLocalSync.syncFeeds(feedId = failingJsonId)
            }

            assertNotEquals(
                "Last sync should have been updated",
                Instant.EPOCH,
                testDb.db.feedDao().getFeed(failingJsonId)!!.lastSync,
            )

            // Assert the feed was retrieved
            assertEquals("/feed.json", server.takeRequest().path)
        }

    @Test
    fun feedWithNoUniqueLinksGetsSomeGeneratedGUIDsFromTitles() =
        runBlocking {
            val response =
                MockResponse().also {
                    it.setResponseCode(200)
                    it.setBody(String(nixosRss.readBytes()))
                    it.setHeader("Content-Type", "application/xml")
                }
            server.enqueue(response)

            val url = server.url("/news-rss.xml")

            val feedId =
                testDb.db.feedDao().insertFeed(
                    Feed(
                        title = "NixOS",
                        url = URL("$url"),
                        tag = "",
                    ),
                )

            runBlocking {
                rssLocalSync.syncFeeds(
                    feedId = feedId,
                )
            }

            // Assert the feed was retrieved
            assertEquals("/news-rss.xml", server.takeRequest().path)
            val items = testDb.db.feedItemDao().loadFeedItemsInFeedDesc(feedId)
            assertEquals(
                "Unique IDs should have been generated for items",
                99,
                items.size,
            )

            // Should be unique to item so that it stays the same after updates
            assertTrue {
                items.first().guid.startsWith("https://nixos.org/news.html|")
            }
        }

    @Test
    fun feedWithNoDatesShouldGetSomeGenerated() =
        runBlocking {
            val response =
                MockResponse().also {
                    it.setResponseCode(200)
                    it.setBody(fooRss(2))
                    it.setHeader("Content-Type", "application/xml")
                }
            server.enqueue(response)

            val url = server.url("/rss")

            val feedId =
                testDb.db.feedDao().insertFeed(
                    Feed(
                        url = URL("$url"),
                    ),
                )

            val beforeSyncTime = Instant.now()

            runBlocking {
                rssLocalSync.syncFeeds(feedId = feedId)
            }

            // Assert the feed was retrieved
            assertEquals("/rss", server.takeRequest().path)

            val items = testDb.db.feedItemDao().loadFeedItemsInFeedDesc(feedId)

            assertNotNull(
                "Item should have gotten a pubDate generated",
                items[0].pubDate,
            )

            assertNotEquals(
                "Items should have distinct pubDates",
                items[0].pubDate,
                items[1].pubDate,
            )

            assertTrue(
                "The pubDate should be after 'before sync time'",
                items[0].pubDate!!.toInstant() > beforeSyncTime,
            )

            // Compare ID to compare insertion order (and thus pubdate compared to raw feed)
            assertTrue("The pubDates' magnitude should match descending iteration order") {
                items[0].guid == "https://foo.bar/1" &&
                    items[1].guid == "https://foo.bar/2" &&
                    items[0].pubDate!! > items[1].pubDate!!
            }
        }

    @Test
    fun feedShouldNotBeCleanedToHaveLessItemsThanActualFeed() =
        runBlocking {
            val feedItemCount = 9
            server.enqueue(
                MockResponse().also {
                    it.setResponseCode(200)
                    it.setBody(fooRss(feedItemCount))
                    it.setHeader("Content-Type", "application/xml")
                },
            )

            val url = server.url("/rss")

            val feedId =
                testDb.db.feedDao().insertFeed(
                    Feed(
                        url = URL("$url"),
                    ),
                )

            val maxFeedItemCount = 5

            // Sync first time
            runBlocking {
                rssLocalSync.syncFeeds(
                    feedId = feedId,
                    maxFeedItemCount = maxFeedItemCount,
                )
            }

            // Assert the feed was retrieved
            assertEquals("/rss", server.takeRequest(100, TimeUnit.MILLISECONDS)!!.path)

            testDb.db.feedItemDao().loadFeedItemsInFeedDesc(feedId).let { items ->
                assertEquals(
                    "Feed should have no less items than in the raw feed even if that's more than cleanup count",
                    feedItemCount,
                    items.size,
                )
            }
        }

    @Test
    @Ignore("This test is slow, would be nice to rewrite with a delay...")
    fun slowResponseShouldBeOk() =
        runBlocking {
            val url = server.url("/atom.xml").toUrl()
            val cowboyAtomId = insertFeed("cowboy", url, cowboyAtom, isJson = false)
            responses[url]!!.throttleBody(1024 * 100, 29, TimeUnit.SECONDS)

            runBlocking {
                rssLocalSync.syncFeeds(feedId = cowboyAtomId)
            }

            assertEquals(
                "Feed should have been parsed from slow response",
                15,
                testDb.db.feedItemDao().loadFeedItemsInFeedDesc(cowboyAtomId).size,
            )
        }

    @Test
    @Ignore("This test is slow, would be nice to rewrite with a delay...")
    fun verySlowResponseShouldBeCancelled() =
        runBlocking {
            val url = server.url("/atom.xml").toUrl()
            val cowboyAtomId = insertFeed("cowboy", url, cowboyAtom, isJson = false)
            responses[url]!!.throttleBody(1024 * 100, 31, TimeUnit.SECONDS)

            rssLocalSync.syncFeeds(feedId = cowboyAtomId)

            assertEquals(
                "Feed should not have been parsed from extremely slow response",
                0,
                testDb.db.feedItemDao().loadFeedItemsInFeedDesc(cowboyAtomId).size,
            )
        }

    @Test
    fun duplicateItemsAreNotSaved(): Unit =
        runBlocking {
            val atomUrl = server.url("/atom.xml").toUrl()
            val cowboyAtomId = insertFeed("cowboyAtom", atomUrl, cowboyAtom, isJson = false, skipDuplicates = true)

            runBlocking {
                rssLocalSync.syncFeeds(feedId = cowboyAtomId)
            }

            assertEquals(
                "All items from first feed should be present",
                15,
                testDb.db.feedItemDao().loadFeedItemsInFeedDesc(cowboyAtomId).size,
            )

            val jsonUrl = server.url("/feed.json").toUrl()
            val cowboyJsonId = insertFeed("cowboyJson", jsonUrl, cowboyJson, isJson = true, skipDuplicates = true)

            runBlocking {
                rssLocalSync.syncFeeds(feedId = cowboyJsonId)
            }

            assertEquals(
                "All items should have been filtered out due to duplicate checking",
                0,
                testDb.db.feedItemDao().loadFeedItemsInFeedDesc(cowboyJsonId).size,
            )
        }

    private fun fooRss(itemsCount: Int = 1): String {
        val items =
            (1..itemsCount).joinToString("\n") {
                """
                <item>
                  <title>Foo Item $it</title>
                  <link>https://foo.bar/$it</link>
                  <description>Woop woop $it</description>
                </item>
                """.trimIndent()
            }

        return """
            <?xml version="1.0" encoding="UTF-8"?>
            <rss version="2.0">
            <channel>
            <title>Foo Feed</title>
            <link>https://foo.bar</link>
            $items
            </channel>
            </rss>
            """.trimIndent()
    }
}

package com.nononsenseapps.feeder.archmodel

import com.nononsenseapps.feeder.db.room.FeedItemDao
import com.nononsenseapps.feeder.db.room.FeedItemIdWithLink
import com.nononsenseapps.feeder.db.room.FeedItemWithFeed
import io.mockk.MockKAnnotations
import io.mockk.coEvery
import io.mockk.coVerify
import io.mockk.every
import io.mockk.impl.annotations.MockK
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.flow.flowOf
import kotlinx.coroutines.flow.toList
import kotlinx.coroutines.runBlocking
import org.junit.Before
import org.junit.Test
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.bind
import org.kodein.di.instance
import org.kodein.di.singleton
import java.net.URL
import kotlin.test.assertEquals
import kotlin.test.assertNull
import kotlin.test.assertTrue

class FeedItemStoreTest : DIAware {
    private val store: FeedItemStore by instance()

    @MockK
    private lateinit var dao: FeedItemDao

    override val di by DI.lazy {
        bind<FeedItemDao>() with instance(dao)
        bind<FeedItemStore>() with singleton { FeedItemStore(di) }
    }

    @Before
    fun setup() {
        MockKAnnotations.init(this, relaxUnitFun = true, relaxed = true)
    }

    @Test
    fun markAsNotified() {
        runBlocking {
            store.markAsNotified(listOf(1L, 2L))
        }

        coVerify {
            dao.markAsNotified(listOf(1L, 2L), true)
        }
    }

    @Test
    fun markAsReadAndNotified() {
        runBlocking {
            store.markAsReadAndNotified(5L)
        }

        coVerify {
            dao.markAsReadAndNotified(5L, any())
        }
    }

    @Test
    fun markAsUnread() {
        runBlocking {
            store.markAsUnread(5L)
        }

        coVerify {
            dao.markAsUnread(5L)
        }
    }

    @Test
    fun getFullTextByDefault() {
        coEvery { dao.getFullTextByDefault(5L) } returns true

        assertTrue {
            runBlocking {
                store.getFullTextByDefault(5L)
            }
        }
    }

    @Test
    fun getFeedItem() {
        coEvery { dao.getFeedItemFlow(5L) } returns flowOf(FeedItemWithFeed(id = 5L))

        val feedItem =
            runBlocking {
                store.getFeedItem(5L).toList().first()
            }

        assertEquals(5L, feedItem?.id)
    }

    @Test
    fun getLink() {
        coEvery { dao.getLink(5L) } returns "foo"

        val link =
            runBlocking {
                store.getLink(5L)
            }

        assertEquals("foo", link)
    }

    @Test
    fun getArticleOpener() {
        coEvery { dao.getOpenArticleWith(5L) } returns "foo"

        val result =
            runBlocking {
                store.getArticleOpener(5L)
            }

        assertEquals("foo", result)
    }

    @Test
    fun markAllAsReadInFeed() {
        runBlocking {
            store.markAllAsReadInFeed(5L)
        }

        coVerify {
            dao.markAllAsRead(5L, any())
        }
    }

    @Test
    fun markAllAsReadInTag() {
        runBlocking {
            store.markAllAsReadInTag("sfz")
        }

        coVerify {
            dao.markAllAsRead("sfz", any())
        }
    }

    @Test
    fun markAllAsRead() {
        runBlocking {
            store.markAllAsRead()
        }

        coVerify {
            dao.markAllAsRead(readTime = any())
        }
    }

    @Test
    fun getFeedsItemsWithDefaultFullTextParse() {
        val expected =
            listOf(
                FeedItemIdWithLink(5L, "google.com"),
                FeedItemIdWithLink(6L, "cowboy.com"),
            )
        every { dao.getFeedsItemsWithDefaultFullTextNeedingDownload() } returns flowOf(expected)

        val items =
            runBlocking {
                store.getFeedsItemsWithDefaultFullTextNeedingDownload().first()
            }

        assertEquals(
            expected.size,
            items.size,
        )

        expected.zip(items) { expectedItem, actualItem ->
            assertEquals(
                expectedItem,
                actualItem,
            )
        }
    }

    @Test
    fun getFeedItemsNeedingNotifying() {
        val expected = listOf(1L, 2L)
        every { dao.getFeedItemsNeedingNotifying() } returns flowOf(expected)

        val items =
            runBlocking {
                store.getFeedItemsNeedingNotifying().first()
            }

        assertEquals(
            expected.size,
            items.size,
        )

        expected.zip(items) { expectedItem, actualItem ->
            assertEquals(
                expectedItem,
                actualItem,
            )
        }
    }

    @Test
    fun getFeedItemIdUrlAndGuid() {
        val url = URL("https://foo.bar")
        val guid = "foobar"
        coEvery { dao.getItemWith(url, guid) } returns 5L

        val id =
            runBlocking {
                store.getFeedItemId(url, guid)
            }

        assertEquals(5L, id)
    }

    @Test
    fun loadFeedItem() {
        coEvery { dao.loadFeedItem(any(), any()) } returns null

        val result =
            runBlocking {
                store.loadFeedItem("foo", 5L)
            }

        assertNull(result)

        coVerify { dao.loadFeedItem("foo", 5L) }
    }

    @Test
    fun getItemsToBeCleanedFromFeed() {
        coEvery { dao.getItemsToBeCleanedFromFeed(any(), any()) } returns listOf(5L)

        val result =
            runBlocking {
                store.getItemsToBeCleanedFromFeed(6L, 50)
            }

        assertEquals(5L, result.first())

        coVerify { dao.getItemsToBeCleanedFromFeed(6L, 50) }
    }

    @Test
    fun deleteFeedItems() {
        coEvery { dao.deleteFeedItems(any()) } returns 5

        runBlocking {
            store.deleteFeedItems(listOf(3L, 5L))
        }

        coVerify { dao.deleteFeedItems(listOf(3L, 5L)) }
    }
}

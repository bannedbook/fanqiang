package com.nononsenseapps.feeder.archmodel

import android.app.Application
import androidx.work.ExistingWorkPolicy
import androidx.work.OneTimeWorkRequest
import androidx.work.WorkManager
import com.nononsenseapps.feeder.ApplicationCoroutineScope
import jww.app.FeederApplication
import com.nononsenseapps.feeder.db.room.Feed
import com.nononsenseapps.feeder.db.room.ID_ALL_FEEDS
import com.nononsenseapps.feeder.db.room.ID_SAVED_ARTICLES
import com.nononsenseapps.feeder.db.room.ID_UNSET
import com.nononsenseapps.feeder.db.room.RemoteReadMarkReadyToBeApplied
import com.nononsenseapps.feeder.model.workmanager.SyncServiceSendReadWorker
import com.nononsenseapps.feeder.util.addDynamicShortcutToFeed
import com.nononsenseapps.feeder.util.reportShortcutToFeedUsed
import io.mockk.MockKAnnotations
import io.mockk.Runs
import io.mockk.coEvery
import io.mockk.coVerify
import io.mockk.confirmVerified
import io.mockk.every
import io.mockk.impl.annotations.MockK
import io.mockk.just
import io.mockk.mockkStatic
import io.mockk.spyk
import io.mockk.verify
import kotlinx.coroutines.flow.MutableStateFlow
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
import java.time.Instant
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue

class RepositoryTest : DIAware {
    private val repository: Repository by instance()

    @MockK
    private lateinit var feedItemStore: FeedItemStore

    @MockK
    private lateinit var settingsStore: SettingsStore

    @MockK
    private lateinit var sessionStore: SessionStore

    @MockK
    private lateinit var feedStore: FeedStore

    @MockK
    private lateinit var syncRemoteStore: SyncRemoteStore

    @MockK
    private lateinit var androidSystemStore: AndroidSystemStore

    @MockK
    private lateinit var application: FeederApplication

    @MockK
    private lateinit var workManager: WorkManager

    override val di by DI.lazy {
        bind<Repository>() with singleton { spyk(Repository(di)) }
        bind<FeedItemStore>() with instance(feedItemStore)
        bind<SettingsStore>() with instance(settingsStore)
        bind<SessionStore>() with instance(sessionStore)
        bind<SyncRemoteStore>() with instance(syncRemoteStore)
        bind<FeedStore>() with instance(feedStore)
        bind<WorkManager>() with instance(workManager)
        bind<AndroidSystemStore>() with instance(androidSystemStore)
        bind<Application>() with instance(application)
        bind<ApplicationCoroutineScope>() with singleton { ApplicationCoroutineScope() }
    }

    @Before
    fun setup() {
        MockKAnnotations.init(this, relaxUnitFun = true, relaxed = true)

        every { settingsStore.syncOnlyWhenCharging } returns MutableStateFlow(false)
        every { settingsStore.syncOnlyOnWifi } returns MutableStateFlow(false)
        every { settingsStore.addedFeederNews } returns MutableStateFlow(true)
        every { settingsStore.minReadTime } returns MutableStateFlow(Instant.EPOCH)

        every { feedItemStore.getFeedItemCountRaw(any(), any(), any(), any()) } returns flowOf(0)
    }

    @Test
    fun initialStartWillAddFeederNews() {
        every { settingsStore.addedFeederNews } returns MutableStateFlow(false)

        // Construct it
        repository

        coVerify(timeout = 500L, exactly = 1) {
            settingsStore.addedFeederNews
            feedStore.upsertFeed(
                Feed(
                    title = "Feeder News",
                    url = URL("https://news.nononsenseapps.com/index.atom"),
                ),
            )
            settingsStore.setAddedFeederNews(true)
        }
    }

    @Test
    fun secondStartWillNotAddFeederNews() {
        every { settingsStore.addedFeederNews } returns MutableStateFlow(true)

        // Construct it
        repository

        coVerify(timeout = 500L, exactly = 0) {
            feedStore.upsertFeed(
                Feed(
                    title = "Feeder News",
                    url = URL("https://news.nononsenseapps.com/index.atom"),
                ),
            )
        }
    }

    @Test
    fun setCurrentFeedAndTagTagDoesNotReportFeedShortcut() {
        mockkStatic("com.nononsenseapps.feeder.util.ContextExtensionsKt")

        repository.setCurrentFeedAndTag(ID_UNSET, "foo")

        coVerify(timeout = 500L, exactly = 0) {
            application.addDynamicShortcutToFeed(
                "fooFeed",
                10L,
                null,
            )
            application.reportShortcutToFeedUsed(10L)
        }
    }

    @Test
    fun setCurrentFeedAndTagFeedReportsShortcutAndUpdatesMinReadTime() {
        coEvery { feedStore.getDisplayTitle(10L) } returns "fooFeed"
        coEvery { settingsStore.setCurrentFeedAndTag(any(), any()) } returns true
        coEvery { settingsStore.setMinReadTime(any()) } just Runs

        mockkStatic("com.nononsenseapps.feeder.util.ContextExtensionsKt")

        repository.setCurrentFeedAndTag(10L, "")

        coVerify(timeout = 500L) {
            application.addDynamicShortcutToFeed(
                "fooFeed",
                10L,
                null,
            )
            application.reportShortcutToFeedUsed(10L)
            settingsStore.setMinReadTime(any())
        }
    }

    @Test
    fun setCurrentFeedAndTagToSameDoesntChangeMinReadTime() {
        coEvery { feedStore.getDisplayTitle(10L) } returns "fooFeed"
        coEvery { settingsStore.setCurrentFeedAndTag(any(), any()) } returns false
        coEvery { settingsStore.setMinReadTime(any()) } just Runs

        mockkStatic("com.nononsenseapps.feeder.util.ContextExtensionsKt")

        repository.setCurrentFeedAndTag(10L, "")

        coVerify(exactly = 0, timeout = 500L) {
            settingsStore.setMinReadTime(any())
        }
    }

    @Test
    fun getTextToDisplayForItem() {
        coEvery { feedItemStore.getFullTextByDefault(5L) } returns true
        coEvery { feedItemStore.getFullTextByDefault(6L) } returns false

        assertEquals(
            true,
            runBlocking {
                repository.shouldDisplayFullTextForItemByDefault(5L)
            },
        )

        assertEquals(
            false,
            runBlocking {
                repository.shouldDisplayFullTextForItemByDefault(6L)
            },
        )
    }

    @Test
    fun getArticleOpener() {
        coEvery { feedItemStore.getArticleOpener(5L) } returns PREF_VAL_OPEN_WITH_CUSTOM_TAB

        assertEquals(
            ItemOpener.CUSTOM_TAB,
            runBlocking {
                repository.getArticleOpener(5L)
            },
        )
    }

    @Test
    fun getArticleOpenerDefaultFallback() {
        coEvery { feedItemStore.getArticleOpener(5L) } returns ""
        every { settingsStore.itemOpener } returns MutableStateFlow(ItemOpener.DEFAULT_BROWSER)

        assertEquals(
            ItemOpener.DEFAULT_BROWSER,
            runBlocking {
                repository.getArticleOpener(5L)
            },
        )
    }

    @Test
    fun markAllAsReadInCurrentFeed() {
        runBlocking {
            repository.markAllAsReadInFeedOrTag(5L, "foo")
        }

        coVerify {
            feedItemStore.markAllAsReadInFeed(5L)
        }
        verify {
            settingsStore.setMinReadTime(any())
        }
    }

    @Test
    fun markAllAsReadInCurrentTag() {
        runBlocking {
            repository.markAllAsReadInFeedOrTag(ID_UNSET, "foo")
        }

        coVerify {
            feedItemStore.markAllAsReadInTag("foo")
        }
        verify {
            settingsStore.setMinReadTime(any())
        }
    }

    @Test
    fun maxLines() {
        repository.setMaxLines(9)

        verify { settingsStore.setMaxLines(9) }

        repository.setMaxLines(-1)

        verify { settingsStore.setMaxLines(1) }
    }

    @Test
    fun markAllAsReadInCurrentAll() {
        runBlocking {
            repository.markAllAsReadInFeedOrTag(ID_ALL_FEEDS, "")
        }

        coVerify {
            feedItemStore.markAllAsRead()
        }
        verify {
            settingsStore.setMinReadTime(any())
        }
    }

    @Test
    fun getScreenTitleForCurrentFeedOrTagAll() {
        val result =
            runBlocking {
                repository.getScreenTitleForFeedOrTag(ID_ALL_FEEDS, "").toList().first()
            }

        assertEquals(ScreenTitle(title = null, type = FeedType.ALL_FEEDS, unreadCount = 0), result)
    }

    @Test
    fun getScreenTitleForCurrentFeedOrTagSavedArticles() {
        val result =
            runBlocking {
                repository.getScreenTitleForFeedOrTag(ID_SAVED_ARTICLES, "").toList().first()
            }

        assertEquals(ScreenTitle(title = null, type = FeedType.SAVED_ARTICLES, unreadCount = 0), result)
    }

    @Test
    fun getScreenTitleForCurrentFeedOrTagTag() {
        val result =
            runBlocking {
                repository.getScreenTitleForFeedOrTag(ID_UNSET, "fwr").toList().first()
            }

        assertEquals(ScreenTitle(title = "fwr", type = FeedType.TAG, unreadCount = 0), result)
    }

    @Test
    fun getScreenTitleForCurrentFeedOrTagFeed() {
        coEvery { feedStore.getDisplayTitle(5L) } returns "floppa"

        val result =
            runBlocking {
                repository.getScreenTitleForFeedOrTag(5L, "fwr").toList().first()
            }

        assertEquals(ScreenTitle(title = "floppa", type = FeedType.FEED, unreadCount = 0), result)

        coVerify {
            feedStore.getDisplayTitle(5L)
        }
    }

    @Test
    fun deleteFeedsNotCurrentFeed() {
        coEvery { feedStore.deleteFeeds(any()) } just Runs
        every { settingsStore.currentFeedAndTag } returns MutableStateFlow(3L to "")

        runBlocking {
            repository.deleteFeeds(listOf(1, 2))
        }

        coVerify {
            feedStore.deleteFeeds(listOf(1, 2))
        }
        verify(exactly = 0) {
            settingsStore.setCurrentFeedAndTag(any(), any())
        }

        verify {
            androidSystemStore.removeDynamicShortcuts(listOf(1, 2))
        }
    }

    @Test
    fun deleteFeedsCurrentFeed() {
        coEvery { feedStore.deleteFeeds(any()) } just Runs
        every { settingsStore.currentFeedAndTag } returns MutableStateFlow(1L to "")

        runBlocking {
            repository.deleteFeeds(listOf(1, 2))
        }

        coVerify {
            feedStore.deleteFeeds(listOf(1, 2))
        }
        verify {
            settingsStore.setCurrentFeedAndTag(ID_ALL_FEEDS, any())
        }

        verify {
            androidSystemStore.removeDynamicShortcuts(listOf(1, 2))
        }
    }

    @Test
    fun ensurePeriodicSyncConfigured() {
        coEvery { settingsStore.configurePeriodicSync(any()) } just Runs

        runBlocking {
            repository.ensurePeriodicSyncConfigured()
        }

        coVerify {
            settingsStore.configurePeriodicSync(false)
        }
    }

    @Test
    fun getFeedsItemsWithDefaultFullTextParse() {
        coEvery { feedItemStore.getFeedsItemsWithDefaultFullTextNeedingDownload() } returns
            flowOf(
                emptyList(),
            )

        val result =
            runBlocking {
                repository.getFeedsItemsWithDefaultFullTextNeedingDownload().first()
            }

        assertTrue {
            result.isEmpty()
        }

        coVerify {
            feedItemStore.getFeedsItemsWithDefaultFullTextNeedingDownload()
        }
    }

    @Test
    fun currentlySyncingLatestTimestamp() {
        every { feedStore.getCurrentlySyncingLatestTimestamp() } returns flowOf(null)

        val result =
            runBlocking {
                repository.currentlySyncing.toList()
            }

        assertEquals(1, result.size)
        assertFalse(result.first())

        verify {
            feedStore.getCurrentlySyncingLatestTimestamp()
        }
    }

    @Test
    fun applyRemoteReadMarks() {
        coEvery { syncRemoteStore.getRemoteReadMarksReadyToBeApplied() } returns
            listOf(
                RemoteReadMarkReadyToBeApplied(1L, 2L),
                RemoteReadMarkReadyToBeApplied(3L, 4L),
            )

        runBlocking {
            repository.applyRemoteReadMarks()
        }

        coVerify {
            syncRemoteStore.getRemoteReadMarksReadyToBeApplied()
            feedItemStore.markAsRead(listOf(2L, 4L))
            syncRemoteStore.setSynced(2L)
            syncRemoteStore.setSynced(4L)
            syncRemoteStore.deleteReadStatusSyncs(listOf(1L, 3L))
        }
        confirmVerified(feedItemStore, syncRemoteStore)
    }

    @Test
    fun remoteMarkAsReadExistingItem() {
        coEvery { feedItemStore.getFeedItemId(URL("https://foo"), "guid") } returns 5L

        runBlocking {
            repository.remoteMarkAsRead(URL("https://foo"), "guid")
        }

        coVerify {
            feedItemStore.getFeedItemId(URL("https://foo"), "guid")
            syncRemoteStore.addRemoteReadMark(feedUrl = URL("https://foo"), articleGuid = "guid")
            syncRemoteStore.setSynced(5L)
            feedItemStore.markAsReadAndNotified(5L, any())
        }
        confirmVerified(feedItemStore, syncRemoteStore)
    }

    @Test
    fun markAsReadSchedulesSend() {
        runBlocking {
            repository.markAsReadAndNotified(1L)
        }

        coVerify {
            feedItemStore.markAsReadAndNotified(1L, any())
            workManager.enqueueUniqueWork(
                SyncServiceSendReadWorker.UNIQUE_SENDREAD_NAME,
                ExistingWorkPolicy.REPLACE,
                any<OneTimeWorkRequest>(),
            )
        }
        confirmVerified(feedItemStore, workManager)
    }

    @Test
    fun remoteMarkAsReadNonExistingItem() {
        coEvery { feedItemStore.getFeedItemId(any(), any()) } returns null
        coEvery { syncRemoteStore.addRemoteReadMark(any(), any()) } just Runs

        runBlocking {
            repository.remoteMarkAsRead(URL("https://foo"), "guid")
        }

        coVerify {
            feedItemStore.getFeedItemId(URL("https://foo"), "guid")
            syncRemoteStore.addRemoteReadMark(URL("https://foo"), "guid")
        }
        confirmVerified(feedItemStore, syncRemoteStore)
    }
}

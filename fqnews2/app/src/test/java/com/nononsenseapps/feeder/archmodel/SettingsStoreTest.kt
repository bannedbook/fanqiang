package com.nononsenseapps.feeder.archmodel

import android.content.SharedPreferences
import androidx.work.ExistingPeriodicWorkPolicy
import androidx.work.WorkManager
import com.nononsenseapps.feeder.db.room.BlocklistDao
import com.nononsenseapps.feeder.model.workmanager.UNIQUE_PERIODIC_NAME
import com.nononsenseapps.feeder.model.workmanager.oldPeriodics
import com.nononsenseapps.feeder.util.PREF_MAX_ITEM_COUNT_PER_FEED
import io.mockk.MockKAnnotations
import io.mockk.clearMocks
import io.mockk.coVerify
import io.mockk.confirmVerified
import io.mockk.every
import io.mockk.impl.annotations.MockK
import io.mockk.verify
import kotlinx.coroutines.runBlocking
import org.junit.Before
import org.junit.Test
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.bind
import org.kodein.di.instance
import org.kodein.di.singleton
import java.time.Instant
import kotlin.test.assertEquals
import kotlin.test.assertTrue

class SettingsStoreTest : DIAware {
    private val store: SettingsStore by instance()

    @MockK
    private lateinit var sp: SharedPreferences

    @MockK
    private lateinit var blocklistDao: BlocklistDao

    @MockK
    private lateinit var workManager: WorkManager
    override val di by DI.lazy {
        bind<SharedPreferences>() with instance(sp)
        bind<WorkManager>() with instance(workManager)
        bind<SettingsStore>() with singleton { SettingsStore(di) }
        bind<BlocklistDao>() with singleton { blocklistDao }
    }

    @Before
    fun setup() {
        MockKAnnotations.init(this, relaxUnitFun = true, relaxed = true)

        // Necessary globally for enum conversion
        every { sp.getString(PREF_THEME, null) } returns null
        every { sp.getString(PREF_DARK_THEME, null) } returns null
        every { sp.getString(PREF_SORT, null) } returns null
        every { sp.getString(PREF_MAX_ITEM_COUNT_PER_FEED, "100") } returns null
        every { sp.getString(PREF_SYNC_FREQ, "60") } returns null
        // Incorrect value by design
        every {
            sp.getString(
                PREF_SWIPE_AS_READ,
                SwipeAsRead.ONLY_FROM_END.name,
            )
        } returns SwipeAsRead.DISABLED.name
    }

    @Test
    fun swipeAsReadSet() {
        store.setSwipeAsRead(SwipeAsRead.DISABLED)

        verify {
            sp.edit().putString(PREF_SWIPE_AS_READ, "DISABLED").apply()
        }

        assertEquals(SwipeAsRead.DISABLED, store.swipeAsRead.value, "Expected get to match mock")
    }

    @Test
    fun textScaleSet() {
        store.setTextScale(1.5f)

        verify {
            sp.edit().putFloat(PREF_TEXT_SCALE, 1.5f).apply()
        }

        assertEquals(1.5f, store.textScale.value, "Expected get to match mock")
    }

    @Test
    fun swipeAsReadBadValueInPrefs() {
        every { sp.getString(PREF_SWIPE_AS_READ, any()) } returns "Not an enum value"

        assertEquals(
            SwipeAsRead.ONLY_FROM_END,
            store.swipeAsRead.value,
            "Expected bad value to be ignored",
        )
    }

    @Test
    fun filterRead() {
        every { sp.getBoolean(PREFS_FILTER_READ, any()) } returns true

        store.setFeedListFilterRead(false)

        verify {
            sp.edit().putBoolean(PREFS_FILTER_READ, false).apply()
        }

        assertEquals(false, store.feedListFilter.value.read)
    }

    @Test
    fun filterRecentlyRead() {
        every { sp.getBoolean(PREFS_FILTER_RECENTLY_READ, any()) } returns true

        store.setFeedListFilterRecentlyRead(false)

        verify {
            sp.edit().putBoolean(PREFS_FILTER_RECENTLY_READ, false).apply()
        }

        assertEquals(false, store.feedListFilter.value.recentlyRead)
    }

    @Test
    fun filterSaved() {
        every { sp.getBoolean(PREFS_FILTER_SAVED, any()) } returns true

        store.setFeedListFilterSaved(false)

        verify {
            sp.edit().putBoolean(PREFS_FILTER_SAVED, false).apply()
        }

        assertEquals(false, store.feedListFilter.value.saved)
    }

    @Test
    fun currentFeedAndTag() {
        store.setCurrentFeedAndTag(8L, "bar")

        verify {
            sp.edit().putLong(PREF_LAST_FEED_ID, 8L).apply()
            sp.edit().putString(PREF_LAST_FEED_TAG, "bar").apply()
        }

        assertEquals(8L to "bar", store.currentFeedAndTag.value)
    }

    @Test
    fun currentArticle() {
        store.setCurrentArticle(8L)

        verify {
            sp.edit().putLong(PREF_LAST_ARTICLE_ID, 8L).apply()
        }

        assertEquals(8L, store.currentArticleId.value)
    }

    @Test
    fun currentTheme() {
        store.setCurrentTheme(ThemeOptions.NIGHT)

        verify {
            sp.edit().putString(PREF_THEME, "night").apply()
        }

        assertEquals(ThemeOptions.NIGHT, store.currentTheme.value)
    }

    @Test
    fun darkThemePreference() {
        store.setDarkThemePreference(DarkThemePreferences.DARK)

        verify {
            sp.edit().putString(PREF_DARK_THEME, "dark").apply()
        }

        assertEquals(DarkThemePreferences.DARK, store.darkThemePreference.value)
    }

    @Test
    fun maxLines() {
        store.setMaxLines(5)

        verify {
            sp.edit().putInt(PREF_MAX_LINES, 5).apply()
        }

        assertEquals(5, store.maxLines.value)
    }

    @Test
    fun currentSorting() {
        store.setCurrentSorting(SortingOptions.OLDEST_FIRST)

        verify {
            sp.edit().putString(PREF_SORT, "oldest_first").apply()
        }

        assertEquals(SortingOptions.OLDEST_FIRST, store.currentSorting.value)
    }

    @Test
    fun showFab() {
        store.setShowFab(false)

        verify {
            sp.edit().putBoolean(PREF_SHOW_FAB, false).apply()
        }

        assertEquals(false, store.showFab.value)
    }

    @Test
    fun syncOnResume() {
        store.setSyncOnResume(true)

        verify {
            sp.edit().putBoolean(PREF_SYNC_ON_RESUME, true).apply()
        }

        assertEquals(true, store.syncOnResume.value)
    }

    @Test
    fun syncOnlyOnWifi() {
        runBlocking {
            store.setSyncOnlyOnWifi(true)
        }
        coVerify {
            sp.edit().putBoolean(PREF_SYNC_ONLY_WIFI, true).apply()
        }

        assertEquals(true, store.syncOnlyOnWifi.value)
    }

    @Test
    fun syncOnlyWhenCharging() {
        runBlocking {
            store.setSyncOnlyWhenCharging(true)
        }
        coVerify {
            sp.edit().putBoolean(PREF_SYNC_ONLY_CHARGING, true).apply()
        }

        assertEquals(true, store.syncOnlyWhenCharging.value)
    }

    @Test
    fun loadImageOnlyOnWifi() {
        store.setLoadImageOnlyOnWifi(true)

        verify {
            sp.edit().putBoolean(PREF_IMG_ONLY_WIFI, true).apply()
        }

        assertEquals(true, store.loadImageOnlyOnWifi.value)
    }

    @Test
    fun showThumbnails() {
        store.setShowThumbnails(false)

        verify {
            sp.edit().putBoolean(PREF_IMG_SHOW_THUMBNAILS, false).apply()
        }

        assertEquals(false, store.showThumbnails.value)
    }

    @Test
    fun useDetectLanguage() {
        store.setUseDetectLanguage(false)
        verify {
            sp.edit().putBoolean(PREF_READALOUD_USE_DETECT_LANGUAGE, false).apply()
        }

        assertEquals(false, store.useDetectLanguage.value)
    }

    @Test
    fun useDynamicTheme() {
        store.setUseDynamicTheme(false)
        verify {
            sp.edit().putBoolean(PREF_DYNAMIC_THEME, false).apply()
        }

        assertEquals(false, store.useDynamicTheme.value)
    }

    @Test
    fun maximumCountPerFeed() {
        store.setMaxCountPerFeed(200)

        verify {
            sp.edit().putString(PREF_MAX_ITEM_COUNT_PER_FEED, "200").apply()
        }

        assertEquals(200, store.maximumCountPerFeed.value, "Expected get to match mock")
    }

    @Test
    fun itemOpener() {
        store.setItemOpener(ItemOpener.CUSTOM_TAB)

        verify {
            sp.edit().putString(PREF_DEFAULT_OPEN_ITEM_WITH, PREF_VAL_OPEN_WITH_CUSTOM_TAB).apply()
        }

        assertEquals(ItemOpener.CUSTOM_TAB, store.itemOpener.value)
    }

    @Test
    fun linkOpener() {
        store.setLinkOpener(LinkOpener.DEFAULT_BROWSER)

        verify {
            sp.edit().putString(PREF_OPEN_LINKS_WITH, PREF_VAL_OPEN_WITH_BROWSER).apply()
        }

        assertEquals(LinkOpener.DEFAULT_BROWSER, store.linkOpener.value)
    }

    @Test
    fun syncFrequency() {
        runBlocking {
            store.setSyncFrequency(SyncFrequency.EVERY_3_HOURS)
        }
        coVerify {
            sp.edit().putString(PREF_SYNC_FREQ, "180").apply()
            for (oldPeriodic in oldPeriodics) {
                workManager.cancelUniqueWork(oldPeriodic)
            }
            workManager.enqueueUniquePeriodicWork(
                UNIQUE_PERIODIC_NAME,
                ExistingPeriodicWorkPolicy.UPDATE,
                any(),
            )
        }

        assertEquals(SyncFrequency.EVERY_3_HOURS, store.syncFrequency.value)
    }

    @Test
    fun blockListGlobs() {
        runBlocking {
            for (pattern in listOf("FOO", " bAr ", "inj'ection", "att\\ack", "  ")) {
                store.addBlocklistPattern(pattern)
            }
        }
        coVerify {
            blocklistDao.insertSafely("foo")
            blocklistDao.insertSafely("bar")
            blocklistDao.insertSafely("inj'ection")
            blocklistDao.insertSafely("att\\ack")
        }
        confirmVerified(blocklistDao)
    }

    @Test
    fun minReadTimeOnlyUnread() {
        assertTrue {
            store.minReadTime.value > Instant.EPOCH
        }

        val value = Instant.ofEpochSecond(1691013971)

        clearMocks(sp)

        store.setMinReadTime(value)
        assertEquals(value, store.minReadTime.value)

        confirmVerified(sp)
    }

    @Test
    fun getAllSettingsForOPMLExport() {
        every { sp.all } returns
            mapOf(
                PREF_SHOW_FAB to false,
                "Not a pref" to true,
                // Not OK if imported on a fresh device
                PREF_LAST_FEED_TAG to "foo",
                PREF_LAST_FEED_ID to 1L,
                PREF_LAST_ARTICLE_ID to 2L,
                PREF_IS_ARTICLE_OPEN to true,
            )
        val allSettings = store.getAllSettings()
        assertEquals(1, allSettings.size)
    }

    @Test
    fun showTitleUnreadCount() {
        store.setShowTitleUnreadCount(true)

        verify {
            sp.edit().putBoolean(PREF_SHOW_TITLE_UNREAD_COUNT, true).apply()
        }

        assertEquals(true, store.showTitleUnreadCount.value)
    }
}

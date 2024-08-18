package com.nononsenseapps.feeder.db.room

import androidx.room.testing.MigrationTestHelper
import androidx.sqlite.db.framework.FrameworkSQLiteOpenHelperFactory
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.LargeTest
import androidx.test.platform.app.InstrumentationRegistry
import jww.app.FeederApplication
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.android.closestDI
import kotlin.test.assertEquals

@RunWith(AndroidJUnit4::class)
@LargeTest
class TestMigrationFrom31To32 : DIAware {
    private val dbName = "testDb"
    private val feederApplication: FeederApplication = ApplicationProvider.getApplicationContext()
    override val di: DI by closestDI(feederApplication)

    @Rule
    @JvmField
    val testHelper: MigrationTestHelper =
        MigrationTestHelper(
            InstrumentationRegistry.getInstrumentation(),
            AppDatabase::class.java,
            emptyList(),
            FrameworkSQLiteOpenHelperFactory(),
        )

    @Test
    fun migrate() {
        @Suppress("SimpleRedundantLet")
        testHelper.createDatabase(dbName, FROM_VERSION).let { oldDB ->
            oldDB.execSQL(
                """
                INSERT INTO feeds(id, title, url, custom_title, tag, notify, last_sync, response_hash, fulltext_by_default, open_articles_with, alternate_id, currently_syncing, when_modified, site_fetched)
                VALUES(1, 'feed', 'http://url', '', '', 0, 0, 666, 0, '', 0, 0, 0, 0)
                """.trimIndent(),
            )
            oldDB.execSQL(
                """
                INSERT INTO feed_items(id, guid, title, plain_title, plain_snippet, notified, feed_id, first_synced_time, primary_sort_time, pinned, bookmarked, fulltext_downloaded, read_time, unread, word_count, word_count_full)
                VALUES(8, 'http://item1', 'title', 'ptitle', 'psnippet', 0, 1, 0, 0, 1, 0, 0, 0, 1, 5, 900)
                """.trimIndent(),
            )
        }
        val db =
            testHelper.runMigrationsAndValidate(
                dbName,
                TO_VERSION,
                true,
                MigrationFrom31To32(di),
            )

        db.query(
            """
            select image_from_body from feed_items
            """.trimIndent(),
        ).use {
            assert(it.count == 1)
            assert(it.moveToFirst())
            assertEquals(0, it.getInt(0))
        }
    }

    companion object {
        private const val FROM_VERSION = 31
        private const val TO_VERSION = 32
    }
}

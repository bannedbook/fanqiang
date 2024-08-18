package com.nononsenseapps.feeder.db.room

import androidx.room.testing.MigrationTestHelper
import androidx.sqlite.db.framework.FrameworkSQLiteOpenHelperFactory
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.LargeTest
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Assert.assertEquals
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
@LargeTest
class MigrationFrom13To14 {
    private val dbName = "testDb"

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
    fun migrate13to14() {
        var db = testHelper.createDatabase(dbName, 13)

        db.use {
            db.execSQL(
                """
                INSERT INTO feeds(id, title, url, custom_title, tag, notify, last_sync, response_hash, fulltext_by_default)
                VALUES(1, 'feed', 'http://url', '', '', 0, 0, 666, 0)
                """.trimIndent(),
            )

            db.execSQL(
                """
                INSERT INTO feed_items(id, guid, title, plain_title, plain_snippet, unread, notified, feed_id, first_synced_time, primary_sort_time)
                VALUES(8, 'http://item', 'title', 'ptitle', 'psnippet', 1, 0, 1, 0, 0)
                """.trimIndent(),
            )
        }

        db = testHelper.runMigrationsAndValidate(dbName, 14, true, MIGRATION_13_14)

        db.query(
            """
            SELECT open_articles_with FROM feeds
            """.trimIndent(),
        ).use {
            assert(it.count == 1)
            assert(it.moveToFirst())
            assertEquals(0L, it.getLong(0))
        }
    }
}

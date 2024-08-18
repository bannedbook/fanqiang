package com.nononsenseapps.feeder.db.room

import androidx.room.testing.MigrationTestHelper
import androidx.sqlite.db.framework.FrameworkSQLiteOpenHelperFactory
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.LargeTest
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import kotlin.test.assertEquals

@RunWith(AndroidJUnit4::class)
@LargeTest
class MigrationFrom21To22 {
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
    fun migrate() {
        testHelper.createDatabase(dbName, 21).let { oldDB ->
            oldDB.execSQL(
                """
                INSERT INTO feeds(id, title, url, custom_title, tag, notify, last_sync, response_hash, fulltext_by_default, open_articles_with, alternate_id, currently_syncing, when_modified)
                VALUES(1, 'feed', 'http://url', '', '', 0, 0, 666, 0, '', 0, 0, 0)
                """.trimIndent(),
            )
            oldDB.execSQL(
                """
                INSERT INTO feed_items(id, guid, title, plain_title, plain_snippet, unread, notified, feed_id, first_synced_time, primary_sort_time)
                VALUES(8, 'http://item', 'title', 'ptitle', 'psnippet', 1, 0, 1, 0, 0)
                """.trimIndent(),
            )
        }

        val db = testHelper.runMigrationsAndValidate(dbName, 22, true, MIGRATION_21_22)

        db.query(
            """
            SELECT pinned FROM feed_items
            """.trimIndent(),
        ).use {
            assert(it.count == 1)
            assert(it.moveToFirst())
            assertEquals(0, it.getInt(0))
        }
    }
}

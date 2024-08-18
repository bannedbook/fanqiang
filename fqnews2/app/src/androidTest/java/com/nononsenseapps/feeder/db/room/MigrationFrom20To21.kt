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
import kotlin.test.assertTrue

@RunWith(AndroidJUnit4::class)
@LargeTest
class MigrationFrom20To21 {
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
        testHelper.createDatabase(dbName, 20).let { oldDB ->
            oldDB.execSQL(
                """
                INSERT INTO sync_remote(id, url, sync_chain_id, latest_message_timestamp, device_id, device_name)
                VALUES (1, '', '', 0, 5, 'Foo')
                """.trimIndent(),
            )
            oldDB.execSQL(
                """
                INSERT INTO feeds(id, title, url, custom_title, tag, notify, last_sync, response_hash, fulltext_by_default, open_articles_with, alternate_id, currently_syncing)
                VALUES(1, 'feed', 'http://url', '', '', 0, 0, 666, 0, '', 0, 0)
                """.trimIndent(),
            )
        }

        val db = testHelper.runMigrationsAndValidate(dbName, 21, true, MIGRATION_20_21)

        db.query(
            """
            SELECT secret_key, last_feeds_remote_hash  FROM sync_remote WHERE id IS 1
            """.trimIndent(),
        ).use {
            assert(it.count == 1)
            assert(it.moveToFirst())
            assertTrue {
                it.getString(0).isNotBlank()
            }
            assertEquals(0, it.getLong(1))
        }

        db.query(
            """
            SELECT when_modified FROM feeds LIMIT 1
            """.trimIndent(),
        ).use {
            assert(it.count == 1)
            assert(it.moveToFirst())
            assertEquals(0, it.getLong(0))
        }
    }
}

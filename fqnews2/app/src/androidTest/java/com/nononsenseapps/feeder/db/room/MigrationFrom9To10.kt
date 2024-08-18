package com.nononsenseapps.feeder.db.room

import androidx.room.testing.MigrationTestHelper
import androidx.sqlite.db.framework.FrameworkSQLiteOpenHelperFactory
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.LargeTest
import androidx.test.platform.app.InstrumentationRegistry
import jww.app.FeederApplication
import com.nononsenseapps.feeder.blob.blobInputStream
import org.junit.Assert.assertEquals
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
@LargeTest
class MigrationFrom9To10 {
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
    fun migrate9to10() {
        var db = testHelper.createDatabase(dbName, 9)

        db.use {
            db.execSQL(
                """
                INSERT INTO feeds(id, title, url, custom_title, tag, notify, last_sync, response_hash)
                VALUES(1, 'feed', 'http://url', '', '', 0, 0, 666)
                """.trimIndent(),
            )

            db.execSQL(
                """
                INSERT INTO feed_items(id, guid, title, plain_title, plain_snippet, unread, notified, feed_id, description)
                VALUES(8, 'http://item', 'title', 'ptitle', 'psnippet', 1, 0, 1, '$bigBody')
                """.trimIndent(),
            )
        }

        db = testHelper.runMigrationsAndValidate(dbName, 10, true, MIGRATION_9_10)

        db.query(
            """
            SELECT response_hash FROM feeds WHERE id IS 1
            """.trimIndent(),
        ).use {
            assert(it.count == 1)
            assert(it.moveToFirst())
            assertEquals(0L, it.getLong(0))
        }

        db.query(
            """
            SELECT id, title FROM feed_items
            """.trimIndent(),
        ).use {
            assert(it.count == 1)
            assert(it.moveToFirst())
            assertEquals(8L, it.getLong(0))
            assertEquals("title", it.getString(1))
        }

        blobInputStream(
            itemId = 8,
            filesDir = ApplicationProvider.getApplicationContext<FeederApplication>().filesDir,
        ).bufferedReader().useLines {
            val lines = it.toList()
            assertEquals(1, lines.size)
            assertEquals(bigBody.take(999_999), lines.first())
        }
    }

    // 4MB field
    private val bigBody: String = "a".repeat(4 * 1024 * 1024)
}

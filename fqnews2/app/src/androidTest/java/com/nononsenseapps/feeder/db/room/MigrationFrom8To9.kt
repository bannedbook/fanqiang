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
class MigrationFrom8To9 {
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
    fun migrate8to9() {
        var db = testHelper.createDatabase(dbName, 8)

        db.use {
            db.execSQL(
                """
                INSERT INTO feeds(title, url, custom_title, tag, notify, last_sync)
                VALUES('feed', 'http://url', '', '', 0, 0)
                """.trimIndent(),
            )
        }

        db = testHelper.runMigrationsAndValidate(dbName, 9, true, MIGRATION_8_9)

        db.query(
            """
            SELECT title, url, response_hash FROM feeds
            """.trimIndent(),
        ).use {
            assert(it.count == 1)
            assert(it.moveToFirst())
            assertEquals("feed", it.getString(0))
            assertEquals("http://url", it.getString(1))
            assertEquals(0L, it.getLong(2))
        }
    }
}

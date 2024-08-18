package com.nononsenseapps.feeder.db.room

import androidx.room.testing.MigrationTestHelper
import androidx.sqlite.db.framework.FrameworkSQLiteOpenHelperFactory
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.LargeTest
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Assert
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
@LargeTest
class MigrationFrom19To20 {
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
        testHelper.createDatabase(dbName, 19).execSQL(
            """
            INSERT INTO sync_remote(id, url, sync_chain_id, latest_message_timestamp)
            VALUES (1, '', '', 0)
            """.trimIndent(),
        )

        val db = testHelper.runMigrationsAndValidate(dbName, 20, true, MIGRATION_19_20)

        db.query(
            """
            SELECT device_id, device_name FROM sync_remote WHERE id IS 1
            """.trimIndent(),
        ).use {
            assert(it.count == 1)
            assert(it.moveToFirst())
            Assert.assertEquals(ID_UNSET, it.getLong(0))
            Assert.assertEquals("", it.getString(1))
        }
    }
}

package com.nononsenseapps.feeder.db.room

import androidx.room.testing.MigrationTestHelper
import androidx.sqlite.db.framework.FrameworkSQLiteOpenHelperFactory
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.LargeTest
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
@LargeTest
class MigrationFrom16To17 {
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
    fun migrate15to16() {
        testHelper.createDatabase(dbName, 16)

        val db = testHelper.runMigrationsAndValidate(dbName, 17, true, MIGRATION_16_17)

        db.query(
            """
            SELECT * FROM sync_remote
            """.trimIndent(),
        ).use {
            assert(it.count == 0)
        }
    }
}

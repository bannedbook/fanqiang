package com.nononsenseapps.feeder.db.room

import android.content.SharedPreferences
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
import org.kodein.di.android.closestDI
import org.kodein.di.instance
import kotlin.test.assertTrue

@RunWith(AndroidJUnit4::class)
@LargeTest
class TestMigrationFrom23To24 {
    private val feederApplication: FeederApplication = ApplicationProvider.getApplicationContext()
    private val di: DI by closestDI(feederApplication)

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
        testHelper.createDatabase(dbName, 23).let {
        }

        val sharedPrefs by di.instance<SharedPreferences>()

        val blockValues = setOf("foo", "bar", "car")

        sharedPrefs.edit()
            .putStringSet("pref_block_list_values", blockValues)
            .apply()

        val remainingValues = mutableSetOf("*foo*", "*bar*", "*car*")

        val db = testHelper.runMigrationsAndValidate(dbName, 24, true, MigrationFrom23To24(di))

        db.query(
            """
            SELECT glob_pattern FROM blocklist
            """.trimIndent(),
        ).use {
            assert(it.count == 3)

            assert(it.moveToFirst())

            assertTrue(it.getString(0)) {
                val value = it.getString(0)
                (value in remainingValues).also { remainingValues.remove(value) }
            }

            assert(it.moveToNext())
            assertTrue(it.getString(0)) {
                val value = it.getString(0)
                (value in remainingValues).also { remainingValues.remove(value) }
            }

            assert(it.moveToNext())
            assertTrue(it.getString(0)) {
                val value = it.getString(0)
                (value in remainingValues).also { remainingValues.remove(value) }
            }
        }

        val blocks =
            sharedPrefs.getStringSet("pref_block_list_values", null)
                ?: emptySet()

        assertTrue {
            blocks.isEmpty()
        }
    }
}

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
import kotlin.test.assertFalse
import kotlin.test.assertTrue

@RunWith(AndroidJUnit4::class)
@LargeTest
class TestMigrationFrom24To25 : DIAware {
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
        testHelper.createDatabase(dbName, 24).let { oldDB ->
            oldDB.execSQL(
                """
                INSERT INTO feeds(id, title, url, custom_title, tag, notify, last_sync, response_hash, fulltext_by_default, open_articles_with, alternate_id, currently_syncing, when_modified)
                VALUES(1, 'feed', 'http://url', '', '', 0, 0, 666, 0, '', 0, 0, 0)
                """.trimIndent(),
            )
            oldDB.execSQL(
                """
                INSERT INTO feed_items(id, guid, title, plain_title, plain_snippet, unread, notified, feed_id, first_synced_time, primary_sort_time, pinned, bookmarked)
                VALUES(8, 'http://item', 'title', 'ptitle', 'psnippet', 1, 0, 1, 0, 0, 0, 0)
                """.trimIndent(),
            )
        }

        // Test files here
        val name1 = "1.txt.gz"
        val name2 = "2.txt.gz"
        val fullName1 = "1.full.html.gz"
        val fullName2 = "2.full.html.gz"

        assertTrue {
            feederApplication.filesDir.resolve(name1).createNewFile()
        }

        assertTrue {
            feederApplication.filesDir.resolve(name2).createNewFile()
        }

        assertTrue {
            feederApplication.filesDir.resolve(fullName1).createNewFile()
        }

        assertTrue {
            feederApplication.filesDir.resolve(fullName2).createNewFile()
        }

        val db = testHelper.runMigrationsAndValidate(dbName, 25, true, MigrationFrom24To25(di))

        db.query(
            """
            SELECT fulltext_downloaded FROM feed_items
            """.trimIndent(),
        ).use {
            assert(it.count == 1)
            assert(it.moveToFirst())
            assertEquals(0, it.getInt(0))
        }

        assertFalse {
            feederApplication.cacheDir.resolve("full_articles/$fullName1").exists()
        }

        assertFalse {
            feederApplication.cacheDir.resolve("full_articles/$fullName2").exists()
        }

        assertTrue {
            feederApplication.cacheDir.resolve("articles/$name1").isFile
        }

        assertTrue {
            feederApplication.cacheDir.resolve("articles/$name2").isFile
        }
    }
}

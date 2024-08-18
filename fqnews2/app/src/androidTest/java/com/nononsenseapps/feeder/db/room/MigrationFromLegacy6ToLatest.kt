package com.nononsenseapps.feeder.db.room

import androidx.room.Room
import androidx.room.testing.MigrationTestHelper
import androidx.sqlite.db.framework.FrameworkSQLiteOpenHelperFactory
import androidx.test.core.app.ApplicationProvider.getApplicationContext
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.LargeTest
import androidx.test.platform.app.InstrumentationRegistry
import jww.app.FeederApplication
import com.nononsenseapps.feeder.db.legacy.COL_AUTHOR
import com.nononsenseapps.feeder.db.legacy.COL_CUSTOM_TITLE
import com.nononsenseapps.feeder.db.legacy.COL_DESCRIPTION
import com.nononsenseapps.feeder.db.legacy.COL_ENCLOSURELINK
import com.nononsenseapps.feeder.db.legacy.COL_FEED
import com.nononsenseapps.feeder.db.legacy.COL_FEEDTITLE
import com.nononsenseapps.feeder.db.legacy.COL_FEEDURL
import com.nononsenseapps.feeder.db.legacy.COL_GUID
import com.nononsenseapps.feeder.db.legacy.COL_IMAGEURL
import com.nononsenseapps.feeder.db.legacy.COL_LINK
import com.nononsenseapps.feeder.db.legacy.COL_NOTIFIED
import com.nononsenseapps.feeder.db.legacy.COL_NOTIFY
import com.nononsenseapps.feeder.db.legacy.COL_PLAINSNIPPET
import com.nononsenseapps.feeder.db.legacy.COL_PLAINTITLE
import com.nononsenseapps.feeder.db.legacy.COL_PUBDATE
import com.nononsenseapps.feeder.db.legacy.COL_TAG
import com.nononsenseapps.feeder.db.legacy.COL_TITLE
import com.nononsenseapps.feeder.db.legacy.COL_UNREAD
import com.nononsenseapps.feeder.db.legacy.COL_URL
import com.nononsenseapps.feeder.db.legacy.CREATE_FEED_ITEM_TABLE
import com.nononsenseapps.feeder.db.legacy.CREATE_FEED_TABLE
import com.nononsenseapps.feeder.db.legacy.FEED_ITEM_TABLE_NAME
import com.nononsenseapps.feeder.db.legacy.FEED_TABLE_NAME
import com.nononsenseapps.feeder.db.legacy.LegacyDatabaseHandler
import com.nononsenseapps.feeder.db.legacy.createViewsAndTriggers
import com.nononsenseapps.feeder.util.DoNotUseInProd
import com.nononsenseapps.feeder.util.contentValues
import com.nononsenseapps.feeder.util.setInt
import com.nononsenseapps.feeder.util.setLong
import com.nononsenseapps.feeder.util.setString
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.kodein.di.DI
import org.kodein.di.android.closestDI
import java.net.URL
import java.time.Instant
import java.time.ZoneOffset
import java.time.ZonedDateTime

@OptIn(DoNotUseInProd::class)
@RunWith(AndroidJUnit4::class)
@LargeTest
class MigrationFromLegacy6ToLatest {
    private val feederApplication: FeederApplication = getApplicationContext()
    private val di: DI by closestDI(feederApplication)

    @Rule
    @JvmField
    val testHelper: MigrationTestHelper =
        MigrationTestHelper(
            InstrumentationRegistry.getInstrumentation(),
            AppDatabase::class.java,
            emptyList(),
            FrameworkSQLiteOpenHelperFactory(),
        )

    private val testDbName = "TestingDatabase"

    private val legacyDb: LegacyDatabaseHandler
        get() =
            LegacyDatabaseHandler(
                context = feederApplication,
                name = testDbName,
                version = 6,
            )

    private val roomDb: AppDatabase
        get() =
            Room.databaseBuilder(
                feederApplication,
                AppDatabase::class.java,
                testDbName,
            )
                .addMigrations(*getAllMigrations(di))
                .build().also { testHelper.closeWhenFinished(it) }

    @Before
    fun setup() {
        legacyDb.writableDatabase.use { db ->
            db.execSQL(CREATE_FEED_TABLE)
            db.execSQL(CREATE_FEED_ITEM_TABLE)
            createViewsAndTriggers(db)

            // Bare minimum non-null feeds
            val idA =
                db.insert(
                    FEED_TABLE_NAME,
                    null,
                    contentValues {
                        setString(COL_TITLE to "feedA")
                        setString(COL_CUSTOM_TITLE to "feedACustom")
                        setString(COL_URL to "https://feedA")
                        setString(COL_TAG to "")
                    },
                )

            // All fields filled
            val idB =
                db.insert(
                    FEED_TABLE_NAME,
                    null,
                    contentValues {
                        setString(COL_TITLE to "feedB")
                        setString(COL_CUSTOM_TITLE to "feedBCustom")
                        setString(COL_URL to "https://feedB")
                        setString(COL_TAG to "tag")
                        setString(COL_IMAGEURL to "https://image")
                        setInt(COL_NOTIFY to 1)
                    },
                )

            IntRange(0, 1).forEach { index ->
                db.insert(
                    FEED_ITEM_TABLE_NAME,
                    null,
                    contentValues {
                        setLong(COL_FEED to idA)
                        setString(COL_GUID to "guid$index")
                        setString(COL_TITLE to "title$index")
                        setString(COL_DESCRIPTION to "desc$index")
                        setString(COL_PLAINTITLE to "plain$index")
                        setString(COL_PLAINSNIPPET to "snippet$index")
                        setString(COL_FEEDTITLE to "feedA")
                        setString(COL_FEEDURL to "https://feedA")
                        setString(COL_TAG to "")
                    },
                )

                db.insert(
                    FEED_ITEM_TABLE_NAME,
                    null,
                    contentValues {
                        setLong(COL_FEED to idB)
                        setString(COL_GUID to "guid$index")
                        setString(COL_TITLE to "title$index")
                        setString(COL_DESCRIPTION to "desc$index")
                        setString(COL_PLAINTITLE to "plain$index")
                        setString(COL_PLAINSNIPPET to "snippet$index")
                        setString(COL_FEEDTITLE to "feedB")
                        setString(COL_FEEDURL to "https://feedB")
                        setString(COL_TAG to "tag")
                        setInt(COL_NOTIFIED to 1)
                        setInt(COL_UNREAD to 0)
                        setString(COL_AUTHOR to "author$index")
                        setString(COL_ENCLOSURELINK to "https://enclosure$index")
                        setString(COL_IMAGEURL to "https://image$index")
                        setString(COL_PUBDATE to "2018-02-03T04:05:00Z")
                        setString(COL_LINK to "https://link$index")
                    },
                )
            }
        }
    }

    @After
    fun tearDown() {
        assertTrue(feederApplication.deleteDatabase(testDbName))
    }

    @Test
    fun legacyMigrationTo7MinimalFeed() =
        runBlocking {
            testHelper.runMigrationsAndValidate(
                testDbName,
                7,
                true,
                MIGRATION_6_7,
            )

            roomDb.let { db ->
                val feeds = db.feedDao().getAllFeeds()

                assertEquals("Wrong number of feeds", 2, feeds.size)

                val feedA = feeds[0]

                assertEquals("feedA", feedA.title)
                assertEquals("feedACustom", feedA.customTitle)
                assertEquals(URL("https://feedA"), feedA.url)
                assertEquals("", feedA.tag)
                assertEquals(Instant.EPOCH, feedA.lastSync)
                assertFalse(feedA.notify)
                assertNull(feedA.imageUrl)
            }
        }

    @Test
    fun legacyMigrationTo7CompleteFeed() =
        runBlocking {
            testHelper.runMigrationsAndValidate(
                testDbName,
                7,
                true,
                MIGRATION_6_7,
            )

            roomDb.let { db ->
                val feeds = db.feedDao().getAllFeeds()

                assertEquals("Wrong number of feeds", 2, feeds.size)

                val feedB = feeds[1]

                assertEquals("feedB", feedB.title)
                assertEquals("feedBCustom", feedB.customTitle)
                assertEquals(URL("https://feedB"), feedB.url)
                assertEquals("tag", feedB.tag)
                assertEquals(Instant.EPOCH, feedB.lastSync)
                assertTrue(feedB.notify)
                assertEquals(URL("https://image"), feedB.imageUrl)
            }
        }

    @Test
    fun legacyMigrationTo7MinimalFeedItem() =
        runBlocking {
            testHelper.runMigrationsAndValidate(
                testDbName,
                7,
                true,
                MIGRATION_6_7,
            )

            roomDb.let { db ->
                val feed = db.feedDao().getAllFeeds()[0]
                assertEquals("feedA", feed.title)
                val items = db.feedItemDao().loadFeedItemsInFeedDesc(feedId = feed.id)

                assertEquals(2, items.size)

                items.forEachIndexed { index, it ->
                    assertEquals(feed.id, it.feedId)
                    assertEquals("guid$index", it.guid)
                    assertEquals("plain$index", it.plainTitle)
                    assertEquals("plain$index", it.plainTitle)
                    assertEquals("snippet$index", it.plainSnippet)
                    assertTrue(it.unread)
                    assertNull(it.author)
                    assertNull(it.enclosureLink)
                    assertNull(it.thumbnailImage)
                    assertNull(it.pubDate)
                    assertNull(it.link)
                    assertFalse(it.notified)
                }
            }
        }

    @Test
    fun legacyMigrationTo7CompleteFeedItem() =
        runBlocking {
            testHelper.runMigrationsAndValidate(
                testDbName,
                7,
                true,
                MIGRATION_6_7,
            )

            roomDb.let { db ->
                val feed = db.feedDao().getAllFeeds()[1]
                assertEquals("feedB", feed.title)
                val items = db.feedItemDao().loadFeedItemsInFeedDesc(feedId = feed.id)

                assertEquals(2, items.size)

                items.forEachIndexed { index, it ->
                    assertEquals(feed.id, it.feedId)
                    assertEquals("guid$index", it.guid)
                    assertEquals("plain$index", it.plainTitle)
                    assertEquals("plain$index", it.plainTitle)
                    assertEquals("snippet$index", it.plainSnippet)
                    assertFalse(it.unread)
                    assertEquals("author$index", it.author)
                    assertEquals("https://enclosure$index", it.enclosureLink)
                    assertEquals("https://image$index", it.thumbnailImage?.url)
                    assertEquals(ZonedDateTime.of(2018, 2, 3, 4, 5, 0, 0, ZoneOffset.UTC), it.pubDate)
                    assertEquals("https://link$index", it.link)
                    assertTrue(it.notified)
                }
            }
        }
}

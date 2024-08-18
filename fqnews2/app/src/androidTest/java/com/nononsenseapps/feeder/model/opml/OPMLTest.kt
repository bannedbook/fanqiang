package com.nononsenseapps.feeder.model.opml

import android.content.ContentResolver
import android.content.Context
import android.content.SharedPreferences
import androidx.core.net.toUri
import androidx.preference.PreferenceManager
import androidx.room.Room
import androidx.test.core.app.ApplicationProvider.getApplicationContext
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.MediumTest
import androidx.test.filters.SmallTest
import androidx.work.WorkManager
import com.nononsenseapps.feeder.archmodel.FeedStore
import com.nononsenseapps.feeder.archmodel.PREF_VAL_OPEN_WITH_CUSTOM_TAB
import com.nononsenseapps.feeder.archmodel.SettingsStore
import com.nononsenseapps.feeder.archmodel.UserSettings
import com.nononsenseapps.feeder.db.room.AppDatabase
import com.nononsenseapps.feeder.db.room.BlocklistDao
import com.nononsenseapps.feeder.db.room.Feed
import com.nononsenseapps.feeder.db.room.FeedDao
import com.nononsenseapps.feeder.db.room.OPEN_ARTICLE_WITH_APPLICATION_DEFAULT
import com.nononsenseapps.feeder.model.OPMLParserHandler
import com.nononsenseapps.feeder.util.Either
import com.nononsenseapps.feeder.util.ToastMaker
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Assert.fail
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.bind
import org.kodein.di.instance
import org.kodein.di.singleton
import java.io.File
import java.io.IOException
import java.net.URL
import kotlin.random.Random

@RunWith(AndroidJUnit4::class)
class OPMLTest : DIAware {
    private val context: Context = getApplicationContext()
    lateinit var db: AppDatabase
    override val di =
        DI.lazy {
            bind<SharedPreferences>() with
                singleton {
                    PreferenceManager.getDefaultSharedPreferences(
                        this@OPMLTest.context,
                    )
                }
            bind<AppDatabase>() with instance(db)
            bind<FeedDao>() with singleton { db.feedDao() }
            bind<BlocklistDao>() with singleton { db.blocklistDao() }
            bind<SettingsStore>() with singleton { SettingsStore(di) }
            bind<FeedStore>() with singleton { FeedStore(di) }
            bind<OPMLParserHandler>() with singleton { OPMLImporter(di) }
            bind<WorkManager>() with singleton { WorkManager.getInstance(this@OPMLTest.context) }
            bind<ToastMaker>() with
                instance(
                    object : ToastMaker {
                        override suspend fun makeToast(text: String) {}

                        override suspend fun makeToast(resId: Int) {}
                    },
                )
            bind<ContentResolver>() with singleton { this@OPMLTest.context.contentResolver }
        }

    private var dir: File? = null
    private var path: File? = null

    private val opmlParserHandler: OPMLParserHandler by instance()
    private val settingsStore: SettingsStore by instance()
    private val feedStore: FeedStore by instance()

    @Before
    fun setup() {
        // Get internal data dir
        dir = context.externalCacheDir!!.resolve("${Random.nextInt()}").also { it.mkdir() }
        path = context.externalCacheDir!!.resolve("${Random.nextInt()}").also { it.createNewFile() }
        assertTrue("Need to be able to write to data dir $dir", dir!!.canWrite())

        db = Room.inMemoryDatabaseBuilder(context, AppDatabase::class.java).build()
    }

    @After
    fun tearDown() {
        // Remove everything in database
    }

    @MediumTest
    @Test
    fun testWrite() =
        runBlocking {
            // Create some feeds
            createSampleFeeds()

            writeFile(
                path = path!!.absolutePath,
                settings = ALL_SETTINGS_WITH_VALUES,
                blockedPatterns = BLOCKED_PATTERNS,
                tags = getTags(),
            ) { tag ->
                db.feedDao().getFeedsByTitle(tag = tag)
            }

            // check contents of file
            path!!.bufferedReader().useLines { lines ->
                lines.forEachIndexed { i, line ->
                    assertEquals("line $i differed", sampleFile[i], line)
                }
            }
        }

    @MediumTest
    @Test
    fun testReadSettings() =
        runBlocking {
            writeSampleFile()

            val parser = OpmlPullParser(opmlParserHandler)
            parser.parseFile(path!!.canonicalPath)

            // Verify database is correct
            val actual = settingsStore.getAllSettings()

            ALL_SETTINGS_WITH_VALUES.forEach { (key, expected) ->
                assertEquals(expected, actual[key].toString())
            }

            val actualBlocked = settingsStore.blockListPreference.first()

            assertEquals(1, actualBlocked.size)
            assertEquals("foo", actualBlocked.first())
        }

    @MediumTest
    @Test
    fun testRead() =
        runBlocking {
            writeSampleFile()

            val parser = OpmlPullParser(opmlParserHandler)
            parser.parseFile(path!!.canonicalPath)

            // Verify database is correct
            val seen = ArrayList<Int>()
            val feeds = db.feedDao().getAllFeeds()
            assertFalse("No feeds in DB!", feeds.isEmpty())
            for (feed in feeds) {
                val i = Integer.parseInt(feed.title.replace("[custom \"]".toRegex(), ""))
                seen.add(i)
                assertEquals("URL doesn't match", URL("http://example.com/$i/rss.xml"), feed.url)

                when (i) {
                    0 -> {
                        assertEquals("title should be the same", "\"$i\"", feed.title)
                        assertEquals(
                            "custom title should have been set to title",
                            "\"$i\"",
                            feed.customTitle,
                        )
                    }

                    else -> {
                        assertEquals(
                            "custom title should have overridden title",
                            "custom \"$i\"",
                            feed.title,
                        )
                        assertEquals(
                            "title and custom title should match",
                            feed.customTitle,
                            feed.title,
                        )
                    }
                }

                when {
                    i % 3 == 1 -> assertEquals("tag1", feed.tag)
                    i % 3 == 2 -> assertEquals("tag2", feed.tag)
                    else -> assertEquals("", feed.tag)
                }
            }
            for (i in 0..9) {
                assertTrue("Missing $i", seen.contains(i))
            }
        }

    @MediumTest
    @Test
    fun testReadExisting() =
        runBlocking {
            writeSampleFile()

            // Create something that does not exist
            var feednew =
                Feed(
                    url = URL("http://example.com/20/rss.xml"),
                    title = "\"20\"",
                    tag = "kapow",
                )
            var id = db.feedDao().insertFeed(feednew)
            feednew = feednew.copy(id = id)
            // Create something that will exist
            var feedold =
                Feed(
                    url = URL("http://example.com/0/rss.xml"),
                    title = "\"0\"",
                )
            id = db.feedDao().insertFeed(feedold)

            feedold = feedold.copy(id = id)

            // Read file
            val parser = OpmlPullParser(opmlParserHandler)
            parser.parseFile(path!!.canonicalPath)

            // should not kill the existing stuff
            val seen = ArrayList<Int>()
            val feeds = db.feedDao().getAllFeeds()
            assertFalse("No feeds in DB!", feeds.isEmpty())
            for (feed in feeds) {
                val i = Integer.parseInt(feed.title.replace("[custom \"]".toRegex(), ""))
                seen.add(i)
                assertEquals(URL("http://example.com/$i/rss.xml"), feed.url)

                when {
                    i == 20 -> {
                        assertEquals("Should not have changed", feednew.id, feed.id)
                        assertEquals("Should not have changed", feednew.url, feed.url)
                        assertEquals("Should not have changed", feednew.tag, feed.tag)
                    }

                    i % 3 == 1 -> assertEquals("tag1", feed.tag)
                    i % 3 == 2 -> assertEquals("tag2", feed.tag)
                    else -> assertEquals("", feed.tag)
                }

                // Ensure titles are correct
                when (i) {
                    0 -> {
                        assertEquals("title should be the same", feedold.title, feed.title)
                        assertEquals(
                            "custom title should have been set to title",
                            feedold.title,
                            feed.customTitle,
                        )
                    }

                    20 -> {
                        assertEquals(
                            "feed not present in OPML should not have changed",
                            feednew.title,
                            feed.title,
                        )
                        assertEquals(
                            "feed not present in OPML should not have changed",
                            feednew.customTitle,
                            feednew.customTitle,
                        )
                    }

                    else -> {
                        assertEquals(
                            "custom title should have overridden title",
                            "custom \"$i\"",
                            feed.title,
                        )
                        assertEquals(
                            "title and custom title should match",
                            feed.customTitle,
                            feed.title,
                        )
                    }
                }

                if (i == 0) {
                    // Make sure id is same as old
                    assertEquals("Id should be same still", feedold.id, feed.id)

                    assertTrue("Notify is wrong", feed.notify)
                    assertTrue("AlternateId is wrong", feed.alternateId)
                    assertTrue("FullTextByDefault is wrong", feed.fullTextByDefault)
                    assertEquals("OpenArticlesWith is wrong", "reader", feed.openArticlesWith)
                    assertEquals(
                        "ImageURL is wrong",
                        URL("https://example.com/feedImage.png"),
                        feed.imageUrl,
                    )
                }
            }
            assertTrue("Missing 20", seen.contains(20))
            for (i in 0..9) {
                assertTrue("Missing $i", seen.contains(i))
            }
        }

    @MediumTest
    @Test
    fun testReadBadFile() =
        runBlocking {
            path!!.bufferedWriter().use {
                it.write("This is just some bullshit in the file\n")
            }

            // Read file
            val parser = OpmlPullParser(opmlParserHandler)
            parser.parseFile(path!!.absolutePath)

            val feeds = db.feedDao().getAllFeeds()
            assertTrue("Expected no feeds and no exception", feeds.isEmpty())
        }

    @SmallTest
    @Test
    fun testReadMissingFile() =
        runBlocking {
            val path = File(dir, "lsadflibaslsdfa.opml")
            // Read file
            val parser = OpmlPullParser(opmlParserHandler)
            val result = parser.parseFile(path.absolutePath)

            assertTrue(result.isLeft())
        }

    @Throws(IOException::class)
    private fun writeSampleFile() =
        runBlocking {
            // Use test write to write the sample file
            testWrite()
            // Then delete all feeds again
            db.runInTransaction {
                runBlocking {
                    db.feedDao().getAllFeeds().forEach {
                        db.feedDao().deleteFeed(it)
                    }
                }
            }
        }

    private suspend fun createSampleFeeds() {
        for (i in 0..9) {
            val feed =
                Feed(
                    url = URL("http://example.com/$i/rss.xml"),
                    title = "\"$i\"",
                    customTitle = if (i == 0) "" else "custom \"$i\"",
                    tag =
                        when (i % 3) {
                            1 -> "tag1"
                            2 -> "tag2"
                            else -> ""
                        },
                    notify = i == 0,
                    alternateId = i == 0,
                    fullTextByDefault = i == 0,
                    imageUrl =
                        if (i == 0) {
                            URL("https://example.com/feedImage.png")
                        } else {
                            null
                        },
                    openArticlesWith =
                        if (i == 0) {
                            "reader"
                        } else {
                            OPEN_ARTICLE_WITH_APPLICATION_DEFAULT
                        },
                )

            db.feedDao().insertFeed(feed)
        }
    }

    private suspend fun getTags(): List<String> = db.feedDao().loadTags()

    @Test
    @MediumTest
    fun antennaPodOPMLImports() =
        runBlocking {
            // given
            val opmlStream = this@OPMLTest.javaClass.getResourceAsStream("antennapod-feeds.opml")!!

            // when
            val parser = OpmlPullParser(opmlParserHandler)
            parser.parseInputStreamWithFallback(opmlStream)

            // then
            val feeds = db.feedDao().getAllFeeds()
            val tags = db.feedDao().loadTags()
            assertEquals("Expecting 8 feeds", 8, feeds.size)
            assertEquals("Expecting 1 tags (incl empty)", 1, tags.size)

            feeds.forEach { feed ->
                assertEquals("No tag expected", "", feed.tag)
                when (feed.url) {
                    URL("http://aliceisntdead.libsyn.com/rss") -> {
                        assertEquals("Alice Isn't Dead", feed.title)
                    }

                    URL("http://feeds.soundcloud.com/users/soundcloud:users:154104768/sounds.rss") -> {
                        assertEquals("Invisible City", feed.title)
                    }

                    URL("http://feeds.feedburner.com/PodCastle_Main") -> {
                        assertEquals("PodCastle", feed.title)
                    }

                    URL("http://www.artofstorytellingshow.com/podcast/storycast.xml") -> {
                        assertEquals("The Art of Storytelling with Brother Wolf", feed.title)
                    }

                    URL("http://feeds.feedburner.com/TheCleansed") -> {
                        assertEquals("The Cleansed: A Post-Apocalyptic Saga", feed.title)
                    }

                    URL("http://media.signumuniversity.org/tolkienprof/feed") -> {
                        assertEquals("The Tolkien Professor", feed.title)
                    }

                    URL("http://nightvale.libsyn.com/rss") -> {
                        assertEquals("Welcome to Night Vale", feed.title)
                    }

                    URL("http://withinthewires.libsyn.com/rss") -> {
                        assertEquals("Within the Wires", feed.title)
                    }

                    else -> fail("Unexpected URI. Feed: $feed")
                }
            }
        }

    @Test
    @MediumTest
    fun flymOPMLImports() =
        runBlocking {
            // given
            val opmlStream = this@OPMLTest.javaClass.getResourceAsStream("Flym_auto_backup.opml")!!

            // when
            val parser = OpmlPullParser(opmlParserHandler)
            parser.parseInputStreamWithFallback(opmlStream)

            // then
            val feeds = db.feedDao().getAllFeeds()
            val tags = db.feedDao().loadTags()
            assertEquals("Expecting 11 feeds", 11, feeds.size)
            assertEquals("Expecting 4 tags (incl empty)", 4, tags.size)

            feeds.forEach { feed ->
                when (feed.url) {
                    URL("http://www.smbc-comics.com/rss.php") -> {
                        assertEquals("black humor", feed.tag)
                        assertEquals("SMBC", feed.customTitle)
                        assertFalse(feed.fullTextByDefault)
                    }

                    URL("http://www.deathbulge.com/rss.xml") -> {
                        assertEquals("black humor", feed.tag)
                        assertEquals("Deathbulge", feed.customTitle)
                        assertTrue(feed.fullTextByDefault)
                    }

                    URL("http://www.sandraandwoo.com/gaia/feed/") -> {
                        assertEquals("comics", feed.tag)
                        assertEquals("Gaia", feed.customTitle)
                        assertFalse(feed.fullTextByDefault)
                    }

                    URL("http://replaycomic.com/feed/") -> {
                        assertEquals("comics", feed.tag)
                        assertEquals("Replay", feed.customTitle)
                        assertTrue(feed.fullTextByDefault)
                    }

                    URL("http://www.cuttimecomic.com/rss.php") -> {
                        assertEquals("comics", feed.tag)
                        assertEquals("Cut Time", feed.customTitle)
                        assertFalse(feed.fullTextByDefault)
                    }

                    URL("http://www.commitstrip.com/feed/") -> {
                        assertEquals("comics", feed.tag)
                        assertEquals("Commit strip", feed.customTitle)
                        assertTrue(feed.fullTextByDefault)
                    }

                    URL("http://www.sandraandwoo.com/feed/") -> {
                        assertEquals("comics", feed.tag)
                        assertEquals("Sandra and Woo", feed.customTitle)
                        assertFalse(feed.fullTextByDefault)
                    }

                    URL("http://www.awakencomic.com/rss.php") -> {
                        assertEquals("comics", feed.tag)
                        assertEquals("Awaken", feed.customTitle)
                        assertTrue(feed.fullTextByDefault)
                    }

                    URL("http://www.questionablecontent.net/QCRSS.xml") -> {
                        assertEquals("comics", feed.tag)
                        assertEquals("Questionable Content", feed.customTitle)
                        assertFalse(feed.fullTextByDefault)
                    }

                    URL("https://www.archlinux.org/feeds/news/") -> {
                        assertEquals("Tech", feed.tag)
                        assertEquals("Arch news", feed.customTitle)
                        assertFalse(feed.fullTextByDefault)
                    }

                    URL("https://grisebouille.net/feed/") -> {
                        assertEquals("Political humour", feed.tag)
                        assertEquals("Grisebouille", feed.customTitle)
                        assertTrue(feed.fullTextByDefault)
                    }

                    else -> fail("Unexpected URI. Feed: $feed")
                }
            }
        }

    @Test
    @MediumTest
    fun rssGuardOPMLImports1() =
        runBlocking {
            // given
            val opmlStream = this@OPMLTest.javaClass.getResourceAsStream("rssguard_1.opml")!!

            // when
            val parser = OpmlPullParser(opmlParserHandler)
            parser.parseInputStreamWithFallback(opmlStream)

            // then
            val feeds = db.feedDao().getAllFeeds()
            val tags = db.feedDao().loadTags()
            assertEquals("Expecting 30 feeds", 30, feeds.size)
            assertEquals("Expecting 6 tags (incl empty)", 6, tags.size)

            feeds.forEach { feed ->
                when (feed.url) {
                    URL("http://www.les-trois-sagesses.org/rss-articles.xml") -> {
                        assertEquals("Religion", feed.tag)
                        assertEquals("Les trois sagesses", feed.customTitle)
                    }

                    URL("http://www.avrildeperthuis.com/feed/") -> {
                        assertEquals("Amis", feed.tag)
                        assertEquals("avril de perthuis", feed.customTitle)
                    }

                    URL("http://www.fashioningtech.com/profiles/blog/feed?xn_auth=no") -> {
                        assertEquals("Actu Geek", feed.tag)
                        assertEquals("Everyone's Blog Posts - Fashioning Technology", feed.customTitle)
                    }

                    URL("http://feeds2.feedburner.com/ChartPorn") -> {
                        assertEquals("Graphs", feed.tag)
                        assertEquals("Chart Porn", feed.customTitle)
                    }

                    URL("http://www.mosqueedeparis.net/index.php?format=feed&amp;type=atom") -> {
                        assertEquals("Religion", feed.tag)
                        assertEquals("Mosquee de Paris", feed.customTitle)
                    }

                    URL("http://sourceforge.net/projects/stuntrally/rss") -> {
                        assertEquals("Mainstream update", feed.tag)
                        assertEquals("Stunt Rally", feed.customTitle)
                    }

                    URL("http://www.mairie6.lyon.fr/cs/Satellite?Thematique=&TypeContenu=Actualite&pagename=RSSFeed&site=Mairie6") -> {
                        assertEquals("", feed.tag)
                        assertEquals("Actualités", feed.customTitle)
                    }
                }
            }
        }

    @MediumTest
    @Test
    fun testExportThenImport(): Unit =
        runBlocking {
            val fileUri = context.cacheDir.resolve("exporttest.opml").toUri()
            val feedIds = mutableSetOf<Long>()
            feedStore.saveFeed(
                Feed(
                    title = "Ampersands are & the worst",
                    url = URL("https://example.com/ampersands"),
                ),
            ).also { feedIds.add(it) }
            feedStore.saveFeed(
                Feed(
                    title = "So are > brackets",
                    url = URL("https://example.com/lt"),
                ),
            ).also { feedIds.add(it) }
            feedStore.saveFeed(
                Feed(
                    title = "So are < brackets",
                    url = URL("https://example.com/gt"),
                ),
            ).also { feedIds.add(it) }

            assertEquals(3, feedIds.size)

            val exportResult = exportOpml(di, fileUri)

            exportResult.leftOrNull()?.let { e ->
                throw e.throwable!!
            }

            val opmlFeedList = OpmlFeedList()
            val parser = OpmlPullParser(opmlFeedList)
            val result = parser.parseFile(fileUri.path!!)

            result.leftOrNull()?.let { e ->
                throw e.throwable!!
            }

            assertEquals(3, opmlFeedList.feeds.size)
        }

    @Test
    @MediumTest
    fun importPlenaryProgramming(): Unit =
        runBlocking {
            // given
            val opmlStream = this@OPMLTest.javaClass.getResourceAsStream("Programming.opml")!!

            // when
            val opmlFeedList = OpmlFeedList()
            val parser = OpmlPullParser(opmlFeedList)
            val result = parser.parseInputStreamWithFallback(opmlStream)

            result.leftOrNull()?.let {
                throw it.throwable!!
            }

            // then
            assertEquals("Expecting feeds", 50, opmlFeedList.feeds.size)
        }

    @Test
    @MediumTest
    fun rssGuardOPMLImports2() =
        runBlocking {
            // given
            val opmlStream = this@OPMLTest.javaClass.getResourceAsStream("rssguard_2.opml")!!

            // when
            val parser = OpmlPullParser(opmlParserHandler)
            parser.parseInputStreamWithFallback(opmlStream)

            // then
            val feeds = db.feedDao().getAllFeeds()
            val tags = db.feedDao().loadTags()
            assertEquals("Expecting 30 feeds", 30, feeds.size)
            assertEquals("Expecting 6 tags (incl empty)", 6, tags.size)

            feeds.forEach { feed ->
                when (feed.url) {
                    URL("http://www.les-trois-sagesses.org/rss-articles.xml") -> {
                        assertEquals("Religion", feed.tag)
                        assertEquals("Les trois sagesses", feed.customTitle)
                    }

                    URL("http://www.avrildeperthuis.com/feed/") -> {
                        assertEquals("Amis", feed.tag)
                        assertEquals("avril de perthuis", feed.customTitle)
                    }

                    URL("http://www.fashioningtech.com/profiles/blog/feed?xn_auth=no") -> {
                        assertEquals("Actu Geek", feed.tag)
                        assertEquals("Everyone's Blog Posts - Fashioning Technology", feed.customTitle)
                    }

                    URL("http://feeds2.feedburner.com/ChartPorn") -> {
                        assertEquals("Graphs", feed.tag)
                        assertEquals("Chart Porn", feed.customTitle)
                    }

                    URL("http://www.mosqueedeparis.net/index.php?format=feed&amp;type=atom") -> {
                        assertEquals("Religion", feed.tag)
                        assertEquals("Mosquee de Paris", feed.customTitle)
                    }

                    URL("http://sourceforge.net/projects/stuntrally/rss") -> {
                        assertEquals("Mainstream update", feed.tag)
                        assertEquals("Stunt Rally", feed.customTitle)
                    }

                    URL("http://www.mairie6.lyon.fr/cs/Satellite?Thematique=&TypeContenu=Actualite&pagename=RSSFeed&site=Mairie6") -> {
                        assertEquals("", feed.tag)
                        assertEquals("Actualités", feed.customTitle)
                    }
                }
            }
        }

    companion object {
        private val BLOCKED_PATTERNS: List<String> = listOf("foo")
        private val ALL_SETTINGS_WITH_VALUES: Map<String, String> =
            UserSettings.values().associate { userSetting ->
                userSetting.key to
                    when (userSetting) {
                        UserSettings.SETTING_OPEN_LINKS_WITH -> PREF_VAL_OPEN_WITH_CUSTOM_TAB
                        UserSettings.SETTING_ADDED_FEEDER_NEWS -> "true"
                        UserSettings.SETTING_THEME -> "night"
                        UserSettings.SETTING_DARK_THEME -> "dark"
                        UserSettings.SETTING_DYNAMIC_THEME -> "false"
                        UserSettings.SETTING_SORT -> "oldest_first"
                        UserSettings.SETTING_SHOW_FAB -> "false"
                        UserSettings.SETTING_FEED_ITEM_STYLE -> "SUPER_COMPACT"
                        UserSettings.SETTING_SWIPE_AS_READ -> "DISABLED"
                        UserSettings.SETTING_SYNC_ON_RESUME -> "true"
                        UserSettings.SETTING_SYNC_ONLY_WIFI -> "false"
                        UserSettings.SETTING_IMG_ONLY_WIFI -> "true"
                        UserSettings.SETTING_IMG_SHOW_THUMBNAILS -> "false"
                        UserSettings.SETTING_DEFAULT_OPEN_ITEM_WITH -> PREF_VAL_OPEN_WITH_CUSTOM_TAB
                        UserSettings.SETTING_TEXT_SCALE -> "1.6"
                        UserSettings.SETTING_IS_MARK_AS_READ_ON_SCROLL -> "true"
                        UserSettings.SETTING_READALOUD_USE_DETECT_LANGUAGE -> "true"
                        UserSettings.SETTING_SYNC_ONLY_CHARGING -> "true"
                        UserSettings.SETTING_SYNC_FREQ -> "720"
                        UserSettings.SETTING_MAX_LINES -> "6"
                        UserSettings.SETTINGS_FILTER_SAVED -> "true"
                        UserSettings.SETTINGS_FILTER_RECENTLY_READ -> "true"
                        UserSettings.SETTINGS_FILTER_READ -> "false"
                        UserSettings.SETTINGS_LIST_SHOW_ONLY_TITLES -> "true"
                        UserSettings.SETTING_OPEN_ADJACENT -> "true"
                    }
            }
    }
}

suspend fun OpmlPullParser.parseFile(path: String): Either<OpmlError, Unit> {
    return try {
        File(path).inputStream().use {
            parseInputStreamWithFallback(it)
        }
    } catch (t: Throwable) {
        Either.Left(OpmlUnknownError(t))
    }
}

private val sampleFile: List<String> =
    """
    <?xml version="1.0" encoding="UTF-8"?>
    <opml version="1.1" xmlns:feeder="$OPML_FEEDER_NAMESPACE">
      <head>
        <title>
          Feeder
        </title>
      </head>
      <body>
        <outline feeder:notify="true" feeder:imageUrl="https://example.com/feedImage.png" feeder:fullTextByDefault="true" feeder:openArticlesWith="reader" feeder:alternateId="true" title="&quot;0&quot;" text="&quot;0&quot;" type="rss" xmlUrl="http://example.com/0/rss.xml"/>
        <outline feeder:notify="false" feeder:fullTextByDefault="false" feeder:openArticlesWith="" feeder:alternateId="false" title="custom &quot;3&quot;" text="custom &quot;3&quot;" type="rss" xmlUrl="http://example.com/3/rss.xml"/>
        <outline feeder:notify="false" feeder:fullTextByDefault="false" feeder:openArticlesWith="" feeder:alternateId="false" title="custom &quot;6&quot;" text="custom &quot;6&quot;" type="rss" xmlUrl="http://example.com/6/rss.xml"/>
        <outline feeder:notify="false" feeder:fullTextByDefault="false" feeder:openArticlesWith="" feeder:alternateId="false" title="custom &quot;9&quot;" text="custom &quot;9&quot;" type="rss" xmlUrl="http://example.com/9/rss.xml"/>
        <outline title="tag1" text="tag1">
          <outline feeder:notify="false" feeder:fullTextByDefault="false" feeder:openArticlesWith="" feeder:alternateId="false" title="custom &quot;1&quot;" text="custom &quot;1&quot;" type="rss" xmlUrl="http://example.com/1/rss.xml"/>
          <outline feeder:notify="false" feeder:fullTextByDefault="false" feeder:openArticlesWith="" feeder:alternateId="false" title="custom &quot;4&quot;" text="custom &quot;4&quot;" type="rss" xmlUrl="http://example.com/4/rss.xml"/>
          <outline feeder:notify="false" feeder:fullTextByDefault="false" feeder:openArticlesWith="" feeder:alternateId="false" title="custom &quot;7&quot;" text="custom &quot;7&quot;" type="rss" xmlUrl="http://example.com/7/rss.xml"/>
        </outline>
        <outline title="tag2" text="tag2">
          <outline feeder:notify="false" feeder:fullTextByDefault="false" feeder:openArticlesWith="" feeder:alternateId="false" title="custom &quot;2&quot;" text="custom &quot;2&quot;" type="rss" xmlUrl="http://example.com/2/rss.xml"/>
          <outline feeder:notify="false" feeder:fullTextByDefault="false" feeder:openArticlesWith="" feeder:alternateId="false" title="custom &quot;5&quot;" text="custom &quot;5&quot;" type="rss" xmlUrl="http://example.com/5/rss.xml"/>
          <outline feeder:notify="false" feeder:fullTextByDefault="false" feeder:openArticlesWith="" feeder:alternateId="false" title="custom &quot;8&quot;" text="custom &quot;8&quot;" type="rss" xmlUrl="http://example.com/8/rss.xml"/>
        </outline>
        <feeder:settings>
          <feeder:setting key="pref_added_feeder_news" value="true"/>
          <feeder:setting key="pref_theme" value="night"/>
          <feeder:setting key="pref_dark_theme" value="dark"/>
          <feeder:setting key="pref_dynamic_theme" value="false"/>
          <feeder:setting key="pref_sort" value="oldest_first"/>
          <feeder:setting key="pref_show_fab" value="false"/>
          <feeder:setting key="pref_feed_item_style" value="SUPER_COMPACT"/>
          <feeder:setting key="pref_swipe_as_read" value="DISABLED"/>
          <feeder:setting key="pref_sync_only_charging" value="true"/>
          <feeder:setting key="pref_sync_only_wifi" value="false"/>
          <feeder:setting key="pref_sync_freq" value="720"/>
          <feeder:setting key="pref_sync_on_resume" value="true"/>
          <feeder:setting key="pref_img_only_wifi" value="true"/>
          <feeder:setting key="pref_img_show_thumbnails" value="false"/>
          <feeder:setting key="pref_default_open_item_with" value="3"/>
          <feeder:setting key="pref_open_links_with" value="3"/>
          <feeder:setting key="pref_open_adjacent" value="true"/>
          <feeder:setting key="pref_body_text_scale" value="1.6"/>
          <feeder:setting key="pref_is_mark_as_read_on_scroll" value="true"/>
          <feeder:setting key="pref_readaloud_detect_lang" value="true"/>
          <feeder:setting key="pref_max_lines" value="6"/>
          <feeder:setting key="prefs_filter_saved" value="true"/>
          <feeder:setting key="prefs_filter_recently_read" value="true"/>
          <feeder:setting key="prefs_filter_read" value="false"/>
          <feeder:setting key="prefs_list_show_only_titles" value="true"/>
          <feeder:blocked pattern="foo"/>
        </feeder:settings>
      </body>
    </opml>
    """.trimIndent()
        .split("\n")

class OpmlFeedList : OPMLParserHandler {
    val feeds = mutableMapOf<URL, Feed>()
    val settings = mutableMapOf<String, String>()
    val blockList = mutableListOf<String>()

    override suspend fun saveFeed(feed: Feed) {
        feeds[feed.url] = feed
    }

    override suspend fun saveSetting(
        key: String,
        value: String,
    ) {
        settings.put(key, value)
    }

    override suspend fun saveBlocklistPatterns(patterns: Iterable<String>) {
        blockList.addAll(patterns)
    }
}

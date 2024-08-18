@file:Suppress("ktlint:standard:max-line-length", "ktlint:standard:property-naming")

package com.nononsenseapps.feeder.model

import com.nononsenseapps.feeder.di.networkModule
import com.nononsenseapps.feeder.model.gofeed.GoFeedAdapter
import com.nononsenseapps.feeder.util.getOrElse
import com.nononsenseapps.jsonfeed.cachingHttpClient
import kotlinx.coroutines.runBlocking
import okhttp3.MediaType.Companion.toMediaTypeOrNull
import okhttp3.OkHttpClient
import okhttp3.Protocol
import okhttp3.Request
import okhttp3.Response
import okhttp3.ResponseBody
import okhttp3.ResponseBody.Companion.toResponseBody
import org.intellij.lang.annotations.Language
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.bind
import org.kodein.di.instance
import org.kodein.di.singleton
import java.io.BufferedReader
import java.net.URL
import kotlin.test.Ignore
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNull
import kotlin.test.assertTrue

class FeedParserTest : DIAware {
    @Rule
    @JvmField
    var tempFolder = TemporaryFolder()

    private val feedParser: FeedParser by instance()

    override val di by DI.lazy {
        bind<OkHttpClient>() with singleton { cachingHttpClient() }
        import(networkModule)
    }

    private val exp = GoFeedAdapter()

    @Test
    @Ignore
    fun getAlternateLinksHandlesYoutubeOnline() {
        // I want this to be an Online test to make sure that I notice if/when Youtube changes something which breaks it
        runBlocking {
            val feeds =
                feedParser.getSiteMetaData(URL("https://www.youtube.com/watch?v=-m5I_5Vnh6A")).getOrNull()!!.alternateFeedLinks.first()
            assertEquals(
                AlternateLink(
                    URL("https://www.youtube.com/feeds/videos.xml?channel_id=UCG1h-Wqjtwz7uUANw6gazRw"),
                    "atom",
                ),
                feeds,
            )
        }
    }

    @Test
    fun canParseUkrnet() {
        runBlocking {
            readResource("rss_ukrnet.xml") {
//                val feed = exp.parseBody(it)
//

//                assertEquals(20, feed?.items?.size, "Expected 20 items")

                val feed =
                    feedParser.parseFeedResponse(
                        URL("https://suspilne.media/rss/ukrnet.rss"),
                        it,
                    )

                feed.leftOrNull()?.throwable?.let { t ->
                    throw t
                }

                assertEquals(20, feed.getOrNull()?.items?.size, "Expected 20 items")
            }
        }
    }

    @Test
    fun anime2youHasThumbnails() {
        runBlocking {
            readResource("rss_anime2you.xml") {
                val feed =
                    feedParser.parseFeedResponse(
                        URL("https://www.anime2you.de/feed/"),
                        it,
                    )

                val item = feed.getOrNull()?.items!!.first()

                val expected =
                    ImageFromHTML(
                        url = "https://img.anime2you.de/2023/12/jujutsu-kaisen-6.jpg",
                        width = 700,
                        height = 350,
                    )

                assertEquals(
                    expected,
                    item.image,
                    "Thumbnail image was unexpected",
                )
            }
        }
    }

    @Test
    fun dcCreatorEndsUpAsAuthor() =
        runBlocking {
            readResource("openstreetmap.xml") {
                val feed =
                    feedParser.parseFeedResponse(
                        URL("http://https://www.openstreetmap.org/diary/rss"),
                        it,
                    )
                val item = feed.getOrNull()?.items!!.first()

                assertEquals(ParsedAuthor(name = "0235"), item.author)
            }
        }

    @Test
    @Throws(Exception::class)
    fun htmlAtomContentGetsUnescaped() {
        readResource("atom_hnapp.xml") {
            val feed =
                feedParser.parseFeedResponse(
                    URL(
                        "http://hnapp.com/rss?q=type%3Astory%20score%3E36%20-bitcoin%20-ethereum%20-cryptocurrency%20-blockchain%20-snowden%20-hiring%20-ask",
                    ),
                    it,
                )

            val item = feed.getOrNull()?.items!![0]
            assertEquals(
                "37 – Spectre Mitigations in Microsoft's C/C++ Compiler",
                item.title,
            )
            assertEquals(
                "37 points, 1 comment",
                item.content_text,
            )
            assertEquals(
                "<p>37 points, <a href=\"https://news.ycombinator.com/item?id=16381978\">1 comment</a></p>",
                item.content_html,
            )
        }
    }

    @Test
    @Throws(Exception::class)
    fun enclosedImageIsUsedAsThumbnail() {
        readResource("rss_lemonde.xml") {
            val feed =
                feedParser.parseFeedResponse(
                    URL("http://www.lemonde.fr/rss/une.xml"),
                    it,
                )

            val item = feed.getOrNull()?.items!![0]
            assertEquals(
                "http://s1.lemde.fr/image/2018/02/11/644x322/5255112_3_a8dc_martin-fourcade_02be61d126b2da39d977b2e1902c819a.jpg",
                item.image?.url,
            )
        }
    }

    @Test
    fun parsesYoutubeMediaInfo() =
        runBlocking {
            val feed =
                readResource("atom_youtube.xml") {
                    feedParser.parseFeedResponse(
                        URL("http://www.youtube.com/feeds/videos.xml"),
                        it,
                    )
                }

            val item = feed.getOrNull()?.items!!.first()

            assertEquals("Can You Observe a Typical Universe?", item.title)
            assertEquals("https://i2.ytimg.com/vi/q-6oU3jXAho/hqdefault.jpg", item.image?.url)
            assertTrue {
                item.content_text!!.startsWith("Sign Up on Patreon to get access to the Space Time Discord!")
            }
        }

    @Test
    fun parsesPeertubeMediaInfo() =
        runBlocking {
            val feed =
                readResource("rss_peertube.xml") {
                    feedParser.parseFeedResponse(URL("https://framatube.org/feeds/videos.xml"), it)
                }

            val item = feed.getOrNull()?.items!!.first()

            assertEquals("1.4. Et les réseaux sociaux ?", item.title)
            assertEquals(
                "https://framatube.org/static/thumbnails/ed5c048d-01f3-4ceb-97db-6e278de512b0.jpg",
                item.image?.url,
            )
            assertTrue {
                item.content_text!!.startsWith("MOOC CHATONS#1 - Internet")
            }
        }

    @Test
    fun parsesYahooMediaRss() =
        runBlocking {
            val feed =
                readResource("rss_mediarss.xml") {
                    feedParser.parseFeedResponse(
                        URL("https://rutube.ru/mrss/video/person/11234072/"),
                        it,
                    )
                }

            val item = feed.getOrNull()?.items!!.first()

            assertEquals("Камеди Клаб: «3 сентября»", item.title)
            assertEquals(
                "https://pic.rutubelist.ru/video/93/24/93245691f0e18d063da5fa5cd60fa6de.jpg?size=l",
                item.image?.url,
            )
        }

    @Test
    fun parsesYahooMediaRss2() =
        runBlocking {
            val feed =
                readResource("rss_myanimelist.xml") {
                    feedParser.parseFeedResponse(
                        URL("https://myanimelist.net/rss/news.xml"),
                        it,
                    )
                }

            val item = feed.getOrNull()?.items!!.first()

            assertEquals(
                "https://cdn.myanimelist.net/s/common/uploaded_files/1664092688-dd34666e64d7ae624e6e2c70087c181f.jpeg",
                item.image?.url,
            )
        }

    @Test
    fun parsesYahooMediaRssPicksLargestThumbnail() =
        runBlocking {
            val feed =
                readResource("rss_theguardian.xml") {
                    feedParser.parseFeedResponse(
                        URL("https://www.theguardian.com/world/rss"),
                        it,
                    )
                }

            val item = feed.getOrNull()?.items!!.first()

            assertEquals(
                "https://i.guim.co.uk/img/media/c4d7049b24ee34d1c4c630c751094cabc57c54f6/0_32_6000_3601/master/6000.jpg?width=460&quality=85&auto=format&fit=max&s=919d72fef6d4f3469aff69e94964126c",
                item.image?.url,
            )
        }

    @Test
    fun encodingTestWithSmileys() =
        runBlocking {
            val feed =
                readResource("rss_lawnchair.xml") {
                    feedParser.parseFeedResponse(
                        URL("https://nitter.weiler.rocks/lawnchairapp/rss"),
                        it,
                    )
                }

            val item = feed.getOrNull()?.items!!.first()

            assertTrue {
                "\uD83D\uDE0D\uD83E\uDD29" in item.content_html!!
            }
        }

    @Test
    @Throws(Exception::class)
    fun getAlternateFeedLinksDoesNotReturnRelativeLinks() {
        readResource("fz.html") {
            val metadata = feedParser.getSiteMetaDataInHtml(URL("https://www.fz.se"), it)
            assertEquals(
                listOf(
                    AlternateLink(link = URL("https://www.fz.se/feeds/nyheter"), type = "application/rss+xml"),
                    AlternateLink(link = URL("https://www.fz.se/feeds/forum"), type = "application/rss+xml"),
                ),
                metadata.getOrNull()!!.alternateFeedLinks,
            )
        }
    }

    @Test
    fun findsAppleTouchIconForFeed() =
        runBlocking {
            val metadata =
                readResource("fz.html") {
                    feedParser.getSiteMetaDataInHtml(URL("https://www.fz.se"), it)
                }

            assertEquals("https://www.fz.se/apple-touch-icon.png", metadata.getOrNull()!!.feedImage)
        }

    @Test
    fun findsAppleTouchIconInHtml() =
        runBlocking {
            val icon =
                readResource("fz.html") {
                    feedParser.getFeedIconInHtml(it, URL("https://www.fz.se"))
                }

            assertEquals("https://www.fz.se/apple-touch-icon.png", icon)
        }

    @Test
    fun findsFaviconInHtml() =
        runBlocking {
            val icon =
                readResource("slashdot.html") {
                    feedParser.getFeedIconInHtml(it, URL("https://slashdot.org"))
                }

            assertEquals("https://slashdot.org/favicon.ico", icon)
        }

    @Test
    @Throws(Exception::class)
    fun successfullyParsesAlternateLinkInBodyOfDocument() {
        readResource("nixos.html") {
            val metadata = feedParser.getSiteMetaDataInHtml(URL("https://nixos.org"), it)
            assertEquals(
                listOf(AlternateLink(URL("https://nixos.org/news-rss.xml"), "application/rss+xml")),
                metadata.getOrNull()!!.alternateFeedLinks,
            )
        }
    }

    @Test
    @Throws(Exception::class)
    fun getAlternateFeedLinksResolvesRelativeLinksGivenBaseUrl() {
        readResource("fz.html") {
            val metadata = feedParser.getSiteMetaDataInHtml(URL("https://www.fz.se/index.html"), it)
            assertEquals(
                listOf(
                    AlternateLink(URL("https://www.fz.se/feeds/nyheter"), "application/rss+xml"),
                    AlternateLink(URL("https://www.fz.se/feeds/forum"), "application/rss+xml"),
                ),
                metadata.getOrNull()!!.alternateFeedLinks,
            )
        }
    }

    @Test
    @Throws(Exception::class)
    fun encodingIsHandledInAtomRss() =
        runBlocking {
            val feed = golemDe.use { feedParser.parseFeedResponse(it) }

            assertEquals(true, feed.getOrNull()?.items?.get(0)?.content_text?.contains("größte"))
        }

    // Bug in Rome which I am working around, this will crash if not worked around
    @Test
    @Throws(Exception::class)
    fun emptySlashCommentsDontCrashParsingAndEncodingIsStillRespected() =
        runBlocking {
            val feed = emptySlashComment.use { feedParser.parseFeedResponse(it) }

            assertEquals(1, feed.getOrNull()?.items?.size)
            assertEquals(
                true,
                feed.getOrNull()?.items?.get(0)?.content_text?.contains("größte"),
                feed.getOrNull()?.items?.get(0)?.content_text!!,
            )
        }

    @Test
    @Throws(Exception::class)
    fun correctAlternateLinkInAtomIsUsedForUrl() =
        runBlocking {
            val feed = utdelningsSeglarenAtom.use { feedParser.parseFeedResponse(it) }

            assertEquals(
                "http://utdelningsseglaren.blogspot.com/2017/12/tips-pa-6-podcasts.html",
                feed.getOrNull()?.items?.first()?.url,
            )
        }

    @Test
    @Throws(Exception::class)
    fun relativeLinksAreMadeAbsoluteAtom() =
        runBlocking {
            val feed =
                feedParser.parseFeedResponse(
                    URL("http://cowboyprogrammer.org/feed.atom"),
                    atomRelative,
                )
            assertTrue { feed.isRight() }

            assertEquals("http://cowboyprogrammer.org/feed.atom", feed.getOrNull()!!.feed_url)
        }

    @Test
    @Throws(Exception::class)
    fun relativeLinksAreMadeAbsoluteAtomNoBase() =
        runBlocking {
            val feed =
                feedParser.parseFeedResponse(
                    URL("http://cowboyprogrammer.org/feed.atom"),
                    atomRelativeNoBase,
                )
            assertTrue { feed.isRight() }

            assertEquals("http://cowboyprogrammer.org/feed.atom", feed.getOrNull()!!.feed_url)
        }

    @Test
    @Throws(Exception::class)
    fun relativeFeedLinkInRssIsMadeAbsolute() =
        runBlocking {
            val feed = lineageosRss.use { feedParser.parseFeedResponse(it) }
            assertTrue { feed.isRight() }

            assertEquals("https://lineageos.org/", feed.getOrNull()!!.home_page_url)
            assertEquals("https://lineageos.org/feed.xml", feed.getOrNull()!!.feed_url)

            assertEquals("https://lineageos.org/Changelog-16", feed.getOrNull()?.items?.get(0)?.id)
            assertEquals("https://lineageos.org/Changelog-16/", feed.getOrNull()?.items?.get(0)?.url)
            assertEquals(
                "https://lineageos.org/images/2018-02-25/lineageos-15.1-hero.png",
                feed.getOrNull()?.items?.get(0)?.image?.url,
            )
        }

    @Test
    @Throws(Exception::class)
    fun noStyles() =
        runBlocking {
            val feed = researchRsc.use { feedParser.parseFeedResponse(it) }
            assertTrue { feed.isRight() }

            assertEquals("http://research.swtch.com/feed.atom", feed.getOrNull()!!.feed_url)

            assertEquals(17, feed.getOrNull()?.items!!.size)

            val item = feed.getOrNull()?.items!![9]

            assertEquals("http://research.swtch.com/qr-bbc.png", item.image?.url)

            assertEquals(
                "QArt Codes",
                item.title,
            )

            // Style tags should be ignored
            assertEquals(
                "QR codes are 2-dimensional bar codes that encode arbitrary text strings. A common use of QR codes is to encode URLs so that people can scan a QR code (for example, on an advertising poster, building r",
                item.summary,
            )
        }

    @Test
    @Throws(Exception::class)
    fun feedAuthorIsUsedAsFallback() =
        runBlocking {
            val feed = researchRsc.use { feedParser.parseFeedResponse(it) }
            assertTrue { feed.isRight() }

            assertEquals("http://research.swtch.com/feed.atom", feed.getOrNull()!!.feed_url)

            assertEquals(17, feed.getOrNull()?.items!!.size)

            val item = feed.getOrNull()?.items!![9]

            assertEquals("Russ Cox", feed.getOrNull()!!.author!!.name)
            assertEquals(feed.getOrNull()!!.author!!.name, item.author?.name)
        }

    @Test
    fun nixos() =
        runBlocking {
            val feed = nixosRss.use { feedParser.parseFeedResponse(it) }
            assertTrue { feed.isRight() }

            assertEquals("http://nixos.org/news-rss.xml", feed.getOrNull()!!.feed_url)

            assertEquals(99, feed.getOrNull()?.items!!.size)

            val (_, _, _, title, _, _, _, image) = feed.getOrNull()?.items!![0]

            assertEquals("https://nixos.org/logo/nixos-logo-18.09-jellyfish-lores.png", image?.url)
            assertEquals("NixOS 18.09 released", title)
        }

    @Test
    @Throws(Exception::class)
    fun nixers() =
        runBlocking {
            val feed = nixersRss.use { feedParser.parseFeedResponse(it) }
            assertTrue { feed.isRight() }

            assertEquals("https://newsletter.nixers.net/feed.xml", feed.getOrNull()!!.feed_url)

            assertEquals(111, feed.getOrNull()?.items!!.size)

            val item = feed.getOrNull()?.items!![0]

            // Timezone issues - so only verify date
            assertTrue(message = "Expected a pubdate to have been parsed") {
                item.date_published!!.startsWith("2019-01-25")
            }
        }

    @Test
    fun doesNotFetchVideoContentType(): Unit =
        runBlocking {
            // This request uses a feed but the content type is video and should be ignored
            val result = videoResponse.use { feedParser.parseFeedResponse(it) }
            assertTrue {
                result.isLeft()
            }
            assertTrue {
                result.leftOrNull()!!.description.contains("video/mp4")
            }
        }

    @Test
    fun fetchesNullContentType(): Unit =
        runBlocking {
            val result = nullContentTypeResponse.use { feedParser.parseFeedResponse(it) }
            assertTrue {
                result.isRight()
            }
        }

    @Test
    fun fetchesTextContentType(): Unit =
        runBlocking {
            val result = textContentTypeResponse.use { feedParser.parseFeedResponse(it) }
            assertTrue {
                result.isRight()
            }
        }

    @Test
    @Throws(Exception::class)
    fun cyklist() =
        runBlocking {
            val feed = cyklistBloggen.use { feedParser.parseFeedResponse(it) }
            assertTrue { feed.isRight() }

            assertEquals("http://www.cyklistbloggen.se/feed/", feed.getOrNull()!!.feed_url)

            assertEquals(10, feed.getOrNull()?.items!!.size)

            val (_, _, _, title, _, _, summary, image) = feed.getOrNull()?.items!![0]

            assertEquals(
                "http://www.cyklistbloggen.se/wp-content/uploads/2014/01/Danviksklippan-skyltad.jpg",
                image?.url,
            )

            assertEquals(
                "Ingen ombyggning av Danvikstull",
                title,
            )

            // Make sure character 160 (non-breaking space) is trimmed
            assertEquals(
                "För mer än tre år sedan aviserade dåvarande Allians-styrda Stockholms Stad att man äntligen skulle bredda den extremt smala passagen på pendlingsstråket vid Danvikstull: I smalaste passagen är gångdel",
                summary,
            )
        }

    @Test
    @Throws(Exception::class)
    fun cowboy() =
        runBlocking {
            val feed = cowboyRss.use { feedParser.parseFeedResponse(it) }
            assertTrue { feed.isRight() }

            assertEquals("http://cowboyprogrammer.org/index.xml", feed.getOrNull()!!.feed_url)

            assertEquals(15, feed.getOrNull()?.items!!.size)

            var entry = feed.getOrNull()?.items!![1]

            assertEquals(
                "https://cowboyprogrammer.org/images/zopfli_boxplot.png",
                entry.image?.url,
            )

            // Snippet should not contain images
            entry = feed.getOrNull()?.items!![4]
            assertEquals("Fixing the up button in Python shell history", entry.title)
            assertEquals(
                "In case your python/ipython shell doesn’t have a working history, e.g. pressing ↑ only prints some nonsensical ^[[A, then you are missing either the readline or ncurses library. Ipython is more descri",
                entry.summary,
            )
            // Snippet should not contain links
            entry = feed.getOrNull()?.items!![1]
            assertEquals("Compress all the images!", entry.title)
            assertEquals(
                "Update 2016-11-22: Made the Makefile compatible with BSD sed (MacOS) One advantage that static sites, such as those built by Hugo, provide is fast loading times. Because there is no processing to be d",
                entry.summary,
            )
        }

    @Test
    @Throws(Exception::class)
    fun rss() =
        runBlocking {
            val feed = cornucopiaRss.use { feedParser.parseFeedResponse(it) }.getOrNull()!!

            assertEquals("http://cornucopia.cornubot.se/", feed.home_page_url)
            assertEquals("https://cornucopia.cornubot.se/feeds/posts/default?alt=rss", feed.feed_url)

            assertEquals(25, feed.items!!.size)
            val item = feed.items!!.first()

            assertEquals(
                "Tredje månaden med överhettad svensk ekonomi - tydlig säljsignal för börsen",
                item.title,
            )
            assertEquals(
                "Tredje månaden med överhettad svensk ekonomi - tydlig säljsignal för börsen",
                item.title,
            )

            assertEquals(
                "För tredje månaden på raken ligger Konjunkturinsitutets barometerindikator (\"konjunkturbarometern\") kvar i överhettat läge. Det råder alltså en klart och tydligt långsiktig säljsignal i enlighet med k",
                item.summary,
            )
            assertTrue(item.content_html!!.startsWith("För tredje månaden på raken"))
            assertEquals(
                "https://1.bp.blogspot.com/-hD_mqKJx-XY/WLwTIKSEt6I/AAAAAAAAqfI/sztWEjwSYAoN22y_YfnZ-yotKjQsypZHACLcB/s640/konj.png",
                item.image?.url,
            )

            assertEquals<List<Any>?>(emptyList(), item.attachments ?: emptyList())
        }

    @Test
    @Throws(Exception::class)
    fun atom() =
        runBlocking {
            val feed = cornucopiaAtom.use { feedParser.parseFeedResponse(it) }.getOrNull()!!

            assertEquals("http://cornucopia.cornubot.se/", feed.home_page_url)
            assertEquals("https://cornucopia.cornubot.se/feeds/posts/default", feed.feed_url)

            assertEquals(25, feed.items!!.size)
            val item = feed.items!!.first()

            assertEquals(
                "Tredje månaden med överhettad svensk ekonomi - tydlig säljsignal för börsen",
                item.title,
            )
            assertEquals(
                "Tredje månaden med överhettad svensk ekonomi - tydlig säljsignal för börsen",
                item.title,
            )

            assertEquals(
                "För tredje månaden på raken ligger Konjunkturinsitutets barometerindikator (\"konjunkturbarometern\") kvar i överhettat läge. Det råder alltså en klart och tydligt långsiktig säljsignal i enlighet med k",
                item.summary,
            )
            assertTrue(item.content_html!!.startsWith("För tredje månaden på raken"))
            assertEquals(
                "https://1.bp.blogspot.com/-hD_mqKJx-XY/WLwTIKSEt6I/AAAAAAAAqfI/sztWEjwSYAoN22y_YfnZ-yotKjQsypZHACLcB/s640/konj.png",
                item.image?.url,
            )

            assertEquals<List<Any>?>(emptyList(), item.attachments ?: emptyList())
        }

    @Test
    @Throws(Exception::class)
    fun atomCowboy() =
        runBlocking {
            val feed = cowboyAtom.use { feedParser.parseFeedResponse(it) }.getOrNull()!!

            assertEquals(15, feed.items!!.size)
            val item = feed.items!![1]

            assertEquals("http://cowboyprogrammer.org/dummy-id-to-distinguis-from-alternate-link", item.id)
            assertTrue(item.date_published!!.contains("2016"), "Should take the updated timestamp")
            assertEquals(
                "http://localhost:1313/images/zopfli_boxplot.png",
                item.image?.url,
            )

            assertEquals("http://localhost:1313/css/images/logo.png", feed.icon)
        }

    @Test
    @Throws(Exception::class)
    fun morningPaper() =
        runBlocking {
            val feed = morningPaper.use { feedParser.parseFeedResponse(it) }.getOrNull()!!

            assertEquals("https://blog.acolyer.org", feed.home_page_url)
            assertEquals("https://blog.acolyer.org/feed/", feed.feed_url)

            assertEquals(10, feed.items!!.size)
            val item = feed.items!!.first()

            assertEquals(
                "Thou shalt not depend on me: analysing the use of outdated JavaScript libraries on the web",
                item.title,
            )

            assertEquals(
                "https://adriancolyer.files.wordpress.com/2017/02/js-libs-fig-1.jpeg?w=640",
                item.image?.url,
            )
        }

    @Test
    fun testSlashdot() =
        runBlocking {
            val feed = slashdotResponse.use { feedParser.parseFeedResponse(it) }.getOrElse { throw it.throwable!! }

            assertEquals("https://slashdot.org/", feed.home_page_url)
            assertEquals("https://rss.slashdot.org/Slashdot/slashdotMain", feed.feed_url)

            assertEquals(15, feed.items!!.size)
            val item = feed.items!!.first()

            assertEquals(
                "https://a.fsdn.com/sd/topics/topicslashdot.gif",
                feed.icon,
            )

            assertEquals(
                "https://yro.slashdot.org/story/24/03/02/071229/ransomware-attack-hampers-prescription-drug-sales-at-90-of-us-pharmacies?utm_source=rss1.0mainlinkanon&utm_medium=feed",
                item.url,
            )

            assertEquals(
                "Ransomware Attack Hampers Prescription Drug Sales at 90% of US Pharmacies",
                item.title,
            )

            assertEquals(
                "\"A ransomware gang once thought to have been crippled by law enforcement has snarled prescription processing for millions of Americans over the past week...\" reports the Washington Post. \"The hackers ",
                item.summary,
            )

            assertNull(item.image, "No image should be present")
        }

    @Test
    @Throws(Exception::class)
    fun londoner() =
        runBlocking {
            val feed =
                londoner.use { feedParser.parseFeedResponse(it) }.getOrElse {
                    System.err.println(it)
                    throw it.throwable!!
                }

            assertEquals("http://londonist.com/", feed.home_page_url)
            assertEquals("http://londonist.com/feed", feed.feed_url)

            assertEquals(40, feed.items!!.size)
            val item = feed.items!!.first()

            assertEquals(
                "Make The Most Of London's Offerings With Chip",
                item.title,
            )

            assertEquals(
                "http://assets.londonist.com/uploads/2017/06/chip_2.jpg",
                item.image?.url,
            )
        }

    @Test
    @Throws(Exception::class)
    fun noLinkShouldBeNull() =
        runBlocking {
            val feed = anon.use { feedParser.parseFeedResponse(it) }

            assertEquals("http://ANON.com/sub", feed.getOrNull()!!.home_page_url)
            assertEquals("http://anon.com/rss", feed.getOrNull()!!.feed_url)
            assertEquals("ANON", feed.getOrNull()!!.title)
            assertEquals("ANON", feed.getOrNull()!!.description)

            assertEquals(1, feed.getOrNull()?.items!!.size)
            val item = feed.getOrNull()?.items!![0]

            assertNull(item.url)

            assertEquals("ANON", item.title)
            assertEquals("ANON", item.content_text)
            assertEquals("ANON", item.content_html)
            assertEquals("ANON", item.summary)

        /*assertEquals("2018-12-13 00:00:00",
                item.date_published)*/
        }

    @Test
    fun golem2ShouldBeParsedDespiteEmptySlashComments() =
        runBlocking {
            val feed = golemDe2.use { feedParser.parseFeedResponse(it) }

            assertEquals("Golem.de", feed.getOrNull()?.title)
        }

    @Test
    @Throws(Exception::class)
    @Ignore
    fun cowboyAuthenticatedOnline() =
        runBlocking {
            val feed =
                feedParser.parseFeedUrl(URL("https://test:test@cowboyprogrammer.org/auth_basic/index.xml"))
            assertEquals("Cowboy Programmer", feed.getOrNull()?.title)
        }

    @Test
    fun diskuse() =
        runBlocking {
            runBlocking {
                val feed = diskuse.use { feedParser.parseFeedResponse(it) }
                val entry = feed.getOrNull()?.items!!.first()

                assertEquals(
                    "Kajman, O této diskusi: test <pre> in <description> and <b>bold</b> in title",
                    entry.title,
                )
            }
        }

    @Test
    @Ignore
    fun fzImgUrl() =
        runBlocking {
            val feed = fz.use { feedParser.parseFeedResponse(it) }.getOrNull()!!

            assertEquals("http://www.fz.se/nyheter/", feed.home_page_url)

            assertEquals(20, feed.items!!.size)
            val item = feed.items!!.first()

            assertEquals(
                "Nier: Automata bjuder på maffig lanseringstrailer",
                item.title,
            )

            assertEquals(
                "http://d2ihp3fq52ho68.cloudfront.net/YTo2OntzOjI6ImlkIjtpOjEzOTI3OTM7czoxOiJ3IjtpOjUwMDtzOjE6ImgiO2k6OTk5OTtzOjE6ImMiO2k6MDtzOjE6InMiO2k6MDtzOjE6ImsiO3M6NDA6IjU5YjA2YjgyZjkyY2IxZjBiMDZjZmI5MmE3NTk5NjMzMjIyMmU4NGMiO30=",
                item.image?.url,
            )
        }

    @Test
    fun geekpark() =
        runBlocking {
            val feed = geekpark.use { feedParser.parseFeedResponse(it) }

            assertEquals("极客公园（！）", feed.getOrNull()!!.title)

            assertEquals(30, feed.getOrNull()?.items?.size)
        }

    @Test
    fun contentTypeHtmlIsNotUnescapedTwice() =
        runBlocking {
            val feed = contentTypeHtml.use { feedParser.parseFeedResponse(it) }

            val gofeed = contentTypeHtml.use { exp.parseBody(it.body?.string() ?: "") }
            print(gofeed)

            val item = feed.getOrNull()?.items!!.single()

            assertFalse(
                item.content_html!!.contains(" <pre><code class=\"language-R\">obs.lon <- ncvar_get(nc.obs, 'lon')"),
            )

            assertTrue(
                item.content_html!!.contains(" <pre><code class=\"language-R\">obs.lon &lt;- ncvar_get(nc.obs, 'lon')"),
            )
        }

    @Test
    fun escapedRssDescriptionIsProperlyUnescaped() =
        runBlocking {
            val feed =
                feedParser.parseFeedResponse(
                    URL("http://cowboyprogrammer.org"),
                    rssWithHtmlEscapedDescription,
                )

            val item = feed.getOrNull()?.items!!.single()

            val gofeed = exp.parseBody(rssWithHtmlEscapedDescription)
            print(gofeed)

            assertEquals(
                "http://cowboyprogrammer.org/hello.jpg&cached=true",
                item.image?.url,
            )
            assertEquals(
                "<img src=\"hello.jpg&amp;cached=true\">",
                item.content_html,
            )
        }

    @Test
    fun escapedAtomContentIsProperlyUnescaped() =
        runBlocking {
            val feed =
                feedParser.parseFeedResponse(
                    URL("http://cowboyprogrammer.org"),
                    atomWithHtmlEscapedContents,
                )

            val text = feed.getOrNull()?.items!!.first()
            assertEquals(
                "http://cowboyprogrammer.org/hello.jpg&cached=true",
                text.image?.url,
            )
            assertEquals(
                "<img src=\"hello.jpg&amp;cached=true\">",
                text.content_html,
            )

            val html = feed.getOrNull()?.items!![1]
            assertEquals(
                "http://cowboyprogrammer.org/hello.jpg&cached=true",
                html.image?.url,
            )
            assertEquals(
                "<img src=\"hello.jpg&amp;cached=true\">",
                html.content_html,
            )

            val xhtml = feed.getOrNull()?.items!![2]
            assertEquals(
                "http://cowboyprogrammer.org/hello.jpg&cached=true",
                xhtml.image?.url,
            )
            assertTrue("Actual:\n${xhtml.content_html}") {
                "<img src=\"hello.jpg&amp;cached=true\"/>" in xhtml.content_html!!
            }
        }

    @Test
    fun timestampsAreParsedWithCorrectTimezone() {
        runBlocking {
            val feed =
                feedParser.parseFeedResponse(URL("https://www.bleepingcomputer.com/feed/"), rssBleepingComputer)
                    .getOrElse { throw it.throwable!! }

            // Thu, 14 Mar 2024 14:08:43 -0400"
            assertEquals(
                "2024-03-14T18:08:43Z",
                feed.items!!.first().date_published,
            )
        }
    }

    @Test
    fun handlesUnknownProtocols() =
        runBlocking {
            val feed =
                feedParser.parseFeedResponse(
                    URL("https://gemini.circumlunar.space"),
                    atomWithUnknownProtocol,
                )

            assertEquals(8, feed.getOrNull()?.items!!.size)
        }

    private fun <T> readResource(
        asdf: String,
        block: suspend (String) -> T,
    ): T {
        val body =
            javaClass.getResourceAsStream(asdf)!!
                .bufferedReader()
                .use(BufferedReader::readText)

        return runBlocking {
            block(body)
        }
    }

    private fun getCowboyHtml(): String =
        javaClass.getResourceAsStream("cowboyprogrammer.html")!!
            .bufferedReader()
            .use {
                it.readText()
            }

    private val emptySlashComment: Response
        get() =
            bytesToResponse(
                "empty_slash_comment.xml",
                "https://rss.golem.de/rss.php?feed=RSS2.0",
            )

    private val golemDe: Response
        get() = bytesToResponse("golem-de.xml", "https://rss.golem.de/rss.php?feed=RSS2.0")

    private val golemDe2: Response
        get() = bytesToResponse("rss_golem_2.xml", "https://rss.golem.de/rss.php?feed=RSS2.0")

    private val utdelningsSeglarenAtom: Response
        get() =
            bytesToResponse(
                "atom_utdelningsseglaren.xml",
                "http://utdelningsseglaren.blogspot.com/feeds/posts/default",
            )

    private val slashdotResponse: Response
        get() = bytesToResponse("rdf_slashdot.xml", "https://rss.slashdot.org/Slashdot/slashdotMain")

    private val lineageosRss: Response
        get() = bytesToResponse("rss_lineageos.xml", "https://lineageos.org/feed.xml")

    private val cornucopiaAtom: Response
        get() =
            bytesToResponse(
                "atom_cornucopia.xml",
                "https://cornucopia.cornubot.se/feeds/posts/default",
            )

    private val cornucopiaRss: Response
        get() =
            bytesToResponse(
                "rss_cornucopia.xml",
                "https://cornucopia.cornubot.se/feeds/posts/default?alt=rss",
            )

    private val cowboyRss: Response
        get() = bytesToResponse("rss_cowboy.xml", "http://cowboyprogrammer.org/index.xml")

    private val cowboyAtom: Response
        get() = bytesToResponse("atom_cowboy.xml", "http://cowboyprogrammer.org/atom.xml")

    private val cyklistBloggen: Response
        get() = bytesToResponse("rss_cyklistbloggen.xml", "http://www.cyklistbloggen.se/feed/")

    private val researchRsc: Response
        get() = bytesToResponse("atom_research_rsc.xml", "http://research.swtch.com/feed.atom")

    private val morningPaper: Response
        get() = bytesToResponse("rss_morningpaper.xml", "https://blog.acolyer.org/feed/")

    private val fz: Response
        get() = bytesToResponse("rss_fz.xml", "https://www.fz.se/feeds/nyheter")

    private val londoner: Response
        get() = bytesToResponse("rss_londoner.xml", "http://londonist.com/feed")

    private val anon: Response
        get() = bytesToResponse("rss_anon.xml", "http://ANON.com/rss")

    private val nixosRss: Response
        get() = bytesToResponse("rss_nixos.xml", "http://nixos.org/news-rss.xml")

    private val nixersRss: Response
        get() =
            bytesToResponse(
                "rss_nixers_newsletter.xml",
                "https://newsletter.nixers.net/feed.xml",
            )

    private val videoResponse: Response
        get() =
            bytesToResponse(
                "rss_nixers_newsletter.xml",
                "https://foo.bar/video.mp4",
                "video/mp4",
            )

    private val nullContentTypeResponse: Response
        get() =
            bytesToResponse(
                "rss_nixers_newsletter.xml",
                "https://foo.bar/feed.xml",
                null,
            )

    private val textContentTypeResponse: Response
        get() =
            bytesToResponse(
                "rss_nixers_newsletter.xml",
                "https://foo.bar/feed.xml",
                "text/html",
            )

    private val diskuse: Response
        get() =
            bytesToResponse(
                "rss_diskuse.xml",
                "https://diskuse.jakpsatweb.cz/rss2.php?topic=173233",
            )

    private val geekpark: Response
        get() = bytesToResponse("rss_geekpark.xml", "http://main_test.geekpark.net/rss.rss")

    private val contentTypeHtml: Response
        get() =
            bytesToResponse(
                "atom_content_type_html.xml",
                "http://www.zoocoop.com/contentoob/o1.atom",
            )

    private fun bytesToResponse(
        resourceName: String,
        url: String,
        contentType: String? = "application/xml",
    ): Response {
        val responseBody: ResponseBody =
            javaClass.getResourceAsStream(resourceName)!!
                .use { it.readBytes() }
                .toResponseBody(contentType?.toMediaTypeOrNull())

        return Response.Builder()
            .body(responseBody)
            .protocol(Protocol.HTTP_2)
            .code(200)
            .message("Test")
            .request(
                Request.Builder()
                    .url(url)
                    .build(),
            )
            .build()
    }
}

@Language("xml")
const val rssBleepingComputer = """
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<rss version="2.0">
  <channel>
    <title>BleepingComputer</title>
    <link>https://www.bleepingcomputer.com/</link>
    <description>BleepingComputer - All Stories</description>
    <pubDate>Thu, 14 Mar 2024 14:13:50 EDT</pubDate>
    <generator>https://www.bleepingcomputer.com/</generator>
    <language>en</language>
    <atom:link href="https://www.bleepingcomputer.com/feed/" rel="self" type="application/rss+xml" />
    
    <item>
        <title>SIM swappers now stealing phone numbers from eSIMs</title>
        <link>https://www.bleepingcomputer.com/news/security/sim-swappers-now-stealing-phone-numbers-from-esims/</link>
        <pubDate>Thu, 14 Mar 2024 14:08:43 -0400</pubDate>
        <dc:creator>Bill Toulas</dc:creator>
        <category><![CDATA[Security]]></category>
        <guid>https://www.bleepingcomputer.com/news/security/sim-swappers-now-stealing-phone-numbers-from-esims/</guid>
	    <description><![CDATA[SIM swappers have adapted their attacks to steal a target's phone number from an eSIM card, a rewritable SIM chip present on many recent smartphone models. [...]]]></description>
    </item>
  </channel>
</rss>
"""

@Language("xml")
const val atomRelative = """
<?xml version='1.0' encoding='UTF-8'?>
<feed xmlns='http://www.w3.org/2005/Atom' xml:base='http://cowboyprogrammer.org'>
  <id>http://cowboyprogrammer.org</id>
  <title>Relative links</title>
  <updated>2003-12-13T18:30:02Z</updated>
  <link rel="self" href="/feed.atom"/>
</feed>
"""

@Language("xml")
const val atomRelativeNoBase = """
<?xml version='1.0' encoding='UTF-8'?>
<feed xmlns='http://www.w3.org/2005/Atom'>
  <id>http://cowboyprogrammer.org</id>
  <title>Relative links</title>
  <updated>2003-12-13T18:30:02Z</updated>
  <link rel="self" href="/feed.atom"/>
</feed>
"""

@Language("xml")
const val atomWithAlternateLinks = """
<?xml version='1.0' encoding='UTF-8'?>
<feed xmlns='http://www.w3.org/2005/Atom'>
  <id>http://cowboyprogrammer.org</id>
  <title>Relative links</title>
  <updated>2003-12-13T18:30:02Z</updated>
  <link rel="self" href="/feed.atom"/>
  <link rel="alternate" type="text/html" href="http://localhost:1313/" />
  <link rel="alternate" type="application/rss" href="http://localhost:1313/index.xml" />
  <link rel="alternate" type="application/json" href="http://localhost:1313/feed.json" />
</feed>
"""

@Language("xml")
const val atomWithHtmlEscapedContents = """
<?xml version='1.0' encoding='UTF-8'?>
<feed xmlns='http://www.w3.org/2005/Atom'>
  <id>http://cowboyprogrammer.org</id>
  <title>escaping test</title>
  <updated>2003-12-13T18:30:02Z</updated>
  <link rel="self" href="/feed.atom"/>
  <entry>
    <id>http://cowboyprogrammer.org/1</id>
    <title>text</title>
    <link href="http://cowboyprogrammer.org/1"/>
    <updated>2020-10-12T21:26:03Z</updated>
    <content type="text">&lt;img src=&quot;hello.jpg&amp;amp;cached=true&quot;&gt;</content>
  </entry>
  <entry>
    <id>http://cowboyprogrammer.org/2</id>
    <title>html</title>
    <link href="http://cowboyprogrammer.org/2"/>
    <updated>2020-10-12T21:26:03Z</updated>
    <content type="html">&lt;img src=&quot;hello.jpg&amp;amp;cached=true&quot;&gt;</content>
  </entry>
  <entry>
    <id>http://cowboyprogrammer.org/3</id>
    <title>xhtml</title>
    <link href="http://cowboyprogrammer.org/3"/>
    <updated>2020-10-12T21:26:03Z</updated>
    <content type="xhtml">
       <div xmlns="http://www.w3.org/1999/xhtml">
         <img src="hello.jpg&amp;cached=true"/>
      </div>
    </content>
  </entry>
</feed>
"""

@Language("xml")
const val rssWithHtmlEscapedDescription = """
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<rss version="2.0">
  <channel>
    <title>Cowboy Programmer</title>
    <link>https://cowboyprogrammer.org/</link>
    <description>Recent content in Cowboy Programmer on Cowboy Programmer</description>
    <language>en-us</language>
    <lastBuildDate>Sun, 01 Sep 2019 23:21:00 +0200</lastBuildDate>
    
    <item>
      <title>description</title>
      <link>http://cowboyprogrammer.org/4</link>
      <pubDate>Sun, 01 Sep 2019 23:21:00 +0200</pubDate>
      <author>jonas@cowboyprogrammer.org (Space Cowboy)</author>
      <guid>http://cowboyprogrammer.org/4</guid>
      <description>&lt;img src=&quot;hello.jpg&amp;amp;cached=true&quot;&gt;</description>
    </item>
  </channel>
</rss>
"""

@Language("xml")
const val atomWithUnknownProtocol = """
<?xml version='1.0' encoding='UTF-8'?>
<feed xmlns="http://www.w3.org/2005/Atom">
<id>gemini://gemini.circumlunar.space/news/</id>
<title>Official Project Gemini news feed</title>
<updated>2023-03-01T18:34:36.714818+00:00</updated>
<author>
<name>Solderpunk</name>
<email>solderpunk@posteo.net</email>
</author>
<link href="gemini://gemini.circumlunar.space/news/atom.xml" rel="self"/>
<link href="gemini://gemini.circumlunar.space/news/" rel="alternate"/>
<generator uri="https://lkiesow.github.io/python-feedgen" version="0.9.0">python-feedgen</generator>
<entry>
<id>gemini://gemini.circumlunar.space/news/2022_01_16.gmi</id>
<title>2022-01-16 - Mailing list downtime, official news feed</title>
<updated>2022-01-16T16:10:48.445244+00:00</updated>
<link href="gemini://gemini.circumlunar.space/news/2022_01_16.gmi" rel="alternate"/>
</entry>
<entry>
<id>gemini://gemini.circumlunar.space/news/2022_01_22.gmi</id>
<title>2022-01-22 - Mailing list archives, Atom feed for official news</title>
<updated>2022-01-22T18:44:56.495170+00:00</updated>
<link href="gemini://gemini.circumlunar.space/news/2022_01_22.gmi" rel="alternate"/>
</entry>
<entry>
<id>gemini://gemini.circumlunar.space/news/2022_01_30.gmi</id>
<title>2022-01-30 - Minor specification update (0.16.1)</title>
<updated>2022-01-30T14:38:04.738832+00:00</updated>
<link href="gemini://gemini.circumlunar.space/news/2022_01_30.gmi" rel="alternate"/>
</entry>
<entry>
<id>gemini://gemini.circumlunar.space/news/2022_06_20.gmi</id>
<title>2022-06-20 - Three years of Gemini!</title>
<updated>2022-06-20T17:48:06.479966+00:00</updated>
<link href="gemini://gemini.circumlunar.space/news/2022_06_20.gmi" rel="alternate"/>
</entry>
<entry>
<id>gemini://gemini.circumlunar.space/news/2023_01_08.gmi</id>
<title>2023-01-08 - Changing DNS server</title>
<updated>2023-01-08T14:12:22.568187+00:00</updated>
<link href="gemini://gemini.circumlunar.space/news/2023_01_08.gmi" rel="alternate"/>
</entry>
<entry>
<id>gemini://gemini.circumlunar.space/news/2023_01_14.gmi</id>
<title>2023-01-14 - Tidying up gemini.circumlunar.space user capsules</title>
<updated>2023-01-14T15:58:26.933828+00:00</updated>
<link href="gemini://gemini.circumlunar.space/news/2023_01_14.gmi" rel="alternate"/>
</entry>
<entry>
<id>gemini://gemini.circumlunar.space/news/2023_02_14.gmi</id>
<title>2023-02-14 - Empty user capsules removed</title>
<updated>2023-02-14T08:45:03.559039+00:00</updated>
<link href="gemini://gemini.circumlunar.space/news/2023_02_14.gmi" rel="alternate"/>
</entry>
<entry>
<id>gemini://gemini.circumlunar.space/news/2023_03_01.gmi</id>
<title>2023-03-01 - Molly Brown upgrade</title>
<updated>2023-03-01T18:34:36.714818+00:00</updated>
<link href="gemini://gemini.circumlunar.space/news/2023_03_01.gmi" rel="alternate"/>
</entry>
</feed>
"""

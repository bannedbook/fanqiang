package com.nononsenseapps.feeder.model

import com.nononsenseapps.feeder.di.networkModule
import com.nononsenseapps.jsonfeed.cachingHttpClient
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withContext
import okhttp3.OkHttpClient
import okhttp3.mockwebserver.MockResponse
import okhttp3.mockwebserver.MockWebServer
import org.junit.After
import org.junit.Before
import org.junit.Test
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.bind
import org.kodein.di.instance
import org.kodein.di.singleton
import kotlin.test.assertEquals
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue

class FeedParserClientTest : DIAware {
    override val di by DI.lazy {
        bind<OkHttpClient>() with
            singleton {
                cachingHttpClient()
                    .newBuilder()
                    .addNetworkInterceptor(UserAgentInterceptor)
                    .build()
            }
        import(networkModule)
    }
    val server = MockWebServer()
    private val feedParser: FeedParser by instance()

    @After
    fun stopServer() {
        server.shutdown()
    }

    @Before
    fun setup() {
        server.start()
    }

    @Test
    @Throws(Exception::class)
    fun noPasswordInAuthAlsoWorks() {
        server.enqueue(
            MockResponse().apply {
                setResponseCode(401)
            },
        )
        server.enqueue(
            MockResponse().apply {
                setResponseCode(200)
                addHeader("Content-Type", "application/xml")
                this.setBody(
                    """
                    <?xml version='1.0' encoding='UTF-8'?>
                    <feed xmlns="http://www.w3.org/2005/Atom">
                    	<title>No auth</title>
                    </feed>
                    """.trimIndent(),
                )
            },
        )

        val url = server.url("/foo").newBuilder().username("user").build().toUrl()

        assertTrue {
            url.userInfo == "user"
        }

        runBlocking {
            val feed = feedParser.parseFeedUrl(url)
            assertEquals("No auth", feed.getOrNull()?.title)
        }
        assertNull(
            server.takeRequest().headers["Authorization"],
            message = "First request is done with no auth",
        )
        assertNotNull(
            server.takeRequest().headers["Authorization"],
            message = "After a 401 a new request is made with auth",
        )
    }

    @Test
    fun reasonableUserAgentIsPassed() {
        server.enqueue(
            MockResponse().apply {
                setResponseCode(403)
            },
        )

        // Some feeds return 403 unless they get a user-agent
        val url = server.url("/foo").toUrl()

        runBlocking {
            launch {
                try {
                    feedParser.parseFeedUrl(url)
                } catch (e: Throwable) {
                    // meh
                }
            }

            val headers =
                withContext(Dispatchers.IO) {
                    server.takeRequest().headers
                }

            val userAgents = headers.toMultimap()["User-Agent"]

            assertEquals(1, userAgents?.size)

            val userAgent = userAgents?.first()

            assertTrue(
                userAgent!!.startsWith("SpaceCowboy"),
            )
        }
    }

    @Test
    fun badProtocolInLinksAreHandled() =
        runBlocking {
            server.enqueue(
                MockResponse().apply {
                    setResponseCode(200)
                    addHeader("Content-Type", "application/xml")
                    this.setBody(
                        """
                        <?xml version="1.0" encoding="utf-8"?>
                        <rss xmlns:atom="http://www.w3.org/2005/Atom" version="2.0">
                          <channel>
                            <title>QC RSS</title>
                            <link>http://www.questionablecontent.net</link>
                            <description>The Official QC RSS Feed</description>
                            <generator>Feeder 4.3.2(5732); Mac OS X Version 12.4 (Build 21F79)
                              https://reinventedsoftware.com/feeder/
                            </generator>
                            <docs>http://blogs.law.harvard.edu/tech/rss</docs>
                            <language>en</language>
                            <pubDate>Wed, 01 Jun 2022 22:09:29 -0300</pubDate>
                            <lastBuildDate>Wed, 01 Jun 2022 22:09:29 -0300</lastBuildDate>
                            <atom:link href="http://www.questionablecontent.net/QCRSS.xml" rel="self"
                              type="application/rss+xml" />
                            <item>
                              <title>Callout Post</title>
                              <link>ttp://questionablecontent.net/view.php?comic=4776</link>
                              <description>
                                <![CDATA[<img src="http://www.questionablecontent.net/comics/4776.png">]]></description>
                              <pubDate>Sun, 01 May 2022 22:06:19 -0300</pubDate>
                              <guid isPermaLink="false">325BE5B5-8206-4C4A-9E94-828EE3DD7763</guid>
                            </item>
                          </channel>
                        </rss>
                        """.trimIndent(),
                    )
                },
            )

            val url = server.url("/foo").toUrl()
            // This should not crash
            val result = feedParser.parseFeedUrl(url)
            assertEquals("http://www.questionablecontent.net/comics/4776.png", result.getOrNull()?.items?.first()?.image?.url)
        }
}

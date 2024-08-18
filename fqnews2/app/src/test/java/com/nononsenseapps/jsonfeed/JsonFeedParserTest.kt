package com.nononsenseapps.jsonfeed

import org.junit.Assert
import org.junit.Test
import kotlin.test.assertEquals

class JsonFeedParserTest {
    @Test
    fun basic() {
        val parser = JsonFeedParser()

        val feed =
            parser.parseJson(
                """
{
    "version": "https://jsonfeed.org/version/1",
    "title": "My Example Feed",
    "home_page_url": "https://example.org/",
    "feed_url": "https://example.org/feed.json",
    "items": [
        {
            "id": "2",
            "content_text": "This is a second item.",
            "url": "https://example.org/second-item"
        },
        {
            "id": "1",
            "content_html": "<p>Hello, world!</p>",
            "url": "https://example.org/initial-post"
        }
    ]
}
        """,
            )

        assertEquals("https://jsonfeed.org/version/1", feed.version)
        assertEquals("My Example Feed", feed.title)
        assertEquals("https://example.org/", feed.home_page_url)
        assertEquals("https://example.org/feed.json", feed.feed_url)

        assertEquals(2, feed.items?.size)

        assertEquals("2", feed.items!![0].id)
        assertEquals("This is a second item.", feed.items!![0].content_text)
        assertEquals("https://example.org/second-item", feed.items!![0].url)

        assertEquals("1", feed.items!![1].id)
        assertEquals("<p>Hello, world!</p>", feed.items!![1].content_html)
        assertEquals("https://example.org/initial-post", feed.items!![1].url)
    }

    @Test
    fun dateParsing() {
        val parser = JsonFeedParser()

        val feed =
            parser.parseJson(
                """
{
    "version": "https://jsonfeed.org/version/1",
    "title": "My Example Feed",
    "home_page_url": "https://example.org/",
    "feed_url": "https://example.org/feed.json",
    "items": [
        {
            "id": "1",
            "content_html": "<p>Hello, world!</p>",
            "url": "https://example.org/initial-post",
            "date_published": "2010-02-07T14:04:00-05:00",
            "date_modified": "2012-03-08T15:05:01+09:00"
        }
    ]
}
        """,
            )

        assertEquals("https://jsonfeed.org/version/1", feed.version)
        assertEquals("My Example Feed", feed.title)
        assertEquals("https://example.org/", feed.home_page_url)
        assertEquals("https://example.org/feed.json", feed.feed_url)

        assertEquals(1, feed.items?.size)

        assertEquals("1", feed.items!![0].id)
        assertEquals("<p>Hello, world!</p>", feed.items!![0].content_html)
        assertEquals("https://example.org/initial-post", feed.items!![0].url)

        assertEquals("2010-02-07T14:04:00-05:00", feed.items!![0].date_published)
        assertEquals("2012-03-08T15:05:01+09:00", feed.items!![0].date_modified)
    }

    @Test
    fun cowboyOnline() {
        val parser = JsonFeedParser()

        val feed = parser.parseUrl("https://cowboyprogrammer.org/feed.json")

        assertEquals("https://jsonfeed.org/version/1", feed.version)
        assertEquals("Cowboy Programmer", feed.title)
        assertEquals("Space Cowboy", feed.author?.name)
        assertEquals("https://cowboyprogrammer.org/css/images/logo.png", feed.icon)
    }

    @Test
    fun badUrl() {
        Assert.assertThrows(
            "Bad URL. Perhaps it is missing an http:// prefix?",
            IllegalArgumentException::class.java,
        ) {
            JsonFeedParser().parseUrl("cowboyprogrammer.org/feed.json")
        }
    }
}

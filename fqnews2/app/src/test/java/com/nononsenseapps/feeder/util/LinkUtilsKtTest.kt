package com.nononsenseapps.feeder.util

import org.junit.Test
import java.net.MalformedURLException
import java.net.URL
import kotlin.test.assertEquals
import kotlin.test.assertFails
import kotlin.test.assertFailsWith

class LinkUtilsKtTest {
    @Test
    fun testSloppyToStrictAddsRespectsUnknownProtocols() {
        assertFails {
            sloppyLinkToStrictURL("gemini://google.com")
        }
    }

    @Test
    fun testSloppyToStrictAddsHttp() {
        assertEquals(URL("http://google.com"), sloppyLinkToStrictURL("google.com"))
    }

    @Test
    fun testSloppyToStrictWithAlreadyValidLink() {
        assertEquals(URL("https://google.com"), sloppyLinkToStrictURL("https://google.com"))
    }

    @Test
    fun testSloppyToStrictWithEmptyString() {
        assertFailsWith<MalformedURLException> {
            sloppyLinkToStrictURL("")
        }
    }

    @Test
    fun testRelativeToAbsoluteWithFeedLinkAsBase() {
        assertEquals(
            URL("http://cowboy.com/bob.jpg"),
            relativeLinkIntoAbsoluteOrThrow(URL("http://cowboy.com/feed.xml"), "/bob.jpg"),
        )
    }
}

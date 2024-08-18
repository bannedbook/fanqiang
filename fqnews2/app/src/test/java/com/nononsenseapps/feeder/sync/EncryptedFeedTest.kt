package com.nononsenseapps.feeder.sync

import org.intellij.lang.annotations.Language
import org.junit.Test
import java.net.URL
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue

class EncryptedFeedTest {
    private val moshi = getMoshi()

    @Test
    fun encryptedFeedCanBeParsedFromIncompleteAndOddJson() {
        val adapter = moshi.adapter<EncryptedFeed>()

        @Language("JSON")
        val json =
            """
            {
               "url": "https://foo.bar",
               "title": "foo",
               "alternateId": true,
               "notARealField": 1
            }
            """.trimIndent()
        val feed = adapter.fromJson(json)!!

        assertEquals(URL("https://foo.bar"), feed.url)
        assertEquals("foo", feed.title)
        assertTrue(feed.alternateId)
        assertFalse(feed.fullTextByDefault)
    }
}

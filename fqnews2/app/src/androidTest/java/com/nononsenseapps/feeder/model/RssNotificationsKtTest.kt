package com.nononsenseapps.feeder.model

import android.content.Intent
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry.getInstrumentation
import com.nononsenseapps.feeder.db.COL_LINK
import com.nononsenseapps.feeder.db.room.FeedItem
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNull

@RunWith(AndroidJUnit4::class)
class RssNotificationsKtTest {
    @Test
    fun openInBrowserIntentPointsToActivityWithIdAndLink() {
        val intent: Intent = getOpenInDefaultActivityIntent(getInstrumentation().context, 99, "http://foo")

        assertEquals(
            "com.nononsenseapps.feeder.ui.OpenLinkInDefaultActivity",
            intent.component?.className,
        )
        assertEquals("99", intent.data?.lastPathSegment)
        assertEquals("http://foo", intent.data?.getQueryParameter(COL_LINK))
    }

    @Test
    fun openInDefaultActivityIntentsAreConsideredDifferentForSameItem() {
        val feedItem =
            FeedItem(
                id = 5,
                link = "http://foo",
                enclosureLink = "ftp://bar",
            )

        val linkIntent = getOpenInDefaultActivityIntent(getInstrumentation().context, feedItem.id, link = feedItem.link)
        val enclosureIntent = getOpenInDefaultActivityIntent(getInstrumentation().context, feedItem.id, link = feedItem.enclosureLink)
        val markAsReadIntent = getOpenInDefaultActivityIntent(getInstrumentation().context, feedItem.id, link = null)

        assertFalse(
            linkIntent.filterEquals(enclosureIntent),
            message = "linkIntent should not be considered equal to enclosureIntent",
        )

        assertFalse(
            linkIntent.filterEquals(markAsReadIntent),
            message = "linkIntent should not be considered equal to markAsReadIntent",
        )

        assertFalse(
            enclosureIntent.filterEquals(markAsReadIntent),
            message = "enclosureIntent should not be considered equal to markAsReadIntent",
        )
    }

    @Test
    fun queryParameterDoesntGetGarbled() {
        @Suppress("ktlint:standard:max-line-length")
        val magnetLink = "magnet:?xt=urn:btih:82B1726F2D1B22F383A2B2CD6977B00F908FB315&dn=Crazy+Ex+Girlfriend+S04E10+720p+HDTV+x264+LucidTV&tr=udp%3A%2F%2Ftracker.coppersurfer.tk%3A6969%2Fannounce&tr=udp%3A%2F%2Ftracker.leechers-paradise.org%3A6969%2Fannounce&tr=udp%3A%2F%2Ftracker.opentrackr.org%3A1337%2Fannounce&tr=http%3A%2F%2Ftracker.trackerfix.com%3A80%2Fannounce"

        val enclosureIntent = getOpenInDefaultActivityIntent(getInstrumentation().context, 5, link = magnetLink)

        assertEquals(
            magnetLink,
            enclosureIntent.data?.getQueryParameter(COL_LINK),
            message = "Expected link to not get garbled as query parameter",
        )
    }

    @Test
    fun nullLinkIsNullQueryParam() {
        val enclosureIntent = getOpenInDefaultActivityIntent(getInstrumentation().context, 5, link = null)

        assertNull(
            enclosureIntent.data?.getQueryParameter(COL_LINK),
            message = "Expected a null query parameter",
        )
    }
}

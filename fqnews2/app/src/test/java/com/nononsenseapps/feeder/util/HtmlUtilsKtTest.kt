@file:Suppress("ktlint:standard:max-line-length")

package com.nononsenseapps.feeder.util

import org.junit.Test
import kotlin.test.assertEquals
import kotlin.test.assertNull

class HtmlUtilsKtTest {
    @Test
    fun ignoresBase64InlineImages() {
        val text = """<summary type="html">
&lt;img src="data:image/png;base64,iVBORw0KGgoAAA
ANSUhEUgAAAAUAAAAFCAYAAACNbyblAAAAHElEQVQI12P4
//8/w38GIAXDIBKE0DHxgljNBAAO9TXL0Y4OHwAAAABJRU
5ErkJggg==" alt="Red dot" /&gt;</summary>"""
        assertNull(findFirstImageInHtml(text, null))
    }

    @Test
    fun ignoresBase64InlineImagesSingleLine() {
        val text = """<summary type="html">&lt;img src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAUAAAAFCAYAAACNbyblAAAAHElEQVQI12P4//8/w38GIAXDIBKE0DHxgljNBAAO9TXL0Y4OHwAAAABJRU5ErkJggg==" alt="Red dot" /&gt;</summary>""".trimMargin()
        assertNull(findFirstImageInHtml(text, null))
    }

    @Test
    fun findsImageInSingleLine() {
        val text = "<summary type=\"html\">&lt;img src=\"https://imgs.xkcd.com/comics/interstellar_asteroid.png\" title=\"Every time we detect an asteroid from outside the Solar System, we should immediately launch a mission to fling one of our asteroids back in the direction it came from.\" alt=\"Every time we detect an asteroid from outside the Solar System, we should immediately launch a mission to fling one of our asteroids back in the direction it came from.\" /&gt;</summary>"
        assertEquals(
            "https://imgs.xkcd.com/comics/interstellar_asteroid.png",
            findFirstImageInHtml(text, null)?.url,
        )
    }

    @Test
    fun findsImageInSingleLineSrcFarFromImg() {
        val text = "<summary type=\"html\">&lt;img title=\"Every time we detect an asteroid from outside the Solar System, we should immediately launch a mission to fling one of our asteroids back in the direction it came from.\" alt=\"Every time we detect an asteroid from outside the Solar System, we should immediately launch a mission to fling one of our asteroids back in the direction it came from.\" src=\"https://imgs.xkcd.com/comics/interstellar_asteroid.png\" /&gt;</summary>"
        assertEquals(
            "https://imgs.xkcd.com/comics/interstellar_asteroid.png",
            findFirstImageInHtml(text, null)?.url,
        )
    }

    @Test
    fun returnsNullWhenNoImageLink() {
        val text = "<summary type=\"html\">&lt;img title=\"Every time we detect an asteroid from outside the Solar System, we should immediately launch a mission to fling one of our asteroids back in the direction it came from.\" alt=\"Every time we detect an asteroid from outside the Solar System, we should immediately launch a mission to fling one of our asteroids back in the direction it came from.\" /&gt;</summary>"
        assertNull(findFirstImageInHtml(text, null))
    }

    @Test
    fun returnsNullForNull() {
        assertNull(findFirstImageInHtml(null, null))
    }

    @Test
    fun returnsNullForTwitterAndFacebookIcons() {
        assertNull(
            findFirstImageInHtml(
                "<description>There's a tiny, black-freckled toad that likes the water in hot springs. Unfortunately, the only place in the world where the species is found is on 760 acres of wetlands about 100 miles east of Reno, Nevada, according to the New York Times. And that's near the site for two renewable-energy geothermal plants which poses \"significant risk to the well-being of the species,\" according to America's Fish and Wildlife Service &mdash; which just announced an emergency measure declaring it an endangered species. The temporary protection, which went into effect immediately and lasts for 240 days, was imposed to ward off the toad's potential extinction, the U.S. Fish and Wildlife Service said in a statement, adding that it would consider public comments about whether to extend the toad's emergency listing. The designation would add another hurdle for a plan to build two power plants with the encouragement of the U.S. Bureau of Land Management. The project is already the subject of a lawsuit filed by conservationists and a nearby Native American tribe. They hope the emergency listing can be used to block construction, which recently resumed.... The suit contended that the geothermal plants would dry up nearby hot springs sacred to the tribe and wipe out the Dixie Valley toad species. The U.S. Fish and Wildlife Service argues that \"protecting small population species like this ensures the continued biodiversity necessary to maintain climate-resilient landscapes in one of the driest states in the country.\" They were only recently scientifically described &mdash; or declared a unique species &mdash; in 2017, making the Dixie Valley toad \"&gt;the first new toad species to be described in the U.S. in nearly 50 years. And they are truly unique. When they were described, scientists analyzed 14 different morphological characteristics like size, shape, and markings. Dixie Valley toads scored \"significantly different\" from other western toad species in all categories. Thanks to long-time Slashdot reader walterbyrd for sharing the link!<p><div class=\"share_submission\" style=\"position:relative;\"> <a class=\"slashpop\" href=\"http://twitter.com/home?status=How+A+Tiny+Toad+Could+Upend+a+US+Geothermal+Project%3A+https%3A%2F%2Fbit.ly%2F3U26QW8\"><img src=\"https://a.fsdn.com/sd/twitter_icon_large.png\"></a> <a class=\"slashpop\" href=\"http://www.facebook.com/sharer.php?u=https%3A%2F%2Fhardware.slashdot.org%2Fstory%2F22%2F09%2F11%2F1931213%2Fhow-a-tiny-toad-could-upend-a-us-geothermal-project%3Futm_source%3Dslashdot%26utm_medium%3Dfacebook\"><img src=\"https://a.fsdn.com/sd/facebook_icon_large.png\"></a> </div></p><p><a href=\"https://hardware.slashdot.org/story/22/09/11/1931213/how-a-tiny-toad-could-upend-a-us-geothermal-project?utm_source=rss1.0moreanon&amp;utm_medium=feed\">Read more of this story</a> at Slashdot.</p><iframe src=\"https://slashdot.org/slashdot-it.pl?op=discuss&amp;id=22035223&amp;smallembed=1\" style=\"height: 300px; width: 100%; border: none;\"></iframe></description>",
                null,
            ),
        )
    }

    @Test
    fun returnsNullForTrackingPixels() {
        assertNull(
            findFirstImageInHtml(
                """<description>asdfasd <img src="https://foo/track.png" width="1" height="1"> asdf asd</description>""",
                null,
            ),
        )
    }
}

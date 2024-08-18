package com.nononsenseapps.feeder.model.gofeed

import com.nononsenseapps.feeder.model.ImageFromHTML
import org.junit.Test
import java.net.URL
import kotlin.test.assertEquals

class GoFeedExtensionsKtTest {
    private val baseUrl = URL("http://test.com")

    // Essentially a test for XKCD
    @Test
    fun descriptionWithOnlyImageDoesNotReturnBlankSummaryAndGetsImageSet() {
        val expectedSummary = "[An image]"
        val html = "  <img src='http://google.com/image.png' alt='An image'/> "

        val item =
            FeederGoItem(
                goItem =
                    makeGoItem(
                        guid = "$baseUrl/id",
                        title = "",
                        content = html,
                        description = html,
                    ),
                feedBaseUrl = baseUrl,
                feedAuthor = null,
            )

        assertEquals(
            item.thumbnail,
            ImageFromHTML(url = "http://google.com/image.png", width = null, height = null),
        )

        assertEquals(expectedSummary, item.snippet)
    }
//
//    @Test
//    fun itemSummary() {
//        val rand = Random()
//        var expectedSummary = ""
//        while (expectedSummary.length < 200) {
//            expectedSummary += "${rand.nextInt(10)}"
//        }
//        val longText = "$expectedSummary and some additional text"
//
//        assertEquals(
//            ParsedArticle(
//                id = "$baseUrl/id",
//                title = "",
//                content_text = longText,
//                summary = expectedSummary,
//                url = null,
//                content_html = longText,
//                attachments = emptyList(),
//            ),
//            mockSyndEntry(uri = "id", description = mockSyndContent(value = longText)).asItem(baseUrl),
//        )
//    }
//
//    @Test
//    fun itemShortTextShouldNotBeIndexOutOfBounds() {
//        assertEquals(
//            ParsedArticle(
//                id = "$baseUrl/id",
//                content_text = "abc",
//                summary = "abc",
//                title = "",
//                url = null,
//                content_html = "abc",
//                attachments = emptyList(),
//            ),
//            mockSyndEntry(uri = "id", description = mockSyndContent(value = "abc")).asItem(baseUrl),
//        )
//    }
//
//    @Test
//    fun itemLinkButNoLinks() {
//        assertEquals(
//            ParsedArticle(
//                id = "$baseUrl/id",
//                content_text = "",
//                summary = "",
//                title = "",
//                content_html = "",
//                attachments = emptyList(),
//                url = "$baseUrl/abc",
//            ),
//            mockSyndEntry(
//                uri = "id",
//                description = mockSyndContent(value = ""),
//                link = "abc",
//            ).asItem(baseUrl),
//        )
//    }
//
//    @Test
//    fun itemLinks() {
//        assertEquals(
//            ParsedArticle(
//                id = "$baseUrl/id",
//                content_text = "",
//                summary = "",
//                title = "",
//                content_html = "",
//                attachments = emptyList(),
//                url = "$baseUrl/abc",
//            ),
//            mockSyndEntry(
//                uri = "id",
//                description = mockSyndContent(value = ""),
//                links =
//                listOf(
//                    mockSyndLink(href = "abc", rel = "self"),
//                    mockSyndLink(href = "bcd"),
//                ),
//            ).asItem(baseUrl),
//        )
//    }
//
//    @Test
//    fun asAttachment() {
//        assertEquals(
//            ParsedEnclosure(url = "$baseUrl/uurl", mime_type = "text/html", size_in_bytes = 5),
//            mockSyndEnclosure(url = "uurl", type = "text/html", length = 5).asAttachment(baseUrl),
//        )
//    }
//
//    @Test
//    fun contentTextWithPlainAndOthers() {
//        assertEquals(
//            ParsedArticle(
//                id = "$baseUrl/id",
//                content_text = "PLAIN",
//                summary = "PLAIN",
//                title = "",
//                url = null,
//                content_html = "<b>html</b>",
//                attachments = emptyList(),
//            ),
//            mockSyndEntry(
//                uri = "id",
//                contents =
//                listOf(
//                    mockSyndContent(value = "PLAIN", type = "text"),
//                    mockSyndContent(value = "<b>html</b>", type = "html"),
//                    mockSyndContent(value = null, type = "xhtml"),
//                    mockSyndContent(value = "bah", type = null),
//                ),
//            ).asItem(baseUrl),
//        )
//    }
//
//    @Test
//    fun contentTextWithNullAndOthers() {
//        assertEquals(
//            ParsedArticle(
//                id = "$baseUrl/id",
//                content_text = "bah",
//                summary = "bah",
//                title = "",
//                url = null,
//                content_html = "<b>html</b>",
//                attachments = emptyList(),
//            ),
//            mockSyndEntry(
//                uri = "id",
//                contents =
//                listOf(
//                    mockSyndContent(value = "<b>html</b>", type = "html"),
//                    mockSyndContent(value = null, type = "xhtml"),
//                    mockSyndContent(value = "bah", type = null),
//                ),
//            ).asItem(baseUrl),
//        )
//    }
//
//    @Test
//    fun contentTextWithOthers() {
//        assertEquals(
//            ParsedArticle(
//                id = "$baseUrl/id",
//                content_text = "html",
//                summary = "html",
//                title = "",
//                url = null,
//                content_html = "<b>html</b>",
//                attachments = emptyList(),
//            ),
//            mockSyndEntry(
//                uri = "id",
//                contents =
//                listOf(
//                    mockSyndContent(value = "<b>html</b>", type = "html"),
//                    mockSyndContent(value = null, type = "xhtml"),
//                ),
//            ).asItem(baseUrl),
//        )
//    }
//
//    @Test
//    fun contentHtmlAtomWithOnlyUnknown() {
//        assertEquals(
//            ParsedArticle(
//                id = "$baseUrl/id",
//                title = "",
//                content_text = "foo",
//                summary = "foo",
//                url = null,
//                content_html = "foo",
//                attachments = emptyList(),
//            ),
//            mockSyndEntry(
//                uri = "id",
//                contents =
//                listOf(
//                    mockSyndContent(value = "foo"),
//                ),
//            ).asItem(baseUrl),
//        )
//    }
//
//    @Test
//    fun titleHtmlAtom() {
//        assertEquals(
//            ParsedArticle(
//                id = "$baseUrl/id",
//                title = "600 – Email is your electronic memory",
//                content_text = "",
//                summary = "",
//                url = null,
//                attachments = emptyList(),
//            ),
//            mockSyndEntry(
//                uri = "id",
//                titleEx = mockSyndContent(value = "600 &#8211; Email is your electronic memory", type = "html"),
//            ).asItem(baseUrl),
//        )
//    }
//
//    @Test
//    fun titleXHtmlAtom() {
//        assertEquals(
//            ParsedArticle(
//                id = "$baseUrl/id",
//                title = "600 – Email is your electronic memory",
//                content_text = "",
//                summary = "",
//                url = null,
//                attachments = emptyList(),
//            ),
//            mockSyndEntry(
//                uri = "id",
//                titleEx = mockSyndContent(value = "600 &#8211; Email is your electronic memory", type = "xhtml"),
//            ).asItem(baseUrl),
//        )
//    }
//
//    @Test
//    fun titlePlainAtomRss() {
//        assertEquals(
//            ParsedArticle(
//                id = "$baseUrl/id",
//                title = "600 – Email is your electronic memory",
//                content_text = "",
//                summary = "",
//                url = null,
//                attachments = emptyList(),
//            ),
//            mockSyndEntry(
//                uri = "id",
//                title = "600 &#8211; Email is your electronic memory",
//            ).asItem(baseUrl),
//        )
//    }
//
//    @Test
//    fun contentHtmlRss() {
//        assertEquals(
//            ParsedArticle(
//                id = "$baseUrl/id",
//                content_text = "html",
//                summary = "html",
//                title = "",
//                url = null,
//                content_html = "<b>html</b>",
//                attachments = emptyList(),
//            ),
//            mockSyndEntry(
//                uri = "id",
//                description = mockSyndContent(value = "<b>html</b>"),
//            ).asItem(baseUrl),
//        )
//    }
//
//    @Test
//    fun thumbnailWithThumbnail() {
//        assertEquals(
//            ParsedArticle(
//                id = "$baseUrl/id",
//                title = "",
//                content_text = "",
//                summary = "",
//                attachments = emptyList(),
//                url = null,
//                image = MediaImage(url = "$baseUrl/img", width = 0, height = 0),
//            ),
//            mockSyndEntry(
//                uri = "id",
//                thumbnails = arrayOf(mockThumbnail(url = URI.create("img"))),
//            ).asItem(baseUrl),
//        )
//    }
//
//    @Test
//    fun asItemDiscardsInlineBase64ImagesAsThumbnails() {
//        assertEquals(
//            ParsedArticle(
//                id = "$baseUrl/id",
//                title = "",
//                content_text = "",
//                summary = "",
//                attachments = emptyList(),
//                url = null,
//                image = null,
//            ),
//            mockSyndEntry(
//                uri = "id",
//                thumbnails =
//                arrayOf(
//                    mockThumbnail(
//                        url =
//                        URI.create(
//                            "data:image/png;base64,iVBORw0KGgoAAA" +
//                                    "ANSUhEUgAAAAUAAAAFCAYAAACNbyblAAAAHElEQVQI12P4" +
//                                    "//8/w38GIAXDIBKE0DHxgljNBAAO9TXL0Y4OHwAAAABJRU" +
//                                    "5ErkJggg==",
//                        ),
//                    ),
//                ),
//            ).asItem(baseUrl),
//        )
//    }
//
//    @Test
//    fun thumbnailWithContent() {
//        assertEquals(
//            ParsedArticle(
//                id = "$baseUrl/id",
//                title = "",
//                content_text = "",
//                summary = "",
//                attachments = emptyList(),
//                url = null,
//                image = MediaImage(url = "$baseUrl/img", width = 0, height = 0),
//            ),
//            mockSyndEntry(
//                uri = "id",
//                mediaContents = arrayOf(mockMediaContent(url = "img", medium = "image")),
//            ).asItem(baseUrl),
//        )
//    }
//
//    @Test
//    fun thumbnailFromHtmlDescriptionIsUnescaped() {
//        val description =
//            mockSyndContent(
//                value =
//                """
//                    <img src="https://o.aolcdn.com/images/dims?crop=1200%2C627%2C0%2C0&quality=85&format=jpg&resize=1600%2C836&image_uri=https%3A%2F%2Fs.yimg.com%2Fos%2Fcreatr-uploaded-images%2F2019-03%2Ffa057c20-5050-11e9-bfef-d1614983d7cc&client=a1acac3e1b3290917d92&signature=351348aa11c53a569d5ad40f3a7ef697471b645a" />Google didn&#039;t completely scrap its robotic dreams after it sold off Boston Dynamics and shuttered the other robotic start-ups it acquired over the past decade. Now, the tech giant has given us a glimpse of how the program has changed in a blog post a...
//                    """.trimIndent(),
//                type = null,
//            )
//
//        val item =
//            mockSyndEntry(
//                uri = "id",
//                description = description,
//            ).asItem(baseUrl)
//
//        assertEquals(
//            "https://o.aolcdn.com/images/dims?crop=1200%2C627%2C0%2C0&quality=85&format=jpg&resize=1600%2C836&image_uri=https%3A%2F%2Fs.yimg.com%2Fos%2Fcreatr-uploaded-images%2F2019-03%2Ffa057c20-5050-11e9-bfef-d1614983d7cc&client=a1acac3e1b3290917d92&signature=351348aa11c53a569d5ad40f3a7ef697471b645a",
//            item.image?.url,
//        )
//    }
//
//    @Test
//    fun thumbnailFromTypeTextIsFound() {
//        val description =
//            mockSyndContent(
//                value =
//                """
//                    <img src="https://o.aolcdn.com/images/dims?crop=1200%2C627%2C0%2C0&quality=85&format=jpg&resize=1600%2C836&image_uri=https%3A%2F%2Fs.yimg.com%2Fos%2Fcreatr-uploaded-images%2F2019-03%2Ffa057c20-5050-11e9-bfef-d1614983d7cc&client=a1acac3e1b3290917d92&signature=351348aa11c53a569d5ad40f3a7ef697471b645a" />Google didn&#039;t completely scrap its robotic dreams after it sold off Boston Dynamics and shuttered the other robotic start-ups it acquired over the past decade. Now, the tech giant has given us a glimpse of how the program has changed in a blog post a...
//                    """.trimIndent(),
//                type = "text",
//            )
//
//        val item =
//            mockSyndEntry(
//                uri = "id",
//                description = description,
//            ).asItem(baseUrl)
//
//        assertEquals(
//            "https://o.aolcdn.com/images/dims?crop=1200%2C627%2C0%2C0&quality=85&format=jpg&resize=1600%2C836&image_uri=https%3A%2F%2Fs.yimg.com%2Fos%2Fcreatr-uploaded-images%2F2019-03%2Ffa057c20-5050-11e9-bfef-d1614983d7cc&client=a1acac3e1b3290917d92&signature=351348aa11c53a569d5ad40f3a7ef697471b645a",
//            item.image?.url,
//        )
//    }
//
//    @Test
//    fun thumbnailFromTypeHtmlIsFound() {
//        val description =
//            mockSyndContent(
//                value =
//                """
//                    <img src="https://o.aolcdn.com/images/dims?crop=1200%2C627%2C0%2C0&quality=85&format=jpg&resize=1600%2C836&image_uri=https%3A%2F%2Fs.yimg.com%2Fos%2Fcreatr-uploaded-images%2F2019-03%2Ffa057c20-5050-11e9-bfef-d1614983d7cc&client=a1acac3e1b3290917d92&signature=351348aa11c53a569d5ad40f3a7ef697471b645a" />Google didn&#039;t completely scrap its robotic dreams after it sold off Boston Dynamics and shuttered the other robotic start-ups it acquired over the past decade. Now, the tech giant has given us a glimpse of how the program has changed in a blog post a...
//                    """.trimIndent(),
//                type = "html",
//            )
//
//        val item =
//            mockSyndEntry(
//                uri = "id",
//                description = description,
//            ).asItem(baseUrl)
//
//        assertEquals(
//            "https://o.aolcdn.com/images/dims?crop=1200%2C627%2C0%2C0&quality=85&format=jpg&resize=1600%2C836&image_uri=https%3A%2F%2Fs.yimg.com%2Fos%2Fcreatr-uploaded-images%2F2019-03%2Ffa057c20-5050-11e9-bfef-d1614983d7cc&client=a1acac3e1b3290917d92&signature=351348aa11c53a569d5ad40f3a7ef697471b645a",
//            item.image?.url,
//        )
//    }
//
//    @Test
//    fun thumbnailFromEnclosureIsFound() {
//        val item =
//            mockSyndEntry(
//                uri = "id",
//                enclosures =
//                listOf(
//                    mockSyndEnclosure(
//                        url = "http://foo/bar.png",
//                        type = "image/png",
//                    ),
//                ),
//            ).asItem(baseUrl)
//
//        assertEquals(
//            "http://foo/bar.png",
//            item.image?.url,
//        )
//    }
//
//    @Test
//    fun publishedRFC3339Date() {
//        // Need to convert it so timezone is correct for test
//        val romeDate = Date(ZonedDateTime.parse("2017-11-15T22:36:36+00:00").toInstant().toEpochMilli())
//        val dateTime = ZonedDateTime.ofInstant(Instant.ofEpochMilli(romeDate.time), ZoneOffset.systemDefault())
//        assertEquals(
//            ParsedArticle(
//                id = "$baseUrl/id",
//                title = "",
//                content_text = "",
//                summary = "",
//                attachments = emptyList(),
//                url = null,
//                date_published = dateTime.toString(),
//            ),
//            mockSyndEntry(
//                uri = "id",
//                publishedDate = romeDate,
//            ).asItem(baseUrl),
//        )
//    }
//
//    @Test
//    fun publishedRFC3339DateFallsBackToModified() {
//        // Need to convert it so timezone is correct for test
//        val romeDate = Date(ZonedDateTime.parse("2017-11-15T22:36:36+00:00").toInstant().toEpochMilli())
//        val dateTime = ZonedDateTime.ofInstant(Instant.ofEpochMilli(romeDate.time), ZoneOffset.systemDefault())
//        assertEquals(
//            ParsedArticle(
//                id = "$baseUrl/id",
//                title = "",
//                content_text = "",
//                summary = "",
//                attachments = emptyList(),
//                url = null,
//                date_modified = dateTime.toString(),
//                date_published = dateTime.toString(),
//            ),
//            mockSyndEntry(
//                uri = "id",
//                updatedDate = romeDate,
//            ).asItem(baseUrl),
//        )
//    }
//
//    @Test
//    fun modifiedRFC3339Date() {
//        // Need to convert it so timezone is correct for test
//        val romePubDate = Date(ZonedDateTime.parse("2017-11-15T22:36:36+00:00").toInstant().toEpochMilli())
//        val romeModDate = Date(ZonedDateTime.parse("2017-11-10T22:36:36+00:00").toInstant().toEpochMilli())
//        val pubDate = ZonedDateTime.ofInstant(Instant.ofEpochMilli(romePubDate.time), ZoneOffset.systemDefault())
//        val modDate = ZonedDateTime.ofInstant(Instant.ofEpochMilli(romeModDate.time), ZoneOffset.systemDefault())
//        assertEquals(
//            ParsedArticle(
//                id = "$baseUrl/id",
//                title = "",
//                content_text = "",
//                summary = "",
//                attachments = emptyList(),
//                url = null,
//                date_modified = modDate.toString(),
//                date_published = pubDate.toString(),
//            ),
//            mockSyndEntry(
//                uri = "id",
//                updatedDate = romeModDate,
//                publishedDate = romePubDate,
//            ).asItem(baseUrl),
//        )
//    }
}

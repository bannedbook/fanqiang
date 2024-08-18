package com.nononsenseapps.feeder.model.html

import com.nononsenseapps.feeder.ui.compose.html.toTableData
import org.junit.Before
import org.junit.Ignore
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue

class HtmlLinearizerTest {
    private lateinit var linearizer: HtmlLinearizer

    @Before
    fun setUp() {
        linearizer = HtmlLinearizer()
    }

    @Test
    fun `should return empty list when input is empty`() {
        val html = "<html><body></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(emptyList(), result)
    }

    @Test
    fun `should return single LinearText when input is simple text`() {
        val html = "<html><body>Hello, world!</body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size)
        assertEquals(LinearText("Hello, world!", LinearTextBlockStyle.TEXT), result[0])
    }

    @Test
    fun `should return annotations with bold, italic, and underline`() {
        val html = "<html><body><b><i><u>Hello, world!</u></i></b></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size)
        assertEquals(
            LinearText(
                "Hello, world!",
                LinearTextBlockStyle.TEXT,
                LinearTextAnnotation(LinearTextAnnotationBold, 0, 12),
                LinearTextAnnotation(LinearTextAnnotationItalic, 0, 12),
                LinearTextAnnotation(LinearTextAnnotationUnderline, 0, 12),
            ),
            result[0],
        )
    }

    @Test
    fun `should return annotations with bold, italic, and underline interleaving`() {
        val html = "<html><body><b><i><u>Hell</u>o</i>, wor</b>ld!</body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size)
        assertEquals(
            LinearText(
                "Hello, world!",
                LinearTextBlockStyle.TEXT,
                LinearTextAnnotation(LinearTextAnnotationBold, 0, 9),
                LinearTextAnnotation(LinearTextAnnotationItalic, 0, 4),
                LinearTextAnnotation(LinearTextAnnotationUnderline, 0, 3),
            ),
            result[0],
        )
    }

    @Test
    fun `should return own item for header`() {
        // separate items for the paragraph and the H1
        val html = "<html><body><h1>Header 1</h1>Hello, world!</body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(2, result.size)
        assertEquals(LinearText("Header 1", LinearTextBlockStyle.TEXT, LinearTextAnnotation(LinearTextAnnotationH1, 0, 7)), result[0])
        assertEquals(LinearText("Hello, world!", LinearTextBlockStyle.TEXT), result[1])
    }

    @Test
    fun `should return single item for nested divs`() {
        val html = "<html><body><div><div>Hello, world!</div></div></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size)
        assertEquals(LinearText("Hello, world!", LinearTextBlockStyle.TEXT), result[0])
    }

    @Test
    fun `should return ordered LinearList for ol`() {
        val html = "<html><body><ol><li>Item 1</li><li>Item 2</li></ol></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearList(
                ordered = true,
                items =
                    listOf(
                        LinearListItem(
                            LinearText("Item 1", LinearTextBlockStyle.TEXT),
                        ),
                        LinearListItem(
                            LinearText("Item 2", LinearTextBlockStyle.TEXT),
                        ),
                    ),
            ),
            result[0],
        )
    }

    @Test
    fun `should return unordered LinearList for ul`() {
        val html = "<html><body><ul><li>Item 1</li><li>Item 2</li></ul></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearList(
                ordered = false,
                items =
                    listOf(
                        LinearListItem(
                            LinearText("Item 1", LinearTextBlockStyle.TEXT),
                        ),
                        LinearListItem(
                            LinearText("Item 2", LinearTextBlockStyle.TEXT),
                        ),
                    ),
            ),
            result[0],
        )
    }

    @Test
    fun `surrounding span is preserved with list in middle`() {
        val html = "<html><body><b>Before it<ul><li>Item 1</li><li>Item 22</li></ul>After</b></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(3, result.size, "Expected three items: $result")
        assertEquals(
            LinearText("Before it", LinearTextBlockStyle.TEXT, LinearTextAnnotation(LinearTextAnnotationBold, 0, 8)),
            result[0],
        )
        assertEquals(
            LinearList(
                ordered = false,
                items =
                    listOf(
                        LinearListItem(
                            LinearText("Item 1", LinearTextBlockStyle.TEXT, LinearTextAnnotation(data = LinearTextAnnotationBold, start = 0, end = 5)),
                        ),
                        LinearListItem(
                            LinearText("Item 22", LinearTextBlockStyle.TEXT, LinearTextAnnotation(data = LinearTextAnnotationBold, start = 0, end = 6)),
                        ),
                    ),
            ),
            result[1],
        )
        assertEquals(
            LinearText("After", LinearTextBlockStyle.TEXT, LinearTextAnnotation(data = LinearTextAnnotationBold, start = 0, end = 4)),
            result[2],
        )
    }

    @Test
    fun `simple image with alt text should return single Image`() {
        val html = "<html><body><img src=\"https://example.com/image.jpg\" alt=\"Alt text\"/></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearImage(
                sources =
                    listOf(
                        LinearImageSource(imgUri = "https://example.com/image.jpg", widthPx = null, heightPx = null, pixelDensity = null, screenWidth = null),
                    ),
                caption = LinearText("Alt text", LinearTextBlockStyle.TEXT),
                link = null,
            ),
            result[0],
        )
    }

    @Test
    fun `simple image with bold alt text should return no formatting`() {
        val html = "<html><body><img src=\"https://example.com/image.jpg\" alt=\"<b>Bold</b> text\"/></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearImage(
                sources =
                    listOf(
                        LinearImageSource(imgUri = "https://example.com/image.jpg", widthPx = null, heightPx = null, pixelDensity = null, screenWidth = null),
                    ),
                caption = LinearText("Bold text", LinearTextBlockStyle.TEXT),
                link = null,
            ),
            result[0],
        )
    }

    @Test
    fun `simple image inside a link`() {
        val html = "<html><body><a href=\"https://example.com/link\"><img src=\"https://example.com/image.jpg\" alt=\"Alt text\"/></a></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearImage(
                sources =
                    listOf(
                        LinearImageSource(imgUri = "https://example.com/image.jpg", widthPx = null, heightPx = null, pixelDensity = null, screenWidth = null),
                    ),
                caption = LinearText("Alt text", LinearTextBlockStyle.TEXT),
                link = "https://example.com/link",
            ),
            result[0],
        )
    }

    @Test
    fun `simple image with defined size`() {
        val html = "<html><body><img src=\"https://example.com/image.jpg\" width=\"100\" height=\"200\"/></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearImage(
                sources =
                    listOf(
                        LinearImageSource(imgUri = "https://example.com/image.jpg", widthPx = 100, heightPx = 200, pixelDensity = null, screenWidth = null),
                    ),
                caption = null,
                link = null,
            ),
            result[0],
        )
    }

    @Test
    fun `srcset image with pixel density and screenwidth versions available`() {
        val html =
            """
            <html><body>
            <img srcset="https://example.com/image.jpg 1x, https://example.com/image-2x.jpg 2x, https://example.com/image-700w.jpg 700w, https://example.com/image-fallback.jpg"/>
            </body></html>
            """.trimIndent()
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearImage(
                sources =
                    listOf(
                        LinearImageSource(imgUri = "https://example.com/image.jpg", widthPx = null, heightPx = null, screenWidth = null, pixelDensity = 1f),
                        LinearImageSource(imgUri = "https://example.com/image-2x.jpg", widthPx = null, heightPx = null, screenWidth = null, pixelDensity = 2f),
                        LinearImageSource(imgUri = "https://example.com/image-700w.jpg", widthPx = null, heightPx = null, screenWidth = 700, pixelDensity = null),
                        LinearImageSource(imgUri = "https://example.com/image-fallback.jpg", widthPx = null, heightPx = null, screenWidth = null, pixelDensity = null),
                    ),
                caption = null,
                link = null,
            ),
            result[0],
        )
    }

    @Test
    fun `simple image with dataImgUrl`() {
        val html = "<html><body><img data-img-url=\"https://example.com/image.jpg\"/></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearImage(
                sources =
                    listOf(
                        LinearImageSource(imgUri = "https://example.com/image.jpg", widthPx = null, heightPx = null, pixelDensity = null, screenWidth = null),
                    ),
                caption = null,
                link = null,
            ),
            result[0],
        )
    }

    @Test
    fun `simple image with relative url`() {
        val html = "<html><body><img src=\"/image.jpg\"/></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearImage(
                sources =
                    listOf(
                        LinearImageSource(imgUri = "https://example.com/image.jpg", widthPx = null, heightPx = null, pixelDensity = null, screenWidth = null),
                    ),
                caption = null,
                link = null,
            ),
            result[0],
        )
    }

    @Test
    fun `simple figure image with figcaption`() {
        val html = "<html><body><figure><img src=\"https://example.com/image.jpg\"/><figcaption>Alt <b>t</b>ext</figcaption></figure></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearImage(
                sources =
                    listOf(
                        LinearImageSource(imgUri = "https://example.com/image.jpg", widthPx = null, heightPx = null, pixelDensity = null, screenWidth = null),
                    ),
                caption = LinearText("Alt text", LinearTextBlockStyle.TEXT, LinearTextAnnotation(LinearTextAnnotationBold, 4, 4)),
                link = null,
            ),
            result[0],
        )
    }

    @Test
    fun `figure inside a link`() {
        val html =
            """
            <html><body><a href="https://example.com/link">
            <figure><img src="https://example.com/image.jpg"/><figcaption>Alt text</figcaption></figure>
            </a></body></html>
            """.trimIndent()
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearImage(
                sources =
                    listOf(
                        LinearImageSource(imgUri = "https://example.com/image.jpg", widthPx = null, heightPx = null, pixelDensity = null, screenWidth = null),
                    ),
                caption = LinearText("Alt text", LinearTextBlockStyle.TEXT, LinearTextAnnotation(data = LinearTextAnnotationLink("https://example.com/link"), start = 0, end = 7)),
                link = "https://example.com/link",
            ),
            result[0],
        )
    }

    @Test
    fun `p in a blockquote does not add newlines at end and cite is null`() {
        val html = "<html><body><blockquote><p>Quote</p></blockquote></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertTrue(result[0] is LinearBlockQuote, "Expected LinearBlockQuote: $result")
        assertEquals(
            LinearBlockQuote(
                cite = null,
                content = listOf(LinearText("Quote", LinearTextBlockStyle.TEXT)),
            ),
            result[0],
        )
    }

    @Test
    fun `figure with two img tags one with srcset and one with dataImgUrl - only distinct results`() {
        val html =
            """
            <html><body><figure>
            <img srcset="https://example.com/image.jpg 1x, https://example.com/image-2x.jpg 2x"/>
            <img data-img-url="https://example.com/image.jpg"/>
            </figure></body></html>
            """.trimIndent()
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearImage(
                sources =
                    listOf(
                        LinearImageSource(imgUri = "https://example.com/image.jpg", widthPx = null, heightPx = null, pixelDensity = 1f, screenWidth = null),
                        LinearImageSource(imgUri = "https://example.com/image-2x.jpg", widthPx = null, heightPx = null, pixelDensity = 2f, screenWidth = null),
                    ),
                caption = null,
                link = null,
            ),
            result[0],
        )
    }

    @Test
    fun `figure with two img tags one with srcset and one with dataImgUrl all urls different`() {
        val html =
            """
            <html><body><figure>
            <img srcset="https://example.com/image.jpg 1x, https://example.com/image-2x.jpg 2x"/>
            <img data-img-url="https://example.com/image-3x.jpg"/>
            </figure></body></html>
            """.trimIndent()
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearImage(
                sources =
                    listOf(
                        LinearImageSource(imgUri = "https://example.com/image.jpg", widthPx = null, heightPx = null, pixelDensity = 1f, screenWidth = null),
                        LinearImageSource(imgUri = "https://example.com/image-2x.jpg", widthPx = null, heightPx = null, pixelDensity = 2f, screenWidth = null),
                        LinearImageSource(imgUri = "https://example.com/image-3x.jpg", widthPx = null, heightPx = null, pixelDensity = null, screenWidth = null),
                    ),
                caption = null,
                link = null,
            ),
            result[0],
        )
    }

    @Test
    fun `pre block with code tag`() {
        val html = "<html><body><pre><code>\nCode\n  block\n</code></pre></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearText("\nCode\n  block", LinearTextBlockStyle.CODE_BLOCK, LinearTextAnnotation(LinearTextAnnotationCode, 0, 12)),
            result[0],
        )
    }

    @Test
    fun `pre block`() {
        val html = "<html><body><pre>\nCode\n  block\n</pre></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearText("\nCode\n  block", LinearTextBlockStyle.PRE_FORMATTED),
            result[0],
        )
    }

    @Test
    fun `pre block without code tag`() {
        val html = "<html><body><pre>Not a code block</pre></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearText("Not a code block", LinearTextBlockStyle.PRE_FORMATTED),
            result[0],
        )
    }

    @Test
    fun `audio with no sources is ignored`() {
        val html = "<html><body><audio controls></audio></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(emptyList(), result)
    }

    @Test
    fun `audio with two sources`() {
        val html =
            """
            <html><body>
            <audio controls><source src="audio.mp3" type="audio/mpeg">
            <source src="https://example.com/audio.ogg" type="audio/ogg"></audio></body></html>
            """.trimIndent()
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearAudio(
                sources =
                    listOf(
                        LinearAudioSource("https://example.com/audio.mp3", "audio/mpeg"),
                        LinearAudioSource("https://example.com/audio.ogg", "audio/ogg"),
                    ),
            ),
            result[0],
        )
    }

    @Test
    fun `content inside blockquote`() {
        val html =
            """
            <p><a class="tumblr_blog" href="https://everydaylouie.tumblr.com/post/716429042034802688/challenge">everydaylouie</a>:</p>
            <blockquote>
            <div class="npf_row">
            <figure class="tmblr-full" data-orig-height="624" data-orig-width="1200">
            <img src="https://64.media.tumblr.com/55359709d06eb4309538800b64745d40/5149f25c974a1479-f7/s640x960/75e2a17fad52257ac5c7326e123d125be18ecd14.png" data-orig-height="624" data-orig-width="1200" srcset="https://64.media.tumblr.com/55359709d06eb4309538800b64745d40/5149f25c974a1479-f7/s75x75_c1/48387a66ca77d9e8af781b47efb733f8e5abe553.png 75w, https://64.media.tumblr.com/55359709d06eb4309538800b64745d40/5149f25c974a1479-f7/s100x200/9387531174b8be87ae89fd6f967e274336b7f5a4.png 100w, https://64.media.tumblr.com/55359709d06eb4309538800b64745d40/5149f25c974a1479-f7/s250x400/435ce8fba2aa5f75a548a497f1157f563245dd93.png 250w, https://64.media.tumblr.com/55359709d06eb4309538800b64745d40/5149f25c974a1479-f7/s400x600/3fc4822b65cab32e05ed0611b6721ab3ab0ba5c8.png 400w, https://64.media.tumblr.com/55359709d06eb4309538800b64745d40/5149f25c974a1479-f7/s500x750/f86d4df43eb0de4695d3261cca41739278c9e6bc.png 500w, https://64.media.tumblr.com/55359709d06eb4309538800b64745d40/5149f25c974a1479-f7/s540x810/64c4028177a81a99d416b8994b07b8d228b25ea5.png 540w, https://64.media.tumblr.com/55359709d06eb4309538800b64745d40/5149f25c974a1479-f7/s640x960/75e2a17fad52257ac5c7326e123d125be18ecd14.png 640w, https://64.media.tumblr.com/55359709d06eb4309538800b64745d40/5149f25c974a1479-f7/s1280x1920/258a6c4e80c6403aa4163c73cf39c09183dd4b4a.png 1200w" sizes="(max-width: 1200px) 100vw, 1200px"/>
            </figure></div><p>CHALLENGE!</p></blockquote>
            """.trimIndent()

        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(3, result.size, "Expected items: $result")

        assertTrue { result[1] is LinearImage }
    }

    @Ignore
    @Test
    fun `audio in iframe`() {
        val html =
            """
            <iframe 
            class="tumblr_audio_player tumblr_audio_player_718384010494197760" 
            src="https://pokemonmysterydungeon.tumblr.com/post/718384010494197760/audio_player_iframe/pokemonmysterydungeon/tumblr_pgyj6gSPFJ1skzors?audio_file=https%3A%2F%2Fa.tumblr.com%2Ftumblr_pgyj6gSPFJ1skzorso1.mp3"
            frameborder="0" allowtransparency="true" scrolling="no" width="540" height="169">
            </iframe>
            """.trimIndent()
    }

    @Test
    fun `video with no sources is ignored`() {
        val html = "<html><body><video controls></video></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(emptyList(), result)
    }

    @Test
    fun `video with two sources`() {
        val html =
            """
            <html><body>
            <video controls>
            <source src="video.mp4" type="video/mp4">
            <source src="https://example.com/video.webm" type="video/webm"></video></body></html>
            """.trimIndent()
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearVideo(
                sources =
                    listOf(
                        LinearVideoSource("https://example.com/video.mp4", "https://example.com/video.mp4", null, null, null, "video/mp4"),
                        LinearVideoSource("https://example.com/video.webm", "https://example.com/video.webm", null, null, null, "video/webm"),
                    ),
            ),
            result[0],
        )
    }

    @Test
    fun `video with 1 source and negative width`() {
        val html = "<html><body><video controls width=\"-1\"><source src=\"video.mp4\" type=\"video/mp4\"></video></body></html>"
        val baseUrl = "https://example.com"
        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearVideo(
                sources =
                    listOf(
                        LinearVideoSource("https://example.com/video.mp4", "https://example.com/video.mp4", null, null, null, "video/mp4"),
                    ),
            ),
            result[0],
        )
    }

    @Test
    fun `image with negative heightPx`() {
        val html = "<html><body><img src=\"https://example.com/image.jpg\" height=\"-1\"/></body></html>"
        val baseUrl = "https://example.com"
        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearImage(
                sources =
                    listOf(
                        LinearImageSource(imgUri = "https://example.com/image.jpg", widthPx = null, heightPx = null, pixelDensity = null, screenWidth = null),
                    ),
                caption = null,
                link = null,
            ),
            result[0],
        )
    }

    @Test
    fun `iframe with youtube video`() {
        val html = "<html><body><iframe src=\"https://www.youtube.com/embed/cjxnVO9RpaQ\"></iframe></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearVideo(
                sources =
                    listOf(
                        LinearVideoSource(
                            "https://www.youtube.com/embed/cjxnVO9RpaQ",
                            "https://www.youtube.com/watch?v=cjxnVO9RpaQ",
                            "http://img.youtube.com/vi/cjxnVO9RpaQ/hqdefault.jpg",
                            480,
                            360,
                            null,
                        ),
                    ),
            ),
            result[0],
        )
    }

    @Test
    fun `iframe inside figure with youtube video`() {
        // Seen on AlltOmElbil.se
        val html =
            """
            <html><body>
            <figure class="wp-block-embed is-type-video is-provider-youtube wp-block-embed-youtube wp-embed-aspect-16-9 wp-has-aspect-ratio"><div class="wp-block-embed__wrapper">
            <iframe title="Därför är el-lastbilar bättre än diesel-lastbilar" width="1170" height="658" src="https://www.youtube.com/embed/x_m02bUxfvE?feature=oembed" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen=""></iframe>
            </div></figure>
            </body></html>
            """.trimIndent()
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearVideo(
                sources =
                    listOf(
                        LinearVideoSource(
                            "https://www.youtube.com/embed/x_m02bUxfvE?feature=oembed",
                            "https://www.youtube.com/watch?v=x_m02bUxfvE",
                            "http://img.youtube.com/vi/x_m02bUxfvE/hqdefault.jpg",
                            1170,
                            658,
                            null,
                        ),
                    ),
            ),
            result[0],
        )
    }

    @Test
    fun `table block 2x2`() {
        val html = "<html><body><table><tr><th>1</th><td>2</td></tr><tr><td>3</td><th>4</th></tr></table></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearTable.build(leftToRight = true) {
                newRow()
                add(LinearTableCellItem(type = LinearTableCellItemType.HEADER, colSpan = 1, rowSpan = 1, content = listOf(LinearText("1", LinearTextBlockStyle.TEXT))))
                add(LinearTableCellItem(type = LinearTableCellItemType.DATA, colSpan = 1, rowSpan = 1, content = listOf(LinearText("2", LinearTextBlockStyle.TEXT))))
                newRow()
                add(LinearTableCellItem(type = LinearTableCellItemType.DATA, colSpan = 1, rowSpan = 1, content = listOf(LinearText("3", LinearTextBlockStyle.TEXT))))
                add(LinearTableCellItem(type = LinearTableCellItemType.HEADER, colSpan = 1, rowSpan = 1, content = listOf(LinearText("4", LinearTextBlockStyle.TEXT))))
            },
            result[0],
        )
    }

    @Test
    fun `table block 2x2 rtl`() {
        val html = "<html dir=\"rtl\"><body><table><tr><th>1</th><td>2</td></tr><tr><td>3</td><th>4</th></tr></table></body></html>"
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        assertEquals(
            LinearTable.build(leftToRight = true) {
                newRow()
                add(LinearTableCellItem(type = LinearTableCellItemType.DATA, colSpan = 1, rowSpan = 1, content = listOf(LinearText("2", LinearTextBlockStyle.TEXT))))
                add(LinearTableCellItem(type = LinearTableCellItemType.HEADER, colSpan = 1, rowSpan = 1, content = listOf(LinearText("1", LinearTextBlockStyle.TEXT))))
                newRow()
                add(LinearTableCellItem(type = LinearTableCellItemType.HEADER, colSpan = 1, rowSpan = 1, content = listOf(LinearText("4", LinearTextBlockStyle.TEXT))))
                add(LinearTableCellItem(type = LinearTableCellItemType.DATA, colSpan = 1, rowSpan = 1, content = listOf(LinearText("3", LinearTextBlockStyle.TEXT))))
            },
            result[0],
        )
    }

    @Test
    fun `table with colspan, rowspan, and a double span`() {
        val html =
            """
            <html><body>
            <table>
            <tr>
                <th>Name</th>
                <th>Age</th>
                <th colspan="2">Money Money</th>
            </tr>
            <tr>
                <td rowspan="2" colspan="2">Bob</td>
                <td>${'$'}300</td>
                <td>0</td>
            </tr>
            <tr>
                <td>${'$'}400</td>
                <td>1</td>
            </tr>
            </table></body></html>
            """.trimIndent()
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        val table = result[0] as LinearTable
        assertEquals(
            LinearTable.build(leftToRight = true) {
                newRow()
                add(LinearTableCellItem(type = LinearTableCellItemType.HEADER, colSpan = 1, rowSpan = 1, content = listOf(LinearText("Name", LinearTextBlockStyle.TEXT))))
                add(LinearTableCellItem(type = LinearTableCellItemType.HEADER, colSpan = 1, rowSpan = 1, content = listOf(LinearText("Age", LinearTextBlockStyle.TEXT))))
                add(LinearTableCellItem(type = LinearTableCellItemType.HEADER, colSpan = 2, rowSpan = 1, content = listOf(LinearText("Money Money", LinearTextBlockStyle.TEXT))))
                newRow()
                add(LinearTableCellItem(type = LinearTableCellItemType.DATA, colSpan = 2, rowSpan = 2, content = listOf(LinearText("Bob", LinearTextBlockStyle.TEXT))))
                add(LinearTableCellItem(type = LinearTableCellItemType.DATA, colSpan = 1, rowSpan = 1, content = listOf(LinearText("${'$'}300", LinearTextBlockStyle.TEXT))))
                add(LinearTableCellItem(type = LinearTableCellItemType.DATA, colSpan = 1, rowSpan = 1, content = listOf(LinearText("0", LinearTextBlockStyle.TEXT))))
                newRow()
                add(LinearTableCellItem(type = LinearTableCellItemType.DATA, colSpan = 1, rowSpan = 1, content = listOf(LinearText("${'$'}400", LinearTextBlockStyle.TEXT))))
                add(LinearTableCellItem(type = LinearTableCellItemType.DATA, colSpan = 1, rowSpan = 1, content = listOf(LinearText("1", LinearTextBlockStyle.TEXT))))
            },
            table,
        )

        assertEquals(4, table.colCount, "Expected 4 columns: $table")
        assertEquals(3, table.rowCount, "Expected 3 rows: $table")
    }

    @Test
    fun `table with zero colspan spans entire row`() {
        val html =
            """
            <html><body>
            <table>
            <tr>
                <th>Name</th>
                <th>Age</th>
                <th>Money</th>
            </tr>
            <tr>
                <td colspan="0">Bob</td>
            </tr>
            </table></body></html>
            """.trimIndent()
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        val table = result[0] as LinearTable
        assertEquals(
            LinearTable.build(leftToRight = true) {
                newRow()
                add(LinearTableCellItem(type = LinearTableCellItemType.HEADER, colSpan = 1, rowSpan = 1, content = listOf(LinearText("Name", LinearTextBlockStyle.TEXT))))
                add(LinearTableCellItem(type = LinearTableCellItemType.HEADER, colSpan = 1, rowSpan = 1, content = listOf(LinearText("Age", LinearTextBlockStyle.TEXT))))
                add(LinearTableCellItem(type = LinearTableCellItemType.HEADER, colSpan = 1, rowSpan = 1, content = listOf(LinearText("Money", LinearTextBlockStyle.TEXT))))
                newRow()
                add(LinearTableCellItem(type = LinearTableCellItemType.DATA, colSpan = 0, rowSpan = 1, content = listOf(LinearText("Bob", LinearTextBlockStyle.TEXT))))
            },
            table,
        )

        assertEquals(3, table.colCount, "Expected 3 columns: $table")
        assertEquals(2, table.rowCount, "Expected 2 rows: $table")
    }

    @Test
    fun `cowboy table`() {
        val html =
            """
                <html><body>
            <table>
            <caption>
            <p>Table 1.
            </p><p>This table demonstrates the table rendering capabilities of Feeder's Reader view. This caption
            is by the spec allowed to contain most objects, except other tables. See
            <a href="https://www.w3.org/TR/2014/REC-html5-20141028/dom.html#flow-content-1">flow content</a>.
            </p></caption>
            <thead>
            <tr>
            <th>Name
            </th><th>Number
            </th><th>Money
            </th></tr><tr>
            <th>First and Last name
            </th><th>What number human are you?
            </th><th>How much money have you collected?
            </th></tr></thead><tfoot>
            <tr>
            <th>No Comment
            </th><th>Early!
            </th><th>Sad
            </th></tr></tfoot><tbody>
            <tr>
            <td>Bob
            </td><td>66
            </td><td>${'$'}3
            </td></tr><tr>
            <td>Alice
            </td><td>999
            </td><td>${'$'}999999
            </td></tr><tr>
            <td>:O
            </td><td colspan="2">OMG Col span 2
            </td></tr><tr>
            <td colspan="3">WHAAAT. Triple span?!
            </td></tr><tr>
            <td colspan="0">Firefox special zero span means to the end!
            </td></tr></tbody>
            </table></body></html>
            """.trimIndent()

        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        val table = result[0] as LinearTable

        // Filler items are dropped
        assertEquals(table.cells.size - 3, table.toTableData().cells.size, "Expected filler items to be dropped")
    }

    @Test
    fun `table with thead, tbody, and tfoot`() {
        val html =
            """
            <html><body>
            <table>
                <thead>
                    <tr><th>Name<th>Number<th>Money
                <tbody>
                    <tr><td>Bob<td>66<td>${'$'}3
                    <tr><td>Alice<td>999<td>${'$'}999999
                <tfoot>
                    <tr><td>No Comment<td>Early!<td>Sad
            </table>
            </body></html>
            """.trimIndent()
        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected one item: $result")
        val expected =
            LinearTable.build(leftToRight = true) {
                newRow()
                add(LinearTableCellItem(type = LinearTableCellItemType.HEADER, colSpan = 1, rowSpan = 1, content = listOf(LinearText("Name", LinearTextBlockStyle.TEXT))))
                add(LinearTableCellItem(type = LinearTableCellItemType.HEADER, colSpan = 1, rowSpan = 1, content = listOf(LinearText("Number", LinearTextBlockStyle.TEXT))))
                add(LinearTableCellItem(type = LinearTableCellItemType.HEADER, colSpan = 1, rowSpan = 1, content = listOf(LinearText("Money", LinearTextBlockStyle.TEXT))))
                newRow()
                add(LinearTableCellItem(type = LinearTableCellItemType.DATA, colSpan = 1, rowSpan = 1, content = listOf(LinearText("Bob", LinearTextBlockStyle.TEXT))))
                add(LinearTableCellItem(type = LinearTableCellItemType.DATA, colSpan = 1, rowSpan = 1, content = listOf(LinearText("66", LinearTextBlockStyle.TEXT))))
                add(LinearTableCellItem(type = LinearTableCellItemType.DATA, colSpan = 1, rowSpan = 1, content = listOf(LinearText("${'$'}3", LinearTextBlockStyle.TEXT))))
                newRow()
                add(LinearTableCellItem(type = LinearTableCellItemType.DATA, colSpan = 1, rowSpan = 1, content = listOf(LinearText("Alice", LinearTextBlockStyle.TEXT))))
                add(LinearTableCellItem(type = LinearTableCellItemType.DATA, colSpan = 1, rowSpan = 1, content = listOf(LinearText("999", LinearTextBlockStyle.TEXT))))
                add(LinearTableCellItem(type = LinearTableCellItemType.DATA, colSpan = 1, rowSpan = 1, content = listOf(LinearText("${'$'}999999", LinearTextBlockStyle.TEXT))))
                newRow()
                add(LinearTableCellItem(type = LinearTableCellItemType.DATA, colSpan = 1, rowSpan = 1, content = listOf(LinearText("No Comment", LinearTextBlockStyle.TEXT))))
                add(LinearTableCellItem(type = LinearTableCellItemType.DATA, colSpan = 1, rowSpan = 1, content = listOf(LinearText("Early!", LinearTextBlockStyle.TEXT))))
                add(LinearTableCellItem(type = LinearTableCellItemType.DATA, colSpan = 1, rowSpan = 1, content = listOf(LinearText("Sad", LinearTextBlockStyle.TEXT))))
            }
        val firstDiffIndex =
            expected.cells.map { (key, linearTableCellItem) ->
                val other = (result[0] as LinearTable).cells[key]
                if (linearTableCellItem != other) {
                    key
                } else {
                    null
                }
            }.filterNotNull().firstOrNull()
        val firstDiff: String? =
            firstDiffIndex?.let { index ->
                "First differing cell at index $index: ${expected.cells[index]} vs ${(result[0] as LinearTable).cells[index]}"
            }
        assertEquals(
            expected,
            result[0],
            firstDiff ?: "Expected table: $expected\nActual table: ${result[0]}",
        )
    }

    @Test
    fun `arctechnica list items are actually images readability4j`() {
        val html =
            """
            <ul> 
             <li data-thumb="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-01-150x150.jpeg" data-src="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-01.jpeg" data-responsive="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-01-980x735.jpeg 1080, https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-01.jpeg 2560" data-sub-html="#caption-2025663"> 
              <figure> 
               <figcaption id="caption-2025663"> 
                <span></span> 
                <p> Microsoft's Surface Pro 11 comes with Arm chips and an optional OLED display panel. </p> 
                <p> <span></span> Microsoft </p> 
               </figcaption> 
              </figure> </li> 
             <li data-thumb="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-02-150x150.jpeg" data-src="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-02.jpeg" data-responsive="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-02-980x735.jpeg 1080, https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-02.jpeg 2560" data-sub-html="#caption-2025664"> 
              <figure> 
               <figcaption id="caption-2025664"> 
                <span></span> 
                <p> The Surface Pro 11's design is near-identical to the Surface Pro 8 and Surface Pro 9, and they're compatible with the same accessories. </p> 
                <p> <span></span> Microsoft </p> 
               </figcaption> 
              </figure> </li> 
             <li data-thumb="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-03-150x150.jpeg" data-src="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-03.jpeg" data-responsive="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-03-980x735.jpeg 1080, https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-03.jpeg 2560" data-sub-html="#caption-2025665"> 
              <figure> 
               <figcaption id="caption-2025665"> 
                <span></span> 
                <p> Two USB-C ports, no headphone jack. A Smart Connect port is on the other side. </p> 
                <p> <span></span> Microsoft </p> 
               </figcaption> 
              </figure> </li> 
             <li data-thumb="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-01-150x150.jpeg" data-src="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-01.jpeg" data-responsive="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-01-980x735.jpeg 1080, https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-01.jpeg 2560" data-sub-html="#caption-2025667"> 
              <figure> 
               <figcaption id="caption-2025667"> 
                <span></span> 
                <p> The new Surface Laptop 7, available in 13.8- and 15-inch models. </p> 
                <p> <span></span> Microsoft </p> 
               </figcaption> 
              </figure> </li> 
             <li data-thumb="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-02-150x150.jpeg" data-src="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-02.jpeg" data-responsive="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-02-980x735.jpeg 1080, https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-02.jpeg 2560" data-sub-html="#caption-2025668"> 
              <figure> 
               <figcaption id="caption-2025668"> 
                <span></span> 
                <p> The keyboard, complete with Copilot key. </p> 
                <p> <span></span> Microsoft </p> 
               </figcaption> 
              </figure> </li> 
             <li data-thumb="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-03-150x150.jpeg" data-src="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-03.jpeg" data-responsive="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-03-980x735.jpeg 1080, https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-03.jpeg 2560" data-sub-html="#caption-2025669"> 
              <figure> 
               <figcaption id="caption-2025669"> 
                <span></span> 
                <p> You get one more USB-C port than you did before. USB-A, Smart Connect, and the headphone jack are also present and accounted for. </p> 
                <p> <span></span> Microsoft </p> 
               </figcaption> 
              </figure> </li> 
            </ul>
            """.trimIndent()

        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected items: $result")

        // Expect one un ordered list
        val linearList = result[0] as LinearList

        // This has 6 items
        assertEquals(6, linearList.items.size, "Expected list items: $linearList")

        // All contain only a single image
        linearList.items.forEach {
            assertEquals(1, it.content.size, "Expected single image: $it")
            // Image url ends with jpeg
            val image = it.content[0] as LinearImage
            assertTrue("Expected jpeg image: $image") {
                image.sources[0].imgUri.startsWith("https://cdn.arstechnica.net/wp-content/uploads/2024/05/")
                image.sources[0].imgUri.endsWith(".jpeg")
            }
        }
    }

    @Test
    fun `arstechnica list items are actually images`() {
        val html =
            """
            <div class="gallery shortcode-gallery gallery-wide">
              <ul>
                        <li data-thumb="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-01-150x150.jpeg" data-src="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-01.jpeg" data-responsive="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-01-980x735.jpeg 1080, https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-01.jpeg 2560" data-sub-html="#caption-2025663">
                    <figure style="height:735px;">
                      <div class="image" style="background-image:url('https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-01-980x735.jpeg'); background-color:#000"></div>
                                    <figcaption id="caption-2025663">
                          <span class="icon caption-arrow icon-drop-indicator"></span>
                                            <div class="caption">
                              Microsoft's Surface Pro 11 comes with Arm chips and an optional OLED display panel.                  </div>
                                                            <div class="credit">
                              <span class="icon icon-camera"></span>
                                                    Microsoft                                      </div>
                                        </figcaption>
                                </figure>
                  </li>
                        <li data-thumb="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-02-150x150.jpeg" data-src="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-02.jpeg" data-responsive="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-02-980x735.jpeg 1080, https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-02.jpeg 2560" data-sub-html="#caption-2025664">
                    <figure style="height:735px;">
                      <div class="image" style="background-image:url('https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-02-980x735.jpeg'); background-color:#000"></div>
                                    <figcaption id="caption-2025664">
                          <span class="icon caption-arrow icon-drop-indicator"></span>
                                            <div class="caption">
                              The Surface Pro 11's design is near-identical to the Surface Pro 8 and Surface Pro 9, and they're compatible with the same accessories.                  </div>
                                                            <div class="credit">
                              <span class="icon icon-camera"></span>
                                                    Microsoft                                      </div>
                                        </figcaption>
                                </figure>
                  </li>
                        <li data-thumb="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-03-150x150.jpeg" data-src="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-03.jpeg" data-responsive="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-03-980x735.jpeg 1080, https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-03.jpeg 2560" data-sub-html="#caption-2025665">
                    <figure style="height:735px;">
                      <div class="image" style="background-image:url('https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-11-03-980x735.jpeg'); background-color:#000"></div>
                                    <figcaption id="caption-2025665">
                          <span class="icon caption-arrow icon-drop-indicator"></span>
                                            <div class="caption">
                              Two USB-C ports, no headphone jack. A Smart Connect port is on the other side.                  </div>
                                                            <div class="credit">
                              <span class="icon icon-camera"></span>
                                                    Microsoft                                      </div>
                                        </figcaption>
                                </figure>
                  </li>
                        <li data-thumb="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-01-150x150.jpeg" data-src="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-01.jpeg" data-responsive="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-01-980x735.jpeg 1080, https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-01.jpeg 2560" data-sub-html="#caption-2025667">
                    <figure style="height:735px;">
                      <div class="image" style="background-image:url('https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-01-980x735.jpeg'); background-color:#000"></div>
                                    <figcaption id="caption-2025667">
                          <span class="icon caption-arrow icon-drop-indicator"></span>
                                            <div class="caption">
                              The new Surface Laptop 7, available in 13.8- and 15-inch models.                  </div>
                                                            <div class="credit">
                              <span class="icon icon-camera"></span>
                                                    Microsoft                                      </div>
                                        </figcaption>
                                </figure>
                  </li>
                        <li data-thumb="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-02-150x150.jpeg" data-src="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-02.jpeg" data-responsive="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-02-980x735.jpeg 1080, https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-02.jpeg 2560" data-sub-html="#caption-2025668">
                    <figure style="height:735px;">
                      <div class="image" style="background-image:url('https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-02-980x735.jpeg'); background-color:#000"></div>
                                    <figcaption id="caption-2025668">
                          <span class="icon caption-arrow icon-drop-indicator"></span>
                                            <div class="caption">
                              The keyboard, complete with Copilot key.                  </div>
                                                            <div class="credit">
                              <span class="icon icon-camera"></span>
                                                    Microsoft                                      </div>
                                        </figcaption>
                                </figure>
                  </li>
                        <li data-thumb="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-03-150x150.jpeg" data-src="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-03.jpeg" data-responsive="https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-03-980x735.jpeg 1080, https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-03.jpeg 2560" data-sub-html="#caption-2025669">
                    <figure style="height:735px;">
                      <div class="image" style="background-image:url('https://cdn.arstechnica.net/wp-content/uploads/2024/05/surface-laptop-03-980x735.jpeg'); background-color:#000"></div>
                                    <figcaption id="caption-2025669">
                          <span class="icon caption-arrow icon-drop-indicator"></span>
                                            <div class="caption">
                              You get one more USB-C port than you did before. USB-A, Smart Connect, and the headphone jack are also present and accounted for.                  </div>
                                                            <div class="credit">
                              <span class="icon icon-camera"></span>
                                                    Microsoft                                      </div>
                                        </figcaption>
                                </figure>
                  </li>
                    </ul>
            </div>
            """.trimIndent()

        val baseUrl = "https://arstechnica.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.size, "Expected items: $result")

        // Expect one un ordered list
        val linearList = result[0] as LinearList

        // This has 6 items
        assertEquals(6, linearList.items.size, "Expected list items: $linearList")

        // All contain only a single image
        linearList.items.forEach {
            assertEquals(1, it.content.size, "Expected single image: $it")
            // Image url ends with jpeg
            val image = it.content[0] as LinearImage
            assertTrue("Expected jpeg image: $image") {
                image.sources[0].imgUri.startsWith("https://cdn.arstechnica.net/wp-content/uploads/2024/05/")
                image.sources[0].imgUri.endsWith(".jpeg")
            }
        }
    }

    @Test
    fun `test with feeder news changelog`() {
        val html =
            """
            <p>Aitor Salaberria (1):</p>
            <ul>
            <li>[d719ced2] Translated using Weblate (Basque)</li>
            </ul>
            <p>Belmar BegiÄ‡ (1):</p>
            <ul>
            <li>[42e567d5] Updated Bosnian translation using Weblate</li>
            </ul>
            <p>Jonas Kalderstam (7):</p>
            <ul>
            <li>[f2486f3c] Upgraded some dependency versions</li>
            <li>[e69ed180] Fixed sync indicator: should now stay on screen as long as
            sync is running</li>
            <li>[10358f20] Fixed deprecation warnings</li>
            <li>[05e1066c] Removed unused proguard rule</li>
            <li>[8d87a2a1] Fixed broken navigation after version upgrade</li>
            <li>[cd1d3df0] Fixed foreground service changes in Android 14</li>
            <li>[7939495a] Fixed Saved Articles count only showing unread instead of
            total</li>
            </ul>
            <p>Vitor Henrique (1):</p>
            <ul>
            <li>[67ab5429] Updated Portuguese (Brazil) translation using Weblate</li>
            </ul>
            <p>bowornsin (1):</p>
            <ul>
            <li>[e699f62a] Updated Thai translation using Weblate</li>
            </ul>
            <p>ngocanhtve (1):</p>
            <ul>
            <li>[fa7eb98a] Translated using Weblate (Vietnamese)</li>
            </ul>
            <p>zmni (1):</p>
            <ul>
            <li>[b56e987b] Updated Indonesian translation using Weblate</li>
            </ul>
            """.trimIndent()
        val baseUrl = "https://news.nononsenseapps.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(14, result.size, "Expected items: $result")
    }

    @Test
    fun `cowboyprogrammer transmission`() {
        val html =
            """
            

            <p>Quick post to immortilize the configuration to get transmission-daemon working with a
            wireguard tunnel.</p>
            
            <p>If you don&rsquo;t have a wireguard tunnel, head to <a
                href="https://mullvad.net/en/">https://mullvad.net/en/</a> and get one.</p>
            
            <h2
                id="transmission-config">Transmission config</h2>
            
            <p>First, the transmission config is
            really simple:</p>
            
            <pre><code>#/etc/transmission-daemon/settings.json
            {
              [...]
            
              &quot;bind-address-ipv4&quot;: &quot;X.X.X.X&quot;,
              &quot;bind-address-ipv6&quot;: &quot;xxxx:xxxx:xxxx:xxxx::xxxx&quot;,
              &quot;peer-port&quot;: 24328,
              &quot;rpc-bind-address&quot;: &quot;0.0.0.0&quot;,
            
              [...]
            }
            </code></pre>
            
            
            <p>I also run the daemon using the following service for good measure:</p>
            
            
            <pre><code># /etc/systemd/system/transmission-daemon.service
            [Unit]
            Description=Transmission BitTorrent Daemon Under VPN
            After=network-online.target
            After=wg-quick@wgtorrents.service
            Requires=wg-quick@wgtorrents.service
            
            [Service]
            User=debian-transmission
            ExecStart=/usr/bin/transmission-daemon -f --log-error --bind-address-ipv4 X.X.X.X --bind-address-ipv6 xxxx:xxxx:xxxx:xxxx::xxxx --rpc-bind-address 0.0.0.0
            
            [Install]
            WantedBy=multi-user.target
            
            </code></pre>
            
            
            <h2 id="wireguard-config">Wireguard config</h2>
            
            <p>All the magic happens in the PostUp rule where
                a routing rule is added for any traffic originating from the wireguard IP addresses.</p>
            
            
            <pre><code>#/etc/wireguard/wgtorrents.conf
            [Interface]
            PrivateKey=
            Address=X.X.X.X/32,xxxx:xxxx:xxxx:xxxx::xxxx/128
            # Inhibit default table creation
            Table=off
            # But do create a default route for the specific ip addresses
            PostUp = systemd-resolve -i %i --set-dns=193.138.218.74 --set-domain=~.; ip rule add from X.X.X.X table 42; ip route add default dev %i table 42; ip -6 rule add from xxxx:xxxx:xxxx:xxxx::xxxx table 42
            PostDown = ip rule del from X.X.X.X table 42; ip -6 rule del from xxxx:xxxx:xxxx:xxxx::xxxx table 42
            
            [Peer]
            PersistentKeepalive=25
            PublicKey=m4jnogFbACz7LByjo++8z5+1WV0BuR1T7E1OWA+n8h0=
            Endpoint=se4-wireguard.mullvad.net:51820
            AllowedIPs=0.0.0.0/0,::/0
            </code></pre>
            
            
            <p>Enable it all by doing</p>
            
            
            <pre><code>systemctl enable --now wg-quick@wgtorrents.timer
            systemctl enable --now transmission-daemon.service
            </code></pre>
            """.trimIndent()

        val baseUrl = "https://cowboyprogrammer.org"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(12, result.size, "Expected items: $result")
    }

    @Test
    fun `cowboyprogrammer exhaustive`() {
        val html =
            """
            <p>Just a placeholder so far. Needed a known blog to test a few things with.</p> <h2
                    id="animated-images">Animated images!</h2> <p><img
                    src="https://cowboyprogrammer.org/images/2021/06/animated-webp-supported.webp"
                    alt="Animated Webp image"/></p> <p><img
                    src="https://cowboyprogrammer.org/images/2021/06/rotating_earth.gif" alt="Animated Gif"/>
            </p> <p>And at long last animated in the reader itself!</p> <p><img
                    src="https://cowboyprogrammer.org/images/2021/06/reader_animated.gif"
                    alt="Animated reader"/></p> <h2 id="text-formatting">Text formatting</h2> <p>A <a
                    href="https://gitlab.com/spacecowboy/Feeder/-/merge_requests/318">link</a> to Gitlab.</p>
            <p>Some <code>inline code formatting</code>.</p> <p>And then</p>
            <pre><code>A code block with some lines of code should be scrollable if one very very long line with many sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss </code></pre>
            <p>A table!</p>
            <table>
                <caption><p>Table 1.
                    <p>This table demonstrates the table rendering capabilities of Feeder's Reader view. This
                        caption is by the spec allowed to contain most objects, except other tables. See <a
                                href="https://www.w3.org/TR/2014/REC-html5-20141028/dom.html#flow-content-1">flow
                            content</a>.</caption>
                <thead>
                <tr>
                    <th>Name
                    <th>Number
                    <th>Money
                <tr>
                    <th>First and Last name
                    <th>What number human are you?
                    <th>How much money have you collected?
                <tfoot>
                <tr>
                    <th>No Comment
                    <th>Early!
                    <th>Sad
                        <tbody>
                <tr>
                    <td>Bob
                    <td>66
                    <td>${'$'}3
                <tr>
                    <td>Alice
                    <td>999
                    <td>${'$'}999999
                <tr>
                    <td>:O
                    <td colspan="2">OMG Col span 2
                <tr>
                    <td colspan="3">WHAAAT. Triple span?!
                <tr>
                    <td colspan="0">Firefox special zero span means to the end!
            </table> <p>And this is a table with an image in it</p>
            <table>
                <tbody>
                <tr>
                    <td><img src="https://cowboyprogrammer.org/images/Ardebian_logo_512_0.png"
                             alt="Debian logo"></td>
                </tr>
                <tr>
                    <td> Should be a debian logo above</td>
                </tr>
                </tbody>
            </table> <p>And this is a link with an image inside</p> <p><a
                    href="https://cowboyprogrammer.org/2016/08/zopfli_all_the_things/"> <img
                    src="https://cowboyprogrammer.org/images/2017/10/zopfli_all_the_things_32.png" alt="A meme">
            </a></p> <p>Here is a blockquote with a nested quote in it:</p>
            <blockquote><p>Once upon a time</p>
                <p>A dev coded compose it was written:</p>
                <blockquote><p>@Composable fun FunctionFuns()</p></blockquote>
                <p>And there was code</p></blockquote> <p>Here comes some headers</p> <h1 id="header-1">Header
                1</h1> <h2 id="header-2">Header 2</h2> <h3 id="header-3">Header 3</h3> <h4 id="header-4">Header
                4</h4> <h5 id="header-5">Header 5</h5> <h6 id="header-6">Header 6</h6> <h2 id="lists">Lists</h2>
            <p>Here are some lists</p>
            <ul>
                <li>Bullet</li>
                <li>Point</li>
                <li>List</li>
            </ul> <p>and</p>
            <ol>
                <li>Numbered</li>
                <li>List</li>
                <li>Here</li>
            </ol> <h2 id="videos">Videos</h2> <p>Here&rsquo;s an embedded youtube video</p>
            <iframe width="560" height="315" src="https://www.youtube.com/embed/1OfxlSG6q5Y"
                    title="YouTube video player" frameborder="0"
                    allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture"
                    allowfullscreen></iframe> <p>Here&rsquo;s an HTML5 video</p>
            <video width="320" height="240" controls>
                <source src="https://www.w3schools.com/html/mov_bbb.mp4" type="video/mp4">
                <source src="https://www.w3schools.com/html/mov_bbb.ogg" type="video/ogg">
                Your browser does not support the video tag.
            </video>
            <hr/> <p>Other posts in the <b>Rewriting Feeder in Compose</b> series:</p>
            <ul class="series">
                <li>2021-06-09 &mdash; The biggest update to Feeder so far</li>
            </ul> 
            """.trimIndent()

        val baseUrl = "https://cowboyprogrammer.org"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(1, result.count { it is LinearTable }, "Expected one table in result")
        assertEquals(8, result.filterIsInstance<LinearTable>().first().rowCount, "Expected table with 8 rows")
    }

    @Test
    fun `table with single column is optimized out`() {
        val html =
            """
            <table>
                <tbody>
                    <tr>
                        <td>Single column table</td>
                    </tr>
                    <tr>
                        <td>Second row</td>
                    </tr>
                </tbody>
            </table>
            """.trimIndent()

        val baseUrl = "https://example.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(2, result.size, "Expected two text items: $result")
        assertTrue("Expected all to be linear text items: $result") {
            result.all { it is LinearText }
        }
    }

    @Test
    fun `insane nested table`() {
        // from kill-the-newsletter
        val html =
            """
                  <table class="body"
            style="-ms-text-size-adjust: 100%; border-spacing: 0; border-collapse: collapse; vertical-align: top; margin-bottom: 0; background: #f3f3f3; height: 100%; width: 100%; color: #0a0a0a; font-family: Helvetica,Arial,sans-serif; font-weight: 400; margin: 0; text-align: left; font-size: 16px; line-height: 1.3; mso-table-lspace: 0; mso-table-rspace: 0;">
                    <tbody style="-ms-text-size-adjust: 100%; border: 0 solid transparent;">
                      <tr style="-ms-text-size-adjust: 100%; vertical-align: top; text-align: left;">
                        <td align="center" class="center acym__wysid__template__content" valign="top"
                          style="-ms-text-size-adjust: 100%; word-wrap: break-word; -webkit-hyphens: auto; -moz-hyphens: auto; hyphens: auto; vertical-align: top; color: #0a0a0a; font-family: Helvetica,Arial,sans-serif; font-weight: 400; margin: 0; text-align: left; font-size: 16px; line-height: 1.3; background-color: rgb(239, 239, 239); padding: 40px 0 120px 0; border-collapse: collapse; mso-table-lspace: 0; mso-table-rspace: 0;">
                          <center style="-ms-text-size-adjust: 100%; width: 100%; min-width: 580px;">
                            <table align="center" border="0" cellpadding="0" cellspacing="0"
                              style="-ms-text-size-adjust: 100%; border-spacing: 0; border-collapse: collapse; vertical-align: top; text-align: left; margin-bottom: 0; max-width: 580px; mso-table-lspace: 0; mso-table-rspace: 0;">
                              <tbody style="-ms-text-size-adjust: 100%; border: 0 solid transparent;">
                                <tr style="-ms-text-size-adjust: 100%; vertical-align: top; text-align: left;">
                                  <td class="acym__wysid__row ui-droppable ui-sortable"
                                    style="-ms-text-size-adjust: 100%; word-wrap: break-word; -webkit-hyphens: auto; -moz-hyphens: auto; hyphens: auto; vertical-align: top; color: #0a0a0a; font-family: Helvetica,Arial,sans-serif; font-weight: 400; margin: 0; text-align: left; font-size: 16px; line-height: 1.3; background-color: #fff; min-height: 0px; display: table-cell; border-collapse: collapse; mso-table-lspace: 0; mso-table-rspace: 0;">
                                    <table class="row acym__wysid__row__element" bgcolor="#ffffff" border="0" cellpadding="0"
                                      cellspacing="0"
                                      style="-ms-text-size-adjust: 100%; border-spacing: 0; vertical-align: top; text-align: left; margin-bottom: 0; background-color: #fff; width: 100%; position: relative; padding: 10px; z-index: 100; padding-left: 25px; border-radius: 4px; border-width: 0px; border-style: solid; padding-right: 25px; mso-table-lspace: 0; mso-table-rspace: 0; border-collapse: initial;">
                                      <tbody bgcolor="#ffffff"
                                        style="-ms-text-size-adjust: 100%; border: 0 solid transparent; background-color: transparent;">
                                        <tr style="-ms-text-size-adjust: 100%; vertical-align: top; text-align: left;">
                                          <th class="small-12 medium-12 large-12 columns" height="268.1999969482422"
                                            style="-ms-text-size-adjust: 100%; color: #0a0a0a; font-family: Helvetica,Arial,sans-serif; font-weight: 400; text-align: left; font-size: 16px; line-height: 1.3; margin: 0 auto; width: 580px; height: 268.2px;">
                                            <table class="acym__wysid__column" border="0" cellpadding="0" cellspacing="0"
                                              style="-ms-text-size-adjust: 100%; border-spacing: 0; border-collapse: collapse; vertical-align: top; text-align: left; margin-bottom: 0; width: 100%; min-height: 0px; display: table; mso-table-lspace: 0; mso-table-rspace: 0;">
                                              <tbody class="ui-sortable"
                                                style="-ms-text-size-adjust: 100%; border: 0 solid transparent; min-height: 0px; display: table-row-group;">
                                                <tr class="acym__wysid__column__element ui-draggable"
                                                  style="-ms-text-size-adjust: 100%; vertical-align: top; text-align: left; position: relative; inset: inherit; height: auto;">
                                                  <td class="large-12 acym__wysid__column__element__td"
                                                    style="-ms-text-size-adjust: 100%; word-wrap: break-word; -webkit-hyphens: auto; -moz-hyphens: auto; hyphens: auto; vertical-align: top; color: #0a0a0a; font-family: Helvetica,Arial,sans-serif; font-weight: 400; margin: 0; text-align: left; font-size: 16px; line-height: 1.3; width: 100%; outline: rgb(0, 163, 254) dashed 0px; outline-offset: -1px; mso-table-lspace: 0; mso-table-rspace: 0; border-collapse: collapse;">
                                                    <div class="acym__wysid__tinymce--text mce-content-body" spellcheck="false"
                                                      contenteditable="false" id="mce_12"
                                                      style="-ms-text-size-adjust: 100%; height: 100%; position: relative;">
                                                      <h3
                                                        style="-ms-text-size-adjust: 100%; mso-line-height-rule: exactly; margin: 0; text-align: left; word-wrap: normal; margin-bottom: 10px; font-size: 28px; font-family: Helvetica; font-weight: normal; line-height: inherit; font-style: normal; color: #000000;">
                                                        &nbsp;
                                                      </h3>
                                                      <h3
                                                        style="-ms-text-size-adjust: 100%; mso-line-height-rule: exactly; margin: 0; text-align: left; word-wrap: normal; margin-bottom: 10px; font-size: 28px; font-family: Helvetica; font-weight: normal; line-height: inherit; font-style: normal; color: #000000;">
                                                        Dear E S Znqiiwuyp Sjpv,
                                                      </h3>
                                                      <p
                                                        style="-ms-text-size-adjust: 100%; mso-line-height-rule: exactly; margin: 0; text-align: left; margin-bottom: 0; word-break: break-word; font-family: Helvetica; font-size: 12px; line-height: inherit; font-weight: normal; font-style: normal; color: #000000;">
                                                        <span
                                                          style="-ms-text-size-adjust: 100%; mso-line-height-rule: exactly; color: inherit; font-size: 18px;">You&apos;
                                                          ve subscribed to the OpenSciences.org newsletter. Please confirm this was
                                                          your intention:</span><br style="-ms-text-size-adjust: 100%;"></p>
                                                      <p
                                                        style="-ms-text-size-adjust: 100%; mso-line-height-rule: exactly; margin: 0; text-align: left; margin-bottom: 0; word-break: break-word; font-family: Helvetica; font-size: 12px; line-height: inherit; font-weight: normal; font-style: normal; color: #000000;">
                                                        &nbsp;
                                                      </p>
                                                      <h1
                                                        style="-ms-text-size-adjust: 100%; mso-line-height-rule: exactly; margin: 0; word-wrap: normal; word-break: break-word; font-family: Helvetica; font-size: 34px; line-height: inherit; font-weight: normal; font-style: normal; color: #000000; text-align: left; margin-bottom: 10px;">
                                                        &nbsp;
                                                        <a target="_blank"
                                                          href="https://www.opensciences.org/component/acym/frontusers/confirm?userId=3513&userKey=oqyAhMH5J0BOL4"
                                                          style="-ms-text-size-adjust: 100%; mso-line-height-rule: exactly; font-weight: 400; margin: 0; text-align: left; line-height: 1.3; text-decoration: none; color: #2199e8; font-family: Helvetica;"><span
                                                            class="acym_confirm acym_link"
                                                            style="-ms-text-size-adjust: 100%; color: #0000f1; font-size: 12px; font-weight: 400; font-style: normal; font-family: Helvetica;">Click
                                                            here to confirm your subscription</span></a>&zwj;
                                                        <br style="-ms-text-size-adjust: 100%;">
                                                      </h1>
                                                      <p
                                                        style="-ms-text-size-adjust: 100%; mso-line-height-rule: exactly; margin: 0; margin-bottom: 0; word-break: break-word; font-family: Helvetica; font-size: 12px; line-height: inherit; font-weight: normal; font-style: normal; color: #000000; text-align: left;">
                                                        &nbsp;
                                                        <br style="-ms-text-size-adjust: 100%;">
                                                      </p>
                                                    </div>
                                                  </td>
                                                </tr>
                                              </tbody>
                                            </table>
                                          </th>
                                        </tr>
                                      </tbody>
                                    </table>
                                  </td>
                                </tr>
                              </tbody>
                            </table>
                          </center>
                        </td>
                      </tr>
                    </tbody>
                  </table>
            """.trimIndent()

        // Tables with a single row and column are optimized out

        val baseUrl = "https://kill-the-newsletter.com"

        val result = linearizer.linearize(html, baseUrl).elements

        assertEquals(4, result.size, "Expected text elements: $result")

        assertTrue("Expected all to be linear text items: $result") {
            result.all { it is LinearText }
        }
    }
}

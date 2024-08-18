package com.nononsenseapps.feeder.ui.text

import org.ccil.cowan.tagsoup.HTMLSchema
import org.ccil.cowan.tagsoup.Parser
import org.xml.sax.Attributes
import org.xml.sax.ContentHandler
import org.xml.sax.InputSource
import org.xml.sax.Locator
import org.xml.sax.SAXException
import org.xml.sax.SAXNotRecognizedException
import org.xml.sax.SAXNotSupportedException
import java.io.IOException
import java.io.StringReader
import java.util.Stack

/**
 * Intended primarily to convert HTML into plaintext snippets, useful for previewing content in list.
 */
@Suppress("UNUSED_PARAMETER")
class HtmlToPlainTextConverter : ContentHandler {
    private val parser: Parser = Parser()
    private var builder: StringBuilder? = null
    private val listings = Stack<Listing>()
    private var ignoreCount = 0
    private val ignoredTags = listOf("style", "script")
    private var lastImageAlt: String? = null

    private val isOrderedList: Boolean
        get() = !listings.isEmpty() && listings.peek().ordered

    init {
        try {
            parser.setProperty(Parser.schemaProperty, HTMLSchema())
            parser.contentHandler = this
        } catch (e: SAXNotRecognizedException) {
            throw RuntimeException(e)
        } catch (e: SAXNotSupportedException) {
            throw RuntimeException(e)
        }
    }

    /**
     * Converts HTML into plain text
     */
    fun convert(source: String): String {
        this.builder = StringBuilder()

        try {
            parser.parse(InputSource(StringReader(source)))
        } catch (e: IOException) {
            // We are reading from a string. There should not be IO problems.
            throw RuntimeException(e)
        } catch (e: SAXException) {
            // TagSoup doesn't throw parse exceptions.
            throw RuntimeException(e)
        }

        // Replace non-breaking space (160) with normal space
        return builder!!.toString().replace(160.toChar(), ' ').trim { it <= ' ' }
    }

    override fun setDocumentLocator(locator: Locator) {
    }

    @Throws(SAXException::class)
    override fun startDocument() {
    }

    @Throws(SAXException::class)
    override fun endDocument() {
        // See test mentioning XKCD
        if (builder?.isEmpty() == true) {
            lastImageAlt?.let {
                builder?.append("[$lastImageAlt]")
            }
        }
    }

    @Throws(SAXException::class)
    override fun startPrefixMapping(
        prefix: String,
        uri: String,
    ) {
    }

    @Throws(SAXException::class)
    override fun endPrefixMapping(prefix: String) {
    }

    @Throws(SAXException::class)
    override fun startElement(
        uri: String,
        localName: String,
        qName: String,
        attributes: Attributes,
    ) {
        handleStartTag(localName, attributes)
    }

    private fun handleStartTag(
        tag: String,
        attributes: Attributes,
    ) {
        when {
            tag.equals("br", ignoreCase = true) -> {
                // We don't need to handle this. TagSoup will ensure that there's a </br> for each <br>
                // so we can safely emit the linebreaks when we handle the close tag.
            }
            tag.equals("p", ignoreCase = true) -> ensureSpace(builder)
            tag.equals("div", ignoreCase = true) -> ensureSpace(builder)
            tag.equals("strong", ignoreCase = true) -> strong(builder)
            tag.equals("b", ignoreCase = true) -> strong(builder)
            tag.equals("em", ignoreCase = true) -> emphasize(builder)
            tag.equals("cite", ignoreCase = true) -> emphasize(builder)
            tag.equals("dfn", ignoreCase = true) -> emphasize(builder)
            tag.equals("i", ignoreCase = true) -> emphasize(builder)
            tag.equals("blockquote", ignoreCase = true) -> ensureSpace(builder)
            tag.equals("a", ignoreCase = true) -> startA(builder, attributes)
            tag.length == 2 &&
                Character.toLowerCase(tag[0]) == 'h' &&
                tag[1] >= '1' && tag[1] <= '6' -> ensureSpace(builder)
            tag.equals("ul", ignoreCase = true) -> startUl(builder)
            tag.equals("ol", ignoreCase = true) -> startOl(builder)
            tag.equals("li", ignoreCase = true) -> startLi(builder)
            ignoredTags.contains(tag.lowercase()) -> ignoreCount++
            tag.equals("img", ignoreCase = true) -> startImg(builder, attributes)
        }
    }

    private fun startImg(
        text: StringBuilder?,
        attributes: Attributes,
    ) {
        // Ensure whitespace
        ensureSpace(text)

        lastImageAlt = attributes.getValue("", "alt").orEmpty().ifBlank { "IMG" }
    }

    private fun startOl(text: StringBuilder?) {
        // Start lists with linebreak
        val len = text!!.length
        if (len > 0 && text[len - 1] != '\n') {
            text.append("\n")
        }

        // Remember list type
        listings.push(Listing(true))
    }

    private fun startLi(builder: StringBuilder?) {
        builder!!.append(repeated("  ", listings.size - 1))
        if (isOrderedList) {
            val listing = listings.peek()
            builder.append("").append(listing.number).append(". ")
            listing.number = listing.number + 1
        } else {
            builder.append("* ")
        }
    }

    private fun endLi(text: StringBuilder?) {
        // Add newline
        val len = text!!.length
        if (len > 0 && text[len - 1] != '\n') {
            text.append("\n")
        }
    }

    private fun startUl(text: StringBuilder?) {
        // Start lists with linebreak
        val len = text!!.length
        if (len > 0 && text[len - 1] != '\n') {
            text.append("\n")
        }

        // Remember list type
        listings.push(Listing(false))
    }

    private fun endOl(builder: StringBuilder?) {
        listings.pop()
    }

    private fun endUl(builder: StringBuilder?) {
        listings.pop()
    }

    private fun startA(
        builder: StringBuilder?,
        attributes: Attributes,
    ) {}

    private fun endA(builder: StringBuilder?) {}

    @Throws(SAXException::class)
    override fun endElement(
        uri: String,
        localName: String,
        qName: String,
    ) {
        handleEndTag(localName)
    }

    private fun handleEndTag(tag: String) {
        when {
            tag.equals("br", ignoreCase = true) -> ensureSpace(builder)
            tag.equals("p", ignoreCase = true) -> ensureSpace(builder)
            tag.equals("div", ignoreCase = true) -> ensureSpace(builder)
            tag.equals("strong", ignoreCase = true) -> strong(builder)
            tag.equals("b", ignoreCase = true) -> strong(builder)
            tag.equals("em", ignoreCase = true) -> emphasize(builder)
            tag.equals("cite", ignoreCase = true) -> emphasize(builder)
            tag.equals("dfn", ignoreCase = true) -> emphasize(builder)
            tag.equals("i", ignoreCase = true) -> emphasize(builder)
            tag.equals("blockquote", ignoreCase = true) -> ensureSpace(builder)
            tag.equals("a", ignoreCase = true) -> endA(builder)
            tag.length == 2 &&
                Character.toLowerCase(tag[0]) == 'h' &&
                tag[1] >= '1' && tag[1] <= '6' -> ensureSpace(builder)
            tag.equals("ul", ignoreCase = true) -> endUl(builder)
            tag.equals("ol", ignoreCase = true) -> endOl(builder)
            tag.equals("li", ignoreCase = true) -> endLi(builder)
            ignoredTags.contains(tag.lowercase()) -> ignoreCount--
        }
    }

    private fun emphasize(builder: StringBuilder?) {}

    private fun strong(builder: StringBuilder?) {}

    private fun ensureSpace(text: StringBuilder?) {
        val len = text!!.length
        if (len != 0) {
            val c = text[len - 1]
            // Non-breaking space (160) is not caught by trim or whitespace identification
            if (Character.isWhitespace(c) || c.code == 160) {
                return
            }
            text.append(" ")
        }
    }

    @Throws(SAXException::class)
    override fun characters(
        ch: CharArray,
        start: Int,
        length: Int,
    ) {
        if (ignoreCount > 0) {
            return
        }

        val sb = StringBuilder()

        /*
         * Ignore whitespace that immediately follows other whitespace;
         * newlines count as spaces.
         *
         * TODO handle non-breaking space (character 160)
         */

        for (i in 0 until length) {
            val c = ch[i + start]

            if (c == ' ' || c == '\n') {
                var len = sb.length

                val prev: Char =
                    if (len == 0) {
                        len = builder!!.length

                        if (len == 0) {
                            '\n'
                        } else {
                            builder!![len - 1]
                        }
                    } else {
                        sb[len - 1]
                    }

                if (prev != ' ' && prev != '\n') {
                    sb.append(' ')
                }
            } else {
                sb.append(c)
            }
        }

        builder!!.append(sb)
    }

    @Throws(SAXException::class)
    override fun ignorableWhitespace(
        ch: CharArray,
        start: Int,
        length: Int,
    ) {
    }

    @Throws(SAXException::class)
    override fun processingInstruction(
        target: String,
        data: String,
    ) {
    }

    @Throws(SAXException::class)
    override fun skippedEntity(name: String) {
    }

    private class Bold

    private class Italic

    private class Underline

    private class Big

    private class Small

    private class Monospace

    private class Blockquote

    private class Super

    private class Sub

    class Listing(var ordered: Boolean) {
        var number: Int = 0

        init {
            number = 1
        }
    }

    private open class Bullet

    private class CountBullet : Bullet()

    private class Pre

    private class Code

    private class Font(var mColor: String, var mFace: String?)

    private class Href(var mHref: String?)

    private class Header(var mLevel: Int)
}

fun repeated(
    string: String,
    count: Int,
): String {
    val sb = StringBuilder()

    for (i in 0 until count) {
        sb.append(string)
    }

    return sb.toString()
}

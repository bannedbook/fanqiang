package com.nononsenseapps.feeder.model.opml

import android.util.Log
import android.util.Xml
import com.nononsenseapps.feeder.db.room.Feed
import com.nononsenseapps.feeder.model.OPMLParserHandler
import com.nononsenseapps.feeder.util.Either
import com.nononsenseapps.feeder.util.flatMap
import kotlinx.coroutines.Dispatchers.IO
import kotlinx.coroutines.withContext
import okio.ByteString.Companion.toByteString
import org.xmlpull.v1.XmlPullParser
import org.xmlpull.v1.XmlPullParserException
import java.io.IOException
import java.io.InputStream
import java.net.MalformedURLException
import java.net.URL
import java.util.Arrays
import kotlin.reflect.KProperty

private const val TAG_SETTING = "setting"

private const val TAG_OPML = "opml"

private const val TAG_BODY = "body"

private const val TAG_OUTLINE = "outline"

private const val TAG_SETTINGS = "settings"

private const val ATTR_XMLURL = "xmlUrl"

private const val ATTR_TITLE = "title"

private const val ATTR_TEXT = "text"

private const val ATTR_NOTIFY = "notify"

private const val ATTR_FULL_TEXT_BY_DEFAULT = "fullTextByDefault"

private const val ATTR_FLYM_RETRIEVE_FULL_TEXT = "retrieveFullText"

private const val ATTR_ALTERNATE_ID = "alternateId"

private const val ATTR_IMAGE_URL = "imageUrl"

private const val ATTR_OPEN_ARTICLES_WITH = "openArticlesWith"

private const val TAG_BLOCKED = "blocked"

@Suppress("NAME_SHADOWING")
class OpmlPullParser(private val opmlToDb: OPMLParserHandler) {
    private val feeds: MutableList<Feed> = mutableListOf()
    private val settings: MutableMap<String, String> = mutableMapOf()
    private val blockList: MutableSet<String> = mutableSetOf()
    private val parser: XmlPullParser = Xml.newPullParser()

    suspend fun parseInputStreamWithFallback(inputStream: InputStream): Either<OpmlError, Unit> {
        return Either.catching(
            onCatch = {
                OpmlUnknownError(it)
            },
        ) {
            inputStream.use {
                it.readTheBytes()
            }
        }.flatMap { bytes ->
            val parseResult = parseInputStream(bytes.inputStream())

            if (parseResult.isRight()) {
                parseResult
            } else {
                bytes.toByteString().utf8()
                    .replace(
                        "<outline.*?xmlUrl=\"([^\"]+)\".*?/>".toRegex(),
                        "<outline xmlUrl=\"$1\" type=\"rss\" />",
                    ).byteInputStream().let {
                        parseInputStream(it)
                    }
            }
        }
    }

    private suspend fun parseInputStream(inputStream: InputStream): Either<OpmlError, Unit> =
        Either.catching(
            onCatch = { t ->
                Log.e(LOG_TAG, "OPML Import exploded", t)
                when (t) {
                    is XmlPullParserException -> OpmlParsingError(t)
                    else -> OpmlUnknownError(t)
                }
            },
        ) {
            withContext(IO) {
                inputStream.use { inputStream ->
                    parser.setInput(inputStream, null)
                    parser.nextTag()
                    readOpml()

                    for (feed in feeds) {
                        opmlToDb.saveFeed(feed)
                    }
                    for ((key, value) in settings) {
                        opmlToDb.saveSetting(key = key, value = value)
                    }

                    opmlToDb.saveBlocklistPatterns(blockList)
                }
            }
        }

    @Throws(XmlPullParserException::class, IOException::class)
    private fun readOpml() {
        parser.require(XmlPullParser.START_TAG, null, TAG_OPML)
        while (parser.next() != XmlPullParser.END_TAG) {
            if (parser.eventType != XmlPullParser.START_TAG) {
                continue
            }
            // Starts by looking for the entry tag.
            if (parser.name == TAG_BODY) {
                readBody()
            } else {
                skip()
            }
        }
    }

    @Throws(XmlPullParserException::class, IOException::class)
    private fun readBody() {
        parser.require(XmlPullParser.START_TAG, null, TAG_BODY)
        while (parser.next() != XmlPullParser.END_TAG) {
            if (parser.eventType != XmlPullParser.START_TAG) {
                continue
            }
            if (parser.name == TAG_OUTLINE) {
                readOutline(parser, parentOutlineTag = "")
            } else if (parser.name == TAG_SETTINGS && parser.namespace == OPML_FEEDER_NAMESPACE) {
                readSettings()
            } else {
                skip()
            }
        }
    }

    @Throws(XmlPullParserException::class, IOException::class)
    private fun readSettings() {
        parser.require(XmlPullParser.START_TAG, OPML_FEEDER_NAMESPACE, TAG_SETTINGS)
        while (parser.next() != XmlPullParser.END_TAG) {
            if (parser.eventType != XmlPullParser.START_TAG) {
                continue
            }
            when {
                parser.name == TAG_SETTING && parser.namespace == OPML_FEEDER_NAMESPACE -> {
                    readSetting()
                }

                parser.name == TAG_BLOCKED && parser.namespace == OPML_FEEDER_NAMESPACE -> {
                    readBlocked()
                }

                else -> {
                    skip()
                }
            }
        }
    }

    @Throws(XmlPullParserException::class, IOException::class)
    private fun readSetting() {
        parser.require(XmlPullParser.START_TAG, OPML_FEEDER_NAMESPACE, TAG_SETTING)

        val key by this
        val value by this

        key?.let { key ->
            value?.let { value ->
                settings[key] = unescape(value)
            }
        }

        skip()
    }

    @Throws(XmlPullParserException::class, IOException::class)
    private fun readBlocked() {
        parser.require(XmlPullParser.START_TAG, OPML_FEEDER_NAMESPACE, TAG_BLOCKED)

        val pattern by this

        pattern?.let { pattern ->
            blockList.add(
                unescape(pattern),
            )
        }

        skip()
    }

    @Throws(XmlPullParserException::class, IOException::class)
    private fun readOutline(
        parser: XmlPullParser,
        parentOutlineTag: String,
    ) {
        parser.require(XmlPullParser.START_TAG, null, TAG_OUTLINE)

        val xmlUrl by this
        when {
            xmlUrl != null -> readOutlineAsRss(parser, tag = parentOutlineTag)
            else -> readOutlineAsTag()
        }
    }

    @Throws(XmlPullParserException::class, IOException::class)
    private fun readOutlineAsTag() {
        parser.require(XmlPullParser.START_TAG, null, TAG_OUTLINE)

        val tag =
            unescape(
                parser.getAttributeValue(null, ATTR_TITLE)
                    ?: parser.getAttributeValue(null, ATTR_TEXT)
                    ?: "",
            )

        while (parser.next() != XmlPullParser.END_TAG) {
            if (parser.eventType != XmlPullParser.START_TAG) {
                continue
            }
            if (parser.name == TAG_OUTLINE) {
                readOutline(parser, parentOutlineTag = tag)
            } else {
                skip()
            }
        }
    }

    @Throws(XmlPullParserException::class, IOException::class)
    private fun readOutlineAsRss(
        parser: XmlPullParser,
        tag: String,
    ) {
        parser.require(XmlPullParser.START_TAG, null, TAG_OUTLINE)

        val feedTitle =
            unescape(
                parser.getAttributeValue(null, ATTR_TITLE)
                    ?: parser.getAttributeValue(null, ATTR_TEXT)
                    ?: "",
            )
        try {
            val feedUrl = URL(parser.getAttributeValue(null, ATTR_XMLURL))
            val feed =
                Feed(
                    // Ensure not both are empty string: title will get replaced on sync
                    title = feedTitle.ifBlank { feedUrl.toString() },
                    customTitle = feedTitle,
                    tag = tag,
                    url = feedUrl,
                ).let { feed ->
                    // Copy so default values can be referenced
                    feed.copy(
                        notify =
                            parser.getAttributeValue(OPML_FEEDER_NAMESPACE, ATTR_NOTIFY)
                                ?.toBoolean()
                                ?: feed.notify,
                        fullTextByDefault =
                            (
                                parser.getAttributeValue(
                                    OPML_FEEDER_NAMESPACE,
                                    ATTR_FULL_TEXT_BY_DEFAULT,
                                )
                                    ?.toBoolean()
                                    // Support Flym's value for this
                                    ?: parser.getAttributeValue(null, ATTR_FLYM_RETRIEVE_FULL_TEXT)
                                        ?.toBoolean()
                            ) ?: feed.fullTextByDefault,
                        alternateId =
                            parser.getAttributeValue(OPML_FEEDER_NAMESPACE, ATTR_ALTERNATE_ID)
                                ?.toBoolean()
                                ?: feed.alternateId,
                        openArticlesWith =
                            parser.getAttributeValue(
                                OPML_FEEDER_NAMESPACE,
                                ATTR_OPEN_ARTICLES_WITH,
                            ) ?: feed.openArticlesWith,
                        imageUrl =
                            parser.getAttributeValue(OPML_FEEDER_NAMESPACE, ATTR_IMAGE_URL)
                                ?.let { imageUrl ->
                                    try {
                                        URL(imageUrl)
                                    } catch (e: MalformedURLException) {
                                        Log.e(
                                            LOG_TAG,
                                            "Invalid imageUrl [$imageUrl] on feed [$feedTitle] in OPML",
                                            e,
                                        )
                                        null
                                    }
                                } ?: feed.imageUrl,
                    )
                }

            feeds.add(feed)
        } catch (e: MalformedURLException) {
            // Feed URL is REQUIRED, so don't try to add feeds without valid URLs
            Log.e(LOG_TAG, "Bad url on feed [$feedTitle] in OPML", e)
        }

        skip()
    }

    @Throws(XmlPullParserException::class, IOException::class)
    private fun skip() {
        if (parser.eventType != XmlPullParser.START_TAG) {
            throw IllegalStateException()
        }
        var depth = 1
        while (depth != 0) {
            when (parser.next()) {
                XmlPullParser.END_TAG -> depth--
                XmlPullParser.START_TAG -> depth++
            }
        }
    }

    /**
     * Lets you fetch an un-namespaced attribute like this
     *
     * val attr: String? by this
     *
     * where 'attr' is the case-sensitive attr you are fetching
     */
    operator fun getValue(
        thisRef: Nothing?,
        property: KProperty<*>,
    ): String? {
        return parser.getAttributeValue(null, property.name)
    }
}

private const val LOG_TAG = "FEEDER_OPMLPULL"

sealed class OpmlError {
    abstract val throwable: Throwable?
}

data class OpmlUnknownError(override val throwable: Throwable?) : OpmlError()

data class OpmlParsingError(override val throwable: Throwable) : OpmlError()

fun InputStream.readTheBytes(): ByteArray {
    val len = Int.MAX_VALUE
    require(len >= 0) { "len < 0" }
    var bufs: MutableList<ByteArray>? = null
    var result: ByteArray? = null
    var total = 0
    var remaining = len
    var n: Int
    do {
        val buf = ByteArray(Math.min(remaining, 8192))
        var nread = 0

        // read to EOF which may read more or less than buffer size
        while (read(
                buf,
                nread,
                Math.min(buf.size - nread, remaining),
            ).also { n = it } > 0
        ) {
            nread += n
            remaining -= n
        }
        if (nread > 0) {
            if ((Int.MAX_VALUE - 8) - total < nread) {
                throw OutOfMemoryError("Required array size too large")
            }
            total += nread
            if (result == null) {
                result = buf
            } else {
                if (bufs == null) {
                    bufs = ArrayList()
                    bufs.add(result)
                }
                bufs.add(buf)
            }
        }
        // if the last call to read returned -1 or the number of bytes
        // requested have been read then break
    } while (n >= 0 && remaining > 0)
    if (bufs == null) {
        if (result == null) {
            return ByteArray(0)
        }
        return if (result.size == total) result else Arrays.copyOf(result, total)
    }
    result = ByteArray(total)
    var offset = 0
    remaining = total
    for (b in bufs) {
        val count = Math.min(b.size, remaining)
        System.arraycopy(b, 0, result, offset, count)
        offset += count
        remaining -= count
    }
    return result
}

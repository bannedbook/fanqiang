package com.nononsenseapps.feeder.model.opml

import android.util.Log
import com.nononsenseapps.feeder.db.room.Feed
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.FileOutputStream
import java.io.IOException
import java.io.OutputStream

suspend fun writeFile(
    path: String,
    settings: Map<String, String>,
    blockedPatterns: List<String>,
    tags: Iterable<String>,
    feedsWithTag: suspend (String) -> Iterable<Feed>,
) {
    withContext(Dispatchers.IO) {
        writeOutputStream(
            os = FileOutputStream(path),
            settings = settings,
            blockedPatterns = blockedPatterns,
            tags = tags,
            feedsWithTag = feedsWithTag,
        )
    }
}

suspend fun writeOutputStream(
    os: OutputStream,
    settings: Map<String, String>,
    blockedPatterns: List<String>,
    tags: Iterable<String>,
    feedsWithTag: suspend (String) -> Iterable<Feed>,
) = withContext(Dispatchers.IO) {
    try {
        os.bufferedWriter().use { bw ->
            bw.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
            bw.write(
                opml {
                    head {
                        title { +"Feeder" }
                    }
                    body {
                        tags.forEach { tag ->
                            if (tag.isEmpty()) {
                                feedsWithTag(tag).forEach { feed ->
                                    feed.toOutline()
                                }
                            } else {
                                outline(title = escape(tag)) {
                                    feedsWithTag(tag).forEach { feed ->
                                        feed.toOutline()
                                    }
                                }
                            }
                        }
                        if (settings.isNotEmpty() || blockedPatterns.isNotEmpty()) {
                            feederSettings {
                                settings.forEach { (prefKey, prefValue) ->
                                    feederSetting {
                                        key = prefKey
                                        value = escape(prefValue)
                                    }
                                }
                                blockedPatterns.forEach { blockedPattern ->
                                    feederBlocked {
                                        pattern = escape(blockedPattern)
                                    }
                                }
                            }
                        }
                    }
                }.toString(),
            )
        }
    } catch (e: IOException) {
        Log.e(LOG_TAG, "Failed to write stream", e)
    }
}

/**

 * @param s string to escape
 * *
 * @return String with xml stuff escaped
 */
internal fun escape(s: String): String {
    return s.replace("&", "&amp;")
        .replace("\"", "&quot;")
        .replace("'", "&apos;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
}

/**

 * @param s string to unescape
 * *
 * @return String with xml stuff unescaped
 */
internal fun unescape(s: String): String {
    return s.replace("&quot;", "\"")
        .replace("&apos;", "'")
        .replace("&lt;", "<")
        .replace("&gt;", ">")
        .replace("&amp;", "&")
}

// OPML DSL

suspend fun opml(init: suspend Opml.() -> Unit): Opml {
    val opml = Opml()
    opml.init()
    return opml
}

interface Element {
    fun render(
        builder: StringBuilder,
        indent: String,
    )
}

class TextElement(val text: String) : Element {
    override fun render(
        builder: StringBuilder,
        indent: String,
    ) {
        builder.append("$indent$text\n")
    }
}

abstract class Tag(val name: String) : Element {
    val children = arrayListOf<Element>()
    val attributes = linkedMapOf<String, String>()

    protected suspend fun <T : Element> initTag(
        tag: T,
        init: suspend T.() -> Unit,
    ): T {
        tag.init()
        children.add(tag)
        return tag
    }

    override fun render(
        builder: StringBuilder,
        indent: String,
    ) {
        builder.append("$indent<$name${renderAttributes()}")
        if (children.isEmpty()) {
            builder.append("/>\n")
        } else {
            builder.append(">\n")
            for (c in children) {
                c.render(builder, "$indent  ")
            }
            builder.append("$indent</$name>\n")
        }
    }

    private fun renderAttributes(): String {
        val builder = StringBuilder()
        for (a in attributes.keys) {
            builder.append(" $a=\"${attributes[a]}\"")
        }
        return builder.toString()
    }

    override fun toString(): String {
        val builder = StringBuilder()
        render(builder, "")
        return builder.toString()
    }
}

abstract class TagWithText(name: String) : Tag(name) {
    operator fun String.unaryPlus() {
        children.add(TextElement(this))
    }
}

class Opml : TagWithText("opml") {
    init {
        attributes["version"] = "1.1"
        attributes["xmlns:feeder"] = OPML_FEEDER_NAMESPACE
    }

    suspend fun head(init: suspend Head.() -> Unit) = initTag(Head(), init)

    suspend fun body(init: suspend Body.() -> Unit) = initTag(Body(), init)
}

class Head : TagWithText("head") {
    suspend fun title(init: suspend Title.() -> Unit) = initTag(Title(), init)
}

class Title : TagWithText("title")

abstract class BodyTag(name: String) : TagWithText(name) {
    suspend fun feederSettings(init: suspend FeederSettings.() -> Unit) {
        initTag(FeederSettings(), init)
    }

    suspend fun outline(
        title: String,
        text: String = title,
        type: String? = null,
        xmlUrl: String? = null,
        init: suspend Outline.() -> Unit,
    ) {
        val o = initTag(Outline(), init)
        o.title = title
        o.text = text
        if (type != null) {
            o.type = type
        }
        if (xmlUrl != null) {
            o.xmlUrl = xmlUrl
        }
    }

    suspend fun Feed.toOutline() =
        outline(
            title = escape(displayTitle),
            type = "rss",
            xmlUrl = escape(url.toString()),
        ) {
            val feed = this@toOutline
            notify = feed.notify
            feed.imageUrl?.let { imageUrl = escape(it.toString()) }
            fullTextByDefault = feed.fullTextByDefault
            openArticlesWith = feed.openArticlesWith
            alternateId = feed.alternateId
        }
}

class Body : BodyTag("body")

class FeederSettings : BodyTag("feeder:settings") {
    suspend fun feederSetting(init: suspend FeederSetting.() -> Unit) {
        initTag(FeederSetting(), init)
    }

    suspend fun feederBlocked(init: suspend FeederBlocked.() -> Unit) {
        initTag(FeederBlocked(), init)
    }
}

class FeederBlocked : BodyTag("feeder:blocked") {
    var pattern: String by attributes
}

class FeederSetting : Tag("feeder:setting") {
    var key: String by attributes
    var value: String by attributes
}

class Outline : BodyTag("outline") {
    var title: String by attributes
    var text: String by attributes
    var type: String by attributes
    var xmlUrl: String by attributes

    var notify: Boolean
        get() = attributes["feeder:notify"]!!.toBoolean()
        set(value) {
            attributes["feeder:notify"] = value.toString()
        }
    var imageUrl: String
        get() = attributes["feeder:imageUrl"]!!
        set(value) {
            attributes["feeder:imageUrl"] = value
        }
    var fullTextByDefault: Boolean
        get() = attributes["feeder:fullTextByDefault"]!!.toBoolean()
        set(value) {
            attributes["feeder:fullTextByDefault"] = value.toString()
        }
    var openArticlesWith: String
        get() = attributes["feeder:openArticlesWith"]!!
        set(value) {
            attributes["feeder:openArticlesWith"] = value
        }
    var alternateId: Boolean
        get() = attributes["feeder:alternateId"]!!.toBoolean()
        set(value) {
            attributes["feeder:alternateId"] = value.toString()
        }
}

const val OPML_FEEDER_NAMESPACE = "https://nononsenseapps.com/feeder"
private const val LOG_TAG = "FEEDER_OPMLWRITER"

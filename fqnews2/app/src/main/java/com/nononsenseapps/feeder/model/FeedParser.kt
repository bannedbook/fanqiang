package com.nononsenseapps.feeder.model

import android.os.Parcelable
import android.util.Log
import androidx.annotation.VisibleForTesting
import com.nononsenseapps.feeder.model.gofeed.FeederGoItem
import com.nononsenseapps.feeder.model.gofeed.GoEnclosure
import com.nononsenseapps.feeder.model.gofeed.GoFeed
import com.nononsenseapps.feeder.model.gofeed.GoFeedAdapter
import com.nononsenseapps.feeder.model.gofeed.GoPerson
import com.nononsenseapps.feeder.util.Either
import com.nononsenseapps.feeder.util.flatMap
import com.nononsenseapps.feeder.util.relativeLinkIntoAbsolute
import com.nononsenseapps.feeder.util.relativeLinkIntoAbsoluteOrThrow
import com.nononsenseapps.feeder.util.sloppyLinkToStrictURLOrNull
import kotlinx.coroutines.Dispatchers.IO
import kotlinx.coroutines.withContext
import kotlinx.parcelize.IgnoredOnParcel
import kotlinx.parcelize.Parcelize
import okhttp3.CacheControl
import okhttp3.Credentials
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.Response
import okhttp3.ResponseBody
import org.jsoup.Jsoup
import org.jsoup.nodes.Document
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.instance
import java.io.IOException
import java.lang.NullPointerException
import java.net.MalformedURLException
import java.net.URL
import java.net.URLDecoder
import java.util.Locale

private const val YOUTUBE_CHANNEL_ID_ATTR = "data-channel-external-id"

class FeedParser(override val di: DI) : DIAware {
    private val client: OkHttpClient by instance()
    private val goFeedAdapter = GoFeedAdapter()

    /**
     * Parses all relevant information from a main site so duplicate calls aren't needed
     */
    suspend fun getSiteMetaData(url: URL): Either<FeedParserError, SiteMetaData> {
        return curl(url)
            .flatMap { html ->
                getSiteMetaDataInHtml(url, html)
            }
    }

    @VisibleForTesting
    internal fun getSiteMetaDataInHtml(
        url: URL,
        html: String,
    ): Either<FeedParserError, SiteMetaData> {
        if (!html.contains("<head>", ignoreCase = true)) {
            // Probably a a feed URL and not a page
            return Either.Left(NotHTML(url = url.toString()))
        }

        return Either.catching(
            onCatch = { t ->
                MetaDataParseError(url = url.toString(), throwable = t).also {
                    Log.w(LOG_TAG, "Error when fetching site metadata", t)
                }
            },
        ) {
            SiteMetaData(
                url = url,
                alternateFeedLinks = getAlternateFeedLinksInHtml(html, baseUrl = url),
                feedImage = getFeedIconInHtml(html, baseUrl = url),
            )
        }
    }

    @VisibleForTesting
    internal fun getFeedIconInHtml(
        html: String,
        baseUrl: URL? = null,
    ): String? {
        val doc =
            html.byteInputStream().use {
                Jsoup.parse(it, "UTF-8", baseUrl?.toString() ?: "")
            }

        return (
            doc.getElementsByAttributeValue("rel", "apple-touch-icon") +
                doc.getElementsByAttributeValue("rel", "icon") +
                doc.getElementsByAttributeValue("rel", "shortcut icon")
        )
            .filter { it.hasAttr("href") }
            .firstNotNullOfOrNull { e ->
                when {
                    baseUrl != null ->
                        relativeLinkIntoAbsolute(
                            base = baseUrl,
                            link = e.attr("href"),
                        )

                    else -> sloppyLinkToStrictURLOrNull(e.attr("href"))?.toString()
                }
            }
    }

    /**
     * Returns all alternate links in the HTML/XML document pointing to feeds.
     */
    private fun getAlternateFeedLinksInHtml(
        html: String,
        baseUrl: URL? = null,
    ): List<AlternateLink> {
        val doc =
            html.byteInputStream().use {
                Jsoup.parse(it, "UTF-8", "")
            }

        val feeds =
            doc.getElementsByAttributeValue("rel", "alternate")
                ?.filter { it.hasAttr("href") && it.hasAttr("type") }
                ?.filter {
                    val t = it.attr("type").lowercase(Locale.getDefault())
                    when {
                        t.contains("application/atom") -> true
                        t.contains("application/rss") -> true
                        // Youtube for example has alternate links with application/json+oembed type.
                        t == "application/json" -> true
                        else -> false
                    }
                }
                ?.filter {
                    val l = it.attr("href").lowercase(Locale.getDefault())
                    try {
                        if (baseUrl != null) {
                            relativeLinkIntoAbsoluteOrThrow(base = baseUrl, link = l)
                        } else {
                            URL(l)
                        }
                        true
                    } catch (_: MalformedURLException) {
                        false
                    }
                }
                ?.mapNotNull { e ->
                    when {
                        baseUrl != null -> {
                            try {
                                AlternateLink(
                                    type = e.attr("type"),
                                    link =
                                        relativeLinkIntoAbsoluteOrThrow(
                                            base = baseUrl,
                                            link = e.attr("href"),
                                        ),
                                )
                            } catch (e: Exception) {
                                null
                            }
                        }

                        else ->
                            sloppyLinkToStrictURLOrNull(e.attr("href"))?.let { l ->
                                AlternateLink(
                                    type = e.attr("type"),
                                    link = l,
                                )
                            }
                    }
                } ?: emptyList()

        return when {
            feeds.isNotEmpty() -> feeds
            baseUrl?.host == "www.youtube.com" || baseUrl?.host == "youtube.com" ->
                findFeedLinksForYoutube(
                    doc,
                )

            else -> emptyList()
        }
    }

    private fun findFeedLinksForYoutube(doc: Document): List<AlternateLink> {
        val channelId: String? =
            doc.body()?.getElementsByAttribute(YOUTUBE_CHANNEL_ID_ATTR)
                ?.firstOrNull()
                ?.attr(YOUTUBE_CHANNEL_ID_ATTR)

        return when (channelId) {
            null -> emptyList()
            else ->
                listOf(
                    AlternateLink(
                        type = "atom",
                        link = URL("https://www.youtube.com/feeds/videos.xml?channel_id=$channelId"),
                    ),
                )
        }
    }

    /**
     * @throws IOException if request fails due to network issue for example
     */
    private suspend fun curl(url: URL) = client.curl(url)

    suspend fun parseFeedUrl(url: URL): Either<FeedParserError, ParsedFeed> {
        return client.curlAndOnResponse(url) {
            println("FeedParser client.proxy:" + client.proxy.toString())
            parseFeedResponse(it)
        }
            .map {
                // Preserve original URL to maintain authentication data and/or tokens in query params
                // but this is also done inside parse from the request URL
                it.copy(feed_url = url.toString())
            }
    }

    internal fun parseFeedResponse(response: Response): Either<FeedParserError, ParsedFeed> {
        return response.body?.use {
            // OkHttp string method handles BOM and Content-Type header in request
            parseFeedResponse(
                response.request.url.toUrl(),
                it,
            )
        } ?: Either.Left(NoBody(url = response.request.url.toString()))
    }

    private fun parseFeedBytes(
        url: URL,
        body: ByteArray,
    ): ParsedFeed? {
        return goFeedAdapter.parseBody(body)?.asFeed(url)
    }

    /**
     * Takes body as bytes to handle encoding correctly
     */
    fun parseFeedResponse(
        url: URL,
        responseBody: ResponseBody,
    ): Either<FeedParserError, ParsedFeed> {
        val primaryType = responseBody.contentType()?.type
        val subType = responseBody.contentType()?.subtype ?: ""
        return when {
            primaryType == null || primaryType == "text" || subType.contains("json") || subType.contains("xml") ->
                Either.catching(
                    onCatch = { t ->
                        RSSParseError(url = url.toString(), throwable = t)
                    },
                ) {
                    responseBody.byteStream().use { bs ->
                        parseFeedBytes(url, bs.readBytes())
                            ?: throw NullPointerException("Parsed feed is null")
                    }
                }

            else -> return Either.Left(
                UnsupportedContentType(
                    url = url.toString(),
                    mimeType = responseBody.contentType().toString(),
                ),
            )
        }
    }

    /**
     * Takes body as bytes to handle encoding correctly
     */
    @VisibleForTesting
    internal fun parseFeedResponse(
        url: URL,
        body: String,
    ): Either<FeedParserError, ParsedFeed> {
        return Either.catching(
            onCatch = { t ->
                RSSParseError(url = url.toString(), throwable = t)
            },
        ) {
            parseFeedBytes(url, body.toByteArray())
                ?: throw NullPointerException("Parsed feed is null")
        }
    }

    companion object {
        private const val LOG_TAG = "FEEDER_FEEDPARSER"
    }
}

private fun GoFeed.asFeed(url: URL): ParsedFeed =
    ParsedFeed(
        title = title,
        home_page_url = link?.let { relativeLinkIntoAbsolute(url, it) },
        // Keep original URL to maintain authentication data and/or tokens in query params
        feed_url = url.toString(),
        description = description,
        user_comment = "",
        next_url = "",
        icon = image?.url?.let { relativeLinkIntoAbsolute(url, it) },
        favicon = null,
        author = author?.asParsedAuthor(),
        expired = null,
        items = items?.mapNotNull { it?.let { FeederGoItem(it, author, url).asParsedArticle() } },
    )

private fun FeederGoItem.asParsedArticle() =
    ParsedArticle(
        id = guid,
        url = link,
        external_url = null,
        title = title,
        content_html = content,
        content_text = plainContent,
        summary = snippet,
        image = thumbnail,
        date_published = published,
        date_modified = updated,
        author = author?.asParsedAuthor(),
        tags = categories,
        attachments = enclosures?.map { it.asParsedEnclosure() },
    )

private fun GoEnclosure.asParsedEnclosure() =
    ParsedEnclosure(
        url = url,
        title = null,
        mime_type = type,
        size_in_bytes = length?.toLongOrNull(),
        duration_in_seconds = null,
    )

private fun GoPerson.asParsedAuthor() =
    ParsedAuthor(
        name = name,
        url = null,
        avatar = null,
    )

suspend fun OkHttpClient.getResponse(
    url: URL,
    forceNetwork: Boolean = false,
): Response {
    val request =
        Request.Builder()
            .url(url)
            .run {
                if (forceNetwork) {
                    cacheControl(
                        // Force network will make conditional requests for servers which support them
                        CacheControl.FORCE_NETWORK,
                    )
                } else {
                    this
                }
            }
            .build()

    @Suppress("BlockingMethodInNonBlockingContext")
    val clientToUse =
        if (url.userInfo?.isNotBlank() == true) {
            val parts = url.userInfo.split(':')
            val user = parts.first()
            val pass =
                if (parts.size > 1) {
                    parts[1]
                } else {
                    ""
                }
            val decodedUser = URLDecoder.decode(user, "UTF-8")
            val decodedPass = URLDecoder.decode(pass, "UTF-8")
            val credentials = Credentials.basic(decodedUser, decodedPass)
            newBuilder()
                .authenticator { _, response ->
                    when {
                        response.request.header("Authorization") != null -> {
                            null
                        }

                        else -> {
                            response.request.newBuilder()
                                .header("Authorization", credentials)
                                .build()
                        }
                    }
                }
                .proxyAuthenticator { _, response ->
                    when {
                        response.request.header("Proxy-Authorization") != null -> {
                            null
                        }

                        else -> {
                            response.request.newBuilder()
                                .header("Proxy-Authorization", credentials)
                                .build()
                        }
                    }
                }
                .build()
        } else {
            this
        }

    return withContext(IO) {
        clientToUse.newCall(request).execute()
    }
}

suspend fun OkHttpClient.curl(url: URL): Either<FeedParserError, String> {
    return curlAndOnResponse(url) {
        val contentType = it.body?.contentType()
        when (contentType?.type) {
            "text" -> {
                when (contentType.subtype) {
                    "plain", "html" -> {
                        it.body?.let { body ->
                            Either.catching(
                                onCatch = { throwable ->
                                    FetchError(
                                        throwable = throwable,
                                        url = url.toString(),
                                    )
                                },
                            ) {
                                body.string()
                            }
                        } ?: Either.Left(
                            NoBody(
                                url = url.toString(),
                            ),
                        )
                    }

                    else ->
                        Either.Left(
                            UnsupportedContentType(
                                url = url.toString(),
                                mimeType = contentType.toString(),
                            ),
                        )
                }
            }

            else ->
                Either.Left(
                    UnsupportedContentType(
                        url = url.toString(),
                        mimeType = contentType.toString(),
                    ),
                )
        }
    }
}

suspend fun <T> OkHttpClient.curlAndOnResponse(
    url: URL,
    block: (suspend (Response) -> Either<FeedParserError, T>),
): Either<FeedParserError, T> {
    return Either.catching(
        onCatch = { t ->
            FetchError(url = url.toString(), throwable = t)
        },
    ) {
        getResponse(url)
    }.flatMap { response ->
        if (response.isSuccessful) {
            response.use {
                block(it)
            }
        } else {
            Either.Left(
                HttpError(
                    url = url.toString(),
                    code = response.code,
                    retryAfterSeconds = response.retryAfterSeconds,
                    message = response.message,
                ),
            )
        }
    }
}

@Parcelize
sealed class FeedParserError : Parcelable {
    abstract val url: String
    abstract val description: String
    abstract val throwable: Throwable?
}

@Parcelize
data object NotInitializedYet : FeedParserError() {
    @IgnoredOnParcel
    override val url: String = ""

    @IgnoredOnParcel
    override val description: String = ""

    @IgnoredOnParcel
    override val throwable: Throwable? = null
}

@Parcelize
data class FetchError(
    override val url: String,
    override val throwable: Throwable?,
    override val description: String = throwable?.message ?: "",
) : FeedParserError()

@Parcelize
data class NotHTML(
    override val url: String,
    override val description: String = "",
    override val throwable: Throwable? = null,
) : FeedParserError()

@Parcelize
data class MetaDataParseError(
    override val url: String,
    override val throwable: Throwable?,
    override val description: String = throwable?.message ?: "",
) : FeedParserError()

@Parcelize
data class RSSParseError(
    override val throwable: Throwable?,
    override val url: String,
    override val description: String = throwable?.message ?: "",
) : FeedParserError()

@Parcelize
data class JsonFeedParseError(
    override val throwable: Throwable?,
    override val url: String,
    override val description: String = throwable?.message ?: "",
) : FeedParserError()

@Parcelize
data class NoAlternateFeeds(
    override val url: String,
    override val description: String = "",
    override val throwable: Throwable? = null,
) : FeedParserError()

@Parcelize
data class HttpError(
    override val url: String,
    val code: Int,
    val message: String,
    val retryAfterSeconds: Long?,
    override val description: String = "$code: $message",
    override val throwable: Throwable? = null,
) : FeedParserError()

@Parcelize
data class UnsupportedContentType(
    override val url: String,
    val mimeType: String,
    override val description: String = mimeType,
    override val throwable: Throwable? = null,
) : FeedParserError()

@Parcelize
data class NoBody(
    override val url: String,
    override val description: String = "",
    override val throwable: Throwable? = null,
) : FeedParserError()

@Parcelize
data class NoUrl(
    override val description: String = "",
    override val url: String = "",
    override val throwable: Throwable? = null,
) : FeedParserError()

@Parcelize
data class FullTextDecodingFailure(
    override val url: String,
    override val throwable: Throwable?,
    override val description: String = throwable?.message ?: "",
) : FeedParserError()

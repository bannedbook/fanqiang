package com.nononsenseapps.jsonfeed

import com.squareup.moshi.JsonAdapter
import com.squareup.moshi.Moshi
import com.squareup.moshi.kotlin.reflect.KotlinJsonAdapterFactory
import io.nekohasekai.sagernet.database.DataStore
import okhttp3.Cache
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.ResponseBody
import java.io.File
import java.io.IOException
import java.net.InetSocketAddress
import java.net.Proxy
import java.util.concurrent.TimeUnit

fun cachingHttpClient(
    cacheDirectory: File? = null,
    cacheSize: Long = 10L * 1024L * 1024L,
    trustAllCerts: Boolean = true,
    connectTimeoutSecs: Long = 30L,
    readTimeoutSecs: Long = 30L,
    block: (OkHttpClient.Builder.() -> Unit)? = null,
): OkHttpClient {
    // 配置代理信息
    val proxy = Proxy(Proxy.Type.HTTP, InetSocketAddress("127.0.0.1", DataStore.mixedPort))

    val builder: OkHttpClient.Builder = OkHttpClient.Builder().proxy(proxy)

    if (cacheDirectory != null) {
        builder.cache(Cache(cacheDirectory, cacheSize))
    }

    builder
        .connectTimeout(connectTimeoutSecs, TimeUnit.SECONDS)
        .readTimeout(readTimeoutSecs, TimeUnit.SECONDS)
        .followRedirects(true)

    if (trustAllCerts) {
        builder.trustAllCerts()
    }

    block?.let {
        builder.block()
    }

    return builder.build()
}

fun feedAdapter(): JsonAdapter<Feed> = Moshi.Builder().addLast(KotlinJsonAdapterFactory()).build().adapter(Feed::class.java)

/**
 * A parser for JSONFeeds. CacheDirectory and CacheSize are only relevant if feeds are downloaded. They are not used
 * for parsing JSON directly.
 */
class JsonFeedParser(
    private val httpClient: OkHttpClient,
    private val jsonFeedAdapter: JsonAdapter<Feed>,
) {
    constructor(
        cacheDirectory: File? = null,
        cacheSize: Long = 10L * 1024L * 1024L,
        trustAllCerts: Boolean = true,
        connectTimeoutSecs: Long = 5L,
        readTimeoutSecs: Long = 5L,
    ) : this(
        cachingHttpClient(
            cacheDirectory = cacheDirectory,
            cacheSize = cacheSize,
            trustAllCerts = trustAllCerts,
            connectTimeoutSecs = connectTimeoutSecs,
            readTimeoutSecs = readTimeoutSecs,
        ),
        feedAdapter(),
    )

    /**
     * Download a JSONFeed and parse it
     */
    fun parseUrl(url: String): Feed {
        val request: Request
        try {
            request =
                Request.Builder()
                    .url(url)
                    .build()
        } catch (error: Throwable) {
            throw IllegalArgumentException(
                "Bad URL. Perhaps it is missing an http:// prefix?",
                error,
            )
        }

        val response = httpClient.newCall(request).execute()

        if (!response.isSuccessful) {
            throw IOException("Failed to download feed: $response")
        }

        return response.body?.let { body ->
            val contentType = body.contentType()
            when (contentType?.type) {
                "application", "text" -> {
                    when {
                        contentType.subtype.contains("json") -> {
                            parseJson(body)
                        }

                        else -> {
                            throw IOException("Incorrect subtype: ${contentType.type}/${contentType.subtype}")
                        }
                    }
                }

                else -> {
                    throw IOException("Incorrect type: ${contentType?.type}/${contentType?.subtype}")
                }
            }
        } ?: throw IOException("Failed to parse feed: body was NULL")
    }

    /**
     * Parse a JSONFeed
     */
    fun parseJson(responseBody: ResponseBody): Feed = parseJson(responseBody.string())

    /**
     * Parse a JSONFeed
     */
    fun parseJson(json: String): Feed =
        jsonFeedAdapter.fromJson(json)
            ?: throw IOException("Failed to parse JSONFeed")
}

data class Feed(
    val version: String? = "https://jsonfeed.org/version/1",
    val title: String?,
    val home_page_url: String? = null,
    val feed_url: String? = null,
    val description: String? = null,
    val user_comment: String? = null,
    val next_url: String? = null,
    val icon: String? = null,
    val favicon: String? = null,
    val author: Author? = null,
    val expired: Boolean? = null,
    val hubs: List<Hub>? = null,
    val items: List<Item>?,
)

data class Author(
    val name: String? = null,
    val url: String? = null,
    val avatar: String? = null,
)

data class Item(
    val id: String?,
    val url: String? = null,
    val external_url: String? = null,
    val title: String? = null,
    val content_html: String? = null,
    val content_text: String? = null,
    val summary: String? = null,
    val image: String? = null,
    val banner_image: String? = null,
    val date_published: String? = null,
    val date_modified: String? = null,
    val author: Author? = null,
    val tags: List<String>? = null,
    val attachments: List<Attachment>? = null,
)

data class Attachment(
    val url: String?,
    val mime_type: String? = null,
    val title: String? = null,
    val size_in_bytes: Long? = null,
    val duration_in_seconds: Long? = null,
)

data class Hub(
    val type: String?,
    val url: String?,
)

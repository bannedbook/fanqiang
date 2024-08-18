package com.nononsenseapps.feeder.model

import android.util.Log
import com.nononsenseapps.feeder.archmodel.Repository
import com.nononsenseapps.feeder.blob.blobFile
import com.nononsenseapps.feeder.blob.blobFullFile
import com.nononsenseapps.feeder.blob.blobOutputStream
import com.nononsenseapps.feeder.db.room.Feed
import com.nononsenseapps.feeder.db.room.FeedItem
import com.nononsenseapps.feeder.db.room.ID_UNSET
import com.nononsenseapps.feeder.sync.SyncRestClient
import com.nononsenseapps.feeder.util.Either
import com.nononsenseapps.feeder.util.FilePathProvider
import com.nononsenseapps.feeder.util.flatMap
import com.nononsenseapps.feeder.util.left
import com.nononsenseapps.feeder.util.logDebug
import com.nononsenseapps.feeder.util.right
import com.nononsenseapps.feeder.util.sloppyLinkToStrictURLNoThrows
import kotlinx.coroutines.asCoroutineDispatcher
import kotlinx.coroutines.joinAll
import kotlinx.coroutines.launch
import kotlinx.coroutines.supervisorScope
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlinx.coroutines.withContext
import okhttp3.OkHttpClient
import okhttp3.Response
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.instance
import java.io.IOException
import java.net.URL
import java.time.Instant
import java.time.temporal.ChronoUnit
import java.util.concurrent.Executors
import kotlin.math.max
import kotlin.system.measureTimeMillis

val singleThreadedSync = Executors.newSingleThreadExecutor().asCoroutineDispatcher()
val syncMutex = Mutex()

/**
 * WARNING. DO NOT CHANGE THE DISPATCHER WITHIN THE LOGIC!
 */
class RssLocalSync(override val di: DI) : DIAware {
    private val repository: Repository by instance()
    private val syncClient: SyncRestClient by instance()
    private val feedParser: FeedParser by instance()
    private val okHttpClient: OkHttpClient by instance()
    private val filePathProvider: FilePathProvider by instance()

    suspend fun syncFeeds(
        feedId: Long = ID_UNSET,
        feedTag: String = "",
        forceNetwork: Boolean = false,
        minFeedAgeMinutes: Int = 5,
    ): Boolean {
        logDebug(LOG_TAG, "${Thread.currentThread().name}: Taking sync mutex")
        return syncMutex.withLock {
            withContext(singleThreadedSync) {
                try {
                    repository.setSyncWorkerRunning(true)

                    syncFeeds(
                        feedId = feedId,
                        feedTag = feedTag,
                        maxFeedItemCount = repository.maximumCountPerFeed.value,
                        forceNetwork = forceNetwork,
                        minFeedAgeMinutes = minFeedAgeMinutes,
                    )
                } finally {
                    repository.setSyncWorkerRunning(false)
                }
            }
        }
    }

    internal suspend fun syncFeeds(
        feedId: Long = ID_UNSET,
        feedTag: String = "",
        maxFeedItemCount: Int = 100,
        forceNetwork: Boolean = false,
        minFeedAgeMinutes: Int = 5,
    ): Boolean {
        var result = false
        var needFullTextSync = false
        // Let all new items share download time
        val downloadTime = Instant.now()
        val time =
            measureTimeMillis {
                try {
                    supervisorScope {
                        val staleTime: Long =
                            if (forceNetwork) {
                                // Under no circumstances should we spam servers more than once per minute intentionally
                                Instant.now().minus(1, ChronoUnit.MINUTES).toEpochMilli()
                            } else {
                                Instant.now().minus(minFeedAgeMinutes.toLong().coerceAtLeast(1), ChronoUnit.MINUTES)
                                    .toEpochMilli()
                            }
                        // Fetch sync stuff first - this is fast
                        try {
                            syncClient.getFeeds()
                            syncClient.getRead()
                            syncClient.getDevices()
                            syncClient.sendUpdatedFeeds()
                            syncClient.markAsRead()
                        } catch (e: Exception) {
                            Log.e(LOG_TAG, "error with syncClient: ${e.message}", e)
                        }

                        val feedsToFetch =
                            feedsToSync(feedId, feedTag, staleTime = staleTime)

                        logDebug(LOG_TAG, "Syncing ${feedsToFetch.size} feeds")

                        needFullTextSync = feedsToFetch.any { it.fullTextByDefault }

                        // These coroutines are concurrent but on a single thread,
                        // so they are not truly parallel by design to ensure
                        // IO waits are minimized
                        val concurrentJobs = 2
                        val jobs =
                            (0 until concurrentJobs)
                                .map { jobIndex ->
                                    launch {
                                        feedsToFetch.asSequence()
                                            .filterIndexed { index, _ ->
                                                index % concurrentJobs == jobIndex
                                            }
                                            .forEach { feed ->
                                                handleFeed(
                                                    feed = feed,
                                                    maxFeedItemCount = maxFeedItemCount,
                                                    forceNetwork = forceNetwork,
                                                    downloadTime = downloadTime,
                                                )
                                            }
                                    }
                                }

                        jobs.joinAll()

                        try {
                            repository.applyRemoteReadMarks()
                        } catch (e: Exception) {
                            Log.e(LOG_TAG, "Error on final apply", e)
                        }

                        result = true
                    }
                } catch (e: Throwable) {
                    Log.e(LOG_TAG, "Outer error", e)
                } finally {
                    if (needFullTextSync) {
                        scheduleFullTextParse(
                            di = di,
                        )
                    }
                }
            }
        logDebug(LOG_TAG, "Completed in $time ms")
        return result
    }

    private suspend fun handleFeed(
        feed: Feed,
        maxFeedItemCount: Int,
        forceNetwork: Boolean,
        downloadTime: Instant,
    ) {
        try {
            // Want unique sync times.
            val syncTime = Instant.now()
            repository.setCurrentlySyncingOn(
                feedId = feed.id,
                syncing = true,
                lastSync = syncTime,
            )
            syncFeed(
                feedId = feed.id,
                maxFeedItemCount = maxFeedItemCount,
                forceNetwork = forceNetwork,
                downloadTime = downloadTime,
            ).onLeft { feedParserError ->
                Log.e(
                    LOG_TAG,
                    "Failed to sync ${feed.displayTitle}: ${feed.url} because:\n${feedParserError.description}",
                )

                // Handle retry-after
                if (feedParserError is HttpError) {
                    feedParserError.retryAfterSeconds?.let { retryAfterSeconds ->
                        // Feeds can share retry after if they are on the same server
                        repository.setRetryAfterForFeedsWithBaseUrl(
                            host = feed.url.host,
                            retryAfter = Instant.now().plusSeconds(retryAfterSeconds),
                        )
                    }
                }
            }.onRight {
                repository.setBlockStatusForNewInFeed(feedId = feed.id, blockTime = syncTime)
            }
        } catch (e: Throwable) {
            Log.e(
                LOG_TAG,
                "Failed to sync ${feed.displayTitle}: ${feed.url}",
                e,
            )
        } finally {
            repository.setCurrentlySyncingOn(
                feedId = feed.id,
                syncing = false,
            )
        }
    }

    private suspend fun syncFeed(
        feedId: Long,
        maxFeedItemCount: Int,
        forceNetwork: Boolean = false,
        downloadTime: Instant,
    ): Either<FeedParserError, Unit> {
        // Load it again to ensure we get the latest value for retry-after since this can be shared across feeds
        // if they share the same server
        val feedSql =
            repository.syncLoadFeed(feedId, retryAfter = Instant.now())
                ?: run {
                    // not loaded due to retry-after
                    Log.i(LOG_TAG, "Skipping feed $feedId due to retry-after changing mid sync")
                    return Unit.right()
                }

        val url = feedSql.url

        // Belts and suspenders
        if (feedSql.retryAfter > Instant.now()) {
            Log.i(LOG_TAG, "Skipping ${feedSql.displayTitle} due to retry-after. Earliest retry: ${feedSql.retryAfter}")
            return Unit.right()
        }

        logDebug(LOG_TAG, "Fetching ${feedSql.displayTitle}")

        return Either.catching(
            onCatch = { t ->
                FetchError(url = url.toString(), throwable = t)
            },
        ) {
            println("RssLocalSync client.proxy:" + okHttpClient.proxy.toString())
            okHttpClient.getResponse(feedSql.url, forceNetwork = forceNetwork)
        }.flatMap { response ->
            response.use {
                if (response.isSuccessful) {
                    response.body?.let { responseBody ->
                        feedParser.parseFeedResponse(
                            response.request.url.toUrl(),
                            responseBody,
                        )
                    } ?: NoBody(url = url.toString()).left()
                } else {
                    response.retryAfterSeconds?.let { retryAfterSeconds ->
                        logDebug(LOG_TAG, "$url, Retry after: $retryAfterSeconds")
                    }
                    HttpError(
                        url = url.toString(),
                        code = response.code,
                        retryAfterSeconds = response.retryAfterSeconds,
                        message = response.message,
                    ).left()
                }
            }
        }.map {
            // Double check that icon is not base64
            when {
                it.icon?.startsWith("data") == true -> it.copy(icon = null)
                else -> it
            }
        }.onLeft {
            // Nothing was parsed, nothing to do. lastSync time has already been updated
        }.flatMap { feed ->
            Either.catching(
                onCatch = { t ->
                    FetchError(url = url.toString(), throwable = t)
                },
            ) {
                val items = feed.items
                val uniqueIdCount = items?.map { it.id }?.toSet()?.size
                // This can only detect between items present in one feed. See NIXOS
                val isNotUniqueIds = uniqueIdCount != items?.size

                val alreadyReadGuids = repository.getGuidsWhichAreSyncedAsReadInFeed(feedSql)

                val feedItemSqls =
                    items
                        ?.map {
                            val guid =
                                when (isNotUniqueIds || feedSql.alternateId) {
                                    true -> it.alternateId
                                    else -> it.id ?: it.alternateId
                                }

                            it to guid
                        }
                        ?.reversed()
                        ?.mapNotNull { (item, guid) ->
                            // Always attempt to load existing items using both id schemes
                            // Id is rewritten to preferred on update
                            val feedItemSql =
                                repository.loadFeedItem(
                                    guid = item.alternateId,
                                    feedId = feedSql.id,
                                ) ?: repository.loadFeedItem(
                                    guid = item.id ?: item.alternateId,
                                    feedId = feedSql.id,
                                ) ?: FeedItem(firstSyncedTime = downloadTime)

                            // If new item, see if duplicates exist
                            if (feedItemSql.id != ID_UNSET || !feedSql.skipDuplicates || !repository.duplicateStoryExists(id = feedItemSql.id, title = item.title ?: "", link = item.url)) {
                                feedItemSql.updateFromParsedEntry(item, guid, feed)
                                feedItemSql.feedId = feedSql.id

                                if (feedItemSql.guid in alreadyReadGuids) {
                                    // TODO get read time from sync service
                                    feedItemSql.readTime = feedItemSql.readTime ?: Instant.now()
                                    feedItemSql.notified = true
                                }

                                feedItemSql to (item.content_html ?: item.content_text ?: "")
                            } else {
                                Log.i(LOG_TAG, "Duplicate story ignored: [${item.title}] [${feed.title}]")
                                null
                            }
                        } ?: emptyList()

                repository.upsertFeedItems(feedItemSqls) { feedItem, text ->
                    filePathProvider.articleDir.mkdirs()
                    blobOutputStream(feedItem.id, filePathProvider.articleDir).bufferedWriter()
                        .use {
                            it.write(text)
                        }
                }
                // Try to look for image if not done before
                if (feedSql.imageUrl == null && feedSql.siteFetched == Instant.EPOCH) {
                    val siteUrl =
                        try {
                            URL(feed.home_page_url)
                        } catch (e: Throwable) {
                            logDebug(LOG_TAG, "Bad site url: ${feed.home_page_url}", e)
                            null
                        }
                    if (siteUrl != null) {
                        feedSql.siteFetched = Instant.now()
                        feedParser.getSiteMetaData(siteUrl)
                            .onRight { metadata ->
                                try {
                                    feedSql.imageUrl = URL(metadata.feedImage)
                                } catch (e: Throwable) {
                                    logDebug(LOG_TAG, "Bad feedImage url: ${feed.home_page_url}", e)
                                }
                            }
                    }
                }

                // Update feed last so lastsync is only set after all items have been handled
                // for the rare case that the job is cancelled prematurely
                //feedSql.title = feed.title ?: feedSql.title  //jww

                // Not changing feed url because I don't want to override auth or token params
                // See https://gitlab.com/spacecowboy/Feeder/-/issues/390
                //        feedSql.url = feed.feed_url?.let { sloppyLinkToStrictURLNoThrows(it) } ?: feedSql.url

                // Important to keep image if there is one in case of null
                // the image is fetched when adding a feed by fetching the main site and looking
                // for favicons - but only when first added and icon missing from feed.
                feedSql.imageUrl = feed.icon?.let { sloppyLinkToStrictURLNoThrows(it) }
                    ?: feedSql.imageUrl

                repository.upsertFeed(feedSql)

                // Finally, prune database of old items
                val ids =
                    repository.getItemsToBeCleanedFromFeed(
                        feedId = feedSql.id,
                        keepCount = max(maxFeedItemCount, items?.size ?: 0),
                    )

                for (id in ids) {
                    blobFile(itemId = id, filesDir = filePathProvider.articleDir).let { file ->
                        try {
                            if (file.isFile) {
                                file.delete()
                            }
                        } catch (e: IOException) {
                            Log.e(LOG_TAG, "Failed to delete $file", e)
                        }
                        Unit
                    }
                    blobFullFile(
                        itemId = id,
                        filesDir = filePathProvider.fullArticleDir,
                    ).let { file ->
                        try {
                            if (file.isFile) {
                                file.delete()
                            }
                        } catch (e: IOException) {
                            Log.e(LOG_TAG, "Failed to delete $file", e)
                        }
                        Unit
                    }
                }

                repository.deleteFeedItems(ids)
                repository.deleteStaleRemoteReadMarks()

                logDebug(LOG_TAG, "Fetched ${feedSql.displayTitle}")
            }
        }
    }

    internal suspend fun feedsToSync(
        feedId: Long,
        tag: String,
        staleTime: Long = -1L,
    ): List<Feed> {
        return when {
            feedId > 0 -> {
                val feed =
                    if (staleTime > 0) {
                        repository.syncLoadFeedIfStale(
                            feedId,
                            staleTime = staleTime,
                            retryAfter = Instant.now(),
                        )
                    } else {
                        // Used internally too
                        repository.syncLoadFeed(feedId, retryAfter = Instant.now())
                    }
                if (feed != null) {
                    listOf(feed)
                } else {
                    emptyList()
                }
            }

            tag.isNotEmpty() ->
                if (staleTime > 0) {
                    repository.syncLoadFeedsIfStale(
                        tag = tag,
                        staleTime = staleTime,
                        retryAfter = Instant.now(),
                    )
                } else {
                    repository.syncLoadFeeds(tag, retryAfter = Instant.now())
                }

            else -> if (staleTime > 0) repository.syncLoadFeedsIfStale(staleTime, retryAfter = Instant.now()) else repository.syncLoadFeeds(retryAfter = Instant.now())
        }
    }

    companion object {
        private const val LOG_TAG = "FEEDER_RssLocalSync"
    }
}

/**
 * Remember that text or title literally can mean injection problems if the contain % or similar,
 * so do NOT use them literally
 */
private val ParsedArticle.alternateId: String
    get() = "$id|${content_text.hashCode()}|${title.hashCode()}"

internal val Response.retryAfterSeconds: Long?
    get() =
        headers("retry-after").maxOfOrNull { retryAfter ->
            // Fallback to 1 hour if response is incorrect
            retryAfter.toLongOrNull() ?: 3600L
        }

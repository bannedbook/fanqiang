package com.nononsenseapps.feeder.archmodel

import androidx.paging.Pager
import androidx.paging.PagingConfig
import androidx.paging.PagingData
import androidx.paging.map
import androidx.sqlite.db.SimpleSQLiteQuery
import com.nononsenseapps.feeder.db.room.AppDatabase
import com.nononsenseapps.feeder.db.room.BlocklistDao
import com.nononsenseapps.feeder.db.room.FeedItem
import com.nononsenseapps.feeder.db.room.FeedItemCursor
import com.nononsenseapps.feeder.db.room.FeedItemDao
import com.nononsenseapps.feeder.db.room.FeedItemDao.Companion.FEED_ITEM_LIST_SORT_ORDER_ASC
import com.nononsenseapps.feeder.db.room.FeedItemDao.Companion.FEED_ITEM_LIST_SORT_ORDER_DESC
import com.nononsenseapps.feeder.db.room.FeedItemIdWithLink
import com.nononsenseapps.feeder.db.room.FeedItemWithFeed
import com.nononsenseapps.feeder.db.room.ID_SAVED_ARTICLES
import com.nononsenseapps.feeder.db.room.ID_UNSET
import com.nononsenseapps.feeder.db.room.upsertFeedItems
import com.nononsenseapps.feeder.model.PREVIEW_COLUMNS
import com.nononsenseapps.feeder.model.PreviewItem
import com.nononsenseapps.feeder.ui.compose.feed.FeedListItem
import com.nononsenseapps.feeder.ui.compose.feedarticle.FeedListFilter
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.withContext
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.instance
import java.net.URL
import java.time.Instant
import java.time.LocalDate
import java.time.LocalDateTime
import java.time.ZoneId
import java.time.format.DateTimeFormatter
import java.time.format.FormatStyle
import java.util.Locale

class FeedItemStore(override val di: DI) : DIAware {
    private val dao: FeedItemDao by instance()
    private val blocklistDao: BlocklistDao by instance()
    private val appDatabase: AppDatabase by instance()

    suspend fun setBlockStatusForNewInFeed(
        feedId: Long,
        blockTime: Instant,
    ) {
        blocklistDao.setItemBlockStatusForNewInFeed(feedId, blockTime)
    }

    fun getFeedItemCountRaw(
        feedId: Long,
        tag: String,
        minReadTime: Instant,
        filter: FeedListFilter,
    ): Flow<Int> {
        val queryString = StringBuilder()
        val args = mutableListOf<Any?>()

        queryString.apply {
            append("SELECT count(*) FROM feed_items\n")
            append("LEFT JOIN feeds ON feed_items.feed_id = feeds.id\n")
            append("WHERE\n")

            rawQueryFilter(filter, args, minReadTime, feedId, tag)
        }

        return dao.getPreviewsCount(SimpleSQLiteQuery(queryString.toString(), args.toTypedArray()))
    }

    fun getPagedFeedItemsRaw(
        feedId: Long,
        tag: String,
        minReadTime: Instant,
        newestFirst: Boolean,
        filter: FeedListFilter,
    ): Flow<PagingData<FeedListItem>> =
        Pager(
            config =
                PagingConfig(
                    pageSize = 10,
                    initialLoadSize = 100,
                    prefetchDistance = 100,
                    jumpThreshold = 100,
                ),
        ) {
            val queryString = StringBuilder()
            val args = mutableListOf<Any?>()

            queryString.apply {
                append("SELECT $PREVIEW_COLUMNS FROM feed_items\n")
                append("LEFT JOIN feeds ON feed_items.feed_id = feeds.id\n")
                append("WHERE\n")

                rawQueryFilter(filter, args, minReadTime, feedId, tag)

                when (newestFirst) {
                    true -> append("ORDER BY $FEED_ITEM_LIST_SORT_ORDER_DESC\n")
                    else -> append("ORDER BY $FEED_ITEM_LIST_SORT_ORDER_ASC\n")
                }
            }

            dao.pagingPreviews(SimpleSQLiteQuery(queryString.toString(), args.toTypedArray()))
        }
            .flow.map { pagingData ->
                pagingData
                    .map { it.toFeedListItem() }
            }

    private fun StringBuilder.rawQueryFilter(
        filter: FeedListFilter,
        args: MutableList<Any?>,
        minReadTime: Instant,
        feedId: Long,
        tag: String,
    ) {
        val onlySavedArticles = feedId == ID_SAVED_ARTICLES

        // Always blocklist
        append("block_time is null\n")
        // List filter
        if (!onlySavedArticles) {
            append("AND (\n")
            if (filter.unread && !filter.recentlyRead && !filter.read) {
                append("read_time is null\n")
            } else {
                append("(read_time is null or read_time >= ?)\n").also {
                    args.add(
                        when {
                            filter.read -> Instant.EPOCH
                            else -> minReadTime
                        }.toEpochMilli(),
                    )
                }
            }
            if (filter.saved) {
                append("OR (bookmarked = 1)\n")
            }
            append(")\n")
        }

        when {
            onlySavedArticles -> append("AND bookmarked = 1\n")
            feedId > ID_UNSET -> append("AND feed_id IS ?\n").also { args.add(feedId) }
            tag.isNotEmpty() -> append("AND tag IS ?\n").also { args.add(tag) }
        }
    }

    suspend fun markAsReadRaw(
        feedId: Long,
        tag: String,
        filter: FeedListFilter,
        minReadTime: Instant,
        descending: Boolean,
        cursor: FeedItemCursor,
    ) {
        val queryString = StringBuilder()
        val args = mutableListOf<Any?>()

        queryString.apply {
            append("UPDATE feed_items SET read_time = coalesce(read_time, ?), notified = 1\n").also {
                args.add(Instant.now().toEpochMilli())
            }
            append("WHERE id IN (\n")

            append("SELECT feed_items.id FROM feed_items\n")
            append("LEFT JOIN feeds ON feed_items.feed_id = feeds.id\n")
            append("WHERE\n")
            rawQueryFilter(filter, args, minReadTime, feedId, tag)
            // this version of sqlite doesn't seem to support tuple comparisons
            append("and (\n")

            append(
                when (descending) {
                    true -> "primary_sort_time < ?\n"
                    else -> "primary_sort_time > ?\n"
                },
            ).also { args.add(cursor.primarySortTime.toEpochMilli()) }
            append(
                when (descending) {
                    true -> "or primary_sort_time = ? and pub_date < ?\n"
                    else -> "or primary_sort_time = ? and pub_date > ?\n"
                },
            ).also {
                args.add(cursor.primarySortTime.toEpochMilli())
                args.add(cursor.pubDate)
            }

            append(
                when (descending) {
                    true -> "or primary_sort_time = ? and pub_date = ? and feed_items.id < ?\n"
                    else -> "or primary_sort_time = ? and pub_date = ? and feed_items.id > ?\n"
                },
            ).also {
                args.add(cursor.primarySortTime.toEpochMilli())
                args.add(cursor.pubDate)
                args.add(cursor.id)
            }

            append(")\n")

            // where id in (
            append(")\n")
        }

        // Room does not support writable raw queries
        withContext(Dispatchers.IO) {
            appDatabase.openHelper.writableDatabase.execSQL(
                queryString.toString(),
                args.toTypedArray(),
            )
            appDatabase.invalidationTracker.refreshVersionsAsync()
        }
    }

    suspend fun markAsNotified(itemIds: List<Long>) {
        dao.markAsNotified(itemIds)
    }

    suspend fun markAsRead(itemIds: List<Long>) {
        dao.markAsRead(itemIds)
    }

    suspend fun markAsReadAndNotified(
        itemId: Long,
        readTime: Instant = Instant.now(),
    ) {
        dao.markAsReadAndNotified(
            id = itemId,
            readTime = readTime.coerceAtLeast(Instant.EPOCH),
        )
    }

    suspend fun markAsReadAndNotifiedAndOverwriteReadTime(
        itemId: Long,
        readTime: Instant,
    ) {
        dao.markAsReadAndNotifiedAndOverwriteReadTime(
            id = itemId,
            readTime = readTime.coerceAtLeast(Instant.EPOCH),
        )
    }

    suspend fun markAsUnread(itemId: Long) {
        dao.markAsUnread(itemId)
    }

    suspend fun setBookmarked(
        itemId: Long,
        bookmarked: Boolean,
    ) {
        dao.setBookmarked(itemId, bookmarked)
    }

    suspend fun getFullTextByDefault(itemId: Long): Boolean {
        return dao.getFullTextByDefault(itemId) ?: false
    }

    fun getFeedItem(itemId: Long): Flow<FeedItemWithFeed?> {
        return dao.getFeedItemFlow(itemId)
    }

    suspend fun getFeedItemId(
        feedUrl: URL,
        articleGuid: String,
    ): Long? {
        return dao.getItemWith(feedUrl = feedUrl, articleGuid = articleGuid)
    }

    suspend fun getLink(itemId: Long): String? {
        return dao.getLink(itemId)
    }

    suspend fun getArticleOpener(itemId: Long): String? {
        return dao.getOpenArticleWith(itemId)
    }

    suspend fun markAllAsReadInFeed(feedId: Long) {
        dao.markAllAsRead(feedId)
    }

    suspend fun markAllAsReadInTag(tag: String) {
        dao.markAllAsRead(tag)
    }

    suspend fun markAllAsRead() {
        dao.markAllAsRead()
    }

    fun getFeedsItemsWithDefaultFullTextNeedingDownload(): Flow<List<FeedItemIdWithLink>> = dao.getFeedsItemsWithDefaultFullTextNeedingDownload()

    suspend fun markAsFullTextDownloaded(feedItemId: Long) = dao.markAsFullTextDownloaded(feedItemId)

    fun getFeedItemsNeedingNotifying(): Flow<List<Long>> {
        return dao.getFeedItemsNeedingNotifying()
    }

    suspend fun loadFeedItem(
        guid: String,
        feedId: Long,
    ): FeedItem? = dao.loadFeedItem(guid = guid, feedId = feedId)

    suspend fun upsertFeedItems(
        itemsWithText: List<Pair<FeedItem, String>>,
        block: suspend (FeedItem, String) -> Unit,
    ) {
        dao.upsertFeedItems(itemsWithText = itemsWithText, block = block)
    }

    suspend fun getItemsToBeCleanedFromFeed(
        feedId: Long,
        keepCount: Int,
    ) = dao.getItemsToBeCleanedFromFeed(feedId = feedId, keepCount = keepCount)

    suspend fun deleteFeedItems(ids: List<Long>) {
        dao.deleteFeedItems(ids)
    }

    suspend fun updateWordCountFull(
        id: Long,
        wordCount: Int,
    ) {
        dao.updateWordCountFull(id, wordCount)
    }

    suspend fun duplicateStoryExists(
        id: Long,
        title: String,
        link: String?,
    ): Boolean {
        return dao.duplicationExists(
            id = id,
            plainTitle = title,
            link = link,
        )
    }

    suspend fun getArticle(id: Long): FeedItemWithFeed? {
        return dao.getFeedItem(id)
    }
}

val mediumDateTimeFormat: DateTimeFormatter =
    DateTimeFormatter.ofLocalizedDate(FormatStyle.MEDIUM).withLocale(Locale.getDefault())

val shortTimeFormat: DateTimeFormatter =
    DateTimeFormatter.ofLocalizedTime(FormatStyle.SHORT).withLocale(Locale.getDefault())

private fun PreviewItem.toFeedListItem() =
    FeedListItem(
        id = id,
        title = plainTitle,
        snippet = plainSnippet,
        feedTitle = feedDisplayTitle,
        unread = readTime == null,
        pubDate = pubDate?.withZoneSameInstant(ZoneId.systemDefault())?.toLocalDateTime()?.formatDynamically() ?: "",
        image = image,
        link = link,
        bookmarked = bookmarked,
        feedImageUrl = feedImageUrl,
        rawPubDate = pubDate,
        primarySortTime = primarySortTime,
        wordCount = bestWordCount,
    )

private fun LocalDateTime.formatDynamically(): String {
    val today = LocalDate.now().atStartOfDay()
    return when {
        this >= today -> format(shortTimeFormat)
        else -> format(mediumDateTimeFormat)
    }
}

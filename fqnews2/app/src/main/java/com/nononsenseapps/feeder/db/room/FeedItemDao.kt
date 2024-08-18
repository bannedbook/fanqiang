package com.nononsenseapps.feeder.db.room

import android.database.Cursor
import androidx.paging.PagingSource
import androidx.room.Dao
import androidx.room.Delete
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.RawQuery
import androidx.room.Update
import androidx.sqlite.db.SupportSQLiteQuery
import com.nononsenseapps.feeder.db.COL_ID
import com.nononsenseapps.feeder.db.COL_PLAINSNIPPET
import com.nononsenseapps.feeder.db.COL_TITLE
import com.nononsenseapps.feeder.db.COL_URL
import com.nononsenseapps.feeder.db.FEEDS_TABLE_NAME
import com.nononsenseapps.feeder.model.PREVIEW_COLUMNS
import com.nononsenseapps.feeder.model.PreviewItem
import com.nononsenseapps.feeder.util.DoNotUseInProd
import kotlinx.coroutines.flow.Flow
import java.net.URL
import java.time.Instant

@Dao
interface FeedItemDao {
    @Insert(onConflict = OnConflictStrategy.IGNORE)
    suspend fun insertFeedItem(item: FeedItem): Long

    @Insert(onConflict = OnConflictStrategy.IGNORE)
    suspend fun insertFeedItems(items: List<FeedItem>): List<Long>

    @Update
    suspend fun updateFeedItem(item: FeedItem): Int

    @Update
    suspend fun updateFeedItems(items: List<FeedItem>): Int

    @Delete
    suspend fun deleteFeedItem(item: FeedItem): Int

    @Query(
        """
        DELETE FROM feed_items WHERE id IN (:ids)
        """,
    )
    suspend fun deleteFeedItems(ids: List<Long>): Int

    @Query(
        """
            update feed_items
            set word_count_full = :wordCount
            where id = :id
        """,
    )
    suspend fun updateWordCountFull(
        id: Long,
        wordCount: Int,
    )

    @Query(
        """
        SELECT id FROM feed_items
        WHERE feed_id IS :feedId AND bookmarked = 0
        ORDER BY primary_sort_time DESC, pub_date DESC
        LIMIT -1 OFFSET :keepCount
        """,
    )
    suspend fun getItemsToBeCleanedFromFeed(
        feedId: Long,
        keepCount: Int,
    ): List<Long>

    @Query("SELECT * FROM feed_items WHERE guid IS :guid AND feed_id IS :feedId")
    suspend fun loadFeedItem(
        guid: String,
        feedId: Long?,
    ): FeedItem?

    @Query("SELECT * FROM feed_items WHERE id IS :id")
    suspend fun loadFeedItem(id: Long): FeedItem?

    @Query(
        """
        SELECT $FEED_ITEM_COLUMNS_WITH_FEED
        FROM feed_items
        LEFT JOIN feeds ON feed_items.feed_id = feeds.id
        WHERE feed_items.id IS :id
        """,
    )
    suspend fun loadFeedItemWithFeed(id: Long): FeedItemWithFeed?

    @Query(
        """
        SELECT $FEEDS_TABLE_NAME.$COL_URL
        FROM feed_items
        LEFT JOIN feeds ON feed_items.feed_id = feeds.id
        WHERE feed_items.id IS :id
        """,
    )
    suspend fun loadFeedUrlOfFeedItem(id: Long): URL?

    @DoNotUseInProd
    @Query(
        """
        SELECT *
        FROM feed_items
        WHERE feed_items.feed_id = :feedId
        ORDER BY primary_sort_time DESC, pub_date DESC
        """,
    )
    suspend fun loadFeedItemsInFeedDesc(feedId: Long): List<FeedItem>

    @Query(
        """
        SELECT $COL_ID as id, $COL_TITLE as title, $COL_PLAINSNIPPET as text
        FROM feed_items
        WHERE block_time is null
        ORDER BY primary_sort_time DESC, pub_date DESC
        """,
    )
    fun loadFeedItemsForContentProvider(): Cursor

    @Query(
        """
        SELECT $COL_ID as id, $COL_TITLE as title, $COL_PLAINSNIPPET as text
        FROM feed_items
        WHERE feed_items.feed_id = :feedId
          AND block_time is null
        ORDER BY primary_sort_time DESC, pub_date DESC
        """,
    )
    fun loadFeedItemsInFeedForContentProvider(feedId: Long): Cursor

    @Query(
        """
        SELECT $FEED_ITEM_COLUMNS_WITH_FEED
        FROM feed_items
        LEFT JOIN feeds ON feed_items.feed_id = feeds.id
        WHERE feed_items.id IS :id
        """,
    )
    fun getFeedItemFlow(id: Long): Flow<FeedItemWithFeed?>

    @Query(
        """
        SELECT $FEED_ITEM_COLUMNS_WITH_FEED
        FROM feed_items
        LEFT JOIN feeds ON feed_items.feed_id = feeds.id
        WHERE feed_items.id IS :id
        """,
    )
    suspend fun getFeedItem(id: Long): FeedItemWithFeed?

    @RawQuery(observedEntities = [FeedItem::class])
    fun getPreviewsCount(query: SupportSQLiteQuery): Flow<Int>

    @RawQuery(observedEntities = [FeedItem::class])
    fun pagingPreviews(query: SupportSQLiteQuery): PagingSource<Int, PreviewItem>

    @Query(
        """
        SELECT $PREVIEW_COLUMNS
        FROM feed_items
        LEFT JOIN feeds ON feed_items.feed_id = feeds.id
        WHERE feed_id IS :feedId
          AND (read_time is null or read_time >= :minReadTime)
          AND bookmarked in (1, :bookmarked)
          AND block_time is null
        ORDER BY $FEED_ITEM_LIST_SORT_ORDER_DESC
        """,
    )
    fun pagingUnreadPreviewsDesc(
        feedId: Long,
        minReadTime: Instant,
        bookmarked: Boolean,
    ): PagingSource<Int, PreviewItem>

    @Query(
        """
        SELECT $PREVIEW_COLUMNS
        FROM feed_items
        LEFT JOIN feeds ON feed_items.feed_id = feeds.id
        WHERE tag IS :tag
          AND (read_time is null or read_time >= :minReadTime)
          AND bookmarked in (1, :bookmarked)
          AND block_time is null
        ORDER BY $FEED_ITEM_LIST_SORT_ORDER_DESC
        """,
    )
    fun pagingUnreadPreviewsDesc(
        tag: String,
        minReadTime: Instant,
        bookmarked: Boolean,
    ): PagingSource<Int, PreviewItem>

    @Query(
        """
        SELECT $PREVIEW_COLUMNS
        FROM feed_items
        LEFT JOIN feeds ON feed_items.feed_id = feeds.id
        WHERE (read_time is null or read_time >= :minReadTime)
          AND bookmarked in (1, :bookmarked)
          AND block_time is null
        ORDER BY $FEED_ITEM_LIST_SORT_ORDER_DESC
        """,
    )
    fun pagingUnreadPreviewsDesc(
        minReadTime: Instant,
        bookmarked: Boolean,
    ): PagingSource<Int, PreviewItem>

    @Query(
        """
        SELECT $PREVIEW_COLUMNS
        FROM feed_items
        LEFT JOIN feeds ON feed_items.feed_id = feeds.id
        WHERE feed_id IS :feedId
          AND (read_time is null or read_time >= :minReadTime)
          AND bookmarked in (1, :bookmarked)
          AND block_time is null
        ORDER BY $FEED_ITEM_LIST_SORT_ORDER_ASC
        """,
    )
    fun pagingUnreadPreviewsAsc(
        feedId: Long,
        minReadTime: Instant,
        bookmarked: Boolean,
    ): PagingSource<Int, PreviewItem>

    @Query(
        """
        SELECT $PREVIEW_COLUMNS
        FROM feed_items
        LEFT JOIN feeds ON feed_items.feed_id = feeds.id
        WHERE tag IS :tag 
          AND (read_time is null or read_time >= :minReadTime)
          AND bookmarked in (1, :bookmarked)
          AND block_time is null
        ORDER BY $FEED_ITEM_LIST_SORT_ORDER_ASC
        """,
    )
    fun pagingUnreadPreviewsAsc(
        tag: String,
        minReadTime: Instant,
        bookmarked: Boolean,
    ): PagingSource<Int, PreviewItem>

    @Query(
        """
        SELECT $PREVIEW_COLUMNS
        FROM feed_items
        LEFT JOIN feeds ON feed_items.feed_id = feeds.id
        WHERE (read_time is null or read_time >= :minReadTime)
          AND bookmarked in (1, :bookmarked)
          AND block_time is null
        ORDER BY $FEED_ITEM_LIST_SORT_ORDER_ASC
        """,
    )
    fun pagingUnreadPreviewsAsc(
        minReadTime: Instant,
        bookmarked: Boolean,
    ): PagingSource<Int, PreviewItem>

    @Query(
        """
        SELECT $FEED_ITEM_COLUMNS_WITH_FEED
        FROM feed_items
        LEFT JOIN feeds ON feed_items.feed_id = feeds.id
        WHERE feed_id IN (:feedIds) AND notified IS 0 AND read_time is null
          AND block_time is null
        ORDER BY $FEED_ITEM_LIST_SORT_ORDER_DESC
        LIMIT 20
        """,
    )
    suspend fun loadItemsToNotify(feedIds: List<Long>): List<FeedItemWithFeed>

    @Query("UPDATE feed_items SET read_time = coalesce(read_time, :readTime), notified = 1")
    suspend fun markAllAsRead(readTime: Instant = Instant.now())

    @Query("UPDATE feed_items SET read_time = coalesce(read_time, :readTime), notified = 1 WHERE feed_id IS :feedId")
    suspend fun markAllAsRead(
        feedId: Long?,
        readTime: Instant = Instant.now(),
    )

    @Query(
        """
        UPDATE feed_items
        SET read_time = coalesce(read_time, :readTime), notified = 1
        WHERE id IN (
          SELECT feed_items.id
          FROM feed_items
          LEFT JOIN feeds ON feed_items.feed_id = feeds.id
          WHERE tag IS :tag
        )""",
    )
    suspend fun markAllAsRead(
        tag: String,
        readTime: Instant = Instant.now(),
    )

    @Query("UPDATE feed_items SET read_time = coalesce(read_time, :readTime), notified = 1 WHERE id IS :id")
    suspend fun markAsRead(
        id: Long,
        readTime: Instant = Instant.now(),
    )

    @Query("UPDATE feed_items SET read_time = null WHERE id IS :id")
    suspend fun markAsUnread(id: Long)

    @Query("UPDATE feed_items SET read_time = coalesce(read_time, :readTime), notified = 1 WHERE id IN (:ids)")
    suspend fun markAsRead(
        ids: List<Long>,
        readTime: Instant = Instant.now(),
    )

    @Query("UPDATE feed_items SET bookmarked = :bookmarked WHERE id IS :id")
    suspend fun setBookmarked(
        id: Long,
        bookmarked: Boolean,
    )

    @Query("SELECT link FROM feed_items WHERE bookmarked = 1 order by link")
    suspend fun getLinksOfBookmarks(): List<String?>

    @Query("UPDATE feed_items SET notified = :notified WHERE id IN (:ids)")
    suspend fun markAsNotified(
        ids: List<Long>,
        notified: Boolean = true,
    )

    @Query("UPDATE feed_items SET notified = :notified WHERE id IS :id")
    suspend fun markAsNotified(
        id: Long,
        notified: Boolean = true,
    )

    @Query(
        """
        UPDATE feed_items
        SET notified = :notified
        WHERE id IN (
          SELECT feed_items.id
          FROM feed_items
          LEFT JOIN feeds ON feed_items.feed_id = feeds.id
          WHERE tag IS :tag
        )""",
    )
    suspend fun markTagAsNotified(
        tag: String,
        notified: Boolean = true,
    )

    @Query("UPDATE feed_items SET notified = :notified")
    suspend fun markAllAsNotified(notified: Boolean = true)

    @Query("UPDATE feed_items SET read_time = coalesce(read_time, :readTime), notified = 1 WHERE id IS :id")
    suspend fun markAsReadAndNotified(
        id: Long,
        readTime: Instant = Instant.now(),
    )

    @Query("UPDATE feed_items SET read_time = :readTime, notified = 1 WHERE id IS :id")
    suspend fun markAsReadAndNotifiedAndOverwriteReadTime(
        id: Long,
        readTime: Instant = Instant.now(),
    )

    @Query(
        """
            SELECT feeds.open_articles_with
            FROM feed_items
            LEFT JOIN feeds ON feed_items.feed_id = feeds.id
            WHERE feed_items.id IS :itemId
        """,
    )
    suspend fun getOpenArticleWith(itemId: Long): String?

    @Query(
        """
            SELECT feeds.fulltext_by_default
            FROM feed_items
            LEFT JOIN feeds ON feed_items.feed_id = feeds.id
            WHERE feed_items.id IS :itemId
        """,
    )
    suspend fun getFullTextByDefault(itemId: Long): Boolean?

    @Query(
        """
            SELECT link
            FROM feed_items
            WHERE feed_items.id IS :itemid
        """,
    )
    suspend fun getLink(itemid: Long): String?

    @Query(
        """
            SELECT fi.id, fi.link
            FROM feed_items fi
            JOIN feeds f ON feed_id = f.id
            WHERE f.fulltext_by_default = 1
                AND fi.fulltext_downloaded <> 1
                and block_time is null
        """,
    )
    fun getFeedsItemsWithDefaultFullTextNeedingDownload(): Flow<List<FeedItemIdWithLink>>

    @Query(
        """
            UPDATE feed_items
            SET fulltext_downloaded = 1
            WHERE id = :feedItemId
        """,
    )
    suspend fun markAsFullTextDownloaded(feedItemId: Long)

    @Query(
        """
            SELECT count(*)
            FROM feed_items fi
            WHERE
              (read_time is null or read_time >= :minReadTime)
              and bookmarked in (1, :bookmarked)
              and block_time is null
        """,
    )
    fun getFeedItemCount(
        minReadTime: Instant,
        bookmarked: Boolean,
    ): Flow<Int>

    @Query(
        """
            SELECT count(*)
            FROM feed_items fi
            JOIN feeds f ON feed_id = f.id
            WHERE f.tag IS :tag
              and (read_time is null or read_time >= :minReadTime)
              and bookmarked in (1, :bookmarked)
              and block_time is null
        """,
    )
    fun getFeedItemCount(
        tag: String,
        minReadTime: Instant,
        bookmarked: Boolean,
    ): Flow<Int>

    @Query(
        """
            SELECT count(*)
            FROM feed_items fi
            WHERE fi.feed_id IS :feedId
              and (read_time is null or read_time >= :minReadTime)
              and bookmarked in (1, :bookmarked)
              and block_time is null
        """,
    )
    fun getFeedItemCount(
        feedId: Long,
        minReadTime: Instant,
        bookmarked: Boolean,
    ): Flow<Int>

    @Query(
        """
            SELECT fi.id
            FROM feed_items fi
            JOIN feeds f ON feed_id = f.id
            WHERE f.notify IS 1 AND fi.notified IS 0 AND fi.read_time is null
              and block_time is null
        """,
    )
    fun getFeedItemsNeedingNotifying(): Flow<List<Long>>

    @Query(
        """
            SELECT fi.id
            FROM feed_items fi
            JOIN feeds f ON fi.feed_id = f.id
            where f.url IS :feedUrl AND fi.guid IS :articleGuid
        """,
    )
    suspend fun getItemWith(
        feedUrl: URL,
        articleGuid: String,
    ): Long?

    @Query(
        """
            select exists(
                select id
                from feed_items
                where id <> :id and (plain_title = :plainTitle or link = :link) and plain_title <> '' and link is not null and link <> ''
            )
        """,
    )
    suspend fun duplicationExists(
        id: Long,
        plainTitle: String,
        link: String?,
    ): Boolean

    companion object {
        // These are backed by a database index
        const val FEED_ITEM_LIST_SORT_ORDER_DESC =
            "primary_sort_time DESC, pub_date DESC, feed_items.id DESC"
        const val FEED_ITEM_LIST_SORT_ORDER_ASC =
            "primary_sort_time ASC, pub_date ASC, feed_items.id ASC"
    }
}

suspend fun FeedItemDao.upsertFeedItems(
    itemsWithText: List<Pair<FeedItem, String>>,
    block: suspend (FeedItem, String) -> Unit,
) {
    val updatedItems =
        itemsWithText.filter { (item, _) ->
            item.id > ID_UNSET
        }
    updateFeedItems(updatedItems.map { (item, _) -> item })

    val insertedItems =
        itemsWithText.filter { (item, _) ->
            item.id <= ID_UNSET
        }
    val insertedIds = insertFeedItems(insertedItems.map { (item, _) -> item })

    updatedItems.forEach { (item, text) ->
        block(item, text)
    }

    insertedIds.zip(insertedItems).forEach { (itemId, itemToText) ->
        val (item, text) = itemToText

        item.id = itemId

        block(item, text)
    }
}

package com.nononsenseapps.feeder.db.room

import android.database.Cursor
import androidx.paging.PagingSource
import androidx.room.Dao
import androidx.room.Delete
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.RoomWarnings
import androidx.room.Update
import androidx.room.Upsert
import com.nononsenseapps.feeder.db.COL_CUSTOM_TITLE
import com.nononsenseapps.feeder.db.COL_ID
import com.nononsenseapps.feeder.db.COL_TAG
import com.nononsenseapps.feeder.db.COL_TITLE
import com.nononsenseapps.feeder.model.FeedUnreadCount
import kotlinx.coroutines.flow.Flow
import java.net.URL
import java.time.Instant

@Dao
interface FeedDao {
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertFeed(feed: Feed): Long

    @Update
    suspend fun updateFeed(feed: Feed): Int

    @Delete
    suspend fun deleteFeed(feed: Feed): Int

    @Query("DELETE FROM feeds WHERE id IS :feedId")
    suspend fun deleteFeedWithId(feedId: Long): Int

    @Query(
        """
        DELETE FROM feeds WHERE id IN (:ids)
        """,
    )
    suspend fun deleteFeeds(ids: List<Long>): Int

    @Query("SELECT * FROM feeds WHERE id IS :feedId")
    fun loadFeedFlow(feedId: Long): Flow<Feed?>

    @Query("SELECT DISTINCT tag FROM feeds ORDER BY tag COLLATE NOCASE")
    suspend fun loadTags(): List<String>

    @Query("SELECT DISTINCT tag FROM feeds ORDER BY tag COLLATE NOCASE")
    fun loadAllTags(): Flow<List<String>>

    @Query("SELECT * FROM feeds WHERE id IS :feedId and retry_after < :retryAfter")
    suspend fun syncLoadFeed(
        feedId: Long,
        retryAfter: Instant,
    ): Feed?

    @Query("SELECT * FROM feeds WHERE id IS :feedId")
    suspend fun getFeed(feedId: Long): Feed?

    @Query("SELECT * FROM feeds WHERE tag IS :tag ORDER BY title")
    suspend fun getFeedsByTitle(tag: String): List<Feed>

    @Query(
        """
        SELECT 
            id,
            COALESCE(NULLIF(custom_title, ''), title) as title,
            notify
        FROM feeds
        ORDER BY COALESCE(NULLIF(custom_title, ''), title) COLLATE NOCASE
        """,
    )
    fun loadFlowOfFeedsForSettings(): Flow<List<FeedForSettings>>

    @Query(
        "select * from feeds",
    )
    fun getAllFeeds(): List<Feed>

    @Query(
        """
       SELECT * FROM feeds
       WHERE id is :feedId
       AND last_sync < :staleTime
       AND retry_after < :retryAfter
    """,
    )
    suspend fun syncLoadFeedIfStale(
        feedId: Long,
        staleTime: Long,
        retryAfter: Instant,
    ): Feed?

    @Query("SELECT * FROM feeds WHERE tag IS :tag AND retry_after < :retryAfter ORDER BY last_sync")
    suspend fun syncLoadFeeds(
        tag: String,
        retryAfter: Instant,
    ): List<Feed>

    @Query("SELECT * FROM feeds WHERE tag IS :tag AND last_sync < :staleTime AND retry_after < :retryAfter ORDER BY last_sync")
    suspend fun syncLoadFeedsIfStale(
        tag: String,
        staleTime: Long,
        retryAfter: Instant,
    ): List<Feed>

    @Query("SELECT * FROM feeds WHERE retry_after < :retryAfter ORDER BY last_sync")
    suspend fun syncLoadFeeds(retryAfter: Instant): List<Feed>

    @Query("SELECT * FROM feeds WHERE last_sync < :staleTime AND retry_after < :retryAfter ORDER BY last_sync")
    suspend fun syncLoadFeedsIfStale(
        staleTime: Long,
        retryAfter: Instant,
    ): List<Feed>

    @Query(
        """
        SELECT $COL_ID as id, $COL_TITLE as title
        FROM feeds
        ORDER BY $COL_TITLE
    """,
    )
    fun loadFeedsForContentProvider(): Cursor

    @Query("SELECT * FROM feeds WHERE url IS :url")
    suspend fun loadFeedWithUrl(url: URL): Feed?

    @Query("SELECT id FROM feeds WHERE notify IS 1")
    suspend fun loadFeedIdsToNotify(): List<Long>

    // Suppressing sort fields
    @SuppressWarnings(RoomWarnings.CURSOR_MISMATCH)
    @Query(
        """
            -- all items
            select $ID_ALL_FEEDS as id, '' as display_title, '' as tag, '' as image_url, sum(unread) as unread_count, 0 as expanded, 0 as sort_section, 0 as sort_tag_or_feed
            from feeds_with_items_for_nav_drawer
            -- starred
            union
            select $ID_SAVED_ARTICLES as id, '' as display_title, '' as tag, '' as image_url, sum(bookmarked) as unread_count, 0 as expanded, 1 as sort_section, 0 as sort_tag_or_feed
            from feeds_with_items_for_nav_drawer
            where bookmarked
            -- tags
            union
            select $ID_UNSET as id, tag as display_title, tag, '' as image_url, sum(unread) as unread_count, tag in (:expandedTags) as expanded, 2 as sort_section, 0 as sort_tag_or_feed
            from feeds_with_items_for_nav_drawer
            where tag is not ''
            group by tag
            -- feeds
            union
            select feed_id as id, display_title, tag, image_url, sum(unread) as unread_count, 0 as expanded, case when tag is '' then 3 else 2 end as sort_section, 1 as sort_tag_or_feed
            from feeds_with_items_for_nav_drawer
            where tag is '' or tag in (:expandedTags)
            group by feed_id
            -- sort them
            order by tag,id, sort_tag_or_feed, display_title
        """,
    )
    fun getPagedNavDrawerItems(expandedTags: Set<String>): PagingSource<Int, FeedUnreadCount>

    @Query("UPDATE feeds SET notify = :notify WHERE id IS :id")
    suspend fun setNotify(
        id: Long,
        notify: Boolean,
    )

    @Query("UPDATE feeds SET notify = :notify WHERE tag IS :tag")
    suspend fun setNotify(
        tag: String,
        notify: Boolean,
    )

    @Query("UPDATE feeds SET notify = :notify")
    suspend fun setAllNotify(notify: Boolean)

    @Query("SELECT $COL_ID, $COL_TITLE, $COL_CUSTOM_TITLE FROM feeds WHERE id IS :feedId")
    suspend fun getFeedTitle(feedId: Long): FeedTitle?

    @Query("SELECT $COL_ID FROM feeds where url is :url")
    suspend fun getFeedIdForUrl(url: URL): Long?

    @Query("SELECT $COL_ID, $COL_TITLE, $COL_CUSTOM_TITLE FROM feeds WHERE id IS :feedId")
    fun getFeedTitlesWithId(feedId: Long): Flow<List<FeedTitle>>

    @Query(
        """
        SELECT $COL_ID, $COL_TITLE, $COL_CUSTOM_TITLE
        FROM feeds
        WHERE $COL_TAG IS :feedTag
        ORDER BY $COL_TITLE COLLATE NOCASE
        """,
    )
    fun getFeedTitlesWithTag(feedTag: String): Flow<List<FeedTitle>>

    @Query("SELECT $COL_ID, $COL_TITLE, $COL_CUSTOM_TITLE FROM feeds ORDER BY $COL_TITLE COLLATE NOCASE")
    fun getAllFeedTitles(): Flow<List<FeedTitle>>

    // Not filtering on currently_syncing so the refresh indicator doesn't go up and down due to
    // single threaded sync.
    @Query(
        """
            SELECT MAX(last_sync)
            FROM feeds
        """,
    )
    fun getCurrentlySyncingLatestTimestamp(): Flow<Instant?>

    @Query(
        """
            UPDATE feeds
            SET currently_syncing = :syncing
            WHERE id IS :feedId
        """,
    )
    suspend fun setCurrentlySyncingOn(
        feedId: Long,
        syncing: Boolean,
    )

    @Query(
        """
            UPDATE feeds
            SET currently_syncing = :syncing, last_sync = :lastSync
            WHERE id IS :feedId
        """,
    )
    suspend fun setCurrentlySyncingOn(
        feedId: Long,
        syncing: Boolean,
        lastSync: Instant,
    )

    @Query(
        """
            SELECT *
            FROM feeds
            ORDER BY url
        """,
    )
    suspend fun getFeedsOrderedByUrl(): List<Feed>

    @Query(
        """
            SELECT *
            FROM feeds
            ORDER BY url
        """,
    )
    fun getFlowOfFeedsOrderedByUrl(): Flow<List<Feed>>

    @Query(
        """
            DELETE FROM feeds
            WHERE url is :url
        """,
    )
    suspend fun deleteFeedWithUrl(url: URL): Int

    @Upsert
    suspend fun upsert(feed: Feed): Long

    /**
     * Using HOST because we want to match server, and the possibility of username:password
     * means it's not as simple as SCHEME://HOST to build the prefix.
     */
    @Query(
        """
            UPDATE feeds
            SET retry_after = :retryAfter
            WHERE url LIKE '%' || :host || '%'
            AND retry_after < :retryAfter
        """,
    )
    suspend fun setRetryAfterForFeedsWithBaseUrl(
        host: String,
        retryAfter: Instant,
    )
}

package com.nononsenseapps.feeder.archmodel

import android.database.sqlite.SQLiteConstraintException
import androidx.paging.Pager
import androidx.paging.PagingConfig
import androidx.paging.PagingData
import com.nononsenseapps.feeder.db.room.Feed
import com.nononsenseapps.feeder.db.room.FeedDao
import com.nononsenseapps.feeder.db.room.FeedForSettings
import com.nononsenseapps.feeder.db.room.FeedTitle
import com.nononsenseapps.feeder.db.room.ID_UNSET
import com.nononsenseapps.feeder.model.FeedUnreadCount
import kotlinx.coroutines.flow.Flow
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.instance
import java.net.URL
import java.time.Instant

class FeedStore(override val di: DI) : DIAware {
    private val feedDao: FeedDao by instance()

    // Need only be internally consistent within the composition
    // this object outlives all compositions
    private var nextTagUiId: Long = -1000

    // But IDs need to be consistent even if tags come and go
    private val tagUiIds = mutableMapOf<String, Long>()

    private fun getTagUiId(tag: String): Long {
        return tagUiIds.getOrPut(tag) {
            --nextTagUiId
        }
    }

    suspend fun getFeed(feedId: Long): Feed? = feedDao.getFeed(feedId = feedId)

    suspend fun getFeed(url: URL): Feed? = feedDao.loadFeedWithUrl(url)

    suspend fun saveFeed(feed: Feed): Long {
        return if (feed.id > ID_UNSET) {
            feedDao.updateFeed(feed)
            feed.id
        } else {
            feedDao.insertFeed(feed)
        }
    }

    suspend fun toggleNotifications(
        feedId: Long,
        value: Boolean,
    ) = feedDao.setNotify(id = feedId, notify = value)

    suspend fun getDisplayTitle(feedId: Long): String? = feedDao.getFeedTitle(feedId)?.displayTitle

    suspend fun deleteFeeds(feedIds: List<Long>) {
        feedDao.deleteFeeds(feedIds)
    }

    val allTags: Flow<List<String>> = feedDao.loadAllTags()

    val feedForSettings: Flow<List<FeedForSettings>> = feedDao.loadFlowOfFeedsForSettings()

    fun getPagedNavDrawerItems(expandedTags: Set<String>): Flow<PagingData<FeedUnreadCount>> =
        Pager(
            config =
                PagingConfig(
                    pageSize = 10,
                    initialLoadSize = 50,
                    prefetchDistance = 50,
                    jumpThreshold = 50,
                ),
        ) {
            feedDao.getPagedNavDrawerItems(expandedTags)
        }
            .flow

    fun getFeedTitles(
        feedId: Long,
        tag: String,
    ): Flow<List<FeedTitle>> =
        when {
            feedId > ID_UNSET -> feedDao.getFeedTitlesWithId(feedId)
            tag.isNotBlank() -> feedDao.getFeedTitlesWithTag(tag)
            else -> feedDao.getAllFeedTitles()
        }

    fun getCurrentlySyncingLatestTimestamp(): Flow<Instant?> = feedDao.getCurrentlySyncingLatestTimestamp()

    suspend fun setCurrentlySyncingOn(
        feedId: Long,
        syncing: Boolean,
        lastSync: Instant? = null,
    ) {
        if (lastSync != null) {
            feedDao.setCurrentlySyncingOn(feedId = feedId, syncing = syncing, lastSync = lastSync)
        } else {
            feedDao.setCurrentlySyncingOn(feedId = feedId, syncing = syncing)
        }
    }

    suspend fun upsertFeed(feedSql: Feed): Long {
        return try {
            feedDao.upsert(feed = feedSql)
        } catch (e: SQLiteConstraintException) {
            // upsert only works if ID is defined - need to catch constraint errors still
            if (feedSql.id == ID_UNSET) {
                val feedId = feedDao.getFeedIdForUrl(feedSql.url)

                if (feedId != null) {
                    feedDao.upsert(feed = feedSql.copy(id = feedId))
                } else {
                    throw e
                }
            } else {
                throw e
            }
        }
    }

    suspend fun getFeedsOrderedByUrl(): List<Feed> {
        return feedDao.getFeedsOrderedByUrl()
    }

    suspend fun deleteFeed(url: URL) {
        feedDao.deleteFeedWithUrl(url)
    }

    suspend fun syncLoadFeedIfStale(
        feedId: Long,
        staleTime: Long,
        retryAfter: Instant,
    ) = feedDao.syncLoadFeedIfStale(feedId = feedId, staleTime = staleTime, retryAfter = retryAfter)

    suspend fun syncLoadFeed(
        feedId: Long,
        retryAfter: Instant,
    ): Feed? = feedDao.syncLoadFeed(feedId = feedId, retryAfter = retryAfter)

    suspend fun syncLoadFeedsIfStale(
        tag: String,
        staleTime: Long,
        retryAfter: Instant,
    ) = feedDao.syncLoadFeedsIfStale(tag = tag, staleTime = staleTime, retryAfter = retryAfter)

    suspend fun syncLoadFeedsIfStale(
        staleTime: Long,
        retryAfter: Instant,
    ) = feedDao.syncLoadFeedsIfStale(staleTime = staleTime, retryAfter = retryAfter)

    suspend fun syncLoadFeeds(
        tag: String,
        retryAfter: Instant,
    ) = feedDao.syncLoadFeeds(tag = tag, retryAfter = retryAfter)

    suspend fun syncLoadFeeds(retryAfter: Instant) = feedDao.syncLoadFeeds(retryAfter = retryAfter)

    suspend fun setRetryAfterForFeedsWithBaseUrl(
        host: String,
        retryAfter: Instant,
    ) {
        feedDao.setRetryAfterForFeedsWithBaseUrl(host = host, retryAfter = retryAfter)
    }
}

package com.nononsenseapps.feeder.db.room

import androidx.room.Dao
import androidx.room.Delete
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.Transaction
import java.net.URL

@Dao
interface RemoteFeedDao {
    @Insert(onConflict = OnConflictStrategy.IGNORE)
    suspend fun insert(remoteFeed: RemoteFeed): Long

    @Delete
    suspend fun delete(remoteFeed: RemoteFeed): Int

    @Query(
        """
            SELECT url
            FROM remote_feed
            WHERE sync_remote IS 1
        """,
    )
    suspend fun getRemotelySeenFeeds(): List<URL>

    @Query(
        """
            DELETE FROM remote_feed
        """,
    )
    suspend fun deleteAllRemoteFeeds()

    @Transaction
    suspend fun replaceRemoteFeedsWith(remoteFeeds: List<RemoteFeed>) {
        deleteAllRemoteFeeds()
        for (remoteFeed in remoteFeeds) {
            insert(remoteFeed)
        }
    }
}

package com.nononsenseapps.feeder.archmodel

import com.nononsenseapps.feeder.crypto.AesCbcWithIntegrity
import com.nononsenseapps.feeder.db.room.FeedItemForReadMark
import com.nononsenseapps.feeder.db.room.ReadStatusSynced
import com.nononsenseapps.feeder.db.room.ReadStatusSyncedDao
import com.nononsenseapps.feeder.db.room.RemoteFeed
import com.nononsenseapps.feeder.db.room.RemoteFeedDao
import com.nononsenseapps.feeder.db.room.RemoteReadMark
import com.nononsenseapps.feeder.db.room.RemoteReadMarkDao
import com.nononsenseapps.feeder.db.room.SyncDevice
import com.nononsenseapps.feeder.db.room.SyncDeviceDao
import com.nononsenseapps.feeder.db.room.SyncRemote
import com.nononsenseapps.feeder.db.room.SyncRemoteDao
import com.nononsenseapps.feeder.db.room.generateDeviceName
import kotlinx.coroutines.flow.Flow
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.instance
import java.net.URL
import java.time.Instant
import java.time.temporal.ChronoUnit

class SyncRemoteStore(override val di: DI) : DIAware {
    private val dao: SyncRemoteDao by instance()
    private val readStatusDao: ReadStatusSyncedDao by instance()
    private val remoteReadMarkDao: RemoteReadMarkDao by instance()
    private val syncDeviceDao: SyncDeviceDao by instance()
    private val remoteFeedDao: RemoteFeedDao by instance()

    suspend fun getSyncRemote(): SyncRemote {
        dao.getSyncRemote()?.let {
            return it
        }

        return createDefaultSyncRemote()
    }

    fun getSyncRemoteFlow(): Flow<SyncRemote?> {
        return dao.getSyncRemoteFlow()
    }

    suspend fun updateSyncRemote(syncRemote: SyncRemote) {
        dao.update(syncRemote)
    }

    suspend fun updateSyncRemoteMessageTimestamp(timestamp: Instant) {
        dao.updateLastMessageTimestamp(timestamp)
    }

    suspend fun deleteAllReadStatusSyncs() {
        readStatusDao.deleteAll()
    }

    suspend fun deleteReadStatusSyncs(ids: List<Long>) {
        readStatusDao.deleteReadStatusSyncs(ids)
    }

    fun getNextFeedItemWithoutSyncedReadMark(): Flow<FeedItemForReadMark?> {
        return readStatusDao.getNextFeedItemWithoutSyncedReadMark()
    }

    fun getFlowOfFeedItemsWithoutSyncedReadMark(): Flow<List<FeedItemForReadMark>> {
        return readStatusDao.getFlowOfFeedItemsWithoutSyncedReadMark()
    }

    suspend fun getFeedItemsWithoutSyncedReadMark(): List<FeedItemForReadMark> {
        return readStatusDao.getFeedItemsWithoutSyncedReadMark()
    }

    suspend fun setSynced(feedItemId: Long) {
        // Ignores duplicates
        readStatusDao.insert(
            ReadStatusSynced(
                feed_item = feedItemId,
                sync_remote = 1L,
            ),
        )
    }

    suspend fun setNotSynced(feedItemId: Long) {
        // Ignores duplicates
        readStatusDao.deleteReadStatusSyncForItem(feedItemId)
    }

    suspend fun addRemoteReadMark(
        feedUrl: URL,
        articleGuid: String,
    ) {
        // Ignores duplicates
        remoteReadMarkDao.insert(
            RemoteReadMark(
                sync_remote = 1L,
                feedUrl = feedUrl,
                guid = articleGuid,
                timestamp = Instant.now(),
            ),
        )
    }

    suspend fun deleteStaleRemoteReadMarks(now: Instant) {
        // 7 days ago
        remoteReadMarkDao.deleteStaleRemoteReadMarks(now.minus(7, ChronoUnit.DAYS))
    }

    suspend fun getRemoteReadMarksReadyToBeApplied() = remoteReadMarkDao.getRemoteReadMarksReadyToBeApplied()

    suspend fun getGuidsWhichAreSyncedAsReadInFeed(feedUrl: URL) = remoteReadMarkDao.getGuidsWhichAreSyncedAsReadInFeed(feedUrl = feedUrl)

    suspend fun replaceWithDefaultSyncRemote() {
        dao.replaceWithDefaultSyncRemote()
    }

    private suspend fun createDefaultSyncRemote(): SyncRemote {
        val remote =
            SyncRemote(
                id = 1L,
                deviceName = generateDeviceName(),
                secretKey = AesCbcWithIntegrity.generateKey().toString(),
            )
        dao.insert(remote)
        return remote
    }

    fun getDevices(): Flow<List<SyncDevice>> {
        return syncDeviceDao.getDevices()
    }

    suspend fun replaceDevices(devices: List<SyncDevice>) {
        syncDeviceDao.replaceDevices(devices)
    }

    suspend fun getRemotelySeenFeeds(): List<URL> {
        return remoteFeedDao.getRemotelySeenFeeds()
    }

    suspend fun replaceRemoteFeedsWith(remoteFeeds: List<RemoteFeed>) {
        remoteFeedDao.replaceRemoteFeedsWith(remoteFeeds)
    }

    companion object {
        private const val LOG_TAG = "FEEDER_SyncRemoteStore"
    }
}

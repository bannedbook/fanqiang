@file:OptIn(ExperimentalContracts::class)

package com.nononsenseapps.feeder.sync

import android.util.Log
import com.nononsenseapps.feeder.archmodel.Repository
import com.nononsenseapps.feeder.crypto.AesCbcWithIntegrity
import com.nononsenseapps.feeder.crypto.SecretKeys
import com.nononsenseapps.feeder.db.room.DEFAULT_SERVER_ADDRESS
import com.nononsenseapps.feeder.db.room.DEPRECATED_SYNC_HOSTS
import com.nononsenseapps.feeder.db.room.Feed
import com.nononsenseapps.feeder.db.room.FeedItemForReadMark
import com.nononsenseapps.feeder.db.room.RemoteFeed
import com.nononsenseapps.feeder.db.room.SyncDevice
import com.nononsenseapps.feeder.db.room.SyncRemote
import com.nononsenseapps.feeder.db.room.generateDeviceName
import com.nononsenseapps.feeder.util.Either
import com.nononsenseapps.feeder.util.logDebug
import kotlinx.coroutines.runBlocking
import okhttp3.OkHttpClient
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.instance
import retrofit2.Response
import java.net.URL
import java.time.Instant
import kotlin.contracts.ExperimentalContracts

class SyncRestClient(override val di: DI) : DIAware {
    private val repository: Repository by instance()
    private val okHttpClient: OkHttpClient by instance()
    private var feederSync: FeederSync? = null
    private var secretKey: SecretKeys? = null
    private val moshi = getMoshi()
    private val readMarkAdapter = moshi.adapter<ReadMarkContent>()
    private val feedsAdapter = moshi.adapter<EncryptedFeeds>()

    init {
        runBlocking {
            initialize()
        }
    }

    private val isInitialized: Boolean
        get() = feederSync != null && secretKey != null
    private val isNotInitialized
        get() = !isInitialized

    internal suspend fun initialize() {
        if (isNotInitialized) {
            try {
                var syncRemote = repository.getSyncRemote()
                if (DEPRECATED_SYNC_HOSTS.any { host -> host in "${syncRemote.url}" }) {
                    logDebug(
                        LOG_TAG,
                        "Updating to latest sync host: $DEFAULT_SERVER_ADDRESS",
                    )
                    syncRemote =
                        syncRemote.copy(
                            url = URL(DEFAULT_SERVER_ADDRESS),
                        )
                    repository.updateSyncRemote(syncRemote)
                }
                if (syncRemote.hasSyncChain()) {
                    secretKey = AesCbcWithIntegrity.decodeKey(syncRemote.secretKey)
                    println("SyncRestClient client.proxy:" + okHttpClient.proxy.toString())
                    feederSync =
                        getFeederSyncClient(
                            syncRemote = syncRemote,
                            okHttpClient = okHttpClient,
                        )
                }
            } catch (e: Exception) {
                Log.e(LOG_TAG, "Failed to initialize", e)
            }
        }
    }

    private suspend fun <A> safeBlock(block: (suspend (SyncRemote, FeederSync, SecretKeys) -> Either<ErrorResponse, A>)?): Either<ErrorResponse, A> {
        if (block != null) {
            repository.getSyncRemote().let { syncRemote ->
                if (syncRemote.hasSyncChain()) {
                    feederSync?.let { feederSync ->
                        secretKey?.let { secretKey ->
                            return block(syncRemote, feederSync, secretKey)
                        }
                    }
                }
            }
        }
        return Either.Left(
            ErrorResponse(
                code = 1001,
                body = null,
            ),
        )
    }

    suspend fun create(): Either<ErrorResponse, String> {
        logDebug(LOG_TAG, "create")
        // To ensure always uses correct client, manually set remote here ALWAYS
        val syncRemote = repository.getSyncRemote()

        val secretKey = AesCbcWithIntegrity.decodeKey(syncRemote.secretKey)
        this.secretKey = secretKey

        val feederSync =
            getFeederSyncClient(
                syncRemote = syncRemote,
                okHttpClient = okHttpClient,
            )
        this.feederSync = feederSync

        val deviceName = generateDeviceName()

        return feederSync.create(
            CreateRequest(
                deviceName = AesCbcWithIntegrity.encryptString(deviceName, secretKey),
            ),
        ).toEither()
            .onRight { response ->
                repository.updateSyncRemote(
                    syncRemote.copy(
                        syncChainId = response.syncCode,
                        deviceId = response.deviceId,
                        deviceName = deviceName,
                        latestMessageTimestamp = Instant.EPOCH,
                    ),
                )
            }
            .map { response ->
                response.syncCode
            }
    }

    suspend fun join(
        syncCode: String,
        remoteSecretKey: String,
    ): Either<ErrorResponse, String> {
        logDebug(LOG_TAG, "join")
        try {
            logDebug(LOG_TAG, "Really joining")
            // To ensure always uses correct client, manually set remote here ALWAYS
            val syncRemote = repository.getSyncRemote()
            syncRemote.secretKey = remoteSecretKey
            syncRemote.deviceName = generateDeviceName()
            repository.updateSyncRemote(syncRemote)

            val secretKey = AesCbcWithIntegrity.decodeKey(syncRemote.secretKey)
            this.secretKey = secretKey

            val feederSync =
                getFeederSyncClient(
                    syncRemote = syncRemote,
                    okHttpClient = okHttpClient,
                )
            this.feederSync = feederSync

            logDebug(LOG_TAG, "Updated objects")

            return feederSync.join(
                syncChainId = syncCode,
                request =
                    JoinRequest(
                        deviceName =
                            AesCbcWithIntegrity.encryptString(
                                syncRemote.deviceName,
                                secretKey,
                            ),
                    ),
            ).toEither()
                .onRight { response ->
                    logDebug(LOG_TAG, "Join response: $response")

                    repository.updateSyncRemote(
                        syncRemote.copy(
                            syncChainId = response.syncCode,
                            deviceId = response.deviceId,
                            latestMessageTimestamp = Instant.EPOCH,
                        ),
                    )

                    logDebug(LOG_TAG, "Updated sync remote")
                }
                .map { response ->
                    response.syncCode
                }
        } catch (e: Exception) {
            if (e is retrofit2.HttpException) {
                Log.e(
                    LOG_TAG,
                    "Error during leave: msg: code: ${e.code()}, error: ${
                        e.response()?.errorBody()?.string()
                    }",
                    e,
                )
            } else {
                Log.e(LOG_TAG, "Error during leave", e)
            }
            return Either.Left(ErrorResponse(999, e.message))
        }
    }

    suspend fun leave(): Either<ErrorResponse, Unit> {
        logDebug(LOG_TAG, "leave")
        return try {
            safeBlock { syncRemote, feederSync, _ ->
                logDebug(LOG_TAG, "Really leaving")
                feederSync.removeDevice(
                    syncChainId = syncRemote.syncChainId,
                    currentDeviceId = syncRemote.deviceId,
                    deviceId = syncRemote.deviceId,
                ).toEither()
                    .onLeft {
                        Log.e(LOG_TAG, "Error during leave: ${it.code}, ${it.body}", it.throwable)
                    }
                    .map {
                    }.also {
                        this.feederSync = null
                        this.secretKey = null
                    }
            }
        } catch (e: Exception) {
            Log.e(LOG_TAG, "Error during leave", e)
            Either.Left(
                ErrorResponse(1000, e.message, e),
            )
        } finally {
            repository.replaceWithDefaultSyncRemote()
        }
    }

    suspend fun removeDevice(deviceId: Long): Either<ErrorResponse, DeviceListResponse> {
        return try {
            safeBlock { syncRemote, feederSync, secretKey ->
                logDebug(LOG_TAG, "removeDevice")
                feederSync.removeDevice(
                    syncChainId = syncRemote.syncChainId,
                    currentDeviceId = syncRemote.deviceId,
                    deviceId = deviceId,
                ).toEither()
                    .onRight { deviceListResponse ->
                        logDebug(LOG_TAG, "Updating device list: $deviceListResponse")

                        repository.replaceDevices(
                            deviceListResponse.devices.map {
                                SyncDevice(
                                    deviceId = it.deviceId,
                                    deviceName =
                                        AesCbcWithIntegrity.decryptString(
                                            it.deviceName,
                                            secretKey,
                                        ),
                                    syncRemote = syncRemote.id,
                                )
                            },
                        )
                    }
                    .onLeft {
                        it.leaveChainIfKickedOutElseLog()
                    }
            }
        } catch (e: Exception) {
            Log.e(LOG_TAG, "Error in removeDevice", e)
            Either.Left(
                ErrorResponse(1000, e.message, e),
            )
        }
    }

    internal suspend fun markAsRead(feedItems: List<FeedItemForReadMark>): Either<ErrorResponse, SendReadMarkResponse> {
        return try {
            safeBlock { syncRemote, feederSync, secretKey ->
                logDebug(LOG_TAG, "markAsRead: ${feedItems.size} items")
                feederSync.sendEncryptedReadMarks(
                    currentDeviceId = syncRemote.deviceId,
                    syncChainId = syncRemote.syncChainId,
                    request =
                        SendEncryptedReadMarkBulkRequest(
                            items =
                                feedItems.map { feedItem ->
                                    SendEncryptedReadMarkRequest(
                                        encrypted =
                                            AesCbcWithIntegrity.encryptString(
                                                secretKeys = secretKey,
                                                plaintext =
                                                    readMarkAdapter.toJson(
                                                        ReadMarkContent(
                                                            feedUrl = feedItem.feedUrl,
                                                            articleGuid = feedItem.guid,
                                                        ),
                                                    ),
                                            ),
                                    )
                                },
                        ),
                ).toEither()
                    .onRight {
                        for (feedItem in feedItems) {
                            repository.setSynced(feedItemId = feedItem.id)
                        }
                    }
                    .onLeft {
                        it.leaveChainIfKickedOutElseLog()
                    }

                // Should not set latest timestamp here because we cant be sure to retrieved them
            }
        } catch (e: Exception) {
            Log.e(LOG_TAG, "Error in markAsRead", e)
            Either.Left(
                ErrorResponse(1000, e.message, e),
            )
        }
    }

    suspend fun markAsRead(): Either<ErrorResponse, Unit> {
        return try {
            safeBlock { _, _, _ ->
                val readItems = repository.getFeedItemsWithoutSyncedReadMark()

                if (readItems.isNotEmpty()) {
                    logDebug(LOG_TAG, "markAsReadBatch: ${readItems.size} items")

                    readItems.asSequence()
                        .chunked(100)
                        .forEach { feedItems ->
                            markAsRead(feedItems)
                        }
                }
                Either.Right(Unit)
            }
        } catch (e: Exception) {
            Log.e(LOG_TAG, "Error in markAsRead", e)
            Either.Left(
                ErrorResponse(1000, e.message, e),
            )
        }
    }

    internal suspend fun getDevices(): Either<ErrorResponse, DeviceListResponse> {
        return try {
            logDebug(LOG_TAG, "getDevices")
            safeBlock { syncRemote, feederSync, secretKey ->
                logDebug(LOG_TAG, "getDevices Inside block")
                feederSync.getDevices(
                    syncChainId = syncRemote.syncChainId,
                    currentDeviceId = syncRemote.deviceId,
                ).toEither()
                    .onRight { response ->
                        logDebug(LOG_TAG, "getDevices: $response")

                        repository.replaceDevices(
                            response.devices.map {
                                logDebug(LOG_TAG, "device: $it")
                                SyncDevice(
                                    deviceId = it.deviceId,
                                    deviceName =
                                        AesCbcWithIntegrity.decryptString(
                                            it.deviceName,
                                            secretKey,
                                        ),
                                    syncRemote = syncRemote.id,
                                )
                            },
                        )
                    }
                    .onLeft {
                        it.leaveChainIfKickedOutElseLog()
                    }
            }
        } catch (e: Exception) {
            Log.e(LOG_TAG, "Error in getDevices", e)
            Either.Left(
                ErrorResponse(1000, e.message, e),
            )
        }
    }

    internal suspend fun getRead() {
        try {
            safeBlock { syncRemote, feederSync, secretKey ->
                logDebug(LOG_TAG, "getRead")
                feederSync.getEncryptedReadMarks(
                    currentDeviceId = syncRemote.deviceId,
                    syncChainId = syncRemote.syncChainId,
                    // Add one ms so we don't get inclusive of last message we got
                    sinceMillis = syncRemote.latestMessageTimestamp.plusMillis(1).toEpochMilli(),
                ).toEither()
                    .onRight { response ->
                        logDebug(LOG_TAG, "getRead: ${response.readMarks.size} read marks")
                        for (readMark in response.readMarks) {
                            val readMarkContent =
                                readMarkAdapter.fromJson(
                                    AesCbcWithIntegrity.decryptString(readMark.encrypted, secretKey),
                                )

                            if (readMarkContent == null) {
                                Log.e(LOG_TAG, "Failed to decrypt readMark content")
                                continue
                            }

                            repository.remoteMarkAsRead(
                                feedUrl = readMarkContent.feedUrl,
                                articleGuid = readMarkContent.articleGuid,
                            )
                            repository.updateSyncRemoteMessageTimestamp(readMark.timestamp)
                        }
                    }
                    .onLeft {
                        it.leaveChainIfKickedOutElseLog()
                    }
            }
        } catch (e: Exception) {
            Log.e(LOG_TAG, "Error in getRead", e)
        }
    }

    internal suspend fun getFeeds() {
        try {
            safeBlock { syncRemote, feederSync, secretKey ->
                logDebug(LOG_TAG, "getFeeds")
                feederSync.getFeeds(
                    syncChainId = syncRemote.syncChainId,
                    currentDeviceId = syncRemote.deviceId,
                ).toEither()
                    .onRight { response ->
                        logDebug(LOG_TAG, "GetFeeds response hash: ${response.hash}")

                        if (response.hash == syncRemote.lastFeedsRemoteHash) {
                            // Nothing to do
                            logDebug(LOG_TAG, "GetFeeds got nothing new, returning.")
                            return@onRight
                        }

                        val encryptedFeeds =
                            feedsAdapter.fromJson(
                                AesCbcWithIntegrity.decryptString(
                                    response.encrypted,
                                    secretKeys = secretKey,
                                ),
                            )

                        if (encryptedFeeds == null) {
                            Log.e(LOG_TAG, "Failed to decrypt encrypted feeds")
                            return@onRight
                        }

                        feedDiffing(encryptedFeeds.feeds)

                        syncRemote.lastFeedsRemoteHash = response.hash
                        repository.updateSyncRemote(syncRemote)
                    }
                    .onLeft {
                        it.leaveChainIfKickedOutElseLog()
                    }
            }
        } catch (e: Exception) {
            Log.e(LOG_TAG, "Error in getFeeds", e)
        }
    }

    private suspend fun feedDiffing(remoteFeeds: List<EncryptedFeed>) {
        try {
            logDebug(LOG_TAG, "feedDiffing: ${remoteFeeds.size}")
            val remotelySeenFeedUrls = repository.getRemotelySeenFeeds()

            val feedUrlsWhichWereDeletedOnRemote =
                remotelySeenFeedUrls
                    .filterNot { url -> remoteFeeds.asSequence().map { it.url }.contains(url) }

            logDebug(LOG_TAG, "RemotelyDeleted: ${feedUrlsWhichWereDeletedOnRemote.size}")

            for (url in feedUrlsWhichWereDeletedOnRemote) {
                logDebug(LOG_TAG, "Deleting remotely deleted feed: $url")
                repository.deleteFeed(url)
            }

            for (remoteFeed in remoteFeeds) {
                val seenRemotelyBefore = remoteFeed.url in remotelySeenFeedUrls
                val dbFeed = repository.getFeed(remoteFeed.url)

                when {
                    dbFeed == null && !seenRemotelyBefore -> {
                        // Entirely new remote feed
                        logDebug(LOG_TAG, "Saving new feed: ${remoteFeed.url}")
                        repository.saveFeed(
                            remoteFeed.updateFeedCopy(Feed()),
                        )
                    }

                    dbFeed == null && seenRemotelyBefore -> {
                        // Has been locally deleted, it will be deleted on next call of updateFeeds
                        logDebug(
                            LOG_TAG,
                            "Received update for locally deleted feed: ${remoteFeed.url}",
                        )
                    }

                    dbFeed != null -> {
                        // Update of feed
                        // Compare modification date - only save if remote is newer
                        if (remoteFeed.whenModified > dbFeed.whenModified) {
                            logDebug(LOG_TAG, "Saving updated feed: ${remoteFeed.url}")
                            repository.saveFeed(
                                remoteFeed.updateFeedCopy(dbFeed),
                            )
                        } else {
                            logDebug(
                                LOG_TAG,
                                "Not saving feed because local date trumps it: ${remoteFeed.url}",
                            )
                        }
                    }
                }
            }

            repository.replaceRemoteFeedsWith(
                remoteFeeds.map {
                    RemoteFeed(
                        syncRemote = 1L,
                        url = it.url,
                    )
                },
            )
        } catch (e: Exception) {
            Log.e(LOG_TAG, "Error in feedDiffing", e)
        }
    }

    suspend fun sendUpdatedFeeds(): Either<ErrorResponse, Boolean> {
        return try {
            safeBlock { syncRemote, feederSync, secretKey ->
                logDebug(LOG_TAG, "sendUpdatedFeeds")
                val lastRemoteHash = syncRemote.lastFeedsRemoteHash

                // Only send if hash does not match
                // Important to keep iteration order stable - across devices. So sort on URL, not ID or date
                val feeds =
                    repository.getFeedsOrderedByUrl()
                        .map { it.toEncryptedFeed() }

                // Yes, List hashCodes are based on elements. Just remember to hash what you send
                // - and not raw database objects
                val currentContentHash = feeds.hashCode()

                if (lastRemoteHash == currentContentHash) {
                    // Nothing to do
                    logDebug(LOG_TAG, "Feeds haven't changed - so not sending")
                    return@safeBlock Either.Right(false)
                }

                val encrypted =
                    AesCbcWithIntegrity.encryptString(
                        feedsAdapter.toJson(
                            EncryptedFeeds(
                                feeds = feeds,
                            ),
                        ),
                        secretKeys = secretKey,
                    )

                logDebug(
                    LOG_TAG,
                    "Sending updated feeds with locally computed hash: $currentContentHash",
                )
                // Might fail with 412 in case already updated remotely - need to call get
                feederSync.updateFeeds(
                    syncChainId = syncRemote.syncChainId,
                    currentDeviceId = syncRemote.deviceId,
                    etagValue = syncRemote.lastFeedsRemoteHash.asWeakETagValue(),
                    request =
                        UpdateFeedsRequest(
                            contentHash = currentContentHash,
                            encrypted = encrypted,
                        ),
                ).toEither()
                    .onLeft {
                        if (it.code == 412) {
                            // Need to call get first because updates have happened
                            getFeeds()
                            // Now try again
                            sendUpdatedFeeds()
                        } else {
                            it.leaveChainIfKickedOutElseLog()
                        }
                    }
                    .onRight { response ->
                        // Store hash for future
                        syncRemote.lastFeedsRemoteHash = response.hash
                        repository.updateSyncRemote(syncRemote)

                        logDebug(LOG_TAG, "Received updated feeds hash: ${response.hash}")
                    }
                    .map {
                        true
                    }
            }
        } catch (e: Exception) {
            Log.e(LOG_TAG, "Error in sendUpdatedFeeds", e)
            Either.Left(
                ErrorResponse(1000, e.message, e),
            )
        }
    }

    private suspend fun ErrorResponse.leaveChainIfKickedOutElseLog() {
        Log.e(LOG_TAG, "leaveChainIfKickedOutElseLog: $code, $body", throwable)
        if (code == 400 && body?.contains("Device not registered", ignoreCase = true) == true) {
            // Forbidden, this device has been removed from the chain from another device
            leave()
        }
    }

    companion object {
        private const val LOG_TAG = "FEEDER_REST_CLIENT"
    }
}

fun Any.asWeakETagValue() = "W/\"$this\""

fun <T> Response<T>.toEither(): Either<ErrorResponse, T> {
    return try {
        if (isSuccessful) {
            body()?.let { Either.Right(it) }
                ?: Either.Left(
                    ErrorResponse(
                        code = 998,
                        body = "No body but success",
                    ),
                )
        } else {
            Either.Left(
                ErrorResponse(
                    code = code(),
                    body = errorBody()?.string(),
                ),
            )
        }
    } catch (e: Exception) {
        Either.Left(
            ErrorResponse(
                code = 999,
                body = e.message,
                throwable = e,
            ),
        )
    }
}

data class ErrorResponse(
    val code: Int,
    val body: String? = null,
    val throwable: Throwable? = null,
)

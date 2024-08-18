package com.nononsenseapps.feeder.db.room

import android.os.Build
import androidx.room.ColumnInfo
import androidx.room.Entity
import androidx.room.Ignore
import androidx.room.PrimaryKey
import com.nononsenseapps.feeder.db.COL_DEVICE_ID
import com.nononsenseapps.feeder.db.COL_DEVICE_NAME
import com.nononsenseapps.feeder.db.COL_ID
import com.nononsenseapps.feeder.db.COL_LAST_FEEDS_REMOTE_HASH
import com.nononsenseapps.feeder.db.COL_LATEST_MESSAGE_TIMESTAMP
import com.nononsenseapps.feeder.db.COL_SECRET_KEY
import com.nononsenseapps.feeder.db.COL_SYNC_CHAIN_ID
import com.nononsenseapps.feeder.db.COL_URL
import com.nononsenseapps.feeder.db.SYNC_REMOTE_TABLE_NAME
import java.net.URL
import java.time.Instant
import java.util.Locale
import kotlin.random.Random

@Entity(
    tableName = SYNC_REMOTE_TABLE_NAME,
)
data class SyncRemote
    @Ignore
    constructor(
        @PrimaryKey(autoGenerate = true)
        @ColumnInfo(name = COL_ID)
        var id: Long = ID_UNSET,
        @ColumnInfo(name = COL_URL) var url: URL = URL(DEFAULT_SERVER_ADDRESS),
        @ColumnInfo(name = COL_SYNC_CHAIN_ID) var syncChainId: String = "",
        @ColumnInfo(
            name = COL_LATEST_MESSAGE_TIMESTAMP,
            typeAffinity = ColumnInfo.INTEGER,
        ) var latestMessageTimestamp: Instant = Instant.EPOCH,
        @ColumnInfo(name = COL_DEVICE_ID) var deviceId: Long = 0L,
        @ColumnInfo(name = COL_DEVICE_NAME) var deviceName: String = generateDeviceName(),
        @ColumnInfo(name = COL_SECRET_KEY) var secretKey: String = "",
        @ColumnInfo(name = COL_LAST_FEEDS_REMOTE_HASH) var lastFeedsRemoteHash: Int = 0,
    ) {
        constructor() : this(id = ID_UNSET)

        fun hasSyncChain(): Boolean = syncChainId.length == 64

        fun notHasSyncChain() = !hasSyncChain()
    }

private const val DEFAULT_SERVER_HOST = "feeder-sync.nononsenseapps.com"
private const val DEFAULT_SERVER_PORT = 443
const val DEFAULT_SERVER_ADDRESS = "https://$DEFAULT_SERVER_HOST:$DEFAULT_SERVER_PORT"

val DEPRECATED_SYNC_HOSTS =
    listOf(
        "feederapp.nononsenseapps.com",
        "feeder-sync.nononsenseapps.workers.dev",
    )

fun generateDeviceName(): String {
    val manufacturer = Build.MANUFACTURER ?: ""
    val model = Build.MODEL ?: ""

    return if (model.startsWith(manufacturer, ignoreCase = true)) {
        model
    } else {
        "$manufacturer $model"
    }.replaceFirstChar {
        if (it.isLowerCase()) {
            it.titlecase(
                Locale.getDefault(),
            )
        } else {
            it.toString()
        }
    }.ifBlank { "${Random.nextInt(100_000)}" }
}

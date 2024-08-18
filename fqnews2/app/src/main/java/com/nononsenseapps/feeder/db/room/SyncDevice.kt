package com.nononsenseapps.feeder.db.room

import androidx.room.ColumnInfo
import androidx.room.Entity
import androidx.room.ForeignKey
import androidx.room.Ignore
import androidx.room.Index
import androidx.room.PrimaryKey
import com.nononsenseapps.feeder.db.COL_ID
import com.nononsenseapps.feeder.db.SYNC_DEVICE_TABLE_NAME

@Entity(
    tableName = SYNC_DEVICE_TABLE_NAME,
    indices = [
        Index(value = ["sync_remote", "device_id"], unique = true),
        Index(value = ["sync_remote"]),
    ],
    foreignKeys = [
        ForeignKey(
            entity = SyncRemote::class,
            parentColumns = [COL_ID],
            childColumns = ["sync_remote"],
            onDelete = ForeignKey.CASCADE,
        ),
    ],
)
data class SyncDevice
    @Ignore
    constructor(
        @PrimaryKey(autoGenerate = true)
        @ColumnInfo(name = COL_ID)
        var id: Long = ID_UNSET,
        @ColumnInfo(name = "sync_remote") var syncRemote: Long = ID_UNSET,
        @ColumnInfo(name = "device_id") var deviceId: Long = ID_UNSET,
        @ColumnInfo(name = "device_name") var deviceName: String = "",
    ) {
        constructor() : this(id = ID_UNSET)
    }

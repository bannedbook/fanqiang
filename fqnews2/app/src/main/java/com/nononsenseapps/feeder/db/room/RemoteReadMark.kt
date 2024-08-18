package com.nononsenseapps.feeder.db.room

import androidx.room.ColumnInfo
import androidx.room.Entity
import androidx.room.ForeignKey
import androidx.room.Ignore
import androidx.room.Index
import androidx.room.PrimaryKey
import com.nononsenseapps.feeder.db.COL_FEEDURL
import com.nononsenseapps.feeder.db.COL_GUID
import com.nononsenseapps.feeder.db.COL_ID
import java.net.URL
import java.time.Instant

@Entity(
    tableName = "remote_read_mark",
    indices = [
        Index(value = ["sync_remote", COL_FEEDURL, COL_GUID], unique = true),
        Index(value = [COL_FEEDURL, COL_GUID]),
        Index(value = ["sync_remote"]),
        Index(value = ["timestamp"]),
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
data class RemoteReadMark
    @Ignore
    constructor(
        @PrimaryKey(autoGenerate = true)
        @ColumnInfo(name = COL_ID)
        var id: Long = ID_UNSET,
        @ColumnInfo(name = "sync_remote") var sync_remote: Long = ID_UNSET,
        @ColumnInfo(name = COL_FEEDURL) var feedUrl: URL = URL("http://"),
        @ColumnInfo(name = COL_GUID) var guid: String = "",
        @ColumnInfo(
            name = "timestamp",
            typeAffinity = ColumnInfo.INTEGER,
        ) var timestamp: Instant = Instant.EPOCH,
    ) {
        @Suppress("unused")
        constructor() : this(id = ID_UNSET)
    }

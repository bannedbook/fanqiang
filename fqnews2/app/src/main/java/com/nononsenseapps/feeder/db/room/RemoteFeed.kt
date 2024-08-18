package com.nononsenseapps.feeder.db.room

import androidx.room.ColumnInfo
import androidx.room.Entity
import androidx.room.ForeignKey
import androidx.room.Ignore
import androidx.room.Index
import androidx.room.PrimaryKey
import com.nononsenseapps.feeder.db.COL_ID
import com.nononsenseapps.feeder.db.COL_URL
import java.net.URL

@Entity(
    tableName = "remote_feed",
    indices = [
        Index(value = ["sync_remote", COL_URL], unique = true),
        Index(value = [COL_URL]),
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
data class RemoteFeed
    @Ignore
    constructor(
        @PrimaryKey(autoGenerate = true)
        @ColumnInfo(name = COL_ID)
        var id: Long = ID_UNSET,
        @ColumnInfo(name = "sync_remote") var syncRemote: Long = ID_UNSET,
        @ColumnInfo(name = COL_URL) var url: URL = URL("http://"),
    ) {
        constructor() : this(id = ID_UNSET)
    }

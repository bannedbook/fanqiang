package com.nononsenseapps.feeder.db.room

import androidx.room.ColumnInfo
import androidx.room.Entity
import androidx.room.ForeignKey
import androidx.room.Ignore
import androidx.room.Index
import androidx.room.PrimaryKey
import com.nononsenseapps.feeder.db.COL_ID
import java.net.URL

@Entity(
    tableName = "read_status_synced",
    indices = [
        Index(value = ["feed_item", "sync_remote"], unique = true),
        Index(value = ["feed_item"]),
        Index(value = ["sync_remote"]),
    ],
    foreignKeys = [
        ForeignKey(
            entity = FeedItem::class,
            parentColumns = [COL_ID],
            childColumns = ["feed_item"],
            onDelete = ForeignKey.CASCADE,
        ),
        ForeignKey(
            entity = SyncRemote::class,
            parentColumns = [COL_ID],
            childColumns = ["sync_remote"],
            onDelete = ForeignKey.CASCADE,
        ),
    ],
)
data class ReadStatusSynced
    @Ignore
    constructor(
        @PrimaryKey(autoGenerate = true)
        @ColumnInfo(name = COL_ID)
        var id: Long = ID_UNSET,
        @ColumnInfo(name = "sync_remote") var sync_remote: Long = ID_UNSET,
        @ColumnInfo(name = "feed_item") var feed_item: Long = ID_UNSET,
    ) {
        constructor() : this(id = ID_UNSET)
    }

interface ReadStatusFeedItem {
    val id: Long
    val feedId: Long
    val guid: String
    val feedUrl: URL
}

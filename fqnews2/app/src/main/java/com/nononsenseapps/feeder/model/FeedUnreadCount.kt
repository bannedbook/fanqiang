package com.nononsenseapps.feeder.model

import androidx.room.ColumnInfo
import androidx.room.Ignore
import com.nononsenseapps.feeder.db.room.ID_UNSET
import com.nononsenseapps.feeder.ui.compose.navdrawer.FeedIdTag
import java.net.URL

data class FeedUnreadCount
    @Ignore
    constructor(
        override var id: Long = ID_UNSET,
        @ColumnInfo(name = "display_title")
        var displayTitle: String = "",
        override var tag: String = "",
        @ColumnInfo(name = "image_url") var imageUrl: URL? = null,
        @ColumnInfo(name = "unread_count") var unreadCount: Int = 0,
        var expanded: Boolean = false,
    ) : FeedIdTag {
        constructor() : this(id = ID_UNSET)
    }

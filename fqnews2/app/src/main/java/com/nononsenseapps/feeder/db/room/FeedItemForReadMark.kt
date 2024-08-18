package com.nononsenseapps.feeder.db.room

import androidx.room.ColumnInfo
import androidx.room.Ignore
import com.nononsenseapps.feeder.db.COL_FEEDID
import com.nononsenseapps.feeder.db.COL_FEEDURL
import com.nononsenseapps.feeder.db.COL_GUID
import com.nononsenseapps.feeder.db.COL_ID
import java.net.URL

data class FeedItemForReadMark
    @Ignore
    constructor(
        @ColumnInfo(name = COL_ID) override var id: Long = ID_UNSET,
        @ColumnInfo(name = COL_FEEDID) override var feedId: Long = ID_UNSET,
        @ColumnInfo(name = COL_GUID) override var guid: String = "",
        @ColumnInfo(name = COL_FEEDURL) override var feedUrl: URL = URL("http://"),
    ) : ReadStatusFeedItem {
        constructor() : this(id = ID_UNSET)
    }

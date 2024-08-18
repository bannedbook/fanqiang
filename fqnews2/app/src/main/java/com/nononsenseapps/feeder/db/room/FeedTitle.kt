package com.nononsenseapps.feeder.db.room

import androidx.room.ColumnInfo
import androidx.room.Ignore
import com.nononsenseapps.feeder.db.COL_CUSTOM_TITLE
import com.nononsenseapps.feeder.db.COL_ID
import com.nononsenseapps.feeder.db.COL_TITLE

data class FeedTitle
    @Ignore
    constructor(
        @ColumnInfo(name = COL_ID) var id: Long = ID_UNSET,
        @ColumnInfo(name = COL_TITLE) var title: String = "",
        @ColumnInfo(name = COL_CUSTOM_TITLE) var customTitle: String = "",
    ) {
        constructor() : this(id = ID_UNSET)

        val displayTitle: String
            get() = (customTitle.ifBlank { title })
    }

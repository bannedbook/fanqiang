package com.nononsenseapps.feeder.db.room

import androidx.room.ColumnInfo
import androidx.room.Entity
import androidx.room.Ignore
import androidx.room.Index
import androidx.room.PrimaryKey
import com.nononsenseapps.feeder.db.COL_ALTERNATE_ID
import com.nononsenseapps.feeder.db.COL_CURRENTLY_SYNCING
import com.nononsenseapps.feeder.db.COL_CUSTOM_TITLE
import com.nononsenseapps.feeder.db.COL_FULLTEXT_BY_DEFAULT
import com.nononsenseapps.feeder.db.COL_ID
import com.nononsenseapps.feeder.db.COL_IMAGEURL
import com.nononsenseapps.feeder.db.COL_LASTSYNC
import com.nononsenseapps.feeder.db.COL_NOTIFY
import com.nononsenseapps.feeder.db.COL_OPEN_ARTICLES_WITH
import com.nononsenseapps.feeder.db.COL_RESPONSEHASH
import com.nononsenseapps.feeder.db.COL_RETRY_AFTER
import com.nononsenseapps.feeder.db.COL_SITE_FETCHED
import com.nononsenseapps.feeder.db.COL_SKIP_DUPLICATES
import com.nononsenseapps.feeder.db.COL_TAG
import com.nononsenseapps.feeder.db.COL_TITLE
import com.nononsenseapps.feeder.db.COL_URL
import com.nononsenseapps.feeder.db.COL_WHEN_MODIFIED
import com.nononsenseapps.feeder.db.FEEDS_TABLE_NAME
import java.net.URL
import java.time.Instant

const val OPEN_ARTICLE_WITH_APPLICATION_DEFAULT = ""

@Entity(
    tableName = FEEDS_TABLE_NAME,
    indices = [
        Index(value = [COL_URL], unique = true),
        Index(value = [COL_ID, COL_URL, COL_TITLE], unique = true),
    ],
)
data class Feed
    @Ignore
    constructor(
        @PrimaryKey(autoGenerate = true)
        @ColumnInfo(name = COL_ID)
        var id: Long = ID_UNSET,
        @ColumnInfo(name = COL_TITLE) var title: String = "",
        @ColumnInfo(name = COL_CUSTOM_TITLE) var customTitle: String = "",
        @ColumnInfo(name = COL_URL) var url: URL = URL("http://"),
        @ColumnInfo(name = COL_TAG) var tag: String = "",
        @ColumnInfo(name = COL_NOTIFY) var notify: Boolean = false,
        @ColumnInfo(name = COL_IMAGEURL) var imageUrl: URL? = null,
        @ColumnInfo(name = COL_LASTSYNC, typeAffinity = ColumnInfo.INTEGER) var lastSync: Instant = Instant.EPOCH,
        @ColumnInfo(name = COL_RESPONSEHASH) var responseHash: Int = 0,
        @ColumnInfo(name = COL_FULLTEXT_BY_DEFAULT) var fullTextByDefault: Boolean = false,
        @ColumnInfo(name = COL_OPEN_ARTICLES_WITH) var openArticlesWith: String = OPEN_ARTICLE_WITH_APPLICATION_DEFAULT,
        @ColumnInfo(name = COL_ALTERNATE_ID) var alternateId: Boolean = false,
        @ColumnInfo(name = COL_CURRENTLY_SYNCING) var currentlySyncing: Boolean = false,
        // Only update this field when user modifies the feed
        @ColumnInfo(name = COL_WHEN_MODIFIED) var whenModified: Instant = Instant.EPOCH,
        @ColumnInfo(name = COL_SITE_FETCHED) var siteFetched: Instant = Instant.EPOCH,
        @ColumnInfo(name = COL_SKIP_DUPLICATES) var skipDuplicates: Boolean = false,
        // Time when feed is allowed to be synced again earliest, based on retry-after response header
        @ColumnInfo(name = COL_RETRY_AFTER) var retryAfter: Instant = Instant.EPOCH,
    ) {
        constructor() : this(id = ID_UNSET)

        val displayTitle: String
            get() = (customTitle.ifBlank { title })
    }

package com.nononsenseapps.feeder.db.room

import androidx.room.ColumnInfo
import androidx.room.Ignore
import com.nononsenseapps.feeder.db.COL_AUTHOR
import com.nononsenseapps.feeder.db.COL_BOOKMARKED
import com.nononsenseapps.feeder.db.COL_CUSTOM_TITLE
import com.nononsenseapps.feeder.db.COL_ENCLOSURELINK
import com.nononsenseapps.feeder.db.COL_ENCLOSURE_TYPE
import com.nononsenseapps.feeder.db.COL_FEEDCUSTOMTITLE
import com.nononsenseapps.feeder.db.COL_FEEDID
import com.nononsenseapps.feeder.db.COL_FEEDTITLE
import com.nononsenseapps.feeder.db.COL_FEEDURL
import com.nononsenseapps.feeder.db.COL_FULLTEXT_BY_DEFAULT
import com.nononsenseapps.feeder.db.COL_GUID
import com.nononsenseapps.feeder.db.COL_ID
import com.nononsenseapps.feeder.db.COL_IMAGEURL
import com.nononsenseapps.feeder.db.COL_LINK
import com.nononsenseapps.feeder.db.COL_PLAINSNIPPET
import com.nononsenseapps.feeder.db.COL_PLAINTITLE
import com.nononsenseapps.feeder.db.COL_PUBDATE
import com.nononsenseapps.feeder.db.COL_READ_TIME
import com.nononsenseapps.feeder.db.COL_TAG
import com.nononsenseapps.feeder.db.COL_TITLE
import com.nononsenseapps.feeder.db.COL_URL
import com.nononsenseapps.feeder.db.COL_WORD_COUNT
import com.nononsenseapps.feeder.db.COL_WORD_COUNT_FULL
import com.nononsenseapps.feeder.db.FEEDS_TABLE_NAME
import com.nononsenseapps.feeder.db.FEED_ITEMS_TABLE_NAME
import com.nononsenseapps.feeder.model.ThumbnailImage
import com.nononsenseapps.feeder.model.host
import com.nononsenseapps.feeder.util.sloppyLinkToStrictURLNoThrows
import java.net.URI
import java.net.URL
import java.time.Instant
import java.time.ZonedDateTime

const val FEED_ITEM_COLUMNS_WITH_FEED = """
    $FEED_ITEMS_TABLE_NAME.$COL_ID AS $COL_ID, $COL_GUID, $FEED_ITEMS_TABLE_NAME.$COL_TITLE AS $COL_TITLE,
    $COL_PLAINTITLE, $COL_PLAINSNIPPET, $FEED_ITEMS_TABLE_NAME.$COL_IMAGEURL, $COL_ENCLOSURELINK, $COL_ENCLOSURE_TYPE,
    $COL_AUTHOR, $COL_PUBDATE, $COL_LINK, $COL_READ_TIME, $FEEDS_TABLE_NAME.$COL_TAG AS $COL_TAG, $FEEDS_TABLE_NAME.$COL_ID AS $COL_FEEDID,
    $FEEDS_TABLE_NAME.$COL_TITLE AS $COL_FEEDTITLE,
    $FEEDS_TABLE_NAME.$COL_CUSTOM_TITLE AS $COL_FEEDCUSTOMTITLE,
    $FEEDS_TABLE_NAME.$COL_URL AS $COL_FEEDURL,
    $FEEDS_TABLE_NAME.$COL_FULLTEXT_BY_DEFAULT AS $COL_FULLTEXT_BY_DEFAULT,
    $COL_BOOKMARKED,
    $COL_WORD_COUNT,
    $COL_WORD_COUNT_FULL
"""

data class FeedItemWithFeed
    @Ignore
    constructor(
        override var id: Long = ID_UNSET,
        var guid: String = "",
        @Deprecated("This is never different from plainTitle", replaceWith = ReplaceWith("plainTitle"))
        var title: String = "",
        @ColumnInfo(name = COL_PLAINTITLE) var plainTitle: String = "",
        @ColumnInfo(name = COL_PLAINSNIPPET) var plainSnippet: String = "",
        @ColumnInfo(name = COL_IMAGEURL) var thumbnailImage: ThumbnailImage? = null,
        @ColumnInfo(name = COL_ENCLOSURELINK) var enclosureLink: String? = null,
        @ColumnInfo(name = COL_ENCLOSURE_TYPE) var enclosureType: String? = null,
        var author: String? = null,
        @ColumnInfo(name = COL_PUBDATE) var pubDate: ZonedDateTime? = null,
        override var link: String? = null,
        var tag: String = "",
        @ColumnInfo(name = COL_READ_TIME) var readTime: Instant? = null,
        @ColumnInfo(name = COL_FEEDID) var feedId: Long? = null,
        @ColumnInfo(name = COL_FEEDTITLE) var feedTitle: String = "",
        @ColumnInfo(name = COL_FEEDCUSTOMTITLE) var feedCustomTitle: String = "",
        @ColumnInfo(name = COL_FEEDURL) var feedUrl: URL = sloppyLinkToStrictURLNoThrows(""),
        @ColumnInfo(name = COL_FULLTEXT_BY_DEFAULT) var fullTextByDefault: Boolean = false,
        @ColumnInfo(name = COL_BOOKMARKED) var bookmarked: Boolean = false,
        @ColumnInfo(name = COL_WORD_COUNT) var wordCount: Int = 0,
        @ColumnInfo(name = COL_WORD_COUNT_FULL) var wordCountFull: Int = 0,
    ) : FeedItemForFetching {
        constructor() : this(id = ID_UNSET)

        val feedDisplayTitle: String
            get() = feedCustomTitle.ifBlank { feedTitle }

        val enclosureFilename: String?
            get() {
                enclosureLink?.let { enclosureLink ->
                    var fname: String? = null
                    try {
                        fname = URI(enclosureLink).path.split("/").last()
                    } catch (_: Exception) {
                    }
                    return if (fname.isNullOrEmpty()) {
                        null
                    } else {
                        fname
                    }
                }
                return null
            }

        val domain: String?
            get() {
                return (enclosureLink ?: link)?.host()
            }
    }

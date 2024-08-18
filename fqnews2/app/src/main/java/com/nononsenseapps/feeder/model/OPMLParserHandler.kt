package com.nononsenseapps.feeder.model

import com.nononsenseapps.feeder.db.room.Feed

interface OPMLParserHandler {
    suspend fun saveFeed(feed: Feed)

    suspend fun saveSetting(
        key: String,
        value: String,
    )

    suspend fun saveBlocklistPatterns(patterns: Iterable<String>)
}

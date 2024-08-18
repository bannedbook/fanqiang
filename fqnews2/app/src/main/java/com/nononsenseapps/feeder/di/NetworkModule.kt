package com.nononsenseapps.feeder.di

import com.nononsenseapps.feeder.model.FeedParser
import com.nononsenseapps.feeder.model.FullTextParser
import com.nononsenseapps.feeder.model.RssLocalSync
import com.nononsenseapps.feeder.sync.SyncRestClient
import com.nononsenseapps.jsonfeed.Feed
import com.nononsenseapps.jsonfeed.JsonFeedParser
import com.nononsenseapps.jsonfeed.feedAdapter
import com.squareup.moshi.JsonAdapter
import okhttp3.OkHttpClient
import org.kodein.di.DI
import org.kodein.di.bind
import org.kodein.di.instance
import org.kodein.di.provider
import org.kodein.di.singleton

val networkModule =
    DI.Module(name = "network") {
        // Parsers can carry state so safer to use providers
        bind<JsonAdapter<Feed>>() with provider { feedAdapter() }
        bind<JsonFeedParser>() with provider { JsonFeedParser(instance<OkHttpClient>(), instance()) }
        bind<FeedParser>() with provider { FeedParser(di) }
        // These don't have state issues
        bind<SyncRestClient>() with singleton { SyncRestClient(di) }
        bind<RssLocalSync>() with singleton { RssLocalSync(di) }
        bind<FullTextParser>() with singleton { FullTextParser(di) }
    }

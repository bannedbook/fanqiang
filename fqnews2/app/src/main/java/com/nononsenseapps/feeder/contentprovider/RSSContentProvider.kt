package com.nononsenseapps.feeder.contentprovider

import android.content.ContentProvider
import android.content.ContentValues
import android.content.UriMatcher
import android.database.Cursor
import android.net.Uri
import com.nononsenseapps.feeder.BuildConfig
import jww.app.FeederApplication
import com.nononsenseapps.feeder.db.room.FeedDao
import com.nononsenseapps.feeder.db.room.FeedItemDao
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.instance

class RSSContentProvider : ContentProvider(), DIAware {
    override val di: DI by lazy {
        val application = context!!.applicationContext as FeederApplication
        application.di
    }

    private val feedDao: FeedDao by instance()
    private val feedItemDao: FeedItemDao by instance()

    private val uriMatcher =
        UriMatcher(UriMatcher.NO_MATCH).apply {
            addURI(AUTHORITY, RssContentProviderContract.FEEDS_URI_PATH_LIST, URI_FEED_LIST)
            addURI(
                AUTHORITY,
                RssContentProviderContract.ARTICLES_URI_PATH_LIST,
                URI_ARTICLE_LIST,
            )
            addURI(
                AUTHORITY,
                RssContentProviderContract.ARTICLES_URI_PATH_ITEM,
                URI_ARTICLE_IN_FEED_LIST,
            )
        }

    override fun onCreate(): Boolean {
        return true
    }

    override fun delete(
        uri: Uri,
        selection: String?,
        selectionArgs: Array<out String>?,
    ): Int {
        throw UnsupportedOperationException("Not implemented")
    }

    override fun getType(uri: Uri): String? =
        when (uriMatcher.match(uri)) {
            URI_FEED_LIST -> RssContentProviderContract.FEEDS_MIME_TYPE_LIST
            URI_ARTICLE_LIST -> RssContentProviderContract.ARTICLES_MIME_TYPE_LIST
            URI_ARTICLE_IN_FEED_LIST -> RssContentProviderContract.ARTICLES_MIME_TYPE_ITEM
            else -> null
        }

    override fun insert(
        uri: Uri,
        values: ContentValues?,
    ): Uri? {
        throw UnsupportedOperationException("Not implemented")
    }

    override fun update(
        uri: Uri,
        values: ContentValues?,
        selection: String?,
        selectionArgs: Array<out String>?,
    ): Int {
        throw UnsupportedOperationException("Not implemented")
    }

    override fun query(
        uri: Uri,
        projection: Array<out String>?,
        selection: String?,
        selectionArgs: Array<out String>?,
        sortOrder: String?,
    ): Cursor? {
        return when (uriMatcher.match(uri)) {
            URI_FEED_LIST -> feedDao.loadFeedsForContentProvider()
            URI_ARTICLE_LIST -> feedItemDao.loadFeedItemsForContentProvider()
            URI_ARTICLE_IN_FEED_LIST -> feedItemDao.loadFeedItemsInFeedForContentProvider(uri.lastPathSegment!!.toLong())
            else -> null
        }
    }

    companion object {
        private const val AUTHORITY = "${BuildConfig.APPLICATION_ID}.rssprovider"
        private const val URI_FEED_LIST = 1
        private const val URI_ARTICLE_LIST = 2
        private const val URI_ARTICLE_IN_FEED_LIST = 3
    }
}

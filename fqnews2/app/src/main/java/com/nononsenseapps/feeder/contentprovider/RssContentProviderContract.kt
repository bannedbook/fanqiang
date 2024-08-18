package com.nononsenseapps.feeder.contentprovider

object RssContentProviderContract {
    const val FEEDS_MIME_TYPE_LIST = "vnd.android.cursor.dir/vnd.rssprovider.feeds"
    const val FEEDS_URI_PATH_LIST = "feeds"

    /**
     * Columns available via the content provider
     */
    @Suppress("unused")
    val feedsColumns =
        listOf(
            "id",
            "title",
        )

    const val ARTICLES_MIME_TYPE_LIST = "vnd.android.cursor.dir/vnd.rssprovider.items"
    const val ARTICLES_URI_PATH_LIST = "articles"
    const val ARTICLES_MIME_TYPE_ITEM = "vnd.android.cursor.item/vnd.rssprovider.item"
    const val ARTICLES_URI_PATH_ITEM = "articles/#"

    /**
     * Columns available via the content provider
     */
    @Suppress("unused")
    val articlesColumns =
        listOf(
            "id",
            "title",
            "text",
        )
}

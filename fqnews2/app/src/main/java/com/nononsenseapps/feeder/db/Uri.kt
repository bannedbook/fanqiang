@file:Suppress("RECEIVER_NULLABILITY_MISMATCH_BASED_ON_JAVA_ANNOTATIONS")

package com.nononsenseapps.feeder.db

import android.net.Uri

const val AUTHORITY = "com.nononsenseapps.feeder.provider"
const val SCHEME = "content://"

// URIs
// Feed
@JvmField
val URI_FEEDS: Uri = Uri.withAppendedPath(Uri.parse(SCHEME + AUTHORITY), "feeds")

// Feed item
@JvmField
val URI_FEEDITEMS: Uri = Uri.withAppendedPath(Uri.parse(SCHEME + AUTHORITY), "feed_items")

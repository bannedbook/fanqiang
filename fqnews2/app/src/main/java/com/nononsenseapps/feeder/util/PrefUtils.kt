package com.nononsenseapps.feeder.util

import android.content.SharedPreferences

/**
 * Database settings
 */
const val PREF_MAX_ITEM_COUNT_PER_FEED = "pref_max_item_count_per_feed"

fun SharedPreferences.getStringNonNull(
    key: String,
    defaultValue: String,
): String = getString(key, defaultValue) ?: defaultValue

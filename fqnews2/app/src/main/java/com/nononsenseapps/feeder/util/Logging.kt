package com.nononsenseapps.feeder.util

import android.util.Log
import com.nononsenseapps.feeder.BuildConfig

fun logDebug(
    tag: String,
    msg: String,
    exception: Throwable? = null,
) {
    if (BuildConfig.DEBUG) {
        Log.d(tag, msg, exception)
    }
}

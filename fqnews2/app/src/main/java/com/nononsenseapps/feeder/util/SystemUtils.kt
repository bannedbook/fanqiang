package com.nononsenseapps.feeder.util

import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.net.ConnectivityManager
import android.os.BatteryManager
import android.os.Build
import java.net.URLEncoder

/**
 * Note that cellular typically is metered - it is NOT the same as hotspot
 */
private fun currentlyMetered(context: Context): Boolean {
    val connManager = context.getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager?
    return connManager?.isActiveNetworkMetered ?: false
}

fun currentlyUnmetered(context: Context): Boolean = !currentlyMetered(context)

fun currentlyCharging(context: Context): Boolean {
    return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
        val batteryManager = context.getSystemService(Context.BATTERY_SERVICE) as BatteryManager?
        batteryManager?.isCharging ?: false
    } else {
        // Sticky intent
        val ifilter = IntentFilter(Intent.ACTION_BATTERY_CHANGED)
        val batteryStatus = context.registerReceiver(null, ifilter)
        val status = batteryStatus!!.getIntExtra(BatteryManager.EXTRA_STATUS, -1)
        status == BatteryManager.BATTERY_STATUS_CHARGING || status == BatteryManager.BATTERY_STATUS_FULL
    }
}

fun currentlyConnected(context: Context): Boolean {
    val connManager = context.getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager?

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
        val net = connManager?.activeNetwork

        @Suppress("DEPRECATION")
        val netInfo = connManager?.getNetworkInfo(net)
        @Suppress("DEPRECATION")
        return netInfo != null && netInfo.isConnected
    }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
        @Suppress("DEPRECATION")
        return connManager?.allNetworks?.map { connManager.getNetworkInfo(it)?.isConnected }
            ?.fold(false) { result, connected ->
                result || (connected ?: false)
            } ?: false
    }
    @Suppress("DEPRECATION")
    return connManager?.allNetworkInfo?.map { it?.isConnected }
        ?.fold(false) { result, connected ->
            result || (connected ?: false)
        } ?: false
}

fun String.urlEncode(): String = URLEncoder.encode(this, "UTF-8")

package com.nononsenseapps.feeder.model.workmanager

import android.annotation.TargetApi
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import android.content.pm.ServiceInfo
import android.os.Build
import androidx.annotation.RequiresApi
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import androidx.work.ForegroundInfo
import com.nononsenseapps.feeder.R

private const val SYNC_NOTIFICATION_ID = 42623
private const val SYNC_CHANNEL_ID = "feederSyncNotifications"
private const val SYNC_NOTIFICATION_GROUP = "com.nononsenseapps.feeder.SYNC"

/**
 * This is safe to call multiple times
 */
@TargetApi(Build.VERSION_CODES.O)
@RequiresApi(Build.VERSION_CODES.O)
private fun createNotificationChannel(
    context: Context,
    notificationManager: NotificationManagerCompat,
) {
    val name = context.getString(R.string.sync_status)
    val description = context.getString(R.string.sync_status)

    val channel =
        NotificationChannel(SYNC_CHANNEL_ID, name, NotificationManager.IMPORTANCE_LOW)
    channel.description = description

    notificationManager.createNotificationChannel(channel)
}

/**
 * Necessary for expedited work.
 * Pre Android 12 they will run as foreground services, but on Android 12+ they will run as expedited Jobs.
 */
fun createForegroundInfo(
    context: Context,
    notificationManager: NotificationManagerCompat,
): ForegroundInfo {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        createNotificationChannel(context, notificationManager)
    }

    val syncingText = context.getString(R.string.syncing)

    val notification =
        NotificationCompat.Builder(context.applicationContext, SYNC_CHANNEL_ID)
            .setContentTitle(syncingText)
            .setTicker(syncingText)
            .setGroup(SYNC_NOTIFICATION_GROUP)
            .setSmallIcon(R.drawable.ic_stat_sync)
            .setOngoing(true)
            .build()

    return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
        ForegroundInfo(
            SYNC_NOTIFICATION_ID,
            notification,
            ServiceInfo.FOREGROUND_SERVICE_TYPE_DATA_SYNC,
        )
    } else {
        ForegroundInfo(SYNC_NOTIFICATION_ID, notification)
    }
}

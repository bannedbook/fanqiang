package com.nononsenseapps.feeder.model

import android.Manifest
import android.annotation.TargetApi
import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.PendingIntent.FLAG_IMMUTABLE
import android.content.Context
import android.content.Context.NOTIFICATION_SERVICE
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.BitmapFactory
import android.net.Uri
import android.os.Build
import android.os.TransactionTooLargeException
import android.provider.Browser.EXTRA_CREATE_NEW_TAB
import android.util.Log
import androidx.annotation.RequiresApi
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationCompat.GROUP_ALERT_SUMMARY
import androidx.core.app.NotificationManagerCompat
import androidx.core.content.ContextCompat
import androidx.core.net.toUri
import androidx.navigation.NavDeepLinkBuilder
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.archmodel.ItemOpener
import com.nononsenseapps.feeder.archmodel.Repository
import com.nononsenseapps.feeder.db.COL_LINK
import com.nononsenseapps.feeder.db.URI_FEEDITEMS
import com.nononsenseapps.feeder.db.room.FeedDao
import com.nononsenseapps.feeder.db.room.FeedItemDao
import com.nononsenseapps.feeder.db.room.FeedItemWithFeed
import com.nononsenseapps.feeder.db.room.ID_ALL_FEEDS
import com.nononsenseapps.feeder.db.room.ID_UNSET
import com.nononsenseapps.feeder.ui.EXTRA_FEEDITEMS_TO_MARK_AS_NOTIFIED
import com.nononsenseapps.feeder.ui.MainActivity
import com.nononsenseapps.feeder.ui.OpenLinkInDefaultActivity
import com.nononsenseapps.feeder.util.DEEP_LINK_BASE_URI
import com.nononsenseapps.feeder.util.notificationManager
import com.nononsenseapps.feeder.util.urlEncode
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.kodein.di.DI
import org.kodein.di.android.closestDI
import org.kodein.di.instance

const val SUMMARY_NOTIFICATION_ID = 2_147_483_646
private const val CHANNEL_ID = "feederNotifications"
private const val ARTICLE_NOTIFICATION_GROUP = "com.nononsenseapps.feeder.ARTICLE"

private const val LOG_TAG = "FEEDER_NOTIFY"

suspend fun notify(
    appContext: Context,
    updateSummaryOnly: Boolean = false,
) = withContext(Dispatchers.Default) {
    if (ContextCompat.checkSelfPermission(
            appContext,
            Manifest.permission.POST_NOTIFICATIONS,
        ) != PackageManager.PERMISSION_GRANTED
    ) {
        return@withContext
    }

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        createNotificationChannel(appContext)
    }

    val di by closestDI(appContext)

    val nm: NotificationManagerCompat by di.instance()

    // If too many it can cause a crash, so it's limited to 20
    val feedItems = getItemsToNotify(di)

    try {
        if (feedItems.isNotEmpty()) {
            if (!updateSummaryOnly) {
                feedItems.map {
                    it.id.toInt() to singleNotification(appContext, it)
                }.forEach { (id, notification) ->
                    nm.notify(id, notification)
                }
            }
            // Shown on API Level < 24
            nm.notify(SUMMARY_NOTIFICATION_ID, inboxNotification(appContext, feedItems))
        }
    } catch (e: TransactionTooLargeException) {
        // This can happen if there are too many notifications
        Log.e(LOG_TAG, "Too many notifications", e)
    }
}

suspend fun cancelNotification(
    context: Context,
    feedItemId: Long,
) = cancelNotifications(context, listOf(feedItemId))

suspend fun cancelNotifications(
    context: Context,
    feedItemIds: List<Long>,
) = withContext(Dispatchers.Default) {
    if (ContextCompat.checkSelfPermission(
            context,
            Manifest.permission.POST_NOTIFICATIONS,
        ) != PackageManager.PERMISSION_GRANTED
    ) {
        return@withContext
    }

    val nm = context.notificationManager

    for (feedItemId in feedItemIds) {
        nm.cancel(feedItemId.toInt())
    }

    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.N) {
        notify(context)
    }
}

/**
 * This is an update operation if channel already exists so it's safe to call multiple times
 */
@TargetApi(Build.VERSION_CODES.O)
@RequiresApi(Build.VERSION_CODES.O)
private fun createNotificationChannel(context: Context) {
    val name = context.getString(R.string.notification_channel_name)
    val description = context.getString(R.string.notification_channel_description)

    val notificationManager: NotificationManager =
        context.getSystemService(NOTIFICATION_SERVICE) as NotificationManager

    val channel = NotificationChannel(CHANNEL_ID, name, NotificationManager.IMPORTANCE_LOW)
    channel.description = description

    notificationManager.createNotificationChannel(channel)
}

private suspend fun singleNotification(
    context: Context,
    item: FeedItemWithFeed,
): Notification {
    val di by closestDI(context)
    val repository: Repository by di.instance()

    val style = NotificationCompat.BigTextStyle()
    val title = item.plainTitle
    val text = item.feedDisplayTitle

    style.bigText(text)
    style.setBigContentTitle(title)

    val contentIntent =
        Intent(
            Intent.ACTION_VIEW,
            "$DEEP_LINK_BASE_URI/article/${item.id}".toUri(),
            context,
            MainActivity::class.java,
        ).apply {
            addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        }

    val pendingIntent =
        PendingIntent.getActivity(
            context,
            item.id.toInt(),
            contentIntent,
            PendingIntent.FLAG_UPDATE_CURRENT or FLAG_IMMUTABLE,
        )

    val builder = notificationBuilder(context)

    builder.setContentText(text)
        .setContentTitle(title)
        .setGroup(ARTICLE_NOTIFICATION_GROUP)
        .setGroupAlertBehavior(GROUP_ALERT_SUMMARY)
        .setDeleteIntent(getPendingDeleteIntent(context, item))
        .setSilent(true)
        .setNumber(1)

    // Note that notifications must use PNG resources, because there is no compatibility for vector drawables here

    if (repository.getArticleOpener(item.id) == ItemOpener.DEFAULT_BROWSER && item.link != null) {
        builder.setContentIntent(
            PendingIntent.getActivity(
                context,
                item.id.toInt(),
                getOpenInDefaultActivityIntent(context, item.id, item.link),
                PendingIntent.FLAG_UPDATE_CURRENT or FLAG_IMMUTABLE,
            ),
        )
    } else {
        builder.setContentIntent(pendingIntent)
    }

    item.enclosureLink?.let { enclosureLink ->
        val intent = Intent(Intent.ACTION_VIEW, Uri.parse(enclosureLink))
        intent.putExtra(EXTRA_CREATE_NEW_TAB, true)
        builder.addAction(
            R.drawable.notification_play_circle_outline,
            context.getString(R.string.open_enclosed_media),
            PendingIntent.getActivity(
                context,
                item.id.toInt(),
                getOpenInDefaultActivityIntent(context, item.id, enclosureLink),
                PendingIntent.FLAG_UPDATE_CURRENT or FLAG_IMMUTABLE,
            ),
        )
    }

    if (repository.getArticleOpener(item.id) != ItemOpener.DEFAULT_BROWSER) {
        item.link?.let { link ->
            builder.addAction(
                R.drawable.notification_open_in_browser,
                context.getString(R.string.open_link_in_browser),
                PendingIntent.getActivity(
                    context,
                    item.id.toInt(),
                    getOpenInDefaultActivityIntent(context, item.id, link),
                    PendingIntent.FLAG_UPDATE_CURRENT or FLAG_IMMUTABLE,
                ),
            )
        }
    }

    builder.addAction(
        R.drawable.notification_check,
        context.getString(R.string.mark_as_read),
        PendingIntent.getBroadcast(
            context,
            item.id.toInt(),
            getMarkAsReadIntent(context, item.id),
            PendingIntent.FLAG_UPDATE_CURRENT or FLAG_IMMUTABLE,
        ),
    )

    style.setBuilder(builder)
    return style.build() ?: error("Null??")
}

internal fun getOpenInDefaultActivityIntent(
    context: Context,
    feedItemId: Long,
    link: String? = null,
): Intent =
    Intent(
        Intent.ACTION_VIEW,
        // Important to keep the URI different so PendingIntents don't collide
        URI_FEEDITEMS.buildUpon().appendPath("$feedItemId").also {
            if (link != null) {
                it.appendQueryParameter(COL_LINK, link)
            }
        }.build(),
        context,
        OpenLinkInDefaultActivity::class.java,
    )

internal fun getMarkAsReadIntent(
    context: Context,
    feedItemId: Long,
): Intent =
    Intent(
        ACTION_MARK_AS_READ,
        // Important to keep the URI different so PendingIntents don't collide
        URI_FEEDITEMS.buildUpon().appendPath("$feedItemId").build(),
        context,
        RssNotificationBroadcastReceiver::class.java,
    )

/**
 * Use this on platforms older than 24 to bundle notifications together
 */
private fun inboxNotification(
    context: Context,
    feedItems: List<FeedItemWithFeed>,
): Notification {
    val style = NotificationCompat.InboxStyle()
    val title = context.getString(R.string.updated_feeds)
    val text = feedItems.map { it.feedDisplayTitle }.toSet().joinToString(separator = ", ")

    style.setBigContentTitle(title)
    feedItems.forEach {
        style.addLine("${it.feedDisplayTitle} \u2014 ${it.plainTitle}")
    }

    // We can be a little bit smart - if all items are from the same feed then go to that feed
    // Otherwise we should go to All feeds
    val feedTags = feedItems.map { it.tag }.toSet()

    val deepLinkTag =
        if (feedTags.size == 1) {
            feedTags.first()
        } else {
            ""
        }

    val feedIds = feedItems.map { it.feedId }.toSet()

    val deepLinkId =
        if (feedIds.size == 1) {
            feedIds.first() ?: ID_UNSET
        } else {
            if (deepLinkTag.isNotEmpty()) {
                ID_ALL_FEEDS // Tag will take precedence
            } else {
                ID_UNSET // Will default to last open when tag is empty too
            }
        }

    val deepLinkUri =
        "$DEEP_LINK_BASE_URI/feed?id=$deepLinkId&tag=${deepLinkTag.urlEncode()}"

    val contentIntent =
        Intent(
            Intent.ACTION_VIEW,
            deepLinkUri.toUri(),
            context,
            // Proxy activity to mark as read
            OpenLinkInDefaultActivity::class.java,
        ).apply {
            putExtra(
                EXTRA_FEEDITEMS_TO_MARK_AS_NOTIFIED,
                LongArray(feedItems.size) { i -> feedItems[i].id },
            )
            addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        }

    val pendingIntent =
        PendingIntent.getActivity(
            context,
            SUMMARY_NOTIFICATION_ID,
            contentIntent,
            PendingIntent.FLAG_UPDATE_CURRENT or FLAG_IMMUTABLE,
        )

    val builder = notificationBuilder(context)

    builder.setContentText(text)
        .setContentTitle(title)
        .setContentIntent(pendingIntent)
        .setGroup(ARTICLE_NOTIFICATION_GROUP)
        .setGroupAlertBehavior(GROUP_ALERT_SUMMARY)
        .setGroupSummary(true)
        .setDeleteIntent(getDeleteIntent(context, feedItems))
        .setNumber(feedItems.size)

    style.setBuilder(builder)
    return style.build() ?: error("How null??")
}

private fun getDeleteIntent(
    context: Context,
    feedItems: List<FeedItemWithFeed>,
): PendingIntent {
    val intent = Intent(context, RssNotificationBroadcastReceiver::class.java)
    intent.action = ACTION_MARK_AS_NOTIFIED

    val ids = LongArray(feedItems.size) { i -> feedItems[i].id }
    intent.putExtra(EXTRA_FEEDITEM_ID_ARRAY, ids)

    return PendingIntent.getBroadcast(
        context,
        0,
        intent,
        PendingIntent.FLAG_UPDATE_CURRENT or FLAG_IMMUTABLE,
    )
}

internal fun getDeleteIntent(
    context: Context,
    feedItem: FeedItemWithFeed,
): Intent {
    val intent = Intent(context, RssNotificationBroadcastReceiver::class.java)
    intent.action = ACTION_MARK_AS_NOTIFIED
    intent.data = Uri.withAppendedPath(URI_FEEDITEMS, "${feedItem.id}")
    val ids: LongArray = longArrayOf(feedItem.id)
    intent.putExtra(EXTRA_FEEDITEM_ID_ARRAY, ids)

    return intent
}

private fun getPendingDeleteIntent(
    context: Context,
    feedItem: FeedItemWithFeed,
): PendingIntent =
    PendingIntent.getBroadcast(
        context,
        0,
        getDeleteIntent(context, feedItem),
        PendingIntent.FLAG_UPDATE_CURRENT or FLAG_IMMUTABLE,
    )

private fun notificationBuilder(context: Context): NotificationCompat.Builder {
    val bm = BitmapFactory.decodeResource(context.resources, R.mipmap.ic_launcher)

    return NotificationCompat.Builder(context, CHANNEL_ID)
        .setSmallIcon(R.drawable.ic_stat_f)
        .setLargeIcon(bm)
        .setAutoCancel(true)
        .setCategory(NotificationCompat.CATEGORY_SOCIAL)
        .setPriority(NotificationCompat.PRIORITY_LOW)
}

private suspend fun getItemsToNotify(di: DI): List<FeedItemWithFeed> {
    val feedDao: FeedDao by di.instance()
    val feedItemDao: FeedItemDao by di.instance()

    val feeds = feedDao.loadFeedIdsToNotify()

    return when (feeds.isEmpty()) {
        true -> emptyList()
        false -> feedItemDao.loadItemsToNotify(feeds)
    }
}

fun NavDeepLinkBuilder.createPendingIntent(requestCode: Int): PendingIntent? = this.createTaskStackBuilder().getPendingIntent(requestCode, PendingIntent.FLAG_UPDATE_CURRENT)

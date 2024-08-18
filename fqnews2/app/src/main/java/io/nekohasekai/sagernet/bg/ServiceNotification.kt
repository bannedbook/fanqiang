package io.nekohasekai.sagernet.bg

import android.app.PendingIntent
import android.app.Service
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.ServiceInfo

import android.os.Build

import android.text.format.Formatter
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import io.nekohasekai.sagernet.Action
import com.nononsenseapps.feeder.R
import io.nekohasekai.sagernet.aidl.SpeedDisplayData
import io.nekohasekai.sagernet.database.DataStore
import io.nekohasekai.sagernet.database.ProxyEntity
import io.nekohasekai.sagernet.database.SagerDatabase
import io.nekohasekai.sagernet.ktx.app
import io.nekohasekai.sagernet.ktx.runOnMainDispatcher
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock

/**
 * User can customize visibility of notification since Android 8.
 * The default visibility:
 *
 * Android 8.x: always visible due to system limitations
 * VPN:         always invisible because of VPN notification/icon
 * Other:       always visible
 *
 * See also: https://github.com/aosp-mirror/platform_frameworks_base/commit/070d142993403cc2c42eca808ff3fafcee220ac4
 */
class ServiceNotification(
    private val service: BaseService.Interface, title: String,
    channel: String, visible: Boolean = false,
) : BroadcastReceiver() {
    companion object {
        const val notificationId = 1
        val flags =
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) PendingIntent.FLAG_IMMUTABLE else 0

        fun genTitle(ent: ProxyEntity): String {
            val gn = if (DataStore.showGroupInNotification)
                SagerDatabase.groupDao.getById(ent.groupId)?.displayName() else null
            return if (gn == null) ent.displayName() else "[$gn] ${ent.displayName()}"
        }
    }

    var listenPostSpeed = true

    suspend fun postNotificationSpeedUpdate(stats: SpeedDisplayData) {
        useBuilder {
            if (showDirectSpeed) {
                val speedDetail = (service as Context).getString(
                    R.string.speed_detail, service.getString(
                        R.string.speed, Formatter.formatFileSize(service, stats.txRateProxy)
                    ), service.getString(
                        R.string.speed, Formatter.formatFileSize(service, stats.rxRateProxy)
                    ), service.getString(
                        R.string.speed,
                        Formatter.formatFileSize(service, stats.txRateDirect)
                    ), service.getString(
                        R.string.speed,
                        Formatter.formatFileSize(service, stats.rxRateDirect)
                    )
                )
                it.setStyle(NotificationCompat.BigTextStyle().bigText(speedDetail))
                it.setContentText(speedDetail)
            } else {
                val speedSimple = (service as Context).getString(
                    R.string.traffic, service.getString(
                        R.string.speed, Formatter.formatFileSize(service, stats.txRateProxy)
                    ), service.getString(
                        R.string.speed, Formatter.formatFileSize(service, stats.rxRateProxy)
                    )
                )
                it.setContentText(speedSimple)
            }
            it.setSubText(
                service.getString(
                    R.string.traffic,
                    Formatter.formatFileSize(service, stats.txTotal),
                    Formatter.formatFileSize(service, stats.rxTotal)
                )
            )
        }
        update()
    }

    suspend fun postNotificationTitle(newTitle: String) {
        useBuilder {
            it.setContentTitle(newTitle)
        }
        update()
    }

    suspend fun postNotificationWakeLockStatus(acquired: Boolean) {
        updateActions()
        useBuilder {
            it.priority =
                if (acquired) NotificationCompat.PRIORITY_HIGH else NotificationCompat.PRIORITY_LOW
        }
        update()
    }

    private val showDirectSpeed = DataStore.showDirectSpeed

    private val builder = NotificationCompat.Builder(service as Context, channel)
        .setWhen(0)
        .setTicker(service.getString(R.string.forward_success))
        .setContentTitle(title)
        .setOnlyAlertOnce(true)
        .setContentIntent(app.configureIntent(service))
        .setSmallIcon(R.drawable.ic_service_active)
        .setCategory(NotificationCompat.CATEGORY_SERVICE)
        .setPriority(if (visible) NotificationCompat.PRIORITY_LOW else NotificationCompat.PRIORITY_MIN)

    private val buildLock = Mutex()

    private suspend fun useBuilder(f: (NotificationCompat.Builder) -> Unit) {
        buildLock.withLock {
            f(builder)
        }
    }

    init {
        service as Context
        builder.color = service.getColor(R.color.primary)

        service.registerReceiver(this, IntentFilter().apply {
            addAction(Intent.ACTION_SCREEN_ON)
            addAction(Intent.ACTION_SCREEN_OFF)
        })

        runOnMainDispatcher {
            updateActions()
            show()
        }
    }

    private suspend fun updateActions() {
        service as Context
        useBuilder {
            it.clearActions()

            val closeAction = NotificationCompat.Action.Builder(
                0, service.getText(R.string.stop), PendingIntent.getBroadcast(
                    service, 0, Intent(Action.CLOSE).setPackage(service.packageName), flags
                )
            ).setShowsUserInterface(false).build()
            it.addAction(closeAction)
        }
    }

    override fun onReceive(context: Context, intent: Intent) {
        if (service.data.state == BaseService.State.Connected) {
            listenPostSpeed = intent.action == Intent.ACTION_SCREEN_ON
        }
    }


    private suspend fun show() =
        useBuilder {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
                (service as Service).startForeground(notificationId, it.build(), ServiceInfo.FOREGROUND_SERVICE_TYPE_DATA_SYNC)
            }
            else{
                (service as Service).startForeground(notificationId, it.build())
            }
        }

    private suspend fun update() = useBuilder {
        NotificationManagerCompat.from(service as Service).notify(notificationId, it.build())
    }

    fun destroy() {
        listenPostSpeed = false
        (service as Service).stopForeground(true)
        service.unregisterReceiver(this)
    }
}

package io.nekohasekai.sagernet.bg

import android.annotation.SuppressLint
import android.app.Service
import android.content.Intent
import android.os.PowerManager
import jww.app.FeederApplication

class ProxyService : Service(), BaseService.Interface {
    override val data = BaseService.Data(this)
    override val tag: String get() = "SagerNetProxyService"
    override fun createNotification(profileName: String): ServiceNotification =
        ServiceNotification(this, "节点[$profileName]正在为您同步新闻...", "service-proxy", true)

    override var wakeLock: PowerManager.WakeLock? = null
    override var upstreamInterfaceName: String? = null

    @SuppressLint("WakelockTimeout")
    override fun acquireWakeLock() {
        wakeLock = FeederApplication.instance.power.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "sagernet:proxy")
            .apply { acquire() }
    }

    override fun onBind(intent: Intent) = super.onBind(intent)
    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int =
        super<BaseService.Interface>.onStartCommand(intent, flags, startId)
}

package io.nekohasekai.sagernet.bg

import android.graphics.drawable.Icon
import android.service.quicksettings.Tile
import androidx.annotation.RequiresApi
import com.nononsenseapps.feeder.R
import jww.app.FeederApplication
import io.nekohasekai.sagernet.aidl.ISagerNetService
import io.nekohasekai.sagernet.database.SagerDatabase
import android.service.quicksettings.TileService as BaseTileService

@RequiresApi(24)
class TileService : BaseTileService(), SagerConnection.Callback {
    private val iconIdle by lazy { Icon.createWithResource(this, R.drawable.ic_service_idle) }
    private val iconBusy by lazy { Icon.createWithResource(this, R.drawable.ic_service_busy) }
    private val iconConnected by lazy {
        Icon.createWithResource(this, R.drawable.ic_service_active)
    }
    private var tapPending = false

    private val connection = SagerConnection(SagerConnection.CONNECTION_ID_TILE)
    override fun stateChanged(state: BaseService.State, profileName: String?, msg: String?) =
        updateTile(state, profileName)

    override fun onServiceConnected(service: ISagerNetService) {
        updateTile(BaseService.State.values()[service.state], service.profileName)
        if (tapPending) {
            tapPending = false
            onClick()
        }
    }

    override fun cbSelectorUpdate(id: Long) {
        val profile = SagerDatabase.proxyDao.getById(id) ?: return
        updateTile(BaseService.State.Connected, profile.displayName())
    }

    override fun onStartListening() {
        super.onStartListening()
        connection.connect(this, this)
    }

    override fun onStopListening() {
        connection.disconnect(this)
        super.onStopListening()
    }

    override fun onClick() {
        if (isLocked) unlockAndRun(this::toggle) else toggle()
    }

    private fun updateTile(serviceState: BaseService.State, profileName: String?) {
        qsTile?.apply {
            label = null
            when (serviceState) {
                BaseService.State.Idle -> error("serviceState")
                BaseService.State.Connecting -> {
                    icon = iconBusy
                    state = Tile.STATE_ACTIVE
                }

                BaseService.State.Connected -> {
                    icon = iconConnected
                    label = profileName
                    state = Tile.STATE_ACTIVE
                }

                BaseService.State.Stopping -> {
                    icon = iconBusy
                    state = Tile.STATE_UNAVAILABLE
                }

                BaseService.State.Stopped -> {
                    icon = iconIdle
                    state = Tile.STATE_INACTIVE
                }
            }
            label = label ?: getString(R.string.app_name)
            updateTile()
        }
    }

    private fun toggle() {
        val service = connection.service
        if (service == null) tapPending =
            true else BaseService.State.values()[service.state].let { state ->
            when {
                state.canStop -> FeederApplication.instance.stopService()
                state == BaseService.State.Stopped -> FeederApplication.instance.startService()
            }
        }
    }
}

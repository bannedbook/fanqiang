package com.nononsenseapps.feeder.ui.compose.sync

import android.app.Application
import android.util.Log
import android.widget.Toast
import androidx.compose.runtime.Immutable
import androidx.lifecycle.SavedStateHandle
import androidx.lifecycle.viewModelScope
import com.nononsenseapps.feeder.ApplicationCoroutineScope
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.archmodel.Repository
import com.nononsenseapps.feeder.base.DIAwareViewModel
import com.nononsenseapps.feeder.db.room.SyncDevice
import com.nononsenseapps.feeder.db.room.SyncRemote
import com.nononsenseapps.feeder.model.workmanager.requestFeedSync
import com.nononsenseapps.feeder.util.DEEP_LINK_BASE_URI
import com.nononsenseapps.feeder.util.logDebug
import com.nononsenseapps.feeder.util.urlEncode
import kotlinx.coroutines.async
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import org.kodein.di.DI
import org.kodein.di.instance
import java.net.URL

class SyncScreenViewModel(di: DI, private val state: SavedStateHandle) : DIAwareViewModel(di) {
    private val context: Application by instance()
    private val repository: Repository by instance()

    private val applicationCoroutineScope: ApplicationCoroutineScope by instance()

    @Suppress("ktlint:standard:property-naming")
    private val _syncCode: MutableStateFlow<String> =
        MutableStateFlow(
            state["syncCode"] ?: "",
        )

    @Suppress("ktlint:standard:property-naming")
    private val _secretKey: MutableStateFlow<String> =
        MutableStateFlow(
            state["secretKey"] ?: "",
        )

    @Suppress("ktlint:standard:property-naming")
    private val _screenToShow: MutableStateFlow<SyncScreenToShow> =
        MutableStateFlow(
            state["syncScreen"] ?: SyncScreenToShow.SETUP,
        )

    init {
        if (_syncCode.value.isNotBlank() || _secretKey.value.isNotBlank()) {
            if (!state.contains("syncScreen")) {
                setScreen(SyncScreenToShow.JOIN)
            }
        }
    }

    fun setSyncCode(value: String) {
        val possibleUrlCode = value.syncCodeQueryParam

        val syncCode =
            if (possibleUrlCode.length == 64) {
                possibleUrlCode
            } else {
                value
            }

        state["syncCode"] = syncCode
        _syncCode.update { syncCode }
    }

    fun setSecretKey(value: String) {
        val secretKey = value.secretKeyQueryParam

        state["secretKey"] = secretKey
        _secretKey.update { secretKey }
    }

    fun setScreen(value: SyncScreenToShow) {
        state["syncScreen"] = value
        _screenToShow.update { value }
    }

    fun updateDeviceList() {
        applicationCoroutineScope.launch {
            logDebug(tag = LOG_TAG, "Update Devices")
            repository.updateDeviceList()
                .onLeft {
                    Log.e(LOG_TAG, "updateDeviceList: ${it.code}: ${it.body}", it.throwable)
                }
        }
    }

    fun joinSyncChain(
        syncCode: String,
        secretKey: String,
    ) {
        logDebug(tag = LOG_TAG, "Joining sync chain")
        viewModelScope.launch {
            try {
                applicationCoroutineScope.async {
                    repository.joinSyncChain(syncCode = syncCode, secretKey = secretKey)
                }.await()
                    .onRight {
                        requestFeedSync(di)
                        joinedWithSyncCode(syncCode = syncCode, secretKey = secretKey)
                    }
                    .onLeft {
                        Log.e(LOG_TAG, "joinSyncChain: ${it.code}, ${it.body}", it.throwable)
                    }
            } catch (e: Exception) {
                Log.e(LOG_TAG, "Error when joining sync chain", e)
            }
        }
    }

    fun leaveSyncChain() {
        applicationCoroutineScope.launch {
            repository.leaveSyncChain()
            setSyncCode("")
            setSecretKey("")
            setScreen(SyncScreenToShow.SETUP)
        }
    }

    fun removeDevice(deviceId: Long) {
        applicationCoroutineScope.launch {
            try {
                repository.removeDevice(deviceId = deviceId)
            } catch (e: Exception) {
                Log.e(LOG_TAG, "Error when removing device", e)
            }
        }
    }

    private fun joinedWithSyncCode(
        syncCode: String,
        secretKey: String,
    ) {
        setSyncCode(syncCode)
        setSecretKey(secretKey)
        setScreen(SyncScreenToShow.ADD_DEVICE)
    }

    fun startNewSyncChain() {
        applicationCoroutineScope.launch {
            try {
                repository.startNewSyncChain()
                    .onRight { (syncCode, secretKey) ->
                        joinedWithSyncCode(syncCode = syncCode, secretKey = secretKey)
                    }
                    .onLeft {
                        Log.e(LOG_TAG, "startNewChain: ${it.body}", it.throwable)
                    }
            } catch (e: Exception) {
                Log.e(LOG_TAG, "Error when starting new sync chain", e)
            }
        }
    }

    fun onMissingBarCodeScanner() {
        Toast.makeText(context, R.string.no_barcode_scanner_installed, Toast.LENGTH_SHORT).show()
    }

    private val _viewState = MutableStateFlow(SyncScreenViewState())
    val viewState: StateFlow<SyncScreenViewState>
        get() = _viewState.asStateFlow()

    init {
        viewModelScope.launch {
            repository.getSyncRemote().let { syncRemote ->
                if (!state.contains("syncCode")) {
                    setSyncCode(syncRemote.syncChainId)
                }
            }

            combine(
                _syncCode,
                repository.getSyncRemoteFlow(),
                _screenToShow,
                repository.getDevices(),
                _secretKey,
            ) { params ->
                val syncCode = params[0] as String
                val syncRemote = params[1] as SyncRemote?
                val screen = params[2] as SyncScreenToShow
                val secretKey = params[4] as String

                @Suppress("UNCHECKED_CAST")
                val deviceList = params[3] as List<SyncDevice>

                val actualScreen =
                    if (syncRemote?.syncChainId?.length == 64) {
                        when (screen) {
                            // Setup and join only possible if nothing setup already
                            SyncScreenToShow.SETUP,
                            SyncScreenToShow.JOIN,
                            -> SyncScreenToShow.DEVICELIST

                            SyncScreenToShow.DEVICELIST,
                            SyncScreenToShow.ADD_DEVICE,
                            -> screen
                        }
                    } else {
                        when (screen) {
                            SyncScreenToShow.SETUP,
                            SyncScreenToShow.JOIN,
                            -> screen

                            SyncScreenToShow.DEVICELIST,
                            SyncScreenToShow.ADD_DEVICE,
                            -> SyncScreenToShow.SETUP
                        }
                    }

                Log.v(
                    LOG_TAG,
                    "Showing $actualScreen, remoteCode: ${syncRemote?.syncChainId?.take(10)}",
                )

                val remoteKeyEncoded = (syncRemote?.secretKey ?: "").urlEncode()

                SyncScreenViewState(
                    syncCode = syncCode,
                    addNewDeviceUrl = URL("$DEEP_LINK_BASE_URI/sync/join?sync_code=${syncRemote?.syncChainId ?: ""}&key=$remoteKeyEncoded"),
                    singleScreenToShow = actualScreen,
                    deviceId = syncRemote?.deviceId ?: 0,
                    deviceList = deviceList,
                    secretKey = secretKey,
                )
            }.collect {
                _viewState.value = it
            }
        }
    }

    companion object {
        private const val LOG_TAG = "FEEDER_SYNCVMODEL"
    }
}

@Immutable
data class SyncScreenViewState(
    val syncCode: String = "",
    val secretKey: String = "",
    val addNewDeviceUrl: URL = URL("https://"),
    val singleScreenToShow: SyncScreenToShow = SyncScreenToShow.SETUP,
    val deviceId: Long = 0,
    val deviceList: List<SyncDevice> = emptyList(),
) {
    val leftScreenToShow: LeftScreenToShow
        get() =
            when (singleScreenToShow) {
                SyncScreenToShow.SETUP, SyncScreenToShow.JOIN -> LeftScreenToShow.SETUP
                SyncScreenToShow.DEVICELIST, SyncScreenToShow.ADD_DEVICE -> LeftScreenToShow.DEVICELIST
            }

    val rightScreenToShow: RightScreenToShow
        get() =
            when (singleScreenToShow) {
                SyncScreenToShow.SETUP, SyncScreenToShow.JOIN -> RightScreenToShow.JOIN
                SyncScreenToShow.DEVICELIST, SyncScreenToShow.ADD_DEVICE -> RightScreenToShow.ADD_DEVICE
            }
}

enum class SyncScreenToShow {
    SETUP,
    DEVICELIST,
    ADD_DEVICE,
    JOIN,
}

enum class LeftScreenToShow {
    SETUP,
    DEVICELIST,
}

enum class RightScreenToShow {
    ADD_DEVICE,
    JOIN,
}

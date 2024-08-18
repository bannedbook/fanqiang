package com.nononsenseapps.feeder.ui

import android.app.Application
import androidx.lifecycle.viewModelScope
import com.nononsenseapps.feeder.archmodel.Repository
import com.nononsenseapps.feeder.base.DIAwareViewModel
import com.nononsenseapps.feeder.util.currentlyCharging
import com.nononsenseapps.feeder.util.currentlyConnected
import com.nononsenseapps.feeder.util.currentlyUnmetered
import kotlinx.coroutines.launch
import org.kodein.di.DI
import org.kodein.di.instance
import java.time.Instant

class MainActivityViewModel(di: DI) : DIAwareViewModel(di) {
    private val repository: Repository by instance()
    private val context: Application by instance()

    fun setResumeTime() {
        repository.setResumeTime(Instant.now())
    }

    val shouldSyncOnResume: Boolean =
        repository.syncOnResume.value

    fun ensurePeriodicSyncConfigured() =
        viewModelScope.launch {
            repository.ensurePeriodicSyncConfigured()
        }

    fun isOkToSyncAutomatically(): Boolean =
        currentlyConnected(context) &&
            (!repository.syncOnlyWhenCharging.value || currentlyCharging(context)) &&
            (!repository.syncOnlyOnWifi.value || currentlyUnmetered(context))
}

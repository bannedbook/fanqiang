package com.nononsenseapps.feeder.ui

import androidx.lifecycle.viewModelScope
import com.nononsenseapps.feeder.archmodel.Repository
import com.nononsenseapps.feeder.base.DIAwareViewModel
import kotlinx.coroutines.launch
import org.kodein.di.DI
import org.kodein.di.instance

class OpenLinkInDefaultActivityViewModel(di: DI) : DIAwareViewModel(di) {
    private val repository: Repository by instance()

    fun markAsNotifiedInBackground(itemIds: List<Long>) {
        viewModelScope.launch {
            repository.markAsNotified(itemIds)
        }
    }

    suspend fun markAsReadAndNotified(itemId: Long) {
        repository.markAsReadAndNotified(itemId)
    }
}

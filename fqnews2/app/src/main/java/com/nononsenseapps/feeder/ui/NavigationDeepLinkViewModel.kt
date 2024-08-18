package com.nononsenseapps.feeder.ui

import androidx.lifecycle.viewModelScope
import com.nononsenseapps.feeder.archmodel.Repository
import com.nononsenseapps.feeder.base.DIAwareViewModel
import kotlinx.coroutines.launch
import org.kodein.di.DI
import org.kodein.di.instance

class NavigationDeepLinkViewModel(di: DI) : DIAwareViewModel(di) {
    private val repository: Repository by instance()

    fun setCurrentFeedAndTag(
        feedId: Long,
        tag: String,
    ) {
        repository.setCurrentFeedAndTag(feedId, tag)
        // Should open feed in portrait
        repository.setIsArticleOpen(false)
    }

    fun setCurrentArticle(itemId: Long) {
        viewModelScope.launch {
            repository.markAsReadAndNotified(itemId)
        }
        repository.setCurrentArticle(itemId)
        // Should open article in portrait
        repository.setIsArticleOpen(true)
    }
}

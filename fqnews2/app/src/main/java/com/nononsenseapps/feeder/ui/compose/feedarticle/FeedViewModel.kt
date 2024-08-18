package com.nononsenseapps.feeder.ui.compose.feedarticle

import androidx.compose.runtime.Immutable
import androidx.lifecycle.SavedStateHandle
import androidx.lifecycle.viewModelScope
import androidx.paging.PagingData
import androidx.paging.cachedIn
import com.nononsenseapps.feeder.ApplicationCoroutineScope
import com.nononsenseapps.feeder.archmodel.FeedItemStyle
import com.nononsenseapps.feeder.archmodel.FeedType
import com.nononsenseapps.feeder.archmodel.ItemOpener
import com.nononsenseapps.feeder.archmodel.Repository
import com.nononsenseapps.feeder.archmodel.ScreenTitle
import com.nononsenseapps.feeder.archmodel.SwipeAsRead
import com.nononsenseapps.feeder.archmodel.ThemeOptions
import com.nononsenseapps.feeder.base.DIAwareViewModel
import com.nononsenseapps.feeder.blob.blobFullInputStream
import com.nononsenseapps.feeder.blob.blobInputStream
import com.nononsenseapps.feeder.db.room.FeedItemCursor
import com.nononsenseapps.feeder.db.room.FeedTitle
import com.nononsenseapps.feeder.db.room.ID_UNSET
import com.nononsenseapps.feeder.model.FeedUnreadCount
import com.nononsenseapps.feeder.model.LocaleOverride
import com.nononsenseapps.feeder.model.PlaybackStatus
import com.nononsenseapps.feeder.model.TTSStateHolder
import com.nononsenseapps.feeder.model.workmanager.requestFeedSync
import com.nononsenseapps.feeder.ui.compose.feed.FeedListItem
import com.nononsenseapps.feeder.ui.compose.feed.FeedOrTag
import com.nononsenseapps.feeder.ui.compose.text.htmlToAnnotatedString
import com.nononsenseapps.feeder.util.Either
import com.nononsenseapps.feeder.util.FilePathProvider
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import org.kodein.di.DI
import org.kodein.di.instance
import java.io.FileNotFoundException
import java.util.Locale

class FeedViewModel(
    di: DI,
    private val state: SavedStateHandle,
) : DIAwareViewModel(di), FeedListFilterCallback {
    private val repository: Repository by instance()
    private val ttsStateHolder: TTSStateHolder by instance()
    private val filePathProvider: FilePathProvider by instance()

    // Use this for actions which should complete even if app goes off screen
    private val applicationCoroutineScope: ApplicationCoroutineScope by instance()

    val currentFeedListItems: Flow<PagingData<FeedListItem>> =
        repository.getCurrentFeedListItems()
            .cachedIn(viewModelScope)

    val pagedNavDrawerItems: Flow<PagingData<FeedUnreadCount>> =
        repository.getPagedNavDrawerItems()
            .cachedIn(viewModelScope)

    private val visibleFeedItemCount: StateFlow<Int> =
        repository.getCurrentFeedListVisibleItemCount()
            .stateIn(
                viewModelScope,
                SharingStarted.Eagerly,
                // So we display an empty screen before data is loaded (less flicker)
                1,
            )

    private val screenTitleForCurrentFeedOrTag: StateFlow<ScreenTitle> =
        repository.getScreenTitleForCurrentFeedOrTag()
            .stateIn(
                viewModelScope,
                SharingStarted.Eagerly,
                ScreenTitle("", FeedType.ALL_FEEDS, 0),
            )

    private val visibleFeeds: StateFlow<List<FeedTitle>> =
        repository.getCurrentlyVisibleFeedTitles()
            .stateIn(
                viewModelScope,
                SharingStarted.Eagerly,
                emptyList(),
            )

    fun deleteFeeds(feedIds: List<Long>) =
        applicationCoroutineScope.launch {
            repository.deleteFeeds(feedIds)
        }

    fun markAllAsRead() =
        applicationCoroutineScope.launch {
            val (feedId, feedTag) = repository.currentFeedAndTag.value
            repository.markAllAsReadInFeedOrTag(feedId, feedTag)
        }

    fun markAsUnread(itemId: Long) =
        applicationCoroutineScope.launch {
            repository.markAsUnread(itemId)
        }

    fun markAsRead(
        itemId: Long,
        feedOrTag: FeedOrTag?,
    ) = applicationCoroutineScope.launch {
        val (feedId, tag) = repository.currentFeedAndTag.value
        // Ensure mark as read on scroll doesn't fire when navigating between feeds
        if (feedOrTag == null || feedId == feedOrTag.id && tag == feedOrTag.tag) {
            repository.markAsReadAndNotified(itemId)
        }
    }

    fun markAsReadOnSwipe(itemId: Long) =
        applicationCoroutineScope.launch {
            repository.markAsReadAndNotified(itemId = itemId, readTimeBeforeMinReadTime = true)
        }

    fun markBeforeAsRead(cursor: FeedItemCursor) =
        applicationCoroutineScope.launch {
            val (feedId, feedTag) = repository.currentFeedAndTag.value
            repository.markBeforeAsRead(cursor, feedId, feedTag)
        }

    fun markAfterAsRead(cursor: FeedItemCursor) =
        applicationCoroutineScope.launch {
            val (feedId, feedTag) = repository.currentFeedAndTag.value
            repository.markAfterAsRead(cursor, feedId, feedTag)
        }

    fun setBookmarked(
        itemId: Long,
        bookmarked: Boolean,
    ) = applicationCoroutineScope.launch {
        repository.setBookmarked(itemId, bookmarked)
    }

    fun requestImmediateSyncOfCurrentFeedOrTag() {
        val (feedId, feedTag) = repository.currentFeedAndTag.value
        requestFeedSync(
            di = di,
            feedId = feedId,
            feedTag = feedTag,
            forceNetwork = true,
        )
    }

    fun requestImmediateSyncOfAll() {
        requestFeedSync(
            di = di,
            forceNetwork = true,
        )
    }

    private val toolbarVisible: MutableStateFlow<Boolean> =
        MutableStateFlow(state["toolbarMenuVisible"] ?: false)

    fun setToolbarMenuVisible(visible: Boolean) {
        state["toolbarMenuVisible"] = visible
        toolbarVisible.update { visible }
    }

    private val filterMenuVisible: MutableStateFlow<Boolean> =
        MutableStateFlow(state["filterMenuVisible"] ?: false)

    fun setFilterMenuVisible(visible: Boolean) {
        state["filterMenuVisible"] = visible
        filterMenuVisible.update { visible }
    }

    val filterCallback: FeedListFilterCallback = this

    fun toggleTagExpansion(tag: String) = repository.toggleTagExpansion(tag)

    private val editDialogVisible = MutableStateFlow(false)

    fun setShowEditDialog(visible: Boolean) {
        editDialogVisible.update { visible }
    }

    private val deleteDialogVisible = MutableStateFlow(false)

    fun setShowDeleteDialog(visible: Boolean) {
        deleteDialogVisible.update { visible }
    }

    private fun setCurrentArticle(itemId: Long) {
        repository.setCurrentArticle(itemId)
        repository.setIsArticleOpen(true)
    }

    fun openArticle(
        itemId: Long,
        openInCustomTab: (String) -> Unit,
        openInBrowser: (String) -> Unit,
        navigateToArticle: () -> Unit,
    ) = viewModelScope.launch {
        val itemOpener = repository.getArticleOpener(itemId = itemId)
        val articleLink = repository.getLink(itemId)
        when {
            ItemOpener.CUSTOM_TAB == itemOpener && articleLink != null -> {
                openInCustomTab(articleLink)
            }

            ItemOpener.DEFAULT_BROWSER == itemOpener && articleLink != null -> {
                openInBrowser(articleLink)
            }

            else -> {
                setCurrentArticle(itemId)
                navigateToArticle()
            }
        }
        markAsRead(itemId, null)
    }

    val viewState: StateFlow<FeedScreenViewState> =
        combine(
            // 0
            repository.showFab,
            repository.showThumbnails,
            repository.currentTheme,
            repository.currentlySyncing,
            repository.feedItemStyle,
            // 5
            repository.expandedTags,
            toolbarVisible,
            visibleFeedItemCount,
            screenTitleForCurrentFeedOrTag,
            editDialogVisible,
            // 10
            deleteDialogVisible,
            visibleFeeds,
            repository.isArticleOpen,
            repository.currentFeedAndTag.map { (feedId, tag) -> FeedOrTag(feedId, tag) },
            ttsStateHolder.ttsState,
            // 15
            repository.swipeAsRead,
            ttsStateHolder.availableLanguages,
            repository.isMarkAsReadOnScroll,
            repository.maxLines,
            filterMenuVisible,
            // 20
            repository.feedListFilter,
            repository.showOnlyTitle,
            repository.showReadingTime,
            repository.showTitleUnreadCount,
            repository.syncWorkerRunning,
        ) { params: Array<Any> ->
            val haveVisibleFeedItems = (params[7] as Int) > 0
            val currentFeedOrTag = params[13] as FeedOrTag
            val ttsState = params[14] as PlaybackStatus

            @Suppress("UNCHECKED_CAST")
            FeedState(
                showFab = haveVisibleFeedItems && (params[0] as Boolean),
                showThumbnails = params[1] as Boolean,
                currentTheme = params[2] as ThemeOptions,
                currentlySyncing = (params[24] as Boolean) && (params[3] as Boolean),
                feedItemStyle = params[4] as FeedItemStyle,
                expandedTags = params[5] as Set<String>,
                showToolbarMenu = params[6] as Boolean,
                // 7
                haveVisibleFeedItems = haveVisibleFeedItems,
                feedScreenTitle = params[8] as ScreenTitle,
                showEditDialog = params[9] as Boolean,
                showDeleteDialog = params[10] as Boolean,
                visibleFeeds = params[11] as List<FeedTitle>,
                isArticleOpen = params[12] as Boolean,
                // 13
                currentFeedOrTag = currentFeedOrTag,
                // 14
                isTTSPlaying = ttsState == PlaybackStatus.PLAYING,
                // 14
                isBottomBarVisible = ttsState != PlaybackStatus.STOPPED,
                swipeAsRead = params[15] as SwipeAsRead,
                ttsLanguages = params[16] as List<Locale>,
                markAsReadOnScroll = params[17] as Boolean,
                maxLines = params[18] as Int,
                showFilterMenu = params[19] as Boolean,
                filter = params[20] as FeedListFilter,
                showOnlyTitle = params[21] as Boolean,
                showReadingTime = params[22] as Boolean,
                showTitleUnreadCount = params[23] as Boolean,
            )
        }
            .stateIn(
                viewModelScope,
                SharingStarted.Eagerly,
                FeedState(),
            )

    fun ttsStop() {
        ttsStateHolder.stop()
    }

    fun ttsPause() {
        ttsStateHolder.pause()
    }

    fun ttsSkipNext() {
        ttsStateHolder.skipNext()
    }

    fun ttsOnSelectLanguage(lang: LocaleOverride) {
        ttsStateHolder.setLanguage(lang)
    }

    fun ttsPlay() {
        viewModelScope.launch(Dispatchers.IO) {
            val article =
                repository.getCurrentArticle()
                    ?: return@launch
            val isFullText = repository.shouldDisplayFullTextForItemByDefault(article.id)
            val textToRead =
                when (isFullText) {
                    false ->
                        Either.catching(
                            onCatch = {
                                when (it) {
                                    is FileNotFoundException -> TTSFileNotFound
                                    else -> TTSUnknownError
                                }
                            },
                        ) {
                            blobInputStream(article.id, filePathProvider.articleDir).use {
                                htmlToAnnotatedString(
                                    inputStream = it,
                                    baseUrl = article.feedUrl.toString(),
                                )
                            }
                        }

                    true ->
                        Either.catching(
                            onCatch = {
                                when (it) {
                                    is FileNotFoundException -> TTSFileNotFound
                                    else -> TTSUnknownError
                                }
                            },
                        ) {
                            blobFullInputStream(
                                article.id,
                                filePathProvider.fullArticleDir,
                            ).use {
                                htmlToAnnotatedString(
                                    inputStream = it,
                                    baseUrl = article.feedUrl.toString(),
                                )
                            }
                        }
                }

            // TODO show error some message
            textToRead.onRight {
                ttsStateHolder.tts(
                    textArray = it,
                    useDetectLanguage = repository.useDetectLanguage.value,
                )
            }
        }
    }

    override fun onCleared() {
        super.onCleared()
        ttsStateHolder.shutdown()
    }

    override fun setSaved(value: Boolean) {
        repository.setFeedListFilterSaved(value)
    }

    override fun setRecentlyRead(value: Boolean) {
        repository.setFeedListFilterRecentlyRead(value)
    }

    override fun setRead(value: Boolean) {
        repository.setFeedListFilterRead(value)
    }

    companion object {
        private const val LOG_TAG = "FEEDER_FeedVM"
    }
}

@Immutable
data class FeedState(
    override val currentFeedOrTag: FeedOrTag = FeedOrTag(ID_UNSET, ""),
    override val showFab: Boolean = true,
    override val showThumbnails: Boolean = true,
    override val currentTheme: ThemeOptions = ThemeOptions.SYSTEM,
    override val currentlySyncing: Boolean = false,
    // Defaults to empty string to avoid rendering until loading complete
    override val feedScreenTitle: ScreenTitle = ScreenTitle("", FeedType.FEED, 0),
    override val visibleFeeds: List<FeedTitle> = emptyList(),
    override val feedItemStyle: FeedItemStyle = FeedItemStyle.CARD,
    override val expandedTags: Set<String> = emptySet(),
    override val isBottomBarVisible: Boolean = false,
    override val isTTSPlaying: Boolean = false,
    override val ttsLanguages: List<Locale> = emptyList(),
    override val showToolbarMenu: Boolean = false,
    override val showDeleteDialog: Boolean = false,
    override val showEditDialog: Boolean = false,
    // Defaults to true so empty screen doesn't appear before load
    override val haveVisibleFeedItems: Boolean = true,
    override val swipeAsRead: SwipeAsRead = SwipeAsRead.ONLY_FROM_END,
    override val markAsReadOnScroll: Boolean = false,
    override val maxLines: Int = 2,
    override val showOnlyTitle: Boolean = false,
    override val showReadingTime: Boolean = false,
    override val showFilterMenu: Boolean = false,
    override val filter: FeedListFilter = emptyFeedListFilter,
    val isArticleOpen: Boolean = false,
    override val showTitleUnreadCount: Boolean = false,
) : FeedScreenViewState

@Immutable
interface FeedScreenViewState {
    val currentFeedOrTag: FeedOrTag
    val showFab: Boolean
    val showThumbnails: Boolean
    val currentTheme: ThemeOptions
    val currentlySyncing: Boolean
    val feedScreenTitle: ScreenTitle
    val visibleFeeds: List<FeedTitle>
    val feedItemStyle: FeedItemStyle
    val expandedTags: Set<String>
    val isBottomBarVisible: Boolean
    val isTTSPlaying: Boolean
    val ttsLanguages: List<Locale>
    val showToolbarMenu: Boolean
    val showDeleteDialog: Boolean
    val showEditDialog: Boolean
    val haveVisibleFeedItems: Boolean
    val swipeAsRead: SwipeAsRead
    val markAsReadOnScroll: Boolean
    val maxLines: Int
    val showOnlyTitle: Boolean
    val showReadingTime: Boolean
    val filter: FeedListFilter
    val showFilterMenu: Boolean
    val showTitleUnreadCount: Boolean
}

@Immutable
interface FeedListFilter {
    val unread: Boolean
    val saved: Boolean
    val recentlyRead: Boolean
    val read: Boolean
}

val emptyFeedListFilter =
    object : FeedListFilter {
        override val unread: Boolean = true
        override val saved: Boolean = false
        override val recentlyRead: Boolean = false
        override val read: Boolean = false
    }

@Immutable
interface FeedListFilterCallback {
    fun setSaved(value: Boolean)

    fun setRecentlyRead(value: Boolean)

    fun setRead(value: Boolean)
}

val FeedListFilter.onlyUnread: Boolean
    get() = unread && !saved && !recentlyRead && !read

val FeedListFilter.onlyUnreadAndSaved: Boolean
    get() = unread && saved && !recentlyRead && !read

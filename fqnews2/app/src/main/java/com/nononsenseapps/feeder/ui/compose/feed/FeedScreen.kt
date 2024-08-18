package com.nononsenseapps.feeder.ui.compose.feed

import android.content.Intent
import androidx.activity.compose.BackHandler
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.ExperimentalAnimationApi
import androidx.compose.animation.core.MutableTransitionState
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.scaleIn
import androidx.compose.animation.scaleOut
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.RowScope
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.asPaddingValues
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.navigationBars
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.only
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBars
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyListState
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.lazy.staggeredgrid.LazyStaggeredGridState
import androidx.compose.foundation.lazy.staggeredgrid.LazyVerticalStaggeredGrid
import androidx.compose.foundation.lazy.staggeredgrid.StaggeredGridCells
import androidx.compose.foundation.lazy.staggeredgrid.rememberLazyStaggeredGridState
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.CheckBox
import androidx.compose.material.icons.filled.CheckBoxOutlineBlank
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.DoneAll
import androidx.compose.material.icons.filled.Edit
import androidx.compose.material.icons.filled.Email
import androidx.compose.material.icons.filled.FilterList
import androidx.compose.material.icons.filled.ImportExport
import androidx.compose.material.icons.filled.Menu
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.filled.Refresh
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.FloatingActionButton
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.PlainTooltip
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TooltipBox
import androidx.compose.material3.TooltipDefaults
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.material3.rememberTooltipState
import androidx.compose.material3.rememberTopAppBarState
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.Immutable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.key
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.CollectionInfo
import androidx.compose.ui.semantics.CollectionItemInfo
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.collectionInfo
import androidx.compose.ui.semantics.collectionItemInfo
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.role
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.semantics.testTag
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.navigation.NavController
import androidx.paging.compose.LazyPagingItems
import androidx.paging.compose.collectAsLazyPagingItems
import androidx.paging.compose.itemContentType
import androidx.paging.compose.itemKey
import app.NodeImporter
import com.nononsenseapps.feeder.ApplicationCoroutineScope
import jww.app.FeederApplication
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.archmodel.FeedItemStyle
import com.nononsenseapps.feeder.archmodel.FeedType
import com.nononsenseapps.feeder.db.FAR_FUTURE
import com.nononsenseapps.feeder.db.room.FeedItemCursor
import com.nononsenseapps.feeder.db.room.ID_SAVED_ARTICLES
import com.nononsenseapps.feeder.db.room.ID_UNSET
import com.nononsenseapps.feeder.model.LocaleOverride
import com.nononsenseapps.feeder.model.export.exportSavedArticles
import com.nononsenseapps.feeder.model.opml.exportOpml
import com.nononsenseapps.feeder.model.opml.importOpml
import com.nononsenseapps.feeder.ui.compose.components.safeSemantics
import com.nononsenseapps.feeder.ui.compose.deletefeed.DeletableFeed
import com.nononsenseapps.feeder.ui.compose.deletefeed.DeleteFeedDialog
import com.nononsenseapps.feeder.ui.compose.empty.NothingToRead
import com.nononsenseapps.feeder.ui.compose.feedarticle.FeedListFilterCallback
import com.nononsenseapps.feeder.ui.compose.feedarticle.FeedScreenViewState
import com.nononsenseapps.feeder.ui.compose.feedarticle.FeedViewModel
import com.nononsenseapps.feeder.ui.compose.feedarticle.onlyUnread
import com.nononsenseapps.feeder.ui.compose.feedarticle.onlyUnreadAndSaved
import com.nononsenseapps.feeder.ui.compose.material3.DrawerState
import com.nononsenseapps.feeder.ui.compose.material3.DrawerValue
import com.nononsenseapps.feeder.ui.compose.material3.rememberDrawerState
import com.nononsenseapps.feeder.ui.compose.navdrawer.ScreenWithNavDrawer
import com.nononsenseapps.feeder.ui.compose.navigation.ArticleDestination
import com.nononsenseapps.feeder.ui.compose.navigation.EditFeedDestination
import com.nononsenseapps.feeder.ui.compose.navigation.FeedDestination
import com.nononsenseapps.feeder.ui.compose.navigation.SearchFeedDestination
import com.nononsenseapps.feeder.ui.compose.navigation.SettingsDestination
import com.nononsenseapps.feeder.ui.compose.pullrefresh.PullRefreshIndicator
import com.nononsenseapps.feeder.ui.compose.pullrefresh.pullRefresh
import com.nononsenseapps.feeder.ui.compose.pullrefresh.rememberPullRefreshState
import com.nononsenseapps.feeder.ui.compose.readaloud.HideableTTSPlayer
import com.nononsenseapps.feeder.ui.compose.theme.LocalDimens
import com.nononsenseapps.feeder.ui.compose.theme.SensibleTopAppBar
import com.nononsenseapps.feeder.ui.compose.theme.SetStatusBarColorToMatchScrollableTopAppBar
import com.nononsenseapps.feeder.ui.compose.utils.ImmutableHolder
import com.nononsenseapps.feeder.ui.compose.utils.addMargin
import com.nononsenseapps.feeder.ui.compose.utils.isCompactDevice
import com.nononsenseapps.feeder.ui.compose.utils.onKeyEventLikeEscape
import com.nononsenseapps.feeder.ui.compose.utils.rememberIsItemMostlyVisible
import com.nononsenseapps.feeder.util.ActivityLauncher
import com.nononsenseapps.feeder.util.ToastMaker
import com.nononsenseapps.feeder.util.emailBugReportIntent
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import org.kodein.di.compose.LocalDI
import org.kodein.di.compose.instance
import org.kodein.di.instance
import java.time.Instant
import java.time.LocalDateTime

private const val LOG_TAG = "FEEDER_FEEDSCREEN"

@OptIn(
    ExperimentalMaterial3Api::class,
)
@Composable
fun FeedScreen(
    navController: NavController,
    viewModel: FeedViewModel,
    navDrawerListState: LazyListState,
) {
    val toastMaker: ToastMaker by instance()
    val viewState: FeedScreenViewState by viewModel.viewState.collectAsStateWithLifecycle()
    val pagedFeedItems = viewModel.currentFeedListItems.collectAsLazyPagingItems()
    val pagedNavDrawerItems = viewModel.pagedNavDrawerItems.collectAsLazyPagingItems()

    val di = LocalDI.current
    val savedArticleExporter =
        rememberLauncherForActivityResult(
            ActivityResultContracts.CreateDocument("text/x-opml"),
        ) { uri ->
            if (uri != null) {
                val applicationCoroutineScope: ApplicationCoroutineScope by di.instance()
                applicationCoroutineScope.launch {
                    exportSavedArticles(di, uri)
                }
            }
        }
    val opmlExporter =
        rememberLauncherForActivityResult(
            ActivityResultContracts.CreateDocument("text/x-opml"),
        ) { uri ->
            if (uri != null) {
                val applicationCoroutineScope: ApplicationCoroutineScope by di.instance()
                applicationCoroutineScope.launch {
                    exportOpml(di, uri)
                }
            }
        }
    val opmlImporter =
        rememberLauncherForActivityResult(
            ActivityResultContracts.OpenDocument(),
        ) { uri ->
            if (uri != null) {
                val applicationCoroutineScope: ApplicationCoroutineScope by di.instance()
                applicationCoroutineScope.launch {
                    importOpml(di, uri)
                }
            }
        }

    val activityLauncher: ActivityLauncher by LocalDI.current.instance()

    // Each feed gets its own scroll state. Persists across device rotations, but is cleared when
    // switching feeds
    val feedListState =
        key(viewState.currentFeedOrTag) {
            pagedFeedItems.rememberLazyListState()
        }

    val feedGridState =
        key(viewState.currentFeedOrTag) {
            rememberLazyStaggeredGridState()
        }

    val toolbarColor = MaterialTheme.colorScheme.surface.toArgb()

    val drawerState = rememberDrawerState(initialValue = DrawerValue.Closed)
    val coroutineScope = rememberCoroutineScope()

    val focusNavDrawer = remember { FocusRequester() }

    BackHandler(
        enabled = drawerState.isOpen,
        onBack = {
            if (drawerState.isOpen) {
                coroutineScope.launch {
                    drawerState.close()
                }
            }
        },
    )

    // See https://issuetracker.google.com/issues/177245496#comment24
    // After recreation, items first return 0 items, then the cached items.
    // This behavior/issue is resetting the LazyListState scroll position.
    // Below is a workaround. More info: https://issuetracker.google.com/issues/177245496.
    val workaroundNavDrawerListState = rememberLazyListState()

    val navDrawerListStateToUse by remember {
        derivedStateOf {
            when (pagedNavDrawerItems.itemCount) {
                0 -> workaroundNavDrawerListState
                else -> navDrawerListState
            }
        }
    }

    ScreenWithNavDrawer(
        feedsAndTags = pagedNavDrawerItems,
        onToggleTagExpansion = { tag ->
            viewModel.toggleTagExpansion(tag)
        },
        onDrawerItemSelect = { feedId, tag ->
            FeedDestination.navigate(navController, feedId = feedId, tag = tag)
        },
        focusRequester = focusNavDrawer,
        drawerState = drawerState,
        navDrawerListState = navDrawerListStateToUse,
    ) {
        FeedScreen(
            viewState = viewState,
            onRefreshVisible = {
                viewModel.requestImmediateSyncOfCurrentFeedOrTag()
            },
            onRefreshAll = {
                viewModel.requestImmediateSyncOfAll()
                coroutineScope.launch {
                    if (feedListState.firstVisibleItemIndex != 0) {
                        feedListState.animateScrollToItem(0)
                    }
                    if (feedGridState.firstVisibleItemIndex != 0) {
                        feedGridState.animateScrollToItem(0)
                    }
                }
            },
            onMarkAllAsRead = {
                viewModel.markAllAsRead()
            },
            onShowToolbarMenu = { visible ->
                viewModel.setToolbarMenuVisible(visible)
            },
            ttsOnPlay = viewModel::ttsPlay,
            ttsOnPause = viewModel::ttsPause,
            ttsOnStop = viewModel::ttsStop,
            ttsOnSkipNext = viewModel::ttsSkipNext,
            ttsOnSelectLanguage = viewModel::ttsOnSelectLanguage,
            onAddFeed = { SearchFeedDestination.navigate(navController) },
            onEditFeed = { feedId ->
                EditFeedDestination.navigate(navController, feedId)
            },
            onShowEditDialog = {
                viewModel.setShowEditDialog(true)
            },
            onDismissEditDialog = {
                viewModel.setShowEditDialog(false)
            },
            onDeleteFeeds = { feedIds ->
                viewModel.deleteFeeds(feedIds.toList())
            },
            onShowDeleteDialog = {
                viewModel.setShowDeleteDialog(true)
            },
            onDismissDeleteDialog = {
                viewModel.setShowDeleteDialog(false)
            },
            onSettings = {
                SettingsDestination.navigate(navController)
            },
            onSendFeedback = {
                activityLauncher.startActivity(
                    openAdjacentIfSuitable = true,
                    intent = emailBugReportIntent(),
                )
            },
            onImport = {
                try {
                    opmlImporter.launch(
                        arrayOf(
                            // All valid for OPML files
                            "text/xml",
                            "text/x-opml",
                            "application/xml",
                            // This is the mimetype the file actually gets when exported
                            "application/octet-stream",
                            // But just in case a file isn't named right etc, accept all
                            "*/*",
                        ),
                    )
                } catch (e: Exception) {
                    // ActivityNotFoundException in particular
                    coroutineScope.launch {
                        toastMaker.makeToast(R.string.failed_to_import_OPML)
                    }
                }
            },
            onImportNode = {
                val nodeImporter = NodeImporter(FeederApplication.instance, coroutineScope, toastMaker)

                nodeImporter.importNodes()
                FeederApplication.instance.reloadService()
                viewModel.requestImmediateSyncOfAll()
                coroutineScope.launch {
                    if (feedListState.firstVisibleItemIndex != 0) {
                        feedListState.animateScrollToItem(0)
                    }
                    if (feedGridState.firstVisibleItemIndex != 0) {
                        feedGridState.animateScrollToItem(0)
                    }
                }
            },
            onExportOPML = {
                try {
                    opmlExporter.launch("feeder-export-${LocalDateTime.now()}.opml")
                } catch (e: Exception) {
                    // ActivityNotFoundException in particular
                    coroutineScope.launch {
                        toastMaker.makeToast(R.string.failed_to_export_OPML)
                    }
                }
            },
            onExportSavedArticles = {
                try {
                    savedArticleExporter.launch("feeder-saved-articles-${LocalDateTime.now()}.txt")
                } catch (e: Exception) {
                    // ActivityNotFoundException in particular
                    coroutineScope.launch {
                        toastMaker.makeToast(R.string.failed_to_export_saved_articles)
                    }
                }
            },
            drawerState = drawerState,
            markAsUnread = { itemId, unread, feedOrTag ->
                if (unread) {
                    viewModel.markAsUnread(itemId)
                } else {
                    viewModel.markAsRead(itemId, feedOrTag)
                }
            },
            markAsReadOnSwipe = { itemId, unread, saved ->
                when {
                    viewState.filter.onlyUnread -> {
                        // Get rid of it
                        viewModel.markAsReadOnSwipe(itemId)
                    }

                    viewState.filter.onlyUnreadAndSaved && !saved -> {
                        // Get rid of it
                        viewModel.markAsReadOnSwipe(itemId)
                    }

                    unread -> {
                        viewModel.markAsUnread(itemId)
                    }

                    else -> {
                        viewModel.markAsRead(itemId, null)
                    }
                }
            },
            markBeforeAsRead = { cursor ->
                viewModel.markBeforeAsRead(cursor)
            },
            markAfterAsRead = { cursor ->
                viewModel.markAfterAsRead(cursor)
            },
            onOpenFeedItem = { itemId ->
                viewModel.openArticle(
                    itemId = itemId,
                    openInBrowser = { articleLink ->
                        activityLauncher.openLinkInBrowser(articleLink)
                    },
                    openInCustomTab = { articleLink ->
                        activityLauncher.openLinkInCustomTab(articleLink, toolbarColor)
                    },
                    navigateToArticle = {
                        ArticleDestination.navigate(navController, itemId)
                    },
                )
            },
            onSetBookmark = { itemId, value ->
                viewModel.setBookmarked(itemId, value)
            },
            onShowFilterMenu = viewModel::setFilterMenuVisible,
            filterCallback = viewModel.filterCallback,
            feedListState = feedListState,
            feedGridState = feedGridState,
            pagedFeedItems = pagedFeedItems,
        )
    }
}

@OptIn(ExperimentalMaterial3Api::class, ExperimentalFoundationApi::class)
@Composable
fun FeedScreen(
    viewState: FeedScreenViewState,
    onRefreshVisible: () -> Unit,
    onRefreshAll: () -> Unit,
    onMarkAllAsRead: () -> Unit,
    onShowToolbarMenu: (Boolean) -> Unit,
    ttsOnPlay: () -> Unit,
    ttsOnPause: () -> Unit,
    ttsOnStop: () -> Unit,
    ttsOnSkipNext: () -> Unit,
    ttsOnSelectLanguage: (LocaleOverride) -> Unit,
    onAddFeed: () -> Unit,
    onImportNode: () -> Unit,
    onEditFeed: (Long) -> Unit,
    onShowEditDialog: () -> Unit,
    onDismissEditDialog: () -> Unit,
    onDeleteFeeds: (Iterable<Long>) -> Unit,
    onShowDeleteDialog: () -> Unit,
    onDismissDeleteDialog: () -> Unit,
    onSettings: () -> Unit,
    onSendFeedback: () -> Unit,
    onImport: () -> Unit,
    onExportOPML: () -> Unit,
    onExportSavedArticles: () -> Unit,
    drawerState: DrawerState,
    markAsUnread: (Long, Boolean, FeedOrTag?) -> Unit,
    markAsReadOnSwipe: (id: Long, unread: Boolean, saved: Boolean) -> Unit,
    markBeforeAsRead: (FeedItemCursor) -> Unit,
    markAfterAsRead: (FeedItemCursor) -> Unit,
    onOpenFeedItem: (Long) -> Unit,
    onSetBookmark: (Long, Boolean) -> Unit,
    onShowFilterMenu: (Boolean) -> Unit,
    filterCallback: FeedListFilterCallback,
    feedListState: LazyListState,
    feedGridState: LazyStaggeredGridState,
    pagedFeedItems: LazyPagingItems<FeedListItem>,
    modifier: Modifier = Modifier,
) {
    val coroutineScope = rememberCoroutineScope()
    val context = LocalContext.current
    val closeMenuText = stringResource(id = R.string.close_menu)

    FeedScreen(
        modifier = modifier,
        viewState = viewState,
        onRefreshVisible = onRefreshVisible,
        onOpenNavDrawer = {
            coroutineScope.launch {
                if (drawerState.isOpen) {
                    drawerState.close()
                } else {
                    drawerState.open()
                }
            }
        },
        onMarkAllAsRead = onMarkAllAsRead,
        ttsOnPlay = ttsOnPlay,
        ttsOnPause = ttsOnPause,
        ttsOnStop = ttsOnStop,
        ttsOnSkipNext = ttsOnSkipNext,
        ttsOnSelectLanguage = ttsOnSelectLanguage,
        onDismissDeleteDialog = onDismissDeleteDialog,
        onDismissEditDialog = onDismissEditDialog,
        onDelete = onDeleteFeeds,
        onEditFeed = onEditFeed,
        toolbarActions = {
            if (viewState.currentFeedOrTag.isNotSavedArticles) {
                PlainTooltipBox(
                    tooltip = {
                        Text(stringResource(id = R.string.filter_noun))
                    },
                ) {
                    Box {
                        IconButton(
                            onClick = { onShowFilterMenu(true) },
                        ) {
                            Icon(
                                Icons.Default.FilterList,
                                contentDescription = stringResource(R.string.filter_noun),
                            )
                        }
                        DropdownMenu(
                            expanded = viewState.showFilterMenu,
                            onDismissRequest = { onShowFilterMenu(false) },
                            modifier =
                                Modifier
                                    .onKeyEventLikeEscape {
                                        onShowFilterMenu(false)
                                    },
                        ) {
                            DropdownMenuItem(
                                enabled = false,
                                onClick = { /* Can't be modified - only shown for completeness */ },
                                leadingIcon = {
                                    Icon(
                                        when (viewState.filter.unread) {
                                            true -> Icons.Default.CheckBox
                                            false -> Icons.Default.CheckBoxOutlineBlank
                                        },
                                        contentDescription = null,
                                    )
                                },
                                text = {
                                    Text(stringResource(id = R.string.unread_adjective))
                                },
                                modifier =
                                    Modifier
                                        .safeSemantics {
                                            stateDescription =
                                                when (viewState.filter.unread) {
                                                    true -> context.getString(androidx.compose.ui.R.string.selected)
                                                    else -> context.getString(androidx.compose.ui.R.string.not_selected)
                                                }
                                            role = Role.Checkbox
                                        },
                            )
                            DropdownMenuItem(
                                onClick = {
                                    filterCallback.setSaved(!viewState.filter.saved)
                                    onShowFilterMenu(false)
                                },
                                leadingIcon = {
                                    Icon(
                                        when (viewState.filter.saved) {
                                            true -> Icons.Default.CheckBox
                                            false -> Icons.Default.CheckBoxOutlineBlank
                                        },
                                        contentDescription = null,
                                    )
                                },
                                text = {
                                    Text(stringResource(id = R.string.saved_adjective))
                                },
                                modifier =
                                    Modifier
                                        .safeSemantics {
                                            stateDescription =
                                                when (viewState.filter.saved) {
                                                    true -> context.getString(androidx.compose.ui.R.string.selected)
                                                    else -> context.getString(androidx.compose.ui.R.string.not_selected)
                                                }
                                            role = Role.Checkbox
                                        },
                            )
                            DropdownMenuItem(
                                onClick = {
                                    filterCallback.setRecentlyRead(!viewState.filter.recentlyRead)
                                    onShowFilterMenu(false)
                                },
                                leadingIcon = {
                                    Icon(
                                        when (viewState.filter.recentlyRead) {
                                            true -> Icons.Default.CheckBox
                                            false -> Icons.Default.CheckBoxOutlineBlank
                                        },
                                        contentDescription = null,
                                    )
                                },
                                text = {
                                    Text(stringResource(id = R.string.recently_read_adjective))
                                },
                                modifier =
                                    Modifier
                                        .safeSemantics {
                                            stateDescription =
                                                when (viewState.filter.recentlyRead) {
                                                    true -> context.getString(androidx.compose.ui.R.string.selected)
                                                    else -> context.getString(androidx.compose.ui.R.string.not_selected)
                                                }
                                            role = Role.Checkbox
                                        },
                            )
                            DropdownMenuItem(
                                onClick = {
                                    filterCallback.setRead(!viewState.filter.read)
                                    // Closing it is important for accessibility
                                    onShowFilterMenu(false)
                                },
                                leadingIcon = {
                                    Icon(
                                        when (viewState.filter.read) {
                                            true -> Icons.Default.CheckBox
                                            false -> Icons.Default.CheckBoxOutlineBlank
                                        },
                                        contentDescription = null,
                                    )
                                },
                                text = {
                                    Text(stringResource(id = R.string.read_adjective))
                                },
                                modifier =
                                    Modifier
                                        .safeSemantics {
                                            stateDescription =
                                                when (viewState.filter.read) {
                                                    true -> context.getString(androidx.compose.ui.R.string.selected)
                                                    else -> context.getString(androidx.compose.ui.R.string.not_selected)
                                                }
                                            role = Role.Checkbox
                                        },
                            )
                            // Hidden button for TalkBack
                            DropdownMenuItem(
                                onClick = {
                                    onShowFilterMenu(false)
                                },
                                text = {},
                                modifier =
                                    Modifier
                                        .height(0.dp)
                                        .safeSemantics {
                                            contentDescription = closeMenuText
                                            role = Role.Button
                                        },
                            )
                        }
                    }
                }
            }

            PlainTooltipBox(tooltip = { Text(stringResource(R.string.open_menu)) }) {
                Box {
                    IconButton(
                        onClick = { onShowToolbarMenu(true) },
                    ) {
                        Icon(
                            Icons.Default.MoreVert,
                            contentDescription = stringResource(R.string.open_menu),
                        )
                    }
                    DropdownMenu(
                        expanded = viewState.showToolbarMenu,
                        onDismissRequest = { onShowToolbarMenu(false) },
                        modifier =
                            Modifier.onKeyEventLikeEscape {
                                onShowToolbarMenu(false)
                            },
                    ) {
                        DropdownMenuItem(
                            onClick = {
                                onMarkAllAsRead()
                                onShowToolbarMenu(false)
                            },
                            leadingIcon = {
                                Icon(
                                    Icons.Default.DoneAll,
                                    contentDescription = null,
                                )
                            },
                            text = {
                                Text(stringResource(id = R.string.mark_all_as_read))
                            },
                        )
                        HorizontalDivider()
                        DropdownMenuItem(
                            onClick = {
                                onRefreshAll()
                                onShowToolbarMenu(false)
                            },
                            leadingIcon = {
                                Icon(
                                    Icons.Default.Refresh,
                                    contentDescription = stringResource(R.string.synchronize_feeds),
                                )
                            },
                            text = {
                                Text(stringResource(id = R.string.synchronize_feeds))
                            },
                        )
                        HorizontalDivider()
                        DropdownMenuItem(
                            onClick = {
                                onShowToolbarMenu(false)
                                onImportNode()
                            },
                            leadingIcon = {
                                Icon(
                                    Icons.Default.Add,
                                    contentDescription = null,
                                )
                            },
                            text = {
                                Text(stringResource(id = R.string.import_node))
                            },
                        )
                        DropdownMenuItem(
                            onClick = {
                                onShowToolbarMenu(false)
                                onAddFeed()
                            },
                            leadingIcon = {
                                Icon(
                                    Icons.Default.Add,
                                    contentDescription = null,
                                )
                            },
                            text = {
                                Text(stringResource(id = R.string.add_feed))
                            },
                        )
                        DropdownMenuItem(
                            onClick = {
                                if (viewState.visibleFeeds.size == 1) {
                                    onEditFeed(viewState.visibleFeeds.first().id)
                                } else {
                                    onShowEditDialog()
                                }
                                onShowToolbarMenu(false)
                            },
                            leadingIcon = {
                                Icon(
                                    Icons.Default.Edit,
                                    contentDescription = null,
                                )
                            },
                            text = {
                                Text(stringResource(id = R.string.edit_feed))
                            },
                        )
                        DropdownMenuItem(
                            onClick = {
                                onShowDeleteDialog()
                                onShowToolbarMenu(false)
                            },
                            leadingIcon = {
                                Icon(
                                    Icons.Default.Delete,
                                    contentDescription = null,
                                )
                            },
                            text = {
                                Text(stringResource(id = R.string.delete_feed))
                            },
                        )
                        HorizontalDivider()
                        DropdownMenuItem(
                            onClick = {
                                onShowToolbarMenu(false)
                                onImport()
                            },
                            leadingIcon = {
                                Icon(
                                    Icons.Default.ImportExport,
                                    contentDescription = null,
                                )
                            },
                            text = {
                                Text(stringResource(id = R.string.import_feeds_from_opml))
                            },
                        )
                        DropdownMenuItem(
                            onClick = {
                                onShowToolbarMenu(false)
                                onExportOPML()
                            },
                            leadingIcon = {
                                Icon(
                                    Icons.Default.ImportExport,
                                    contentDescription = null,
                                )
                            },
                            text = {
                                Text(stringResource(id = R.string.export_feeds_to_opml))
                            },
                        )
                        DropdownMenuItem(
                            onClick = {
                                onShowToolbarMenu(false)
                                onExportSavedArticles()
                            },
                            leadingIcon = {
                                Icon(
                                    Icons.Default.ImportExport,
                                    contentDescription = null,
                                )
                            },
                            text = {
                                Text(stringResource(id = R.string.export_saved_articles))
                            },
                        )
                        HorizontalDivider()
                        DropdownMenuItem(
                            onClick = {
                                onShowToolbarMenu(false)
                                onSettings()
                            },
                            leadingIcon = {
                                Icon(
                                    Icons.Default.Settings,
                                    contentDescription = null,
                                )
                            },
                            text = {
                                Text(stringResource(id = R.string.action_settings))
                            },
                        )
                        HorizontalDivider()
                        DropdownMenuItem(
                            onClick = {
                                onShowToolbarMenu(false)
                                onSendFeedback()
                            },
                            leadingIcon = {
                                Icon(
                                    Icons.Default.Email,
                                    contentDescription = null,
                                )
                            },
                            text = {
                                Text(stringResource(id = R.string.send_bug_report))
                            },
                        )
                        // Hidden button for TalkBack
                        DropdownMenuItem(
                            onClick = {
                                onShowToolbarMenu(false)
                            },
                            text = {},
                            modifier =
                                Modifier
                                    .height(0.dp)
                                    .safeSemantics {
                                        contentDescription = closeMenuText
                                        role = Role.Button
                                    },
                        )
                    }
                }
            }
        },
    ) { innerModifier ->
        val screenType =
            when (isCompactDevice()) {
                true -> FeedScreenType.FeedList
                false -> FeedScreenType.FeedGrid
            }

        when (screenType) {
            FeedScreenType.FeedGrid ->
                FeedGridContent(
                    viewState = viewState,
                    onOpenNavDrawer = {
                        coroutineScope.launch {
                            if (drawerState.isOpen) {
                                drawerState.close()
                            } else {
                                drawerState.open()
                            }
                        }
                    },
                    onAddFeed = onAddFeed,
                    onImportNode = onImportNode,
                    onRefreshAll = onRefreshAll,
                    markAsUnread = markAsUnread,
                    markAsReadOnSwipe = markAsReadOnSwipe,
                    markBeforeAsRead = markBeforeAsRead,
                    markAfterAsRead = markAfterAsRead,
                    onItemClick = onOpenFeedItem,
                    onSetBookmark = onSetBookmark,
                    gridState = feedGridState,
                    pagedFeedItems = pagedFeedItems,
                    modifier = innerModifier,
                )

            FeedScreenType.FeedList ->
                FeedListContent(
                    viewState = viewState,
                    onOpenNavDrawer = {
                        coroutineScope.launch {
                            if (drawerState.isOpen) {
                                drawerState.close()
                            } else {
                                drawerState.open()
                            }
                        }
                    },
                    onAddFeed = onAddFeed,
                    onImportNode = onImportNode,
                    onRefreshAll = onRefreshAll,
                    markAsUnread = markAsUnread,
                    markAsReadOnSwipe = markAsReadOnSwipe,
                    markBeforeAsRead = markBeforeAsRead,
                    markAfterAsRead = markAfterAsRead,
                    onItemClick = onOpenFeedItem,
                    onSetBookmark = onSetBookmark,
                    listState = feedListState,
                    pagedFeedItems = pagedFeedItems,
                    modifier = innerModifier,
                )
        }
    }
}

@OptIn(
    ExperimentalMaterial3Api::class,
    ExperimentalAnimationApi::class,
)
@Composable
fun FeedScreen(
    viewState: FeedScreenViewState,
    onRefreshVisible: () -> Unit,
    onOpenNavDrawer: () -> Unit,
    onMarkAllAsRead: () -> Unit,
    ttsOnPlay: () -> Unit,
    ttsOnPause: () -> Unit,
    ttsOnStop: () -> Unit,
    ttsOnSkipNext: () -> Unit,
    ttsOnSelectLanguage: (LocaleOverride) -> Unit,
    onDismissDeleteDialog: () -> Unit,
    onDismissEditDialog: () -> Unit,
    onDelete: (Iterable<Long>) -> Unit,
    onEditFeed: (Long) -> Unit,
    toolbarActions: @Composable (RowScope.() -> Unit),
    modifier: Modifier = Modifier,
    content: @Composable (Modifier) -> Unit,
) {
    var manuallyTriggeredRefresh by rememberSaveable {
        mutableStateOf(false)
    }
    val isRefreshing =
        remember(manuallyTriggeredRefresh, viewState.currentlySyncing) {
            (manuallyTriggeredRefresh || viewState.currentlySyncing)
        }

    LaunchedEffect(viewState.currentlySyncing) {
        if (manuallyTriggeredRefresh && viewState.currentlySyncing) {
            // A sync has happened so can safely set this to false now
            manuallyTriggeredRefresh = false
        }
    }

    LaunchedEffect(manuallyTriggeredRefresh) {
        // In the event that pulling doesn't trigger a refresh. Say if no feeds are present
        // or all feeds are so recent that no sync is triggered - or an error happens in sync
        // THEN we need to manually disable this variable so we don't get an infinite spinner
        if (manuallyTriggeredRefresh) {
            delay(5_000L)
            manuallyTriggeredRefresh = false
        }
    }

    val floatingActionButton: @Composable () -> Unit = {
        PlainTooltipBox(tooltip = { Text(stringResource(R.string.mark_all_as_read)) }) {
            FloatingActionButton(
                onClick = onMarkAllAsRead,
                modifier =
                    Modifier
                        .navigationBarsPadding(),
            ) {
                Icon(
                    Icons.Default.DoneAll,
                    contentDescription = stringResource(R.string.mark_all_as_read),
                )
            }
        }
    }
    val bottomBarVisibleState = remember { MutableTransitionState(viewState.isBottomBarVisible) }
    LaunchedEffect(viewState.isBottomBarVisible) {
        bottomBarVisibleState.targetState = viewState.isBottomBarVisible
    }

    val topAppBarState =
        key(viewState.currentFeedOrTag) {
            rememberTopAppBarState()
        }
    val scrollBehavior = TopAppBarDefaults.enterAlwaysScrollBehavior(topAppBarState)

    SetStatusBarColorToMatchScrollableTopAppBar(scrollBehavior)

    val pullRefreshState =
        rememberPullRefreshState(
            refreshing = isRefreshing,
            onRefresh = {
                manuallyTriggeredRefresh = true
                onRefreshVisible()
            },
        )

    Scaffold(
        topBar = {
            SensibleTopAppBar(
                scrollBehavior = scrollBehavior,
                title =
                    when (viewState.feedScreenTitle.type) {
                        FeedType.FEED, FeedType.TAG -> viewState.feedScreenTitle.title
                        FeedType.SAVED_ARTICLES -> stringResource(id = R.string.saved_articles)
                        FeedType.ALL_FEEDS -> stringResource(id = R.string.all_feeds)
                    }.let { title ->
                        if (viewState.feedScreenTitle.type == FeedType.SAVED_ARTICLES ||
                            !viewState.showTitleUnreadCount ||
                            viewState.feedScreenTitle.unreadCount == 0 ||
                            title == null
                        ) {
                            title
                        } else {
                            title + " ${stringResource(id = R.string.title_unread_count, viewState.feedScreenTitle.unreadCount)}"
                        }
                    } ?: "",
                navigationIcon = {
                    IconButton(
                        onClick = onOpenNavDrawer,
                    ) {
                        Icon(
                            Icons.Default.Menu,
                            contentDescription = stringResource(R.string.navigation_drawer_open),
                        )
                    }
                },
                actions = toolbarActions,
            )
        },
        bottomBar = {
            HideableTTSPlayer(
                visibleState = bottomBarVisibleState,
                currentlyPlaying = viewState.isTTSPlaying,
                onPlay = ttsOnPlay,
                onPause = ttsOnPause,
                onStop = ttsOnStop,
                onSkipNext = ttsOnSkipNext,
                languages = ImmutableHolder(viewState.ttsLanguages),
                floatingActionButton =
                    when (viewState.showFab) {
                        true -> floatingActionButton
                        false -> null
                    },
                onSelectLanguage = ttsOnSelectLanguage,
            )
        },
        floatingActionButton = {
            if (viewState.showFab) {
                AnimatedVisibility(
                    visible = bottomBarVisibleState.isIdle && !bottomBarVisibleState.targetState,
                    enter = scaleIn(animationSpec = tween(256)),
                    exit = scaleOut(animationSpec = tween(256)),
                ) {
                    floatingActionButton()
                }
            }
        },
        modifier =
            modifier
                // The order is important! PullToRefresh MUST come first
                .pullRefresh(state = pullRefreshState)
                .nestedScroll(scrollBehavior.nestedScrollConnection)
                .windowInsetsPadding(WindowInsets.navigationBars.only(WindowInsetsSides.Horizontal)),
        contentWindowInsets = WindowInsets.statusBars,
    ) { padding ->
        Box(
            modifier =
                Modifier
                    .padding(padding),
        ) {
            content(
                Modifier,
            )

            PullRefreshIndicator(
                isRefreshing,
                pullRefreshState,
                modifier =
                    Modifier
                        .align(Alignment.TopCenter),
            )
        }

        if (viewState.showDeleteDialog) {
            DeleteFeedDialog(
                feeds =
                    ImmutableHolder(
                        viewState.visibleFeeds.map {
                            DeletableFeed(it.id, it.displayTitle)
                        },
                    ),
                onDismiss = onDismissDeleteDialog,
                onDelete = onDelete,
            )
        }

        if (viewState.showEditDialog) {
            EditFeedDialog(
                feeds =
                    ImmutableHolder(
                        viewState.visibleFeeds.map {
                            DeletableFeed(
                                it.id,
                                it.displayTitle,
                            )
                        },
                    ),
                onDismiss = onDismissEditDialog,
                onEdit = onEditFeed,
            )
        }
    }
}

@Composable
fun FeedListContent(
    viewState: FeedScreenViewState,
    onOpenNavDrawer: () -> Unit,
    onAddFeed: () -> Unit,
    onRefreshAll: () -> Unit,
    onImportNode: () -> Unit,
    markAsUnread: (Long, Boolean, FeedOrTag?) -> Unit,
    markAsReadOnSwipe: (id: Long, unread: Boolean, saved: Boolean) -> Unit,
    markBeforeAsRead: (FeedItemCursor) -> Unit,
    markAfterAsRead: (FeedItemCursor) -> Unit,
    onItemClick: (Long) -> Unit,
    onSetBookmark: (Long, Boolean) -> Unit,
    listState: LazyListState,
    pagedFeedItems: LazyPagingItems<FeedListItem>,
    modifier: Modifier = Modifier,
) {
    val coroutineScope = rememberCoroutineScope()
    val activityLauncher: ActivityLauncher by LocalDI.current.instance()

    Box(modifier = modifier) {
        AnimatedVisibility(
            enter = fadeIn(),
            exit = fadeOut(),
            visible = !viewState.haveVisibleFeedItems,
        ) {
            // Keeping the Box behind so the scrollability doesn't override clickable
            // Separate box because scrollable will ignore max size.
            Box(
                modifier =
                    Modifier
                        .fillMaxSize()
                        .verticalScroll(rememberScrollState()),
            )
            NothingToRead(
                modifier = Modifier,
                onOpenOtherFeed = onRefreshAll,
                onAddFeed = onImportNode,
            )
        }

        val arrangement =
            when (viewState.feedItemStyle) {
                FeedItemStyle.CARD -> Arrangement.spacedBy(LocalDimens.current.margin)
                FeedItemStyle.COMPACT_CARD -> Arrangement.spacedBy(LocalDimens.current.margin)
                FeedItemStyle.COMPACT -> Arrangement.spacedBy(0.dp)
                FeedItemStyle.SUPER_COMPACT -> Arrangement.spacedBy(0.dp)
            }

        AnimatedVisibility(
            enter = fadeIn(),
            exit = fadeOut(),
            visible = viewState.haveVisibleFeedItems,
        ) {
            val screenHeightPx =
                with(LocalDensity.current) {
                    LocalConfiguration.current.screenHeightDp.dp.toPx().toInt()
                }
            LazyColumn(
                state = listState,
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = arrangement,
                contentPadding =
                    if (viewState.isBottomBarVisible) {
                        PaddingValues(0.dp)
                    } else {
                        WindowInsets.navigationBars.only(
                            WindowInsetsSides.Bottom,
                        ).run {
                            when (viewState.feedItemStyle) {
                                FeedItemStyle.CARD -> addMargin(horizontal = LocalDimens.current.margin)
                                FeedItemStyle.COMPACT_CARD -> addMargin(horizontal = LocalDimens.current.margin)
                                // No margin since dividers
                                FeedItemStyle.COMPACT, FeedItemStyle.SUPER_COMPACT -> this
                            }
                        }
                            .asPaddingValues()
                    },
                modifier =
                    Modifier
                        .fillMaxSize()
                        .safeSemantics {
                            testTag = "feed_list"
                            collectionInfo = CollectionInfo(pagedFeedItems.itemCount, 1)
                        },
            ) {
                /*
                This is a trick to make the list stay at item 0 when updates come in IF it is
                scrolled to the top.
                 */
                item(
                    key = "SpacerScrollTrick",
                    contentType = "SpacerScrollTrick",
                ) {
                    Spacer(modifier = Modifier.fillMaxWidth())
                }
                items(
                    count = pagedFeedItems.itemCount,
                    key = pagedFeedItems.itemKey { it.id },
                    contentType = pagedFeedItems.itemContentType { it.contentType(viewState.feedItemStyle) },
                ) { itemIndex ->
                    val previewItem = pagedFeedItems[itemIndex] ?: PLACEHOLDER_ITEM

                    if (viewState.markAsReadOnScroll && previewItem.unread) {
                        val mostlyVisible: Boolean by listState.rememberIsItemMostlyVisible(
                            key = previewItem.id,
                            screenHeightPx = screenHeightPx,
                        )
                        MarkItemAsReadOnScroll(
                            itemId = previewItem.id,
                            mostlyVisible = mostlyVisible,
                            currentFeedOrTag = viewState.currentFeedOrTag,
                            coroutineScope = coroutineScope,
                            markAsRead = markAsUnread,
                        )
                    }

                    SwipeableFeedItemPreview(
                        onSwipe = { currentState ->
                            markAsReadOnSwipe(
                                previewItem.id,
                                !currentState,
                                previewItem.bookmarked,
                            )
                        },
                        filter = viewState.filter,
                        item = previewItem,
                        showThumbnail = viewState.showThumbnails,
                        feedItemStyle = viewState.feedItemStyle,
                        swipeAsRead = viewState.swipeAsRead,
                        bookmarkIndicator = !viewState.currentFeedOrTag.isSavedArticles,
                        maxLines = viewState.maxLines,
                        showOnlyTitle = viewState.showOnlyTitle,
                        showReadingTime = viewState.showReadingTime,
                        onMarkAboveAsRead = {
                            markBeforeAsRead(previewItem.cursor)
                            if (viewState.filter.onlyUnread) {
                                coroutineScope.launch {
                                    listState.scrollToItem(0)
                                }
                            }
                        },
                        onMarkBelowAsRead = {
                            markAfterAsRead(previewItem.cursor)
                        },
                        onToggleBookmark = {
                            onSetBookmark(previewItem.id, !previewItem.bookmarked)
                        },
                        onShareItem = {
                            val intent =
                                Intent.createChooser(
                                    Intent(Intent.ACTION_SEND).apply {
                                        if (previewItem.link != null) {
                                            putExtra(Intent.EXTRA_TEXT, previewItem.link)
                                        }
                                        putExtra(Intent.EXTRA_TITLE, previewItem.title)
                                        type = "text/plain"
                                    },
                                    null,
                                )
                            activityLauncher.startActivity(
                                openAdjacentIfSuitable = false,
                                intent = intent,
                            )
                        },
                        modifier =
                            Modifier
                                .safeSemantics {
                                    collectionItemInfo =
                                        CollectionItemInfo(
                                            rowIndex = itemIndex,
                                            rowSpan = 1,
                                            columnIndex = 1,
                                            columnSpan = 1,
                                        )
                                },
                    ) {
                        onItemClick(previewItem.id)
                    }

                    if (viewState.feedItemStyle != FeedItemStyle.CARD &&
                        viewState.feedItemStyle != FeedItemStyle.COMPACT_CARD
                    ) {
                        if (itemIndex < pagedFeedItems.itemCount - 1) {
                            HorizontalDivider(
                                modifier =
                                    Modifier
                                        .height(1.dp)
                                        .fillMaxWidth(),
                            )
                        }
                    }
                }
                /*
                This item is provide padding for the FAB
                 */
                if (viewState.showFab && !viewState.isBottomBarVisible) {
                    item(
                        key = "SpacerForFab",
                        contentType = "SpacerForFab",
                    ) {
                        Spacer(
                            modifier =
                                Modifier
                                    .fillMaxWidth()
                                    .height((56 + 16).dp),
                        )
                    }
                }
            }
        }
    }
}

@OptIn(ExperimentalFoundationApi::class)
@Composable
fun FeedGridContent(
    viewState: FeedScreenViewState,
    onOpenNavDrawer: () -> Unit,
    onAddFeed: () -> Unit,
    onRefreshAll: () -> Unit,
    onImportNode: () -> Unit,
    markAsUnread: (Long, Boolean, FeedOrTag?) -> Unit,
    markAsReadOnSwipe: (id: Long, unread: Boolean, saved: Boolean) -> Unit,
    markBeforeAsRead: (FeedItemCursor) -> Unit,
    markAfterAsRead: (FeedItemCursor) -> Unit,
    onItemClick: (Long) -> Unit,
    onSetBookmark: (Long, Boolean) -> Unit,
    gridState: LazyStaggeredGridState,
    pagedFeedItems: LazyPagingItems<FeedListItem>,
    modifier: Modifier = Modifier,
) {
    val coroutineScope = rememberCoroutineScope()
    val activityLauncher: ActivityLauncher by LocalDI.current.instance()

    val screenHeightPx =
        with(LocalDensity.current) {
            LocalConfiguration.current.screenHeightDp.dp.toPx().toInt()
        }

    Box(modifier = modifier) {
        AnimatedVisibility(
            enter = fadeIn(),
            exit = fadeOut(),
            visible = !viewState.haveVisibleFeedItems,
        ) {
            // Keeping the Box behind so the scrollability doesn't override clickable
            // Separate box because scrollable will ignore max size.
            Box(
                modifier =
                    Modifier
                        .fillMaxSize()
                        .verticalScroll(rememberScrollState()),
            )
            NothingToRead(
                modifier = Modifier,
                onOpenOtherFeed = onRefreshAll,
                onAddFeed = onImportNode,
            )
        }

        // Grid has hard-coded card
        val feedItemStyle =
            remember {
                FeedItemStyle.CARD
            }

        val arrangement =
            when (feedItemStyle) {
                FeedItemStyle.CARD -> Arrangement.spacedBy(LocalDimens.current.gutter)
                FeedItemStyle.COMPACT_CARD -> Arrangement.spacedBy(LocalDimens.current.gutter)
                FeedItemStyle.COMPACT -> Arrangement.spacedBy(LocalDimens.current.gutter)
                FeedItemStyle.SUPER_COMPACT -> Arrangement.spacedBy(LocalDimens.current.gutter)
            }

        AnimatedVisibility(
            enter = fadeIn(),
            exit = fadeOut(),
            visible = viewState.haveVisibleFeedItems,
        ) {
            LazyVerticalStaggeredGrid(
                state = gridState,
                columns = StaggeredGridCells.Fixed(LocalDimens.current.feedScreenColumns),
                contentPadding =
                    if (viewState.isBottomBarVisible) {
                        PaddingValues(0.dp)
                    } else {
                        WindowInsets.navigationBars.only(
                            WindowInsetsSides.Bottom,
                        ).addMargin(LocalDimens.current.margin)
                            .asPaddingValues()
                    },
                verticalItemSpacing = LocalDimens.current.gutter,
                horizontalArrangement = arrangement,
                modifier = Modifier.fillMaxSize(),
            ) {
                items(
                    count = pagedFeedItems.itemCount,
                    key = pagedFeedItems.itemKey { it.id },
                    contentType = pagedFeedItems.itemContentType { it.contentType(feedItemStyle) },
                ) { itemIndex ->
                    val previewItem = pagedFeedItems[itemIndex] ?: PLACEHOLDER_ITEM

                    // Very important that items don't change size or disappear when scrolling
                    // Placeholder will have no id
                    if (previewItem.id > ID_UNSET && viewState.markAsReadOnScroll && previewItem.unread) {
                        val mostlyVisible: Boolean by gridState.rememberIsItemMostlyVisible(
                            key = previewItem.id,
                            screenHeightPx = screenHeightPx,
                        )
                        MarkItemAsReadOnScroll(
                            itemId = previewItem.id,
                            mostlyVisible = mostlyVisible,
                            currentFeedOrTag = viewState.currentFeedOrTag,
                            coroutineScope = coroutineScope,
                            markAsRead = markAsUnread,
                        )
                    }

                    SwipeableFeedItemPreview(
                        onSwipe = { currentState ->
                            markAsReadOnSwipe(
                                previewItem.id,
                                !currentState,
                                previewItem.bookmarked,
                            )
                        },
                        filter = viewState.filter,
                        item = previewItem,
                        showThumbnail = viewState.showThumbnails,
                        feedItemStyle = feedItemStyle,
                        swipeAsRead = viewState.swipeAsRead,
                        bookmarkIndicator = !viewState.currentFeedOrTag.isSavedArticles,
                        maxLines = viewState.maxLines,
                        showOnlyTitle = viewState.showOnlyTitle,
                        showReadingTime = viewState.showReadingTime,
                        onMarkAboveAsRead = {
                            markBeforeAsRead(previewItem.cursor)
                            if (viewState.filter.onlyUnread) {
                                coroutineScope.launch {
                                    gridState.scrollToItem(0)
                                }
                            }
                        },
                        onMarkBelowAsRead = {
                            markAfterAsRead(previewItem.cursor)
                        },
                        onToggleBookmark = {
                            onSetBookmark(previewItem.id, !previewItem.bookmarked)
                        },
                        onShareItem = {
                            val intent =
                                Intent.createChooser(
                                    Intent(Intent.ACTION_SEND).apply {
                                        if (previewItem.link != null) {
                                            putExtra(Intent.EXTRA_TEXT, previewItem.link)
                                        }
                                        putExtra(Intent.EXTRA_TITLE, previewItem.title)
                                        type = "text/plain"
                                    },
                                    null,
                                )
                            activityLauncher.startActivity(
                                openAdjacentIfSuitable = false,
                                intent = intent,
                            )
                        },
                    ) {
                        onItemClick(previewItem.id)
                    }
                }
            }
        }
    }
}

@Immutable
data class FeedOrTag(
    val id: Long,
    val tag: String,
)

@Suppress("unused")
val FeedOrTag.isFeed
    get() = id > ID_UNSET

val FeedOrTag.isSavedArticles
    get() = id == ID_SAVED_ARTICLES

val FeedOrTag.isNotSavedArticles
    get() = !isSavedArticles

enum class FeedScreenType {
    FeedGrid,
    FeedList,
}

// See https://issuetracker.google.com/issues/177245496#comment24
@Composable
fun <T : Any> LazyPagingItems<T>.rememberLazyListState(): LazyListState {
    // After recreation, LazyPagingItems first return 0 items, then the cached items.
    // This behavior/issue is resetting the LazyListState scroll position.
    // Below is a workaround. More info: https://issuetracker.google.com/issues/177245496.
    return when (itemCount) {
        // Return a different LazyListState instance.
        0 -> remember(this) { LazyListState(0, 0) }
        // Return rememberLazyListState (normal case).
        else -> androidx.compose.foundation.lazy.rememberLazyListState()
    }
}

/**
 * @param itemId id of item to mark as read
 * @param mostlyVisible if item is mostly visible
 * @param currentFeedOrTag current feed or tag at the time of display
 * @param coroutineScope a scope which will be cancelled when navigated away from feed screen
 * @param markAsRead action to run
 */
@Composable
fun MarkItemAsReadOnScroll(
    itemId: Long,
    mostlyVisible: Boolean,
    currentFeedOrTag: FeedOrTag,
    coroutineScope: CoroutineScope,
    markAsRead: (Long, Boolean, FeedOrTag?) -> Unit,
) {
    // Lambda parameters in a @Composable that are referenced directly inside of restarting effects can cause issues or unpredictable behavior.
    val markAsReadCallback by rememberUpdatedState(newValue = markAsRead)
    var visibleTime by remember {
        mutableStateOf(FAR_FUTURE)
    }
    LaunchedEffect(mostlyVisible) {
        if (mostlyVisible && visibleTime == FAR_FUTURE) {
            visibleTime = Instant.now()
        }
    }

    DisposableEffect(visibleTime) {
        onDispose {
            // Check time BEFORE delaying action
            if (visibleTime.isBefore(Instant.now().minusMillis(REQUIRED_VISIBLE_TIME_FOR_MARK_AS_READ))) {
                coroutineScope.launch(Dispatchers.IO) {
                    // Why Coroutine? Why a delay?
                    // Because this scope will be cancelled if the screen
                    // is navigated away from and I only want things to be marked
                    // during scroll - not during navigation.
                    // Navigating between feeds is a special case, which is
                    // why the currentFeedOrTag needs to be passed to the
                    // view model.
                    @Suppress("UNNECESSARYVARIABLE")
                    val feedOrTag = currentFeedOrTag
                    delay(100)
                    markAsReadCallback(
                        itemId,
                        false,
                        feedOrTag,
                    )
                }
            }
        }
    }
}

private const val REQUIRED_VISIBLE_TIME_FOR_MARK_AS_READ = 500L

private val PLACEHOLDER_ITEM =
    FeedListItem(
        id = ID_UNSET,
        title = "",
        snippet = "",
        feedTitle = "",
        unread = true,
        pubDate = "",
        image = null,
        link = null,
        bookmarked = false,
        feedImageUrl = null,
        primarySortTime = Instant.EPOCH,
        rawPubDate = null,
        wordCount = 0,
    )

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun PlainTooltipBox(
    tooltip: @Composable () -> Unit,
    modifier: Modifier = Modifier,
    content: @Composable () -> Unit,
) {
    TooltipBox(
        positionProvider = TooltipDefaults.rememberPlainTooltipPositionProvider(),
        state = rememberTooltipState(),
        tooltip = {
            PlainTooltip {
                tooltip()
            }
        },
        modifier = modifier,
        content = content,
    )
}

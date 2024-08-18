package com.nononsenseapps.feeder.ui.compose.feedarticle

import android.content.Intent
import androidx.activity.compose.BackHandler
import androidx.compose.animation.core.MutableTransitionState
import androidx.compose.foundation.focusGroup
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.navigationBars
import androidx.compose.foundation.layout.only
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBars
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.lazy.LazyListScope
import androidx.compose.foundation.lazy.LazyListState
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.automirrored.filled.Article
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.filled.OpenInBrowser
import androidx.compose.material.icons.filled.Share
import androidx.compose.material.icons.filled.Star
import androidx.compose.material.icons.filled.VisibilityOff
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusProperties
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.role
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.archmodel.TextToDisplay
import com.nononsenseapps.feeder.db.room.ID_UNSET
import com.nononsenseapps.feeder.model.LocaleOverride
import com.nononsenseapps.feeder.ui.compose.components.safeSemantics
import com.nononsenseapps.feeder.ui.compose.feed.PlainTooltipBox
import com.nononsenseapps.feeder.ui.compose.html.linearArticleContent
import com.nononsenseapps.feeder.ui.compose.icons.CustomFilled
import com.nononsenseapps.feeder.ui.compose.icons.TextToSpeech
import com.nononsenseapps.feeder.ui.compose.readaloud.HideableTTSPlayer
import com.nononsenseapps.feeder.ui.compose.theme.SensibleTopAppBar
import com.nononsenseapps.feeder.ui.compose.theme.SetStatusBarColorToMatchScrollableTopAppBar
import com.nononsenseapps.feeder.ui.compose.utils.ImmutableHolder
import com.nononsenseapps.feeder.ui.compose.utils.ScreenType
import com.nononsenseapps.feeder.ui.compose.utils.onKeyEventLikeEscape
import com.nononsenseapps.feeder.util.ActivityLauncher
import com.nononsenseapps.feeder.util.FilePathProvider
import com.nononsenseapps.feeder.util.unicodeWrap
import org.kodein.di.compose.LocalDI
import org.kodein.di.instance
import java.time.ZoneId
import java.time.ZonedDateTime

@Composable
fun ArticleScreen(
    onNavigateUp: () -> Unit,
    onNavigateToFeed: (Long) -> Unit,
    viewModel: ArticleViewModel,
) {
    BackHandler(onBack = onNavigateUp)
    val activityLauncher: ActivityLauncher by LocalDI.current.instance()

    val viewState by viewModel.viewState.collectAsStateWithLifecycle()

    val articleListState = rememberLazyListState()

    val toolbarColor = MaterialTheme.colorScheme.surface.toArgb()

    ArticleScreen(
        viewState = viewState,
        onToggleFullText = viewModel::toggleFullText,
        onMarkAsUnread = viewModel::markAsUnread,
        onShare = {
            if (viewState.articleId > ID_UNSET) {
                val intent =
                    Intent.createChooser(
                        Intent(Intent.ACTION_SEND).apply {
                            if (viewState.articleLink != null) {
                                putExtra(Intent.EXTRA_TEXT, viewState.articleLink)
                            }
                            putExtra(Intent.EXTRA_TITLE, viewState.articleTitle)
                            type = "text/plain"
                        },
                        null,
                    )
                activityLauncher.startActivity(
                    openAdjacentIfSuitable = false,
                    intent = intent,
                )
            }
        },
        onOpenInCustomTab = {
            viewState.articleLink?.let { link ->
                activityLauncher.openLinkInCustomTab(link, toolbarColor)
            }
        },
        onFeedTitleClick = {
            onNavigateToFeed(viewState.articleFeedId)
        },
        onShowToolbarMenu = viewModel::setToolbarMenuVisible,
        ttsOnPlay = viewModel::ttsPlay,
        ttsOnPause = viewModel::ttsPause,
        ttsOnStop = viewModel::ttsStop,
        ttsOnSkipNext = viewModel::ttsSkipNext,
        ttsOnSelectLanguage = viewModel::ttsOnSelectLanguage,
        onToggleBookmark = {
            viewModel.setBookmarked(!viewState.isBookmarked)
        },
        articleListState = articleListState,
        onNavigateUp = onNavigateUp,
    )
}

@OptIn(
    ExperimentalMaterial3Api::class,
)
@Composable
fun ArticleScreen(
    viewState: ArticleScreenViewState,
    onToggleFullText: () -> Unit,
    onMarkAsUnread: () -> Unit,
    onShare: () -> Unit,
    onOpenInCustomTab: () -> Unit,
    onFeedTitleClick: () -> Unit,
    onShowToolbarMenu: (Boolean) -> Unit,
    ttsOnPlay: () -> Unit,
    ttsOnPause: () -> Unit,
    ttsOnStop: () -> Unit,
    ttsOnSkipNext: () -> Unit,
    ttsOnSelectLanguage: (LocaleOverride) -> Unit,
    onToggleBookmark: () -> Unit,
    articleListState: LazyListState,
    modifier: Modifier = Modifier,
    onNavigateUp: () -> Unit,
) {
    val scrollBehavior = TopAppBarDefaults.enterAlwaysScrollBehavior()

    SetStatusBarColorToMatchScrollableTopAppBar(scrollBehavior)

    val bottomBarVisibleState = remember { MutableTransitionState(viewState.isBottomBarVisible) }
    LaunchedEffect(viewState.isBottomBarVisible) {
        bottomBarVisibleState.targetState = viewState.isBottomBarVisible
    }

    val focusArticle = remember { FocusRequester() }
    val focusTopBar = remember { FocusRequester() }

    val closeMenuText = stringResource(id = R.string.close_menu)

    Scaffold(
        modifier =
            modifier
                .nestedScroll(scrollBehavior.nestedScrollConnection)
                .windowInsetsPadding(WindowInsets.navigationBars.only(WindowInsetsSides.Horizontal)),
        contentWindowInsets = WindowInsets.statusBars,
        topBar = {
            SensibleTopAppBar(
                modifier =
                    Modifier
                        .focusGroup()
                        .focusRequester(focusTopBar)
                        .focusProperties {
                            down = focusArticle
                        },
                scrollBehavior = scrollBehavior,
                title = viewState.feedDisplayTitle,
                navigationIcon = {
                    IconButton(onClick = onNavigateUp) {
                        Icon(
                            Icons.AutoMirrored.Filled.ArrowBack,
                            contentDescription = stringResource(R.string.go_back),
                        )
                    }
                },
                actions = {
                    PlainTooltipBox(tooltip = { Text(stringResource(R.string.fetch_full_article)) }) {
                        IconButton(
                            onClick = onToggleFullText,
                        ) {
                            Icon(
                                Icons.AutoMirrored.Filled.Article,
                                contentDescription = stringResource(R.string.fetch_full_article),
                            )
                        }
                    }

                    PlainTooltipBox(tooltip = { Text(stringResource(id = R.string.open_in_web_view)) }) {
                        IconButton(
                            onClick = onOpenInCustomTab,
                        ) {
                            Icon(
                                Icons.Default.OpenInBrowser,
                                contentDescription = stringResource(id = R.string.open_in_web_view),
                            )
                        }
                    }

                    PlainTooltipBox(tooltip = { Text(stringResource(id = R.string.open_menu)) }) {
                        Box {
                            IconButton(
                                onClick = { onShowToolbarMenu(true) },
                            ) {
                                Icon(
                                    Icons.Default.MoreVert,
                                    contentDescription = stringResource(id = R.string.open_menu),
                                )
                            }
                            DropdownMenu(
                                expanded = viewState.showToolbarMenu,
                                onDismissRequest = { onShowToolbarMenu(false) },
                                modifier =
                                    Modifier
                                        .onKeyEventLikeEscape {
                                            onShowToolbarMenu(false)
                                        },
                            ) {
                                DropdownMenuItem(
                                    onClick = {
                                        onShowToolbarMenu(false)
                                        onShare()
                                    },
                                    leadingIcon = {
                                        Icon(
                                            Icons.Default.Share,
                                            contentDescription = null,
                                        )
                                    },
                                    text = {
                                        Text(stringResource(id = R.string.share))
                                    },
                                )

                                DropdownMenuItem(
                                    onClick = {
                                        onShowToolbarMenu(false)
                                        onMarkAsUnread()
                                    },
                                    leadingIcon = {
                                        Icon(
                                            Icons.Default.VisibilityOff,
                                            contentDescription = null,
                                        )
                                    },
                                    text = {
                                        Text(stringResource(id = R.string.mark_as_unread))
                                    },
                                )
                                DropdownMenuItem(
                                    onClick = {
                                        onShowToolbarMenu(false)
                                        onToggleBookmark()
                                    },
                                    leadingIcon = {
                                        Icon(
                                            Icons.Default.Star,
                                            contentDescription = null,
                                        )
                                    },
                                    text = {
                                        Text(
                                            stringResource(
                                                if (viewState.isBookmarked) {
                                                    R.string.unsave_article
                                                } else {
                                                    R.string.save_article
                                                },
                                            ),
                                        )
                                    },
                                )
                                DropdownMenuItem(
                                    onClick = {
                                        onShowToolbarMenu(false)
                                        ttsOnPlay()
                                    },
                                    leadingIcon = {
                                        Icon(
                                            Icons.CustomFilled.TextToSpeech,
                                            contentDescription = null,
                                        )
                                    },
                                    text = {
                                        Text(stringResource(id = R.string.read_article))
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
                onSelectLanguage = ttsOnSelectLanguage,
            )
        },
    ) { padding ->
        // Box handles the dynamic padding so ArticleContent don't have to recompose on scroll
        Box(
            modifier =
                Modifier
                    .padding(padding),
        ) {
            ArticleContent(
                viewState = viewState,
                screenType = ScreenType.SINGLE,
                articleListState = articleListState,
                onFeedTitleClick = onFeedTitleClick,
                modifier =
                    Modifier
                        .focusGroup()
                        .focusRequester(focusArticle)
                        .focusProperties {
                            up = focusTopBar
                        },
            )
        }
    }
}

@Composable
fun ArticleContent(
    viewState: ArticleScreenViewState,
    screenType: ScreenType,
    onFeedTitleClick: () -> Unit,
    articleListState: LazyListState,
    modifier: Modifier = Modifier,
) {
    val filePathProvider by LocalDI.current.instance<FilePathProvider>()

    val toolbarColor = MaterialTheme.colorScheme.surface.toArgb()

    val context = LocalContext.current
    val activityLauncher: ActivityLauncher by LocalDI.current.instance()

    ReaderView(
        screenType = screenType,
        wordCount = viewState.wordCount,
        onEnclosureClick = {
            if (viewState.enclosure.present) {
                activityLauncher.openLinkInBrowser(link = viewState.enclosure.link)
            }
        },
        onFeedTitleClick = onFeedTitleClick,
        enclosure = viewState.enclosure,
        articleTitle = viewState.articleTitle,
        feedTitle = viewState.feedDisplayTitle,
        authorDate =
            when {
                viewState.author == null && viewState.pubDate != null ->
                    stringResource(
                        R.string.on_date,
                        (viewState.pubDate?.withZoneSameInstant(ZoneId.systemDefault()) ?: ZonedDateTime.now()).format(dateTimeFormat),
                    )

                viewState.author != null && viewState.pubDate != null ->
                    stringResource(
                        R.string.by_author_on_date,
                        // Must wrap author in unicode marks to ensure it formats
                        // correctly in RTL
                        context.unicodeWrap(viewState.author ?: ""),
                        (viewState.pubDate?.withZoneSameInstant(ZoneId.systemDefault()) ?: ZonedDateTime.now()).format(dateTimeFormat),
                    )

                else -> null
            },
        image = viewState.image,
        isFeedText = viewState.textToDisplay == TextToDisplay.CONTENT,
        modifier = modifier,
        articleListState = articleListState,
    ) {
        // Can take a composition or two before viewstate is set to its actual values
        if (viewState.articleId > ID_UNSET) {
            when (viewState.textToDisplay) {
                TextToDisplay.CONTENT -> {
                    linearArticleContent(
                        articleContent = viewState.articleContent,
                        onLinkClick = { link ->
                            activityLauncher.openLink(
                                link = link,
                                toolbarColor = toolbarColor,
                            )
                        },
                    )
                }

                TextToDisplay.LOADING_FULLTEXT -> {
                    LoadingItem()
                }

                TextToDisplay.FAILED_TO_LOAD_FULLTEXT -> {
                    item {
                        Text(text = stringResource(id = R.string.failed_to_fetch_full_article))
                    }
                }

                TextToDisplay.FAILED_MISSING_BODY -> {
                    item {
                        Text(text = stringResource(id = R.string.failed_to_fetch_full_article_missing_body))
                    }
                }

                TextToDisplay.FAILED_MISSING_LINK -> {
                    item {
                        Text(text = stringResource(id = R.string.failed_to_fetch_full_article_missing_link))
                    }
                }

                TextToDisplay.FAILED_NOT_HTML -> {
                    item {
                        Text(text = stringResource(id = R.string.failed_to_fetch_full_article_not_html))
                    }
                }
            }
        }
    }
}

@Suppress("FunctionName")
private fun LazyListScope.LoadingItem() {
    item {
        Text(text = stringResource(id = R.string.fetching_full_article))
    }
}

private const val LOG_TAG = "FEEDER_ARTICLESCREEN"

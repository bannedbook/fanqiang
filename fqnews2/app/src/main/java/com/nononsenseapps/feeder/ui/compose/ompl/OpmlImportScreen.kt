package com.nononsenseapps.feeder.ui.compose.ompl

import android.content.ContentResolver
import android.net.Uri
import android.util.Log
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.navigationBars
import androidx.compose.foundation.layout.only
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBars
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.Immutable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.res.pluralStringResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.nononsenseapps.feeder.ApplicationCoroutineScope
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.db.room.Feed
import com.nononsenseapps.feeder.model.OPMLParserHandler
import com.nononsenseapps.feeder.model.opml.OpmlPullParser
import com.nononsenseapps.feeder.model.opml.importOpml
import com.nononsenseapps.feeder.ui.compose.components.OkCancelWithNonScrollableContent
import com.nononsenseapps.feeder.ui.compose.text.WithBidiDeterminedLayoutDirection
import com.nononsenseapps.feeder.ui.compose.theme.FeederTheme
import com.nononsenseapps.feeder.ui.compose.theme.LocalDimens
import com.nononsenseapps.feeder.ui.compose.theme.SensibleTopAppBar
import com.nononsenseapps.feeder.ui.compose.theme.SetStatusBarColorToMatchScrollableTopAppBar
import com.nononsenseapps.feeder.ui.compose.utils.LocalWindowSizeMetrics
import com.nononsenseapps.feeder.ui.compose.utils.PreviewThemes
import com.nononsenseapps.feeder.ui.compose.utils.ProvideScaledText
import com.nononsenseapps.feeder.ui.compose.utils.ScreenType
import com.nononsenseapps.feeder.ui.compose.utils.getScreenType
import kotlinx.coroutines.launch
import org.kodein.di.compose.LocalDI
import org.kodein.di.instance
import java.net.URL

private const val LOG_TAG = "FEEDER_OPMLIMPORT"

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun OpmlImportScreen(
    onNavigateUp: () -> Unit,
    uri: Uri?,
    onDismiss: () -> Unit,
    modifier: Modifier = Modifier,
    onOk: () -> Unit,
) {
    val di = LocalDI.current
    var viewState: ViewState by remember {
        mutableStateOf(ViewState())
    }
    LaunchedEffect(uri) {
        if (uri == null) {
            viewState = ViewState(error = true, initial = false)
        } else {
            try {
                val parser =
                    OpmlPullParser(
                        object : OPMLParserHandler {
                            override suspend fun saveFeed(feed: Feed) {
                                viewState =
                                    viewState.copy(
                                        initial = false,
                                        feeds = viewState.feeds + (feed.url to feed),
                                    )
                            }

                            override suspend fun saveSetting(
                                key: String,
                                value: String,
                            ) {
                            }

                            override suspend fun saveBlocklistPatterns(patterns: Iterable<String>) {
                            }
                        },
                    )
                val contentResolver: ContentResolver by di.instance()
                contentResolver.openInputStream(uri).use {
                    it?.let { stream ->
                        parser.parseInputStreamWithFallback(stream)
                    }
                }
            } catch (t: Throwable) {
                Log.e(LOG_TAG, "Could not import file", t)
                viewState = viewState.copy(initial = false, error = true)
            }
        }
    }

    val windowSize = LocalWindowSizeMetrics.current
    val screenHeight by remember(windowSize) {
        derivedStateOf {
            when (getScreenType(windowSize)) {
                ScreenType.DUAL -> 0.5f
                ScreenType.SINGLE -> 1.0f
            }
        }
    }

    val scrollBehavior = TopAppBarDefaults.pinnedScrollBehavior()

    SetStatusBarColorToMatchScrollableTopAppBar(scrollBehavior)

//    when (screenType) {
//        ScreenType.DUAL -> {
// //            Dialog(onDismissRequest = onDismiss) {
//            Surface {
//                OpmlImportView(
//                    screenType = screenType,
//                    viewState = viewState,
//                    modifier = Modifier,
//                    onDismiss = onDismiss,
//                    onOk = {
//                        if (uri != null) {
//                            val applicationCoroutineScope: ApplicationCoroutineScope by di.instance()
//                            applicationCoroutineScope.launch {
//                                importOpml(di, uri)
//                            }
//                        }
//                        onOk()
//                    },
//                )
//            }
// //            }
//        }
//        ScreenType.SINGLE -> {
    Scaffold(
        modifier =
            modifier
                .fillMaxHeight(screenHeight)
                .nestedScroll(scrollBehavior.nestedScrollConnection)
                .windowInsetsPadding(WindowInsets.navigationBars.only(WindowInsetsSides.Horizontal)),
        contentWindowInsets = WindowInsets.statusBars,
        topBar = {
            SensibleTopAppBar(
                scrollBehavior = scrollBehavior,
                title = stringResource(id = R.string.import_feeds_from_opml),
                navigationIcon = {
                    IconButton(onClick = onNavigateUp) {
                        Icon(
                            Icons.AutoMirrored.Filled.ArrowBack,
                            contentDescription = stringResource(R.string.go_back),
                        )
                    }
                },
            )
        },
    ) { padding ->
        OpmlImportView(
            viewState = viewState,
            modifier = Modifier.padding(padding),
            onDismiss = onDismiss,
            onOk = {
                if (uri != null) {
                    val applicationCoroutineScope: ApplicationCoroutineScope by di.instance()
                    applicationCoroutineScope.launch {
                        importOpml(di, uri)
                    }
                }
                onOk()
            },
        )
    }
//        }
//    }
}

@OptIn(ExperimentalLayoutApi::class)
@Composable
fun OpmlImportView(
    viewState: ViewState,
    onDismiss: () -> Unit,
    modifier: Modifier = Modifier,
    onOk: () -> Unit,
) {
    OkCancelWithNonScrollableContent(
        onOk = onOk,
        onCancel = onDismiss,
        okEnabled = viewState.okEnabled,
        modifier =
            modifier
                .padding(horizontal = LocalDimens.current.margin)
                .fillMaxHeight(),
    ) {
        ProvideScaledText(
            style = MaterialTheme.typography.titleMedium,
        ) {
            val text =
                when {
                    viewState.initial -> ""
                    viewState.error -> stringResource(id = R.string.failed_to_import_OPML)
                    viewState.feeds.isEmpty() -> stringResource(id = R.string.no_feeds)
                    else ->
                        stringResource(
                            id = R.string.import_x_feeds_with_y_tags,
                            pluralStringResource(
                                id = R.plurals.n_feeds,
                                count = viewState.feedCount,
                            ).format(viewState.feedCount),
                            pluralStringResource(id = R.plurals.n_tags, count = viewState.tagCount).format(
                                viewState.tagCount,
                            ),
                        )
                }
            WithBidiDeterminedLayoutDirection(paragraph = text) {
                Text(
                    text = text,
                    modifier = Modifier,
//                        .width(dimens.maxReaderWidth)
                )
            }
        }

        val feedValues by remember(viewState.feeds) {
            derivedStateOf {
                viewState.feeds.values.sortedBy { it.title }
            }
        }

        LazyColumn(
            verticalArrangement = Arrangement.spacedBy(8.dp),
            modifier =
                Modifier
                    .padding(vertical = 16.dp)
                    .fillMaxWidth()
                    .weight(1f),
        ) {
            items(
                count = feedValues.size,
            ) {
                val feed = feedValues[it]

                FlowRow(
                    horizontalArrangement = Arrangement.spacedBy(4.dp),
                ) {
                    ProvideScaledText(
                        style = MaterialTheme.typography.titleSmall,
                    ) {
                        WithBidiDeterminedLayoutDirection(paragraph = feed.title) {
                            Text(
                                text = feed.title,
                                modifier = Modifier,
                            )
                        }
                    }
                    ProvideScaledText(
                        style = MaterialTheme.typography.bodySmall,
                    ) {
                        Text(
                            text = "${feed.url}",
                            modifier = Modifier,
                        )
                    }
                }
            }
        }
    }
}

@Immutable
data class ViewState(
    val feeds: Map<URL, Feed> = mutableMapOf(),
    val error: Boolean = false,
    val initial: Boolean = true,
) {
    val okEnabled: Boolean = feeds.isNotEmpty()
    val feedCount: Int = feeds.size
    val tagCount: Int = feeds.values.asSequence().map { it.tag }.filterNot { it.isBlank() }.count()
}

@PreviewThemes
@Composable
private fun PreviewOpmlImportScreenSingle() {
    FeederTheme {
        Surface {
            OpmlImportView(
                viewState =
                    ViewState(
                        feeds =
                            mapOf(
                                URL("https://example.com/foo") to
                                    Feed(
                                        title = "Foo Feed",
                                        url = URL("https://example.com/foo"),
                                    ),
                            ),
                    ),
                onDismiss = {},
                onOk = {},
            )
        }
    }
}

package com.nononsenseapps.feeder.ui.compose.searchfeed

import android.content.res.Configuration.UI_MODE_NIGHT_NO
import android.os.Parcelable
import android.util.Log
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.navigationBars
import androidx.compose.foundation.layout.only
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBars
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material3.Card
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextField
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.Immutable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.key.Key
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalFocusManager
import androidx.compose.ui.platform.LocalSoftwareKeyboardController
import androidx.compose.ui.platform.SoftwareKeyboardController
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.testTag
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.KeyboardCapitalization
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.tooling.preview.Devices
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.model.FeedParserError
import com.nononsenseapps.feeder.model.FetchError
import com.nononsenseapps.feeder.model.FullTextDecodingFailure
import com.nononsenseapps.feeder.model.HttpError
import com.nononsenseapps.feeder.model.JsonFeedParseError
import com.nononsenseapps.feeder.model.MetaDataParseError
import com.nononsenseapps.feeder.model.NoAlternateFeeds
import com.nononsenseapps.feeder.model.NoBody
import com.nononsenseapps.feeder.model.NoUrl
import com.nononsenseapps.feeder.model.NotHTML
import com.nononsenseapps.feeder.model.NotInitializedYet
import com.nononsenseapps.feeder.model.RSSParseError
import com.nononsenseapps.feeder.model.UnsupportedContentType
import com.nononsenseapps.feeder.ui.compose.components.safeSemantics
import com.nononsenseapps.feeder.ui.compose.modifiers.interceptKey
import com.nononsenseapps.feeder.ui.compose.theme.Dimensions
import com.nononsenseapps.feeder.ui.compose.theme.FeederTheme
import com.nononsenseapps.feeder.ui.compose.theme.LocalDimens
import com.nononsenseapps.feeder.ui.compose.theme.SensibleTopAppBar
import com.nononsenseapps.feeder.ui.compose.theme.SetStatusBarColorToMatchScrollableTopAppBar
import com.nononsenseapps.feeder.ui.compose.utils.LocalWindowSizeMetrics
import com.nononsenseapps.feeder.ui.compose.utils.ScreenType
import com.nononsenseapps.feeder.ui.compose.utils.StableHolder
import com.nononsenseapps.feeder.ui.compose.utils.getScreenType
import com.nononsenseapps.feeder.ui.compose.utils.stableListHolderOf
import com.nononsenseapps.feeder.util.sloppyLinkToStrictURLNoThrows
import kotlinx.coroutines.flow.onCompletion
import kotlinx.coroutines.launch
import kotlinx.parcelize.Parcelize
import java.net.MalformedURLException
import java.net.URL

private const val LOG_TAG = "FEEDER_SEARCH"

@Suppress("ktlint:compose:vm-forwarding-check")
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SearchFeedScreen(
    onNavigateUp: () -> Unit,
    searchFeedViewModel: SearchFeedViewModel,
    modifier: Modifier = Modifier,
    initialFeedUrl: String? = null,
    onClick: (SearchResult) -> Unit,
) {
    val windowSize = LocalWindowSizeMetrics.current
    val screenType by remember(windowSize) {
        derivedStateOf {
            getScreenType(windowSize)
        }
    }

    val scrollBehavior = TopAppBarDefaults.enterAlwaysScrollBehavior()

    SetStatusBarColorToMatchScrollableTopAppBar(scrollBehavior)

    Scaffold(
        modifier =
            modifier
                .nestedScroll(scrollBehavior.nestedScrollConnection)
                .windowInsetsPadding(WindowInsets.navigationBars.only(WindowInsetsSides.Horizontal)),
        contentWindowInsets = WindowInsets.statusBars,
        topBar = {
            SensibleTopAppBar(
                scrollBehavior = scrollBehavior,
                title = stringResource(id = R.string.add_feed),
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
        SearchFeedView(
            screenType = screenType,
            searchFeedViewModel = searchFeedViewModel,
            modifier = Modifier.padding(padding),
            initialFeedUrl = initialFeedUrl ?: "",
            onClick = onClick,
        )
    }
}

@Composable
fun SearchFeedView(
    screenType: ScreenType,
    searchFeedViewModel: SearchFeedViewModel,
    modifier: Modifier = Modifier,
    initialFeedUrl: String = "",
    onClick: (SearchResult) -> Unit,
) {
    val coroutineScope = rememberCoroutineScope()

    var feedUrl by rememberSaveable {
        mutableStateOf(initialFeedUrl)
    }

    var currentlySearching by rememberSaveable {
        mutableStateOf(false)
    }

    var results by rememberSaveable {
        mutableStateOf(listOf<SearchResult>())
    }
    var errors by rememberSaveable {
        mutableStateOf(listOf<FeedParserError>())
    }

    SearchFeedView(
        screenType = screenType,
        onUrlChange = {
            feedUrl = it
        },
        onSearch = { url ->
            results = emptyList()
            errors = emptyList()
            currentlySearching = true
            coroutineScope.launch {
                searchFeedViewModel.searchForFeeds(url)
                    .onCompletion {
                        currentlySearching = false
                    }
                    .collect {
                        it.onLeft { e ->
                            errors = errors + e
                        }.onRight { r ->
                            results = results + r
                        }
                    }
            }
        },
        results = StableHolder(results),
        errors = if (currentlySearching) StableHolder(emptyList()) else StableHolder(errors),
        currentlySearching = currentlySearching,
        modifier = modifier,
        feedUrl = feedUrl,
        onClick = onClick,
    )
}

@Composable
fun SearchFeedView(
    screenType: ScreenType,
    onUrlChange: (String) -> Unit,
    onSearch: (URL) -> Unit,
    results: StableHolder<List<SearchResult>>,
    errors: StableHolder<List<FeedParserError>>,
    currentlySearching: Boolean,
    modifier: Modifier = Modifier,
    feedUrl: String = "",
    onClick: (SearchResult) -> Unit,
) {
    val focusManager = LocalFocusManager.current
    val dimens = LocalDimens.current
    val keyboardController = LocalSoftwareKeyboardController.current

    val onSearchCallback by rememberUpdatedState(newValue = onSearch)

    // If screen is opened from intent with pre-filled URL, trigger search directly
    LaunchedEffect(Unit) {
        if (results.item.isEmpty() && errors.item.isEmpty() && feedUrl.isNotBlank() &&
            isValidUrl(
                feedUrl,
            )
        ) {
            onSearchCallback(sloppyLinkToStrictURLNoThrows(feedUrl))
        }
    }

    val scrollState = rememberScrollState()

    Box(
        contentAlignment = Alignment.TopCenter,
        modifier =
            modifier
                .fillMaxWidth()
                .verticalScroll(scrollState),
    ) {
        if (screenType == ScreenType.DUAL) {
            Row(
                modifier = Modifier.width(dimens.maxContentWidth),
            ) {
                Column(
                    verticalArrangement = Arrangement.spacedBy(8.dp),
                    horizontalAlignment = Alignment.CenterHorizontally,
                    modifier =
                        Modifier
                            .weight(1f, fill = true)
                            .padding(horizontal = dimens.margin, vertical = 8.dp),
                ) {
                    leftContent(
                        feedUrl = feedUrl,
                        clearFocus = {
                            focusManager.clearFocus()
                        },
                        dimens = dimens,
                        keyboardController = keyboardController,
                        onUrlChange = onUrlChange,
                        onSearch = onSearch,
                    )
                }

                Column(
                    verticalArrangement = Arrangement.spacedBy(8.dp),
                    horizontalAlignment = Alignment.CenterHorizontally,
                    modifier =
                        Modifier
                            .weight(1f, fill = true)
                            .padding(horizontal = dimens.margin, vertical = 8.dp),
                ) {
                    rightContent(
                        results = results,
                        errors = errors,
                        currentlySearching = currentlySearching,
                        onClick = onClick,
                    )
                }
            }
        } else {
            Column(
                verticalArrangement = Arrangement.spacedBy(8.dp),
                horizontalAlignment = Alignment.CenterHorizontally,
                modifier =
                    Modifier
                        .padding(horizontal = dimens.margin, vertical = 8.dp)
                        .width(dimens.maxContentWidth),
            ) {
                leftContent(
                    feedUrl = feedUrl,
                    onUrlChange = onUrlChange,
                    onSearch = onSearch,
                    clearFocus = {
                        focusManager.clearFocus()
                    },
                    dimens = dimens,
                    keyboardController = keyboardController,
                )
                rightContent(
                    results = results,
                    errors = errors,
                    currentlySearching = currentlySearching,
                    onClick = onClick,
                )
            }
        }
    }
}

@Suppress("UnusedReceiverParameter")
@Composable
fun ColumnScope.leftContent(
    feedUrl: String,
    onUrlChange: (String) -> Unit,
    onSearch: (URL) -> Unit,
    clearFocus: () -> Unit,
    dimens: Dimensions,
    keyboardController: SoftwareKeyboardController?,
    modifier: Modifier = Modifier,
) {
    val isNotValidUrl by remember(feedUrl) {
        derivedStateOf {
            feedUrl.isNotEmpty() && isNotValidUrl(feedUrl)
        }
    }
    val isValidUrl by remember(feedUrl) {
        derivedStateOf {
            isValidUrl(feedUrl)
        }
    }
    TextField(
        value = feedUrl,
        onValueChange = onUrlChange,
        label = {
            Text(stringResource(id = R.string.add_feed_search_hint))
        },
        isError = isNotValidUrl,
        keyboardOptions =
            KeyboardOptions(
                capitalization = KeyboardCapitalization.None,
                autoCorrect = false,
                keyboardType = KeyboardType.Uri,
                imeAction = ImeAction.Search,
            ),
        keyboardActions =
            KeyboardActions(
                onSearch = {
                    if (isValidUrl) {
                        onSearch(sloppyLinkToStrictURLNoThrows(feedUrl))
                        keyboardController?.hide()
                    }
                },
            ),
        singleLine = true,
        modifier =
            modifier
                .width(dimens.maxContentWidth)
                .interceptKey(Key.Enter) {
                    if (isValidUrl(feedUrl)) {
                        onSearch(sloppyLinkToStrictURLNoThrows(feedUrl))
                        keyboardController?.hide()
                    }
                }
                .interceptKey(Key.Escape, clearFocus)
                .safeSemantics {
                    testTag = "urlField"
                },
    )

    OutlinedButton(
        enabled = isValidUrl,
        onClick = {
            if (isValidUrl) {
                try {
                    onSearch(sloppyLinkToStrictURLNoThrows(feedUrl))
                    clearFocus()
                } catch (e: Exception) {
                    Log.e(LOG_TAG, "Can't search", e)
                }
            }
        },
    ) {
        Text(
            stringResource(android.R.string.search_go),
        )
    }
}

@Composable
fun ColumnScope.rightContent(
    results: StableHolder<List<SearchResult>>,
    errors: StableHolder<List<FeedParserError>>,
    currentlySearching: Boolean,
    onClick: (SearchResult) -> Unit,
) {
    if (results.item.isEmpty()) {
        for (error in errors.item) {
            val title =
                when (error) {
                    is FetchError -> stringResource(R.string.failed_to_download)
                    is MetaDataParseError -> stringResource(R.string.failed_to_parse_the_page)
                    is NoAlternateFeeds -> stringResource(R.string.no_feeds_in_the_page)
                    is NotHTML -> stringResource(R.string.content_is_not_html)
                    is NotInitializedYet -> "Not initialized yet" // Should never happen
                    is RSSParseError -> stringResource(R.string.failed_to_parse_rss_feed)
                    is HttpError -> stringResource(R.string.http_error)
                    is JsonFeedParseError -> stringResource(R.string.failed_to_parse_json_feed)
                    is NoBody -> stringResource(R.string.no_body_in_response)
                    is UnsupportedContentType -> stringResource(R.string.unsupported_content_type)
                    is FullTextDecodingFailure -> stringResource(R.string.failed_to_parse_full_article)
                    is NoUrl -> stringResource(R.string.no_url)
                }

            ErrorResultView(
                title = title,
                description = error.description,
                url = error.url,
            ) {
                onClick(
                    SearchResult(
                        title = error.url,
                        url = error.url,
                        description = "",
                        feedImage = "",
                    ),
                )
            }
        }
    }
    for (result in results.item) {
        SearchResultView(
            title = result.title,
            url = result.url,
            description = result.description,
        ) {
            onClick(result)
        }
    }
    AnimatedVisibility(visible = currentlySearching) {
        SearchingIndicator()
    }
}

@Composable
fun SearchingIndicator(modifier: Modifier = Modifier) {
    Box(
        contentAlignment = Alignment.Center,
        modifier =
            modifier
                .fillMaxWidth()
                .safeSemantics {
                    testTag = "searchingIndicator"
                },
    ) {
        CircularProgressIndicator()
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SearchResultView(
    title: String,
    url: String,
    description: String,
    modifier: Modifier = Modifier,
    onClick: () -> Unit,
) {
    val dimens = LocalDimens.current
    Card(
        onClick = onClick,
        modifier =
            modifier
                .width(dimens.maxContentWidth)
                .safeSemantics {
                    testTag = "searchResult"
                },
    ) {
        Column(
            verticalArrangement = Arrangement.spacedBy(4.dp),
            modifier =
                Modifier
                    .width(dimens.maxContentWidth)
                    .padding(8.dp),
        ) {
            Text(
                title,
                style = MaterialTheme.typography.titleSmall,
            )
            Text(
                url,
                style = MaterialTheme.typography.bodyMedium,
            )
            Text(
                description,
                style = MaterialTheme.typography.bodyMedium,
            )
        }
    }
}

@Composable
fun ErrorResultView(
    title: String,
    description: String,
    url: String,
    modifier: Modifier = Modifier,
    onAddAnyway: () -> Unit,
) {
    val dimens = LocalDimens.current

    Card(
        modifier =
            modifier
                .width(dimens.maxContentWidth)
                .safeSemantics {
                    testTag = "errorResult"
                },
    ) {
        Column(
            verticalArrangement = Arrangement.spacedBy(4.dp),
            modifier =
                Modifier
                    .width(dimens.maxContentWidth)
                    .padding(8.dp),
        ) {
            Text(
                title,
                style =
                    MaterialTheme.typography.titleSmall
                        .copy(color = MaterialTheme.colorScheme.error),
            )
            Text(
                url,
                style = MaterialTheme.typography.bodyMedium,
            )
            Text(
                description,
                style = MaterialTheme.typography.bodyMedium,
            )
            OutlinedButton(onClick = onAddAnyway) {
                Text(stringResource(id = R.string.add_anyway))
            }
        }
    }
}

@Preview(
    name = "Search with results Phone",
    showSystemUi = true,
    device = Devices.NEXUS_5,
    uiMode = UI_MODE_NIGHT_NO,
)
@Composable
private fun SearchPreview() {
    FeederTheme {
        Surface {
            SearchFeedView(
                screenType = ScreenType.SINGLE,
                onUrlChange = {},
                onSearch = {},
                results =
                    stableListHolderOf(
                        SearchResult(
                            title = "Atom feed",
                            url = "https://cowboyprogrammer.org/atom",
                            description = "An atom feed",
                            feedImage = "",
                        ),
                        SearchResult(
                            title = "RSS feed",
                            url = "https://cowboyprogrammer.org/rss",
                            description = "An RSS feed",
                            feedImage = "",
                        ),
                    ),
                errors = stableListHolderOf(),
                currentlySearching = false,
                modifier = Modifier,
                feedUrl = "https://cowboyprogrammer.org",
            ) {}
        }
    }
}

@Preview(
    name = "Search with error Phone",
    showSystemUi = true,
    device = Devices.NEXUS_5,
    uiMode = UI_MODE_NIGHT_NO,
)
@Composable
private fun ErrorPreview() {
    FeederTheme {
        Surface {
            SearchFeedView(
                screenType = ScreenType.SINGLE,
                onUrlChange = {},
                onSearch = {},
                results = stableListHolderOf(),
                errors =
                    stableListHolderOf(
                        RSSParseError(
                            url = "https://example.com/bad",
                            throwable = NullPointerException("Missing header or something"),
                        ),
                    ),
                currentlySearching = false,
                modifier = Modifier,
                feedUrl = "https://cowboyprogrammer.org",
            ) {}
        }
    }
}

@Preview(
    name = "Search with results Foldable",
    showSystemUi = true,
    device = Devices.FOLDABLE,
    uiMode = UI_MODE_NIGHT_NO,
)
@Preview(
    name = "Search with results Tablet",
    showSystemUi = true,
    device = Devices.PIXEL_C,
    uiMode = UI_MODE_NIGHT_NO,
)
@Composable
private fun SearchPreviewLarge() {
    FeederTheme {
        Surface {
            SearchFeedView(
                screenType = ScreenType.DUAL,
                onUrlChange = {},
                onSearch = {},
                results =
                    stableListHolderOf(
                        SearchResult(
                            title = "Atom feed",
                            url = "https://cowboyprogrammer.org/atom",
                            description = "An atom feed",
                            feedImage = "",
                        ),
                        SearchResult(
                            title = "RSS feed",
                            url = "https://cowboyprogrammer.org/rss",
                            description = "An RSS feed",
                            feedImage = "",
                        ),
                    ),
                errors = stableListHolderOf(),
                currentlySearching = false,
                modifier = Modifier,
                feedUrl = "https://cowboyprogrammer.org",
            ) {}
        }
    }
}

private fun isValidUrl(url: String): Boolean {
    if (url.isBlank()) {
        return false
    }
    return try {
        try {
            URL(url)
            true
        } catch (_: MalformedURLException) {
            URL("http://$url")
            true
        }
    } catch (e: Exception) {
        false
    }
}

private fun isNotValidUrl(url: String) = !isValidUrl(url)

@Immutable
@Parcelize
data class SearchResult(
    val title: String,
    val url: String,
    val description: String,
    // Empty string instead of null because query params
    val feedImage: String,
) : Parcelable

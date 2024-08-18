package com.nononsenseapps.feeder.ui.compose.editfeed

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.focusGroup
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.navigationBars
import androidx.compose.foundation.layout.only
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBars
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextField
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.Stable
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.ExperimentalComposeUiApi
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusDirection
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusProperties
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.focus.onFocusChanged
import androidx.compose.ui.input.key.Key
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalFocusManager
import androidx.compose.ui.platform.LocalSoftwareKeyboardController
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.KeyboardCapitalization
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.tooling.preview.Devices
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.google.accompanist.permissions.ExperimentalPermissionsApi
import com.google.accompanist.permissions.PermissionStatus
import com.google.accompanist.permissions.shouldShowRationale
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.archmodel.PREF_VAL_OPEN_WITH_BROWSER
import com.nononsenseapps.feeder.archmodel.PREF_VAL_OPEN_WITH_CUSTOM_TAB
import com.nononsenseapps.feeder.archmodel.PREF_VAL_OPEN_WITH_READER
import com.nononsenseapps.feeder.ui.compose.components.AutoCompleteResults
import com.nononsenseapps.feeder.ui.compose.components.OkCancelWithContent
import com.nononsenseapps.feeder.ui.compose.feed.ExplainPermissionDialog
import com.nononsenseapps.feeder.ui.compose.modifiers.interceptKey
import com.nononsenseapps.feeder.ui.compose.settings.GroupTitle
import com.nononsenseapps.feeder.ui.compose.settings.RadioButtonSetting
import com.nononsenseapps.feeder.ui.compose.settings.SwitchSetting
import com.nononsenseapps.feeder.ui.compose.theme.FeederTheme
import com.nononsenseapps.feeder.ui.compose.theme.LocalDimens
import com.nononsenseapps.feeder.ui.compose.theme.SensibleTopAppBar
import com.nononsenseapps.feeder.ui.compose.theme.SetStatusBarColorToMatchScrollableTopAppBar
import com.nononsenseapps.feeder.ui.compose.utils.ImmutableHolder
import com.nononsenseapps.feeder.ui.compose.utils.LocalWindowSizeMetrics
import com.nononsenseapps.feeder.ui.compose.utils.ScreenType
import com.nononsenseapps.feeder.ui.compose.utils.getScreenType
import com.nononsenseapps.feeder.ui.compose.utils.rememberApiPermissionState

@Composable
fun CreateFeedScreen(
    onNavigateUp: () -> Unit,
    createFeedScreenViewModel: CreateFeedScreenViewModel,
    onOk: (Long) -> Unit,
) {
    val windowSize = LocalWindowSizeMetrics.current

    val screenType by remember(windowSize) {
        derivedStateOf {
            getScreenType(windowSize)
        }
    }

    @Suppress("ktlint:compose:vm-forwarding-check")
    EditFeedScreen(
        onNavigateUp = onNavigateUp,
        viewState = createFeedScreenViewModel,
        screenType = screenType,
        onOk = {
            createFeedScreenViewModel.saveAndRequestSync { feedId ->
                onOk(feedId)
            }
        },
        onCancel = {
            onNavigateUp()
        },
    )
}

@Composable
fun EditFeedScreen(
    onNavigateUp: () -> Unit,
    editFeedScreenViewModel: EditFeedScreenViewModel,
    onOk: (Long) -> Unit,
) {
    val windowSize = LocalWindowSizeMetrics.current

    val screenType by remember(windowSize) {
        derivedStateOf {
            getScreenType(windowSize)
        }
    }

    @Suppress("ktlint:compose:vm-forwarding-check")
    EditFeedScreen(
        onNavigateUp = onNavigateUp,
        viewState = editFeedScreenViewModel,
        screenType = screenType,
        onOk = {
            editFeedScreenViewModel.saveAndRequestSync { feedId ->
                onOk(feedId)
            }
        },
        onCancel = {
            onNavigateUp()
        },
    )
}

@OptIn(ExperimentalMaterial3Api::class, ExperimentalPermissionsApi::class)
@Composable
fun EditFeedScreen(
    onNavigateUp: () -> Unit,
    viewState: EditFeedScreenState,
    screenType: ScreenType,
    onOk: () -> Unit,
    onCancel: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val notificationsPermissionState =
        rememberApiPermissionState(
            permission = "android.permission.POST_NOTIFICATIONS",
            minimumApiLevel = 33,
        ) { value ->
            viewState.notify = value
        }

    val shouldShowExplanationForPermission by remember {
        derivedStateOf {
            notificationsPermissionState.status.shouldShowRationale
        }
    }

    var permissionDismissed by rememberSaveable {
        mutableStateOf(true)
    }

    val screenState =
        remember {
            object : EditFeedScreenState by viewState {
                override var notify: Boolean
                    get() = viewState.notify
                    set(value) {
                        if (!value) {
                            viewState.notify = false
                        } else {
                            when (notificationsPermissionState.status) {
                                is PermissionStatus.Denied -> {
                                    if (notificationsPermissionState.status.shouldShowRationale) {
                                        // Dialog is shown inside EditFeedScreen with a button
                                        permissionDismissed = false
                                    } else {
                                        notificationsPermissionState.launchPermissionRequest()
                                    }
                                }

                                PermissionStatus.Granted -> viewState.notify = true
                            }
                        }
                    }
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
                title = stringResource(id = R.string.edit_feed),
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
        EditFeedView(
            screenType = screenType,
            viewState = screenState,
            onOk = onOk,
            onCancel = onCancel,
            modifier = Modifier.padding(padding),
        )

        if (shouldShowExplanationForPermission && !permissionDismissed) {
            ExplainPermissionDialog(
                explanation = R.string.explanation_permission_notifications,
                onDismiss = {
                    permissionDismissed = true
                },
            ) {
                notificationsPermissionState.launchPermissionRequest()
            }
        }
    }
}

@OptIn(ExperimentalComposeUiApi::class)
@Composable
fun EditFeedView(
    screenType: ScreenType,
    viewState: EditFeedScreenState,
    onOk: () -> Unit,
    onCancel: () -> Unit,
    modifier: Modifier = Modifier,
) {
    val dimens = LocalDimens.current

    val leftFocusRequester = remember { FocusRequester() }
    val rightFocusRequester = remember { FocusRequester() }

    OkCancelWithContent(
        onOk = {
            onOk()
        },
        onCancel = onCancel,
        okEnabled = viewState.isOkToSave,
        modifier =
            modifier
                .padding(horizontal = LocalDimens.current.margin),
    ) {
        if (screenType == ScreenType.DUAL) {
            Row(
                modifier = Modifier.width(dimens.maxContentWidth),
            ) {
                Column(
                    verticalArrangement = Arrangement.spacedBy(8.dp),
                    modifier =
                        Modifier
                            .weight(1f, fill = true)
                            .padding(horizontal = dimens.margin, vertical = 8.dp),
                ) {
                    LeftContent(
                        viewState = viewState,
                        rightFocusRequester = rightFocusRequester,
                    )
                }

                Column(
                    verticalArrangement = Arrangement.spacedBy(8.dp),
                    modifier =
                        Modifier
                            .weight(1f, fill = true)
                            .padding(horizontal = dimens.margin, vertical = 8.dp),
                ) {
                    RightContent(
                        viewState = viewState,
                        leftFocusRequester = leftFocusRequester,
                        rightFocusRequester = rightFocusRequester,
                    )
                }
            }
        } else {
            Column(
                verticalArrangement = Arrangement.spacedBy(8.dp),
                modifier = Modifier.width(dimens.maxContentWidth),
            ) {
                LeftContent(
                    viewState = viewState,
                    rightFocusRequester = rightFocusRequester,
                )

                HorizontalDivider(modifier = Modifier.fillMaxWidth())

                RightContent(
                    viewState = viewState,
                    leftFocusRequester = leftFocusRequester,
                    rightFocusRequester = rightFocusRequester,
                )
            }
        }
    }
}

@Suppress("ktlint:compose:modifier-missing-check")
@OptIn(
    ExperimentalComposeUiApi::class,
    ExperimentalFoundationApi::class,
)
@Composable
fun ColumnScope.LeftContent(
    viewState: EditFeedScreenState,
    rightFocusRequester: FocusRequester,
) {
    val filteredTags by remember(viewState.allTags, viewState.feedTag) {
        derivedStateOf {
            ImmutableHolder(
                viewState.allTags.filter { tag ->
                    tag.isNotBlank() && tag.startsWith(viewState.feedTag, ignoreCase = true)
                },
            )
        }
    }

    val focusManager = LocalFocusManager.current
    val keyboardController = LocalSoftwareKeyboardController.current

    var showTagSuggestions by rememberSaveable { mutableStateOf(false) }

    TextField(
        value = viewState.feedUrl,
        onValueChange = { viewState.feedUrl = it.trim() },
        label = {
            Text(stringResource(id = R.string.url))
        },
        isError = viewState.isNotValidUrl,
        keyboardOptions =
            KeyboardOptions(
                capitalization = KeyboardCapitalization.None,
                autoCorrect = false,
                keyboardType = KeyboardType.Uri,
                imeAction = ImeAction.Next,
            ),
        keyboardActions =
            KeyboardActions(
                onNext = {
                    focusManager.moveFocus(focusDirection = FocusDirection.Down)
                },
            ),
        singleLine = true,
        modifier =
            Modifier
                .fillMaxWidth()
                .heightIn(min = 64.dp)
                .interceptKey(Key.Enter) {
                    focusManager.moveFocus(FocusDirection.Down)
                },
    )
    AnimatedVisibility(visible = viewState.isNotValidUrl) {
        Text(
            textAlign = TextAlign.Center,
            text = stringResource(R.string.invalid_url),
            style = MaterialTheme.typography.labelMedium.copy(color = MaterialTheme.colorScheme.error),
        )
    }
    OutlinedTextField(
        value = viewState.feedTitle,
        onValueChange = { viewState.feedTitle = it },
        placeholder = {
            Text(viewState.defaultTitle)
        },
        label = {
            Text(stringResource(id = R.string.title))
        },
        singleLine = true,
        keyboardOptions =
            KeyboardOptions(
                capitalization = KeyboardCapitalization.Words,
                autoCorrect = true,
                keyboardType = KeyboardType.Text,
                imeAction = ImeAction.Next,
            ),
        keyboardActions =
            KeyboardActions(
                onNext = {
                    focusManager.moveFocus(focusDirection = FocusDirection.Down)
                },
            ),
        modifier =
            Modifier
                .fillMaxWidth()
                .heightIn(min = 64.dp)
                .interceptKey(Key.Enter) {
                    focusManager.moveFocus(FocusDirection.Down)
                },
    )

    AutoCompleteResults(
        modifier =
            Modifier
                .focusGroup()
                .onFocusChanged {
                    // Someone in hierarchy has focus - different from isFocused
                    showTagSuggestions = it.hasFocus
                },
        displaySuggestions = showTagSuggestions,
        suggestions = filteredTags,
        onSuggestionClick = { tag ->
            viewState.feedTag = tag
            rightFocusRequester.requestFocus()
            showTagSuggestions = false
        },
        suggestionContent = {
            Box(
                contentAlignment = Alignment.CenterStart,
                modifier =
                    Modifier
                        .padding(horizontal = 16.dp)
                        .height(48.dp)
                        .fillMaxWidth(),
            ) {
                Text(
                    text = it,
                    style = MaterialTheme.typography.titleMedium,
                )
            }
        },
    ) {
        OutlinedTextField(
            value = viewState.feedTag,
            onValueChange = { viewState.feedTag = it },
            label = {
                Text(stringResource(id = R.string.tag))
            },
            singleLine = true,
            keyboardOptions =
                KeyboardOptions(
                    capitalization = KeyboardCapitalization.Words,
                    autoCorrect = true,
                    keyboardType = KeyboardType.Text,
                    imeAction = ImeAction.Done,
                ),
            keyboardActions =
                KeyboardActions(
                    onDone = {
                        showTagSuggestions = false
                        keyboardController?.hide()
                        rightFocusRequester.requestFocus()
                    },
                ),
            modifier =
                Modifier
                    .fillMaxWidth()
                    .heightIn(min = 64.dp)
                    .interceptKey(Key.Enter) {
                        rightFocusRequester.requestFocus()
                    },
        )
    }
}

@Suppress("ktlint:compose:modifier-missing-check", "UnusedReceiverParameter")
@Composable
fun ColumnScope.RightContent(
    viewState: EditFeedScreenState,
    leftFocusRequester: FocusRequester,
    rightFocusRequester: FocusRequester,
) {
    SwitchSetting(
        title = stringResource(id = R.string.fetch_full_articles_by_default),
        checked = viewState.fullTextByDefault,
        icon = null,
        modifier =
            Modifier
                .focusRequester(rightFocusRequester)
                .focusProperties {
                    previous = leftFocusRequester
                },
    ) { viewState.fullTextByDefault = it }
    SwitchSetting(
        title = stringResource(id = R.string.notify_for_new_items),
        checked = viewState.notify,
        icon = null,
    ) { viewState.notify = it }
    SwitchSetting(
        title = stringResource(id = R.string.skip_duplicate_articles),
        description = stringResource(id = R.string.skip_duplicate_articles_desc),
        checked = viewState.skipDuplicates,
        icon = null,
    ) { viewState.skipDuplicates = it }
    SwitchSetting(
        title = stringResource(id = R.string.generate_extra_unique_ids),
        checked = viewState.alternateId,
        description = stringResource(id = R.string.only_enable_for_bad_id_feeds),
        icon = null,
    ) { viewState.alternateId = it }
    HorizontalDivider(modifier = Modifier.fillMaxWidth())
    GroupTitle(
        startingSpace = false,
        height = 48.dp,
    ) {
        Text(stringResource(id = R.string.open_item_by_default_with))
    }
    RadioButtonSetting(
        title = stringResource(id = R.string.use_app_default),
        selected = viewState.isOpenItemWithAppDefault,
        minHeight = 48.dp,
        icon = null,
        onClick = {
            viewState.articleOpener = ""
        },
    )
    RadioButtonSetting(
        title = stringResource(id = R.string.open_in_reader),
        selected = viewState.isOpenItemWithReader,
        minHeight = 48.dp,
        icon = null,
        onClick = {
            viewState.articleOpener = PREF_VAL_OPEN_WITH_READER
        },
    )
    RadioButtonSetting(
        title = stringResource(id = R.string.open_in_custom_tab),
        selected = viewState.isOpenItemWithCustomTab,
        minHeight = 48.dp,
        icon = null,
        onClick = {
            viewState.articleOpener = PREF_VAL_OPEN_WITH_CUSTOM_TAB
        },
    )
    RadioButtonSetting(
        title = stringResource(id = R.string.open_in_default_browser),
        selected = viewState.isOpenItemWithBrowser,
        minHeight = 48.dp,
        icon = null,
        onClick = {
            viewState.articleOpener = PREF_VAL_OPEN_WITH_BROWSER
        },
    )
}

@Stable
interface EditFeedScreenState {
    var feedUrl: String
    var feedTitle: String
    var feedTag: String
    var fullTextByDefault: Boolean
    var notify: Boolean
    var skipDuplicates: Boolean
    var articleOpener: String
    var alternateId: Boolean
    val isOkToSave: Boolean
    val isNotValidUrl: Boolean
    val isOpenItemWithBrowser: Boolean
    val isOpenItemWithCustomTab: Boolean
    val isOpenItemWithReader: Boolean
    val isOpenItemWithAppDefault: Boolean
    val allTags: List<String>
    val defaultTitle: String
    val feedImage: String
}

fun EditFeedScreenState(): EditFeedScreenState = ScreenState()

private class ScreenState(
    override val isOkToSave: Boolean = false,
    override val isNotValidUrl: Boolean = false,
    override val isOpenItemWithBrowser: Boolean = false,
    override val isOpenItemWithCustomTab: Boolean = false,
    override val isOpenItemWithReader: Boolean = false,
    override val isOpenItemWithAppDefault: Boolean = false,
    override val allTags: List<String> = emptyList(),
    override val defaultTitle: String = "",
    override val feedImage: String = "",
) : EditFeedScreenState {
    override var feedUrl: String by mutableStateOf("")
    override var feedTitle: String by mutableStateOf("")
    override var feedTag: String by mutableStateOf("")
    override var fullTextByDefault: Boolean by mutableStateOf(false)
    override var notify: Boolean by mutableStateOf(false)
    override var skipDuplicates: Boolean by mutableStateOf(false)
    override var articleOpener: String by mutableStateOf("")
    override var alternateId: Boolean by mutableStateOf(false)
}

@Preview("Edit Feed Phone")
@Composable
private fun PreviewEditFeedScreenPhone() {
    FeederTheme {
        EditFeedScreen(
            screenType = ScreenType.SINGLE,
            onNavigateUp = {},
            onOk = {},
            onCancel = {},
            viewState = EditFeedScreenState(),
        )
    }
}

@Preview("Edit Feed Foldable", device = Devices.FOLDABLE)
@Preview("Edit Feed Tablet", device = Devices.PIXEL_C)
@Composable
private fun PreviewEditFeedScreenLarge() {
    FeederTheme {
        EditFeedScreen(
            screenType = ScreenType.DUAL,
            onNavigateUp = {},
            onOk = {},
            onCancel = {},
            viewState = EditFeedScreenState(),
        )
    }
}

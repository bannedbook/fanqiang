package com.nononsenseapps.feeder.ui.compose.sync

import android.app.Activity.RESULT_OK
import android.content.ActivityNotFoundException
import android.content.Intent
import android.util.Log
import androidx.activity.compose.BackHandler
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.foundation.Image
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.navigationBars
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.only
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.statusBars
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TextField
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.material3.windowsizeclass.WindowSizeClass
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.role
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.tooling.preview.Devices
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.nononsenseapps.feeder.BuildConfig
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.crypto.AesCbcWithIntegrity
import com.nononsenseapps.feeder.db.room.ID_UNSET
import com.nononsenseapps.feeder.db.room.SyncDevice
import com.nononsenseapps.feeder.ui.compose.components.safeSemantics
import com.nononsenseapps.feeder.ui.compose.feed.PlainTooltipBox
import com.nononsenseapps.feeder.ui.compose.minimumTouchSize
import com.nononsenseapps.feeder.ui.compose.theme.FeederTheme
import com.nononsenseapps.feeder.ui.compose.theme.LinkTextStyle
import com.nononsenseapps.feeder.ui.compose.theme.LocalDimens
import com.nononsenseapps.feeder.ui.compose.theme.SensibleTopAppBar
import com.nononsenseapps.feeder.ui.compose.theme.SetStatusBarColorToMatchScrollableTopAppBar
import com.nononsenseapps.feeder.ui.compose.utils.ImmutableHolder
import com.nononsenseapps.feeder.ui.compose.utils.LocalWindowSizeMetrics
import com.nononsenseapps.feeder.ui.compose.utils.ScreenType
import com.nononsenseapps.feeder.ui.compose.utils.getScreenType
import com.nononsenseapps.feeder.ui.compose.utils.onKeyEventLikeEscape
import com.nononsenseapps.feeder.util.ActivityLauncher
import com.nononsenseapps.feeder.util.DEEP_LINK_BASE_URI
import com.nononsenseapps.feeder.util.KOFI_URL
import com.nononsenseapps.feeder.util.openKoFiIntent
import net.glxn.qrgen.android.QRCode
import net.glxn.qrgen.core.scheme.Url
import org.kodein.di.compose.LocalDI
import org.kodein.di.instance
import java.net.URL
import java.net.URLDecoder

private const val LOG_TAG = "FEEDER_SYNCSCREEN"

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun SyncScaffold(
    onNavigateUp: () -> Unit,
    leaveSyncVisible: Boolean,
    onLeaveSyncChain: () -> Unit,
    title: String,
    modifier: Modifier = Modifier,
    content: @Composable (Modifier) -> Unit,
) {
    var showToolbar by rememberSaveable {
        mutableStateOf(false)
    }
    val scrollBehavior = TopAppBarDefaults.enterAlwaysScrollBehavior()
    val closeMenuText = stringResource(R.string.close_menu)

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
                title = title,
                navigationIcon = {
                    IconButton(onClick = onNavigateUp) {
                        Icon(
                            Icons.AutoMirrored.Filled.ArrowBack,
                            contentDescription = stringResource(R.string.go_back),
                        )
                    }
                },
                actions = {
                    if (leaveSyncVisible) {
                        PlainTooltipBox(tooltip = { Text(stringResource(id = R.string.open_menu)) }) {
                            Box {
                                IconButton(
                                    onClick = { showToolbar = true },
                                ) {
                                    Icon(
                                        Icons.Default.MoreVert,
                                        contentDescription = stringResource(id = R.string.open_menu),
                                    )
                                }
                                DropdownMenu(
                                    expanded = showToolbar,
                                    onDismissRequest = { showToolbar = false },
                                    modifier =
                                        Modifier.onKeyEventLikeEscape {
                                            showToolbar = false
                                        },
                                ) {
                                    DropdownMenuItem(
                                        onClick = {
                                            showToolbar = false
                                            onLeaveSyncChain()
                                        },
                                        text = {
                                            Text(stringResource(R.string.leave_sync_chain))
                                        },
                                    )
                                    // Hidden button for TalkBack
                                    DropdownMenuItem(
                                        onClick = {
                                            showToolbar = false
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
                },
            )
        },
        content = {
            content(
                Modifier
                    .padding(it)
                    .navigationBarsPadding(),
            )
        },
    )
}

@Composable
fun SyncScreen(
    onNavigateUp: () -> Unit,
    viewModel: SyncScreenViewModel,
) {
    val viewState: SyncScreenViewState by viewModel.viewState.collectAsStateWithLifecycle()

    val windowSize = LocalWindowSizeMetrics.current
    val syncScreenType =
        getSyncScreenType(
            windowSize = windowSize,
            viewState = viewState,
        )

    var previousScreen: SyncScreenType? by remember {
        mutableStateOf(null)
    }

    LaunchedEffect(Unit) {
        viewModel.updateDeviceList()
    }

    val launcher =
        rememberLauncherForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
            if (result.resultCode == RESULT_OK) {
                result.data?.getStringExtra("SCAN_RESULT")?.let {
                    val code = it.syncCodeQueryParam
                    if (code.isNotBlank()) {
                        viewModel.setSyncCode(code)
                    }
                    val secretKey = it.secretKeyQueryParam
                    if (secretKey.isNotBlank()) {
                        viewModel.setSecretKey(secretKey)
                    }
                }
            }
        }

    var showLeaveSyncChainDialog by rememberSaveable {
        mutableStateOf(false)
    }

    SyncScreen(
        viewState = viewState,
        targetScreen = syncScreenType,
        previousScreen = previousScreen,
        onLeaveSyncSettings = onNavigateUp,
        onLeaveAddDevice = {
            previousScreen = SyncScreenType.SINGLE_ADD_DEVICE
            viewModel.setScreen(SyncScreenToShow.DEVICELIST)
        },
        onLeaveSyncJoin = {
            previousScreen = SyncScreenType.SINGLE_JOIN
            viewModel.setScreen(SyncScreenToShow.SETUP)
        },
        onJoinSyncChain = { syncCode, secretKey ->
            viewModel.joinSyncChain(syncCode = syncCode, secretKey = secretKey)
        },
        onAddNewDevice = {
            previousScreen = SyncScreenType.SINGLE_DEVICELIST
            viewModel.setScreen(SyncScreenToShow.ADD_DEVICE)
        },
        onDeleteDevice = {
            if (it == viewState.deviceId) {
                viewModel.leaveSyncChain()
            } else {
                viewModel.removeDevice(it)
            }
        },
        onLeaveSyncChain = {
            previousScreen = SyncScreenType.SINGLE_DEVICELIST
            showLeaveSyncChainDialog = true
        },
        onScanSyncCode = {
            viewModel.setScreen(SyncScreenToShow.JOIN)
            // Open barcode scanner on open in case initialSyncCode is empty
            if (viewState.syncCode.isEmpty()) {
                try {
                    launcher.launch(Intent("com.google.zxing.client.android.SCAN"))
                } catch (e: ActivityNotFoundException) {
                    viewModel.onMissingBarCodeScanner()
                }
            }
        },
        onStartNewSyncChain = {
            viewModel.startNewSyncChain()
        },
        onSetSyncCode = {
            viewModel.setSyncCode(it)
        },
        onSetSecretKey = {
            viewModel.setSecretKey(it)
        },
        currentDeviceId = viewState.deviceId,
        devices = ImmutableHolder(viewState.deviceList),
    )

    if (showLeaveSyncChainDialog) {
        LeaveSyncChainDialog(
            onDismiss = {
                showLeaveSyncChainDialog = false
            },
            onOk = {
                showLeaveSyncChainDialog = false
                viewModel.leaveSyncChain()
            },
        )
    }
}

@Composable
fun SyncScreen(
    viewState: SyncScreenViewState,
    targetScreen: SyncScreenType,
    previousScreen: SyncScreenType?,
    onLeaveSyncSettings: () -> Unit,
    onLeaveAddDevice: () -> Unit,
    onLeaveSyncJoin: () -> Unit,
    onJoinSyncChain: (String, String) -> Unit,
    onAddNewDevice: () -> Unit,
    onDeleteDevice: (Long) -> Unit,
    onScanSyncCode: () -> Unit,
    onStartNewSyncChain: () -> Unit,
    onSetSyncCode: (String) -> Unit,
    onSetSecretKey: (String) -> Unit,
    currentDeviceId: Long,
    devices: ImmutableHolder<List<SyncDevice>>,
    onLeaveSyncChain: () -> Unit,
) {
    if (targetScreen == SyncScreenType.DUAL) {
        DualSyncScreen(
            onNavigateUp = onLeaveSyncSettings,
            leftScreenToShow = viewState.leftScreenToShow,
            rightScreenToShow = viewState.rightScreenToShow,
            onScanSyncCode = onScanSyncCode,
            onStartNewSyncChain = onStartNewSyncChain,
            onAddNewDevice = onAddNewDevice,
            onDeleteDevice = onDeleteDevice,
            currentDeviceId = currentDeviceId,
            devices = devices,
            addDeviceUrl = ImmutableHolder(viewState.addNewDeviceUrl),
            onJoinSyncChain = onJoinSyncChain,
            syncCode = viewState.syncCode,
            onSetSyncCode = onSetSyncCode,
            onLeaveSyncChain = onLeaveSyncChain,
            secretKey = viewState.secretKey,
            onSetSecretKey = onSetSecretKey,
        )
    }

    AnimatedVisibility(
        visible = targetScreen == SyncScreenType.SINGLE_SETUP,
        enter =
            when (previousScreen) {
                null -> fadeIn(initialAlpha = 1.0f)
                else -> fadeIn()
            },
            /*
        This may seem weird - but it's a special case. This exit animation actually runs
        when the first screen is device list. So to prevent a flicker effect it's important to block
        sideways movement. The setup screen will be momentarily on screen because it takes
        a few millis to fetch the sync remote.
             */
        exit =
            when (previousScreen) {
                null -> fadeOut(targetAlpha = 1.0f)
                else -> fadeOut()
            },
    ) {
        SyncSetupScreen(
            onNavigateUp = onLeaveSyncSettings,
            onScanSyncCode = onScanSyncCode,
            onStartNewSyncChain = onStartNewSyncChain,
            onLeaveSyncChain = onLeaveSyncChain,
        )
    }

    AnimatedVisibility(
        visible = targetScreen == SyncScreenType.SINGLE_JOIN,
        enter = fadeIn(),
        exit = fadeOut(),
    ) {
        SyncJoinScreen(
            onNavigateUp = onLeaveSyncJoin,
            onJoinSyncChain = onJoinSyncChain,
            syncCode = viewState.syncCode,
            onSetSyncCode = onSetSyncCode,
            onLeaveSyncChain = onLeaveSyncChain,
            secretKey = viewState.secretKey,
            onSetSecretKey = onSetSecretKey,
        )
    }

    AnimatedVisibility(
        visible = targetScreen == SyncScreenType.SINGLE_DEVICELIST,
        enter =
            when (previousScreen) {
                SyncScreenType.SINGLE_ADD_DEVICE -> fadeIn()
                null -> fadeIn(initialAlpha = 1.0f)
                else -> fadeIn()
            },
        exit = fadeOut(),
    ) {
        SyncDeviceListScreen(
            onNavigateUp = onLeaveSyncSettings,
            currentDeviceId = currentDeviceId,
            devices = devices,
            onAddNewDevice = onAddNewDevice,
            onDeleteDevice = onDeleteDevice,
            onLeaveSyncChain = onLeaveSyncChain,
        )
    }

    AnimatedVisibility(
        visible = targetScreen == SyncScreenType.SINGLE_ADD_DEVICE,
        enter = fadeIn(),
        exit = fadeOut(),
    ) {
        SyncAddNewDeviceScreen(
            onNavigateUp = onLeaveAddDevice,
            syncUrl = ImmutableHolder(viewState.addNewDeviceUrl),
            onLeaveSyncChain = onLeaveSyncChain,
        )
    }
}

enum class SyncScreenType {
    DUAL,
    SINGLE_SETUP,
    SINGLE_DEVICELIST,
    SINGLE_ADD_DEVICE,
    SINGLE_JOIN,
}

fun getSyncScreenType(
    windowSize: WindowSizeClass,
    viewState: SyncScreenViewState,
): SyncScreenType =
    when (getScreenType(windowSize)) {
        ScreenType.SINGLE -> {
            when (viewState.singleScreenToShow) {
                SyncScreenToShow.SETUP -> SyncScreenType.SINGLE_SETUP
                SyncScreenToShow.DEVICELIST -> SyncScreenType.SINGLE_DEVICELIST
                SyncScreenToShow.ADD_DEVICE -> SyncScreenType.SINGLE_ADD_DEVICE
                SyncScreenToShow.JOIN -> SyncScreenType.SINGLE_JOIN
            }
        }

        ScreenType.DUAL -> SyncScreenType.DUAL
    }

@Composable
fun DualSyncScreen(
    onNavigateUp: () -> Unit,
    leftScreenToShow: LeftScreenToShow,
    rightScreenToShow: RightScreenToShow,
    onScanSyncCode: () -> Unit,
    onStartNewSyncChain: () -> Unit,
    onAddNewDevice: () -> Unit,
    onDeleteDevice: (Long) -> Unit,
    currentDeviceId: Long,
    devices: ImmutableHolder<List<SyncDevice>>,
    addDeviceUrl: ImmutableHolder<URL>,
    onJoinSyncChain: (String, String) -> Unit,
    syncCode: String,
    onSetSyncCode: (String) -> Unit,
    secretKey: String,
    onSetSecretKey: (String) -> Unit,
    modifier: Modifier = Modifier,
    onLeaveSyncChain: () -> Unit,
) {
    BackHandler(onBack = onNavigateUp)

    val scrollState = rememberScrollState()

    LaunchedEffect(key1 = leftScreenToShow) {
        scrollState.scrollTo(0)
    }

    SyncScaffold(
        leaveSyncVisible = leftScreenToShow == LeftScreenToShow.DEVICELIST,
        title = stringResource(id = R.string.device_sync),
        onNavigateUp = onNavigateUp,
        onLeaveSyncChain = onLeaveSyncChain,
        modifier = modifier,
    ) { innerModifier ->
        Row(
            modifier =
                innerModifier
                    .verticalScroll(scrollState),
        ) {
            when (leftScreenToShow) {
                LeftScreenToShow.SETUP -> {
                    SyncSetupContent(
                        onScanSyncCode = onScanSyncCode,
                        modifier =
                            Modifier
                                .weight(1f, fill = true),
                        onStartNewSyncChain = onStartNewSyncChain,
                    )
                }

                LeftScreenToShow.DEVICELIST -> {
                    SyncDeviceListContent(
                        currentDeviceId = currentDeviceId,
                        devices = devices,
                        onAddNewDevice = onAddNewDevice,
                        onDeleteDevice = onDeleteDevice,
                        showAddDeviceButton = false,
                        modifier =
                            Modifier
                                .weight(1f, fill = true),
                    )
                }
            }

            when (rightScreenToShow) {
                RightScreenToShow.ADD_DEVICE -> {
                    SyncAddNewDeviceContent(
                        syncUrl = addDeviceUrl,
                        modifier =
                            Modifier
                                .weight(1f, fill = true),
                    )
                }

                RightScreenToShow.JOIN -> {
                    SyncJoinContent(
                        onJoinSyncChain = onJoinSyncChain,
                        syncCode = syncCode,
                        onSetSyncCode = onSetSyncCode,
                        secretKey = secretKey,
                        modifier =
                            Modifier
                                .weight(1f, fill = true),
                        onSetSecretKey = onSetSecretKey,
                    )
                }
            }
        }
    }
}

@Composable
fun SyncSetupScreen(
    onNavigateUp: () -> Unit,
    onScanSyncCode: () -> Unit,
    onStartNewSyncChain: () -> Unit,
    modifier: Modifier = Modifier,
    onLeaveSyncChain: () -> Unit,
) {
    BackHandler(onBack = onNavigateUp)
    val scrollState = rememberScrollState()

    SyncScaffold(
        leaveSyncVisible = false,
        title = stringResource(id = R.string.device_sync),
        onNavigateUp = onNavigateUp,
        onLeaveSyncChain = onLeaveSyncChain,
        modifier = modifier,
    ) { innerModifier ->
        SyncSetupContent(
            onScanSyncCode = onScanSyncCode,
            modifier = innerModifier.verticalScroll(scrollState),
            onStartNewSyncChain = onStartNewSyncChain,
        )
    }
}

@Composable
fun SyncSetupContent(
    onScanSyncCode: () -> Unit,
    modifier: Modifier = Modifier,
    onStartNewSyncChain: () -> Unit,
) {
    val dimens = LocalDimens.current
    val activityLauncher: ActivityLauncher by LocalDI.current.instance()

    Column(
        verticalArrangement = Arrangement.spacedBy(8.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        modifier =
            modifier
                .fillMaxSize()
                .padding(horizontal = dimens.margin, vertical = 8.dp),
    ) {
        Text(
            text = stringResource(R.string.device_sync_description_1),
            style = MaterialTheme.typography.bodyLarge,
            modifier = Modifier.fillMaxWidth(),
        )
        Text(
            text = stringResource(R.string.device_sync_description_2),
            style = MaterialTheme.typography.bodyLarge,
            modifier = Modifier.fillMaxWidth(),
        )
        Text(
            text = stringResource(R.string.device_sync_financed_by_community),
            style = MaterialTheme.typography.bodyLarge,
            modifier = Modifier.fillMaxWidth(),
        )
        // Google Play does not allow direct donation links
        if (!BuildConfig.BUILD_TYPE.contains("play", ignoreCase = true)) {
            Text(
                text = KOFI_URL,
                style = MaterialTheme.typography.bodyLarge.merge(LinkTextStyle()),
                modifier =
                    Modifier
                        .fillMaxWidth()
                        .clickable {
                            activityLauncher.startActivity(
                                openAdjacentIfSuitable = true,
                                intent = openKoFiIntent(),
                            )
                        },
            )
        }
        // Let this be hard-coded. It should not be localized.
        Text(
            text = "WARNING! This is a Beta feature. Do an OPML-export of all your feeds before and save as a backup.",
            style = MaterialTheme.typography.titleMedium,
            modifier = Modifier.fillMaxWidth(),
        )

        Spacer(modifier = Modifier.size(24.dp))

        Button(
            onClick = onScanSyncCode,
        ) {
            Text(
                text = stringResource(R.string.scan_or_enter_code),
                style = MaterialTheme.typography.labelLarge,
            )
        }
        TextButton(
            onClick = onStartNewSyncChain,
        ) {
            Text(
                text = stringResource(R.string.start_new_sync_chain),
                style = MaterialTheme.typography.labelLarge,
            )
        }
    }
}

internal val String.syncCodeQueryParam
    get() = substringAfter("sync_code=").take(64)

internal val String.secretKeyQueryParam
    get() =
        substringAfter("key=")
            .substringBefore("&")
            .let {
                // Deeplinks are already decoded - but not if you scan a QR code
                if ("%3A" in it) {
                    try {
                        URLDecoder.decode(it, "UTF-8")
                    } catch (e: Exception) {
                        Log.e(LOG_TAG, "Failed to decode secret key", e)
                        it
                    }
                } else {
                    it
                }
            }

@Composable
fun SyncJoinScreen(
    onNavigateUp: () -> Unit,
    onJoinSyncChain: (String, String) -> Unit,
    syncCode: String,
    onSetSyncCode: (String) -> Unit,
    secretKey: String,
    onSetSecretKey: (String) -> Unit,
    modifier: Modifier = Modifier,
    onLeaveSyncChain: () -> Unit,
) {
    BackHandler(onBack = onNavigateUp)
    val scrollState = rememberScrollState()

    SyncScaffold(
        leaveSyncVisible = false,
        title = stringResource(id = R.string.join_sync_chain),
        onNavigateUp = onNavigateUp,
        onLeaveSyncChain = onLeaveSyncChain,
        modifier = modifier,
    ) { innerModifier ->
        SyncJoinContent(
            onJoinSyncChain = onJoinSyncChain,
            syncCode = syncCode,
            onSetSyncCode = onSetSyncCode,
            secretKey = secretKey,
            modifier = innerModifier.verticalScroll(scrollState),
            onSetSecretKey = onSetSecretKey,
        )
    }
}

@Composable
fun SyncJoinContent(
    onJoinSyncChain: (String, String) -> Unit,
    syncCode: String,
    onSetSyncCode: (String) -> Unit,
    secretKey: String,
    modifier: Modifier = Modifier,
    onSetSecretKey: (String) -> Unit,
) {
    val dimens = LocalDimens.current

    Column(
        verticalArrangement = Arrangement.spacedBy(8.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        modifier =
            modifier
                .fillMaxSize()
                .padding(horizontal = dimens.margin, vertical = 8.dp),
    ) {
        TextField(
            value = syncCode,
            label = {
                Text(text = stringResource(R.string.sync_code))
            },
            onValueChange = onSetSyncCode,
            isError = syncCode.syncCodeQueryParam.length != 64,
            singleLine = true,
            modifier =
                Modifier
                    .fillMaxWidth()
                    .heightIn(min = 64.dp),
        )
        TextField(
            value = secretKey,
            label = {
                Text(text = stringResource(R.string.secret_key))
            },
            onValueChange = onSetSecretKey,
            isError = !AesCbcWithIntegrity.isKeyDecodable(secretKey),
            modifier =
                Modifier
                    .fillMaxWidth()
                    .heightIn(min = 64.dp),
        )
        Button(
            enabled = syncCode.syncCodeQueryParam.length == 64,
            onClick = {
                onJoinSyncChain(syncCode, secretKey)
            },
        ) {
            Text(
                text = stringResource(R.string.join_sync_chain),
                style = MaterialTheme.typography.labelLarge,
            )
        }
    }
}

@Composable
fun SyncDeviceListScreen(
    onNavigateUp: () -> Unit,
    currentDeviceId: Long,
    devices: ImmutableHolder<List<SyncDevice>>,
    onAddNewDevice: () -> Unit,
    onDeleteDevice: (Long) -> Unit,
    modifier: Modifier = Modifier,
    onLeaveSyncChain: () -> Unit,
) {
    BackHandler(onBack = onNavigateUp)
    val scrollState = rememberScrollState()

    SyncScaffold(
        leaveSyncVisible = true,
        title = stringResource(id = R.string.device_sync),
        onNavigateUp = onNavigateUp,
        onLeaveSyncChain = onLeaveSyncChain,
        modifier = modifier,
    ) { innerModifier ->
        SyncDeviceListContent(
            currentDeviceId = currentDeviceId,
            devices = devices,
            onAddNewDevice = onAddNewDevice,
            onDeleteDevice = onDeleteDevice,
            showAddDeviceButton = true,
            modifier = innerModifier.verticalScroll(scrollState),
        )
    }
}

@Composable
fun SyncDeviceListContent(
    currentDeviceId: Long,
    devices: ImmutableHolder<List<SyncDevice>>,
    onAddNewDevice: () -> Unit,
    onDeleteDevice: (Long) -> Unit,
    showAddDeviceButton: Boolean,
    modifier: Modifier = Modifier,
) {
    var itemToDelete by remember {
        mutableStateOf(SyncDevice(deviceId = ID_UNSET, deviceName = ""))
    }

    val dimens = LocalDimens.current
    val activityLauncher: ActivityLauncher by LocalDI.current.instance()

    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        modifier =
            modifier
                .fillMaxSize()
                .padding(horizontal = dimens.margin, vertical = 8.dp),
    ) {
        Text(
            text = stringResource(R.string.devices_on_sync_chain),
            style = MaterialTheme.typography.titleMedium,
            modifier =
                Modifier
                    .padding(top = 8.dp, bottom = 8.dp)
                    .fillMaxWidth(),
        )
        for (device in devices.item) {
            DeviceEntry(
                currentDeviceId = currentDeviceId,
                device = device,
                onDelete = {
                    itemToDelete = device
                },
            )
        }
        Text(
            text = stringResource(R.string.device_sync_financed_by_community),
            style = MaterialTheme.typography.bodyLarge,
            modifier =
                Modifier
                    .fillMaxWidth()
                    .padding(top = 8.dp),
        )
        // Google Play does not allow direct donation links
        if (!BuildConfig.BUILD_TYPE.contains("play", ignoreCase = true)) {
            Text(
                text = KOFI_URL,
                style = MaterialTheme.typography.bodyLarge.merge(LinkTextStyle()),
                modifier =
                    Modifier
                        .fillMaxWidth()
                        .clickable {
                            activityLauncher.startActivity(
                                openAdjacentIfSuitable = true,
                                intent = openKoFiIntent(),
                            )
                        },
            )
        }
        if (showAddDeviceButton) {
            Button(
                onClick = onAddNewDevice,
                modifier = Modifier.padding(top = 8.dp),
            ) {
                Text(
                    text = stringResource(R.string.add_new_device),
                    style = MaterialTheme.typography.labelLarge,
                    textAlign = TextAlign.Center,
                )
            }
        }
        Spacer(modifier = Modifier.size(8.dp))
    }

    if (itemToDelete.deviceId != ID_UNSET) {
        DeleteDeviceDialog(
            deviceName = itemToDelete.deviceName,
            onDismiss = { itemToDelete = SyncDevice(deviceId = ID_UNSET, deviceName = "") },
        ) {
            onDeleteDevice(itemToDelete.deviceId)
            itemToDelete = SyncDevice(deviceId = ID_UNSET, deviceName = "")
        }
    }
}

@Composable
fun DeviceEntry(
    currentDeviceId: Long,
    device: SyncDevice,
    modifier: Modifier = Modifier,
    onDelete: () -> Unit,
) {
    Row(
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.SpaceBetween,
        modifier =
            modifier
                .fillMaxWidth()
                .heightIn(min = minimumTouchSize),
    ) {
        val text =
            if (device.deviceId == currentDeviceId) {
                stringResource(id = R.string.this_device, device.deviceName)
            } else {
                device.deviceName
            }
        Text(
            text = text,
            style = MaterialTheme.typography.titleMedium,
            modifier = Modifier.weight(1f, fill = true),
        )
        IconButton(onClick = onDelete) {
            Icon(
                Icons.Filled.Delete,
                contentDescription = stringResource(R.string.disconnect_device_from_sync),
            )
        }
    }
}

@Preview
@Composable
private fun PreviewDeviceEntry() {
    FeederTheme {
        Surface {
            DeviceEntry(
                currentDeviceId = 77L,
                device = SyncDevice(deviceId = 1L, deviceName = "ONEPLUS A6003"),
                onDelete = {},
            )
        }
    }
}

@Composable
fun DeleteDeviceDialog(
    deviceName: String,
    onDismiss: () -> Unit,
    onOk: () -> Unit,
) {
    AlertDialog(
        onDismissRequest = onDismiss,
        confirmButton = {
            Button(onClick = onOk) {
                Text(text = stringResource(id = android.R.string.ok))
            }
        },
        dismissButton = {
            Button(onClick = onDismiss) {
                Text(text = stringResource(id = android.R.string.cancel))
            }
        },
        title = {
            Text(
                text = stringResource(R.string.remove_device),
                style = MaterialTheme.typography.titleLarge,
                textAlign = TextAlign.Center,
                modifier =
                    Modifier
                        .padding(vertical = 8.dp),
            )
        },
        text = {
            Text(
                text = stringResource(R.string.remove_device_question, deviceName),
                style = MaterialTheme.typography.bodyLarge,
            )
        },
    )
}

@Composable
fun SyncAddNewDeviceScreen(
    onNavigateUp: () -> Unit,
    syncUrl: ImmutableHolder<URL>,
    modifier: Modifier = Modifier,
    onLeaveSyncChain: () -> Unit,
) {
    BackHandler(onBack = onNavigateUp)
    val scrollState = rememberScrollState()

    SyncScaffold(
        leaveSyncVisible = false,
        onNavigateUp = onNavigateUp,
        onLeaveSyncChain = onLeaveSyncChain,
        title = stringResource(id = R.string.add_new_device),
        modifier = modifier,
    ) { innerModifier ->
        SyncAddNewDeviceContent(
            syncUrl = syncUrl,
            modifier = innerModifier.verticalScroll(scrollState),
        )
    }
}

@Composable
fun SyncAddNewDeviceContent(
    syncUrl: ImmutableHolder<URL>,
    modifier: Modifier = Modifier,
) {
    val activityLauncher: ActivityLauncher by LocalDI.current.instance()

    val qrCode by remember(syncUrl) {
        derivedStateOf {
            QRCode.from(
                Url().also {
                    it.url = "${syncUrl.item}"
                },
            )
                .withSize(1000, 1000)
                .bitmap()
                .asImageBitmap()
        }
    }
    val dimens = LocalDimens.current

    Column(
        verticalArrangement = Arrangement.spacedBy(8.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        modifier =
            modifier
                .fillMaxSize()
                .padding(horizontal = dimens.margin, vertical = 8.dp),
    ) {
        Text(
            text = stringResource(R.string.press_scan_sync),
            style = MaterialTheme.typography.bodyLarge,
            modifier =
                Modifier
                    .fillMaxWidth(),
        )
        Text(
            text = stringResource(R.string.or_open_device_sync_link),
            style = MaterialTheme.typography.bodyLarge,
            modifier =
                Modifier
                    .fillMaxWidth(),
        )
        Text(
            text = stringResource(R.string.treat_like_password),
            style = MaterialTheme.typography.bodyLarge.copy(color = Color.Red),
            modifier =
                Modifier
                    .fillMaxWidth(),
        )
        Image(
            bitmap = qrCode,
            contentDescription = stringResource(R.string.qr_code),
            contentScale = ContentScale.Fit,
            modifier = Modifier.size(250.dp),
        )
        val intentTitle = stringResource(R.string.feeder_device_sync_code)
        Text(
            text = "$syncUrl",
            style = LinkTextStyle(),
            modifier =
                Modifier
                    .fillMaxWidth()
                    .clickable {
                        val intent =
                            Intent.createChooser(
                                Intent(Intent.ACTION_SEND).apply {
                                    putExtra(Intent.EXTRA_TEXT, "$syncUrl")
                                    putExtra(Intent.EXTRA_TITLE, intentTitle)
                                    type = "text/plain"
                                },
                                null,
                            )
                        activityLauncher.startActivity(
                            openAdjacentIfSuitable = false,
                            intent = intent,
                        )
                    },
        )
    }
}

@Preview("Device List Tablet", device = Devices.PIXEL_C)
@Composable
private fun PreviewDualSyncScreenDeviceList() {
    FeederTheme {
        DualSyncScreen(
            onNavigateUp = { },
            leftScreenToShow = LeftScreenToShow.DEVICELIST,
            rightScreenToShow = RightScreenToShow.ADD_DEVICE,
            onScanSyncCode = { },
            onStartNewSyncChain = { },
            onAddNewDevice = { },
            onDeleteDevice = {},
            onLeaveSyncChain = {},
            currentDeviceId = 5L,
            devices =
                ImmutableHolder(
                    listOf(
                        SyncDevice(deviceId = 1L, deviceName = "ONEPLUS A6003"),
                        SyncDevice(deviceId = 2L, deviceName = "SM-T970"),
                        SyncDevice(deviceId = 3L, deviceName = "Nexus 6"),
                    ),
                ),
            addDeviceUrl = ImmutableHolder(URL("$DEEP_LINK_BASE_URI/sync/join?sync_code=123foo")),
            onJoinSyncChain = { _, _ -> },
            syncCode = "",
            onSetSyncCode = {},
            secretKey = "",
            onSetSecretKey = {},
        )
    }
}

@Preview("Setup Tablet", device = Devices.PIXEL_C)
@Preview("Setup Foldable", device = Devices.FOLDABLE, widthDp = 720, heightDp = 360)
@Composable
private fun PreviewDualSyncScreenSetup() {
    FeederTheme {
        DualSyncScreen(
            onNavigateUp = { },
            leftScreenToShow = LeftScreenToShow.SETUP,
            rightScreenToShow = RightScreenToShow.JOIN,
            onScanSyncCode = { },
            onStartNewSyncChain = { },
            onAddNewDevice = { },
            onDeleteDevice = {},
            onLeaveSyncChain = {},
            currentDeviceId = 5L,
            devices =
                ImmutableHolder(
                    listOf(
                        SyncDevice(deviceId = 1L, deviceName = "ONEPLUS A6003"),
                        SyncDevice(deviceId = 2L, deviceName = "SM-T970"),
                        SyncDevice(deviceId = 3L, deviceName = "Nexus 6"),
                    ),
                ),
            addDeviceUrl = ImmutableHolder(URL("$DEEP_LINK_BASE_URI/sync/join?sync_code=123foo&key=123ABF")),
            onJoinSyncChain = { _, _ -> },
            syncCode = "",
            onSetSyncCode = {},
            secretKey = "",
            onSetSecretKey = {},
        )
    }
}

@Preview("Scan or Enter Phone")
@Preview("Scan or Enter Small Tablet", device = Devices.NEXUS_7_2013)
@Composable
private fun PreviewJoin() {
    FeederTheme {
        SyncJoinScreen(
            onNavigateUp = {},
            onJoinSyncChain = { _, _ -> },
            syncCode = "",
            onSetSyncCode = {},
            onLeaveSyncChain = {},
            secretKey = "",
            onSetSecretKey = {},
        )
    }
}

@Preview("Empty Phone")
@Preview("Empty Small Tablet", device = Devices.NEXUS_7_2013)
@Composable
private fun PreviewEmpty() {
    FeederTheme {
        SyncSetupScreen(
            onNavigateUp = {},
            onScanSyncCode = {},
            onStartNewSyncChain = {},
            onLeaveSyncChain = {},
        )
    }
}

@Preview("Device List Phone")
@Preview("Device List Small Tablet", device = Devices.NEXUS_7_2013)
@Composable
private fun PreviewDeviceList() {
    FeederTheme {
        SyncDeviceListScreen(
            onNavigateUp = {},
            currentDeviceId = 5L,
            devices =
                ImmutableHolder(
                    listOf(
                        SyncDevice(deviceId = 1L, deviceName = "ONEPLUS A6003"),
                        SyncDevice(deviceId = 2L, deviceName = "SM-T970"),
                        SyncDevice(deviceId = 3L, deviceName = "Nexus 6"),
                    ),
                ),
            onAddNewDevice = {},
            onDeleteDevice = {},
            onLeaveSyncChain = {},
        )
    }
}

@Preview("Add New Device Phone")
@Preview("Add New Device Small Tablet", device = Devices.NEXUS_7_2013)
@Composable
private fun PreviewAddNewDeviceContent() {
    FeederTheme {
        SyncAddNewDeviceScreen(
            onNavigateUp = {},
            onLeaveSyncChain = {},
            syncUrl = ImmutableHolder(URL("https://feeder-sync.nononsenseapps.com/join?sync_code=1234abc572335asdbc&key=123ABF")),
        )
    }
}

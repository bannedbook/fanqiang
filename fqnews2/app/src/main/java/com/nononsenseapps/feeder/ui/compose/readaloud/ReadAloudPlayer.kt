package com.nononsenseapps.feeder.ui.compose.readaloud

import android.os.Build
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.Crossfade
import androidx.compose.animation.core.MutableTransitionState
import androidx.compose.animation.core.tween
import androidx.compose.animation.slideInVertically
import androidx.compose.animation.slideOutVertically
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.height
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.DoneAll
import androidx.compose.material.icons.filled.Pause
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.SkipNext
import androidx.compose.material.icons.filled.Stop
import androidx.compose.material.icons.filled.Translate
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.FloatingActionButton
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.role
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.model.AppSetting
import com.nononsenseapps.feeder.model.ForcedAuto
import com.nononsenseapps.feeder.model.ForcedLocale
import com.nononsenseapps.feeder.model.LocaleOverride
import com.nononsenseapps.feeder.ui.compose.components.PaddedBottomAppBar
import com.nononsenseapps.feeder.ui.compose.components.safeSemantics
import com.nononsenseapps.feeder.ui.compose.feed.PlainTooltipBox
import com.nononsenseapps.feeder.ui.compose.theme.FeederTheme
import com.nononsenseapps.feeder.ui.compose.utils.ImmutableHolder
import com.nononsenseapps.feeder.ui.compose.utils.onKeyEventLikeEscape
import java.util.Locale

@Composable
fun HideableTTSPlayer(
    visibleState: MutableTransitionState<Boolean>,
    currentlyPlaying: Boolean,
    onPlay: () -> Unit,
    onPause: () -> Unit,
    onStop: () -> Unit,
    onSkipNext: () -> Unit,
    languages: ImmutableHolder<List<Locale>>,
    modifier: Modifier = Modifier,
    floatingActionButton: @Composable (() -> Unit)? = null,
    onSelectLanguage: (LocaleOverride) -> Unit,
) {
    AnimatedVisibility(
        modifier = modifier,
        visibleState = visibleState,
        enter = slideInVertically(initialOffsetY = { it }, animationSpec = tween(256)),
        exit = slideOutVertically(targetOffsetY = { it }, animationSpec = tween(256)),
    ) {
        TTSPlayer(
            currentlyPlaying = currentlyPlaying,
            onPlay = onPlay,
            onPause = onPause,
            onStop = onStop,
            onSkipNext = onSkipNext,
            languages = languages,
            floatingActionButton = floatingActionButton,
            onSelectLanguage = onSelectLanguage,
        )
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun TTSPlayer(
    currentlyPlaying: Boolean,
    onPlay: () -> Unit,
    onPause: () -> Unit,
    onStop: () -> Unit,
    onSkipNext: () -> Unit,
    languages: ImmutableHolder<List<Locale>>,
    modifier: Modifier = Modifier,
    floatingActionButton: @Composable (() -> Unit)? = null,
    onSelectLanguage: (LocaleOverride) -> Unit,
) {
    var showMenu by remember {
        mutableStateOf(false)
    }
    val closeMenuText = stringResource(id = R.string.close_menu)
    PaddedBottomAppBar(
        modifier = modifier,
        floatingActionButton = floatingActionButton,
        actions = {
            PlainTooltipBox(tooltip = { Text(stringResource(R.string.stop_reading)) }) {
                IconButton(
                    onClick = onStop,
                ) {
                    Icon(
                        Icons.Default.Stop,
                        contentDescription = stringResource(R.string.stop_reading),
                    )
                }
            }
            Crossfade(targetState = currentlyPlaying) { playing ->
                if (playing) {
                    PlainTooltipBox(tooltip = { Text(stringResource(R.string.pause_reading)) }) {
                        IconButton(
                            onClick = onPause,
                        ) {
                            Icon(
                                Icons.Default.Pause,
                                contentDescription = stringResource(R.string.pause_reading),
                            )
                        }
                    }
                } else {
                    PlainTooltipBox(tooltip = { Text(stringResource(R.string.resume_reading)) }) {
                        IconButton(
                            onClick = onPlay,
                        ) {
                            Icon(
                                // TextToSpeech
                                Icons.Default.PlayArrow,
                                contentDescription = stringResource(R.string.resume_reading),
                            )
                        }
                    }
                }
            }
            PlainTooltipBox(tooltip = { Text(stringResource(R.string.skip_to_next)) }) {
                IconButton(
                    onClick = onSkipNext,
                ) {
                    Icon(
                        Icons.Default.SkipNext,
                        contentDescription = stringResource(R.string.skip_to_next),
                    )
                }
            }
            Box {
                PlainTooltipBox(tooltip = { Text(stringResource(R.string.set_language)) }) {
                    IconButton(
                        onClick = {
                            showMenu = true
                        },
                    ) {
                        Icon(
                            Icons.Default.Translate,
                            contentDescription = stringResource(R.string.set_language),
                        )
                    }
                }
                DropdownMenu(
                    expanded = showMenu,
                    onDismissRequest = { showMenu = false },
                    modifier =
                        Modifier.onKeyEventLikeEscape {
                            showMenu = false
                        },
                ) {
                    // Hidden button for TalkBack
                    DropdownMenuItem(
                        onClick = {
                            showMenu = false
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
                    DropdownMenuItem(
                        onClick = {
                            onSelectLanguage(AppSetting)
                            showMenu = false
                        },
                        text = {
                            Text(stringResource(id = R.string.use_app_default))
                        },
                    )
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                        DropdownMenuItem(
                            onClick = {
                                onSelectLanguage(ForcedAuto)
                                showMenu = false
                            },
                            text = {
                                Text(stringResource(id = R.string.use_detect_language))
                            },
                        )
                    }
                    HorizontalDivider()
                    for (lang in languages.item) {
                        DropdownMenuItem(
                            onClick = {
                                onSelectLanguage(ForcedLocale(lang))
                                showMenu = false
                            },
                            text = {
                                Text(text = lang.getDisplayName(lang))
                            },
                        )
                    }
                }
            }
        },
    )
}

@Preview
@Composable
private fun PlayerPreview() {
    FeederTheme {
        TTSPlayer(
            currentlyPlaying = true,
            onPlay = {},
            onPause = {},
            onStop = {},
            onSkipNext = {},
            languages = ImmutableHolder(emptyList()),
        ) {}
    }
}

@Preview
@Composable
private fun PlayerPreviewWithFab() {
    FeederTheme {
        TTSPlayer(
            currentlyPlaying = true,
            onPlay = {},
            onPause = {},
            onStop = {},
            onSkipNext = {},
            languages = ImmutableHolder(emptyList()),
            floatingActionButton = {
                FloatingActionButton(
                    onClick = {},
                ) {
                    Icon(
                        Icons.Default.DoneAll,
                        contentDescription = stringResource(R.string.mark_all_as_read),
                    )
                }
            },
        ) {}
    }
}

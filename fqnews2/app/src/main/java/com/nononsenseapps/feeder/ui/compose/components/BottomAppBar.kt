package com.nononsenseapps.feeder.ui.compose.components

import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.focusGroup
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.RowScope
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.navigationBars
import androidx.compose.foundation.layout.only
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Pause
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Stop
import androidx.compose.material3.BottomAppBarDefaults
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.Surface
import androidx.compose.material3.contentColorFor
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.RectangleShape
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import com.nononsenseapps.feeder.ui.compose.bottomBarHeight
import com.nononsenseapps.feeder.ui.compose.theme.FeederTheme

/**
 * Material Design bottom app bar. Forked from library because the library version does not take
 * navigation bar padding into account properly - nor does it allow the FAB to be animated
 * according to the guidelines.
 *
 * A bottom app bar displays navigation and key actions at the bottom of mobile screens.
 */
@OptIn(ExperimentalFoundationApi::class)
@Composable
fun PaddedBottomAppBar(
    actions: @Composable RowScope.() -> Unit,
    modifier: Modifier = Modifier,
    floatingActionButton: @Composable (() -> Unit)? = null,
    containerColor: Color = BottomAppBarDefaults.containerColor,
    contentColor: Color = contentColorFor(containerColor),
    tonalElevation: Dp = BottomAppBarDefaults.ContainerElevation,
    contentPadding: PaddingValues = BottomAppBarDefaults.ContentPadding,
) = BottomAppBar(
    modifier = modifier.focusGroup(),
    containerColor = containerColor,
    contentColor = contentColor,
    tonalElevation = tonalElevation,
    contentPadding = contentPadding,
) {
    actions()
    if (floatingActionButton != null) {
        Spacer(Modifier.weight(1f, true))
        Box(
            Modifier
                .fillMaxHeight()
                .padding(
                    top = FABVerticalPadding,
                    end = FABHorizontalPadding,
                ),
            contentAlignment = Alignment.TopStart,
        ) {
            floatingActionButton()
        }
    }
}

@Preview
@Composable
private fun PreviewPaddedBottomBar() {
    FeederTheme {
        PaddedBottomAppBar(
            actions = {
                IconButton(onClick = {}) {
                    Icon(
                        Icons.Default.PlayArrow,
                        contentDescription = null,
                    )
                }
                IconButton(onClick = {}) {
                    Icon(
                        Icons.Default.Pause,
                        contentDescription = null,
                    )
                }
                IconButton(onClick = {}) {
                    Icon(
                        Icons.Default.Stop,
                        contentDescription = null,
                    )
                }
            },
        )
    }
}

// Padding minus IconButton's min touch target expansion
private val BottomAppBarHorizontalPadding = 16.dp - 12.dp

// Padding minus IconButton's min touch target expansion
private val BottomAppBarVerticalPadding = 16.dp - 12.dp

// Padding minus content padding
private val FABHorizontalPadding = 16.dp - BottomAppBarHorizontalPadding
private val FABVerticalPadding = 12.dp - BottomAppBarVerticalPadding

@OptIn(ExperimentalFoundationApi::class)
@Composable
private fun BottomAppBar(
    modifier: Modifier = Modifier,
    containerColor: Color = BottomAppBarDefaults.containerColor,
    contentColor: Color = contentColorFor(containerColor),
    tonalElevation: Dp = BottomAppBarDefaults.ContainerElevation,
    contentPadding: PaddingValues = BottomAppBarDefaults.ContentPadding,
    content: @Composable RowScope.() -> Unit,
) {
    Surface(
        color = containerColor,
        contentColor = contentColor,
        tonalElevation = tonalElevation,
        shape = RectangleShape,
        modifier = modifier,
    ) {
        Box(
            modifier =
                Modifier.windowInsetsPadding(
                    WindowInsets.navigationBars.only(WindowInsetsSides.Bottom),
                ),
        ) {
            Row(
                modifier =
                    Modifier
                        .fillMaxWidth()
                        .height(bottomBarHeight)
                        .padding(contentPadding)
                        .focusGroup(),
                horizontalArrangement = Arrangement.Start,
                verticalAlignment = Alignment.CenterVertically,
                content = content,
            )
        }
    }
}

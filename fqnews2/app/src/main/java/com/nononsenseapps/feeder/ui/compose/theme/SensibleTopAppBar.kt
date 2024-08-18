package com.nononsenseapps.feeder.ui.compose.theme

import androidx.compose.foundation.layout.RowScope
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults.topAppBarColors
import androidx.compose.material3.TopAppBarScrollBehavior
import androidx.compose.material3.surfaceColorAtElevation
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import com.nononsenseapps.feeder.ui.compose.text.WithBidiDeterminedLayoutDirection

/**
 * On a small but tall screen this will be a LargeTopAppBar to make the screen
 * more one-hand friendly.
 *
 * One a short screen - or bigger tablet size - then it's a small top app bar which can scoll
 * out of the way to make best use of the available screen space.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SensibleTopAppBar(
    title: String,
    modifier: Modifier = Modifier,
    navigationIcon: @Composable () -> Unit = {},
    actions: @Composable RowScope.() -> Unit = {},
    scrollBehavior: TopAppBarScrollBehavior? = null,
) {
    TopAppBar(
        scrollBehavior = scrollBehavior,
        title = {
            WithBidiDeterminedLayoutDirection(paragraph = title) {
                Text(
                    title,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                )
            }
        },
        navigationIcon = navigationIcon,
        actions = actions,
        modifier = modifier,
        colors =
            topAppBarColors(
                containerColor = MaterialTheme.colorScheme.background,
                scrolledContainerColor = MaterialTheme.colorScheme.surfaceColorAtElevation(3.dp),
            ),
    )
}

package com.nononsenseapps.feeder.ui.compose.theme

import androidx.compose.animation.animateColorAsState
import androidx.compose.animation.core.FastOutLinearInEasing
import androidx.compose.animation.core.Spring
import androidx.compose.animation.core.spring
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.TopAppBarScrollBehavior
import androidx.compose.material3.surfaceColorAtElevation
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.ui.graphics.lerp
import androidx.compose.ui.unit.dp
import com.google.accompanist.systemuicontroller.rememberSystemUiController

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SetStatusBarColorToMatchScrollableTopAppBar(scrollBehavior: TopAppBarScrollBehavior) {
    // This is what is changed by Black theme
    val surfaceColor = MaterialTheme.colorScheme.background
    val surfaceScrolledColor = MaterialTheme.colorScheme.surfaceColorAtElevation(3.dp)

    val colorTransitionFraction = scrollBehavior.state.overlappedFraction
    val fraction = if (colorTransitionFraction > 0.01f) 1f else 0f

    val appBarContainerColor by animateColorAsState(
        targetValue =
            lerp(
                surfaceColor,
                surfaceScrolledColor,
                FastOutLinearInEasing.transform(fraction),
            ),
        animationSpec = spring(stiffness = Spring.StiffnessMediumLow),
    )

    val systemUiController = rememberSystemUiController()
    DisposableEffect(systemUiController, appBarContainerColor) {
        systemUiController.setStatusBarColor(
            appBarContainerColor,
        )
        onDispose {}
    }
}

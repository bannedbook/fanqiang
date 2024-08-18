package com.nononsenseapps.feeder.ui.compose.utils

import android.app.Activity
import android.content.res.Configuration
import androidx.compose.material3.windowsizeclass.ExperimentalMaterial3WindowSizeClassApi
import androidx.compose.material3.windowsizeclass.WindowHeightSizeClass
import androidx.compose.material3.windowsizeclass.WindowSizeClass
import androidx.compose.material3.windowsizeclass.WindowWidthSizeClass
import androidx.compose.material3.windowsizeclass.calculateWindowSizeClass
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.ProvidableCompositionLocal
import androidx.compose.runtime.compositionLocalOf
import androidx.compose.ui.graphics.toComposeRect
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.DpSize
import androidx.window.layout.WindowMetricsCalculator

val LocalWindowSizeMetrics: ProvidableCompositionLocal<WindowSizeClass> =
    compositionLocalOf { error("Missing WindowSize container!") }

val LocalWindowSize: ProvidableCompositionLocal<DpSize> =
    compositionLocalOf { error("Missing WindowMetrics container!") }

@OptIn(ExperimentalMaterial3WindowSizeClassApi::class)
@Composable
fun Activity.withWindowSize(content: @Composable () -> Unit) {
    val windowSizeclass = calculateWindowSizeClass(activity = this)

    CompositionLocalProvider(LocalWindowSizeMetrics provides windowSizeclass) {
        content()
    }
}

@Composable
fun Activity.withWindowMetrics(content: @Composable () -> Unit) {
    LocalConfiguration.current
    val density = LocalDensity.current
    val metrics = WindowMetricsCalculator.getOrCreate().computeCurrentWindowMetrics(this)
    val size = with(density) { metrics.bounds.toComposeRect().size.toDpSize() }
    CompositionLocalProvider(LocalWindowSize provides size) {
        content()
    }
}

@Composable
fun WithPreviewWindowSize(
    windowSizeclass: WindowSizeClass,
    content: @Composable () -> Unit,
) {
    CompositionLocalProvider(LocalWindowSizeMetrics provides windowSizeclass) {
        content()
    }
}

@Composable
fun isCompactLandscape(): Boolean {
    return LocalConfiguration.current.orientation == Configuration.ORIENTATION_LANDSCAPE &&
        LocalWindowSizeMetrics.current.heightSizeClass == WindowHeightSizeClass.Compact
}

@Composable
fun isCompactDevice(): Boolean {
    val windowSize = LocalWindowSizeMetrics.current
    return windowSize.heightSizeClass == WindowHeightSizeClass.Compact ||
        windowSize.widthSizeClass == WindowWidthSizeClass.Compact
}

enum class ScreenType {
    DUAL,
    SINGLE,
}

fun getScreenType(windowSize: WindowSizeClass) =
    when (windowSize.widthSizeClass) {
        WindowWidthSizeClass.Compact -> ScreenType.SINGLE
        else -> ScreenType.DUAL
    }

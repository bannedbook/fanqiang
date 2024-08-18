package com.nononsenseapps.feeder.ui.compose.theme

import androidx.compose.material3.windowsizeclass.WindowHeightSizeClass
import androidx.compose.material3.windowsizeclass.WindowWidthSizeClass
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.Immutable
import androidx.compose.runtime.remember
import androidx.compose.runtime.staticCompositionLocalOf
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.coerceAtMost
import androidx.compose.ui.unit.dp
import com.nononsenseapps.feeder.ui.compose.utils.LocalWindowSize
import com.nononsenseapps.feeder.ui.compose.utils.LocalWindowSizeMetrics

@Immutable
class Dimensions(
    /**
     * Margin of the navigation button in app bar
     */
    val navIconMargin: Dp,
    /**
     * A gutter is the space between columns that helps separate content.
     */
    val gutter: Dp,
    /**
     * Margins are the space between content and the left and right edges of the screen.
     */
    val margin: Dp,
    /**
     * The max width of the content in case of very wide screens.
     */
    val maxContentWidth: Dp,
    /**
     * The max width of the reader in case of very wide screens.
     */
    val maxReaderWidth: Dp,
    /**
     * Non-null if image has a constrained aspect ratio in reader (TVs)
     */
    val imageAspectRatioInReader: Float?,
    /**
     * The responsive column grid is made up of columns, gutters, and margins, providing a
     * convenient structure for the layout of elements within the body region.
     * Components, imagery, and text align with the column grid to ensure a logical and
     * consistent layout across screen sizes and orientations.
     *
     * As the size of the body region grows or shrinks, the number of grid columns
     * changes in response.
     */
    val layoutColumns: Int,
    /**
     * Number of columns in feed screen
     */
    val feedScreenColumns: Int,
)

val Dimensions.hasImageAspectRatioInReader: Boolean
    get() = imageAspectRatioInReader != null

val phoneDimensions =
    Dimensions(
        maxContentWidth = 600.dp,
        maxReaderWidth = 600.dp,
        imageAspectRatioInReader = null,
        navIconMargin = 16.dp,
        margin = 16.dp,
        gutter = 16.dp,
        layoutColumns = 4,
        feedScreenColumns = 1,
    )

fun tabletDimensions(windowWidthDp: Dp): Dimensions {
    // Items look good at around 300dp width. Account for 32dp margin at the sides, and the gutters
    // 3 columns: 3*300 + 4*32 = 1028
    val columns =
        when {
            windowWidthDp > 1360.dp -> 4
            windowWidthDp > 1028.dp -> 3
            else -> 2
        }
    return Dimensions(
        maxContentWidth = 840.dp.coerceAtMost(windowWidthDp),
        maxReaderWidth = 640.dp.coerceAtMost(windowWidthDp),
        imageAspectRatioInReader = null,
        navIconMargin = 32.dp,
        margin = 32.dp,
        gutter = 32.dp,
        layoutColumns = columns * 4,
        feedScreenColumns = columns,
    )
}

val tvDimensions =
    Dimensions(
        maxContentWidth = 840.dp,
        maxReaderWidth = 640.dp,
        imageAspectRatioInReader = 16.0f / 9.0f,
        navIconMargin = 32.dp,
        margin = 32.dp,
        gutter = 32.dp,
        layoutColumns = 12,
        feedScreenColumns = 3,
    )

val LocalDimens =
    staticCompositionLocalOf {
        phoneDimensions
    }

@Composable
fun ProvideDimens(content: @Composable () -> Unit) {
    val windowSize = LocalWindowSize.current
    val windowSizeClass = LocalWindowSizeMetrics.current
    val config = LocalConfiguration.current

    val dimensionSet =
        remember {
            if (config.screenWidthDp == 960 && config.screenHeightDp == 540) {
                // TV dimensions are special case
                tvDimensions
            } else {
                when (windowSizeClass.widthSizeClass) {
                    WindowWidthSizeClass.Compact -> phoneDimensions
                    else -> {
                        when (windowSizeClass.heightSizeClass) {
                            WindowHeightSizeClass.Compact -> phoneDimensions
                            else ->
                                tabletDimensions(windowSize.width)
                        }
                    }
                }
            }
        }

    CompositionLocalProvider(LocalDimens provides dimensionSet, content = content)
}

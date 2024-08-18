/*
 * Copyright 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.nononsenseapps.feeder.ui.compose.material3

import androidx.compose.animation.core.AnimationSpec
import androidx.compose.animation.core.TweenSpec
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.gestures.Orientation
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.interaction.Interaction
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.only
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.sizeIn
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.material3.ColorScheme
import androidx.compose.material3.DismissibleNavigationDrawer
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.LocalContentColor
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.ModalNavigationDrawer
import androidx.compose.material3.NavigationDrawerItem
import androidx.compose.material3.PermanentNavigationDrawer
import androidx.compose.material3.Surface
import androidx.compose.material3.contentColorFor
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.Immutable
import androidx.compose.runtime.Stable
import androidx.compose.runtime.State
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.saveable.Saver
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.RectangleShape
import androidx.compose.ui.graphics.Shape
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.Layout
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.platform.LocalLayoutDirection
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.dismiss
import androidx.compose.ui.semantics.onClick
import androidx.compose.ui.semantics.paneTitle
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.LayoutDirection
import androidx.compose.ui.unit.dp
import com.nononsenseapps.feeder.ui.compose.material3.tokens.NavigationDrawerTokens
import com.nononsenseapps.feeder.ui.compose.material3.tokens.NavigationDrawerTokens.getContainerWidth
import com.nononsenseapps.feeder.ui.compose.material3.tokens.ScrimTokens
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.launch
import kotlin.math.roundToInt

/**
 * Possible values of [DrawerState].
 */
@ExperimentalMaterial3Api
enum class DrawerValue {
    /**
     * The state of the drawer when it is closed.
     */
    Closed,

    /**
     * The state of the drawer when it is open.
     */
    Open,
}

/**
 * State of the [ModalNavigationDrawer] and [DismissibleNavigationDrawer] composable.
 *
 * @param initialValue The initial value of the state.
 * @param confirmStateChange Optional callback invoked to confirm or veto a pending state change.
 */
@Suppress("NotCloseable")
@ExperimentalMaterial3Api
@Stable
class DrawerState(
    initialValue: DrawerValue,
    confirmStateChange: (DrawerValue) -> Boolean = { true },
) {
    internal val swipeableState =
        SwipeableState(
            initialValue = initialValue,
            animationSpec = AnimationSpec,
            confirmStateChange = confirmStateChange,
        )

    /**
     * Whether the drawer is open.
     */
    val isOpen: Boolean
        get() = currentValue == DrawerValue.Open

    /**
     * Whether the drawer is closed.
     */
    val isClosed: Boolean
        get() = currentValue == DrawerValue.Closed

    /**
     * The current value of the state.
     *
     * If no swipe or animation is in progress, this corresponds to the start the drawer
     * currently in. If a swipe or an animation is in progress, this corresponds the state drawer
     * was in before the swipe or animation started.
     */
    val currentValue: DrawerValue
        get() {
            return swipeableState.currentValue
        }

    /**
     * Whether the state is currently animating.
     */
    val isAnimationRunning: Boolean
        get() {
            return swipeableState.isAnimationRunning
        }

    /**
     * Open the drawer with animation and suspend until it if fully opened or animation has been
     * cancelled. This method will throw [CancellationException] if the animation is
     * interrupted
     *
     * @return the reason the open animation ended
     */
    suspend fun open() = animateTo(DrawerValue.Open, AnimationSpec)

    /**
     * Close the drawer with animation and suspend until it if fully closed or animation has been
     * cancelled. This method will throw [CancellationException] if the animation is
     * interrupted
     *
     * @return the reason the close animation ended
     */
    suspend fun close() = animateTo(DrawerValue.Closed, AnimationSpec)

    /**
     * Set the state of the drawer with specific animation
     *
     * @param targetValue The new value to animate to.
     * @param anim The animation that will be used to animate to the new value.
     */
    @ExperimentalMaterial3Api
    suspend fun animateTo(
        targetValue: DrawerValue,
        anim: AnimationSpec<Float>,
    ) {
        swipeableState.animateTo(targetValue, anim)
    }

    /**
     * Set the state without any animation and suspend until it's set
     *
     * @param targetValue The new target value
     */
    @ExperimentalMaterial3Api
    suspend fun snapTo(targetValue: DrawerValue) {
        swipeableState.snapTo(targetValue)
    }

    /**
     * The target value of the drawer state.
     *
     * If a swipe is in progress, this is the value that the Drawer would animate to if the
     * swipe finishes. If an animation is running, this is the target value of that animation.
     * Finally, if no swipe or animation is in progress, this is the same as the [currentValue].
     */
    @Suppress("OPT_IN_MARKER_ON_WRONG_TARGET")
    @ExperimentalMaterial3Api
    @get:ExperimentalMaterial3Api
    val targetValue: DrawerValue
        get() = swipeableState.targetValue

    /**
     * The current position (in pixels) of the drawer container.
     */
    @Suppress("OPT_IN_MARKER_ON_WRONG_TARGET")
    @ExperimentalMaterial3Api
    @get:ExperimentalMaterial3Api
    val offset: State<Float>
        get() = swipeableState.offset

    companion object {
        /**
         * The default [Saver] implementation for [DrawerState].
         */
        fun saver(confirmStateChange: (DrawerValue) -> Boolean) =
            Saver<DrawerState, DrawerValue>(
                save = { it.currentValue },
                restore = { DrawerState(it, confirmStateChange) },
            )
    }
}

/**
 * Create and [remember] a [DrawerState].
 *
 * @param initialValue The initial value of the state.
 * @param confirmStateChange Optional callback invoked to confirm or veto a pending state change.
 */
@Composable
@ExperimentalMaterial3Api
fun rememberDrawerState(
    initialValue: DrawerValue,
    confirmStateChange: (DrawerValue) -> Boolean = { true },
): DrawerState {
    return rememberSaveable(saver = DrawerState.saver(confirmStateChange)) {
        DrawerState(initialValue, confirmStateChange)
    }
}

/**
 * <a href="https://m3.material.io/components/navigation-drawer/overview" class="external" target="_blank">Material Design navigation drawer</a>.
 *
 * Navigation drawers provide ergonomic access to destinations in an app.
 *
 * Modal navigation drawers block interaction with the rest of an app’s content with a scrim.
 * They are elevated above most of the app’s UI and don’t affect the screen’s layout grid.
 *
 * ![Navigation drawer image](https://developer.android.com/images/reference/androidx/compose/material3/navigation-drawer.png)
 *
 * @sample androidx.compose.material3.samples.ModalNavigationDrawerSample
 *
 * @param drawerContent content inside this drawer
 * @param modifier the [Modifier] to be applied to this drawer
 * @param drawerState state of the drawer
 * @param gesturesEnabled whether or not the drawer can be interacted by gestures
 * @param scrimColor color of the scrim that obscures content when the drawer is open
 * @param content content of the rest of the UI
 */
@Composable
@ExperimentalMaterial3Api
fun ModalNavigationDrawer(
    drawerContent: @Composable () -> Unit,
    modifier: Modifier = Modifier,
    drawerState: DrawerState = rememberDrawerState(DrawerValue.Closed),
    gesturesEnabled: Boolean = true,
    scrimColor: Color = DrawerDefaults.scrimColor,
    content: @Composable () -> Unit,
) {
    val scope = rememberCoroutineScope()
    val navigationMenu = getString(Strings.NavigationMenu)
    val minValue = -with(LocalDensity.current) { getContainerWidth().toPx() }
    val maxValue = 0f

    val anchors = mapOf(minValue to DrawerValue.Closed, maxValue to DrawerValue.Open)
    val isRtl = LocalLayoutDirection.current == LayoutDirection.Rtl
    Box(
        modifier
            .fillMaxSize()
            .swipeable(
                state = drawerState.swipeableState,
                anchors = anchors,
                thresholds = { _, _ -> FractionalThreshold(0.5f) },
                orientation = Orientation.Horizontal,
                enabled = gesturesEnabled,
                reverseDirection = isRtl,
                velocityThreshold = DrawerVelocityThreshold,
                resistance = null,
            ),
    ) {
        Box {
            content()
        }
        Scrim(
            open = drawerState.isOpen,
            onClose = {
                if (
                    gesturesEnabled &&
                    drawerState.swipeableState.confirmStateChange(DrawerValue.Closed)
                ) {
                    scope.launch { drawerState.close() }
                }
            },
            fraction = {
                calculateFraction(minValue, maxValue, drawerState.offset.value)
            },
            color = scrimColor,
        )
        Box(
            Modifier
                .offset { IntOffset(drawerState.offset.value.roundToInt(), 0) }
                .semantics {
                    paneTitle = navigationMenu
                    if (drawerState.isOpen) {
                        dismiss {
                            if (
                                drawerState.swipeableState
                                    .confirmStateChange(DrawerValue.Closed)
                            ) {
                                scope.launch { drawerState.close() }
                            }
                            true
                        }
                    }
                },
        ) {
            drawerContent()
        }
    }
}

/**
 * <a href="https://m3.material.io/components/navigation-drawer/overview" class="external" target="_blank">Material Design navigation drawer</a>.
 *
 * Navigation drawers provide ergonomic access to destinations in an app. They’re often next to
 * app content and affect the screen’s layout grid.
 *
 * ![Navigation drawer image](https://developer.android.com/images/reference/androidx/compose/material3/navigation-drawer.png)
 *
 * Dismissible standard drawers can be used for layouts that prioritize content (such as a
 * photo gallery) or for apps where users are unlikely to switch destinations often. They should
 * use a visible navigation menu icon to open and close the drawer.
 *
 * @sample androidx.compose.material3.samples.DismissibleNavigationDrawerSample
 *
 * @param drawerContent content inside this drawer
 * @param modifier the [Modifier] to be applied to this drawer
 * @param drawerState state of the drawer
 * @param gesturesEnabled whether or not the drawer can be interacted by gestures
 * @param content content of the rest of the UI
 */
@Composable
@ExperimentalMaterial3Api
fun DismissibleNavigationDrawer(
    drawerContent: @Composable () -> Unit,
    modifier: Modifier = Modifier,
    drawerState: DrawerState = rememberDrawerState(DrawerValue.Closed),
    gesturesEnabled: Boolean = true,
    content: @Composable () -> Unit,
) {
    val drawerWidth = getContainerWidth()
    val drawerWidthPx = with(LocalDensity.current) { drawerWidth.toPx() }
    val minValue = -drawerWidthPx
    val maxValue = 0f

    val scope = rememberCoroutineScope()
    val navigationMenu = getString(Strings.NavigationMenu)

    val anchors = mapOf(minValue to DrawerValue.Closed, maxValue to DrawerValue.Open)
    val isRtl = LocalLayoutDirection.current == LayoutDirection.Rtl
    Box(
        modifier.swipeable(
            state = drawerState.swipeableState,
            anchors = anchors,
            thresholds = { _, _ -> FractionalThreshold(0.5f) },
            orientation = Orientation.Horizontal,
            enabled = gesturesEnabled,
            reverseDirection = isRtl,
            velocityThreshold = DrawerVelocityThreshold,
            resistance = null,
        ),
    ) {
        Layout(content = {
            Box(
                Modifier.semantics {
                    paneTitle = navigationMenu
                    if (drawerState.isOpen) {
                        dismiss {
                            if (
                                drawerState.swipeableState
                                    .confirmStateChange(DrawerValue.Closed)
                            ) {
                                scope.launch { drawerState.close() }
                            }
                            true
                        }
                    }
                },
            ) {
                drawerContent()
            }
            Box {
                content()
            }
        }) { measurables, constraints ->
            val sheetPlaceable = measurables[0].measure(constraints)
            val contentPlaceable = measurables[1].measure(constraints)
            layout(contentPlaceable.width, contentPlaceable.height) {
                contentPlaceable.placeRelative(
                    sheetPlaceable.width + drawerState.offset.value.roundToInt(),
                    0,
                )
                sheetPlaceable.placeRelative(drawerState.offset.value.roundToInt(), 0)
            }
        }
    }
}

/**
 * <a href="https://m3.material.io/components/navigation-drawer/overview" class="external" target="_blank">Material Design navigation permanent drawer</a>.
 *
 * Navigation drawers provide ergonomic access to destinations in an app. They’re often next to app
 * content and affect the screen’s layout grid.
 *
 * ![Navigation drawer image](https://developer.android.com/images/reference/androidx/compose/material3/navigation-drawer.png)
 *
 * The permanent navigation drawer is always visible and usually used for frequently switching
 * destinations. On mobile screens, use [ModalNavigationDrawer] instead.
 *
 * @sample androidx.compose.material3.samples.PermanentNavigationDrawerSample
 *
 * @param drawerContent content inside this drawer
 * @param modifier the [Modifier] to be applied to this drawer
 * @param content content of the rest of the UI
 */
@ExperimentalMaterial3Api
@Composable
fun PermanentNavigationDrawer(
    drawerContent: @Composable () -> Unit,
    modifier: Modifier = Modifier,
    content: @Composable () -> Unit,
) {
    Row(modifier.fillMaxSize()) {
        drawerContent()
        Box {
            content()
        }
    }
}

/**
 * Content inside of a modal navigation drawer.
 *
 * @param modifier the [Modifier] to be applied to this drawer's content
 * @param drawerShape defines the shape of this drawer's container
 * @param drawerContainerColor the color used for the background of this drawer. Use
 * [Color.Transparent] to have no color.
 * @param drawerContentColor the preferred color for content inside this drawer. Defaults to either
 * the matching content color for [drawerContainerColor], or to the current [LocalContentColor] if
 * [drawerContainerColor] is not a color from the theme.
 * @param drawerTonalElevation when [drawerContainerColor] is [ColorScheme.surface], a translucent
 * primary color overlay is applied on top of the container. A higher tonal elevation value will
 * result in a darker color in light theme and lighter color in dark theme. See also: [Surface].
 * @param windowInsets a window insets for the sheet.
 * @param content content inside of a modal navigation drawer
 */
@ExperimentalMaterial3Api
@Composable
fun ModalDrawerSheet(
    modifier: Modifier = Modifier,
    drawerShape: Shape = DrawerDefaults.shape,
    drawerContainerColor: Color = MaterialTheme.colorScheme.surface,
    drawerContentColor: Color = contentColorFor(drawerContainerColor),
    drawerTonalElevation: Dp = DrawerDefaults.ModalDrawerElevation,
    windowInsets: WindowInsets = DrawerDefaults.windowInsets,
    content: @Composable ColumnScope.() -> Unit,
) {
    DrawerSheet(
        windowInsets,
        modifier,
        drawerShape,
        drawerContainerColor,
        drawerContentColor,
        drawerTonalElevation,
        content,
    )
}

/**
 * Content inside of a dismissible navigation drawer.
 *
 * @param modifier the [Modifier] to be applied to this drawer's content
 * @param drawerShape defines the shape of this drawer's container
 * @param drawerContainerColor the color used for the background of this drawer. Use
 * [Color.Transparent] to have no color.
 * @param drawerContentColor the preferred color for content inside this drawer. Defaults to either
 * the matching content color for [drawerContainerColor], or to the current [LocalContentColor] if
 * [drawerContainerColor] is not a color from the theme.
 * @param drawerTonalElevation when [drawerContainerColor] is [ColorScheme.surface], a translucent
 * primary color overlay is applied on top of the container. A higher tonal elevation value will
 * result in a darker color in light theme and lighter color in dark theme. See also: [Surface].
 * @param windowInsets a window insets for the sheet.
 * @param content content inside of a dismissible navigation drawer
 */
@ExperimentalMaterial3Api
@Composable
fun DismissibleDrawerSheet(
    modifier: Modifier = Modifier,
    drawerShape: Shape = RectangleShape,
    drawerContainerColor: Color = MaterialTheme.colorScheme.surface,
    drawerContentColor: Color = contentColorFor(drawerContainerColor),
    drawerTonalElevation: Dp = DrawerDefaults.DismissibleDrawerElevation,
    windowInsets: WindowInsets = DrawerDefaults.windowInsets,
    content: @Composable ColumnScope.() -> Unit,
) {
    DrawerSheet(
        windowInsets,
        modifier,
        drawerShape,
        drawerContainerColor,
        drawerContentColor,
        drawerTonalElevation,
        content,
    )
}

/**
 * Content inside of a permanent navigation drawer.
 *
 * @param modifier the [Modifier] to be applied to this drawer's content
 * @param drawerShape defines the shape of this drawer's container
 * @param drawerContainerColor the color used for the background of this drawer. Use
 * [Color.Transparent] to have no color.
 * @param drawerContentColor the preferred color for content inside this drawer. Defaults to either
 * the matching content color for [drawerContainerColor], or to the current [LocalContentColor] if
 * [drawerContainerColor] is not a color from the theme.
 * @param drawerTonalElevation when [drawerContainerColor] is [ColorScheme.surface], a translucent
 * primary color overlay is applied on top of the container. A higher tonal elevation value will
 * result in a darker color in light theme and lighter color in dark theme. See also: [Surface].
 * @param windowInsets a window insets for the sheet.
 * @param content content inside a permanent navigation drawer
 */
@ExperimentalMaterial3Api
@Composable
fun PermanentDrawerSheet(
    modifier: Modifier = Modifier,
    drawerShape: Shape = RectangleShape,
    drawerContainerColor: Color = MaterialTheme.colorScheme.surface,
    drawerContentColor: Color = contentColorFor(drawerContainerColor),
    drawerTonalElevation: Dp = DrawerDefaults.PermanentDrawerElevation,
    windowInsets: WindowInsets = DrawerDefaults.windowInsets,
    content: @Composable ColumnScope.() -> Unit,
) {
    val navigationMenu = getString(Strings.NavigationMenu)
    DrawerSheet(
        windowInsets,
        modifier.semantics {
            paneTitle = navigationMenu
        },
        drawerShape,
        drawerContainerColor,
        drawerContentColor,
        drawerTonalElevation,
        content,
    )
}

@ExperimentalMaterial3Api
@Composable
private fun DrawerSheet(
    windowInsets: WindowInsets,
    modifier: Modifier = Modifier,
    drawerShape: Shape = RectangleShape,
    drawerContainerColor: Color = MaterialTheme.colorScheme.surface,
    drawerContentColor: Color = contentColorFor(drawerContainerColor),
    drawerTonalElevation: Dp = DrawerDefaults.PermanentDrawerElevation,
    content: @Composable ColumnScope.() -> Unit,
) {
    Surface(
        modifier =
            modifier
                .sizeIn(
                    minWidth = MinimumDrawerWidth,
                    maxWidth = DrawerDefaults.getMaximumDrawerWidth(),
                )
                .fillMaxHeight(),
        shape = drawerShape,
        color = drawerContainerColor,
        contentColor = drawerContentColor,
        tonalElevation = drawerTonalElevation,
    ) {
        Column(
            Modifier
                .sizeIn(
                    minWidth = MinimumDrawerWidth,
                    maxWidth = DrawerDefaults.getMaximumDrawerWidth(),
                )
                .windowInsetsPadding(windowInsets),
            content = content,
        )
    }
}

/**
 * Object to hold default values for [ModalNavigationDrawer]
 */
@ExperimentalMaterial3Api
object DrawerDefaults {
    /**
     * Default Elevation for drawer container in the [ModalNavigationDrawer] as specified in the
     * Material specification.
     */
    val ModalDrawerElevation = NavigationDrawerTokens.ModalContainerElevation

    /**
     * Default Elevation for drawer container in the [PermanentNavigationDrawer] as specified in the
     * Material specification.
     */
    val PermanentDrawerElevation = NavigationDrawerTokens.StandardContainerElevation

    /**
     * Default Elevation for drawer container in the [DismissibleNavigationDrawer] as specified in
     * the Material specification.
     */
    val DismissibleDrawerElevation = NavigationDrawerTokens.StandardContainerElevation

    /** Default shape for a navigation drawer. */
    val shape: Shape @Composable get() = NavigationDrawerTokens.ContainerShape.toShape()

    /** Default color of the scrim that obscures content when the drawer is open */
    val scrimColor: Color
        @Composable get() = ScrimTokens.ContainerColor.toColor().copy(ScrimTokens.CONTAINER_OPACITY)

    /** Default container color for a navigation drawer */
    val containerColor: Color @Composable get() = NavigationDrawerTokens.ContainerColor.toColor()

    /** Default and maximum width of a navigation drawer **/
    @Composable
    fun getMaximumDrawerWidth() = getContainerWidth()

    /**
     * Default window insets for drawer sheets
     */
    val windowInsets: WindowInsets
        @Composable
        get() =
            WindowInsets.systemBarsForVisualComponents
                .only(WindowInsetsSides.Vertical + WindowInsetsSides.Start)
}

/**
 * Material Design navigation drawer item.
 *
 * A [NavigationDrawerItem] represents a destination within drawers, either [ModalNavigationDrawer],
 * [PermanentNavigationDrawer] or [DismissibleNavigationDrawer].
 *
 * @sample androidx.compose.material3.samples.ModalNavigationDrawerSample
 *
 * @param label text label for this item
 * @param selected whether this item is selected
 * @param onClick called when this item is clicked
 * @param modifier the [Modifier] to be applied to this item
 * @param icon optional icon for this item, typically an [Icon]
 * @param badge optional badge to show on this item from the end side
 * @param colors [NavigationDrawerItemColors] that will be used to resolve the colors used for this
 * item in different states. See [NavigationDrawerItemDefaults.colors].
 * @param interactionSource the [MutableInteractionSource] representing the stream of [Interaction]s
 * for this item. You can create and pass in your own `remember`ed instance to observe
 * [Interaction]s and customize the appearance / behavior of this item in different states.
 */
@Composable
@ExperimentalMaterial3Api
fun NavigationDrawerItem(
    label: @Composable () -> Unit,
    selected: Boolean,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
    icon: (@Composable () -> Unit)? = null,
    badge: (@Composable () -> Unit)? = null,
    shape: Shape = NavigationDrawerTokens.ActiveIndicatorShape.toShape(),
    colors: NavigationDrawerItemColors = NavigationDrawerItemDefaults.colors(),
    interactionSource: MutableInteractionSource = remember { MutableInteractionSource() },
) {
    Surface(
        selected = selected,
        onClick = onClick,
        modifier =
            modifier
                .height(NavigationDrawerTokens.ActiveIndicatorHeight)
                .fillMaxWidth(),
        shape = shape,
        color = colors.containerColor(selected).value,
        interactionSource = interactionSource,
    ) {
        Row(
            Modifier.padding(start = 16.dp, end = 24.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            if (icon != null) {
                val iconColor = colors.iconColor(selected).value
                CompositionLocalProvider(LocalContentColor provides iconColor, content = icon)
                Spacer(Modifier.width(12.dp))
            }
            Box(Modifier.weight(1f)) {
                val labelColor = colors.textColor(selected).value
                CompositionLocalProvider(LocalContentColor provides labelColor, content = label)
            }
            if (badge != null) {
                Spacer(Modifier.width(12.dp))
                val badgeColor = colors.badgeColor(selected).value
                CompositionLocalProvider(LocalContentColor provides badgeColor, content = badge)
            }
        }
    }
}

/** Represents the colors of the various elements of a drawer item. */
@Stable
@ExperimentalMaterial3Api
interface NavigationDrawerItemColors {
    /**
     * Represents the icon color for this item, depending on whether it is [selected].
     *
     * @param selected whether the item is selected
     */
    @Composable
    fun iconColor(selected: Boolean): State<Color>

    /**
     * Represents the text color for this item, depending on whether it is [selected].
     *
     * @param selected whether the item is selected
     */
    @Composable
    fun textColor(selected: Boolean): State<Color>

    /**
     * Represents the badge color for this item, depending on whether it is [selected].
     *
     * @param selected whether the item is selected
     */
    @Composable
    fun badgeColor(selected: Boolean): State<Color>

    /**
     * Represents the container color for this item, depending on whether it is [selected].
     *
     * @param selected whether the item is selected
     */
    @Composable
    fun containerColor(selected: Boolean): State<Color>
}

/** Defaults used in [NavigationDrawerItem]. */
@ExperimentalMaterial3Api
object NavigationDrawerItemDefaults {
    /**
     * Creates a [NavigationDrawerItemColors] with the provided colors according to the Material
     * specification.
     *
     * @param selectedContainerColor the color to use for the background of the item when selected
     * @param unselectedContainerColor the color to use for the background of the item when
     * unselected
     * @param selectedIconColor the color to use for the icon when the item is selected.
     * @param unselectedIconColor the color to use for the icon when the item is unselected.
     * @param selectedTextColor the color to use for the text label when the item is selected.
     * @param unselectedTextColor the color to use for the text label when the item is unselected.
     * @param selectedBadgeColor the color to use for the badge when the item is selected.
     * @param unselectedBadgeColor the color to use for the badge when the item is unselected.
     *
     * @return the resulting [NavigationDrawerItemColors] used for [NavigationDrawerItem]
     */
    @Composable
    fun colors(
        selectedContainerColor: Color = NavigationDrawerTokens.ActiveIndicatorColor.toColor(),
        unselectedContainerColor: Color = NavigationDrawerTokens.ContainerColor.toColor(),
        selectedIconColor: Color = NavigationDrawerTokens.ActiveIconColor.toColor(),
        unselectedIconColor: Color = NavigationDrawerTokens.InactiveIconColor.toColor(),
        selectedTextColor: Color = NavigationDrawerTokens.ActiveLabelTextColor.toColor(),
        unselectedTextColor: Color = NavigationDrawerTokens.InactiveLabelTextColor.toColor(),
        selectedBadgeColor: Color = selectedTextColor,
        unselectedBadgeColor: Color = unselectedTextColor,
    ): NavigationDrawerItemColors =
        DefaultDrawerItemsColor(
            selectedIconColor,
            unselectedIconColor,
            selectedTextColor,
            unselectedTextColor,
            selectedContainerColor,
            unselectedContainerColor,
            selectedBadgeColor,
            unselectedBadgeColor,
        )

    /**
     * Default external padding for a [NavigationDrawerItem] according to the Material
     * specification.
     */
    val ItemPadding = PaddingValues(horizontal = 12.dp)
}

@OptIn(ExperimentalMaterial3Api::class)
private class DefaultDrawerItemsColor(
    val selectedIconColor: Color,
    val unselectedIconColor: Color,
    val selectedTextColor: Color,
    val unselectedTextColor: Color,
    val selectedContainerColor: Color,
    val unselectedContainerColor: Color,
    val selectedBadgeColor: Color,
    val unselectedBadgeColor: Color,
) : NavigationDrawerItemColors {
    @Composable
    override fun iconColor(selected: Boolean): State<Color> {
        return rememberUpdatedState(if (selected) selectedIconColor else unselectedIconColor)
    }

    @Composable
    override fun textColor(selected: Boolean): State<Color> {
        return rememberUpdatedState(if (selected) selectedTextColor else unselectedTextColor)
    }

    @Composable
    override fun containerColor(selected: Boolean): State<Color> {
        return rememberUpdatedState(
            if (selected) selectedContainerColor else unselectedContainerColor,
        )
    }

    @Composable
    override fun badgeColor(selected: Boolean): State<Color> {
        return rememberUpdatedState(
            if (selected) selectedBadgeColor else unselectedBadgeColor,
        )
    }

    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (other !is DefaultDrawerItemsColor) return false

        if (selectedIconColor != other.selectedIconColor) return false
        if (unselectedIconColor != other.unselectedIconColor) return false
        if (selectedTextColor != other.selectedTextColor) return false
        if (unselectedTextColor != other.unselectedTextColor) return false
        if (selectedContainerColor != other.selectedContainerColor) return false
        if (unselectedContainerColor != other.unselectedContainerColor) return false
        if (selectedBadgeColor != other.selectedBadgeColor) return false
        if (unselectedBadgeColor != other.unselectedBadgeColor) return false

        return true
    }

    override fun hashCode(): Int {
        var result = selectedIconColor.hashCode()
        result = 31 * result + unselectedIconColor.hashCode()
        result = 31 * result + selectedTextColor.hashCode()
        result = 31 * result + unselectedTextColor.hashCode()
        result = 31 * result + selectedContainerColor.hashCode()
        result = 31 * result + unselectedContainerColor.hashCode()
        result = 31 * result + selectedBadgeColor.hashCode()
        result = 31 * result + unselectedBadgeColor.hashCode()
        return result
    }
}

private fun calculateFraction(
    a: Float,
    b: Float,
    pos: Float,
) = ((pos - a) / (b - a)).coerceIn(0f, 1f)

@Composable
private fun Scrim(
    open: Boolean,
    onClose: () -> Unit,
    fraction: () -> Float,
    color: Color,
) {
    val closeDrawer = getString(Strings.CloseDrawer)
    val dismissDrawer =
        if (open) {
            Modifier
                .pointerInput(onClose) { detectTapGestures { onClose() } }
                .semantics(mergeDescendants = true) {
                    contentDescription = closeDrawer
                    onClick {
                        onClose()
                        true
                    }
                }
        } else {
            Modifier
        }

    Canvas(
        Modifier
            .fillMaxSize()
            .then(dismissDrawer),
    ) {
        drawRect(color, alpha = fraction())
    }
}

private val DrawerVelocityThreshold = 400.dp
private val MinimumDrawerWidth = 240.dp

// TODO: b/177571613 this should be a proper decay settling
// this is taken from the DrawerLayout's DragViewHelper as a min duration.
private val AnimationSpec = TweenSpec<Float>(durationMillis = 256)

@Immutable
@kotlin.jvm.JvmInline
internal value class Strings private constructor(private val value: Int) {
    companion object {
        val NavigationMenu = Strings(0)
        val CloseDrawer = Strings(1)
    }
}

@Composable
internal fun getString(string: Strings): String {
    LocalConfiguration.current
    val resources = LocalContext.current.resources
    return when (string) {
        Strings.NavigationMenu -> resources.getString(androidx.compose.ui.R.string.navigation_menu)
        Strings.CloseDrawer -> resources.getString(androidx.compose.ui.R.string.close_drawer)
        else -> ""
    }
}

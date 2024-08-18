package com.nononsenseapps.feeder.ui.compose.pullrefresh

import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.input.nestedscroll.NestedScrollConnection
import androidx.compose.ui.input.nestedscroll.NestedScrollSource
import androidx.compose.ui.input.nestedscroll.NestedScrollSource.Companion.Drag
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.debugInspectorInfo
import androidx.compose.ui.platform.inspectable
import androidx.compose.ui.unit.Velocity

/**
 * PullRefresh modifier to be used in conjunction with [PullRefreshState]. Provides a connection
 * to the nested scroll system. Based on Android's SwipeRefreshLayout.
 *
 * @sample androidx.compose.material.samples.PullRefreshSample
 *
 * @param state The [PullRefreshState] associated with this pull-to-refresh component.
 * The state will be updated by this modifier.
 * @param enabled If not enabled, all scroll delta and fling velocity will be ignored.
 */
fun Modifier.pullRefresh(
    state: PullRefreshState,
    enabled: Boolean = true,
) = inspectable(
    inspectorInfo =
        debugInspectorInfo {
            name = "pullRefresh"
            properties["state"] = state
            properties["enabled"] = enabled
        },
) {
    Modifier.pullRefresh(state::onPull, { state.onRelease() }, enabled)
}

/**
 * A modifier for building pull-to-refresh components. Provides a connection to the nested scroll
 * system.
 *
 * @sample androidx.compose.material.samples.CustomPullRefreshSample
 *
 * @param onPull Callback for dispatching vertical scroll delta, takes float pullDelta as argument.
 * Positive delta (pulling down) is dispatched only if the child does not consume it (i.e. pulling
 * down despite being at the top of a scrollable component), whereas negative delta (swiping up) is
 * dispatched first (in case it is needed to push the indicator back up), and then whatever is not
 * consumed is passed on to the child.
 * @param onRelease Callback for when drag is released, takes float flingVelocity as argument.
 * @param enabled If not enabled, all scroll delta and fling velocity will be ignored and neither
 * [onPull] nor [onRelease] will be invoked.
 */
fun Modifier.pullRefresh(
    onPull: (pullDelta: Float) -> Float,
    onRelease: suspend (flingVelocity: Float) -> Unit,
    enabled: Boolean = true,
) = inspectable(
    inspectorInfo =
        debugInspectorInfo {
            name = "pullRefresh"
            properties["onPull"] = onPull
            properties["onRelease"] = onRelease
            properties["enabled"] = enabled
        },
) {
    Modifier.nestedScroll(PullRefreshNestedScrollConnection(onPull, onRelease, enabled))
}

private class PullRefreshNestedScrollConnection(
    private val onPull: (pullDelta: Float) -> Float,
    private val onRelease: suspend (flingVelocity: Float) -> Unit,
    private val enabled: Boolean,
) : NestedScrollConnection {
    override fun onPreScroll(
        available: Offset,
        source: NestedScrollSource,
    ): Offset =
        when {
            !enabled -> Offset.Zero
            source == Drag && available.y < 0 -> Offset(0f, onPull(available.y)) // Swiping up
            else -> Offset.Zero
        }

    override fun onPostScroll(
        consumed: Offset,
        available: Offset,
        source: NestedScrollSource,
    ): Offset =
        when {
            !enabled -> Offset.Zero
            source == Drag && available.y > 0 -> Offset(0f, onPull(available.y)) // Pulling down
            else -> Offset.Zero
        }

    override suspend fun onPreFling(available: Velocity): Velocity {
        onRelease(available.y)
        return Velocity.Zero
    }
}

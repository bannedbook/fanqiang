package com.nononsenseapps.feeder.ui.compose.pullrefresh

import androidx.compose.animation.core.animate
import androidx.compose.material.ExperimentalMaterialApi
import androidx.compose.runtime.Composable
import androidx.compose.runtime.SideEffect
import androidx.compose.runtime.Stable
import androidx.compose.runtime.State
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.setValue
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.launch
import kotlin.math.abs
import kotlin.math.pow

/**
 * Creates a [PullRefreshState] that is remembered across compositions.
 *
 * Changes to [refreshing] will result in [PullRefreshState] being updated.
 *
 * @sample androidx.compose.material.samples.PullRefreshSample
 *
 * @param refreshing A boolean representing whether a refresh is currently occurring.
 * @param onRefresh The function to be called to trigger a refresh.
 * @param refreshThreshold The threshold below which, if a release
 * occurs, [onRefresh] will be called.
 * @param refreshingOffset The offset at which the indicator will be drawn while refreshing. This
 * offset corresponds to the position of the bottom of the indicator.
 */
@OptIn(ExperimentalMaterialApi::class)
@Composable
fun rememberPullRefreshState(
    refreshing: Boolean,
    onRefresh: () -> Unit,
    refreshThreshold: Dp = PullRefreshDefaults.RefreshThreshold,
    refreshingOffset: Dp = PullRefreshDefaults.RefreshingOffset,
): PullRefreshState {
    require(refreshThreshold > 0.dp) { "The refresh trigger must be greater than zero!" }

    val scope = rememberCoroutineScope()
    val onRefreshState = rememberUpdatedState(onRefresh)
    val thresholdPx: Float
    val refreshingOffsetPx: Float

    with(LocalDensity.current) {
        thresholdPx = refreshThreshold.toPx()
        refreshingOffsetPx = refreshingOffset.toPx()
    }

    // refreshThreshold and refreshingOffset should not be changed after instantiation, so any
    // changes to these values are ignored.
    val state =
        remember(scope) {
            PullRefreshState(scope, onRefreshState, refreshingOffsetPx, thresholdPx)
        }

    SideEffect {
        state.setRefreshing(refreshing)
    }

    return state
}

/**
 * A state object that can be used in conjunction with [pullRefresh] to add pull-to-refresh
 * behaviour to a scroll component. Based on Android's SwipeRefreshLayout.
 *
 * Provides [progress], a float representing how far the user has pulled as a percentage of the
 * refreshThreshold. Values of one or less indicate that the user has not yet pulled past the
 * threshold. Values greater than one indicate how far past the threshold the user has pulled.
 *
 * Can be used in conjunction with [pullRefreshIndicatorTransform] to implement Android-like
 * pull-to-refresh behaviour with a custom indicator.
 *
 * Should be created using [rememberPullRefreshState].
 */
@Stable
class PullRefreshState internal constructor(
    private val animationScope: CoroutineScope,
    private val onRefreshState: State<() -> Unit>,
    private val refreshingOffset: Float,
    internal val threshold: Float,
) {
    /**
     * A float representing how far the user has pulled as a percentage of the refreshThreshold.
     *
     * If the component has not been pulled at all, progress is zero. If the pull has reached
     * halfway to the threshold, progress is 0.5f. A value greater than 1 indicates that pull has
     * gone beyond the refreshThreshold - e.g. a value of 2f indicates that the user has pulled to
     * two times the refreshThreshold.
     */
    val progress get() = adjustedDistancePulled / threshold

    internal val refreshing get() = _refreshing
    internal val position get() = _position

    private val adjustedDistancePulled by derivedStateOf { distancePulled * DRAG_MULTIPLIER }

    private var _refreshing by mutableStateOf(false)
    private var _position by mutableFloatStateOf(0f)
    private var distancePulled by mutableFloatStateOf(0f)

    internal fun onPull(pullDelta: Float): Float {
        if (this._refreshing) return 0f // Already refreshing, do nothing.

        val newOffset = (distancePulled + pullDelta).coerceAtLeast(0f)
        val dragConsumed = newOffset - distancePulled
        distancePulled = newOffset
        _position = calculateIndicatorPosition()
        return dragConsumed
    }

    internal fun onRelease() {
        if (!this._refreshing) {
            if (adjustedDistancePulled > threshold) {
                onRefreshState.value()
            } else {
                animateIndicatorTo(0f)
            }
        }
        distancePulled = 0f
    }

    internal fun setRefreshing(refreshing: Boolean) {
        if (this._refreshing != refreshing) {
            this._refreshing = refreshing
            this.distancePulled = 0f
            animateIndicatorTo(if (refreshing) refreshingOffset else 0f)
        }
    }

    private fun animateIndicatorTo(offset: Float) =
        animationScope.launch {
            animate(initialValue = _position, targetValue = offset) { value, _ ->
                _position = value
            }
        }

    private fun calculateIndicatorPosition(): Float =
        when {
            // If drag hasn't gone past the threshold, the position is the adjustedDistancePulled.
            adjustedDistancePulled <= threshold -> adjustedDistancePulled
            else -> {
                // How far beyond the threshold pull has gone, as a percentage of the threshold.
                val overshootPercent = abs(progress) - 1.0f
                // Limit the overshoot to 200%. Linear between 0 and 200.
                val linearTension = overshootPercent.coerceIn(0f, 2f)
                // Non-linear tension. Increases with linearTension, but at a decreasing rate.
                val tensionPercent = linearTension - linearTension.pow(2) / 4
                // The additional offset beyond the threshold.
                val extraOffset = threshold * tensionPercent
                threshold + extraOffset
            }
        }
}

/**
 * Default parameter values for [rememberPullRefreshState].
 */
@ExperimentalMaterialApi
object PullRefreshDefaults {
    /**
     * If the indicator is below this threshold offset when it is released, a refresh
     * will be triggered.
     */
    val RefreshThreshold = 80.dp

    /**
     * The offset at which the indicator should be rendered whilst a refresh is occurring.
     */
    val RefreshingOffset = 56.dp
}

/**
 * The distance pulled is multiplied by this value to give us the adjusted distance pulled, which
 * is used in calculating the indicator position (when the adjusted distance pulled is less than
 * the refresh threshold, it is the indicator position, otherwise the indicator position is
 * derived from the progress).
 */
private const val DRAG_MULTIPLIER = 0.5f

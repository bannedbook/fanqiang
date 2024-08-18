package com.nononsenseapps.feeder.ui.compose.pullrefresh

import androidx.compose.animation.Crossfade
import androidx.compose.animation.core.LinearOutSlowInEasing
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.runtime.Composable
import androidx.compose.runtime.Immutable
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.geometry.center
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.PathFillType
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.drawscope.rotate
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.dp
import kotlin.math.abs
import kotlin.math.max
import kotlin.math.min
import kotlin.math.pow

/**
 * Forked official PullToRefresh to make it Material3 and because they insist on making state
 * internal, this meant forking the whole shebang.
 */
@Composable
fun PullRefreshIndicator(
    refreshing: Boolean,
    state: PullRefreshState,
    modifier: Modifier = Modifier,
    backgroundColor: Color = MaterialTheme.colorScheme.surface,
    contentColor: Color = MaterialTheme.colorScheme.onSurface,
    scale: Boolean = false,
) {
    val showElevation by remember(refreshing, state) {
        derivedStateOf { refreshing || state.position > 0.5f }
    }

    Surface(
        modifier =
            modifier
                .size(IndicatorSize)
                .pullRefreshIndicatorTransform(state, scale),
        shape = SpinnerShape,
        color = backgroundColor,
        tonalElevation = if (showElevation) Elevation else 0.dp,
    ) {
        Crossfade(
            targetState = refreshing,
            label = "PullToRefresh",
            animationSpec = tween(durationMillis = CROSSFADE_DURATION_MS),
        ) { refreshing ->
            Box(
                modifier = Modifier.fillMaxSize(),
                contentAlignment = Alignment.Center,
            ) {
                val spinnerSize = (ArcRadius + StrokeWidth).times(2)

                if (refreshing) {
                    CircularProgressIndicator(
                        color = contentColor,
                        strokeWidth = StrokeWidth,
                        modifier = Modifier.size(spinnerSize),
                    )
                } else {
                    CircularArrowIndicator(state, contentColor, Modifier.size(spinnerSize))
                }
            }
        }
    }
}

@Composable
private fun CircularArrowIndicator(
    state: PullRefreshState,
    color: Color,
    modifier: Modifier = Modifier,
) {
    val path = remember { Path().apply { fillType = PathFillType.EvenOdd } }

    Canvas(modifier.semantics { contentDescription = "Refreshing" }) {
        val values = ArrowValues(state.progress)

        rotate(degrees = values.rotation) {
            val arcRadius = ArcRadius.toPx() + StrokeWidth.toPx() / 2f
            val arcBounds =
                Rect(
                    size.center.x - arcRadius,
                    size.center.y - arcRadius,
                    size.center.x + arcRadius,
                    size.center.y + arcRadius,
                )
            drawArc(
                color = color,
                alpha = values.alpha,
                startAngle = values.startAngle,
                sweepAngle = values.endAngle - values.startAngle,
                useCenter = false,
                topLeft = arcBounds.topLeft,
                size = arcBounds.size,
                style =
                    Stroke(
                        width = StrokeWidth.toPx(),
                        cap = StrokeCap.Square,
                    ),
            )
            drawArrow(path, arcBounds, color, values)
        }
    }
}

@Immutable
private class ArrowValues(
    val alpha: Float,
    val rotation: Float,
    val startAngle: Float,
    val endAngle: Float,
    val scale: Float,
)

private fun ArrowValues(progress: Float): ArrowValues {
    // Discard first 40% of progress. Scale remaining progress to full range between 0 and 100%.
    val adjustedPercent = max(min(1f, progress) - 0.4f, 0f) * 5 / 3
    // How far beyond the threshold pull has gone, as a percentage of the threshold.
    val overshootPercent = abs(progress) - 1.0f
    // Limit the overshoot to 200%. Linear between 0 and 200.
    val linearTension = overshootPercent.coerceIn(0f, 2f)
    // Non-linear tension. Increases with linearTension, but at a decreasing rate.
    val tensionPercent = linearTension - linearTension.pow(2) / 4

    // Calculations based on SwipeRefreshLayout specification.
    val alpha = progress.coerceIn(0f, 1f)
    val endTrim = adjustedPercent * MAX_PROGRESS_ARC
    val rotation = (-0.25f + 0.4f * adjustedPercent + tensionPercent) * 0.5f
    val startAngle = rotation * 360
    val endAngle = (rotation + endTrim) * 360
    val scale = min(1f, adjustedPercent)

    return ArrowValues(alpha, rotation, startAngle, endAngle, scale)
}

private fun DrawScope.drawArrow(
    arrow: Path,
    bounds: Rect,
    color: Color,
    values: ArrowValues,
) {
    arrow.reset()
    arrow.moveTo(0f, 0f) // Move to left corner
    arrow.lineTo(x = ArrowWidth.toPx() * values.scale, y = 0f) // Line to right corner

    // Line to tip of arrow
    arrow.lineTo(
        x = ArrowWidth.toPx() * values.scale / 2,
        y = ArrowHeight.toPx() * values.scale,
    )

    val radius = min(bounds.width, bounds.height) / 2f
    val inset = ArrowWidth.toPx() * values.scale / 2f
    arrow.translate(
        Offset(
            x = radius + bounds.center.x - inset,
            y = bounds.center.y + StrokeWidth.toPx() / 2f,
        ),
    )
    arrow.close()
    rotate(degrees = values.endAngle) {
        drawPath(path = arrow, color = color, alpha = values.alpha)
    }
}

private const val CROSSFADE_DURATION_MS = 100
private const val MAX_PROGRESS_ARC = 0.8f

private val IndicatorSize = 40.dp
private val SpinnerShape = CircleShape
private val ArcRadius = 7.5.dp
private val StrokeWidth = 2.5.dp
private val ArrowWidth = 10.dp
private val ArrowHeight = 5.dp
private val Elevation = 6.dp

@Composable
@Suppress("ktlint:compose:modifier-composable-check")
fun Modifier.pullRefreshIndicatorTransform(
    state: PullRefreshState,
    scale: Boolean = false,
): Modifier {
    var height by remember { mutableIntStateOf(0) }

    return this.then(
        Modifier
            .onSizeChanged { height = it.height }
            .graphicsLayer {
                translationY = state.position - height

                if (scale && !state.refreshing) {
                    val scaleFraction =
                        LinearOutSlowInEasing
                            .transform(state.position / state.threshold)
                            .coerceIn(0f, 1f)
                    scaleX = scaleFraction
                    scaleY = scaleFraction
                }
            },
    )
}

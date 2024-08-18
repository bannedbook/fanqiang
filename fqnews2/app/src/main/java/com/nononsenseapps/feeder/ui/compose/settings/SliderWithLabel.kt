package com.nononsenseapps.feeder.ui.compose.settings

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.RowScope
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.Surface
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Slider
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import com.nononsenseapps.feeder.ui.compose.theme.FeederTheme

// Credit to
// https://www.devbitsandbytes.com/jetpack-compose-configuring-slider-with-label/
@Composable
fun SliderWithLabel(
    value: Float,
    valueToLabel: (Float) -> String,
    valueRange: ClosedFloatingPointRange<Float>,
    modifier: Modifier = Modifier,
    steps: Int = 0,
    labelMinWidth: Dp = 28.dp,
    onValueChange: (Float) -> Unit,
) {
    Column(
        modifier = modifier,
    ) {
        BoxWithConstraints(
            modifier = Modifier.fillMaxWidth(),
        ) {
            val offset =
                getSliderOffset(
                    value = value,
                    valueRange = valueRange,
                    boxWidth = maxWidth,
                    // Since we use a padding of 4.dp on either sides of the SliderLabel, we need to account for this in our calculation
                    labelWidth = labelMinWidth + 8.dp,
                )

            SliderLabel(
                label = valueToLabel(value),
                modifier =
                    Modifier
                        .padding(start = offset),
                minWidth = labelMinWidth,
            )
        }

        Slider(
            value = value,
            onValueChange = onValueChange,
            valueRange = valueRange,
            steps = steps,
            modifier = Modifier.fillMaxWidth(),
        )
    }
}

@Composable
fun SliderWithEndLabels(
    value: Float,
    startLabel: @Composable (RowScope.() -> Unit),
    endLabel: @Composable (RowScope.() -> Unit),
    valueRange: ClosedFloatingPointRange<Float>,
    modifier: Modifier = Modifier,
    steps: Int = 0,
    onValueChange: (Float) -> Unit,
) {
    Column(
        modifier = modifier,
    ) {
        Row(
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.Bottom,
            modifier = Modifier.fillMaxWidth(),
        ) {
            startLabel()
            endLabel()
        }

        Slider(
            value = value,
            onValueChange = onValueChange,
            valueRange = valueRange,
            steps = steps,
            modifier = Modifier.fillMaxWidth(),
        )
    }
}

@Composable
fun SliderLabel(
    label: String,
    modifier: Modifier = Modifier,
    minWidth: Dp = 28.dp,
) {
    Box(
        contentAlignment = Alignment.Center,
        modifier =
            modifier
                .background(
                    color = MaterialTheme.colorScheme.primary,
                    shape = RoundedCornerShape(10.dp),
                )
                .padding(4.dp)
                .size(minWidth),
    ) {
        Text(
            label,
            textAlign = TextAlign.Center,
            color = MaterialTheme.colorScheme.onPrimary,
            style = MaterialTheme.typography.labelMedium,
            maxLines = 1,
            modifier = Modifier,
        )
    }
}

private fun getSliderOffset(
    value: Float,
    valueRange: ClosedFloatingPointRange<Float>,
    boxWidth: Dp,
    labelWidth: Dp,
): Dp {
    val coerced = value.coerceIn(valueRange.start, valueRange.endInclusive)
    val positionFraction = calcFraction(valueRange.start, valueRange.endInclusive, coerced)

    return (boxWidth - labelWidth) * positionFraction
}

// Calculate the 0..1 fraction that `pos` value represents between `a` and `b`
private fun calcFraction(
    a: Float,
    b: Float,
    pos: Float,
) = (if (b - a == 0f) 0f else (pos - a) / (b - a)).coerceIn(0f, 1f)

@Preview
@Composable
private fun PreviewSliderWithLabel() {
    FeederTheme {
        Surface {
            var value by remember {
                mutableFloatStateOf(1f)
            }
            SliderWithLabel(
                value = value,
                onValueChange = { value = it },
                valueToLabel = { "%.1fx".format(value) },
                valueRange = 1f..2f,
                steps = 9,
            )
        }
    }
}

@Preview
@Composable
private fun PreviewSliderWithEndLabels() {
    FeederTheme {
        Surface {
            var value by remember {
                mutableFloatStateOf(1f)
            }
            SliderWithEndLabels(
                value = value,
                startLabel = {
                    Text(
                        "A",
                        style =
                            MaterialTheme.typography.bodyLarge
                                .merge(
                                    TextStyle(
                                        color = MaterialTheme.colorScheme.onBackground,
                                        fontSize = MaterialTheme.typography.bodyLarge.fontSize,
                                    ),
                                ),
                        modifier = Modifier.alignByBaseline(),
                    )
                },
                endLabel = {
                    Text(
                        "A",
                        style =
                            MaterialTheme.typography.bodyLarge
                                .merge(
                                    TextStyle(
                                        color = MaterialTheme.colorScheme.onBackground,
                                        fontSize = MaterialTheme.typography.bodyLarge.fontSize * 2,
                                    ),
                                ),
                        modifier = Modifier.alignByBaseline(),
                    )
                },
                valueRange = 1f..2f,
                steps = 9,
            ) { value = it }
        }
    }
}

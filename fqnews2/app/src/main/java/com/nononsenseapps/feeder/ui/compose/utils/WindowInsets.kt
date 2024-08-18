package com.nononsenseapps.feeder.ui.compose.utils

import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.add
import androidx.compose.runtime.Composable
import androidx.compose.ui.platform.LocalLayoutDirection
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.LayoutDirection
import androidx.compose.ui.unit.dp

fun WindowInsets.addMargin(all: Dp = 0.dp) = addMargin(vertical = all, horizontal = all)

fun WindowInsets.addMargin(
    vertical: Dp = 0.dp,
    horizontal: Dp = 0.dp,
) = addMargin(
    left = horizontal,
    right = horizontal,
    top = vertical,
    bottom = vertical,
)

@Composable
fun WindowInsets.addMarginLayout(
    start: Dp = 0.dp,
    end: Dp = 0.dp,
    top: Dp = 0.dp,
    bottom: Dp = 0.dp,
): WindowInsets {
    val layoutDirection = LocalLayoutDirection.current
    return addMargin(
        left =
            when (layoutDirection) {
                LayoutDirection.Ltr -> start
                LayoutDirection.Rtl -> end
            },
        right =
            when (layoutDirection) {
                LayoutDirection.Ltr -> end
                LayoutDirection.Rtl -> start
            },
        top = top,
        bottom = bottom,
    )
}

fun WindowInsets.addMargin(
    left: Dp = 0.dp,
    right: Dp = 0.dp,
    top: Dp = 0.dp,
    bottom: Dp = 0.dp,
) = add(
    WindowInsets(
        left = left,
        right = right,
        top = top,
        bottom = bottom,
    ),
)

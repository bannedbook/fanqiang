package com.nononsenseapps.feeder.ui.compose.utils

import android.app.Activity
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.ProvidableCompositionLocal
import androidx.compose.runtime.compositionLocalOf
import androidx.compose.ui.graphics.toComposeRect
import androidx.window.layout.FoldingFeature
import com.google.accompanist.adaptive.calculateDisplayFeatures

class FoldableHinge(
    val bounds: androidx.compose.ui.geometry.Rect,
) {
    val isTopToBottom: Boolean = bounds.top == 0.0f
    val isLeftToRight: Boolean = bounds.left == 0.0f
}

val LocalFoldableHinge: ProvidableCompositionLocal<FoldableHinge?> =
    compositionLocalOf { error("Missing FoldableHinge container!") }

@Composable
fun Activity.withFoldableHinge(content: @Composable () -> Unit) {
    val displayFeatures = calculateDisplayFeatures(this)
    val fold =
        displayFeatures.find {
            it is FoldingFeature
        } as FoldingFeature?

    val foldableHinge =
        fold?.let {
            FoldableHinge(it.bounds.toComposeRect())
        }

    CompositionLocalProvider(LocalFoldableHinge provides foldableHinge) {
        content()
    }
}

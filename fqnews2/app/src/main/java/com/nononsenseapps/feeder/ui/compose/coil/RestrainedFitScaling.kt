package com.nononsenseapps.feeder.ui.compose.coil

import androidx.compose.runtime.Stable
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.layout.ScaleFactor
import kotlin.math.max
import kotlin.math.min

/**
 * Scales an image to fit the destination size, such that the largest dimension equals the size
 * of the destination.
 *
 * However, the scaling will never exceed the designated pixel density, which is at least 1.
 */
@Stable
class RestrainedFitScaling(private val pixelDensity: Float) : ContentScale {
    override fun computeScaleFactor(
        srcSize: Size,
        dstSize: Size,
    ): ScaleFactor {
        val minFillScaling = computeFillMinDimension(srcSize, dstSize)
        val minScaling = min(pixelDensity.coerceAtLeast(1f), minFillScaling)
        return ScaleFactor(minScaling, minScaling)
    }
}

/**
 * Scales an image to fill the destination size, such that the smallest dimension equals the size
 * of the destination.
 *
 * However, the scaling will never exceed the designated pixel density, which is at least 1.
 */
@Stable
class RestrainedCropScaling(private val pixelDensity: Float) : ContentScale {
    override fun computeScaleFactor(
        srcSize: Size,
        dstSize: Size,
    ): ScaleFactor {
        val maxFillScaling = computeFillMaxDimension(srcSize, dstSize)
        val minScaling = min(pixelDensity.coerceAtLeast(1f), maxFillScaling)
        return ScaleFactor(minScaling, minScaling)
    }
}

/**
 * Scales an image to fill the destination width.
 *
 * However, the scaling will never exceed the designated pixel density, which is at least 1.
 */
@Stable
class RestrainedFillWidthScaling(private val pixelDensity: Float) : ContentScale {
    override fun computeScaleFactor(
        srcSize: Size,
        dstSize: Size,
    ): ScaleFactor {
        val fillWidthScaling = computeFillWidth(srcSize, dstSize)
        val minScaling = min(pixelDensity.coerceAtLeast(1f), fillWidthScaling)
        return ScaleFactor(minScaling, minScaling)
    }
}

private fun computeFillMaxDimension(
    srcSize: Size,
    dstSize: Size,
): Float {
    val widthScale = computeFillWidth(srcSize, dstSize)
    val heightScale = computeFillHeight(srcSize, dstSize)
    return max(widthScale, heightScale)
}

private fun computeFillMinDimension(
    srcSize: Size,
    dstSize: Size,
): Float {
    val widthScale = computeFillWidth(srcSize, dstSize)
    val heightScale = computeFillHeight(srcSize, dstSize)
    return min(widthScale, heightScale)
}

private fun computeFillWidth(
    srcSize: Size,
    dstSize: Size,
): Float = dstSize.width / srcSize.width

private fun computeFillHeight(
    srcSize: Size,
    dstSize: Size,
): Float = dstSize.height / srcSize.height

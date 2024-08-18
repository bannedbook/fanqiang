package com.nononsenseapps.feeder.ui.compose.material3

/** Linear interpolation between `startValue` and `endValue` by `fraction`.  */
fun lerp(
    startValue: Float,
    endValue: Float,
    fraction: Float,
): Float {
    return startValue + fraction * (endValue - startValue)
}

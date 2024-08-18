package io.nekohasekai.sagernet.ktx

import android.content.res.Resources
import kotlin.math.ceil

private val density = Resources.getSystem().displayMetrics.density

fun dp2pxf(dpValue: Int): Float {
    return density * dpValue
}

fun dp2px(dpValue: Int): Int {
    return ceil(dp2pxf(dpValue)).toInt()
}

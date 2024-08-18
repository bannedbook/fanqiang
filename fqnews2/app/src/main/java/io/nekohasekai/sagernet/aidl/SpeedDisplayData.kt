package io.nekohasekai.sagernet.aidl

import android.os.Parcelable
import kotlinx.parcelize.Parcelize

@Parcelize
data class SpeedDisplayData(
    // Bytes per second
    var txRateProxy: Long = 0L,
    var rxRateProxy: Long = 0L,
    var txRateDirect: Long = 0L,
    var rxRateDirect: Long = 0L,

    // Bytes for the current session
    // Outbound "bypass" usage is not counted
    var txTotal: Long = 0L,
    var rxTotal: Long = 0L,
) : Parcelable

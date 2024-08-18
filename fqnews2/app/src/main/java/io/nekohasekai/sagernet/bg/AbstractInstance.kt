package io.nekohasekai.sagernet.bg

import java.io.Closeable

interface AbstractInstance : Closeable {

    fun launch()

}
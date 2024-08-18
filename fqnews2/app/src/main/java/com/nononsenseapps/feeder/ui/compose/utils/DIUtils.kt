package com.nononsenseapps.feeder.ui.compose.utils

import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import com.nononsenseapps.feeder.ApplicationCoroutineScope
import org.kodein.di.compose.LocalDI
import org.kodein.di.direct
import org.kodein.di.instance

@Composable
fun rememberApplicationCoroutineScope(): ApplicationCoroutineScope {
    val di = LocalDI.current
    return remember {
        di.direct.instance<ApplicationCoroutineScope>()
    }
}

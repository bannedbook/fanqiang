package com.nononsenseapps.feeder.ui.compose.utils

import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.lazy.LazyListState
import androidx.compose.foundation.lazy.staggeredgrid.LazyStaggeredGridState
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.State
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.snapshotFlow
import kotlinx.coroutines.flow.distinctUntilChanged

/**
 * Becomes true when any pixels of the item are visible on screen, false otherwise
 */
@Composable
fun LazyListState.rememberIsItemVisible(key: Any): State<Boolean> {
    val isVisible =
        remember {
            mutableStateOf(layoutInfo.visibleItemsInfo.any { it.key == key })
        }
    LaunchedEffect(this, key) {
        snapshotFlow {
            layoutInfo.visibleItemsInfo.any { it.key == key }
        }
            .distinctUntilChanged()
            .collect {
                isVisible.value = it
            }
    }
    return isVisible
}

/**
 * Becomes true when any pixels of the item are visible on screen, false otherwise
 *
 * Assumes vertical scrolling.
 */
@OptIn(ExperimentalFoundationApi::class)
@Composable
fun LazyStaggeredGridState.rememberIsItemVisible(key: Any): State<Boolean> {
    val isVisible =
        remember {
            mutableStateOf(layoutInfo.visibleItemsInfo.any { it.key == key })
        }
    LaunchedEffect(this, key) {
        snapshotFlow {
            layoutInfo.visibleItemsInfo.any { it.key == key }
        }
            .distinctUntilChanged()
            .collect {
                isVisible.value = it
            }
    }
    return isVisible
}

/**
 * Becomes true the item is mostly visible on screen.
 */
@Composable
fun LazyListState.rememberIsItemMostlyVisible(
    key: Any,
    screenHeightPx: Int,
): State<Boolean> {
    val result =
        remember {
            mutableStateOf(false)
        }
    LaunchedEffect(this, key) {
        snapshotFlow {
            val item =
                layoutInfo.visibleItemsInfo.firstOrNull { it.key == key }
                    ?: return@snapshotFlow false

            /*
             // constrained height
            val h = item.size.coerceAtMost(screenHeightPx)
            // real bottom
            val b = item.offset + item.size
            // visible bottom
            val bs = b.coerceAtMost(screenHeightPx)
            // visible top
            val ts = item.offset.coerceAtLeast(0)
            // visible height
            val vh = bs - ts
            // visible percentage
            val vhp = (vh * 100) / h
             */

            val visibleHeightPercentage =
                (
                    (
                        (item.offset + item.size).coerceAtMost(screenHeightPx) -
                            item.offset.coerceAtLeast(
                                0,
                            )
                    ) * 100
                ) / item.size.coerceAtMost(screenHeightPx)

            // true when over limit
            visibleHeightPercentage >= 80
        }
            .distinctUntilChanged()
            .collect {
                result.value = it
            }
    }
    return result
}

/**
 * Becomes true the item is mostly visible on screen.
 *
 * Assumes vertical scrolling.
 */
@OptIn(ExperimentalFoundationApi::class)
@Composable
fun LazyStaggeredGridState.rememberIsItemMostlyVisible(
    key: Any,
    screenHeightPx: Int,
): State<Boolean> {
    val result =
        remember {
            mutableStateOf(false)
        }
    LaunchedEffect(this, key) {
        snapshotFlow {
            val item =
                layoutInfo.visibleItemsInfo.firstOrNull { it.key == key }
                    ?: return@snapshotFlow false

            val visibleHeightPercentage =
                (
                    (
                        (item.offset.y + item.size.height).coerceAtMost(screenHeightPx) -
                            item.offset.y.coerceAtLeast(
                                0,
                            )
                    ) * 100
                ) / item.size.height.coerceAtMost(screenHeightPx)

            // true when over limit
            visibleHeightPercentage >= 80
        }
            .distinctUntilChanged()
            .collect {
                result.value = it
            }
    }
    return result
}

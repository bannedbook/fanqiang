package com.nononsenseapps.feeder.ui.compose.utils

import androidx.compose.runtime.Immutable
import androidx.compose.runtime.Stable

/**
 * An object that is stable on the other hand is not necessarily immutable. A stable class can hold
 * mutable data, but all mutable data needs to notify Compose when they change, so that
 * recomposition can happen as necessary.
 *
 * Some types - such as List - can't be inferred as stable.
 * This class is useful to wrap them to improve performance.
 *
 * See https://chris.banes.dev/composable-metrics/
 */
@Stable
data class StableHolder<T>(val item: T) {
    override fun toString(): String {
        return item.toString()
    }
}

/**
 * An object that is immutable means that ‘all publicly accessible properties and fields will not
 * change after the instance is constructed’. This characteristic means that Compose can detect
 * ‘changes’ between two instances very easily.
 *
 * Some types - such as List - can't be inferred as stable.
 * This class is useful to wrap them to improve performance.
 *
 * See https://chris.banes.dev/composable-metrics/
 */
@Immutable
data class ImmutableHolder<T>(val item: T) {
    override fun toString(): String {
        return item.toString()
    }
}

fun <T> immutableListHolderOf(vararg elements: T): ImmutableHolder<List<T>> =
    ImmutableHolder(
        listOf(*elements),
    )

fun <T> stableListHolderOf(vararg elements: T): StableHolder<List<T>> =
    StableHolder(
        listOf(*elements),
    )

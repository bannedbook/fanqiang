package com.nononsenseapps.feeder.ui

import kotlinx.coroutines.delay
import kotlinx.coroutines.withTimeout

/**
 * Delays until the factory provides the correct object
 */
suspend fun <T> whileNotEq(
    other: Any?,
    timeoutMillis: Long = 500,
    sleepMillis: Long = 50,
    body: (suspend () -> T),
): T =
    withTimeout(timeoutMillis) {
        var item = body.invoke()
        while (item != other) {
            delay(sleepMillis)
            item = body.invoke()
        }
        item
    }

/**
 * Delays until the factory provides a different object
 */
suspend fun <T> whileEq(
    other: Any?,
    timeoutMillis: Long = 500,
    sleepMillis: Long = 50,
    body: (suspend () -> T),
): T =
    withTimeout(timeoutMillis) {
        var item = body.invoke()
        while (item == other) {
            delay(sleepMillis)
            item = body.invoke()
        }
        item
    }

/**
 * Delays until the factory provides a different object
 */
suspend fun <T> untilNotEq(
    other: Any?,
    timeoutMillis: Long = 500,
    sleepMillis: Long = 50,
    body: (suspend () -> T),
): T =
    whileEq(
        other = other,
        timeoutMillis = timeoutMillis,
        sleepMillis = sleepMillis,
        body = body,
    )

/**
 * Delays until the factory provides the correct object
 */
suspend fun <T> untilEq(
    other: Any?,
    timeoutMillis: Long = 500,
    sleepMillis: Long = 50,
    body: (suspend () -> T),
): T =
    whileNotEq(
        other = other,
        timeoutMillis = timeoutMillis,
        sleepMillis = sleepMillis,
        body = body,
    )

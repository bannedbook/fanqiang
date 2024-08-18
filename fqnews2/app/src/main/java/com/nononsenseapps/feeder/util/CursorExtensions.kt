package com.nononsenseapps.feeder.util

import android.database.Cursor

/**
 * Executes the block of code for each cursor position. Once finished the cursor will be pointing beyond the last item.
 * Assumes that the cursor is already pointing before the first item.
 */
inline fun Cursor.forEach(block: (Cursor) -> Unit) {
    while (moveToNext()) {
        block(this)
    }
}

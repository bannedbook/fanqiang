/*
 * Copyright 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.nononsenseapps.feeder.ui.compose.modifiers

import androidx.compose.ui.Modifier
import androidx.compose.ui.input.key.Key
import androidx.compose.ui.input.key.KeyEventType.Companion.KeyUp
import androidx.compose.ui.input.key.key
import androidx.compose.ui.input.key.onPreviewKeyEvent
import androidx.compose.ui.input.key.type

/**
 * Intercepts a key event rather than passing it on to children
 */
fun Modifier.interceptKey(
    key: Key,
    onKeyEvent: () -> Unit,
): Modifier {
    return this.onPreviewKeyEvent {
        if (it.key == key && it.type == KeyUp) { // fire onKeyEvent on KeyUp to prevent duplicates
            onKeyEvent()
            true
        } else {
            it.key == key // only pass the key event to children if it's not the chosen key
        }
    }
}

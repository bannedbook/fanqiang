package com.nononsenseapps.feeder.ui.compose.utils

import androidx.compose.foundation.focusable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.focusProperties
import androidx.compose.ui.input.InputMode
import androidx.compose.ui.input.key.Key
import androidx.compose.ui.input.key.key
import androidx.compose.ui.input.key.onKeyEvent
import androidx.compose.ui.platform.LocalInputModeManager

@Composable
@Suppress("ktlint:compose:modifier-composable-check")
fun Modifier.onKeyEventLikeEscape(action: () -> Unit): Modifier {
    return this.then(
        Modifier.onKeyEvent {
            when (it.key) {
                Key.Escape, Key.Back, Key.NavigateOut -> {
                    action()
                    true
                }

                else -> false
            }
        },
    )
}

@Composable
@Suppress("ktlint:compose:modifier-composable-check")
fun Modifier.focusableInNonTouchMode(
    enabled: Boolean = true,
    interactionSource: MutableInteractionSource? = null,
): Modifier {
    val inputModeManager = LocalInputModeManager.current
    return this.then(
        Modifier
            .focusProperties { canFocus = inputModeManager.inputMode != InputMode.Touch }
            .focusable(enabled, interactionSource),
    )
}

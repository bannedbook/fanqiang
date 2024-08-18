package com.nononsenseapps.feeder.ui.compose.utils

import androidx.compose.runtime.Stable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.SavedStateHandle
import kotlin.reflect.KProperty

@Stable
class DelegatedMutableSavedState<T>(
    private val savedStateHandle: SavedStateHandle,
    defaultValue: T,
    private val onChange: ((T) -> Unit)?,
) {
    private var initialized: Boolean = false
    private var value: T by mutableStateOf(defaultValue)

    operator fun getValue(
        thisRef: Any?,
        property: KProperty<*>,
    ): T {
        if (!initialized) {
            value = savedStateHandle[property.name] ?: value
            initialized = true
            onChange?.invoke(value)
        }
        return value
    }

    operator fun setValue(
        thisRef: Any?,
        property: KProperty<*>,
        value: T,
    ) {
        savedStateHandle[property.name] = value
        this.value = value
        onChange?.invoke(value)
    }
}

fun <T> mutableSavedStateOf(
    state: SavedStateHandle,
    defaultValue: T,
    onChange: ((T) -> Unit)? = null,
) = DelegatedMutableSavedState(state, defaultValue, onChange)

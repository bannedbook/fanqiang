package com.nononsenseapps.feeder.ui

import com.nononsenseapps.feeder.archmodel.DarkThemePreferences
import com.nononsenseapps.feeder.archmodel.Repository
import com.nononsenseapps.feeder.archmodel.ThemeOptions
import com.nononsenseapps.feeder.base.DIAwareViewModel
import kotlinx.coroutines.flow.StateFlow
import org.kodein.di.DI
import org.kodein.di.instance

class CommonActivityViewModel(di: DI) : DIAwareViewModel(di) {
    private val repository: Repository by instance()

    val currentTheme: StateFlow<ThemeOptions> =
        repository.currentTheme

    val darkThemePreference: StateFlow<DarkThemePreferences> =
        repository.preferredDarkTheme

    val dynamicColors: StateFlow<Boolean> =
        repository.useDynamicTheme

    val textScale: StateFlow<Float> =
        repository.textScale
}

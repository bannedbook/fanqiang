package com.nononsenseapps.feeder.ui.compose.utils

import androidx.compose.material3.Surface
import androidx.compose.material3.windowsizeclass.ExperimentalMaterial3WindowSizeClassApi
import androidx.compose.material3.windowsizeclass.WindowSizeClass
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.DpSize
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.nononsenseapps.feeder.archmodel.ThemeOptions
import com.nononsenseapps.feeder.base.DIAwareComponentActivity
import com.nononsenseapps.feeder.base.diAwareViewModel
import com.nononsenseapps.feeder.ui.CommonActivityViewModel
import com.nononsenseapps.feeder.ui.compose.theme.FeederTheme
import com.nononsenseapps.feeder.ui.compose.theme.ProvideFontScale
import org.kodein.di.compose.withDI

@Composable
fun DIAwareComponentActivity.withAllProviders(content: @Composable () -> Unit) {
    withDI {
        val viewModel: CommonActivityViewModel = diAwareViewModel()
        val currentTheme by viewModel.currentTheme.collectAsStateWithLifecycle()
        val darkThemePreference by viewModel.darkThemePreference.collectAsStateWithLifecycle()
        val dynamicColors by viewModel.dynamicColors.collectAsStateWithLifecycle()
        val textScale by viewModel.textScale.collectAsStateWithLifecycle()
        withFoldableHinge {
            withWindowMetrics {
                withWindowSize {
                    FeederTheme(
                        currentTheme = currentTheme,
                        darkThemePreference = darkThemePreference,
                        dynamicColors = dynamicColors,
                    ) {
                        ProvideFontScale(fontScale = textScale) {
                            WithFeederTextToolbar(content)
                        }
                    }
                }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3WindowSizeClassApi::class)
@Composable
@Suppress("ktlint:compose:modifier-missing-check")
fun WithAllPreviewProviders(
    currentTheme: ThemeOptions = ThemeOptions.DAY,
    content: @Composable () -> Unit,
) {
    val dm = LocalContext.current.resources.displayMetrics
    val dpSize =
        with(LocalDensity.current) {
            DpSize(
                dm.widthPixels.toDp(),
                dm.heightPixels.toDp(),
            )
        }
    WithPreviewWindowSize(WindowSizeClass.calculateFromSize(dpSize)) {
        FeederTheme(currentTheme = currentTheme) {
            Surface {
                content()
            }
        }
    }
}

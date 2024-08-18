package com.nononsenseapps.feeder.ui.compose.theme

import android.os.Build
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.ColorScheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.dynamicDarkColorScheme
import androidx.compose.material3.dynamicLightColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import com.google.accompanist.systemuicontroller.rememberSystemUiController
import com.nononsenseapps.feeder.archmodel.DarkThemePreferences
import com.nononsenseapps.feeder.archmodel.ThemeOptions

private val lightColors =
    lightColorScheme(
        primary = md_theme_light_primary,
        onPrimary = md_theme_light_onPrimary,
        primaryContainer = md_theme_light_primaryContainer,
        onPrimaryContainer = md_theme_light_onPrimaryContainer,
        secondary = md_theme_light_secondary,
        onSecondary = md_theme_light_onSecondary,
        secondaryContainer = md_theme_light_secondaryContainer,
        onSecondaryContainer = md_theme_light_onSecondaryContainer,
        tertiary = md_theme_light_tertiary,
        onTertiary = md_theme_light_onTertiary,
        tertiaryContainer = md_theme_light_tertiaryContainer,
        onTertiaryContainer = md_theme_light_onTertiaryContainer,
        error = md_theme_light_error,
        errorContainer = md_theme_light_errorContainer,
        onError = md_theme_light_onError,
        onErrorContainer = md_theme_light_onErrorContainer,
        background = md_theme_light_background,
        onBackground = md_theme_light_onBackground,
        surface = md_theme_light_surface,
        onSurface = md_theme_light_onSurface,
        surfaceVariant = md_theme_light_surfaceVariant,
        onSurfaceVariant = md_theme_light_onSurfaceVariant,
        outline = md_theme_light_outline,
        inverseOnSurface = md_theme_light_inverseOnSurface,
        inverseSurface = md_theme_light_inverseSurface,
        inversePrimary = md_theme_light_inversePrimary,
        surfaceTint = md_theme_light_surfaceTint,
    )

private val darkColors =
    darkColorScheme(
        primary = md_theme_dark_primary,
        onPrimary = md_theme_dark_onPrimary,
        primaryContainer = md_theme_dark_primaryContainer,
        onPrimaryContainer = md_theme_dark_onPrimaryContainer,
        secondary = md_theme_dark_secondary,
        onSecondary = md_theme_dark_onSecondary,
        secondaryContainer = md_theme_dark_secondaryContainer,
        onSecondaryContainer = md_theme_dark_onSecondaryContainer,
        tertiary = md_theme_dark_tertiary,
        onTertiary = md_theme_dark_onTertiary,
        tertiaryContainer = md_theme_dark_tertiaryContainer,
        onTertiaryContainer = md_theme_dark_onTertiaryContainer,
        error = md_theme_dark_error,
        errorContainer = md_theme_dark_errorContainer,
        onError = md_theme_dark_onError,
        onErrorContainer = md_theme_dark_onErrorContainer,
        background = md_theme_dark_background,
        onBackground = md_theme_dark_onBackground,
        surface = md_theme_dark_surface,
        onSurface = md_theme_dark_onSurface,
        surfaceVariant = md_theme_dark_surfaceVariant,
        onSurfaceVariant = md_theme_dark_onSurfaceVariant,
        outline = md_theme_dark_outline,
        inverseOnSurface = md_theme_dark_inverseOnSurface,
        inverseSurface = md_theme_dark_inverseSurface,
        inversePrimary = md_theme_dark_inversePrimary,
        surfaceTint = md_theme_dark_surfaceTint,
    )

private val eInkColors =
    lightColorScheme(
        primary = md_theme_eink_primary,
        onPrimary = md_theme_eink_onPrimary,
        primaryContainer = md_theme_eink_primaryContainer,
        onPrimaryContainer = md_theme_eink_onPrimaryContainer,
        secondary = md_theme_eink_secondary,
        onSecondary = md_theme_eink_onSecondary,
        secondaryContainer = md_theme_eink_secondaryContainer,
        onSecondaryContainer = md_theme_eink_onSecondaryContainer,
        tertiary = md_theme_eink_tertiary,
        onTertiary = md_theme_eink_onTertiary,
        tertiaryContainer = md_theme_eink_tertiaryContainer,
        onTertiaryContainer = md_theme_eink_onTertiaryContainer,
        error = md_theme_eink_error,
        errorContainer = md_theme_eink_errorContainer,
        onError = md_theme_eink_onError,
        onErrorContainer = md_theme_eink_onErrorContainer,
        background = md_theme_eink_background,
        onBackground = md_theme_eink_onBackground,
        surface = md_theme_eink_surface,
        onSurface = md_theme_eink_onSurface,
        surfaceVariant = md_theme_eink_surfaceVariant,
        onSurfaceVariant = md_theme_eink_onSurfaceVariant,
        outline = md_theme_eink_outline,
        inverseOnSurface = md_theme_eink_inverseOnSurface,
        inverseSurface = md_theme_eink_inverseSurface,
        inversePrimary = md_theme_eink_inversePrimary,
        surfaceTint = md_theme_eink_surfaceTint,
    )

/**
 * Only use this in the root of the activity
 */
@Composable
fun FeederTheme(
    currentTheme: ThemeOptions = ThemeOptions.SYSTEM,
    darkThemePreference: DarkThemePreferences = DarkThemePreferences.BLACK,
    dynamicColors: Boolean = false,
    content: @Composable () -> Unit,
) {
    val darkSystemIcons = currentTheme.isDarkSystemIcons()
    val darkNavIcons = currentTheme.isDarkNavIcons()
    val navBarColor = currentTheme.getNavBarColor()
    val colorScheme = currentTheme.getColorScheme(darkThemePreference, dynamicColors)
    MaterialTheme(
        colorScheme = colorScheme,
        typography = FeederTypography.typography,
    ) {
        val systemUiController = rememberSystemUiController()
        val surfaceColor = Color(MaterialTheme.colorScheme.surface.value)
        DisposableEffect(systemUiController, darkSystemIcons, darkNavIcons) {
            systemUiController.setStatusBarColor(
                surfaceColor,
                darkIcons = darkSystemIcons,
            )
            systemUiController.setNavigationBarColor(
                navBarColor,
                darkIcons = darkNavIcons,
            )

            onDispose {}
        }
        ProvideDimens {
            content()
        }
    }
}

@Composable
private fun ThemeOptions.isDarkSystemIcons(): Boolean {
    val isDarkTheme =
        when (this) {
            ThemeOptions.DAY,
            ThemeOptions.E_INK,
            -> false
            ThemeOptions.NIGHT -> true
            ThemeOptions.SYSTEM -> isSystemInDarkTheme()
        }

    return !isDarkTheme
}

@Composable
private fun ThemeOptions.isDarkNavIcons(): Boolean {
    // Only Api 27+ supports dark nav bar icons
    return (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O_MR1) && isDarkSystemIcons()
}

@Composable
private fun ThemeOptions.getNavBarColor(): Color {
    // Api 29 handles transparency
    return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
        Color.Transparent
    } else if (isDarkNavIcons()) {
        NavBarScrimLight
    } else {
        NavBarScrimDark
    }
}

@Composable
private fun ThemeOptions.getColorScheme(
    darkThemePreference: DarkThemePreferences,
    dynamicColors: Boolean,
): ColorScheme {
    val dark =
        when (this) {
            ThemeOptions.DAY,
            ThemeOptions.E_INK,
            -> false
            ThemeOptions.NIGHT -> true
            ThemeOptions.SYSTEM -> {
                isSystemInDarkTheme()
            }
        }

    val colorScheme =
        when {
            Build.VERSION.SDK_INT >= Build.VERSION_CODES.S && dynamicColors && dark -> {
                dynamicDarkColorScheme(LocalContext.current)
            }

            Build.VERSION.SDK_INT >= Build.VERSION_CODES.S && dynamicColors && !dark -> {
                dynamicLightColorScheme(LocalContext.current)
            }

            dark -> darkColors
            else ->
                if (this == ThemeOptions.E_INK) {
                    eInkColors
                } else {
                    lightColors
                }
        }

    return if (dark && darkThemePreference == DarkThemePreferences.BLACK) {
        colorScheme.copy(
            background = Color.Black,
        )
    } else {
        colorScheme
    }
}

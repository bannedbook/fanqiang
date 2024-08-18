package com.nononsenseapps.feeder.ui.compose.theme

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Typography
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.Immutable
import androidx.compose.runtime.staticCompositionLocalOf
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.Hyphens
import androidx.compose.ui.text.style.LineBreak
import androidx.compose.ui.text.style.TextDecoration

object FeederTypography {
    private val materialTypography = Typography()

    val typography: Typography =
        materialTypography.copy(
            headlineLarge =
                materialTypography.headlineLarge.merge(
                    TextStyle(
                        lineBreak = LineBreak.Paragraph,
                    ),
                ),
            headlineMedium =
                materialTypography.headlineMedium.merge(
                    TextStyle(
                        lineBreak = LineBreak.Paragraph,
                    ),
                ),
            headlineSmall =
                materialTypography.headlineSmall.merge(
                    TextStyle(
                        lineBreak = LineBreak.Paragraph,
                    ),
                ),
            bodyLarge =
                materialTypography.bodyLarge.merge(
                    TextStyle(
                        hyphens = Hyphens.Auto,
                        lineBreak = LineBreak.Paragraph,
                    ),
                ),
            bodyMedium =
                materialTypography.bodyMedium.merge(
                    TextStyle(
                        hyphens = Hyphens.Auto,
                        lineBreak = LineBreak.Paragraph,
                    ),
                ),
            bodySmall =
                materialTypography.bodySmall.merge(
                    TextStyle(
                        hyphens = Hyphens.Auto,
                        lineBreak = LineBreak.Paragraph,
                    ),
                ),
        )
}

@Composable
fun LinkTextStyle(): TextStyle =
    TextStyle(
        color = MaterialTheme.colorScheme.primary,
        textDecoration = TextDecoration.Underline,
    )

fun titleFontWeight(unread: Boolean) =
    if (unread) {
        FontWeight.Black
    } else {
        FontWeight.Normal
    }

@Composable
fun FeedListItemTitleTextStyle(): TextStyle =
    MaterialTheme.typography.titleMedium.merge(
        TextStyle(lineBreak = LineBreak.Paragraph, hyphens = Hyphens.Auto),
    )

@Composable
fun FeedListItemSnippetTextStyle(): TextStyle =
    MaterialTheme.typography.titleSmall.merge(
        TextStyle(
            lineBreak = LineBreak.Paragraph,
            hyphens = Hyphens.Auto,
        ),
    )

@Composable
fun FeedListItemStyle(): TextStyle = MaterialTheme.typography.bodyLarge

@Composable
fun FeedListItemFeedTitleStyle(): TextStyle = FeedListItemDateStyle()

@Composable
fun FeedListItemDateStyle(): TextStyle = MaterialTheme.typography.labelMedium

@Composable
fun TTSPlayerStyle(): TextStyle = MaterialTheme.typography.titleMedium

@Composable
fun CodeInlineStyle(): SpanStyle =
    SpanStyle(
        background = CodeBlockBackground(),
        fontFamily = FontFamily.Monospace,
    )

/**
 * Has no background because it is meant to be put over a Surface which has the proper background.
 */
@Composable
fun CodeBlockStyle(): TextStyle =
    MaterialTheme.typography.bodyMedium.merge(
        SpanStyle(
            fontFamily = FontFamily.Monospace,
        ),
    )

@Composable
fun CodeBlockBackground(): Color = MaterialTheme.colorScheme.surfaceVariant

@Composable
fun OnCodeBlockBackground(): Color = MaterialTheme.colorScheme.onSurfaceVariant

@Composable
fun BlockQuoteStyle(): SpanStyle =
    MaterialTheme.typography.bodyLarge.toSpanStyle().merge(
        SpanStyle(
            fontWeight = FontWeight.Light,
        ),
    )

@Immutable
data class TypographySettings(
    val fontScale: Float = 1.0f,
)

val LocalTypographySettings =
    staticCompositionLocalOf {
        TypographySettings()
    }

@Composable
fun ProvideFontScale(
    fontScale: Float,
    content: @Composable () -> Unit,
) {
    val typographySettings =
        TypographySettings(
            fontScale = fontScale,
        )
    CompositionLocalProvider(LocalTypographySettings provides typographySettings, content = content)
}

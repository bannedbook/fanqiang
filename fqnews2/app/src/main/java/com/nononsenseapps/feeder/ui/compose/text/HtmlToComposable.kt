package com.nononsenseapps.feeder.ui.compose.text

import androidx.compose.foundation.layout.BoxWithConstraintsScope
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.text.font.FontFamily
import com.nononsenseapps.feeder.ui.compose.feed.PlainTooltipBox
import com.nononsenseapps.feeder.util.asUTF8Sequence
import org.jsoup.nodes.Element
import org.jsoup.nodes.TextNode
import kotlin.math.roundToInt

fun Element.attrInHierarchy(attr: String): String {
    var current: Element? = this

    while (current != null) {
        val value = current.attr(attr)
        if (value.isNotEmpty()) {
            return value
        }
        current = current.parent()
    }

    return ""
}

fun Element.ancestors(predicate: (Element) -> Boolean): Sequence<Element> {
    return ancestors().filter(predicate)
}

private fun Element.ancestors(): Sequence<Element> {
    return sequence {
        var current: Element? = this@ancestors.parent()

        while (current != null) {
            yield(current)
            current = current.parent()
        }
    }
}

fun String.asFontFamily(): FontFamily? =
    when (this.lowercase()) {
        "monospace" -> FontFamily.Monospace
        "serif" -> FontFamily.Serif
        "sans-serif" -> FontFamily.SansSerif
        else -> null
    }

@Composable
fun BoxWithConstraintsScope.rememberMaxImageWidth() =
    with(LocalDensity.current) {
        remember {
            derivedStateOf {
                maxWidth.toPx().roundToInt().coerceAtMost(2000)
            }
        }
    }

/**
 * Can't use JSoup's text() method because that strips invisible characters
 * such as ZWNJ which are crucial for several languages.
 */
fun TextNode.appendCorrectlyNormalizedWhiteSpace(
    builder: HtmlParser,
    stripLeading: Boolean,
) {
    wholeText.asUTF8Sequence()
        .dropWhile {
            stripLeading && isCollapsableWhiteSpace(it)
        }
        .fold(false) { lastWasWhite, char ->
            if (isCollapsableWhiteSpace(char)) {
                if (!lastWasWhite) {
                    builder.append(' ')
                }
                true
            } else {
                builder.append(char)
                false
            }
        }
}

fun Element.appendCorrectlyNormalizedWhiteSpaceRecursively(
    builder: HtmlParser,
    stripLeading: Boolean,
) {
    for (child in childNodes()) {
        when (child) {
            is TextNode -> child.appendCorrectlyNormalizedWhiteSpace(builder, stripLeading)
            is Element ->
                child.appendCorrectlyNormalizedWhiteSpaceRecursively(
                    builder,
                    stripLeading,
                )
        }
    }
}

private const val SPACE = ' '
private const val TAB = '\t'
private const val LINE_FEED = '\n'
private const val CARRIAGE_RETURN = '\r'

// 12 is form feed which as no escape in kotlin
private const val FORM_FEED = 12.toChar()

// 160 is &nbsp; (non-breaking space). Not in the spec but expected.
private const val NON_BREAKING_SPACE = 160.toChar()

internal fun isCollapsableWhiteSpace(c: String) = c.firstOrNull()?.let { isCollapsableWhiteSpace(it) } ?: false

private fun isCollapsableWhiteSpace(c: Char) = c == SPACE || c == TAB || c == LINE_FEED || c == CARRIAGE_RETURN || c == FORM_FEED || c == NON_BREAKING_SPACE

/**
 * Super basic function to strip html formatting from alt-texts.
 */
fun stripHtml(html: String): String {
    val result = StringBuilder()

    var skipping = false

    for (char in html) {
        if (!skipping) {
            if (char == '<') {
                skipping = true
            } else {
                result.append(char)
            }
        } else {
            if (char == '>') {
                skipping = false
            } else {
                // Skipping char
            }
        }
    }

    return result.toString()
}

@Composable
fun WithTooltipIfNotBlank(
    tooltip: String,
    modifier: Modifier = Modifier,
    content: @Composable () -> Unit,
) {
    if (tooltip.isNotBlank()) {
        PlainTooltipBox(modifier = modifier, tooltip = { Text(tooltip) }) {
            content()
        }
    } else {
        content()
    }
}

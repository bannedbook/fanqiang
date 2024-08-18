package com.nononsenseapps.feeder.ui.compose.text

import androidx.compose.runtime.Composable
import androidx.compose.ui.text.SpanStyle

abstract class HtmlParser {
    protected val spanStack: MutableList<Span> = mutableListOf()

    // The identity of this will change - do not reference it in blocks
    protected var builder: AnnotatedParagraphStringBuilder = AnnotatedParagraphStringBuilder()

    /**
     * returns true if any content was emitted, false otherwise
     */
    abstract fun emitParagraph(): Boolean

    val endsWithWhitespace: Boolean
        get() = builder.endsWithWhitespace

    fun append(text: String) = builder.append(text)

    fun append(char: Char) = builder.append(char)

    fun pop(index: Int) = builder.pop(index)

    fun pushStyle(style: SpanStyle): Int = builder.pushStyle(style)

    fun pushSpan(span: Span) = spanStack.add(span)

    fun pushStringAnnotation(
        tag: String,
        annotation: String,
    ): Int = builder.pushStringAnnotation(tag = tag, annotation = annotation)

    fun popSpan() = spanStack.removeLast()
}

inline fun <R : Any> HtmlParser.withParagraph(crossinline block: HtmlParser.() -> R): R {
    emitParagraph()
    return block(this).also {
        emitParagraph()
    }
}

inline fun <R : Any> HtmlParser.withStyle(
    style: SpanStyle?,
    crossinline block: HtmlParser.() -> R,
): R {
    if (style == null) {
        return block()
    }

    pushSpan(SpanWithStyle(style))
    val index = pushStyle(style)
    return try {
        block()
    } finally {
        pop(index)
        popSpan()
    }
}

inline fun <R : Any> HtmlParser.withAnnotation(
    tag: String,
    annotation: String,
    crossinline block: HtmlParser.() -> R,
): R {
    pushSpan(SpanWithAnnotation(tag = tag, annotation = annotation))
    val index = pushStringAnnotation(tag = tag, annotation = annotation)
    return try {
        block()
    } finally {
        pop(index)
        popSpan()
    }
}

sealed class Span

data class SpanWithStyle(
    val spanStyle: SpanStyle,
) : Span()

data class SpanWithAnnotation(
    val tag: String,
    val annotation: String,
) : Span()

data class SpanWithComposableStyle(
    val spanStyle: @Composable () -> SpanStyle,
) : Span()

data class SpanWithVerbatim(
    val verbatim: String,
) : Span()

package com.nononsenseapps.feeder.ui.compose.text

import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.text.AnnotatedString
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.VerbatimTtsAnnotation

class AnnotatedParagraphStringBuilder {
    // Private for a reason
    private val builder: AnnotatedString.Builder = AnnotatedString.Builder()

    private val poppedComposableStyles = mutableListOf<ComposableStyleWithStartEnd>()
    private val composableStyles = mutableListOf<ComposableStyleWithStartEnd>()
    private val mLastTwoChars: MutableList<Char> = mutableListOf()

    val lastTwoChars: List<Char>
        get() = mLastTwoChars

    val length: Int
        get() = builder.length

    val endsWithWhitespace: Boolean
        get() {
            if (mLastTwoChars.isEmpty()) {
                return true
            }
            mLastTwoChars.peekLatest()?.let { latest ->
                // Non-breaking space (160) is not caught by trim or whitespace identification
                if (latest.isWhitespace() || latest.code == 160) {
                    return true
                }
            }

            return false
        }

    val endsWithNonBreakingSpace: Boolean
        get() {
            if (mLastTwoChars.isEmpty()) {
                return false
            }
            mLastTwoChars.peekLatest()?.let { latest ->
                if (latest.code == 160) {
                    return true
                }
            }

            return false
        }

    fun pushVerbatimTtsAnnotation(verbatim: String) = builder.pushTtsAnnotation(VerbatimTtsAnnotation(verbatim))

    fun pushStyle(style: SpanStyle): Int = builder.pushStyle(style = style)

    fun pop(index: Int) = builder.pop(index)

    fun pushStringAnnotation(
        tag: String,
        annotation: String,
    ): Int = builder.pushStringAnnotation(tag = tag, annotation = annotation)

    fun pushComposableStyle(style: @Composable () -> SpanStyle): Int {
        composableStyles.add(
            ComposableStyleWithStartEnd(
                style = style,
                start = builder.length,
            ),
        )
        return composableStyles.lastIndex
    }

    fun popComposableStyle(index: Int) {
        poppedComposableStyles.add(
            composableStyles.removeAt(index).copy(end = builder.length),
        )
    }

    fun append(text: String) {
        if (text.count() >= 2) {
            mLastTwoChars.pushMaxTwo(text.secondToLast())
        }
        if (text.isNotEmpty()) {
            mLastTwoChars.pushMaxTwo(text.last())
        }
        builder.append(text)
    }

    fun append(char: Char) {
        mLastTwoChars.pushMaxTwo(char)
        builder.append(char)
    }

    @Composable
    fun rememberComposableAnnotatedString(): AnnotatedString {
        for (composableStyle in poppedComposableStyles) {
            builder.addStyle(
                style = composableStyle.style(),
                start = composableStyle.start,
                end = composableStyle.end,
            )
        }
        for (composableStyle in composableStyles) {
            builder.addStyle(
                style = composableStyle.style(),
                start = composableStyle.start,
                end = builder.length,
            )
        }
        return remember {
            builder.toAnnotatedString()
        }
    }

    fun toAnnotatedString(): AnnotatedString {
        return builder.toAnnotatedString()
    }
}

fun AnnotatedParagraphStringBuilder.isEmpty() = lastTwoChars.isEmpty()

fun AnnotatedParagraphStringBuilder.isNotEmpty() = lastTwoChars.isNotEmpty()

private fun CharSequence.secondToLast(): Char {
    if (count() < 2) {
        throw NoSuchElementException("List has less than two items.")
    }
    return this[lastIndex - 1]
}

private fun <T> MutableList<T>.pushMaxTwo(item: T) {
    this.add(0, item)
    if (count() > 2) {
        this.removeLast()
    }
}

private fun <T> List<T>.peekLatest(): T? {
    return this.firstOrNull()
}

private fun <T> List<T>.peekSecondLatest(): T? {
    if (count() < 2) {
        return null
    }
    return this[1]
}

data class ComposableStyleWithStartEnd(
    val style: @Composable () -> SpanStyle,
    val start: Int,
    val end: Int = -1,
)

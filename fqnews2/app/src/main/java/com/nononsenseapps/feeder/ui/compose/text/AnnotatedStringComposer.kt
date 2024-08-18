package com.nononsenseapps.feeder.ui.compose.text

import androidx.compose.ui.text.AnnotatedString

class AnnotatedStringComposer : HtmlParser() {
    private val strings = mutableListOf<AnnotatedString>()

    val result: List<AnnotatedString> =
        strings

    override fun emitParagraph(): Boolean {
        // List items emit dots and non-breaking space. Don't newline after that
        if (builder.isEmpty() || builder.endsWithNonBreakingSpace) {
            // Nothing to emit, and nothing to reset
            return false
        }

        strings.add(builder.toAnnotatedString())

        resetAfterEmit()
        return true
    }

    fun appendTable(block: () -> Unit) {
        emitParagraph()

        block()
    }

    private fun resetAfterEmit() {
        builder = AnnotatedParagraphStringBuilder()

        for (span in spanStack) {
            when (span) {
                is SpanWithStyle -> builder.pushStyle(span.spanStyle)
                is SpanWithAnnotation ->
                    builder.pushStringAnnotation(
                        tag = span.tag,
                        annotation = span.annotation,
                    )

                is SpanWithComposableStyle -> builder.pushComposableStyle(span.spanStyle)
                is SpanWithVerbatim -> builder.pushVerbatimTtsAnnotation(span.verbatim)
            }
        }
    }
}

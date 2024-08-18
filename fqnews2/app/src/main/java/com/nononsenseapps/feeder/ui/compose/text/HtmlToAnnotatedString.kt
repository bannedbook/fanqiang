package com.nononsenseapps.feeder.ui.compose.text

import androidx.compose.ui.text.AnnotatedString
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.style.BaselineShift
import org.jsoup.Jsoup
import org.jsoup.nodes.Element
import org.jsoup.nodes.Node
import org.jsoup.nodes.TextNode
import java.io.InputStream

/**
 * Returns "plain text" with annotations for TTS
 */
fun htmlToAnnotatedString(
    inputStream: InputStream,
    baseUrl: String,
): List<AnnotatedString> =
    Jsoup.parse(inputStream, null, baseUrl)
        ?.body()
        ?.let { body ->
            formatBody(
                element = body,
                baseUrl = baseUrl,
            )
        } ?: emptyList()

private fun formatBody(
    element: Element,
    baseUrl: String,
): List<AnnotatedString> {
    val composer = AnnotatedStringComposer()

    composer.appendTextChildren(
        nodes = element.childNodes(),
        baseUrl = baseUrl,
    )

    composer.emitParagraph()

    return composer.result
}

private fun AnnotatedStringComposer.appendTextChildren(
    nodes: List<Node>,
    preFormatted: Boolean = false,
    baseUrl: String,
) {
    var node = nodes.firstOrNull()
    while (node != null) {
        when (node) {
            is TextNode -> {
                if (preFormatted) {
                    append(node.wholeText)
                } else {
                    node.appendCorrectlyNormalizedWhiteSpace(
                        this,
                        stripLeading = endsWithWhitespace,
                    )
                }
            }

            is Element -> {
                val element = node
                when (element.tagName()) {
                    "p" -> {
                        emitParagraph()
                        // Readability4j inserts p-tags in divs for algorithmic purposes.
                        // They screw up formatting.
                        if (node.hasClass("readability-styled")) {
                            appendTextChildren(
                                element.childNodes(),
                                baseUrl = baseUrl,
                            )
                        } else {
                            withParagraph {
                                appendTextChildren(
                                    element.childNodes(),
                                    baseUrl = baseUrl,
                                )
                            }
                        }

                        emitParagraph()
                    }

                    "br" -> append('\n')
                    "h1" -> {
                        emitParagraph()
                        withParagraph {
                            element.appendCorrectlyNormalizedWhiteSpaceRecursively(
                                this,
                                stripLeading = endsWithWhitespace,
                            )
                        }
                        emitParagraph()
                    }

                    "h2" -> {
                        emitParagraph()
                        withParagraph {
                            element.appendCorrectlyNormalizedWhiteSpaceRecursively(
                                this,
                                stripLeading = endsWithWhitespace,
                            )
                        }
                        emitParagraph()
                    }

                    "h3" -> {
                        emitParagraph()
                        withParagraph {
                            element.appendCorrectlyNormalizedWhiteSpaceRecursively(
                                this,
                                stripLeading = endsWithWhitespace,
                            )
                        }
                        emitParagraph()
                    }

                    "h4" -> {
                        emitParagraph()
                        withParagraph {
                            element.appendCorrectlyNormalizedWhiteSpaceRecursively(
                                this,
                                stripLeading = endsWithWhitespace,
                            )
                        }
                        emitParagraph()
                    }

                    "h5" -> {
                        emitParagraph()
                        withParagraph {
                            element.appendCorrectlyNormalizedWhiteSpaceRecursively(
                                this,
                                stripLeading = endsWithWhitespace,
                            )
                        }
                        emitParagraph()
                    }

                    "h6" -> {
                        emitParagraph()
                        withParagraph {
                            element.appendCorrectlyNormalizedWhiteSpaceRecursively(
                                this,
                                stripLeading = endsWithWhitespace,
                            )
                        }
                        emitParagraph()
                    }

                    "strong", "b" -> {
                        appendTextChildren(
                            element.childNodes(),
                            baseUrl = baseUrl,
                        )
                    }

                    "i", "em", "cite", "dfn" -> {
                        appendTextChildren(
                            element.childNodes(),
                            baseUrl = baseUrl,
                        )
                    }

                    "tt" -> {
                        appendTextChildren(
                            element.childNodes(),
                            baseUrl = baseUrl,
                        )
                    }

                    "u" -> {
                        appendTextChildren(
                            element.childNodes(),
                            baseUrl = baseUrl,
                        )
                    }

                    "sup" -> {
                        withStyle(SpanStyle(baselineShift = BaselineShift.Superscript)) {
                            appendTextChildren(
                                element.childNodes(),
                                baseUrl = baseUrl,
                            )
                        }
                    }

                    "sub" -> {
                        withStyle(SpanStyle(baselineShift = BaselineShift.Subscript)) {
                            appendTextChildren(
                                element.childNodes(),
                                baseUrl = baseUrl,
                            )
                        }
                    }

                    "font" -> {
                        appendTextChildren(
                            element.childNodes(),
                            baseUrl = baseUrl,
                        )
                    }

                    "pre" -> {
                        emitParagraph()
                        // TODO some TTS annotation?
                        appendTextChildren(
                            element.childNodes(),
                            preFormatted = true,
                            baseUrl = baseUrl,
                        )
                        emitParagraph()
                    }

                    "code" -> {
                        emitParagraph()
                        // TODO some TTS annotation?
                        appendTextChildren(
                            element.childNodes(),
                            preFormatted = preFormatted,
                            baseUrl = baseUrl,
                        )
                        emitParagraph()
                    }

                    "blockquote" -> {
                        emitParagraph()
                        withParagraph {
                            appendTextChildren(
                                element.childNodes(),
                                baseUrl = baseUrl,
                            )
                        }
                        emitParagraph()
                    }

                    "a" -> {
                        // abs:href will be blank for mailto: links
                        withAnnotation("URL", element.attr("abs:href").ifBlank { element.attr("href") }) {
                            appendTextChildren(
                                element.childNodes(),
                                baseUrl = baseUrl,
                            )
                        }
                    }

                    "figure" -> {
                        emitParagraph()
                    }

                    "img" -> {
                        emitParagraph()
                    }

                    "ul" -> {
                        element.children()
                            .filter { it.tagName() == "li" }
                            .forEach { listItem ->
                                withParagraph {
                                    // no break space
                                    append("• ")
                                    appendTextChildren(
                                        listItem.childNodes(),
                                        baseUrl = baseUrl,
                                    )
                                }
                            }
                    }

                    "ol" -> {
                        element.children()
                            .filter { it.tagName() == "li" }
                            .forEachIndexed { i, listItem ->
                                withParagraph {
                                    // no break space
                                    append("${i + 1}. ")
                                    appendTextChildren(
                                        listItem.childNodes(),
                                        baseUrl = baseUrl,
                                    )
                                }
                            }
                    }

                    "table" -> {
                        appendTable {
                            /*
                            In this order:
                            optionally a caption element (containing text children for instance),
                            followed by zero or more colgroup elements,
                            followed optionally by a thead element,
                            followed by either zero or more tbody elements
                            or one or more tr elements,
                            followed optionally by a tfoot element
                             */
                            element.children()
                                .filter { it.tagName() == "caption" }
                                .forEach {
                                    appendTextChildren(
                                        it.childNodes(),
                                        baseUrl = baseUrl,
                                    )
                                    emitParagraph()
                                }

                            element.children()
                            element.children()
                                .filter {
                                    it.tagName() in
                                        setOf(
                                            "thead",
                                            "tbody",
                                            "tfoot",
                                        )
                                }
                                .sortedBy {
                                    when (it.tagName()) {
                                        "thead" -> 0
                                        "tbody" -> 1
                                        "tfoot" -> 10
                                        else -> 2
                                    }
                                }
                                .flatMap {
                                    it.children()
                                        .filter { child -> child.tagName() == "tr" }
                                }
                                .forEach { row ->
                                    appendTextChildren(
                                        row.childNodes(),
                                        baseUrl = baseUrl,
                                    )
                                    emitParagraph()
                                }

                            // This is just for TTS so newlines are fine
                            append("\n\n")
                        }
                    }

                    "rt", "rp" -> {
                        // Ruby text elements. TTS has no need for furigana and similar
                        // so ignore
                    }

                    "iframe" -> {
                        // not implemented
                    }

                    "video" -> {
                        // not implemented
                    }

                    else -> {
                        appendTextChildren(
                            nodes = element.childNodes(),
                            preFormatted = preFormatted,
                            baseUrl = baseUrl,
                        )
                    }
                }
            }
        }

        node = node.nextSibling()
    }
}

package com.nononsenseapps.feeder.ui.compose.text

import android.content.res.Resources
import android.text.Annotation
import android.text.Spanned
import androidx.annotation.StringRes
import androidx.compose.runtime.Composable
import androidx.compose.runtime.ReadOnlyComposable
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.AnnotatedString
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.core.text.getSpans
import com.nononsenseapps.feeder.ui.compose.theme.LinkTextStyle

@Composable
@ReadOnlyComposable
fun resources(): Resources {
    LocalConfiguration.current
    return LocalContext.current.resources
}

@Composable
fun annotatedStringResource(
    @StringRes id: Int,
): AnnotatedString {
    val resources = resources()
    val text = resources.getText(id)

    return buildAnnotatedString {
        this.append(text.toString())

        if (text is Spanned) {
            for (annotation in text.getSpans<Annotation>()) {
                when (annotation.key) {
                    "style" -> {
                        getSpanStyle(annotation.value)?.let { spanStyle ->
                            addStyle(
                                spanStyle,
                                text.getSpanStart(annotation),
                                text.getSpanEnd(annotation),
                            )
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun getSpanStyle(name: String?): SpanStyle? {
    return when (name) {
        "link" -> LinkTextStyle().toSpanStyle()
        else -> null
    }
}

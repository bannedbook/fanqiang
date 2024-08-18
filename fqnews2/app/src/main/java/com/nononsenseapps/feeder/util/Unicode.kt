package com.nononsenseapps.feeder.util

fun String.asUTF8Sequence(): Sequence<String> =
    sequence {
        var i = 0
        while (i < length) {
            val code = codePointAt(i)
            i += Character.charCount(code)
            // Unicode smileys are an example of where toChar() won't work. Needs to be String.
            yield(String(intArrayOf(code), 0, 1))
        }
    }

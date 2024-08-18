package com.nononsenseapps.feeder.util

import com.nononsenseapps.feeder.model.ImageFromHTML
import org.jsoup.Jsoup
import org.jsoup.parser.Parser.unescapeEntities

fun findFirstImageInHtml(
    text: String?,
    baseUrl: String?,
): ImageFromHTML? =
    if (text != null) {
        val doc =
            unescapeEntities(text, true).byteInputStream().use {
                Jsoup.parse(it, "UTF-8", baseUrl ?: "")
            }

        doc.getElementsByTag("img").asSequence()
            .filterNot { it.attr("width") == "1" || it.attr("height") == "1" }
            .map {
                // abs: will resolve relative urls against the baseurl - and non-url value will get
                // dropped, such as invalid values and data/base64 values
                ImageFromHTML(
                    url = it.attr("abs:src"),
                    width = it.attr("width").toIntOrNull(),
                    height = it.attr("height").toIntOrNull(),
                )
            }
            .firstOrNull {
                it.url.isNotBlank() &&
                    !it.url.contains("twitter_icon", ignoreCase = true) &&
                    !it.url.contains("facebook_icon", ignoreCase = true)
            }
    } else {
        null
    }

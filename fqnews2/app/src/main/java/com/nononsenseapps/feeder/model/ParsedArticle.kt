package com.nononsenseapps.feeder.model

data class ParsedArticle(
    val id: String?,
    val url: String? = null,
    val external_url: String? = null,
    val title: String? = null,
    val content_html: String? = null,
    val content_text: String? = null,
    val summary: String? = null,
    val image: ThumbnailImage? = null,
    val date_published: String? = null,
    val date_modified: String? = null,
    val author: ParsedAuthor? = null,
    val tags: List<String>? = null,
    val attachments: List<ParsedEnclosure>? = null,
)

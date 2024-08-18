package com.nononsenseapps.feeder.model

data class ParsedFeed(
    val title: String?,
    val home_page_url: String? = null,
    val feed_url: String? = null,
    val description: String? = null,
    val user_comment: String? = null,
    val next_url: String? = null,
    val icon: String? = null,
    val favicon: String? = null,
    val author: ParsedAuthor? = null,
    val expired: Boolean? = null,
    val items: List<ParsedArticle>?,
)

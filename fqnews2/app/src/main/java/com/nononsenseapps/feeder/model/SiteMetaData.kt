package com.nononsenseapps.feeder.model

import androidx.compose.runtime.Immutable
import java.net.URL

@Immutable
data class SiteMetaData(
    val url: URL,
    val alternateFeedLinks: List<AlternateLink>,
    val feedImage: String?,
)

@Immutable
data class AlternateLink(
    val link: URL,
    val type: String,
)

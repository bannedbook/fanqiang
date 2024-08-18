package com.nononsenseapps.feeder.model.gofeed

import com.nononsenseapps.feeder.model.EnclosureImage
import com.nononsenseapps.feeder.model.MediaImage
import com.nononsenseapps.feeder.model.ThumbnailImage
import com.nononsenseapps.feeder.ui.text.HtmlToPlainTextConverter
import com.nononsenseapps.feeder.util.findFirstImageInHtml
import com.nononsenseapps.feeder.util.relativeLinkIntoAbsolute
import com.nononsenseapps.feeder.util.relativeLinkIntoAbsoluteOrNull
import okhttp3.internal.toLongOrDefault
import java.net.URL

class FeederGoItem(
    private val goItem: GoItem,
    private val feedAuthor: GoPerson?,
    private val feedBaseUrl: URL,
) {
    val title: String? by lazy {
        goItem.title?.let {
            HtmlToPlainTextConverter().convert(it)
        }
    }

    /**
     * Potentially with HTML
     */
    val content: String by lazy {
        sequence {
            if (goItem.content?.isNotBlank() == true) {
                yield(goItem.content)
            }

            if (goItem.description?.isNotBlank() == true) {
                yield(goItem.description)
            }

            goItem.extensions?.entries?.forEach { (_, value) ->
                value.entries.forEach { (_, value) ->
                    value.forEach { extension ->
                        recursiveExtensionMediaDescription(extension)
                    }
                }
            }
        }.firstOrNull() ?: ""
    }

    val plainContent: String by lazy {
        val result = content.let { HtmlToPlainTextConverter().convert(it) }

        // Description consists of at least one image, avoid opening browser for this item
        when {
            result.isBlank() && content.contains("img") -> "<image>"
            else -> result
        }
    }

    val snippet: String by lazy { plainContent.take(200) }

    val link: String? by lazy { goItem.link?.let { relativeLinkIntoAbsolute(feedBaseUrl, it) } }

    val updated: String?
        get() = goItem.updatedParsed

    val published: String?
        get() = goItem.publishedParsed

    val author: GoPerson?
        get() = goItem.author ?: feedAuthor

    val guid: String? by lazy {
        (goItem.guid ?: goItem.link)?.let { relativeLinkIntoAbsolute(feedBaseUrl, it) }
    }

    val categories: List<String>?
        get() = goItem.categories

    val enclosures: List<GoEnclosure>?
        get() = goItem.enclosures?.filterNotNull()

    val dublinCoreExt: GoDublinCoreExtension?
        get() = goItem.dublinCoreExt

    val iTunesExt: GoITunesItemExtension?
        get() = goItem.iTunesExt

    val extensions: GoExtensions?
        get() = goItem.extensions

    val thumbnail: ThumbnailImage? by lazy {
        val thumbnailCandidates =
            sequence {
                goItem.image?.let { goImage ->
                    goImage.url?.let { url ->
                        yield(
                            MediaImage(
                                url = relativeLinkIntoAbsolute(feedBaseUrl, url),
                            ),
                        )
                    }
                }

                goItem.extensions?.entries?.forEach { (_, value) ->
                    // Key is whatever name was assigned to the namespace in the XML
                    value.entries.forEach { (_, value) ->
                        value.forEach { extension ->
                            recursiveExtensionThumbnailCandidates(extension, feedBaseUrl)
                        }
                    }
                }

                enclosures?.forEach { enclosure ->
                    if (enclosure.type?.startsWith("image/") == true) {
                        enclosure.url?.let { url ->
                            yield(
                                EnclosureImage(
                                    url = relativeLinkIntoAbsolute(feedBaseUrl, url),
                                    length = enclosure.length?.toLongOrDefault(0L) ?: 0L,
                                ),
                            )
                        }
                    }
                }
            }

        val thumbnail = thumbnailCandidates.maxByOrNull { it.width ?: -1 }

        if (thumbnail is EnclosureImage && (thumbnail.length == 0L || thumbnail.length > 50_000L)) {
            // Enclosures don't have width/height, so guessing from length
            return@lazy thumbnail
        } else if (thumbnail != null && (thumbnail.width ?: 0) >= 640) {
            return@lazy thumbnail
        }

        // Now we are resolving against original, not the feed
        val baseUrl: String = linkToHtml(feedBaseUrl, link) ?: feedBaseUrl.toString()

        findFirstImageInHtml(this.content, baseUrl) ?: thumbnail
    }
}

suspend fun SequenceScope<ThumbnailImage>.recursiveExtensionThumbnailCandidates(
    extension: GoExtension,
    baseURL: URL,
) {
    extension.attrs?.get("url")?.let { url ->
        if (pointsToImage(url)) {
            yield(
                MediaImage(
                    url = relativeLinkIntoAbsolute(baseURL, url),
                    height = extension.attrs["height"]?.toIntOrNull(),
                    width = extension.attrs["width"]?.toIntOrNull(),
                ),
            )
        }
    }

    if (extension.name.equals("thumbnail", ignoreCase = true)) {
        extension.value?.let { value ->
            if (pointsToImage(value)) {
                yield(
                    MediaImage(
                        url = relativeLinkIntoAbsolute(baseURL, value),
                    ),
                )
            }
        }
    }

    extension.children?.entries?.forEach { (_, value) ->
        value.forEach { extension ->
            recursiveExtensionThumbnailCandidates(extension, baseURL)
        }
    }
}

suspend fun SequenceScope<String>.recursiveExtensionMediaDescription(extension: GoExtension) {
    if (extension.name.equals("description", ignoreCase = true)) {
        extension.value?.let { value ->
            yield(value)
        }
    }

    extension.children?.entries?.forEach { (_, value) ->
        value.forEach { extension ->
            recursiveExtensionMediaDescription(extension)
        }
    }
}

// fun GoItem.thumbnail(feedBaseUrl: URL): ThumbnailImage? {
//    val media = this.getModule(MediaModule.URI) as MediaEntryModule?

//    this.extensions
//
//    val thumbnailCandidates =
//        sequence {
//            media?.findThumbnailCandidates(feedBaseUrl)?.let {
//                yieldAll(it)
//            }
//            enclosures?.asSequence()
//                ?.mapNotNull { it.findThumbnailCandidate(feedBaseUrl) }
//                ?.let {
//                    yieldAll(it)
//                }
//        }
//
//    val thumbnail = thumbnailCandidates.maxByOrNull { it.width ?: -1 }
//
//    if (thumbnail is EnclosureImage && (thumbnail.length == 0L || thumbnail.length > 50_000L)) {
//        // Enclosures don't have width/height, so guessing from length
//        return thumbnail
//    } else if (thumbnail != null && (thumbnail.width ?: 0) >= 640) {
//        return thumbnail
//    }
//
//    // Either image is too small for liking, or no image found. Try to find one in the content first
//    val thumbnail = null
// //    // Now we are resolving against original, not the feed
//    val baseUrl: String = this.linkToHtml(feedBaseUrl) ?: feedBaseUrl.toString()
//
//    return findFirstImageInHtml(this.body(), baseUrl) ?: thumbnail
// }

private fun linkToHtml(
    feedBaseUrl: URL,
    link: String?,
): String? {
//    this.links?.firstOrNull { "alternate" == it.rel && "text/html" == it.type }?.let {
//        return relativeLinkIntoAbsoluteOrNull(feedBaseUrl, it.href)
//    }
//
//    this.links?.firstOrNull { "self" == it.rel && "text/html" == it.type }?.let {
//        return relativeLinkIntoAbsoluteOrNull(feedBaseUrl, it.href)
//    }
//
//    this.links?.firstOrNull { "self" == it.rel }?.let {
//        return relativeLinkIntoAbsoluteOrNull(feedBaseUrl, it.href)
//    }

    link?.let {
        return relativeLinkIntoAbsoluteOrNull(feedBaseUrl, it)
    }

    return null
}

//
// private fun SyndEnclosure.findThumbnailCandidate(feedBaseUrl: URL): ThumbnailImage? {
//    if (type?.startsWith("image/") == true) {
//        url?.let { url ->
//            return EnclosureImage(
//                url = relativeLinkIntoAbsolute(feedBaseUrl, url),
//                length = this.length,
//            )
//        }
//    }
//    return null
// }
//
// private fun MediaGroup.findThumbnailCandidates(feedBaseUrl: URL): Sequence<ThumbnailImage> =
//    sequence {
//        metadata.thumbnail?.forEach { thumbnail ->
//            thumbnail.findThumbnailCandidate(feedBaseUrl)?.let { thumbnailCandidate ->
//                yield(thumbnailCandidate)
//            }
//        }
//    }
//
// private fun Thumbnail.findThumbnailCandidate(feedBaseUrl: URL): ThumbnailImage? {
//    return url?.let { url ->
//        MediaImage(
//            width = width,
//            height = height,
//            url = relativeLinkIntoAbsolute(feedBaseUrl, url.toString()),
//        )
//    }
// }
//
// private fun MediaContent.findThumbnailCandidates(feedBaseUrl: URL): Sequence<ThumbnailImage> =
//    sequence {
//        metadata?.thumbnail?.forEach { thumbnail ->
//            thumbnail.findThumbnailCandidate(feedBaseUrl)?.let { thumbnailCandidate ->
//                yield(thumbnailCandidate)
//            }
//        }
//
//        if (isImage()) {
//            reference?.let { ref ->
//                yield(
//                    MediaImage(
//                        width = width,
//                        height = height,
//                        url = relativeLinkIntoAbsolute(feedBaseUrl, ref.toString()),
//                    ),
//                )
//            }
//        }
//    }
//
// private fun MediaContent.isImage(): Boolean {
//    return when {
//        medium == "image" -> true
//        pointsToImage(reference.toString()) -> true
//        else -> false
//    }
// }
//
private fun pointsToImage(url: String): Boolean {
    return try {
        val u = URL(url)

        u.path.endsWith(".jpg", ignoreCase = true) ||
            u.path.endsWith(".jpeg", ignoreCase = true) ||
            u.path.endsWith(".gif", ignoreCase = true) ||
            u.path.endsWith(".png", ignoreCase = true) ||
            u.path.endsWith(".webp", ignoreCase = true)
    } catch (_: Exception) {
        false
    }
}

fun makeGoItem(
    guid: String? = null,
    title: String? = null,
    content: String? = null,
    description: String? = null,
    image: GoImage? = null,
    link: String? = null,
    links: List<String>? = null,
    extensions: GoExtensions? = null,
    author: GoPerson? = null,
    authors: List<GoPerson?>? = null,
    categories: List<String>? = null,
    dublinCoreExt: GoDublinCoreExtension? = null,
    enclosures: List<GoEnclosure?>? = null,
    iTunesExt: GoITunesItemExtension? = null,
    published: String? = null,
    publishedParsed: String? = null,
    updated: String? = null,
    updatedParsed: String? = null,
) = GoItem(
    guid = guid,
    title = title,
    content = content,
    description = description,
    image = image,
    link = link,
    links = links,
    extensions = extensions,
    author = author,
    authors = authors,
    categories = categories,
    dublinCoreExt = dublinCoreExt,
    enclosures = enclosures,
    iTunesExt = iTunesExt,
    published = published,
    publishedParsed = publishedParsed,
    updated = updated,
    updatedParsed = updatedParsed,
)

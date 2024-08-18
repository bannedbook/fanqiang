package com.nononsenseapps.feeder.ui.text

// Example strings
// www.youtube.com/embed/cjxnVO9RpaQ
// www.youtube.com/embed/cjxnVO9RpaQ?feature=oembed
// www.youtube.com/embed/cjxnVO9RpaQ/theoretical_crap
// www.youtube.com/embed/cjxnVO9RpaQ/crap?feature=oembed
internal val YoutubeIdPattern = "youtube.com/embed/([^?/]*)".toRegex()

fun getVideo(src: String?): Video? {
    return src?.let {
        YoutubeIdPattern.find(src)?.let { match ->
            val videoId = match.groupValues[1]
            Video(
                src = src,
                imageUrl = "http://img.youtube.com/vi/$videoId/hqdefault.jpg",
                link = "https://www.youtube.com/watch?v=$videoId",
            )
        }
    }
}

data class Video(
    val src: String,
    val imageUrl: String,
    // Youtube needs a different link than embed links
    val link: String,
) {
    val width: Int
        get() = 480

    val height: Int
        get() = 360
}

package com.nononsenseapps.feeder.model

import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

/**
 * URL should be absolute at all times
 */
@Serializable
sealed class ThumbnailImage {
    abstract val url: String
    abstract val width: Int?
    abstract val height: Int?
    abstract val fromBody: Boolean
}

@Serializable
@SerialName("ImageFromHTML")
class ImageFromHTML(
    override val url: String,
    override val width: Int? = null,
    override val height: Int? = null,
) : ThumbnailImage() {
    override val fromBody: Boolean
        get() = true

    override fun equals(other: Any?): Boolean {
        return if (other is ImageFromHTML) {
            return url == other.url && width == other.width && height == other.height
        } else {
            false
        }
    }

    override fun hashCode(): Int {
        val prime = 31
        var result = 1
        result = prime * result + javaClass.simpleName.hashCode()
        result = prime * result + url.hashCode()
        result = prime * result + width.hashCode()
        result = prime * result + height.hashCode()
        return result
    }

    override fun toString(): String {
        return "ImageFromHTML(url='$url', width=$width, height=$height, fromBody=$fromBody)"
    }
}

@Serializable
@SerialName("EnclosureImage")
class EnclosureImage(
    override val url: String,
    /**
     * Number of bytes of images, zero if not known
     */
    val length: Long,
) : ThumbnailImage() {
    override val width: Int?
        get() = null
    override val height: Int?
        get() = null

    override val fromBody: Boolean
        get() = false

    override fun equals(other: Any?): Boolean {
        return if (other is EnclosureImage) {
            return url == other.url && width == other.width && height == other.height
        } else {
            false
        }
    }

    override fun hashCode(): Int {
        val prime = 31
        var result = 1
        result = prime * result + javaClass.simpleName.hashCode()
        result = prime * result + url.hashCode()
        result = prime * result + width.hashCode()
        result = prime * result + height.hashCode()
        return result
    }

    override fun toString(): String {
        return "EnclosureImage(url='$url', width=$width, height=$height, fromBody=$fromBody)"
    }
}

@Serializable
@SerialName("MediaImage")
class MediaImage(
    override val url: String,
    override val width: Int? = null,
    override val height: Int? = null,
) : ThumbnailImage() {
    override val fromBody: Boolean
        get() = false

    override fun equals(other: Any?): Boolean {
        return if (other is MediaImage) {
            return url == other.url && width == other.width && height == other.height
        } else {
            false
        }
    }

    override fun hashCode(): Int {
        val prime = 31
        var result = 1
        result = prime * result + javaClass.simpleName.hashCode()
        result = prime * result + url.hashCode()
        result = prime * result + width.hashCode()
        result = prime * result + height.hashCode()
        return result
    }

    override fun toString(): String {
        return "MediaImage(url='$url', width=$width, height=$height, fromBody=$fromBody)"
    }
}

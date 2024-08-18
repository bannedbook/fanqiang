package com.nononsenseapps.feeder.db.room

import androidx.room.TypeConverter
import com.nononsenseapps.feeder.model.ImageFromHTML
import com.nononsenseapps.feeder.model.ThumbnailImage
import com.nononsenseapps.feeder.util.sloppyLinkToStrictURLNoThrows
import kotlinx.serialization.encodeToString
import kotlinx.serialization.json.Json
import java.net.URL
import java.time.Instant
import java.time.ZonedDateTime

class Converters {
    @TypeConverter
    fun dateTimeFromString(value: String?): ZonedDateTime? {
        var dt: ZonedDateTime? = null
        if (value != null) {
            try {
                dt = ZonedDateTime.parse(value)
            } catch (_: Throwable) {
            }
        }
        return dt
    }

    @TypeConverter
    fun stringFromDateTime(value: ZonedDateTime?): String? = value?.toString()

    @TypeConverter
    fun stringFromURL(value: URL?): String? = value?.toString()

    @TypeConverter
    fun urlFromString(value: String?): URL? = value?.let { sloppyLinkToStrictURLNoThrows(it) }

    @TypeConverter
    fun instantFromLong(value: Long?): Instant? =
        try {
            value?.let { Instant.ofEpochMilli(it) }
        } catch (t: Throwable) {
            null
        }

    @TypeConverter
    fun longFromInstant(value: Instant?): Long? = value?.toEpochMilli()

    @TypeConverter
    fun stringFromThumbnailImage(value: ThumbnailImage?): String? =
        value?.let {
            Json.encodeToString(it)
        }

    @TypeConverter
    fun thumbnailImageFromString(value: String?): ThumbnailImage? =
        value?.let {
            try {
                Json.decodeFromString<ThumbnailImage>(it)
            } catch (_: Throwable) {
                // Legacy values may be stored as just a URL
                try {
                    val url = URL(it)
                    // But we don't know what type so we pick a conservative default
                    ImageFromHTML(url = url.toString(), width = null, height = null)
                } catch (_: Throwable) {
                    null
                }
            }
        }
}

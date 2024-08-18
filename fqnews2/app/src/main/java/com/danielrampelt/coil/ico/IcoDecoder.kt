package com.danielrampelt.coil.ico

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.os.Build.VERSION.SDK_INT
import android.util.Log
import androidx.core.graphics.drawable.toDrawable
import coil.ImageLoader
import coil.decode.DecodeResult
import coil.decode.Decoder
import coil.decode.ImageSource
import coil.fetch.SourceResult
import coil.request.Options
import jww.app.FeederApplication
import com.nononsenseapps.feeder.R
import okio.BufferedSource

private const val LOG_TAG = "FEEDER_ICO"

class IcoDecoder(
    context: Context,
    private val source: ImageSource,
    private val options: Options,
) : Decoder {
    private val resources = context.applicationContext.resources

    override suspend fun decode(): DecodeResult? {
        return try {
            if (!isIco(source)) {
                return null
            }

            return BitmapFactory.Options().decode(source.source())
        } catch (e: Exception) {
            Log.e(LOG_TAG, "Failed to decode ICO", e)
            null
        }
    }

    private fun BitmapFactory.Options.decode(bufferedSource: BufferedSource): DecodeResult {
        // Read the image's dimensions.
//        inJustDecodeBounds = true
//        val peek = bufferedSource.peek()
//        peek.skip(offset)
//        BitmapFactory.decodeStream(peek.inputStream(), null, this)
        inJustDecodeBounds = false

        // Always create immutable bitmaps as they have better performance.
        inMutable = false

        if (SDK_INT >= 26 && options.colorSpace != null) {
            inPreferredColorSpace = options.colorSpace
        }
        inPremultiplied = options.premultipliedAlpha

        // Decode the bitmap.
        val outBitmap: Bitmap? =
            bufferedSource.use {
                BitmapFactory.decodeStream(it.inputStream(), null, this)
            }

        if (outBitmap == null) {
            Log.w(
                LOG_TAG,
                "BitmapFactory returned a null bitmap. Often this means BitmapFactory could not " +
                    "decode the image data read from the input source (e.g. network, disk, or " +
                    "memory) as it's not encoded as a valid image format.",
            )
            return DecodeResult(
                resources.getDrawable(
                    R.drawable.blank_pixel,
                    resources.newTheme(),
                ),
                false,
            )
        }

        // Fix the incorrect density created by overloading inDensity/inTargetDensity.
        outBitmap.density = options.context.resources.displayMetrics.densityDpi

        return DecodeResult(
            drawable = outBitmap.toDrawable(resources),
            isSampled = inSampleSize > 1 || inScaled,
        )
    }

    private data class IconDirEntry(
        val width: Byte,
        val height: Byte,
        val numColors: Byte,
        val colorPlanes: Short,
        val bytesPerPixel: Short,
        val size: Int,
        val offset: Int,
    ) {
        companion object {
            fun parse(source: BufferedSource): IconDirEntry {
                val width = source.readByte()
                val height = source.readByte()
                val numColors = source.readByte()
                source.skip(1)
                val colorPlanes = source.readShortLe()
                val bpp = source.readShortLe()
                val size = source.readIntLe()
                val offset = source.readIntLe()
                return IconDirEntry(
                    width = width,
                    height = height,
                    numColors = numColors,
                    colorPlanes = colorPlanes,
                    bytesPerPixel = bpp,
                    size = size,
                    offset = offset,
                )
            }
        }

        val widthPixels: Int
            get() = width.toInt().takeUnless { it == 0 } ?: 256

        val heightPixels: Int
            get() = height.toInt().takeUnless { it == 0 } ?: 256
    }

    companion object {
        private const val ICO_HEADER_SIZE = 6
        private const val ICO_ENTRY_SIZE = 16
    }

    class Factory(private val application: FeederApplication) : Decoder.Factory {
        override fun create(
            result: SourceResult,
            options: Options,
            imageLoader: ImageLoader,
        ): Decoder? {
            if (!isIco(result.source)) return null
            return IcoDecoder(application, result.source, options)
        }

        override fun equals(other: Any?): Boolean {
            if (this === other) return true
            return other is Factory
        }

        override fun hashCode() = application.hashCode()
    }
}

private fun isIco(source: ImageSource): Boolean {
    val peek = source.source().peek()
    if (peek.readShortLe() != 0.toShort()) return false
    if (peek.readShortLe() != 1.toShort()) return false
    val numImages = peek.readShortLe()
    if (numImages <= 0 || numImages > 256) return false
    return true
}

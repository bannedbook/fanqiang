package com.nononsenseapps.feeder.util

import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Rect
import android.graphics.Typeface
import android.text.TextPaint
import kotlin.math.abs

private val colors =
    arrayOf(
        0xffe57373, 0xfff06292, 0xffba68c8, 0xff9575cd,
        0xff7986cb, 0xff64b5f6, 0xff4fc3f7, 0xff4dd0e1, 0xff4db6ac, 0xff81c784,
        0xffaed581, 0xffff8a65, 0xffd4e157, 0xffffd54f, 0xffffb74d, 0xffa1887f,
        0xff90a4ae,
    )

private const val NUM_OF_TILE_COLORS = 8

/**
 * text: The string where the first character will be used for icon. 'F' if text is blank.
 * key: Anything which denotes the stable ID of the item.
 * radius: Pixel size of icon. Default should be suitable in most cases.
 */
fun getLetterIcon(
    text: String,
    key: Any,
    radius: Int = 128,
): Bitmap {
    val paint = TextPaint()
    val bounds = Rect()
    val canvas = Canvas()
    val bitmap = Bitmap.createBitmap(radius, radius, Bitmap.Config.ARGB_8888)

    paint.isAntiAlias = true
    paint.color = pickColor(key).toInt()

    canvas.setBitmap(bitmap)
    canvas.drawCircle(radius / 2.0f, radius / 2.0f, radius / 2.0f, paint)

    paint.typeface = Typeface.create("sans-serif-light", Typeface.BOLD)
    paint.color = Color.WHITE
    paint.textAlign = Paint.Align.CENTER

    val firstChar = CharArray(1)
    firstChar[0] =
        if (text.isBlank()) {
            'F'
        } else {
            text[0].uppercaseChar()
        }

    val fontSize = radius * 0.8f
    paint.textSize = fontSize
    paint.getTextBounds(firstChar, 0, 1, bounds)
    canvas.drawText(
        firstChar,
        0,
        1,
        radius / 2f,
        radius / 2f + (bounds.bottom - bounds.top) / 2f,
        paint,
    )

    return bitmap
}

private fun pickColor(key: Any): Long {
    val color = abs(key.hashCode()) % NUM_OF_TILE_COLORS
    return colors[color]
}

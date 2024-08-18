package com.nononsenseapps.feeder.ui.compose.icons

import androidx.compose.material.icons.materialIcon
import androidx.compose.material.icons.materialPath
import androidx.compose.ui.graphics.vector.ImageVector

@Suppress("ObjectPropertyName", "ktlint:standard:property-naming")
private var _textToSpeech: ImageVector? = null

@Suppress("UnusedReceiverParameter")
val CustomFilledIcons.TextToSpeech: ImageVector
    get() {
        return _textToSpeech ?: materialIcon(name = "Filled.TextToSpeech") {
            materialPath {
                moveTo(4f, 22f)
                quadTo(3.175f, 22f, 2.588f, 21.413f)
                quadTo(2f, 20.825f, 2f, 20f)
                verticalLineTo(4f)
                quadTo(2f, 3.175f, 2.588f, 2.587f)
                quadTo(3.175f, 2f, 4f, 2f)
                horizontalLineTo(13f)
                lineTo(11f, 4f)
                horizontalLineTo(4f)
                quadTo(4f, 4f, 4f, 4f)
                quadTo(4f, 4f, 4f, 4f)
                verticalLineTo(20f)
                quadTo(4f, 20f, 4f, 20f)
                quadTo(4f, 20f, 4f, 20f)
                horizontalLineTo(15f)
                quadTo(15f, 20f, 15f, 20f)
                quadTo(15f, 20f, 15f, 20f)
                verticalLineTo(17f)
                horizontalLineTo(17f)
                verticalLineTo(20f)
                quadTo(17f, 20.825f, 16.413f, 21.413f)
                quadTo(15.825f, 22f, 15f, 22f)
                close()
            }
            materialPath {
                moveTo(6f, 18f)
                verticalLineTo(16f)
                horizontalLineTo(13f)
                verticalLineTo(18f)
                close()
            }
            materialPath {
                moveTo(6f, 15f)
                verticalLineTo(13f)
                horizontalLineTo(11f)
                verticalLineTo(15f)
                close()
            }
            materialPath {
                moveTo(15f, 15f)
                lineTo(11f, 11f)
                horizontalLineTo(8f)
                verticalLineTo(6f)
                horizontalLineTo(11f)
                lineTo(15f, 2f)
                close()
            }
            materialPath {
                moveTo(17f, 11.95f)
                verticalLineTo(5.05f)
                quadTo(17.9f, 5.575f, 18.45f, 6.475f)
                quadTo(19f, 7.375f, 19f, 8.5f)
                quadTo(19f, 9.625f, 18.45f, 10.525f)
                quadTo(17.9f, 11.425f, 17f, 11.95f)
                close()
            }
            materialPath {
                moveTo(17f, 16.25f)
                verticalLineTo(14.15f)
                quadTo(18.75f, 13.525f, 19.875f, 11.987f)
                quadTo(21f, 10.45f, 21f, 8.5f)
                quadTo(21f, 6.55f, 19.875f, 5.012f)
                quadTo(18.75f, 3.475f, 17f, 2.85f)
                verticalLineTo(0.75f)
                quadTo(19.6f, 1.425f, 21.3f, 3.562f)
                quadTo(23f, 5.7f, 23f, 8.5f)
                quadTo(23f, 11.3f, 21.3f, 13.438f)
                quadTo(19.6f, 15.575f, 17f, 16.25f)
                close()
            }
        }.also { _textToSpeech = it }
    }

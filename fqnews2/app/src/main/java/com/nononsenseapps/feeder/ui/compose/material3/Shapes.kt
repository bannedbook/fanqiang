package com.nononsenseapps.feeder.ui.compose.material3

import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.CornerBasedShape
import androidx.compose.foundation.shape.CornerSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Shapes
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.RectangleShape
import androidx.compose.ui.graphics.Shape
import androidx.compose.ui.unit.dp
import com.nononsenseapps.feeder.ui.compose.material3.tokens.ShapeKeyTokens

/** Helper function for component shape tokens. Used to grab the top values of a shape parameter. */
internal fun CornerBasedShape.top(): CornerBasedShape {
    return copy(bottomStart = CornerSize(0.0.dp), bottomEnd = CornerSize(0.0.dp))
}

/** Helper function for component shape tokens. Used to grab the end values of a shape parameter. */
internal fun CornerBasedShape.end(): CornerBasedShape {
    return copy(topStart = CornerSize(0.0.dp), bottomStart = CornerSize(0.0.dp))
}

/**
 * Helper function for component shape tokens. Here is an example on how to use component color
 * tokens:
 * ``MaterialTheme.shapes.fromToken(FabPrimarySmallTokens.ContainerShape)``
 */
internal fun Shapes.fromToken(value: ShapeKeyTokens): Shape {
    return when (value) {
        ShapeKeyTokens.CornerExtraLarge -> extraLarge
        ShapeKeyTokens.CornerExtraLargeTop -> extraLarge.top()
        ShapeKeyTokens.CornerExtraSmall -> extraSmall
        ShapeKeyTokens.CornerExtraSmallTop -> extraSmall.top()
        ShapeKeyTokens.CornerFull -> CircleShape
        ShapeKeyTokens.CornerLarge -> large
        ShapeKeyTokens.CornerLargeEnd -> large.end()
        ShapeKeyTokens.CornerLargeTop -> large.top()
        ShapeKeyTokens.CornerMedium -> medium
        ShapeKeyTokens.CornerNone -> RectangleShape
        ShapeKeyTokens.CornerSmall -> small
    }
}

/** Converts a shape token key to the local shape provided by the theme */
@Composable
internal fun ShapeKeyTokens.toShape(): Shape {
    return MaterialTheme.shapes.fromToken(this)
}

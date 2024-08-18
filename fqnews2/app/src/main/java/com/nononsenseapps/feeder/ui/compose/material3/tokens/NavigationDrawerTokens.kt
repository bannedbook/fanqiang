@file:Suppress("ktlint:standard:property-naming")

package com.nononsenseapps.feeder.ui.compose.material3.tokens

import androidx.compose.runtime.Composable
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.platform.LocalLayoutDirection
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.LayoutDirection
import androidx.compose.ui.unit.coerceAtMost
import androidx.compose.ui.unit.dp
import com.nononsenseapps.feeder.ui.compose.theme.LocalDimens
import com.nononsenseapps.feeder.ui.compose.utils.LocalFoldableHinge

internal object NavigationDrawerTokens {
    val ActiveFocusIconColor = ColorSchemeKeyTokens.OnSecondaryContainer
    val ActiveFocusLabelTextColor = ColorSchemeKeyTokens.OnSecondaryContainer
    val ActiveHoverIconColor = ColorSchemeKeyTokens.OnSecondaryContainer
    val ActiveHoverLabelTextColor = ColorSchemeKeyTokens.OnSecondaryContainer
    val ActiveIconColor = ColorSchemeKeyTokens.OnSecondaryContainer
    val ActiveIndicatorColor = ColorSchemeKeyTokens.SecondaryContainer
    val ActiveIndicatorHeight = 56.0.dp
    val ActiveIndicatorShape = ShapeKeyTokens.CornerFull
    val ActiveIndicatorWidth = 336.0.dp
    val ActiveLabelTextColor = ColorSchemeKeyTokens.OnSecondaryContainer
    val ActivePressedIconColor = ColorSchemeKeyTokens.OnSecondaryContainer
    val ActivePressedLabelTextColor = ColorSchemeKeyTokens.OnSecondaryContainer
    val BottomContainerShape = ShapeKeyTokens.CornerLargeTop
    val ContainerColor = ColorSchemeKeyTokens.Surface
    const val ContainerHeightPercent = 100.0f
    val ContainerShape = ShapeKeyTokens.CornerLargeEnd
    val ContainerSurfaceTintLayerColor = ColorSchemeKeyTokens.SurfaceTint
    private val ContainerWidth = 360.0.dp
    val HeadlineColor = ColorSchemeKeyTokens.OnSurfaceVariant
    val HeadlineFont = TypographyKeyTokens.TitleSmall
    val IconSize = 24.0.dp
    val InactiveFocusIconColor = ColorSchemeKeyTokens.OnSurface
    val InactiveFocusLabelTextColor = ColorSchemeKeyTokens.OnSurface
    val InactiveHoverIconColor = ColorSchemeKeyTokens.OnSurface
    val InactiveHoverLabelTextColor = ColorSchemeKeyTokens.OnSurface
    val InactiveIconColor = ColorSchemeKeyTokens.OnSurfaceVariant
    val InactiveLabelTextColor = ColorSchemeKeyTokens.OnSurfaceVariant
    val InactivePressedIconColor = ColorSchemeKeyTokens.OnSurface
    val InactivePressedLabelTextColor = ColorSchemeKeyTokens.OnSurface
    val LabelTextFont = TypographyKeyTokens.LabelLarge
    val LargeBadgeLabelColor = ColorSchemeKeyTokens.OnSurfaceVariant
    val LargeBadgeLabelFont = TypographyKeyTokens.LabelLarge
    val ModalContainerElevation = ElevationTokens.Level1
    val StandardContainerElevation = ElevationTokens.Level0

    @Composable
    fun getContainerWidth(): Dp {
        val screenWidth = LocalConfiguration.current.screenWidthDp.dp
        val foldableHinge = LocalFoldableHinge.current

        // TODO remember here?
        return if (foldableHinge != null && foldableHinge.isTopToBottom) {
            val dimens = LocalDimens.current
            val layoutDirection = LocalLayoutDirection.current

            with(LocalDensity.current) {
                when (layoutDirection) {
                    LayoutDirection.Ltr -> foldableHinge.bounds.left.toDp()
                    LayoutDirection.Rtl -> screenWidth - foldableHinge.bounds.right.toDp()
                } - dimens.gutter / 2
            }
        } else {
            ContainerWidth
        }.coerceAtMost(screenWidth)
    }
}

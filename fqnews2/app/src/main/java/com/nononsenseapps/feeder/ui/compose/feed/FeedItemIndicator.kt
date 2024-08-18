package com.nononsenseapps.feeder.ui.compose.feed

import android.util.Log
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Star
import androidx.compose.material.icons.outlined.Circle
import androidx.compose.material.icons.outlined.ErrorOutline
import androidx.compose.material.icons.outlined.Terrain
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import coil.compose.AsyncImage
import coil.request.ImageRequest
import coil.size.Precision
import coil.size.Scale
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.archmodel.ThemeOptions
import com.nononsenseapps.feeder.ui.compose.coil.rememberTintedVectorPainter
import com.nononsenseapps.feeder.ui.compose.theme.FeederTheme

@Composable
fun FeedItemEitherIndicator(
    bookmarked: Boolean,
    itemImage: String?,
    feedImageUrl: String?,
    modifier: Modifier = Modifier,
    size: Dp = 8.dp,
) {
    when {
        bookmarked -> FeedItemSavedIndicator(size = size, modifier = modifier)
//        unread -> FeedItemNewIndicator(size = size, modifier = modifier)
        itemImage != null ->
            FeedItemImageIndicator(
                imageUrl = itemImage,
                size = size,
                modifier = modifier,
            )

        feedImageUrl != null ->
            FeedItemFeedIconIndicator(
                feedImageUrl = feedImageUrl,
                size = size,
                modifier = modifier,
            )

        else ->
            Box(modifier = modifier) {
                Spacer(modifier = Modifier.size(size))
            }
    }
}

@Composable
fun FeedItemImageIndicator(
    imageUrl: String,
    size: Dp,
    modifier: Modifier = Modifier,
) {
    val pixels =
        with(LocalDensity.current) {
            size.roundToPx()
        }
    Box(
        contentAlignment = Alignment.Center,
        modifier = modifier,
    ) {
        AsyncImage(
            model =
                ImageRequest.Builder(LocalContext.current)
                    .data(imageUrl)
                    .listener(
                        onError = { a, b ->
                            Log.e("FEEDER_INDICATOR", "error ${a.data}", b.throwable)
                        },
                    )
                    .scale(Scale.FIT)
                    .size(pixels)
                    .precision(Precision.INEXACT)
                    .build(),
            placeholder = rememberTintedVectorPainter(Icons.Outlined.Terrain),
            error = rememberTintedVectorPainter(Icons.Outlined.ErrorOutline),
            // Don't list this in screen reader
            contentDescription = null,
            contentScale = ContentScale.Fit,
            modifier =
                Modifier
                    .clip(MaterialTheme.shapes.small)
                    .size(size),
        )
    }
}

@Composable
fun FeedItemFeedIconIndicator(
    feedImageUrl: String,
    size: Dp,
    modifier: Modifier = Modifier,
) {
    val pixels =
        with(LocalDensity.current) {
            size.roundToPx()
        }
    Box(
        contentAlignment = Alignment.Center,
        modifier = modifier,
    ) {
        AsyncImage(
            model =
                ImageRequest.Builder(LocalContext.current)
                    .data(feedImageUrl)
                    .listener(
                        onError = { a, b ->
                            Log.e("FEEDER_INDICATOR", "error ${a.data}", b.throwable)
                        },
                    )
                    .scale(Scale.FIT)
                    .size(pixels)
                    .precision(Precision.INEXACT)
                    .build(),
            placeholder = rememberTintedVectorPainter(Icons.Outlined.Terrain),
            error = rememberTintedVectorPainter(Icons.Outlined.ErrorOutline),
            // Don't list this in screen reader
            contentDescription = null,
            contentScale = ContentScale.Fit,
            modifier =
                Modifier
                    .size(size),
        )
    }
}

@Composable
fun FeedItemNewIndicator(
    size: Dp,
    modifier: Modifier = Modifier,
) {
    Box(modifier = modifier) {
        Icon(
            Icons.Outlined.Circle,
            contentDescription = stringResource(id = R.string.unread_adjective),
            modifier =
                Modifier
                    .size(size),
            tint = MaterialTheme.colorScheme.primary,
        )
    }
}

@Composable
fun FeedItemSavedIndicator(
    size: Dp,
    modifier: Modifier = Modifier,
) {
    Box(
        contentAlignment = Alignment.Center,
        modifier = modifier,
    ) {
        Icon(
            Icons.Default.Star,
            contentDescription = stringResource(id = R.string.saved_article),
            modifier =
                Modifier
                    .size(size),
            tint = MaterialTheme.colorScheme.primary,
        )
    }
}

@Preview("Light")
@Composable
private fun PreviewLightFeedItemIndicatorRow() {
    FeederTheme(currentTheme = ThemeOptions.DAY) {
        Surface {
            Box(
                contentAlignment = Alignment.Center,
                modifier =
                    Modifier
                        .padding(32.dp),
            ) {
                Row {
                    FeedItemNewIndicator(size = 8.dp)
                    FeedItemSavedIndicator(size = 8.dp)
                }
            }
        }
    }
}

@Preview("Dark")
@Composable
private fun PreviewDarkFeedItemIndicatorRow() {
    FeederTheme(currentTheme = ThemeOptions.NIGHT) {
        Surface {
            Box(
                contentAlignment = Alignment.Center,
                modifier =
                    Modifier
                        .padding(32.dp),
            ) {
                Row {
                    FeedItemNewIndicator(size = 8.dp)
                    FeedItemSavedIndicator(size = 8.dp)
                }
            }
        }
    }
}

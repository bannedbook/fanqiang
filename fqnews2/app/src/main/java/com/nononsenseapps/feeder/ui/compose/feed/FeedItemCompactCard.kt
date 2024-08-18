package com.nononsenseapps.feeder.ui.compose.feed

import android.util.Log
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.requiredHeightIn
import androidx.compose.foundation.layout.width
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.ErrorOutline
import androidx.compose.material.icons.outlined.Terrain
import androidx.compose.material3.ElevatedCard
import androidx.compose.material3.LocalContentColor
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.Immutable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import coil.compose.AsyncImage
import coil.request.ImageRequest
import coil.size.Precision
import coil.size.Scale
import coil.size.Size
import coil.size.pxOrElse
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.db.room.ID_UNSET
import com.nononsenseapps.feeder.model.EnclosureImage
import com.nononsenseapps.feeder.ui.compose.coil.rememberTintedVectorPainter
import com.nononsenseapps.feeder.ui.compose.minimumTouchSize
import com.nononsenseapps.feeder.ui.compose.theme.FeederTheme
import com.nononsenseapps.feeder.ui.compose.utils.PreviewThemes
import okhttp3.HttpUrl.Companion.toHttpUrlOrNull
import java.net.URL
import java.time.Instant

sealed interface FeedItemEvent {
    data object MarkAboveAsRead : FeedItemEvent

    data object MarkBelowAsRead : FeedItemEvent

    data object ShareItem : FeedItemEvent

    data object ToggleBookmarked : FeedItemEvent

    data object DismissDropdown : FeedItemEvent
}

@Immutable
data class FeedItemState(
    val item: FeedListItem,
    val showThumbnail: Boolean = true,
    val bookmarkIndicator: Boolean = true,
    val dropDownMenuExpanded: Boolean = false,
    val showReadingTime: Boolean = false,
    val maxLines: Int = 2,
)

private val iconSize = 24.dp
private val gradientColors = listOf(Color.Black.copy(alpha = 0.8f), Color.Black.copy(alpha = 0.38f), Color.Black.copy(alpha = 0.8f))

@Composable
fun FeedItemCompactCard(
    state: FeedItemState,
    modifier: Modifier = Modifier,
    onEvent: (FeedItemEvent) -> Unit = { },
) {
    ElevatedCard(
        modifier = modifier,
        shape = MaterialTheme.shapes.medium,
    ) {
        BoxWithConstraints(
            modifier =
                Modifier
                    .requiredHeightIn(min = minimumTouchSize)
                    .fillMaxWidth(),
        ) {
            if (state.showThumbnail) {
                val sizePx =
                    with(LocalDensity.current) {
                        val width = maxWidth.roundToPx()
                        Size(width, (width * 9) / 16)
                    }
                val gradient =
                    Brush.verticalGradient(
                        colors = gradientColors,
                        startY = 0f,
                        endY = sizePx.height.pxOrElse { 0 }.toFloat(),
                    )
                val imageUrl = state.item.image?.url
                if (imageUrl != null) {
                    FeedItemThumbnail(
                        imageUrl = imageUrl,
                        sizePx = sizePx,
                    )
                }
                Box(
                    modifier =
                        Modifier
                            .matchParentSize()
                            .background(gradient),
                )
            }

            Row(
                modifier =
                    Modifier
                        .height(iconSize)
                        .padding(top = 8.dp, end = 8.dp, start = 8.dp)
                        .align(Alignment.TopEnd),
            ) {
                if (state.item.bookmarked && state.bookmarkIndicator) {
                    FeedItemSavedIndicator(size = iconSize, modifier = Modifier)
                }
                if (state.item.unread) {
                    FeedItemNewIndicator(size = iconSize, modifier = Modifier)
                }
                state.item.feedImageUrl?.toHttpUrlOrNull()?.also {
                    FeedItemFeedIconIndicator(
                        feedImageUrl = it.toString(),
                        size = iconSize,
                        modifier = Modifier,
                    )
                }
            }

            Box(
                modifier =
                    Modifier
                        .width(maxWidth)
                        .padding(top = 48.dp)
                        .align(Alignment.BottomCenter),
            ) {
                FeedItemTitle(
                    state = state,
                    onEvent = onEvent,
                )
            }
        }
    }
}

@Composable
private fun FeedItemTitle(
    state: FeedItemState,
    onEvent: (FeedItemEvent) -> Unit,
) {
    Row(
        verticalAlignment = Alignment.CenterVertically,
        modifier =
            Modifier
                .padding(vertical = 8.dp, horizontal = 8.dp),
    ) {
        val textColor =
            when {
                !state.showThumbnail -> LocalContentColor.current
                state.item.unread -> Color.White
                else -> Color.White.copy(alpha = 0.74f)
            }
        CompositionLocalProvider(LocalContentColor provides textColor) {
            FeedItemText(
                item = state.item,
                onMarkAboveAsRead = { onEvent(FeedItemEvent.MarkAboveAsRead) },
                onMarkBelowAsRead = { onEvent(FeedItemEvent.MarkBelowAsRead) },
                onShareItem = { onEvent(FeedItemEvent.ShareItem) },
                onToggleBookmark = { onEvent(FeedItemEvent.ToggleBookmarked) },
                dropDownMenuExpanded = state.dropDownMenuExpanded,
                onDismissDropdown = { onEvent(FeedItemEvent.DismissDropdown) },
                maxLines = state.maxLines,
                showOnlyTitle = true,
                showReadingTime = state.showReadingTime,
            )
        }
    }
}

@Composable
private fun FeedItemThumbnail(
    imageUrl: String?,
    sizePx: Size,
) {
    if (imageUrl != null) {
        AsyncImage(
            model =
                ImageRequest.Builder(LocalContext.current)
                    .data(imageUrl)
                    .listener(
                        onError = { a, b ->
                            Log.e("FEEDER_CARD", "error ${a.data}", b.throwable)
                        },
                    )
                    .scale(Scale.FILL)
                    .size(sizePx)
                    .precision(Precision.INEXACT)
                    .build(),
            placeholder = rememberTintedVectorPainter(Icons.Outlined.Terrain),
            error = rememberTintedVectorPainter(Icons.Outlined.ErrorOutline),
            contentDescription = stringResource(id = R.string.article_image),
            contentScale = ContentScale.Crop,
            alignment = Alignment.Center,
            modifier =
                Modifier
                    .clip(MaterialTheme.shapes.medium)
                    .fillMaxWidth()
                    .aspectRatio(16.0f / 9.0f)
                    .alpha(0.74f),
        )
    }
}

@Composable
@PreviewThemes
private fun Preview() {
    FeederTheme {
        FeedItemCompactCard(
            state =
                FeedItemState(
                    item =
                        FeedListItem(
                            title = "title",
                            snippet =
                                "snippet which is quite long as you might expect from a snipper of a story." +
                                    " It keeps going and going and going and going and going and going and going" +
                                    " and going and going and going and going and going and going and going" +
                                    " and going and going and going and going and going and going and going" +
                                    " and going and going and going and going and going and going and going" +
                                    " and going and going and going and going and going and going and snowing",
                            feedTitle = "Super Duper Feed One two three hup di too dasf dsaf asd fsa dfasdf",
                            pubDate = "Jun 9, 2021",
                            unread = true,
                            image = null,
                            link = null,
                            id = ID_UNSET,
                            bookmarked = true,
                            feedImageUrl = null,
                            primarySortTime = Instant.EPOCH,
                            rawPubDate = null,
                            wordCount = 0,
                        ),
                ),
        )
    }
}

@Composable
@PreviewThemes
private fun PreviewWithImageUnread() {
    FeederTheme {
        Box(
            modifier = Modifier.width((300 - 2 * 16).dp),
        ) {
            FeedItemCompactCard(
                state =
                    FeedItemState(
                        item =
                            FeedListItem(
                                title = "title can be one line",
                                snippet =
                                    "snippet which is quite long as you might expect from a" +
                                        " snipper of a story. It keeps going and going and going" +
                                        " and going and going and going and going and going and" +
                                        " going and going and going and going and going and going" +
                                        " and going and going and going and going and going and" +
                                        " going and going and going and going and going and going" +
                                        " and going and going and going and going and going and" +
                                        " going and going and going and going and snowing",
                                feedTitle = "Super Feed",
                                pubDate = "Jun 9, 2021",
                                unread = true,
                                image = EnclosureImage(url = "blabal", length = 0),
                                link = null,
                                id = ID_UNSET,
                                bookmarked = false,
                                feedImageUrl = URL("https://foo/bar.png"),
                                primarySortTime = Instant.EPOCH,
                                rawPubDate = null,
                                wordCount = 0,
                            ),
                    ),
            )
        }
    }
}

@Composable
@PreviewThemes
private fun PreviewWithImageRead() {
    FeederTheme {
        Box(
            modifier = Modifier.width((300 - 2 * 16).dp),
        ) {
            FeedItemCompactCard(
                state =
                    FeedItemState(
                        item =
                            FeedListItem(
                                title = "title can be one line",
                                snippet =
                                    "snippet which is quite long as you might expect from a" +
                                        " snipper of a story. It keeps going and going and going" +
                                        " and going and going and going and going and going and" +
                                        " going and going and going and going and going and going" +
                                        " and going and going and going and going and going and" +
                                        " going and going and going and going and going and going" +
                                        " and going and going and going and going and going and" +
                                        " going and going and going and going and snowing",
                                feedTitle = "Super Duper Feed",
                                pubDate = "Jun 9, 2021",
                                unread = false,
                                image = EnclosureImage(url = "blabal", length = 0),
                                link = null,
                                id = ID_UNSET,
                                bookmarked = true,
                                feedImageUrl = null,
                                primarySortTime = Instant.EPOCH,
                                rawPubDate = null,
                                wordCount = 0,
                            ),
                    ),
            )
        }
    }
}

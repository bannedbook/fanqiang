package com.nononsenseapps.feeder.ui.compose.feed

import android.util.Log
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.IntrinsicSize
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.requiredHeightIn
import androidx.compose.foundation.layout.width
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.ErrorOutline
import androidx.compose.material.icons.outlined.Terrain
import androidx.compose.material3.Surface
import androidx.compose.runtime.Composable
import androidx.compose.runtime.Immutable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import coil.compose.AsyncImage
import coil.request.ImageRequest
import coil.size.Precision
import coil.size.Scale
import coil.size.Size
import com.nononsenseapps.feeder.archmodel.FeedItemStyle
import com.nononsenseapps.feeder.db.room.FeedItemCursor
import com.nononsenseapps.feeder.db.room.ID_UNSET
import com.nononsenseapps.feeder.model.MediaImage
import com.nononsenseapps.feeder.model.ThumbnailImage
import com.nononsenseapps.feeder.ui.compose.coil.RestrainedCropScaling
import com.nononsenseapps.feeder.ui.compose.coil.RestrainedFitScaling
import com.nononsenseapps.feeder.ui.compose.coil.rememberTintedVectorPainter
import com.nononsenseapps.feeder.ui.compose.minimumTouchSize
import com.nononsenseapps.feeder.ui.compose.theme.FeederTheme
import com.nononsenseapps.feeder.ui.compose.theme.LocalDimens
import java.net.URL
import java.time.Instant
import java.time.ZonedDateTime
import kotlin.math.roundToInt

@Composable
fun FeedItemCompact(
    item: FeedListItem,
    showThumbnail: Boolean,
    onMarkAboveAsRead: () -> Unit,
    onMarkBelowAsRead: () -> Unit,
    onShareItem: () -> Unit,
    onToggleBookmark: () -> Unit,
    dropDownMenuExpanded: Boolean,
    onDismissDropdown: () -> Unit,
    bookmarkIndicator: Boolean,
    maxLines: Int,
    showOnlyTitle: Boolean,
    showReadingTime: Boolean,
    modifier: Modifier = Modifier,
    imageWidth: Dp = 64.dp,
) {
    Row(
        horizontalArrangement = Arrangement.spacedBy(4.dp),
        modifier =
            modifier
                .height(IntrinsicSize.Min)
                .padding(start = LocalDimens.current.margin),
    ) {
        FeedItemText(
            item = item,
            onMarkAboveAsRead = onMarkAboveAsRead,
            onMarkBelowAsRead = onMarkBelowAsRead,
            onShareItem = onShareItem,
            onToggleBookmark = onToggleBookmark,
            dropDownMenuExpanded = dropDownMenuExpanded,
            onDismissDropdown = onDismissDropdown,
            maxLines = maxLines,
            showOnlyTitle = showOnlyTitle,
            showReadingTime = showReadingTime,
            modifier =
                Modifier
                    .requiredHeightIn(min = minimumTouchSize)
                    .padding(vertical = 8.dp),
        )

        if ((item.bookmarked && bookmarkIndicator) || showThumbnail && (item.image != null || item.feedImageUrl != null)) {
            Box(
                modifier = Modifier.fillMaxHeight(),
                contentAlignment = Alignment.TopEnd,
            ) {
                if (item.bookmarked && bookmarkIndicator) {
                    FeedItemEitherIndicator(
                        bookmarked = true,
                        itemImage = null,
                        feedImageUrl = null,
                        size = 24.dp,
                        modifier =
                            Modifier
                                .fillMaxHeight()
                                .width(64.dp),
                    )
                } else {
                    (item.image?.url ?: item.feedImageUrl?.toString())?.let { imageUrl ->
                        val pixelDensity = LocalDensity.current.density
                        val scale =
                            if (item.image != null) {
                                RestrainedCropScaling(pixelDensity)
                            } else {
                                RestrainedFitScaling(pixelDensity)
                            }
                        val pixels =
                            with(LocalDensity.current) {
                                val px = imageWidth.toPx()
                                Size(px.roundToInt(), (px * 1.5).roundToInt())
                            }
                        AsyncImage(
                            model =
                                ImageRequest.Builder(LocalContext.current)
                                    .data(imageUrl)
                                    .listener(
                                        onError = { a, b ->
                                            Log.e("FEEDER_COMPACT", "error ${a.data}", b.throwable)
                                        },
                                    )
                                    .scale(Scale.FILL)
                                    .size(pixels)
                                    .precision(Precision.INEXACT)
                                    .build(),
                            placeholder = rememberTintedVectorPainter(Icons.Outlined.Terrain),
                            error = rememberTintedVectorPainter(Icons.Outlined.ErrorOutline),
                            contentDescription = null,
                            contentScale = scale,
                            modifier =
                                Modifier
                                    .width(imageWidth)
                                    .fillMaxHeight(),
                        )
                    }
                }
            }
        } else {
            // Taking Row spacing into account
            Spacer(modifier = Modifier.width(LocalDimens.current.margin - 4.dp))
        }
    }
}

@Immutable
data class FeedListItem(
    val id: Long,
    val title: String,
    val snippet: String,
    val feedTitle: String,
    val unread: Boolean,
    val pubDate: String,
    val image: ThumbnailImage?,
    val link: String?,
    val bookmarked: Boolean,
    val feedImageUrl: URL?,
    val primarySortTime: Instant,
    val rawPubDate: ZonedDateTime?,
    val wordCount: Int,
) {
    val cursor: FeedItemCursor
        get() =
            object : FeedItemCursor {
                override val primarySortTime: Instant = this@FeedListItem.primarySortTime
                override val pubDate: ZonedDateTime? = this@FeedListItem.rawPubDate
                override val id: Long = this@FeedListItem.id
            }

    /**
     * Used so lazylist/grid can re-use items.
     *
     * Type will depend on having images as that will influence visible items
     */
    fun contentType(feedItemStyle: FeedItemStyle): String =
        when {
            image != null -> "$feedItemStyle/image"
            else -> "$feedItemStyle/other"
        }
}

@Composable
@Preview(showBackground = true)
private fun PreviewRead() {
    FeederTheme {
        Surface {
            FeedItemCompact(
                item =
                    @Suppress("ktlint:standard:max-line-length")
                    FeedListItem(
                        title = "title",
                        snippet = "snippet which is quite long as you might expect from a snipper of a story. It keeps going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and snowing",
                        feedTitle = "Super Duper Feed One two three hup di too dasf",
                        pubDate = "Jun 9, 2021",
                        unread = false,
                        image = null,
                        link = null,
                        id = ID_UNSET,
                        bookmarked = false,
                        feedImageUrl = null,
                        primarySortTime = Instant.EPOCH,
                        rawPubDate = null,
                        wordCount = 900,
                    ),
                showThumbnail = true,
                onMarkAboveAsRead = {},
                onMarkBelowAsRead = {},
                onShareItem = {},
                onToggleBookmark = {},
                dropDownMenuExpanded = false,
                onDismissDropdown = {},
                bookmarkIndicator = true,
                maxLines = 5,
                showOnlyTitle = false,
                showReadingTime = true,
                imageWidth = 64.dp,
            )
        }
    }
}

@Composable
@Preview(showBackground = true)
private fun PreviewUnread() {
    FeederTheme {
        Surface {
            FeedItemCompact(
                item =
                    @Suppress("ktlint:standard:max-line-length")
                    FeedListItem(
                        title = "title",
                        snippet = "snippet which is quite long as you might expect from a snipper of a story. It keeps going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and snowing",
                        feedTitle = "Super Duper Feed One two three hup di too dasf",
                        pubDate = "Jun 9, 2021",
                        unread = true,
                        image = null,
                        link = null,
                        id = ID_UNSET,
                        bookmarked = true,
                        feedImageUrl = null,
                        primarySortTime = Instant.EPOCH,
                        rawPubDate = null,
                        wordCount = 900,
                    ),
                showThumbnail = true,
                onMarkAboveAsRead = {},
                onMarkBelowAsRead = {},
                onShareItem = {},
                onToggleBookmark = {},
                dropDownMenuExpanded = false,
                onDismissDropdown = {},
                bookmarkIndicator = true,
                maxLines = 5,
                showOnlyTitle = false,
                showReadingTime = true,
                imageWidth = 64.dp,
            )
        }
    }
}

@Composable
@Preview(showBackground = true)
private fun PreviewWithImage() {
    FeederTheme {
        Surface {
            Box(
                modifier = Modifier.width(400.dp),
            ) {
                FeedItemCompact(
                    item =
                        @Suppress("ktlint:standard:max-line-length")
                        FeedListItem(
                            title = "title",
                            snippet = "snippet which is quite long as you might expect from a snipper of a story. It keeps going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and going and snowing",
                            feedTitle = "Super Duper Feed One two three hup di too dasf",
                            pubDate = "Jun 9, 2021",
                            unread = true,
                            image = MediaImage("blabla"),
                            link = null,
                            id = ID_UNSET,
                            bookmarked = false,
                            feedImageUrl = null,
                            primarySortTime = Instant.EPOCH,
                            rawPubDate = null,
                            wordCount = 900,
                        ),
                    showThumbnail = true,
                    onMarkAboveAsRead = {},
                    onMarkBelowAsRead = {},
                    onShareItem = {},
                    onToggleBookmark = {},
                    dropDownMenuExpanded = false,
                    onDismissDropdown = {},
                    bookmarkIndicator = true,
                    maxLines = 5,
                    showOnlyTitle = false,
                    showReadingTime = true,
                    imageWidth = 64.dp,
                )
            }
        }
    }
}

package com.nononsenseapps.feeder.ui.compose.feed

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.requiredHeightIn
import androidx.compose.material3.Surface
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.nononsenseapps.feeder.db.room.ID_UNSET
import com.nononsenseapps.feeder.model.MediaImage
import com.nononsenseapps.feeder.ui.compose.minimumTouchSize
import com.nononsenseapps.feeder.ui.compose.theme.FeederTheme
import com.nononsenseapps.feeder.ui.compose.theme.LocalDimens
import java.net.URL
import java.time.Instant

@Composable
fun FeedItemSuperCompact(
    item: FeedListItem,
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
) {
    Row(
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(8.dp),
        modifier =
            modifier
                .requiredHeightIn(min = minimumTouchSize)
                .padding(vertical = 8.dp, horizontal = LocalDimens.current.margin),
    ) {
        FeedItemEitherIndicator(
            bookmarked = item.bookmarked && bookmarkIndicator,
            itemImage = null,
            feedImageUrl = item.feedImageUrl?.toString(),
            size = 24.dp,
        )
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
        )
    }
}

@Composable
@Preview(showBackground = true)
private fun PreviewRead() {
    FeederTheme {
        Surface {
            FeedItemSuperCompact(
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
                        bookmarked = true,
                        feedImageUrl = null,
                        primarySortTime = Instant.EPOCH,
                        rawPubDate = null,
                        wordCount = 900,
                    ),
                onMarkAboveAsRead = {},
                onMarkBelowAsRead = {},
                onShareItem = {},
                onToggleBookmark = {},
                dropDownMenuExpanded = false,
                onDismissDropdown = {},
                bookmarkIndicator = true,
                maxLines = 2,
                showOnlyTitle = false,
                showReadingTime = true,
            )
        }
    }
}

@Composable
@Preview(showBackground = true)
private fun PreviewUnread() {
    FeederTheme {
        Surface {
            FeedItemSuperCompact(
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
                        bookmarked = false,
                        feedImageUrl = null,
                        primarySortTime = Instant.EPOCH,
                        rawPubDate = null,
                        wordCount = 900,
                    ),
                onMarkAboveAsRead = {},
                onMarkBelowAsRead = {},
                onShareItem = {},
                onToggleBookmark = {},
                dropDownMenuExpanded = false,
                onDismissDropdown = {},
                bookmarkIndicator = true,
                maxLines = 2,
                showOnlyTitle = false,
                showReadingTime = true,
            )
        }
    }
}

@Composable
@Preview(showBackground = true)
private fun PreviewWithImage() {
    FeederTheme {
        Surface {
            FeedItemSuperCompact(
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
                        feedImageUrl = URL("https://example.com/image.png"),
                        primarySortTime = Instant.EPOCH,
                        rawPubDate = null,
                        wordCount = 900,
                    ),
                onMarkAboveAsRead = {},
                onMarkBelowAsRead = {},
                onShareItem = {},
                onToggleBookmark = {},
                dropDownMenuExpanded = false,
                onDismissDropdown = {},
                bookmarkIndicator = true,
                maxLines = 2,
                showOnlyTitle = false,
                showReadingTime = true,
            )
        }
    }
}

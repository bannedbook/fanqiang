package com.nononsenseapps.feeder.ui.compose.feed

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.animateColorAsState
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.background
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.gestures.Orientation
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.material.ExperimentalMaterialApi
import androidx.compose.material.FractionalThreshold
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Visibility
import androidx.compose.material.icons.filled.VisibilityOff
import androidx.compose.material.rememberSwipeableState
import androidx.compose.material.swipeable
import androidx.compose.material3.Icon
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.platform.LocalLayoutDirection
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.CustomAccessibilityAction
import androidx.compose.ui.semantics.customActions
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.LayoutDirection
import androidx.compose.ui.unit.dp
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.archmodel.FeedItemStyle
import com.nononsenseapps.feeder.archmodel.SwipeAsRead
import com.nononsenseapps.feeder.ui.compose.components.safeSemantics
import com.nononsenseapps.feeder.ui.compose.feedarticle.FeedListFilter
import com.nononsenseapps.feeder.ui.compose.feedarticle.onlyUnread
import com.nononsenseapps.feeder.ui.compose.theme.LocalDimens
import com.nononsenseapps.feeder.ui.compose.theme.SwipingItemToReadColor
import com.nononsenseapps.feeder.ui.compose.theme.SwipingItemToUnreadColor
import com.nononsenseapps.feeder.ui.compose.utils.isCompactLandscape
import com.nononsenseapps.feeder.util.logDebug
import kotlinx.coroutines.launch
import kotlin.math.absoluteValue
import kotlin.math.roundToInt

private const val LOG_TAG = "FEEDER_SWIPEITEM"

/**
 * OnSwipe takes a boolean parameter of the current read state of the item - so that it can be
 * called multiple times by several DisposableEffects.
 */
@OptIn(
    ExperimentalFoundationApi::class,
    ExperimentalMaterialApi::class,
)
@Composable
fun SwipeableFeedItemPreview(
    onSwipe: (Boolean) -> Unit,
    filter: FeedListFilter,
    item: FeedListItem,
    showThumbnail: Boolean,
    feedItemStyle: FeedItemStyle,
    swipeAsRead: SwipeAsRead,
    bookmarkIndicator: Boolean,
    maxLines: Int,
    showOnlyTitle: Boolean,
    showReadingTime: Boolean,
    onMarkAboveAsRead: () -> Unit,
    onMarkBelowAsRead: () -> Unit,
    onToggleBookmark: () -> Unit,
    onShareItem: () -> Unit,
    modifier: Modifier = Modifier,
    onItemClick: () -> Unit,
) {
    val coroutineScope = rememberCoroutineScope()
    val isRtl = LocalLayoutDirection.current == LayoutDirection.Rtl
    val swipeableState = rememberSwipeableState(initialValue = FeedItemSwipeState.NONE)
    val onSwipeCallback by rememberUpdatedState(newValue = onSwipe)

    val color by animateColorAsState(
        targetValue =
            when {
                swipeableState.targetValue == FeedItemSwipeState.NONE -> Color.Transparent
                item.unread || filter.onlyUnread -> SwipingItemToReadColor
                else -> SwipingItemToUnreadColor
            },
        label = "swipeBackground",
    )

    LaunchedEffect(filter, item.unread) {
        // critical state changes - reset ui state
        swipeableState.animateTo(FeedItemSwipeState.NONE)
    }

    LaunchedEffect(swipeableState.currentValue, swipeableState.isAnimationRunning) {
        if (!swipeableState.isAnimationRunning && swipeableState.currentValue != FeedItemSwipeState.NONE) {
            logDebug(LOG_TAG, "onSwipe ${item.unread}")
            onSwipeCallback(item.unread)
        }
    }

    var swipeIconAlignment by remember { mutableStateOf(Alignment.CenterStart) }
    // Launched effect because I don't want a value change to zero to change the variable
    LaunchedEffect(swipeableState.direction) {
        if (swipeableState.direction == 1f) {
            swipeIconAlignment = Alignment.CenterStart
        } else if (swipeableState.direction == -1f) {
            swipeIconAlignment = Alignment.CenterEnd
        }
    }

    var dropDownMenuExpanded by rememberSaveable {
        mutableStateOf(false)
    }

    val toggleReadStatusLabel = stringResource(R.string.toggle_read_status)
    val saveArticleLabel = stringResource(R.string.save_article)
    val unSaveArticleLabel = stringResource(R.string.unsave_article)
    val markAboveAsReadLabel = stringResource(R.string.mark_items_above_as_read)
    val markBelowAsReadLabel = stringResource(R.string.mark_items_below_as_read)
    val shareLabel = stringResource(R.string.share)

    val unreadLabel = stringResource(R.string.unread_adjective)
    val alreadyReadLabel = stringResource(R.string.read_adjective)
    val readStatusLabel by remember(item.unread) {
        derivedStateOf {
            if (item.unread) {
                unreadLabel
            } else {
                alreadyReadLabel
            }
        }
    }

    val dimens = LocalDimens.current

    BoxWithConstraints(
        modifier =
            modifier
                .width(dimens.maxContentWidth)
                .combinedClickable(
                    onLongClick = {
                        dropDownMenuExpanded = true
                    },
                    onClick = onItemClick,
                )
                .safeSemantics {
                    stateDescription = readStatusLabel
                    customActions =
                        listOf(
                            CustomAccessibilityAction(toggleReadStatusLabel) {
                                coroutineScope.launch {
                                    onSwipe(item.unread)
                                }
                                true
                            },
                            CustomAccessibilityAction(
                                when (item.bookmarked) {
                                    true -> unSaveArticleLabel
                                    false -> saveArticleLabel
                                },
                            ) {
                                onToggleBookmark()
                                true
                            },
                            CustomAccessibilityAction(markAboveAsReadLabel) {
                                onMarkAboveAsRead()
                                true
                            },
                            CustomAccessibilityAction(markBelowAsReadLabel) {
                                onMarkBelowAsRead()
                                true
                            },
                            CustomAccessibilityAction(shareLabel) {
                                onShareItem()
                                true
                            },
                        )
                },
    ) {
        val maxWidthPx =
            with(LocalDensity.current) {
                maxWidth.toPx()
            }
        Box(
            contentAlignment = swipeIconAlignment,
            modifier =
                Modifier
                    .matchParentSize()
                    .background(color)
                    .padding(horizontal = 24.dp),
        ) {
            AnimatedVisibility(
                visible = swipeableState.targetValue != FeedItemSwipeState.NONE,
                enter = fadeIn(),
                exit = fadeOut(),
            ) {
                Icon(
                    when (item.unread) {
                        true -> Icons.Default.VisibilityOff
                        false -> Icons.Default.Visibility
                    },
                    contentDescription = null,
                )
            }
        }

        val itemAlpha by remember(swipeableState.progress) {
            derivedStateOf {
                if (swipeableState.progress.to == FeedItemSwipeState.NONE) {
                    1f
                } else if (swipeableState.progress.from != FeedItemSwipeState.NONE) {
                    0f
                } else {
                    (1f - swipeableState.progress.fraction.absoluteValue).coerceIn(0f, 1f)
                }
            }
        }

        val compactLandscape = isCompactLandscape()

        when (feedItemStyle) {
            FeedItemStyle.CARD -> {
                FeedItemCard(
                    item = item,
                    showThumbnail = showThumbnail && !compactLandscape,
                    onMarkAboveAsRead = onMarkAboveAsRead,
                    onMarkBelowAsRead = onMarkBelowAsRead,
                    onShareItem = onShareItem,
                    onToggleBookmark = onToggleBookmark,
                    dropDownMenuExpanded = dropDownMenuExpanded,
                    onDismissDropdown = { dropDownMenuExpanded = false },
                    bookmarkIndicator = bookmarkIndicator,
                    maxLines = maxLines,
                    showOnlyTitle = showOnlyTitle,
                    showReadingTime = showReadingTime,
                    modifier =
                        Modifier
                            .offset { IntOffset(swipeableState.offset.value.roundToInt(), 0) }
                            .graphicsLayer(alpha = itemAlpha),
                )
            }

            FeedItemStyle.COMPACT_CARD -> {
                FeedItemCompactCard(
                    state =
                        FeedItemState(
                            item = item,
                            showThumbnail = showThumbnail && !compactLandscape,
                            dropDownMenuExpanded = dropDownMenuExpanded,
                            bookmarkIndicator = bookmarkIndicator,
                            maxLines = maxLines,
                            showReadingTime = showReadingTime,
                        ),
                    modifier =
                        Modifier
                            .offset { IntOffset(swipeableState.offset.value.roundToInt(), 0) }
                            .graphicsLayer(alpha = itemAlpha),
                    onEvent = { event ->
                        when (event) {
                            FeedItemEvent.DismissDropdown -> {
                                dropDownMenuExpanded = false
                            }
                            FeedItemEvent.MarkAboveAsRead -> onMarkAboveAsRead()
                            FeedItemEvent.MarkBelowAsRead -> onMarkBelowAsRead()
                            FeedItemEvent.ShareItem -> onShareItem()
                            FeedItemEvent.ToggleBookmarked -> onToggleBookmark()
                        }
                    },
                )
            }

            FeedItemStyle.COMPACT -> {
                FeedItemCompact(
                    item = item,
                    showThumbnail = showThumbnail,
                    onMarkAboveAsRead = onMarkAboveAsRead,
                    onMarkBelowAsRead = onMarkBelowAsRead,
                    onShareItem = onShareItem,
                    onToggleBookmark = onToggleBookmark,
                    dropDownMenuExpanded = dropDownMenuExpanded,
                    onDismissDropdown = { dropDownMenuExpanded = false },
                    bookmarkIndicator = bookmarkIndicator,
                    maxLines = maxLines,
                    showOnlyTitle = showOnlyTitle,
                    showReadingTime = showReadingTime,
                    modifier =
                        Modifier
                            .offset { IntOffset(swipeableState.offset.value.roundToInt(), 0) }
                            .graphicsLayer(alpha = itemAlpha),
                    imageWidth =
                        when (compactLandscape) {
                            true -> 196.dp
                            false -> 64.dp
                        },
                )
            }

            FeedItemStyle.SUPER_COMPACT -> {
                FeedItemSuperCompact(
                    item = item,
                    onMarkAboveAsRead = onMarkAboveAsRead,
                    onMarkBelowAsRead = onMarkBelowAsRead,
                    onShareItem = onShareItem,
                    onToggleBookmark = onToggleBookmark,
                    dropDownMenuExpanded = dropDownMenuExpanded,
                    onDismissDropdown = { dropDownMenuExpanded = false },
                    bookmarkIndicator = bookmarkIndicator,
                    maxLines = maxLines,
                    showOnlyTitle = showOnlyTitle,
                    showReadingTime = showReadingTime,
                    modifier =
                        Modifier
                            .offset { IntOffset(swipeableState.offset.value.roundToInt(), 0) }
                            .graphicsLayer(alpha = itemAlpha),
                )
            }
        }

        // This box handles swiping - it uses padding to allow the nav drawer to still be dragged
        // It's very important that clickable stuff is handled by its parent - or a direct child
        // Wrapped in an outer box to get the height set properly
        if (swipeAsRead != SwipeAsRead.DISABLED) {
            Box(
                modifier =
                    Modifier
                        .matchParentSize(),
            ) {
                val anchors = mutableMapOf(0f to FeedItemSwipeState.NONE)
                Box(
                    modifier =
                        Modifier
                            .run {
                                @Suppress("KotlinConstantConditions")
                                when (swipeAsRead) {
                                    // This never actually gets called due to outer if
                                    SwipeAsRead.DISABLED ->
                                        this
                                            .height(0.dp)
                                            .width(0.dp)

                                    SwipeAsRead.ONLY_FROM_END -> {
                                        anchors[-maxWidthPx] = FeedItemSwipeState.LEFT
                                        this
                                            .fillMaxHeight()
                                            .width(this@BoxWithConstraints.maxWidth / 4)
                                            .align(Alignment.CenterEnd)
                                    }

                                    SwipeAsRead.FROM_ANYWHERE -> {
                                        anchors[-maxWidthPx] = FeedItemSwipeState.LEFT
                                        anchors[maxWidthPx] = FeedItemSwipeState.RIGHT
                                        this
                                            .padding(start = 48.dp)
                                            .matchParentSize()
                                    }
                                }
                            }
                            .swipeable(
                                state = swipeableState,
                                anchors = anchors,
                                orientation = Orientation.Horizontal,
                                reverseDirection = isRtl,
                                velocityThreshold = 1000.dp,
                                thresholds = { _, _ ->
                                    FractionalThreshold(0.50f)
                                },
                            ),
                )
            }
        }
    }
}

enum class FeedItemSwipeState {
    NONE,
    LEFT,
    RIGHT,
}

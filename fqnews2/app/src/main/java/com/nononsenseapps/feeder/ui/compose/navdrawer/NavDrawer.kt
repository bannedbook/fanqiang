package com.nononsenseapps.feeder.ui.compose.navdrawer

import android.util.Log
import androidx.compose.animation.ExperimentalAnimationApi
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.asPaddingValues
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.systemBars
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyListState
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ExpandLess
import androidx.compose.material.icons.filled.Star
import androidx.compose.material.minimumInteractiveComponentSize
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.rotate
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.CollectionInfo
import androidx.compose.ui.semantics.CustomAccessibilityAction
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.collectionInfo
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.customActions
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.semantics.testTag
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.paging.compose.LazyPagingItems
import androidx.paging.compose.itemContentType
import androidx.paging.compose.itemKey
import coil.compose.AsyncImage
import coil.request.ImageRequest
import coil.size.Precision
import coil.size.Scale
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.db.room.ID_ALL_FEEDS
import com.nononsenseapps.feeder.db.room.ID_SAVED_ARTICLES
import com.nononsenseapps.feeder.db.room.ID_UNSET
import com.nononsenseapps.feeder.model.FeedUnreadCount
import com.nononsenseapps.feeder.ui.compose.components.safeSemantics
import com.nononsenseapps.feeder.ui.compose.material3.DismissibleDrawerSheet
import com.nononsenseapps.feeder.ui.compose.material3.DismissibleNavigationDrawer
import com.nononsenseapps.feeder.ui.compose.material3.DrawerState
import com.nononsenseapps.feeder.ui.compose.utils.onKeyEventLikeEscape
import kotlinx.coroutines.launch

@OptIn(ExperimentalMaterial3Api::class, ExperimentalAnimationApi::class)
@Composable
fun ScreenWithNavDrawer(
    feedsAndTags: LazyPagingItems<FeedUnreadCount>,
    onToggleTagExpansion: (String) -> Unit,
    onDrawerItemSelect: (Long, String) -> Unit,
    drawerState: DrawerState,
    focusRequester: FocusRequester,
    navDrawerListState: LazyListState,
    modifier: Modifier = Modifier,
    content: @Composable () -> Unit,
) {
    val coroutineScope = rememberCoroutineScope()

    LaunchedEffect(drawerState.isOpen) {
        if (drawerState.isOpen) {
            focusRequester.requestFocus()
        }
    }

    DismissibleNavigationDrawer(
        modifier =
            modifier
                .onKeyEventLikeEscape {
                    coroutineScope.launch {
                        drawerState.close()
                    }
                },
        drawerState = drawerState,
        drawerContent = {
            DismissibleDrawerSheet {
                ListOfFeedsAndTags(
                    state = navDrawerListState,
                    modifier =
                        Modifier
                            .focusRequester(focusRequester),
                    feedsAndTags = feedsAndTags,
                    onToggleTagExpansion = onToggleTagExpansion,
                ) { item ->
                    coroutineScope.launch {
                        onDrawerItemSelect(item.id, item.tag)
                        drawerState.close()
                    }
                }
            }
        },
        content = content,
    )
}

@ExperimentalAnimationApi
@Composable
fun ListOfFeedsAndTags(
    feedsAndTags: LazyPagingItems<FeedUnreadCount>,
    onToggleTagExpansion: (String) -> Unit,
    state: LazyListState,
    modifier: Modifier = Modifier,
    onItemClick: (FeedIdTag) -> Unit,
) {
    LazyColumn(
        state = state,
        contentPadding = WindowInsets.systemBars.asPaddingValues(),
        modifier =
            modifier
                .fillMaxSize()
                .semantics {
                    testTag = "feedsAndTags"
                    collectionInfo = CollectionInfo(feedsAndTags.itemCount, 1)
                },
    ) {
        items(
            count = feedsAndTags.itemCount,
            key =
                feedsAndTags.itemKey {
                    when (it.id) {
                        ID_ALL_FEEDS -> ID_ALL_FEEDS
                        ID_SAVED_ARTICLES -> ID_SAVED_ARTICLES
                        ID_UNSET -> it.tag
                        else -> it.id
                    }
                },
            contentType =
                feedsAndTags.itemContentType {
                    it.contentType
                },
        ) { itemIndex ->
            val item = feedsAndTags[itemIndex]
            when (item?.contentType) {
                null -> {
                    Placeholder()
                }
                ContentType.AllFeeds -> {
                    AllFeeds(
                        unreadCount = item.unreadCount,
                        title = stringResource(id = R.string.all_feeds),
                        onItemClick = {
                            onItemClick(item)
                        },
                    )
                }
                ContentType.SavedArticles -> {
                    SavedArticles(
                        unreadCount = item.unreadCount,
                        title = stringResource(id = R.string.saved_articles),
                        onItemClick = {
                            onItemClick(item)
                        },
                    )
                }
                ContentType.Tag -> {
                    ExpandableTag(
                        expanded = item.expanded,
                        onToggleExpansion = onToggleTagExpansion,
                        unreadCount = item.unreadCount,
                        title = item.tag,
                        onItemClick = {
                            onItemClick(item)
                        },
                    )
                }
                ContentType.ChildFeed -> {
                    ChildFeed(
                        unreadCount = item.unreadCount,
                        title = item.displayTitle,
                        imageUrl = item.imageUrl?.toString(),
                        onItemClick = {
                            onItemClick(item)
                        },
                    )
                }
                ContentType.TopLevelFeed -> {
                    TopLevelFeed(
                        unreadCount = item.unreadCount,
                        title = item.displayTitle,
                        imageUrl = item.imageUrl?.toString(),
                        onItemClick = {
                            onItemClick(item)
                        },
                    )
                }
            }
        }
    }
}

val FeedUnreadCount.contentType: ContentType
    get() =
        when (this.id) {
            ID_UNSET -> ContentType.Tag
            ID_ALL_FEEDS -> ContentType.AllFeeds
            ID_SAVED_ARTICLES -> ContentType.SavedArticles
            else -> {
                if (this.tag.isNotEmpty()) {
                    ContentType.ChildFeed
                } else {
                    ContentType.TopLevelFeed
                }
            }
        }

enum class ContentType {
    AllFeeds,
    SavedArticles,
    Tag,
    ChildFeed,
    TopLevelFeed,
}

@ExperimentalAnimationApi
@Preview(showBackground = true)
@Composable
private fun ExpandableTag(
    title: String = "Foo",
    unreadCount: Int = 99,
    expanded: Boolean = true,
    onToggleExpansion: (String) -> Unit = {},
    onItemClick: () -> Unit = {},
) {
    val angle: Float by animateFloatAsState(
        targetValue = if (expanded) 0f else 180f,
        animationSpec = tween(),
        label = "angle",
    )

    val toggleExpandLabel = stringResource(id = R.string.toggle_tag_expansion)
    val expandedLabel = stringResource(id = R.string.expanded_tag)
    val contractedLabel = stringResource(id = R.string.contracted_tag)

    Row(
        horizontalArrangement = Arrangement.spacedBy(4.dp),
        verticalAlignment = Alignment.CenterVertically,
        modifier =
            Modifier
                .padding(top = 2.dp, bottom = 2.dp, end = 16.dp)
                .fillMaxWidth()
                .height(48.dp)
                .safeSemantics(mergeDescendants = true) {
                    stateDescription =
                        if (expanded) {
                            expandedLabel
                        } else {
                            contractedLabel
                        }
                    customActions =
                        listOf(
                            CustomAccessibilityAction(toggleExpandLabel) {
                                onToggleExpansion(title)
                                true
                            },
                        )
                },
    ) {
        ExpandArrow(
            degrees = angle,
            onClick = {
                onToggleExpansion(title)
            },
        )
        Box(
            modifier =
                Modifier
                    .clickable(onClick = onItemClick)
                    .fillMaxHeight()
                    .weight(1.0f, fill = true),
        ) {
            Text(
                text = title,
                maxLines = 1,
                modifier =
                    Modifier
                        .fillMaxWidth()
                        .align(Alignment.CenterStart),
            )
        }
        if (unreadCount > 0) {
            val unreadLabel =
                LocalContext.current.resources.getQuantityString(
                    R.plurals.n_unread_articles,
                    unreadCount,
                    unreadCount,
                )
            Text(
                text = unreadCount.toString(),
                maxLines = 1,
                modifier =
                    Modifier
                        .semantics {
                            contentDescription = unreadLabel
                        },
            )
        }
    }
}

@Composable
private fun ExpandArrow(
    degrees: Float,
    onClick: () -> Unit,
) {
    IconButton(onClick = onClick, modifier = Modifier.clearAndSetSemantics { }) {
        Icon(
            Icons.Filled.ExpandLess,
            contentDescription = stringResource(id = R.string.toggle_tag_expansion),
            modifier = Modifier.rotate(degrees = degrees),
        )
    }
}

@Preview(showBackground = true)
@Composable
private fun SavedArticles(
    title: String = "Bar",
    unreadCount: Int = 10,
    onItemClick: () -> Unit = {},
) {
    Feed(
        title = title,
        unreadCount = unreadCount,
        onItemClick = onItemClick,
        image = {
            Icon(
                Icons.Default.Star,
                contentDescription = null,
                modifier = Modifier.size(24.dp),
            )
        },
    )
}

@Preview(showBackground = true)
@Composable
private fun TopLevelFeed(
    title: String = "Foo",
    unreadCount: Int = 99,
    onItemClick: () -> Unit = {},
    imageUrl: String? = null,
) = Feed(
    title = title,
    imageUrl = imageUrl,
    unreadCount = unreadCount,
    onItemClick = onItemClick,
)

@Preview(showBackground = true)
@Composable
private fun ChildFeed(
    title: String = "Foo",
    imageUrl: String? = null,
    unreadCount: Int = 99,
    onItemClick: () -> Unit = {},
) {
    Feed(
        title = title,
        imageUrl = imageUrl,
        unreadCount = unreadCount,
        onItemClick = onItemClick,
    )
}

@Preview(showBackground = true)
@Composable
private fun Placeholder() {
    Box(
        modifier =
            Modifier
                .fillMaxWidth()
                .height(48.dp),
    ) {
    }
}

@Preview(showBackground = true)
@Composable
private fun AllFeeds(
    title: String = "All Feeds",
    unreadCount: Int = 99,
    onItemClick: () -> Unit = {},
) {
    Row(
        horizontalArrangement = Arrangement.spacedBy(4.dp),
        verticalAlignment = Alignment.CenterVertically,
        modifier =
            Modifier
                .clickable(onClick = onItemClick)
                .padding(
                    start = 16.dp,
                    end = 16.dp,
                    top = 2.dp,
                    bottom = 2.dp,
                )
                .fillMaxWidth()
                .height(48.dp),
    ) {
        Box(
            modifier =
                Modifier
                    .fillMaxHeight()
                    .weight(1.0f, fill = true),
        ) {
            Text(
                text = title,
                maxLines = 1,
                modifier =
                    Modifier
                        .fillMaxWidth()
                        .align(Alignment.CenterStart),
            )
        }
        if (unreadCount > 0) {
            val unreadLabel =
                LocalContext.current.resources.getQuantityString(
                    R.plurals.n_unread_articles,
                    unreadCount,
                    unreadCount,
                )
            Text(
                text = unreadCount.toString(),
                maxLines = 1,
                modifier =
                    Modifier.semantics {
                        contentDescription = unreadLabel
                    },
            )
        }
    }
}

@Composable
private fun Feed(
    title: String,
    image: (@Composable () -> Unit),
    unreadCount: Int,
    onItemClick: () -> Unit,
) {
    Row(
        horizontalArrangement = Arrangement.spacedBy(4.dp),
        verticalAlignment = Alignment.CenterVertically,
        modifier =
            Modifier
                .clickable(onClick = onItemClick)
                .padding(
                    start = 0.dp,
                    end = 16.dp,
                    top = 2.dp,
                    bottom = 2.dp,
                )
                .fillMaxWidth()
                .height(48.dp),
    ) {
        Box(
            contentAlignment = Alignment.Center,
            modifier =
                Modifier
                    .minimumInteractiveComponentSize(),
            //                    .height(48.dp)
//                    // Taking 4dp spacing into account
//                    .width(44.dp),
        ) {
            image()
        }
        Box(
            modifier =
                Modifier
                    .fillMaxHeight()
                    .weight(1.0f, fill = true),
        ) {
            Text(
                text = title,
                maxLines = 1,
                modifier =
                    Modifier
                        .fillMaxWidth()
                        .align(Alignment.CenterStart),
            )
        }
        if (unreadCount > 0) {
            val unreadLabel =
                LocalContext.current.resources.getQuantityString(
                    R.plurals.n_unread_articles,
                    unreadCount,
                    unreadCount,
                )
            Text(
                text = unreadCount.toString(),
                maxLines = 1,
                modifier =
                    Modifier.semantics {
                        contentDescription = unreadLabel
                    },
            )
        }
    }
}

@Composable
private fun Feed(
    title: String,
    imageUrl: String?,
    unreadCount: Int,
    onItemClick: () -> Unit,
) {
    Feed(
        title = title,
        unreadCount = unreadCount,
        onItemClick = onItemClick,
        image =
            if (imageUrl != null) {
                {
                    val pixels =
                        with(LocalDensity.current) {
                            24.dp.roundToPx()
                        }
                    AsyncImage(
                        model =
                            ImageRequest.Builder(LocalContext.current)
                                .data(imageUrl.toString()).listener(
                                    onError = { a, b ->
                                        Log.e("FEEDER_DRAWER", "error ${a.data}", b.throwable)
                                    },
                                )
                                .scale(Scale.FIT)
                                .size(pixels)
                                .precision(Precision.INEXACT)
                                .build(),
                        contentDescription = stringResource(id = R.string.feed_icon),
                        contentScale = ContentScale.Crop,
                        modifier = Modifier.size(24.dp),
                    )
                }
            } else {
                {
                    Box(modifier = Modifier.size(24.dp)) {}
                }
            },
    )
}

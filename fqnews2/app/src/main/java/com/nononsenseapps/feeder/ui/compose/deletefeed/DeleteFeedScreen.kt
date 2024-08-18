package com.nononsenseapps.feeder.ui.compose.deletefeed

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.requiredHeightIn
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Checkbox
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.Immutable
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.role
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.ui.compose.components.safeSemantics
import com.nononsenseapps.feeder.ui.compose.minimumTouchSize
import com.nononsenseapps.feeder.ui.compose.utils.ImmutableHolder
import com.nononsenseapps.feeder.ui.compose.utils.immutableListHolderOf

@Composable
fun DeleteFeedDialog(
    feeds: ImmutableHolder<List<DeletableFeed>>,
    onDismiss: () -> Unit,
    onDelete: (Iterable<Long>) -> Unit,
    modifier: Modifier = Modifier,
) {
    var feedsToDelete by rememberSaveable {
        mutableStateOf(emptyMap<Long, Boolean>())
    }
    val selectAllSelected by remember {
        derivedStateOf {
            feeds.item.all { feed -> feedsToDelete[feed.id] ?: false }
        }
    }
    DeleteFeedDialog(
        feeds = feeds,
        isChecked = { feedId ->
            feedsToDelete[feedId] ?: false
        },
        selectAllSelected = selectAllSelected,
        onDismiss = onDismiss,
        onOk = {
            onDelete(feedsToDelete.filterValues { it }.keys)
            onDismiss()
        },
        onToggleFeed = { feedId, checked ->
            feedsToDelete = feedsToDelete + (feedId to (checked ?: !feedsToDelete.contains(feedId)))
        },
        onSelectAll = { checked ->
            feedsToDelete = feeds.item.associate { it.id to checked }
        },
        modifier = modifier,
    )
}

@Composable
fun DeleteFeedDialog(
    feeds: ImmutableHolder<List<DeletableFeed>>,
    isChecked: (Long) -> Boolean,
    selectAllSelected: Boolean,
    onDismiss: () -> Unit,
    onOk: () -> Unit,
    onToggleFeed: (Long, Boolean?) -> Unit,
    onSelectAll: (Boolean) -> Unit,
    modifier: Modifier = Modifier,
) {
    var menuExpanded by rememberSaveable {
        mutableStateOf(false)
    }
    AlertDialog(
        modifier = modifier,
        onDismissRequest = onDismiss,
        confirmButton = {
            Button(onClick = onOk) {
                Text(text = stringResource(id = android.R.string.ok))
            }
        },
        dismissButton = {
            Button(onClick = onDismiss) {
                Text(text = stringResource(id = android.R.string.cancel))
            }
        },
        title = {
            Box(modifier = Modifier.fillMaxWidth()) {
                val stateLabel =
                    if (selectAllSelected) {
                        stringResource(androidx.compose.ui.R.string.selected)
                    } else {
                        stringResource(androidx.compose.ui.R.string.not_selected)
                    }
                Text(
                    text = stringResource(id = R.string.delete_feed),
                    style = MaterialTheme.typography.titleMedium,
                    modifier = Modifier.align(Alignment.CenterStart),
                )
                Box(modifier = Modifier.align(Alignment.CenterEnd)) {
                    IconButton(onClick = { menuExpanded = true }) {
                        Icon(Icons.Default.MoreVert, contentDescription = stringResource(R.string.open_menu))
                    }

                    DropdownMenu(expanded = menuExpanded, onDismissRequest = { menuExpanded = false }) {
                        DropdownMenuItem(
                            text = {
                                Row(verticalAlignment = Alignment.CenterVertically) {
                                    Checkbox(
                                        checked = selectAllSelected,
                                        onCheckedChange = { checked ->
                                            onSelectAll(checked)
                                            menuExpanded = false
                                        },
                                        modifier = Modifier.clearAndSetSemantics { },
                                    )
                                    Spacer(modifier = Modifier.width(4.dp))
                                    Text(
                                        text = stringResource(id = android.R.string.selectAll),
                                        style = MaterialTheme.typography.titleMedium,
                                    )
                                }
                            },
                            onClick = {
                                onSelectAll(!selectAllSelected)
                                menuExpanded = false
                            },
                            modifier =
                                Modifier.safeSemantics(mergeDescendants = true) {
                                    stateDescription = stateLabel
                                },
                        )
                        val closeMenuText = stringResource(id = R.string.close_menu)
                        // Hidden button for TalkBack
                        DropdownMenuItem(
                            onClick = {
                                menuExpanded = false
                            },
                            text = {},
                            modifier =
                                Modifier
                                    .height(0.dp)
                                    .safeSemantics {
                                        contentDescription = closeMenuText
                                        role = Role.Button
                                    },
                        )
                    }
                }
            }
        },
        text = {
            LazyColumn(
                modifier = Modifier.fillMaxWidth(),
            ) {
                items(
                    feeds.item,
                    key = { feed -> feed.id },
                ) { feed ->
                    val stateLabel =
                        if (isChecked(feed.id)) {
                            stringResource(androidx.compose.ui.R.string.selected)
                        } else {
                            stringResource(androidx.compose.ui.R.string.not_selected)
                        }
                    Row(
                        horizontalArrangement = Arrangement.Start,
                        verticalAlignment = Alignment.CenterVertically,
                        modifier =
                            Modifier
                                .fillMaxWidth()
                                .requiredHeightIn(min = minimumTouchSize)
                                .clickable {
                                    onToggleFeed(feed.id, !isChecked(feed.id))
                                }
                                .safeSemantics(mergeDescendants = true) {
                                    stateDescription = stateLabel
                                },
                    ) {
                        Checkbox(
                            checked = isChecked(feed.id),
                            onCheckedChange = { checked ->
                                onToggleFeed(feed.id, checked)
                            },
                            modifier = Modifier.clearAndSetSemantics { },
                        )
                        Spacer(modifier = Modifier.width(8.dp))
                        Text(
                            text = feed.title,
                            style = MaterialTheme.typography.titleMedium,
                        )
                    }
                }
            }
        },
    )
}

@Immutable
data class DeletableFeed(
    val id: Long,
    val title: String,
)

@Composable
@Preview
private fun Preview() =
    DeleteFeedDialog(
        feeds =
            immutableListHolderOf(
                DeletableFeed(1, "A Feed"),
                DeletableFeed(2, "Another Feed"),
            ),
        onDismiss = {},
        onDelete = {},
    )

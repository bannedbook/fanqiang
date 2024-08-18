package com.nononsenseapps.feeder.ui.compose.dialog

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.ProvideTextStyle
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TextFieldDefaults
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.role
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.ui.compose.components.safeSemantics
import com.nononsenseapps.feeder.ui.compose.minimumTouchSize
import com.nononsenseapps.feeder.ui.compose.settings.UIFeedSettings
import com.nononsenseapps.feeder.ui.compose.theme.FeederTheme
import com.nononsenseapps.feeder.ui.compose.theme.LocalDimens
import com.nononsenseapps.feeder.ui.compose.utils.ImmutableHolder

@Composable
fun FeedNotificationsDialog(
    title: @Composable () -> Unit,
    items: ImmutableHolder<List<UIFeedSettings>>,
    onDismiss: () -> Unit,
    onToggleItem: (Long, Boolean) -> Unit,
    modifier: Modifier = Modifier,
) {
    val lazyListState = rememberLazyListState()

    AlertDialog(
        modifier = modifier,
        onDismissRequest = onDismiss,
        confirmButton = {
            Box(
                Modifier
                    .fillMaxWidth(),
            ) {
                TextButton(
                    enabled = true,
                    onClick = onDismiss,
                ) {
                    Text(
                        text = stringResource(id = android.R.string.ok),
                    )
                }
            }
        },
        title = title,
        text = {
            LazyColumn(
                state = lazyListState,
                modifier =
                    Modifier
                        .fillMaxWidth()
                        .heightIn(min = TextFieldDefaults.MinHeight * 3.3f),
            ) {
                items(
                    items.item,
                    key = { item -> item.feedId },
                ) { item ->
                    Row(
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.SpaceBetween,
                        modifier =
                            Modifier
                                .fillMaxWidth()
                                .heightIn(min = minimumTouchSize),
                    ) {
                        val stateLabel =
                            if (item.notify) {
                                stringResource(androidx.compose.ui.R.string.on)
                            } else {
                                stringResource(androidx.compose.ui.R.string.off)
                            }
                        val dimens = LocalDimens.current
                        Row(
                            modifier =
                                modifier
                                    .width(dimens.maxContentWidth)
                                    .heightIn(min = 64.dp)
                                    .clickable(
                                        enabled = true,
                                        onClick = {
                                            onToggleItem(
                                                item.feedId,
                                                !item.notify,
                                            )
                                        },
                                    )
                                    .safeSemantics(mergeDescendants = true) {
                                        stateDescription = stateLabel
                                        role = Role.Switch
                                    },
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.spacedBy(16.dp),
                        ) {
                            ProvideTextStyle(value = MaterialTheme.typography.titleMedium) {
                                Text(
                                    item.title,
                                    overflow = TextOverflow.Ellipsis,
                                    maxLines = 2,
                                    modifier = Modifier.weight(1f),
                                )
                            }

                            Switch(
                                checked = item.notify,
                                onCheckedChange = {
                                    onToggleItem(
                                        item.feedId,
                                        !item.notify,
                                    )
                                },
                                modifier = Modifier.clearAndSetSemantics { },
                                enabled = true,
                            )
                        }
                    }
                }
            }
        },
    )
}

@Preview
@Composable
private fun PreviewNotificationsDialog() {
    FeederTheme {
        FeedNotificationsDialog(
            title = {
                Text(
                    text = stringResource(id = R.string.notify_for_new_items),
                    style = MaterialTheme.typography.titleLarge,
                )
            },
            ImmutableHolder(
                listOf(
                    UIFeedSettings(1L, "Foo", false),
                    UIFeedSettings(2L, "Barasdf asdfasd asdfasdf asdfasdf adsfasd", true),
                    UIFeedSettings(3L, "Caraasdfasdfasdfasdfasdfasdfasdfas", true),
                ),
            ),
            onToggleItem = { _, _ -> },
            onDismiss = {},
        )
    }
}

package com.nononsenseapps.feeder.ui.compose.feed

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.requiredHeightIn
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.RadioButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.ui.compose.deletefeed.DeletableFeed
import com.nononsenseapps.feeder.ui.compose.minimumTouchSize
import com.nononsenseapps.feeder.ui.compose.theme.FeederTheme
import com.nononsenseapps.feeder.ui.compose.utils.ImmutableHolder
import com.nononsenseapps.feeder.ui.compose.utils.PreviewThemes
import com.nononsenseapps.feeder.ui.compose.utils.immutableListHolderOf

@Composable
fun EditFeedDialog(
    feeds: ImmutableHolder<List<DeletableFeed>>,
    onDismiss: () -> Unit,
    modifier: Modifier = Modifier,
    onEdit: (Long) -> Unit,
) {
    AlertDialog(
        modifier = modifier,
        onDismissRequest = onDismiss,
        confirmButton = {
//            Button(onClick = onOk) {
//                Text(text = stringResource(id = R.string.ok))
//            }
        },
        dismissButton = {
            Button(onClick = onDismiss) {
                Text(text = stringResource(id = android.R.string.cancel))
            }
        },
        title = {
            Text(
                text = stringResource(id = R.string.edit_feed),
                style = MaterialTheme.typography.titleLarge,
                textAlign = TextAlign.Center,
                modifier =
                    Modifier
                        .padding(vertical = 8.dp),
            )
        },
        text = {
            LazyColumn(
                modifier =
                    Modifier
                        .fillMaxWidth(),
            ) {
                items(
                    feeds.item,
                    key = { feed -> feed.id },
                ) { feed ->
                    Row(
                        horizontalArrangement = Arrangement.Start,
                        verticalAlignment = Alignment.CenterVertically,
                        modifier =
                            Modifier
                                .fillMaxWidth()
                                .requiredHeightIn(min = minimumTouchSize)
                                .clickable {
                                    onEdit(feed.id)
                                    onDismiss()
                                }
                                .semantics(mergeDescendants = true) {},
                    ) {
                        RadioButton(
                            selected = false,
                            onClick = {
                                onEdit(feed.id)
                                onDismiss()
                            },
                            modifier = Modifier.clearAndSetSemantics { },
                        )
                        Spacer(modifier = Modifier.width(32.dp))
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

@Composable
@PreviewThemes
private fun Preview() {
    FeederTheme {
        EditFeedDialog(
            feeds =
                immutableListHolderOf(
                    DeletableFeed(1, "A Feed"),
                    DeletableFeed(2, "Another Feed"),
                ),
            onDismiss = {},
        ) {}
    }
}

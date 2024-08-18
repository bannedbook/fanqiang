package com.nononsenseapps.feeder.ui.compose.dialog

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TextFieldDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.ExperimentalComposeUiApi
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.key.Key
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.text.input.KeyboardCapitalization
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.ui.compose.minimumTouchSize
import com.nononsenseapps.feeder.ui.compose.modifiers.interceptKey
import com.nononsenseapps.feeder.ui.compose.theme.FeederTheme
import com.nononsenseapps.feeder.ui.compose.utils.ImmutableHolder

@OptIn(ExperimentalComposeUiApi::class)
@Composable
fun EditableListDialog(
    title: @Composable () -> Unit,
    items: ImmutableHolder<List<String>>,
    onDismiss: () -> Unit,
    onRemoveItem: (String) -> Unit,
    onAddItem: (String) -> Unit,
    modifier: Modifier = Modifier,
) {
    var newValue by rememberSaveable {
        mutableStateOf("")
    }

    val lazyListState = rememberLazyListState()

    AlertDialog(
        modifier = modifier,
        onDismissRequest = onDismiss,
        confirmButton = {
            Box(
                Modifier
                    .fillMaxWidth(),
            ) {
                OutlinedTextField(
                    value = newValue,
                    onValueChange = {
                        newValue = it
                    },
                    label = {
                        Text(stringResource(id = R.string.add_item))
                    },
                    keyboardOptions =
                        KeyboardOptions(
                            capitalization = KeyboardCapitalization.None,
                            autoCorrect = true,
                            keyboardType = KeyboardType.Text,
                            imeAction = ImeAction.Next,
                        ),
                    keyboardActions =
                        KeyboardActions(
                            onNext = {
                                onAddItem(newValue)
                                newValue = ""
                            },
                        ),
                    modifier =
                        Modifier
                            .fillMaxWidth()
                            .interceptKey(Key.Enter) {
                                onAddItem(newValue)
                                newValue = ""
                            }
                            .interceptKey(Key.Escape) {
                                onDismiss()
                            },
                )
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
                    key = { item -> item },
                ) { item ->
                    Row(
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.SpaceBetween,
                        modifier =
                            Modifier
                                .fillMaxWidth()
                                .heightIn(min = minimumTouchSize),
                    ) {
                        Text(
                            text = item,
                            style = MaterialTheme.typography.titleMedium,
                            modifier = Modifier.weight(1f, fill = true),
                        )
                        IconButton(
                            onClick = {
                                onRemoveItem(item)
                            },
                        ) {
                            Icon(
                                Icons.Filled.Delete,
                                contentDescription = stringResource(R.string.remove),
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
private fun PreviewDialog() {
    FeederTheme {
        EditableListDialog(
            title = {
                Column(
                    verticalArrangement = Arrangement.spacedBy(8.dp),
                ) {
                    Text(
                        text = stringResource(id = R.string.block_list),
                        style = MaterialTheme.typography.titleLarge,
                    )
                    Text(
                        text = "Hides articles with titles containing any of these terms. Example filters:",
                        style = MaterialTheme.typography.bodyMedium,
                    )
                    Text(
                        text = "feeder feed?r fe*er",
                        style = MaterialTheme.typography.bodySmall.copy(fontFamily = FontFamily.Monospace),
                    )
                }
            },
            items = ImmutableHolder(listOf("Foo")),
            onDismiss = {},
            onAddItem = {},
            onRemoveItem = {},
        )
    }
}

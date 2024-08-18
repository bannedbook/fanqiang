package com.nononsenseapps.feeder.ui.compose.components

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TextFieldDefaults
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import com.nononsenseapps.feeder.ui.compose.utils.ImmutableHolder
import com.nononsenseapps.feeder.ui.compose.utils.immutableListHolderOf

@Composable
fun AutoCompleteResults(
    displaySuggestions: Boolean,
    suggestions: ImmutableHolder<List<String>>,
    onSuggestionClick: (String) -> Unit,
    suggestionContent: @Composable (String) -> Unit,
    modifier: Modifier = Modifier,
    maxHeight: Dp = TextFieldDefaults.MinHeight * 3,
    content: @Composable () -> Unit,
) {
    Column(modifier = modifier) {
        content()

        Spacer(modifier = Modifier.height(4.dp))

        AnimatedVisibility(visible = displaySuggestions) {
            LazyColumn(
                modifier =
                    Modifier
                        .heightIn(max = maxHeight)
                        .fillMaxWidth(0.9f)
                        .border(
                            border = BorderStroke(2.dp, MaterialTheme.colorScheme.onBackground),
                            shape = RoundedCornerShape(8.dp),
                        ),
            ) {
                items(
                    suggestions.item,
                    key = { item -> item },
                ) { item ->
                    Box(
                        modifier =
                            Modifier
                                .clickable { onSuggestionClick(item) },
                    ) {
                        suggestionContent(item)
                    }
                }
            }
        }
    }
}

@Preview
@Composable
private fun PreviewAutoCompleteOutlinedText() {
    AutoCompleteResults(
        displaySuggestions = true,
        suggestions = immutableListHolderOf("One", "Two", "Three"),
        onSuggestionClick = {},
        suggestionContent = {
            Text(text = it)
        },
    ) {
        OutlinedTextField(value = "Testing", onValueChange = {})
    }
}

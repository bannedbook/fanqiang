package com.nononsenseapps.feeder.ui.compose.components

import android.R
import androidx.annotation.StringRes
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp

@Composable
fun ConfirmDialog(
    onDismiss: () -> Unit,
    onOk: () -> Unit,
    @StringRes title: Int,
    @StringRes body: Int,
    modifier: Modifier = Modifier,
) {
    AlertDialog(
        modifier = modifier,
        onDismissRequest = onDismiss,
        confirmButton = {
            Button(onClick = onOk) {
                Text(text = stringResource(id = R.string.ok))
            }
        },
        dismissButton = {
            Button(onClick = onDismiss) {
                Text(text = stringResource(id = R.string.cancel))
            }
        },
        title = {
            Text(
                text = stringResource(id = title),
                style = MaterialTheme.typography.titleLarge,
                textAlign = TextAlign.Center,
                modifier =
                    Modifier
                        .padding(vertical = 8.dp),
            )
        },
        text = {
            Text(
                text = stringResource(id = body),
                style = MaterialTheme.typography.bodyLarge,
            )
        },
    )
}

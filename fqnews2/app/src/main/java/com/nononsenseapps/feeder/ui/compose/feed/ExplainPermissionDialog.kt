package com.nononsenseapps.feeder.ui.compose.feed

import androidx.annotation.StringRes
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.nononsenseapps.feeder.R

@Composable
fun ExplainPermissionDialog(
    @StringRes explanation: Int,
    onDismiss: () -> Unit,
    modifier: Modifier = Modifier,
    onOk: () -> Unit,
) {
    AlertDialog(
        modifier = modifier,
        onDismissRequest = onDismiss,
        confirmButton = {
            Button(onClick = onOk) {
                Text(text = stringResource(id = android.R.string.ok))
            }
        },
        text = {
            Text(
                text = stringResource(id = explanation),
                style = MaterialTheme.typography.bodyLarge,
                modifier =
                    Modifier
                        .padding(vertical = 8.dp),
            )
        },
    )
}

@Composable
@Preview
private fun Preview() =
    ExplainPermissionDialog(
        explanation = R.string.explanation_permission_notifications,
        onDismiss = {},
    ) {}

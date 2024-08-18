package com.nononsenseapps.feeder.ui.compose.sync

import androidx.compose.runtime.Composable
import androidx.compose.ui.tooling.preview.Preview
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.ui.compose.components.ConfirmDialog

@Composable
fun LeaveSyncChainDialog(
    onDismiss: () -> Unit,
    onOk: () -> Unit,
) = ConfirmDialog(
    onDismiss = onDismiss,
    onOk = onOk,
    title = R.string.leave_sync_chain,
    body = R.string.are_you_sure_leave_sync_chain,
)

@Preview
@Composable
private fun PreviewLeaveSyncChainDialog() {
    LeaveSyncChainDialog(
        onDismiss = {},
        onOk = {},
    )
}

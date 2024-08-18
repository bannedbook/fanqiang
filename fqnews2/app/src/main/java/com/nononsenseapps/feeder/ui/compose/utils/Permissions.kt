package com.nononsenseapps.feeder.ui.compose.utils

import android.os.Build
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.platform.LocalInspectionMode
import com.google.accompanist.permissions.ExperimentalPermissionsApi
import com.google.accompanist.permissions.PermissionState
import com.google.accompanist.permissions.PermissionStatus
import com.google.accompanist.permissions.rememberPermissionState

@OptIn(ExperimentalPermissionsApi::class)
@Composable
fun rememberApiPermissionState(
    permission: String,
    minimumApiLevel: Int = 1,
    onPermissionResult: (Boolean) -> Unit = {},
): PermissionState =
    when {
        // When layout is being previewed
        !LocalInspectionMode.current && Build.VERSION.SDK_INT >= minimumApiLevel -> rememberPermissionState(permission = permission, onPermissionResult = onPermissionResult)
        else -> remember { GrantedByDefaultPermission(permission) }
    }

@OptIn(ExperimentalPermissionsApi::class)
class GrantedByDefaultPermission(override val permission: String) : PermissionState {
    override val status: PermissionStatus = PermissionStatus.Granted

    override fun launchPermissionRequest() {
        // Nothing to do
    }
}

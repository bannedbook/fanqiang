package com.nononsenseapps.feeder.ui.compose.utils

import android.content.res.Configuration
import androidx.compose.ui.tooling.preview.Preview

@Preview(
    name = "Light",
    showBackground = true,
    uiMode = Configuration.UI_MODE_NIGHT_NO,
)
@Preview(
    name = "Dark",
    showBackground = true,
    uiMode = Configuration.UI_MODE_NIGHT_YES,
)
annotation class PreviewThemes

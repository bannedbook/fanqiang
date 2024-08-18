package com.nononsenseapps.feeder.ui.compose.empty

import android.content.res.Configuration
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextFieldDefaults
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.ui.compose.text.annotatedStringResource
import com.nononsenseapps.feeder.ui.compose.theme.FeederTheme
import com.nononsenseapps.feeder.ui.compose.theme.LocalDimens

@Composable
fun NothingToRead(
    modifier: Modifier = Modifier,
    onOpenOtherFeed: () -> Unit = {},
    onAddFeed: () -> Unit = {},
) {
    Box(
        contentAlignment = Alignment.Center,
        modifier =
            modifier
                .padding(horizontal = LocalDimens.current.margin)
                .fillMaxHeight()
                .fillMaxWidth(),
    ) {
        Column(
            verticalArrangement = Arrangement.Center,
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            Text(
                text = stringResource(id = R.string.empty_feed_top),
                style =
                    MaterialTheme.typography.headlineMedium.merge(
                        TextStyle(fontWeight = FontWeight.Light),
                    ),
                textAlign = TextAlign.Center,
            )
            Spacer(modifier = Modifier.height(16.dp))
            Box(
                contentAlignment = Alignment.Center,
                modifier =
                    Modifier
                        .heightIn(min = TextFieldDefaults.MinHeight)
                        .fillMaxWidth()
                        .clickable {
                            onOpenOtherFeed()
                        },
            ) {
                Text(
                    text = annotatedStringResource(id = R.string.empty_feed_open),
                    style =
                        MaterialTheme.typography.headlineMedium.merge(
                            TextStyle(fontWeight = FontWeight.Light),
                        ),
                    textAlign = TextAlign.Center,
                )
            }
            Spacer(modifier = Modifier.height(16.dp))
            Box(
                contentAlignment = Alignment.Center,
                modifier =
                    Modifier
                        .heightIn(min = TextFieldDefaults.MinHeight)
                        .fillMaxWidth()
                        .clickable {
                            onAddFeed()
                        },
            ) {
                Text(
                    text = annotatedStringResource(id = R.string.empty_feed_add),
                    style =
                        MaterialTheme.typography.headlineMedium.merge(
                            TextStyle(fontWeight = FontWeight.Light),
                        ),
                    textAlign = TextAlign.Center,
                )
            }
        }
    }
}

@Preview(
    name = "Nothing to read day",
    uiMode = Configuration.UI_MODE_NIGHT_NO,
)
@Preview(
    name = "Nothing to read night",
    uiMode = Configuration.UI_MODE_NIGHT_YES,
)
@Composable
private fun PreviewNothingToRead() {
    FeederTheme {
        Surface {
            NothingToRead()
        }
    }
}

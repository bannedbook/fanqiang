package com.nononsenseapps.feeder.ui.compose.feedarticle

import android.util.Log
import androidx.compose.foundation.LocalIndication
import androidx.compose.foundation.clickable
import androidx.compose.foundation.focusGroup
import androidx.compose.foundation.indication
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyListScope
import androidx.compose.foundation.lazy.LazyListState
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.text.selection.SelectionContainer
import androidx.compose.material.ContentAlpha
import androidx.compose.material.LocalContentAlpha
import androidx.compose.material.Surface
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.ErrorOutline
import androidx.compose.material.icons.outlined.Terrain
import androidx.compose.material.icons.outlined.Timelapse
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.RectangleShape
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.res.pluralStringResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.CustomAccessibilityAction
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.customActions
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.testTag
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import coil.compose.AsyncImage
import coil.request.ImageRequest
import coil.size.Precision
import coil.size.Scale
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.archmodel.Enclosure
import com.nononsenseapps.feeder.archmodel.isImage
import com.nononsenseapps.feeder.model.MediaImage
import com.nononsenseapps.feeder.model.ThumbnailImage
import com.nononsenseapps.feeder.ui.compose.coil.RestrainedCropScaling
import com.nononsenseapps.feeder.ui.compose.coil.rememberTintedVectorPainter
import com.nononsenseapps.feeder.ui.compose.components.safeSemantics
import com.nononsenseapps.feeder.ui.compose.text.WithBidiDeterminedLayoutDirection
import com.nononsenseapps.feeder.ui.compose.text.WithTooltipIfNotBlank
import com.nononsenseapps.feeder.ui.compose.text.rememberMaxImageWidth
import com.nononsenseapps.feeder.ui.compose.theme.FeederTheme
import com.nononsenseapps.feeder.ui.compose.theme.LinkTextStyle
import com.nononsenseapps.feeder.ui.compose.theme.LocalDimens
import com.nononsenseapps.feeder.ui.compose.utils.ProvideScaledText
import com.nononsenseapps.feeder.ui.compose.utils.ScreenType
import com.nononsenseapps.feeder.ui.compose.utils.focusableInNonTouchMode
import java.time.format.DateTimeFormatter
import java.time.format.FormatStyle
import java.util.Locale
import kotlin.math.roundToInt

val dateTimeFormat: DateTimeFormatter =
    DateTimeFormatter.ofLocalizedDateTime(FormatStyle.FULL, FormatStyle.SHORT)
        .withLocale(Locale.getDefault())

@Composable
fun ReaderView(
    screenType: ScreenType,
    wordCount: Int,
    onEnclosureClick: () -> Unit,
    onFeedTitleClick: () -> Unit,
    enclosure: Enclosure,
    articleTitle: String,
    feedTitle: String,
    authorDate: String?,
    image: ThumbnailImage?,
    isFeedText: Boolean,
    modifier: Modifier = Modifier,
    articleListState: LazyListState = rememberLazyListState(),
    articleBody: LazyListScope.() -> Unit,
) {
    val dimens = LocalDimens.current

    val readTimeSecs =
        remember(wordCount) {
            wordsToReadTimeSecs(wordCount)
        }

    SelectionContainer {
        LazyColumn(
            state = articleListState,
            contentPadding =
                PaddingValues(
                    bottom = 92.dp,
                    start =
                        when (screenType) {
                            ScreenType.DUAL -> 0.dp // List items have enough padding
                            ScreenType.SINGLE -> dimens.margin
                        },
                    end = dimens.margin,
                ),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(16.dp),
            modifier =
                modifier
                    .fillMaxWidth()
                    .focusGroup()
                    .safeSemantics {
                        testTag = "readerColumn"
                    },
        ) {
            item {
                val goToFeedLabel = stringResource(R.string.go_to_feed, feedTitle)
                Column(
                    verticalArrangement = Arrangement.spacedBy(8.dp),
                    modifier =
                        Modifier
                            .width(dimens.maxReaderWidth)
                            .semantics(mergeDescendants = true) {
                                try {
                                    customActions =
                                        listOf(
                                            // TODO enclosure?
                                            CustomAccessibilityAction(goToFeedLabel) {
                                                onFeedTitleClick()
                                                true
                                            },
                                        )
                                } catch (e: Exception) {
                                    // Observed nullpointer exception when setting customActions
                                    // No clue why it could be null
                                    Log.e("FeederReaderScreen", "Exception in semantics", e)
                                }
                            },
                ) {
                    if (articleTitle.isNotBlank()) {
                        WithBidiDeterminedLayoutDirection(paragraph = articleTitle) {
                            val interactionSource = remember { MutableInteractionSource() }
                            Text(
                                text = articleTitle,
                                style = MaterialTheme.typography.headlineLarge,
                                modifier =
                                    Modifier
                                        .indication(interactionSource, LocalIndication.current)
                                        .focusableInNonTouchMode(interactionSource = interactionSource)
                                        .width(dimens.maxReaderWidth),
                            )
                        }
                    }
                    ProvideScaledText(
                        style =
                            MaterialTheme.typography.titleMedium.merge(
                                LinkTextStyle(),
                            ),
                    ) {
                        WithBidiDeterminedLayoutDirection(paragraph = feedTitle) {
                            Text(
                                text = feedTitle,
                                modifier =
                                    Modifier
                                        .width(dimens.maxReaderWidth)
                                        .clearAndSetSemantics {
                                            contentDescription = feedTitle
                                        }
                                        .clickable {
                                            onFeedTitleClick()
                                        },
                            )
                        }
                    }
                    if (authorDate != null) {
                        ProvideScaledText(style = MaterialTheme.typography.titleMedium) {
                            CompositionLocalProvider(LocalContentAlpha provides ContentAlpha.medium) {
                                WithBidiDeterminedLayoutDirection(paragraph = authorDate) {
                                    val interactionSource = remember { MutableInteractionSource() }
                                    Text(
                                        text = authorDate,
                                        modifier =
                                            Modifier
                                                .width(dimens.maxReaderWidth)
                                                .indication(interactionSource, LocalIndication.current)
                                                .focusableInNonTouchMode(interactionSource = interactionSource),
                                    )
                                }
                            }
                        }
                    }
                    if (readTimeSecs > 0) {
                        ProvideScaledText(style = MaterialTheme.typography.titleMedium) {
                            CompositionLocalProvider(LocalContentAlpha provides ContentAlpha.medium) {
                                Row(
                                    horizontalArrangement = Arrangement.spacedBy(4.dp),
                                    verticalAlignment = Alignment.CenterVertically,
                                    modifier = Modifier.width(dimens.maxReaderWidth),
                                ) {
                                    Icon(
                                        imageVector = Icons.Outlined.Timelapse,
                                        contentDescription = null,
                                    )
                                    val seconds = "%02d".format(readTimeSecs % 60)
                                    val readTimeText =
                                        pluralStringResource(
                                            id = R.plurals.n_minutes,
                                            count = readTimeSecs / 60,
                                        )
                                            .format(
                                                "${readTimeSecs / 60}:$seconds",
                                            )
                                    WithBidiDeterminedLayoutDirection(paragraph = readTimeText) {
                                        val interactionSource =
                                            remember { MutableInteractionSource() }
                                        Text(
                                            text = readTimeText,
                                            modifier =
                                                Modifier
                                                    .weight(1f)
                                                    .indication(
                                                        interactionSource,
                                                        LocalIndication.current,
                                                    )
                                                    .focusableInNonTouchMode(interactionSource = interactionSource),
                                        )
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (enclosure.present) {
                item {
                    // Image will be shown in block below
                    if (!enclosure.isImage) {
                        val openLabel =
                            if (enclosure.name.isBlank()) {
                                stringResource(R.string.open_enclosed_media)
                            } else {
                                stringResource(R.string.open_enclosed_media_file, enclosure.name)
                            }
                        ProvideScaledText(
                            style =
                                MaterialTheme.typography.bodyLarge.merge(
                                    LinkTextStyle(),
                                ),
                        ) {
                            Text(
                                text = openLabel,
                                modifier =
                                    Modifier
                                        .width(dimens.maxReaderWidth)
                                        .clickable {
                                            onEnclosureClick()
                                        }
                                        .clearAndSetSemantics {
                                            try {
                                                customActions =
                                                    listOf(
                                                        CustomAccessibilityAction(openLabel) {
                                                            onEnclosureClick()
                                                            true
                                                        },
                                                    )
                                            } catch (e: Exception) {
                                                // Observed nullpointer exception when setting customActions
                                                // No clue why it could be null
                                                Log.e(
                                                    LOG_TAG,
                                                    "Exception in semantics",
                                                    e,
                                                )
                                            }
                                        },
                            )
                        }
                    }
                }
            }

            // Don't show image for full text articles since it's typically inside the full article
            if (isFeedText && image?.fromBody == false) {
                val imageWidth: Int =
                    if (image.width?.let { it < 640 } == true) {
                        // Zero or too small, do not show
                        -1
                    } else if (image.width != null) {
                        image.width ?: Int.MAX_VALUE
                    } else {
                        // Enclosures do not have a known width. This will be constrained below.
                        Int.MAX_VALUE
                    }

                if (imageWidth > 0) {
                    item {
                        BoxWithConstraints(
                            contentAlignment = Alignment.Center,
                            modifier =
                                Modifier
                                    .clip(RectangleShape)
                                    .width(dimens.maxReaderWidth),
                        ) {
                            WithTooltipIfNotBlank(tooltip = stringResource(id = R.string.article_image)) {
                                val maxImageWidth by rememberMaxImageWidth()
                                val pixelDensity = LocalDensity.current.density
                                val contentScale =
                                    remember(pixelDensity) {
                                        RestrainedCropScaling(pixelDensity)
                                    }
                                AsyncImage(
                                    model =
                                        ImageRequest.Builder(LocalContext.current)
                                            .data(image.url)
                                            .scale(Scale.FIT)
                                            .size(imageWidth.coerceAtMost(maxImageWidth))
                                            .precision(Precision.INEXACT)
                                            .build(),
                                    contentDescription = enclosure.name,
                                    placeholder =
                                        rememberTintedVectorPainter(
                                            Icons.Outlined.Terrain,
                                        ),
                                    error = rememberTintedVectorPainter(Icons.Outlined.ErrorOutline),
                                    contentScale = contentScale,
                                    alignment = Alignment.Center,
                                    modifier =
                                        Modifier
                                            .fillMaxWidth()
                                            .run {
                                                dimens.imageAspectRatioInReader?.let { ratio ->
                                                    aspectRatio(ratio)
                                                } ?: this
                                            },
                                )
                            }
                        }
                    }
                }
            }

            articleBody()
        }
    }
}

@Preview
@Composable
private fun ReaderPreview() {
    FeederTheme {
        Surface {
            ReaderView(
                screenType = ScreenType.SINGLE,
                wordCount = 9700,
                onEnclosureClick = {},
                onFeedTitleClick = {},
                enclosure = Enclosure(),
                articleTitle = "Article title on top",
                feedTitle = "Feed Title is here",
                authorDate = "2018-01-02",
                image = MediaImage("https://cowboyprogrammer.org/images/2017/10/gimp_image_mode_index.png"),
                isFeedText = true,
            ) {}
        }
    }
}

fun wordsToReadTimeSecs(words: Int): Int {
    return (words * 60 / 220.0).roundToInt()
}

private const val LOG_TAG = "FEEDER_READER"

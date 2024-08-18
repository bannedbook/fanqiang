package com.nononsenseapps.feeder.ui.compose.feedarticle

import android.util.Log
import androidx.compose.runtime.Immutable
import androidx.lifecycle.SavedStateHandle
import androidx.lifecycle.viewModelScope
import com.nononsenseapps.feeder.ApplicationCoroutineScope
import com.nononsenseapps.feeder.archmodel.Article
import com.nononsenseapps.feeder.archmodel.Enclosure
import com.nononsenseapps.feeder.archmodel.LinkOpener
import com.nononsenseapps.feeder.archmodel.Repository
import com.nononsenseapps.feeder.archmodel.TextToDisplay
import com.nononsenseapps.feeder.base.DIAwareViewModel
import com.nononsenseapps.feeder.blob.blobFile
import com.nononsenseapps.feeder.blob.blobFullFile
import com.nononsenseapps.feeder.blob.blobFullInputStream
import com.nononsenseapps.feeder.blob.blobInputStream
import com.nononsenseapps.feeder.db.room.FeedItemForFetching
import com.nononsenseapps.feeder.db.room.ID_UNSET
import com.nononsenseapps.feeder.model.FeedParserError
import com.nononsenseapps.feeder.model.FullTextParser
import com.nononsenseapps.feeder.model.LocaleOverride
import com.nononsenseapps.feeder.model.NoBody
import com.nononsenseapps.feeder.model.NoUrl
import com.nononsenseapps.feeder.model.NotHTML
import com.nononsenseapps.feeder.model.PlaybackStatus
import com.nononsenseapps.feeder.model.TTSStateHolder
import com.nononsenseapps.feeder.model.ThumbnailImage
import com.nononsenseapps.feeder.model.UnsupportedContentType
import com.nononsenseapps.feeder.model.html.HtmlLinearizer
import com.nononsenseapps.feeder.model.html.LinearArticle
import com.nononsenseapps.feeder.ui.compose.text.htmlToAnnotatedString
import com.nononsenseapps.feeder.util.Either
import com.nononsenseapps.feeder.util.FilePathProvider
import com.nononsenseapps.feeder.util.logDebug
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.filterNotNull
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import org.kodein.di.DI
import org.kodein.di.instance
import java.io.FileNotFoundException
import java.time.ZonedDateTime
import java.util.Locale

class ArticleViewModel(
    di: DI,
    private val state: SavedStateHandle,
) : DIAwareViewModel(di) {
    private val repository: Repository by instance()
    private val ttsStateHolder: TTSStateHolder by instance()
    private val fullTextParser: FullTextParser by instance()
    private val filePathProvider: FilePathProvider by instance()

    // Use this for actions which should complete even if app goes off screen
    private val applicationCoroutineScope: ApplicationCoroutineScope by instance()

    val itemId: Long =
        state["itemId"]
            ?: throw IllegalArgumentException("Missing itemId in savedState")

    private val articleFlow =
        repository.getArticleFlow(itemId)
            .stateIn(
                scope = viewModelScope,
                started = SharingStarted.Eagerly,
                initialValue = null,
            )

    private val textToDisplay = MutableStateFlow(TextToDisplay.CONTENT)
    private val displayFullTextOverride = MutableStateFlow<Boolean?>(null)

    private val articleContentFlow: StateFlow<LinearArticle> =
        combine(
            articleFlow,
            displayFullTextOverride,
        ) { article, fullTextOverride ->
            article?.let {
                it to (fullTextOverride ?: it.fullTextByDefault)
            }
        }
            .filterNotNull()
            .map { (article, displayFullText) ->
                parseArticleContent(article, displayFullText)
            }
            .stateIn(
                scope = viewModelScope,
                started = SharingStarted.Eagerly,
                initialValue = LinearArticle(emptyList()),
            )

    fun toggleFullText() {
        // Using as general loading text
        textToDisplay.update { TextToDisplay.LOADING_FULLTEXT }
        displayFullTextOverride.value = displayFullTextOverride.value?.not() ?: articleFlow.value?.fullTextByDefault?.not() ?: true
    }

    private val isFullText: Boolean
        get() = displayFullTextOverride.value ?: articleFlow.value?.fullTextByDefault ?: false

    private val toolbarVisible: MutableStateFlow<Boolean> =
        MutableStateFlow(state["toolbarMenuVisible"] ?: false)

    val viewState: StateFlow<ArticleScreenViewState> =
        combine(
            articleFlow,
            textToDisplay,
            articleContentFlow,
            toolbarVisible,
            repository.linkOpener,
            repository.useDetectLanguage,
            ttsStateHolder.ttsState,
            ttsStateHolder.availableLanguages,
        ) { params ->
            val article = params[0] as Article?
            val textToDisplay = params[1] as TextToDisplay
            val articleContent = params[2] as LinearArticle
            val toolbarVisible = params[3] as Boolean
            val linkOpener = params[4] as LinkOpener
            val useDetectLanguage = params[5] as Boolean
            val ttsState = params[6] as PlaybackStatus

            @Suppress("UNCHECKED_CAST")
            val ttsLanguages = params[7] as List<Locale>

            ArticleState(
                useDetectLanguage = useDetectLanguage,
                isBottomBarVisible = ttsState != PlaybackStatus.STOPPED,
                isTTSPlaying = ttsState == PlaybackStatus.PLAYING,
                ttsLanguages = ttsLanguages,
                articleFeedUrl = article?.feedUrl,
                articleId = itemId,
                articleLink = article?.link,
                articleFeedId = article?.feedId ?: ID_UNSET,
                textToDisplay = textToDisplay,
                linkOpener = linkOpener,
                pubDate = article?.pubDate,
                //author = article?.author,
                enclosure = article?.enclosure ?: Enclosure(),
                articleTitle = article?.title ?: "",
                showToolbarMenu = toolbarVisible,
                feedDisplayTitle = article?.feedDisplayTitle ?: "",
                isBookmarked = article?.bookmarked ?: false,
                wordCount =
                    if (isFullText) {
                        article?.wordCountFull ?: 0
                    } else {
                        article?.wordCount ?: 0
                    },
                image = article?.image,
                articleContent = articleContent,
            )
        }
            .stateIn(
                scope = viewModelScope,
                started = SharingStarted.Eagerly,
                initialValue = ArticleState(),
            )

    private suspend fun parseArticleContent(
        article: Article,
        fullText: Boolean,
    ): LinearArticle {
        logDebug(LOG_TAG, "parseArticleContent(${article.id}, $fullText)")
        return try {
            withContext(Dispatchers.IO) {
                val htmlLinearizer = HtmlLinearizer()
                when (fullText) {
                    false -> {
                        if (blobFile(article.id, filePathProvider.articleDir).isFile) {
                            try {
                                blobInputStream(article.id, filePathProvider.articleDir).use {
                                    htmlLinearizer.linearize(
                                        inputStream = it,
                                        baseUrl = article.feedUrl ?: "",
                                    )
                                }.also {
                                    textToDisplay.update { TextToDisplay.CONTENT }
                                }
                            } catch (e: Exception) {
                                // EOFException is possible
                                Log.e(LOG_TAG, "Could not open blob", e)
                                textToDisplay.update { TextToDisplay.FAILED_TO_LOAD_FULLTEXT }
                                LinearArticle(elements = emptyList())
                            }
                        } else {
                            Log.e(LOG_TAG, "No default file to parse")
                            textToDisplay.update { TextToDisplay.FAILED_MISSING_BODY }
                            LinearArticle(elements = emptyList())
                        }
                    }

                    true -> {
                        if (!blobFullFile(article.id, filePathProvider.fullArticleDir).isFile) {
                            // If the fulltext file is missing, try to fetch it
                            when (retrieveFullText(article.id).leftOrNull()) {
                                null -> {
                                    // Success. Do nothing yet
                                    null
                                }
                                is NoBody -> TextToDisplay.FAILED_MISSING_BODY
                                is NoUrl -> TextToDisplay.FAILED_MISSING_LINK
                                is UnsupportedContentType -> TextToDisplay.FAILED_NOT_HTML
                                is NotHTML -> TextToDisplay.FAILED_NOT_HTML
                                else -> TextToDisplay.FAILED_TO_LOAD_FULLTEXT
                            }?.let { errorText ->
                                textToDisplay.update { errorText }
                            }
                        }
                        if (blobFullFile(article.id, filePathProvider.fullArticleDir).isFile) {
                            try {
                                blobFullInputStream(article.id, filePathProvider.fullArticleDir).use {
                                    htmlLinearizer.linearize(
                                        inputStream = it,
                                        baseUrl = article.feedUrl ?: "",
                                    )
                                }.also {
                                    textToDisplay.update { TextToDisplay.CONTENT }
                                }
                            } catch (e: Exception) {
                                // EOFException is possible
                                Log.e(LOG_TAG, "Could not open blob", e)
                                textToDisplay.update { TextToDisplay.FAILED_TO_LOAD_FULLTEXT }
                                LinearArticle(elements = emptyList())
                            }
                        } else {
                            // Error text should already be set above
                            LinearArticle(elements = emptyList())
                        }
                    }
                }
            }
        } catch (t: Throwable) {
            Log.e(LOG_TAG, "Error parsing article", t)
            LinearArticle(elements = emptyList())
        }
    }

    private suspend fun retrieveFullText(itemId: Long): Either<FeedParserError, Unit> =
        withContext(Dispatchers.IO) {
            logDebug(LOG_TAG, "loadFullTextThenDisplayIt($itemId)")
            if (blobFullFile(itemId, filePathProvider.fullArticleDir).isFile) {
                logDebug(LOG_TAG, "Fulltext file exists")
                return@withContext Either.Right(Unit)
            }

            logDebug(LOG_TAG, "Fulltext file does not exist")
            val link = repository.getLink(itemId)
            return@withContext fullTextParser.parseFullArticleIfMissing(
                object : FeedItemForFetching {
                    override val id = itemId
                    override val link = link
                },
            )
        }

    fun markAsUnread() {
        applicationCoroutineScope.launch {
            repository.markAsUnread(itemId)
        }
    }

    fun setBookmarked(bookmarked: Boolean) =
        applicationCoroutineScope.launch {
            repository.setBookmarked(itemId, bookmarked)
        }

    fun setToolbarMenuVisible(visible: Boolean) {
        state["toolbarMenuVisible"] = visible
        toolbarVisible.update { visible }
    }

    fun ttsPlay() {
        viewModelScope.launch(Dispatchers.IO) {
            val article =
                repository.getCurrentArticle()
                    ?: return@launch
            val readFullText = displayFullTextOverride.value ?: article.fullTextByDefault
            val textToRead =
                when (readFullText) {
                    false ->
                        Either.catching(
                            onCatch = {
                                when (it) {
                                    is FileNotFoundException -> TTSFileNotFound
                                    else -> TTSUnknownError
                                }
                            },
                        ) {
                            blobInputStream(article.id, filePathProvider.articleDir).use {
                                htmlToAnnotatedString(
                                    inputStream = it,
                                    baseUrl = article.feedUrl.toString(),
                                )
                            }
                        }

                    true ->
                        Either.catching(
                            onCatch = {
                                when (it) {
                                    is FileNotFoundException -> TTSFileNotFound
                                    else -> TTSUnknownError
                                }
                            },
                        ) {
                            blobFullInputStream(
                                article.id,
                                filePathProvider.fullArticleDir,
                            ).use {
                                htmlToAnnotatedString(
                                    inputStream = it,
                                    baseUrl = article.feedUrl.toString(),
                                )
                            }
                        }
                }

            // TODO show error some message
            textToRead.onRight {
                ttsStateHolder.tts(
                    textArray = it,
                    useDetectLanguage = repository.useDetectLanguage.value,
                )
            }
        }
    }

    fun ttsPause() {
        ttsStateHolder.pause()
    }

    fun ttsStop() {
        ttsStateHolder.stop()
    }

    fun ttsSkipNext() {
        ttsStateHolder.skipNext()
    }

    fun ttsOnSelectLanguage(lang: LocaleOverride) {
        ttsStateHolder.setLanguage(lang)
    }

    companion object {
        private const val LOG_TAG = "FEEDER_ArticleVM"
    }
}

private data class ArticleState(
    override val useDetectLanguage: Boolean = false,
    override val isBottomBarVisible: Boolean = false,
    override val isTTSPlaying: Boolean = false,
    override val ttsLanguages: List<Locale> = emptyList(),
    override val articleFeedUrl: String? = null,
    override val articleId: Long = ID_UNSET,
    override val articleLink: String? = null,
    override val articleFeedId: Long = ID_UNSET,
    override val textToDisplay: TextToDisplay = TextToDisplay.CONTENT,
    override val linkOpener: LinkOpener = LinkOpener.CUSTOM_TAB,
    override val pubDate: ZonedDateTime? = null,
    override val author: String? = null,
    override val enclosure: Enclosure = Enclosure(),
    override val articleTitle: String = "",
    override val showToolbarMenu: Boolean = false,
    override val feedDisplayTitle: String = "",
    override val isBookmarked: Boolean = false,
    override val keyHolder: ArticleItemKeyHolder = RotatingArticleItemKeyHolder,
    override val wordCount: Int = 0,
    override val image: ThumbnailImage? = null,
    override val articleContent: LinearArticle = LinearArticle(emptyList()),
) : ArticleScreenViewState

@Immutable
interface ArticleScreenViewState {
    val useDetectLanguage: Boolean
    val isBottomBarVisible: Boolean
    val isTTSPlaying: Boolean
    val ttsLanguages: List<Locale>
    val articleFeedUrl: String?
    val articleId: Long
    val articleLink: String?
    val articleFeedId: Long
    val textToDisplay: TextToDisplay
    val linkOpener: LinkOpener
    val pubDate: ZonedDateTime?
    val author: String?
    val enclosure: Enclosure
    val articleTitle: String
    val showToolbarMenu: Boolean
    val feedDisplayTitle: String
    val isBookmarked: Boolean
    val keyHolder: ArticleItemKeyHolder
    val wordCount: Int
    val image: ThumbnailImage?
    val articleContent: LinearArticle
}

interface ArticleItemKeyHolder {
    fun getAndIncrementKey(): Any
}

object RotatingArticleItemKeyHolder : ArticleItemKeyHolder {
    private var key: Long = 0L

    override fun getAndIncrementKey(): Long {
        return key++
    }
}

sealed class TSSError

data object TTSFileNotFound : TSSError()

data object TTSUnknownError : TSSError()

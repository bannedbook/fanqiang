package com.nononsenseapps.feeder.model

import android.content.Context
import android.os.Build
import android.os.Bundle
import android.speech.tts.TextToSpeech
import android.speech.tts.UtteranceProgressListener
import android.util.Log
import android.view.textclassifier.TextClassificationManager
import android.view.textclassifier.TextLanguage
import android.widget.Toast
import androidx.annotation.RequiresApi
import androidx.compose.runtime.Immutable
import androidx.compose.ui.text.AnnotatedString
import com.nononsenseapps.feeder.R
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlinx.coroutines.withContext
import java.util.Locale

/**
 * Any callers must call #shutdown when shutting down
 */
class TTSStateHolder(
    val context: Context,
    val coroutineScope: CoroutineScope,
) : TextToSpeech.OnInitListener {
    private val mutex: Mutex = Mutex()
    private var textToSpeech: TextToSpeech? = null
    private val speechListener: UtteranceProgressListener by lazy {
        object : UtteranceProgressListener() {
            override fun onDone(utteranceId: String) {
                textToSpeechQueue.removeFirstOrNull()
                if (textToSpeechQueue.isEmpty()) {
                    coroutineScope.launch {
                        delay(100)
                        if (textToSpeechQueue.isEmpty()) {
                            // If still empty after the delay
                            _ttsState.value = PlaybackStatus.STOPPED
                        }
                    }
                } else {
                    speakNext()
                }
            }

            override fun onStart(utteranceId: String) {
            }

            override fun onError(
                utteranceId: String?,
                errorCode: Int,
            ) {
                Log.e(LOG_TAG, "onError utteranceId $utteranceId, errorCode $errorCode")

                if (utteranceId != null) {
                    textToSpeechQueue.removeFirstOrNull()
                }
            }

            @Deprecated(
                "Deprecated in super class",
                replaceWith = ReplaceWith("onError(utteranceId, errorCode)"),
            )
            override fun onError(utteranceId: String) {
                Log.e(LOG_TAG, "onError utteranceId $utteranceId")
                textToSpeechQueue.removeFirstOrNull()
            }
        }
    }
    private val textToSpeechQueue = mutableListOf<CharSequence>()
    private var initializedState: Int? = null
    private var startJob: Job? = null

    private var useDetectLanguage: Boolean = true

    private val _ttsState = MutableStateFlow(PlaybackStatus.STOPPED)
    val ttsState: StateFlow<PlaybackStatus> = _ttsState.asStateFlow()

    @Suppress("ktlint:standard:property-naming")
    private val _lang = MutableStateFlow<LocaleOverride>(AppSetting)
    val language: StateFlow<LocaleOverride> = _lang.asStateFlow()

    private val _availableLanguages = MutableStateFlow<List<Locale>>(emptyList())
    val availableLanguages: StateFlow<List<Locale>> = _availableLanguages.asStateFlow()

    private var allAvailableLanguages: Set<Locale> = emptySet()

    private fun speakNext() {
        textToSpeechQueue.firstOrNull()?.let { text ->
            val lang = _lang.value
            val localesToUse: Sequence<Locale> =
                when {
                    lang is ForcedAuto && Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q -> {
                        context.detectLocaleFromText(text)
                            .sortedByDescending { it.confidence }
                            .map { it.locale }
                            .plus(_availableLanguages.value)
                    }
                    lang is ForcedLocale -> {
                        sequenceOf(
                            lang.locale,
                        )
                    }
                    else -> {
                        // Use app setting
                        if (useDetectLanguage && Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                            context.detectLocaleFromText(text)
                                .sortedByDescending { it.confidence }
                                .map { it.locale }
                                .plus(_availableLanguages.value)
                        } else {
                            // User has requested un-dynamic so for context locales first
                            context.getLocales()
                                .plus(_availableLanguages.value)
                        }
                    }
                }

            setFirstBestLocale(localesToUse)

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                textToSpeech?.speak(
                    text,
                    TextToSpeech.QUEUE_ADD,
                    null,
                    "0",
                )
            } else {
                textToSpeech?.speak(
                    text,
                    TextToSpeech.QUEUE_ADD,
                    Bundle.EMPTY,
                    "0",
                )
            }
        }
    }

    fun tts(
        textArray: List<AnnotatedString>,
        useDetectLanguage: Boolean,
    ) {
        this.useDetectLanguage = useDetectLanguage
//        val textArray = fullText.split(*PUNCTUATION)
        for (text in textArray) {
            if (text.isBlank()) {
                continue
            }
            textToSpeechQueue.add(text)
        }
        play()
    }

    fun play() {
        startJob?.cancel()
        startJob =
            coroutineScope.launch {
                if (mutex.isLocked) {
                    // Oops, I was double clicked
                    return@launch
                }
                mutex.withLock {
                    if (textToSpeech == null) {
                        initializedState = null
                        textToSpeech =
                            TextToSpeech(
                                context,
                                this@TTSStateHolder,
                            )
                    }
                    while (initializedState == null) {
                        delay(100)
                    }
                    if (initializedState != TextToSpeech.SUCCESS) {
                        withContext(Dispatchers.Main) {
                            Toast.makeText(
                                context,
                                R.string.failed_to_load_text_to_speech,
                                Toast.LENGTH_SHORT,
                            )
                                .show()
                        }
                        return@launch
                    }
                    _ttsState.value = PlaybackStatus.PLAYING

                    // Can only set this once engine has been initialized
                    textToSpeech?.setOnUtteranceProgressListener(speechListener)
                    try {
                        updateAvailableLanguages()
                        speakNext()
                    } catch (e: ConcurrentModificationException) {
                        Log.e(LOG_TAG, "User probably double clicked play", e)
                        // State will be weird. But mutex should prevent it happening
                    }
                }
            }
    }

    fun pause() {
        startJob?.cancel()
        textToSpeech?.stop()
        _ttsState.value = PlaybackStatus.PAUSED
    }

    fun stop() {
        startJob?.cancel()
        textToSpeech?.stop()
        textToSpeechQueue.clear()
        _ttsState.value = PlaybackStatus.STOPPED
        textToSpeech = null
    }

    fun skipNext() {
        coroutineScope.launch {
            startJob?.cancel()
            textToSpeech?.stop()
            startJob?.join()
            textToSpeechQueue.removeFirstOrNull()
            when (textToSpeechQueue.isEmpty()) {
                true -> stop()
                false -> play()
            }
        }
    }

    fun setLanguage(lang: LocaleOverride) {
        coroutineScope.launch {
            startJob?.cancel()
            textToSpeech?.stop()
            startJob?.join()
            _lang.update { lang }
            play()
        }
    }

    override fun onInit(status: Int) {
        initializedState = status

        if (status != TextToSpeech.SUCCESS) {
            Log.e(LOG_TAG, "Failed to load TextToSpeech object: $status")
        }
    }

    fun updateAvailableLanguages() {
        allAvailableLanguages = textToSpeech?.availableLanguages ?: emptySet()

        val sortedLanguages =
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                context.detectLocaleFromText(
                    textToSpeechQueue.joinToString("\n\n"),
                    minConfidence = 0f,
                )
                    .sortedByDescending { it.confidence }
                    .map { it.locale }
                    .plus(
                        context.getLocales()
                            .sortedBy { it.getDisplayName(it).lowercase(it) },
                    )
                    .plus(
                        allAvailableLanguages.asSequence()
                            .sortedBy { it.getDisplayName(it).lowercase(it) },
                    )
            } else {
                context.getLocales()
                    .sortedBy { it.displayName }
                    .plus(
                        allAvailableLanguages.asSequence()
                            .sortedBy { it.getDisplayName(it).lowercase(it) },
                    )
            }
                .distinctBy { it.toLanguageTag() }
                .toList()

        _availableLanguages.update {
            sortedLanguages
        }
    }

    fun setFirstBestLocale(localesToUse: Sequence<Locale>) {
        val selectedLocale =
            localesToUse
                .firstOrNull { locale ->
                    when (textToSpeech?.setLanguage(locale)) {
                        TextToSpeech.SUCCESS -> {
                            true
                        }
                        else -> {
                            // In this case, try without region because the TTS engine lies about
                            // what locales are available
                            when (textToSpeech?.setLanguage(Locale.forLanguageTag(locale.language))) {
                                TextToSpeech.SUCCESS -> {
                                    true
                                } else -> {
                                    Log.e(LOG_TAG, "${locale.toLanguageTag()} is not supported")
                                    false
                                }
                            }
                        }
                    }
                }

        if (selectedLocale == null) {
            Log.e(LOG_TAG, "None of the locales were supported by text to speech")
        }
    }

    fun shutdown() {
        textToSpeech?.shutdown()
    }

    companion object {
        private const val LOG_TAG = "FeederTextToSpeech"
        private val PUNCTUATION =
            arrayOf(
                // New-lines
                "\n",
                // Very useful: https://unicodelookup.com/
                // Full stop
                ".",
                "։",
                "۔",
                "܁",
                "܂",
                "。",
                "︒",
                "﹒",
                "．",
                "｡",
                // Question mark
                "?",
                ";",
                "՞",
                "؟",
                "⁇",
                "⁈",
                "⁉",
                "︖",
                "﹖",
                "？",
                // Exclamation mark
                "!",
                "՜",
                "‼",
                "︕",
                "﹗",
                "！",
                // Colon and semi-colon
                ":",
                ";",
                "؛",
                "︓",
                "︔",
                "﹔",
                "﹕",
                "：",
                "；",
                // Ellipsis
                "...",
                "…",
                "⋯",
                "⋮",
                "︙",
                // Dash
                "—",
                "〜",
                "〰",
            )
    }
}

fun Context.getLocales(): Sequence<Locale> =
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
        sequence {
            val locales = resources.configuration.locales

            for (i in 0 until locales.size()) {
                yield(locales[i])
            }
        }
    } else {
        @Suppress("DEPRECATION")
        sequenceOf(resources.configuration.locale)
    }

@RequiresApi(Build.VERSION_CODES.Q)
fun Context.detectLocaleFromText(
    text: CharSequence,
    minConfidence: Float = 80.0f,
): Sequence<LocaleWithConfidence> {
    val textClassificationManager = getSystemService(TextClassificationManager::class.java)
    val textClassifier = textClassificationManager.textClassifier

    val textRequest = TextLanguage.Request.Builder(text).build()
    val detectedLanguage = textClassifier.detectLanguage(textRequest)

    return sequence {
        for (i in 0 until detectedLanguage.localeHypothesisCount) {
            val localeDetected = detectedLanguage.getLocale(i)
            val confidence = detectedLanguage.getConfidenceScore(localeDetected) * 100.0f
            if (confidence >= minConfidence) {
                yield(
                    LocaleWithConfidence(
                        locale = localeDetected.toLocale(),
                        confidence = confidence,
                    ),
                )
            }
        }
    }
}

data class LocaleWithConfidence(
    val locale: Locale,
    val confidence: Float,
)

enum class PlaybackStatus {
    STOPPED,
    PAUSED,
    PLAYING,
}

@Immutable
sealed class LocaleOverride

@Immutable
object AppSetting : LocaleOverride()

@Immutable
object ForcedAuto : LocaleOverride()

@Immutable
data class ForcedLocale(val locale: Locale) : LocaleOverride()

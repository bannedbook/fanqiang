package com.nononsenseapps.feeder.ui.compose.searchfeed

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.nononsenseapps.feeder.archmodel.Repository
import com.nononsenseapps.feeder.base.DIAwareViewModel
import com.nononsenseapps.feeder.model.FeedParser
import com.nononsenseapps.feeder.model.FeedParserError
import com.nononsenseapps.feeder.model.HttpError
import com.nononsenseapps.feeder.model.NoAlternateFeeds
import com.nononsenseapps.feeder.model.NotInitializedYet
import com.nononsenseapps.feeder.model.SiteMetaData
import com.nononsenseapps.feeder.util.Either
import com.nononsenseapps.feeder.util.flatMap
import com.nononsenseapps.feeder.util.sloppyLinkToStrictURLOrNull
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.flow.flowOn
import kotlinx.coroutines.flow.map
import org.kodein.di.DI
import org.kodein.di.instance
import java.net.URL
import java.time.Instant

class SearchFeedViewModel(di: DI) : DIAwareViewModel(di) {
    private val feedParser: FeedParser by instance()
    private val repository: Repository by instance()

    private var siteMetaData: Either<FeedParserError, SiteMetaData> by mutableStateOf(
        Either.Left(
            NotInitializedYet,
        ),
    )

    fun searchForFeeds(initialUrl: URL): Flow<Either<FeedParserError, SearchResult>> {
        return flow {
            siteMetaData = feedParser.getSiteMetaData(initialUrl)
            // Flow collection makes this block concurrent with map below
            val initialSiteMetaData = siteMetaData

            initialSiteMetaData
                .onRight { metaData ->
                    metaData.alternateFeedLinks.forEach {
                        emit(Either.Right(it.link))
                    }
                    if (metaData.alternateFeedLinks.isEmpty()) {
                        emit(Either.Left(NoAlternateFeeds(initialUrl.toString())))
                    }
                }
                .onLeft {
                    if (it is HttpError) {
                        handleHttpError(it)
                    }
                    emit(Either.Right(initialUrl))
                }
        }
            .map { urlResult ->
                urlResult.flatMap { url ->
                    feedParser.parseFeedUrl(url)
                        .onRight { feed ->
                            if (siteMetaData.isLeft()) {
                                feed.home_page_url?.let { pageLink ->
                                    sloppyLinkToStrictURLOrNull(pageLink)?.let { pageUrl ->
                                        siteMetaData = feedParser.getSiteMetaData(pageUrl)
                                    }
                                }
                            }
                        }
                        .map { feed ->
                            SearchResult(
                                title = feed.title ?: "",
                                url = feed.feed_url ?: url.toString(),
                                description = feed.description ?: "",
                                feedImage = siteMetaData.getOrNull()?.feedImage ?: "",
                            )
                        }
                        .onLeft {
                            if (it is HttpError) {
                                handleHttpError(it)
                            }
                        }
                }
            }
            .flowOn(Dispatchers.Default)
    }

    private suspend fun handleHttpError(httpError: HttpError) {
        httpError.retryAfterSeconds?.let { retryAfterSeconds ->
            repository.setRetryAfterForFeedsWithBaseUrl(
                host = URL(httpError.url).host,
                retryAfter = Instant.now().plusSeconds(retryAfterSeconds),
            )
        }
    }

    companion object {
        const val LOG_TAG = "FEEDER_SearchFeed"
    }
}

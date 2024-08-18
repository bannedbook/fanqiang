package com.nononsenseapps.feeder.di

import com.nononsenseapps.feeder.archmodel.FeedItemStore
import com.nononsenseapps.feeder.archmodel.FeedStore
import com.nononsenseapps.feeder.archmodel.Repository
import com.nononsenseapps.feeder.archmodel.SessionStore
import com.nononsenseapps.feeder.archmodel.SettingsStore
import com.nononsenseapps.feeder.archmodel.SyncRemoteStore
import com.nononsenseapps.feeder.base.bindWithActivityViewModelScope
import com.nononsenseapps.feeder.base.bindWithComposableViewModelScope
import com.nononsenseapps.feeder.model.OPMLParserHandler
import com.nononsenseapps.feeder.model.opml.OPMLImporter
import com.nononsenseapps.feeder.ui.CommonActivityViewModel
import com.nononsenseapps.feeder.ui.MainActivityViewModel
import com.nononsenseapps.feeder.ui.NavigationDeepLinkViewModel
import com.nononsenseapps.feeder.ui.OpenLinkInDefaultActivityViewModel
import com.nononsenseapps.feeder.ui.compose.editfeed.CreateFeedScreenViewModel
import com.nononsenseapps.feeder.ui.compose.editfeed.EditFeedScreenViewModel
import com.nononsenseapps.feeder.ui.compose.feedarticle.ArticleViewModel
import com.nononsenseapps.feeder.ui.compose.feedarticle.FeedViewModel
import com.nononsenseapps.feeder.ui.compose.searchfeed.SearchFeedViewModel
import com.nononsenseapps.feeder.ui.compose.settings.SettingsViewModel
import org.kodein.di.DI
import org.kodein.di.bind
import org.kodein.di.singleton

val archModelModule =
    DI.Module(name = "arch models") {
        bind<Repository>() with singleton { Repository(di) }
        bind<SessionStore>() with singleton { SessionStore() }
        bind<SettingsStore>() with singleton { SettingsStore(di) }
        bind<FeedStore>() with singleton { FeedStore(di) }
        bind<FeedItemStore>() with singleton { FeedItemStore(di) }
        bind<SyncRemoteStore>() with singleton { SyncRemoteStore(di) }
        bind<OPMLParserHandler>() with singleton { OPMLImporter(di) }

        bindWithActivityViewModelScope<MainActivityViewModel>()
        bindWithActivityViewModelScope<OpenLinkInDefaultActivityViewModel>()
        bindWithActivityViewModelScope<CommonActivityViewModel>()

        bindWithComposableViewModelScope<SettingsViewModel>()
        bindWithComposableViewModelScope<EditFeedScreenViewModel>()
        bindWithComposableViewModelScope<CreateFeedScreenViewModel>()
        bindWithComposableViewModelScope<SearchFeedViewModel>()
        bindWithComposableViewModelScope<ArticleViewModel>()
        bindWithComposableViewModelScope<FeedViewModel>()
        bindWithComposableViewModelScope<NavigationDeepLinkViewModel>()
    }

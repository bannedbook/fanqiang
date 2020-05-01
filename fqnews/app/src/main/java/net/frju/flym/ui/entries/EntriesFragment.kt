/*
 * Copyright (c) 2012-2018 Frederic Julian
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http:></http:>//www.gnu.org/licenses/>.
 */

package net.frju.flym.ui.entries

import SpeedUpVPN.VpnEncrypt
import android.annotation.SuppressLint
import android.content.Intent
import android.content.SharedPreferences.OnSharedPreferenceChangeListener
import android.os.Bundle
import android.os.Handler
import android.text.TextUtils
import android.util.Log
import android.view.*
import android.widget.*
import androidx.appcompat.widget.SearchView
import androidx.core.view.isGone
import androidx.core.view.isVisible
import androidx.fragment.app.Fragment
import androidx.lifecycle.LiveData
import androidx.lifecycle.Observer
import androidx.lifecycle.lifecycleScope
import androidx.paging.LivePagedListBuilder
import androidx.paging.PagedList
import androidx.paging.PagedListAdapter
import androidx.recyclerview.widget.*
import com.bumptech.glide.load.resource.drawable.DrawableTransitionOptions
import com.bumptech.glide.request.transition.DrawableCrossFadeFactory
import com.github.shadowsocks.utils.printLog
import com.google.android.gms.ads.*
import com.google.android.gms.ads.formats.NativeAdOptions
import com.google.android.gms.ads.formats.UnifiedNativeAd
import com.google.android.gms.ads.formats.UnifiedNativeAdView
import kotlinx.android.synthetic.main.fragment_entries.*
import kotlinx.android.synthetic.main.view_entry.view.*
import kotlinx.android.synthetic.main.view_main_containers.*
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import net.fred.feedex.R
import net.frju.flym.App
import net.frju.flym.GlideApp
import net.frju.flym.data.entities.EntryWithFeed
import net.frju.flym.data.entities.Feed
import net.frju.flym.data.utils.PrefConstants
import net.frju.flym.service.FetcherService
import net.frju.flym.ui.main.MainNavigator
import net.frju.flym.utils.closeKeyboard
import net.frju.flym.utils.getPrefBoolean
import net.frju.flym.utils.registerOnPrefChangeListener
import net.frju.flym.utils.unregisterOnPrefChangeListener
import org.jetbrains.anko.*
import org.jetbrains.anko.appcompat.v7.titleResource
import org.jetbrains.anko.design.longSnackbar
import org.jetbrains.anko.sdk21.listeners.onClick
import org.jetbrains.anko.sdk21.listeners.onLongClick
import org.jetbrains.anko.support.v4.dip
import org.jetbrains.anko.support.v4.share
import q.rorbin.badgeview.Badge
import q.rorbin.badgeview.QBadgeView
import java.util.*


class EntriesFragment : Fragment() {

	companion object {
		@JvmField
		val DIFF_CALLBACK = object : DiffUtil.ItemCallback<EntryWithFeed>() {

			override fun areItemsTheSame(oldItem: EntryWithFeed, newItem: EntryWithFeed): Boolean =
					oldItem.entry.id == newItem.entry.id

			override fun areContentsTheSame(oldItem: EntryWithFeed, newItem: EntryWithFeed): Boolean =
					oldItem.entry.id == newItem.entry.id && oldItem.entry.read == newItem.entry.read && oldItem.entry.favorite == newItem.entry.favorite // no need to do more complex in our case
		}

		@JvmField
		val CROSS_FADE_FACTORY: DrawableCrossFadeFactory = DrawableCrossFadeFactory.Builder().setCrossFadeEnabled(true).build()

		private const val ARG_FEED = "ARG_FEED"
		private const val STATE_FEED = "STATE_FEED"
		private const val STATE_SEARCH_TEXT = "STATE_SEARCH_TEXT"
		private const val STATE_SELECTED_ENTRY_ID = "STATE_SELECTED_ENTRY_ID"
		private const val STATE_LIST_DISPLAY_DATE = "STATE_LIST_DISPLAY_DATE"

		fun newInstance(feed: Feed?): EntriesFragment {
			return EntriesFragment().apply {
				feed?.let {
					arguments = bundleOf(ARG_FEED to feed)
				}
			}
		}
	}
	private lateinit var mInterstitialAd: InterstitialAd
	private var layoutManager : LinearLayoutManager? = null
    private var nativeAd: UnifiedNativeAd? = null
    private var nativeAdView: UnifiedNativeAdView? = null
    private var adHost: ViewHolder? = null
	private fun tryBindAd() = lifecycleScope.launchWhenStarted {
		if(layoutManager==null)return@launchWhenStarted
		try {
			val fp = layoutManager!!.findFirstVisibleItemPosition()

			if (fp < 0) return@launchWhenStarted
			for (i in object : Iterator<Int> {
				var first = fp
				var last = layoutManager!!.findLastCompletelyVisibleItemPosition()
				var flipper = false
				override fun hasNext() = first <= last
				override fun next(): Int {
					flipper = !flipper
					return if (flipper) first++ else last--
				}
			}.asSequence().toList().reversed()) {
				try {
					var viewHolder = recycler_view.findViewHolderForAdapterPosition(i)
					if(viewHolder==null)continue
					viewHolder = viewHolder as ViewHolder
					if (i<9) {
						viewHolder.populateUnifiedNativeAdView(nativeAd!!, nativeAdView!!)
						// might be in the middle of a layout after scrolling, need to wait
						withContext(Dispatchers.Main) { adapter.notifyItemChanged(i) }
						break
					}
				}catch (ex:Exception){
					Log.e("ssvpn",ex.message,ex)
					printLog(ex)
					continue
				}
			}
		}catch (e:Exception){
			Log.e("ssvpn",e.message,e)
			printLog(e)
		}
	}

	var feed: Feed? = null
		set(value) {
			field = value

			setupTitle()
			bottom_navigation.post { initDataObservers() } // Needed to retrieve the correct selected tab position
		}

	private val navigator: MainNavigator by lazy { activity as MainNavigator }

	private val adapter = EntryAdapter({ entryWithFeed ->
		navigator.goToEntryDetails(entryWithFeed.entry.id, entryIds!!)
	}, { entryWithFeed ->
		share(entryWithFeed.entry.link.orEmpty(), entryWithFeed.entry.title.orEmpty())
	}, { entryWithFeed, view ->
		entryWithFeed.entry.favorite = !entryWithFeed.entry.favorite

		view.favorite_icon?.let {
			if (entryWithFeed.entry.favorite) {
				it.setImageResource(R.drawable.ic_star_24dp)
			} else {
				it.setImageResource(R.drawable.ic_star_border_24dp)
			}
		}

		doAsync {
			if (entryWithFeed.entry.favorite) {
				App.db.entryDao().markAsFavorite(entryWithFeed.entry.id)
			} else {
				App.db.entryDao().markAsNotFavorite(entryWithFeed.entry.id)
			}
		}
	})
	private var listDisplayDate = Date().time
	private var entriesLiveData: LiveData<PagedList<EntryWithFeed>>? = null
	private var entryIdsLiveData: LiveData<List<String>>? = null
	private var entryIds: List<String>? = null
	private var newCountLiveData: LiveData<Long>? = null
	private var unreadBadge: Badge? = null
	private var searchText: String? = null
	private val searchHandler = Handler()

	private val prefListener = OnSharedPreferenceChangeListener { sharedPreferences, key ->
		if (PrefConstants.IS_REFRESHING == key) {
			refreshSwipeProgress()
		}
	}

	init {
		setHasOptionsMenu(true)
	}

	override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? =
			inflater.inflate(R.layout.fragment_entries, container, false)

	override fun onActivityCreated(savedInstanceState: Bundle?) {
		super.onActivityCreated(savedInstanceState)

		if (savedInstanceState != null) {
			feed = savedInstanceState.getParcelable(STATE_FEED)
			adapter.selectedEntryId = savedInstanceState.getString(STATE_SELECTED_ENTRY_ID)
			listDisplayDate = savedInstanceState.getLong(STATE_LIST_DISPLAY_DATE)
			searchText = savedInstanceState.getString(STATE_SEARCH_TEXT)
		} else {
			feed = arguments?.getParcelable(ARG_FEED)
		}

		setupRecyclerView()

		bottom_navigation.setOnNavigationItemSelectedListener {
			recycler_view.post {
				listDisplayDate = Date().time
				initDataObservers()
				recycler_view.scrollToPosition(0)
			}

			activity?.toolbar?.menu?.findItem(R.id.menu_entries__share)?.isVisible = it.itemId == R.id.favorites
			true
		}

		unreadBadge = QBadgeView(context).bindTarget((bottom_navigation.getChildAt(0) as ViewGroup).getChildAt(0)).apply {
			setGravityOffset(35F, 0F, true)
			isShowShadow = false
			badgeBackgroundColor = requireContext().colorAttr(R.attr.colorAccent)
		}

		read_all_fab.onClick { _ ->
			entryIds?.let { entryIds ->
				if (entryIds.isNotEmpty()) {
					doAsync {
						// TODO check if limit still needed
						entryIds.withIndex().groupBy { it.index / 300 }.map { pair -> pair.value.map { it.value } }.forEach {
							App.db.entryDao().markAsRead(it)
						}
					}

					coordinator.longSnackbar(R.string.marked_as_read, R.string.undo) { _ ->
						doAsync {
							// TODO check if limit still needed
							entryIds.withIndex().groupBy { it.index / 300 }.map { pair -> pair.value.map { it.value } }.forEach {
								App.db.entryDao().markAsUnread(it)
							}

							uiThread {
								// we need to wait for the list to be empty before displaying the new items (to avoid scrolling issues)
								listDisplayDate = Date().time
								initDataObservers()
							}
						}
					}
				}

				if (feed == null || feed?.id == Feed.ALL_ENTRIES_ID) {
					activity?.notificationManager?.cancel(0)
				}
			}
		}

		MobileAds.initialize(activity,"ca-app-pub-2194043486084479~7068327579")
		mInterstitialAd = InterstitialAd(activity)
		mInterstitialAd.adUnitId = "ca-app-pub-2194043486084479/2288264870"
	}

	private fun initDataObservers() {
		entryIdsLiveData?.removeObservers(this)
		entryIdsLiveData = when {
			searchText != null -> App.db.entryDao().observeIdsBySearch(searchText!!)
			feed?.isGroup == true && bottom_navigation.selectedItemId == R.id.unreads -> App.db.entryDao().observeUnreadIdsByGroup(feed!!.id, listDisplayDate)
			feed?.isGroup == true && bottom_navigation.selectedItemId == R.id.favorites -> App.db.entryDao().observeFavoriteIdsByGroup(feed!!.id, listDisplayDate)
			feed?.isGroup == true -> App.db.entryDao().observeIdsByGroup(feed!!.id, listDisplayDate)

			feed != null && feed?.id != Feed.ALL_ENTRIES_ID && bottom_navigation.selectedItemId == R.id.unreads -> App.db.entryDao().observeUnreadIdsByFeed(feed!!.id, listDisplayDate)
			feed != null && feed?.id != Feed.ALL_ENTRIES_ID && bottom_navigation.selectedItemId == R.id.favorites -> App.db.entryDao().observeFavoriteIdsByFeed(feed!!.id, listDisplayDate)
			feed != null && feed?.id != Feed.ALL_ENTRIES_ID -> App.db.entryDao().observeIdsByFeed(feed!!.id, listDisplayDate)

			bottom_navigation.selectedItemId == R.id.unreads -> App.db.entryDao().observeAllUnreadIds(listDisplayDate)
			bottom_navigation.selectedItemId == R.id.favorites -> App.db.entryDao().observeAllFavoriteIds(listDisplayDate)
			else -> App.db.entryDao().observeAllIds(listDisplayDate)
		}

		entryIdsLiveData?.observe(this, Observer<List<String>> { list ->
			entryIds = list
		})

		entriesLiveData?.removeObservers(this)
		entriesLiveData = LivePagedListBuilder(when {
			searchText != null -> App.db.entryDao().observeSearch(searchText!!)
			feed?.isGroup == true && bottom_navigation.selectedItemId == R.id.unreads -> App.db.entryDao().observeUnreadsByGroup(feed!!.id, listDisplayDate)
			feed?.isGroup == true && bottom_navigation.selectedItemId == R.id.favorites -> App.db.entryDao().observeFavoritesByGroup(feed!!.id, listDisplayDate)
			feed?.isGroup == true -> App.db.entryDao().observeByGroup(feed!!.id, listDisplayDate)

			feed != null && feed?.id != Feed.ALL_ENTRIES_ID && bottom_navigation.selectedItemId == R.id.unreads -> App.db.entryDao().observeUnreadsByFeed(feed!!.id, listDisplayDate)
			feed != null && feed?.id != Feed.ALL_ENTRIES_ID && bottom_navigation.selectedItemId == R.id.favorites -> App.db.entryDao().observeFavoritesByFeed(feed!!.id, listDisplayDate)
			feed != null && feed?.id != Feed.ALL_ENTRIES_ID -> App.db.entryDao().observeByFeed(feed!!.id, listDisplayDate)

			bottom_navigation.selectedItemId == R.id.unreads -> App.db.entryDao().observeAllUnreads(listDisplayDate)
			bottom_navigation.selectedItemId == R.id.favorites -> App.db.entryDao().observeAllFavorites(listDisplayDate)
			else -> App.db.entryDao().observeAll(listDisplayDate)
		}, 30).build()

		entriesLiveData?.observe(this, Observer<PagedList<EntryWithFeed>> { pagedList ->
			adapter.submitList(pagedList)
		})

		newCountLiveData?.removeObservers(this)
		newCountLiveData = when {
			feed?.isGroup == true -> App.db.entryDao().observeNewEntriesCountByGroup(feed!!.id, listDisplayDate)
			feed != null && feed?.id != Feed.ALL_ENTRIES_ID -> App.db.entryDao().observeNewEntriesCountByFeed(feed!!.id, listDisplayDate)
			else -> App.db.entryDao().observeNewEntriesCount(listDisplayDate)
		}

		newCountLiveData?.observe(this, Observer<Long> { count ->
			if (count != null && count > 0L) {
				// If we have an empty list, let's immediately display the new items
				if (entryIds?.isEmpty() == true && bottom_navigation.selectedItemId != R.id.favorites) {
					listDisplayDate = Date().time
					initDataObservers()
				} else {
					unreadBadge?.badgeNumber = count.toInt()
				}
			} else {
				unreadBadge?.hide(false)
			}
		})
	}

	override fun onStart() {
		super.onStart()
		context?.registerOnPrefChangeListener(prefListener)
		refreshSwipeProgress()
	}

	override fun onStop() {
		super.onStop()
		context?.unregisterOnPrefChangeListener(prefListener)
	}

	override fun onSaveInstanceState(outState: Bundle) {
		outState.putParcelable(STATE_FEED, feed)
		outState.putString(STATE_SELECTED_ENTRY_ID, adapter.selectedEntryId)
		outState.putLong(STATE_LIST_DISPLAY_DATE, listDisplayDate)
		outState.putString(STATE_SEARCH_TEXT, searchText)

		super.onSaveInstanceState(outState)
	}

	private fun setupRecyclerView() {
		val animator = DefaultItemAnimator()
		animator.supportsChangeAnimations = false // prevent fading-in/out when rebinding
		recycler_view.itemAnimator = animator

		//recycler_view.setHasFixedSize(true)

		layoutManager = LinearLayoutManager(activity)
		recycler_view.addItemDecoration(DividerItemDecoration(context, layoutManager!!.orientation))
		recycler_view.layoutManager = layoutManager
		recycler_view.adapter = adapter

		refresh_layout.setColorScheme(R.color.colorAccent,
				requireContext().attr(R.attr.colorPrimaryDark).resourceId,
				R.color.colorAccent,
				requireContext().attr(R.attr.colorPrimaryDark).resourceId)

		refresh_layout.setOnRefreshListener {
			startRefresh()
		}

		val itemTouchHelperCallback = object : ItemTouchHelper.SimpleCallback(0, ItemTouchHelper.LEFT or ItemTouchHelper.RIGHT) {
			private val VELOCITY = dip(800).toFloat()

			override fun onMove(recyclerView: RecyclerView, viewHolder: RecyclerView.ViewHolder, target: RecyclerView.ViewHolder): Boolean {
				return false
			}

			override fun getSwipeEscapeVelocity(defaultValue: Float): Float {
				return VELOCITY
			}

			override fun getSwipeVelocityThreshold(defaultValue: Float): Float {
				return VELOCITY
			}

			override fun onSwiped(viewHolder: RecyclerView.ViewHolder, direction: Int) {
				adapter.currentList?.get(viewHolder.adapterPosition)?.let { entryWithFeed ->
					entryWithFeed.entry.read = !entryWithFeed.entry.read
					doAsync {
						if (entryWithFeed.entry.read) {
							App.db.entryDao().markAsRead(listOf(entryWithFeed.entry.id))
						} else {
							App.db.entryDao().markAsUnread(listOf(entryWithFeed.entry.id))
						}

						coordinator.longSnackbar(R.string.marked_as_read, R.string.undo) { _ ->
							doAsync {
								if (entryWithFeed.entry.read) {
									App.db.entryDao().markAsUnread(listOf(entryWithFeed.entry.id))
								} else {
									App.db.entryDao().markAsRead(listOf(entryWithFeed.entry.id))
								}
							}
						}

						if (bottom_navigation.selectedItemId != R.id.unreads) {
							uiThread {
								adapter.notifyItemChanged(viewHolder.adapterPosition)
							}
						}
					}
				}
			}
		}

		// attaching the touch helper to recycler view
		ItemTouchHelper(itemTouchHelperCallback).attachToRecyclerView(recycler_view)

		recycler_view.emptyView = empty_view

		recycler_view.addOnScrollListener(object : RecyclerView.OnScrollListener() {
			override fun onScrollStateChanged(recyclerView: RecyclerView, newState: Int) {
				super.onScrollStateChanged(recyclerView, newState)
				activity?.closeKeyboard()
			}
		})
	}

	private fun startRefresh() {
		if (context?.getPrefBoolean(PrefConstants.IS_REFRESHING, false) == false) {
			if (feed?.isGroup == false && feed?.id != Feed.ALL_ENTRIES_ID) {
				context?.startService(Intent(context, FetcherService::class.java).setAction(FetcherService.ACTION_REFRESH_FEEDS).putExtra(FetcherService.EXTRA_FEED_ID,
						feed?.id))
			} else {
				context?.startService(Intent(context, FetcherService::class.java).setAction(FetcherService.ACTION_REFRESH_FEEDS))
			}
		}

		// In case there is no internet, the service won't even start, let's quickly stop the refresh animation
		refresh_layout.postDelayed({ refreshSwipeProgress() }, 500)
	}

	private fun setupTitle() {
		activity?.toolbar?.apply {
			if (feed == null || feed?.id == Feed.ALL_ENTRIES_ID) {
				titleResource = R.string.all_entries
			} else {
				title = feed?.title
			}
		}
	}

	override fun onCreateOptionsMenu(menu: Menu, inflater: MenuInflater) {
		super.onCreateOptionsMenu(menu, inflater)

		inflater.inflate(R.menu.menu_fragment_entries, menu)

		menu.findItem(R.id.menu_entries__share).isVisible = bottom_navigation.selectedItemId == R.id.favorites

		val searchItem = menu.findItem(R.id.menu_entries__search)
		val searchView = searchItem.actionView as SearchView
		if (searchText != null) {
			searchItem.expandActionView()
			searchView.post {
				searchView.setQuery(searchText, false)
				searchView.clearFocus()
			}
		}

		searchView.setOnQueryTextListener(object : SearchView.OnQueryTextListener {
			override fun onQueryTextSubmit(query: String): Boolean {
				return false
			}

			override fun onQueryTextChange(newText: String): Boolean {
				if (searchText != null) { // needed because it can actually be called after the onMenuItemActionCollapse event
					searchText = newText

					// In order to avoid plenty of request, we add a small throttle time
					searchHandler.removeCallbacksAndMessages(null)
					searchHandler.postDelayed({
						initDataObservers()
					}, 700)
				}
				return false
			}
		})
		searchItem.setOnActionExpandListener(object : MenuItem.OnActionExpandListener {
			override fun onMenuItemActionExpand(item: MenuItem?): Boolean {
				searchText = ""
				initDataObservers()
				bottom_navigation.isGone = true
				return true
			}

			override fun onMenuItemActionCollapse(item: MenuItem?): Boolean {
				searchText = null
				initDataObservers()
				bottom_navigation.isVisible = true
				return true
			}
		})
	}

	override fun onOptionsItemSelected(item: MenuItem): Boolean {
		when (item.itemId) {
			R.id.menu_entries__share -> {
				// TODO: will only work for the visible 30 items, need to find something better
				adapter.currentList?.joinToString("\n\n") { it.entry.title + ": " + it.entry.link }?.let { content ->
					val title = getString(R.string.app_name) + " " + getString(R.string.favorites)
					share(content.take(300000), title) // take() to avoid crashing with a too big intent
				}
			}
			R.id.menu_entries__about -> {
				navigator.goToAboutMe()
			}
			R.id.menu_entries__settings -> {
				navigator.goToSettings()
			}
		}

		return true
	}

	fun setSelectedEntryId(selectedEntryId: String) {
		adapter.selectedEntryId = selectedEntryId
	}

	private fun refreshSwipeProgress() {
		refresh_layout.isRefreshing = context?.getPrefBoolean(PrefConstants.IS_REFRESHING, false)
				?: false
	}

	inner class ViewHolder(itemView111: View) : RecyclerView.ViewHolder(itemView111) {
		private val adContainer = this.itemView.findViewById<LinearLayout>(R.id.ad_container)
		@SuppressLint("SetTextI18n")
		fun bind(entryWithFeed: EntryWithFeed, globalClickListener: (EntryWithFeed) -> Unit, globalLongClickListener: (EntryWithFeed) -> Unit, favoriteClickListener: (EntryWithFeed, ImageView) -> Unit) = with(itemView) {
			val mainImgUrl = if (TextUtils.isEmpty(entryWithFeed.entry.imageLink)) null else FetcherService.getDownloadedOrDistantImageUrl(entryWithFeed.entry.id, entryWithFeed.entry.imageLink!!)

			val letterDrawable = Feed.getLetterDrawable(entryWithFeed.entry.feedId, entryWithFeed.feedTitle)
			if (mainImgUrl != null) {
				GlideApp.with(context).load(mainImgUrl).centerCrop().transition(DrawableTransitionOptions.withCrossFade(CROSS_FADE_FACTORY)).placeholder(letterDrawable).error(letterDrawable).into(main_icon)
			} else {
				GlideApp.with(context).clear(main_icon)
				main_icon.setImageDrawable(letterDrawable)
			}

			title.isEnabled = !entryWithFeed.entry.read
			title.text = entryWithFeed.entry.title

			feed_name_layout.isEnabled = !entryWithFeed.entry.read
			feed_name_layout.text = entryWithFeed.feedTitle.orEmpty()

			date.isEnabled = !entryWithFeed.entry.read
			date.text = entryWithFeed.entry.getReadablePublicationDate(context)

			if (entryWithFeed.entry.favorite) {
				favorite_icon.setImageResource(R.drawable.ic_star_24dp)
			} else {
				favorite_icon.setImageResource(R.drawable.ic_star_border_24dp)
			}
			favorite_icon.onClick { favoriteClickListener(entryWithFeed, favorite_icon) }

			onClick {
				//Log.e("ads", "click news.")
				VpnEncrypt.newsClickCount++
				if (VpnEncrypt.newsClickCount%5==0L)mInterstitialAd.loadAd(AdRequest.Builder().build())
				if (VpnEncrypt.newsClickCount%5==1L && mInterstitialAd.isLoaded) {
					//Log.e("ads", "The interstitial loaded.")
					mInterstitialAd.show()
				} else {
					//Log.e("ads", "The interstitial wasn't loaded yet.")
				}
				globalClickListener(entryWithFeed)
			}
			onLongClick {
				globalLongClickListener(entryWithFeed)
				true
			}
		}

		fun clear() = with(itemView) {
			GlideApp.with(context).clear(main_icon)
		}

		fun populateUnifiedNativeAdView(nativeAd: UnifiedNativeAd, adView: UnifiedNativeAdView) {
			// Set other ad assets.
			adView.headlineView = adView.findViewById(R.id.ad_headline)
			adView.bodyView = adView.findViewById(R.id.ad_body)
			adView.callToActionView = adView.findViewById(R.id.ad_call_to_action)
			adView.iconView = adView.findViewById(R.id.ad_app_icon)
			adView.starRatingView = adView.findViewById(R.id.ad_stars)
			adView.advertiserView = adView.findViewById(R.id.ad_advertiser)

			// The headline and media content are guaranteed to be in every UnifiedNativeAd.
			(adView.headlineView as TextView).text = nativeAd.headline

			// These assets aren't guaranteed to be in every UnifiedNativeAd, so it's important to
			// check before trying to display them.
			if (nativeAd.body == null) {
				adView.bodyView.visibility = View.INVISIBLE
			} else {
				adView.bodyView.visibility = View.VISIBLE
				(adView.bodyView as TextView).text = nativeAd.body
			}

			if (nativeAd.callToAction == null) {
				adView.callToActionView.visibility = View.INVISIBLE
			} else {
				adView.callToActionView.visibility = View.VISIBLE
				(adView.callToActionView as Button).text = nativeAd.callToAction
			}

			if (nativeAd.icon == null) {
				adView.iconView.visibility = View.GONE
			} else {
				(adView.iconView as ImageView).setImageDrawable(
						nativeAd.icon.drawable)
				adView.iconView.visibility = View.VISIBLE
			}

			if (nativeAd.starRating == null) {
				adView.starRatingView.visibility = View.INVISIBLE
			} else {
				(adView.starRatingView as RatingBar).rating = nativeAd.starRating!!.toFloat()
				adView.starRatingView.visibility = View.VISIBLE
			}

			if (nativeAd.advertiser == null) {
				adView.advertiserView.visibility = View.INVISIBLE
			} else {
				(adView.advertiserView as TextView).text = nativeAd.advertiser
				adView.advertiserView.visibility = View.VISIBLE
			}

			// This method tells the Google Mobile Ads SDK that you have finished populating your
			// native ad view with this native ad.
			adView.setNativeAd(nativeAd)
			//adView.setBackgroundColor(Color.WHITE) //Adding dividing line for ads
			//adContainer.setPadding(0,1,0,0)  //Adding dividing line for ads
			adContainer.addView(adView)
			adHost = this
		}

		fun attach() {
			if (adHost != null /*|| !item.isBuiltin()*/) return
			if (nativeAdView == null) {
				nativeAdView = layoutInflater.inflate(R.layout.ad_unified, adContainer, false) as UnifiedNativeAdView
				AdLoader.Builder(context,
						//"ca-app-pub-3940256099942544/2247696110" //test ads
						"ca-app-pub-2194043486084479/2721000405"
				).apply {
					forUnifiedNativeAd { unifiedNativeAd ->
						// You must call destroy on old ads when you are done with them,
						// otherwise you will have a memory leak.
						nativeAd?.destroy()
						nativeAd = unifiedNativeAd
						tryBindAd()
					}
					withNativeAdOptions(NativeAdOptions.Builder().apply {
						setVideoOptions(VideoOptions.Builder().apply {
							setStartMuted(true)
						}.build())
					}.build())
				}.build().loadAd(AdRequest.Builder().apply {
					addTestDevice("B08FC1764A7B250E91EA9D0D5EBEB208")
					addTestDevice("7509D18EB8AF82F915874FEF53877A64")
					addTestDevice("F58907F28184A828DD0DB6F8E38189C6")
					addTestDevice("FE983F496D7C5C1878AA163D9420CA97")
				}.build())
			} else if (nativeAd != null) populateUnifiedNativeAdView(nativeAd!!, nativeAdView!!)
		}

		fun detach() {
			if (adHost == this) {
				adHost = null
				adContainer.removeAllViews()
				tryBindAd()
			}
		}

	}

	inner class EntryAdapter(private val globalClickListener: (EntryWithFeed) -> Unit, private val globalLongClickListener: (EntryWithFeed) -> Unit, private val favoriteClickListener: (EntryWithFeed, ImageView) -> Unit) : PagedListAdapter<EntryWithFeed, ViewHolder>(DIFF_CALLBACK) {
		init {
			//setHasStableIds(true)   // see: http://stackoverflow.com/a/32488059/2245107
		}
		override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
			val view = LayoutInflater.from(parent.context).inflate(R.layout.view_entry, parent, false)
			return ViewHolder(view)
		}
		override fun onViewAttachedToWindow(holder: ViewHolder) = holder.attach()
		override fun onViewDetachedFromWindow(holder: ViewHolder) = holder.detach()

		override fun onBindViewHolder(holder: ViewHolder, position: Int) {
			val entryWithFeed = getItem(position)
			if (entryWithFeed != null) {
				holder.bind(entryWithFeed, globalClickListener, globalLongClickListener, favoriteClickListener)
			} else {
				// Null defines a placeholder item - PagedListAdapter will automatically invalidate
				// this row when the actual object is loaded from the database
				holder.clear()
			}

			holder.itemView.isSelected = (selectedEntryId == entryWithFeed?.entry?.id)
		}
		var selectedEntryId: String? = null
			set(newValue) {
				field = newValue
				notifyDataSetChanged()
			}
	}
}

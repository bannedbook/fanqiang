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

package net.frju.flym.ui.entrydetails

import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.util.DisplayMetrics
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import androidx.fragment.app.Fragment
import androidx.lifecycle.LiveData
import androidx.lifecycle.Observer
import com.google.android.gms.ads.AdRequest
import com.google.android.gms.ads.AdSize
import com.google.android.gms.ads.AdView
import com.google.android.gms.ads.MobileAds
import kotlinx.android.synthetic.main.activity_main.*
import kotlinx.android.synthetic.main.fragment_entry_details.*
import me.thanel.swipeactionview.SwipeActionView
import me.thanel.swipeactionview.SwipeGestureListener
import net.fred.feedex.R
import net.frju.flym.App
import net.frju.flym.data.entities.EntryWithFeed
import net.frju.flym.data.utils.PrefConstants
import net.frju.flym.service.FetcherService
import net.frju.flym.ui.main.MainNavigator
import net.frju.flym.utils.getPrefBoolean
import net.frju.flym.utils.isOnline
import org.jetbrains.anko.*
import org.jetbrains.anko.support.v4.browse
import org.jetbrains.anko.support.v4.share
import org.jetbrains.anko.support.v4.toast
import org.jetbrains.annotations.NotNull

class EntryDetailsFragment : Fragment() {

    companion object {

        const val ARG_ENTRY_ID = "ARG_ENTRY_ID"
        const val ARG_ALL_ENTRIES_IDS = "ARG_ALL_ENTRIES_IDS"

        fun newInstance(entryId: String, allEntryIds: List<String>): EntryDetailsFragment {
            return EntryDetailsFragment().apply {
                arguments = bundleOf(ARG_ENTRY_ID to entryId, ARG_ALL_ENTRIES_IDS to allEntryIds)
            }
        }
    }

    private val navigator: MainNavigator? by lazy { activity as? MainNavigator }

    private lateinit var entryId: String
    private var entryWithFeed: EntryWithFeed? = null
    private var allEntryIds = emptyList<String>()
        set(value) {
            field = value

            val currentIdx = value.indexOf(entryId)

            previousId = if (currentIdx <= 0) {
                null
            } else {
                value[currentIdx - 1]
            }

            nextId = if (currentIdx < 0 || currentIdx >= value.size - 1) {
                null
            } else {
                value[currentIdx + 1]
            }
        }
    private var previousId: String? = null
    private var nextId: String? = null
    private var isMobilizingLiveData: LiveData<Int>? = null
    private var isMobilizing = false
    private var preferFullText = true

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? =
            inflater.inflate(R.layout.fragment_entry_details, container, false)

    override fun onDestroyView() {
        super.onDestroyView()
        entry_view.destroy()
    }

    override fun onPause() {
        entry_view.onPause()
        super.onPause()
    }

    override fun onResume() {
        super.onResume()
        entry_view.onResume()
    }

    override fun onActivityCreated(savedInstanceState: Bundle?) {
        super.onActivityCreated(savedInstanceState)

        refresh_layout.setColorScheme(R.color.colorAccent,
                requireContext().attr(R.attr.colorPrimaryDark).resourceId,
                R.color.colorAccent,
                requireContext().attr(R.attr.colorPrimaryDark).resourceId)

        refresh_layout.setOnRefreshListener {
            switchFullTextMode()
        }

        swipe_view.swipeGestureListener = object : SwipeGestureListener {
            override fun onSwipedLeft(@NotNull swipeActionView: SwipeActionView): Boolean {
                nextId?.let { nextId ->
                    setEntry(nextId, allEntryIds)
                    navigator?.setSelectedEntryId(nextId)
                }
                return true
            }

            override fun onSwipedRight(@NotNull swipeActionView: SwipeActionView): Boolean {
                previousId?.let { previousId ->
                    setEntry(previousId, allEntryIds)
                    navigator?.setSelectedEntryId(previousId)
                }
                return true
            }
        }

        setEntry(arguments?.getString(ARG_ENTRY_ID)!!, arguments?.getStringArrayList(ARG_ALL_ENTRIES_IDS)!!)
    }

    private fun initDataObservers() {
        isMobilizingLiveData?.removeObservers(this)
        refresh_layout.isRefreshing = false

        isMobilizingLiveData = App.db.taskDao().observeItemMobilizationTasksCount(entryId)
        isMobilizingLiveData?.observe(this, Observer<Int> { count ->
            if (count ?: 0 > 0) {
                isMobilizing = true
                refresh_layout.isRefreshing = true

                // If the service is not started, start it here to avoid an infinite loading
                if (context?.getPrefBoolean(PrefConstants.IS_REFRESHING, false) == false) {
                    context?.startService(Intent(context, FetcherService::class.java).setAction(FetcherService.ACTION_MOBILIZE_FEEDS))
                }
            } else {
                if (isMobilizing) {
                    doAsync {
                        App.db.entryDao().findByIdWithFeed(entryId)?.let { newEntry ->
                            uiThread {
                                entryWithFeed = newEntry
                                entry_view.setEntry(entryWithFeed, preferFullText)

                                setupToolbar()
                            }
                        }
                    }
                }

                isMobilizing = false
                refresh_layout.isRefreshing = false
            }
        })
    }

    private fun setupToolbar() {
        toolbar.apply {
            entryWithFeed?.let { entryWithFeed ->
                title = entryWithFeed.feedTitle

                menu.clear()
                inflateMenu(R.menu.menu_fragment_entry_details)

                if (activity?.containers_layout?.hasTwoColumns() != true) {
                    setNavigationIcon(R.drawable.ic_back_white_24dp)
                    setNavigationOnClickListener { activity?.onBackPressed() }
                }

                if (entryWithFeed.entry.favorite) {
                    menu.findItem(R.id.menu_entry_details__favorite)
                            .setTitle(R.string.menu_unstar)
                            .setIcon(R.drawable.ic_star_white_24dp)
                }


                if (entryWithFeed.entry.mobilizedContent == null || !preferFullText) {
                    menu.findItem(R.id.menu_entry_details__fulltext).isVisible = true
                    menu.findItem(R.id.menu_entry_details__original_text).isVisible = false
                } else {
                    menu.findItem(R.id.menu_entry_details__fulltext).isVisible = false
                    menu.findItem(R.id.menu_entry_details__original_text).isVisible = true
                }

                setOnMenuItemClickListener { item ->
                    when (item?.itemId) {
                        R.id.menu_entry_details__favorite -> {
                            entryWithFeed.entry.favorite = !entryWithFeed.entry.favorite
                            entryWithFeed.entry.read = true // otherwise it marked it as unread again

                            if (entryWithFeed.entry.favorite) {
                                item.setTitle(R.string.menu_unstar).setIcon(R.drawable.ic_star_white_24dp)
                            } else {
                                item.setTitle(R.string.menu_star).setIcon(R.drawable.ic_star_border_white_24dp)
                            }

                            doAsync {
                                App.db.entryDao().update(entryWithFeed.entry)
                            }
                        }
                        R.id.menu_entry_details__open_browser -> {
                            entryWithFeed.entry.link?.let {
                                browse(it)
                            }
                        }
                        R.id.menu_entry_details__share -> {
                            share(entryWithFeed.entry.link.orEmpty(), entryWithFeed.entry.title.orEmpty())
                        }
                        R.id.menu_entry_details__fulltext -> {
                            switchFullTextMode()
                        }
                        R.id.menu_entry_details__original_text -> {
                            switchFullTextMode()
                        }
                        R.id.menu_entry_details__mark_as_unread -> {
                            doAsync {
                                App.db.entryDao().markAsUnread(listOf(entryId))
                            }
                            if (activity?.containers_layout?.hasTwoColumns() != true) {
                                activity?.onBackPressed()
                            }
                        }
                    }

                    true
                }
            }
        }
    }

    private fun switchFullTextMode() {
        // Enable this to test new manual mobilization
//		doAsync {
//			entryWithFeed?.entry?.let {
//				it.mobilizedContent = null
//				App.db.entryDao().insert(it)
//			}
//		}

        entryWithFeed?.let { entryWithFeed ->
            if (entryWithFeed.entry.mobilizedContent == null || !preferFullText) {
                if (entryWithFeed.entry.mobilizedContent == null) {
                    this@EntryDetailsFragment.context?.let { c ->
                        if (c.isOnline()) {
                            doAsync {
                                FetcherService.addEntriesToMobilize(listOf(entryWithFeed.entry.id))
                                c.startService(Intent(c, FetcherService::class.java).setAction(FetcherService.ACTION_MOBILIZE_FEEDS))
                            }
                        } else {
                            refresh_layout.isRefreshing = false
                            toast(R.string.network_error).show()
                        }
                    }
                } else {
                    refresh_layout.isRefreshing = false
                    preferFullText = true
                    entry_view.setEntry(entryWithFeed, preferFullText)

                    setupToolbar()
                }
            } else {
                refresh_layout.isRefreshing = isMobilizing
                preferFullText = false
                entry_view.setEntry(entryWithFeed, preferFullText)

                setupToolbar()
            }
        }
    }

    private lateinit var adView: AdView
    private var initialLayoutComplete = false
    // Determine the screen width (less decorations) to use for the ad width.
    // If the ad hasn't been laid out, default to the full screen width.
    private val adSize: AdSize
        get() {
            val display = activity?.windowManager?.defaultDisplay
            val outMetrics = DisplayMetrics()
            display?.getMetrics(outMetrics)
            val density = outMetrics.density
            var adWidthPixels = ad_view_container.width.toFloat()
            if (adWidthPixels == 0f) {
                adWidthPixels = outMetrics.widthPixels.toFloat()
            }

            val adWidth = (adWidthPixels / density).toInt()
            return AdSize.getCurrentOrientationAnchoredAdaptiveBannerAdSize(activity, adWidth)
        }
    private fun loadBanner() {
        adView.adUnitId=getString(R.string.banner_adUnitId)
        adView.adSize = adSize
        // Log.e("adSize",adView.adSize.width.toString()+","+adView.adSize.height.toString())
        // Create an ad request.
        val adRequest = AdRequest.Builder().build()
        // Start loading the ad in the background.
        adView.loadAd(adRequest)
    }
    fun setEntry(entryId: String, allEntryIds: List<String>) {
        this.entryId = entryId
        this.allEntryIds = allEntryIds
        arguments?.putString(ARG_ENTRY_ID, entryId)
        arguments?.putStringArrayList(ARG_ALL_ENTRIES_IDS, ArrayList(allEntryIds))

        doAsync {
            App.db.entryDao().findByIdWithFeed(entryId)?.let { entry ->
                entryWithFeed = entry
                preferFullText = true
                isMobilizing = false

                uiThread {
					try {
						val contentText = if (preferFullText) entryWithFeed?.entry?.mobilizedContent
								?: entryWithFeed?.entry?.description.orEmpty() else entryWithFeed?.entry?.description.orEmpty()
						if (!contentText.contains("https://www.youtube.com/embed/")) {
							MobileAds.initialize(activity) {}
                            adView = AdView(activity)
                            ad_view_container.addView(adView)
                            // Since we're loading the banner based on the adContainerView size, we need to wait until this
                            // view is laid out before we can get the width.
                            ad_view_container.viewTreeObserver.addOnGlobalLayoutListener {
                                if (!initialLayoutComplete) {
                                    initialLayoutComplete = true
                                    loadBanner()
                                    val adsHeight: Int = adView.adSize.getHeightInPixels(activity)
                                    swipe_view.bottomPadding = adsHeight + 5
                                }
                            }
						}
                        else{
                            val imageBannerView = ImageView(activity)
                            imageBannerView.setImageResource(R.drawable.v2free)
                            imageBannerView.setOnClickListener{
                                val intent = Intent()
                                intent.action = Intent.ACTION_VIEW
                                intent.addCategory(Intent.CATEGORY_BROWSABLE)
                                intent.data = Uri.parse("https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA")
                                //fqnews2
                                //intent.data = Uri.parse("https://github.com/vpn69/tea/blob/main/V2Ray%E6%9C%BA%E5%9C%BA%E6%8E%A8%E8%8D%90.md")
                                startActivity(intent)
                            }
                            ad_view_container.addView(imageBannerView)
                            val adsHeight: Int = imageBannerView.drawable.intrinsicHeight
                            swipe_view.bottomPadding = adsHeight + 5
                        }
					} catch (e: Exception) {
                        //e.printStackTrace()
						Log.e("ads---", e.message?:"")
					}

                    entry_view.setEntry(entryWithFeed, preferFullText)

                    initDataObservers()

                    setupToolbar()
                }
            }

            App.db.entryDao().markAsRead(listOf(entryId))
        }
    }
}

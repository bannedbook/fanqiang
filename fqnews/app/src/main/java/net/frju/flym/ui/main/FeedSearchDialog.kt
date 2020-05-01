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

package net.frju.flym.ui.main

import android.content.Context
import android.graphics.Color
import android.os.Bundle
import android.text.Html
import android.view.ViewGroup
import android.webkit.URLUtil
import com.rometools.rome.io.SyndFeedInput
import com.rometools.rome.io.XmlReader
import ir.mirrajabi.searchdialog.SimpleSearchDialogCompat
import ir.mirrajabi.searchdialog.core.BaseFilter
import ir.mirrajabi.searchdialog.core.SearchResultListener
import net.fred.feedex.R
import net.frju.flym.App
import net.frju.flym.data.entities.Feed
import net.frju.flym.data.entities.SearchFeedResult
import net.frju.flym.service.FetcherService
import org.jetbrains.anko.AnkoLogger
import org.jetbrains.anko.doAsync
import org.jetbrains.anko.hintTextColor
import org.jetbrains.anko.textColor
import org.jetbrains.anko.toast
import org.jetbrains.anko.warn
import org.json.JSONObject
import java.net.URLEncoder
import java.util.ArrayList


private const val FEED_SEARCH_TITLE = "title"
private const val FEED_SEARCH_URL = "feedId"
private const val FEED_SEARCH_DESC = "description"
private val FEED_SEARCH_BLACKLIST = arrayOf("http://syndication.lesechos.fr/rss/rss_finance.xml")

private val GNEWS_TOPIC_NAME = intArrayOf(R.string.google_news_top_stories, R.string.google_news_world, R.string.google_news_business, R.string.google_news_science_technology, R.string.google_news_entertainment, R.string.google_news_sports, R.string.google_news_health)
private val GNEWS_TOPIC_CODE = arrayOf("", "WORLD", "BUSINESS", "SCITECH", "ENTERTAINMENT", "SPORTS", "HEALTH")

private fun generateDefaultFeeds(context: Context) =
		GNEWS_TOPIC_NAME.mapIndexed { index, name ->
			@Suppress("DEPRECATION")
			val link = if (GNEWS_TOPIC_CODE[index].isNotEmpty())
				"https://news.google.com/news/rss/headlines/section/topic/${GNEWS_TOPIC_CODE[index]}?ned=${context.resources.configuration.locale.language}"
			else
				"https://news.google.com/news/rss/?ned=${context.resources.configuration.locale.language}"

			SearchFeedResult(link, context.getString(name))
		}

class FeedSearchDialog(context: Context, val search: String = "", private var defaultFeeds: List<SearchFeedResult> = generateDefaultFeeds(context))
	: SimpleSearchDialogCompat<SearchFeedResult>(context,
		context.getString(R.string.feed_search),
		context.getString(R.string.feed_search_hint),
		null,
		ArrayList(defaultFeeds),
		SearchResultListener<SearchFeedResult> { dialog, item, position ->
			context.toast(R.string.feed_added)
			dialog.dismiss()

			val feedToAdd = Feed(link = item.link, title = item.name)
			feedToAdd.retrieveFullText = defaultFeeds.contains(item) // do that automatically for google news feeds

			context.doAsync { App.db.feedDao().insert(feedToAdd) }
		}), AnkoLogger {

	init {
		filter = object : BaseFilter<SearchFeedResult>() {
			override fun performFiltering(charSequence: CharSequence): FilterResults? {
				doBeforeFiltering()

				val results = FilterResults()
				val array = ArrayList<SearchFeedResult>()

				if (charSequence.isNotBlank()) {
					try {
						val searchStr = charSequence.toString().trim()

						if (URLUtil.isNetworkUrl(searchStr)) {
							FetcherService.createCall(searchStr).execute().use { response ->
								val romeFeed = SyndFeedInput().build(XmlReader(response.body!!.byteStream()))

								array.add(SearchFeedResult(searchStr, romeFeed.title
										?: searchStr, romeFeed.description ?: ""))
							}
						} else {
							@Suppress("DEPRECATION")
							val searchUrl = "https://cloud.feedly.com/v3/search/feeds?count=20&locale=" + context.resources.configuration.locale.language + "&query=" + URLEncoder.encode(searchStr, "UTF-8")
							FetcherService.createCall(searchUrl).execute().use {
								it.body?.let { body ->
									val jsonStr = body.string()

									// Parse results
									val entries = JSONObject(jsonStr).getJSONArray("results")
									for (i in 0 until entries.length()) {
										try {
											val entry = entries.get(i) as JSONObject
											val url = entry.get(FEED_SEARCH_URL).toString().replace("feed/", "")
											if (url.isNotEmpty() && !FEED_SEARCH_BLACKLIST.contains(url)) {
												@Suppress("DEPRECATION")
												array.add(
														SearchFeedResult(url,
																Html.fromHtml(entry.get(FEED_SEARCH_TITLE).toString()).toString(),
																Html.fromHtml(entry.get(FEED_SEARCH_DESC).toString()).toString()))
											}
										} catch (ignored: Throwable) {
										}
									}
								}
							}
						}
					} catch (t: Throwable) {
						warn("error during feedWithCount search", t)
					}
				} else {
					array.addAll(defaultFeeds)
				}

				results.values = array
				results.count = array.size
				return results
			}

			override fun publishResults(charSequence: CharSequence, filterResults: FilterResults?) {
				filterResults?.let {
					@Suppress("UNCHECKED_CAST")
					this@FeedSearchDialog.filterResultListener.onFilter(it.values as? ArrayList<SearchFeedResult>)
				}
				doAfterFiltering()
			}
		}
	}

	override fun onCreate(savedInstanceState: Bundle?) {
		super.onCreate(savedInstanceState)

		searchBox.run {
			textColor = Color.BLACK
			hintTextColor = Color.GRAY

			// Hack to avoid being able to dismiss the popup by taping around the edit text
			(parent.parent as ViewGroup).isClickable = true

			if (search.isNotBlank()) {
				setText(search.subSequence(0, search.length))
				setSelection(search.length)
			}
		}
	}
}

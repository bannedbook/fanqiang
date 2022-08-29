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

package net.frju.flym.service

import android.annotation.TargetApi
import android.app.IntentService
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.graphics.BitmapFactory
import android.net.ConnectivityManager
import android.net.Uri
import android.os.Build
import android.os.Handler
import android.util.Log
import androidx.core.app.NotificationCompat
import androidx.core.text.HtmlCompat
import com.rometools.rome.io.SyndFeedInput
import com.rometools.rome.io.XmlReader
import net.dankito.readability4j.extended.Readability4JExtended
import net.fred.feedex.R
import net.frju.flym.App
import net.frju.flym.App.Companion.context
import net.frju.flym.data.entities.Entry
import net.frju.flym.data.entities.Feed
import net.frju.flym.data.entities.Task
import net.frju.flym.data.entities.toDbFormat
import net.frju.flym.data.utils.PrefConstants
import net.frju.flym.ui.main.MainActivity
import net.frju.flym.utils.*
import okhttp3.Call
import okhttp3.JavaNetCookieJar
import okhttp3.OkHttpClient
import okhttp3.Request
import okio.buffer
import okio.sink
import org.jetbrains.anko.*
import org.jsoup.Jsoup
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.net.CookieManager
import java.net.CookiePolicy
import java.util.concurrent.ExecutorCompletionService
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit
import kotlin.math.max


class FetcherService : IntentService(FetcherService::class.java.simpleName) {
	companion object : AnkoLogger {
		const val EXTRA_FEED_ID = "EXTRA_FEED_ID"

		private val COOKIE_MANAGER = CookieManager().apply {
			setCookiePolicy(CookiePolicy.ACCEPT_ALL)
		}

		private val HTTP_CLIENT: OkHttpClient = OkHttpClient.Builder()
				.connectTimeout(10, TimeUnit.SECONDS)
				.readTimeout(20, TimeUnit.SECONDS)
				.cookieJar(JavaNetCookieJar(COOKIE_MANAGER))
				.build()

		const val FROM_AUTO_REFRESH = "FROM_AUTO_REFRESH"

		const val ACTION_REFRESH_FEEDS = "net.frju.flym.REFRESH"
		const val ACTION_MOBILIZE_FEEDS = "net.frju.flym.MOBILIZE_FEEDS"
		const val ACTION_DOWNLOAD_IMAGES = "net.frju.flym.DOWNLOAD_IMAGES"

		private const val THREAD_NUMBER = 3
		private const val MAX_TASK_ATTEMPT = 3

		private val IMAGE_FOLDER_FILE = File(App.context.cacheDir, "images/")
		private val IMAGE_FOLDER = IMAGE_FOLDER_FILE.absolutePath + '/'
		private const val TEMP_PREFIX = "TEMP__"
		private const val ID_SEPARATOR = "__"

		fun createCall(url: String): Call = HTTP_CLIENT.newCall(Request.Builder()
				.url(url)
				.header("User-agent", "Mozilla/5.0 (compatible) AppleWebKit Chrome Safari") // some feeds need this to work properly
				.addHeader("accept", "*/*")
				.build())

		fun fetch(context: Context, isFromAutoRefresh: Boolean, action: String, feedId: Long = 0L) {
			//createCall("https://api.ipify.org/").execute().use { response ->Log.e("Refresh myip", ""+response.body?.string())}
			if (context.getPrefBoolean(PrefConstants.IS_REFRESHING, false)) {
				return
			}

			// Connectivity issue, we quit
			if (!context.isOnline()) {
				return
			}

			val skipFetch = isFromAutoRefresh && context.getPrefBoolean(PrefConstants.REFRESH_WIFI_ONLY, false)
					&& context.connectivityManager.activeNetworkInfo?.type != ConnectivityManager.TYPE_WIFI
			Log.e("Refresh", "skipFetch...$skipFetch")
			// We need to skip the fetching process, so we quit
			if (skipFetch) {
				return
			}

            when (action) {
                ACTION_MOBILIZE_FEEDS -> {
                    mobilizeAllEntries()
                    downloadAllImages()
                }
                ACTION_DOWNLOAD_IMAGES -> downloadAllImages()
                else -> { // == Constants.ACTION_REFRESH_FEEDS
                    context.putPrefBoolean(PrefConstants.IS_REFRESHING, true)

					val readEntriesKeepTime = context.getPrefString(PrefConstants.KEEP_TIME, "4")!!.toLong() * 86400000L
					val readEntriesKeepDate = if (readEntriesKeepTime > 0) System.currentTimeMillis() - readEntriesKeepTime else 0

					val unreadEntriesKeepTime = context.getPrefString(PrefConstants.KEEP_TIME_UNREAD, "0")!!.toLong() * 86400000L
					val unreadEntriesKeepDate = if (unreadEntriesKeepTime > 0) System.currentTimeMillis() - unreadEntriesKeepTime else 0

					deleteOldEntries(readEntriesKeepDate, 1)
					deleteOldEntries(unreadEntriesKeepDate, 0)
					COOKIE_MANAGER.cookieStore.removeAll() // Cookies are important for some sites, but we clean them each times

                    // We need to use the more recent date in order to be sure to not see old entries again
                    val acceptMinDate = max(readEntriesKeepDate, unreadEntriesKeepDate)

                    var newCount = 0
                    if (feedId == 0L || App.db.feedDao().findById(feedId)?.isGroup == true) {
                        newCount = refreshFeeds(feedId, acceptMinDate)
                    } else {
                        App.db.feedDao().findById(feedId)?.let {
                            try {
                                newCount = refreshFeed(it, acceptMinDate)
                            } catch (e: Exception) {
                                error("Can't fetch feed ${it.link}", e)
                            }
                        }
                    }

					showRefreshNotification(newCount)
					mobilizeAllEntries()
					downloadAllImages()

					context.putPrefBoolean(PrefConstants.IS_REFRESHING, false)
				}
			}
		}

		private fun showRefreshNotification(itemCount: Int = 0) {

			val shouldDisplayNotification =
				context.getPrefBoolean(PrefConstants.REFRESH_NOTIFICATION_ENABLED, true)

			if (shouldDisplayNotification && itemCount > 0) {
				if (!MainActivity.isInForeground) {
					val unread = App.db.entryDao().countUnread

					if (unread > 0) {
						val text = context.resources.getQuantityString(
							R.plurals.number_of_new_entries,
							unread.toInt(),
							unread
						)

						val notificationIntent = Intent(
							context,
							MainActivity::class.java
						).putExtra(MainActivity.EXTRA_FROM_NOTIF, true)
						val contentIntent = PendingIntent.getActivity(
							context, 0, notificationIntent,
							PendingIntent.FLAG_UPDATE_CURRENT
						)

						val channelId = "notif_channel"

						@TargetApi(Build.VERSION_CODES.O)
						if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
							val channel = NotificationChannel(
								channelId,
								context.getString(R.string.app_name),
								NotificationManager.IMPORTANCE_DEFAULT
							)
							context.notificationManager.createNotificationChannel(channel)
						}

						val notifBuilder = NotificationCompat.Builder(context, channelId)
							.setContentIntent(contentIntent)
							.setSmallIcon(R.drawable.ic_statusbar_rss)
							.setLargeIcon(
								BitmapFactory.decodeResource(
									context.resources,
									R.mipmap.ic_launcher
								)
							)
							.setTicker(text)
							.setWhen(System.currentTimeMillis())
							.setAutoCancel(true)
							.setContentTitle(context.getString(R.string.flym_feeds))
							.setContentText(text)

						context.notificationManager.notify(0, notifBuilder.build())
					}
				} else {
					context.notificationManager.cancel(0)
				}
			}
		}

		fun shouldDownloadPictures(): Boolean {
			val fetchPictureMode = context.getPrefString(PrefConstants.PRELOAD_IMAGE_MODE, PrefConstants.PRELOAD_IMAGE_MODE__WIFI_ONLY)

            if (context.getPrefBoolean(PrefConstants.DISPLAY_IMAGES, true)) {
                if (PrefConstants.PRELOAD_IMAGE_MODE__ALWAYS == fetchPictureMode) {
                    return true
                } else if (PrefConstants.PRELOAD_IMAGE_MODE__WIFI_ONLY == fetchPictureMode
                        && context.connectivityManager.activeNetworkInfo?.type == ConnectivityManager.TYPE_WIFI) {
                    return true
                }
            }

			return false
		}

		fun getDownloadedImagePath(entryId: String, imgUrl: String): String =
				IMAGE_FOLDER + entryId + ID_SEPARATOR + imgUrl.sha1()

		private fun getTempDownloadedImagePath(entryId: String, imgUrl: String): String =
				IMAGE_FOLDER + TEMP_PREFIX + entryId + ID_SEPARATOR + imgUrl.sha1()

		fun getDownloadedOrDistantImageUrl(entryId: String, imgUrl: String): String {
			val dlImgFile = File(getDownloadedImagePath(entryId, imgUrl))
			return if (dlImgFile.exists()) {
				Uri.fromFile(dlImgFile).toString()
			} else {
				imgUrl
			}
		}

		fun addImagesToDownload(imgUrlsToDownload: Map<String, List<String>>) {
			if (imgUrlsToDownload.isNotEmpty()) {
				val newTasks = mutableListOf<Task>()
				for ((key, value) in imgUrlsToDownload) {
					for (img in value) {
						val task = Task().apply {
							entryId = key
							imageLinkToDl = img
						}
						newTasks.add(task)
					}
				}

				App.db.taskDao().insert(*newTasks.toTypedArray())
			}
		}

		fun addEntriesToMobilize(entryIds: List<String>) {
			val newTasks = entryIds.map { Task(entryId = it) }

			App.db.taskDao().insert(*newTasks.toTypedArray())
		}


		private fun mobilizeAllEntries() {

			val tasks = App.db.taskDao().mobilizeTasks
			val imgUrlsToDownload = mutableMapOf<String, List<String>>()

			val downloadPictures = shouldDownloadPictures()

			for (task in tasks) {
				var success = false

				App.db.entryDao().findById(task.entryId)?.let { entry ->
					entry.link?.let { link ->
						try {
							createCall(link).execute().use { response ->
								response.body?.byteStream()?.let { input ->
									Readability4JExtended(link, Jsoup.parse(input, null, link)).parse().articleContent?.html()?.let {
										val mobilizedHtml = HtmlUtils.improveHtmlContent(it, getBaseUrl(link))

                                        val entryDescription = entry.description
                                        if (entryDescription == null || HtmlCompat.fromHtml(mobilizedHtml, HtmlCompat.FROM_HTML_MODE_LEGACY).length > HtmlCompat.fromHtml(entryDescription, HtmlCompat.FROM_HTML_MODE_LEGACY).length) { // If the retrieved text is smaller than the original one, then we certainly failed...
                                            if (downloadPictures) {
                                                val imagesList = HtmlUtils.getImageURLs(mobilizedHtml)
                                                if (imagesList.isNotEmpty()) {
                                                    if (entry.imageLink == null) {
                                                        entry.imageLink = HtmlUtils.getMainImageURL(imagesList)
                                                    }
                                                    imgUrlsToDownload[entry.id] = imagesList
                                                }
                                            } else if (entry.imageLink == null) {
                                                entry.imageLink = HtmlUtils.getMainImageURL(mobilizedHtml)
                                            }

											success = true

											entry.mobilizedContent = mobilizedHtml
											App.db.entryDao().update(entry)

											App.db.taskDao().delete(task)
										}
									}
								}
							}
						} catch (t: Throwable) {
							Log.e("Refresh fetch",this.javaClass.name+":"+t.javaClass.name+":"+t.message)
							error("Can't mobilize feedWithCount ${entry.link}", t)
						}
					}
				}

				if (!success) {
					Log.e("Refresh fetch","failed-"+task.entryId)
					if (task.numberAttempt + 1 > MAX_TASK_ATTEMPT) {
						App.db.taskDao().delete(task)
					} else {
						task.numberAttempt += 1
						App.db.taskDao().update(task)
					}
				}
			}

			addImagesToDownload(imgUrlsToDownload)
		}

		private fun downloadAllImages() {
			if (shouldDownloadPictures()) {
				val tasks = App.db.taskDao().downloadTasks
				for (task in tasks) {
					try {
						downloadImage(task.entryId, task.imageLinkToDl)

						// If we are here, everything is OK
						App.db.taskDao().delete(task)
					} catch (ignored: Exception) {
						if (task.numberAttempt + 1 > MAX_TASK_ATTEMPT) {
							App.db.taskDao().delete(task)
						} else {
							task.numberAttempt += 1
							App.db.taskDao().insert(task)
						}
					}
				}
			}
		}

        private fun refreshFeeds(feedId: Long, acceptMinDate: Long): Int {

			val executor = Executors.newFixedThreadPool(THREAD_NUMBER) { r ->
				Thread(r).apply {
					priority = Thread.MIN_PRIORITY
				}
			}
			val completionService = ExecutorCompletionService<Int>(executor)

            var globalResult = 0
            val feeds: List<Feed>
            if (feedId == 0L) {
                feeds = App.db.feedDao().allNonGroupFeeds
            } else {
                feeds = App.db.feedDao().allFeedsInGroup(feedId)
            }

            for (feed in feeds) {
                completionService.submit {
                    var result = 0
                    try {
                        result = refreshFeed(feed, acceptMinDate)
                    } catch (e: Exception) {
                        error("Can't fetch feedWithCount ${feed.link}", e)
                    }

					result
				}
			}

            for (i in feeds.indices) {
                try {
                    val f = completionService.take()
                    globalResult += f.get()
                } catch (ignored: Exception) {
                }
            }

			executor.shutdownNow() // To purge observeAll threads

			return globalResult
		}

		private fun refreshFeed(feed: Feed, acceptMinDate: Long): Int {
			val entries = mutableListOf<Entry>()
			val entriesToInsert = mutableListOf<Entry>()
			val imgUrlsToDownload = mutableMapOf<String, List<String>>()
			val downloadPictures = shouldDownloadPictures()

			val previousFeedState = feed.copy()
			try {
				createCall(feed.link).execute().use { response ->
					val input = SyndFeedInput()
					val romeFeed = input.build(XmlReader(response.body!!.byteStream()))
					entries.addAll(romeFeed.entries.asSequence().filter { it.publishedDate?.time ?: Long.MAX_VALUE > acceptMinDate }.map { it.toDbFormat(context, feed) })
					feed.update(romeFeed)
				}
			} catch (t: Throwable) {
				Log.e("Refresh refreshFeed",this.javaClass.name+":"+t.javaClass.name+":"+t.message)
				feed.fetchError = true
			}

			if (feed != previousFeedState) {
				App.db.feedDao().update(feed)
			}

			// First we remove the entries that we already have in db (no update to save data)
			val existingIds = App.db.entryDao().idsForFeed(feed.id)
			entries.removeAll { it.id in existingIds }

			// Second, we filter items with same title than one we already have
			if (context.getPrefBoolean(PrefConstants.REMOVE_DUPLICATES, true)) {
				val existingTitles = App.db.entryDao().findAlreadyExistingTitles(entries.mapNotNull { it.title })
				entries.removeAll { it.title in existingTitles }
			}

			// Third, we filter items containing forbidden keywords
			val filterKeywordString = context.getPrefString(PrefConstants.FILTER_KEYWORDS, "")!!
			if (filterKeywordString.isNotBlank()) {
				val keywordLists = filterKeywordString.split(',').map { it.trim() }

                if (keywordLists.isNotEmpty()) {
                    entries.removeAll { entry ->
                        keywordLists.any {
                            entry.title?.contains(it, true) == true ||
                                    entry.description?.contains(it, true) == true ||
                                    entry.author?.contains(it, true) == true
                        }
                    }
                }
            }

			val feedBaseUrl = getBaseUrl(feed.link)
			var foundExisting = false

			// Now we improve the html and find images
			for (entry in entries) {
				if (existingIds.contains(entry.id)) {
					foundExisting = true
				}

				if (entry.publicationDate != entry.fetchDate || !foundExisting) { // we try to not put back old entries, even when there is no date
					if (!existingIds.contains(entry.id)) {
						entriesToInsert.add(entry)

						entry.title = entry.title?.replace("\n", " ")?.trim()
						entry.description?.let { desc ->
							// Improve the description
							val improvedContent = HtmlUtils.improveHtmlContent(desc, feedBaseUrl)

							if (downloadPictures) {
								val imagesList = HtmlUtils.getImageURLs(improvedContent)
								if (imagesList.isNotEmpty()) {
									if (entry.imageLink == null) {
										entry.imageLink = HtmlUtils.getMainImageURL(imagesList)
									}
									imgUrlsToDownload[entry.id] = imagesList
								}
							} else if (entry.imageLink == null) {
								entry.imageLink = HtmlUtils.getMainImageURL(improvedContent)
							}

							entry.description = improvedContent
						}
					} else {
						foundExisting = true
					}
				}
			}

			// Insert everything
			App.db.entryDao().insert(*(entriesToInsert.toTypedArray()))

			if (feed.retrieveFullText) {
				addEntriesToMobilize(entries.map { it.id })
			}

			addImagesToDownload(imgUrlsToDownload)

			return entries.size
		}

		private fun deleteOldEntries(keepDateBorderTime: Long, read: Long) {
			if (keepDateBorderTime > 0) {
				App.db.entryDao().deleteOlderThan(keepDateBorderTime, read)
				// Delete the cache files
				deleteEntriesImagesCache(keepDateBorderTime)
			}
		}

		@Throws(IOException::class)
		private fun downloadImage(entryId: String, imgUrl: String) {
			val tempImgPath = getTempDownloadedImagePath(entryId, imgUrl)
			val finalImgPath = getDownloadedImagePath(entryId, imgUrl)

			if (!File(tempImgPath).exists() && !File(finalImgPath).exists()) {
				IMAGE_FOLDER_FILE.mkdir() // create images dir

                // Compute the real URL (without "&eacute;", ...)
                val realUrl = HtmlCompat.fromHtml(imgUrl, HtmlCompat.FROM_HTML_MODE_LEGACY).toString()

				try {
					createCall(realUrl).execute().use { response ->
						response.body?.let { body ->
							val fileOutput = FileOutputStream(tempImgPath)

							val sink = fileOutput.sink().buffer()
							sink.writeAll(body.source())
							sink.close()

							File(tempImgPath).renameTo(File(finalImgPath))
						}
					}
				} catch (e: Exception) {
					Log.e("Refresh fetch",this.javaClass.name+":"+e.javaClass.name+":"+e.message)
					File(tempImgPath).delete()
					throw e
				}
			}
		}

		private fun deleteEntriesImagesCache(keepDateBorderTime: Long) {
			if (IMAGE_FOLDER_FILE.exists()) {

				// We need to exclude favorite entries images to this cleanup
				val favoriteIds = App.db.entryDao().favoriteIds

                IMAGE_FOLDER_FILE.listFiles()?.forEach { file ->
                    // If old file and not part of a favorite entry
                    if (file.lastModified() < keepDateBorderTime && !favoriteIds.any { file.name.startsWith(it + ID_SEPARATOR) }) {
                        file.delete()
                    }
                }
            }
        }

		private fun getBaseUrl(link: String): String {
			var baseUrl = link
			val index = link.indexOf('/', 8) // this also covers https://
			if (index > -1) {
				baseUrl = link.substring(0, index)
			}

			return baseUrl
		}
	}

	private val handler = Handler()

	public override fun onHandleIntent(intent: Intent?) {
		if (intent == null) { // No intent, we quit
			return
		}

		val isFromAutoRefresh = intent.getBooleanExtra(FROM_AUTO_REFRESH, false)

		// Connectivity issue, we quit
		if (!isOnline()) {
			if (ACTION_REFRESH_FEEDS == intent.action && !isFromAutoRefresh) {
				// Display a toast in that case
				handler.post { toast(R.string.network_error).show() }
			}
			return
		}

		fetch(this, isFromAutoRefresh, intent.action!!, intent.getLongExtra(EXTRA_FEED_ID, 0L))
	}
}

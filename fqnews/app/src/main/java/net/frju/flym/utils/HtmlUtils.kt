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

package net.frju.flym.utils

import android.content.Intent
import android.text.TextUtils
import net.frju.flym.App
import net.frju.flym.service.FetcherService
import org.jetbrains.anko.doAsync
import org.jsoup.Jsoup
import org.jsoup.safety.Whitelist
import java.io.File
import java.util.ArrayList
import java.util.regex.Pattern

object HtmlUtils {

	private val JSOUP_WHITELIST = Whitelist.relaxed().addTags("iframe", "video", "audio", "source", "track")
			.addAttributes("iframe", "src", "frameborder")
			.addAttributes("video", "src", "controls", "poster")
			.addAttributes("audio", "src", "controls")
			.addAttributes("source", "src", "type")
			.addAttributes("track", "src", "kind", "srclang", "label")
			.addAttributes("p", "style")
			.removeAttributes("img", "height", "width")

	private const val URL_SPACE = "%20"

	private val IMG_PATTERN = Pattern.compile("<img\\s+[^>]*src=\\s*['\"]([^'\"]+)['\"][^>]*>", Pattern.CASE_INSENSITIVE)
	private val ADS_PATTERN = Pattern.compile("<div class=('|\")mf-viral('|\")><table border=('|\")0('|\")>.*", Pattern.CASE_INSENSITIVE)
	private val SRCSET_PATTERN = Pattern.compile("\\s+srcset=\\s*['\"]([^'\"\\s]+)[^'\"]*['\"]", Pattern.CASE_INSENSITIVE)
	private val LAZY_LOADING_PATTERN = Pattern.compile("\\s+src=[^>]+\\s+original[-]*src=(\"|')", Pattern.CASE_INSENSITIVE)
	private val PIXEL_IMAGE_PATTERN = Pattern.compile("<img\\s+(height=['\"]1['\"]\\s+width=['\"]1['\"]|width=['\"]1['\"]\\s+height=['\"]1['\"])\\s+[^>]*src=\\s*['\"]([^'\"]+)['\"][^>]*>", Pattern.CASE_INSENSITIVE)
	private val NON_HTTP_IMAGE_PATTERN = Pattern.compile("\\s+(href|src)=(\"|')//", Pattern.CASE_INSENSITIVE)
	private val BAD_IMAGE_PATTERN = Pattern.compile("<img\\s+[^>]*src=\\s*['\"]([^'\"]+)\\.img['\"][^>]*>", Pattern.CASE_INSENSITIVE)
	private val EMPTY_IMAGE_PATTERN = Pattern.compile("<img((?!src=).)*?>", Pattern.CASE_INSENSITIVE)
	private val START_BR_PATTERN = Pattern.compile("^(\\s*<br\\s*[/]*>\\s*)*", Pattern.CASE_INSENSITIVE)
	private val END_BR_PATTERN = Pattern.compile("(\\s*<br\\s*[/]*>\\s*)*$", Pattern.CASE_INSENSITIVE)
	private val MULTIPLE_BR_PATTERN = Pattern.compile("(\\s*<br\\s*[/]*>\\s*){3,}", Pattern.CASE_INSENSITIVE)
	private val EMPTY_LINK_PATTERN = Pattern.compile("<a\\s+[^>]*></a>", Pattern.CASE_INSENSITIVE)

	fun improveHtmlContent(content: String, baseUri: String): String {
		@Suppress("NAME_SHADOWING")
		var content = content

		// remove some ads
		content = ADS_PATTERN.matcher(content).replaceAll("")
		// take the first image in srcset links
		content = SRCSET_PATTERN.matcher(content).replaceAll(" src='$1'")
		// remove lazy loading images stuff
		content = LAZY_LOADING_PATTERN.matcher(content).replaceAll(" src=$2")

		// clean by JSoup
		content = Jsoup.clean(content, baseUri, JSOUP_WHITELIST)

		// remove empty or bad images
		content = PIXEL_IMAGE_PATTERN.matcher(content).replaceAll("")
		content = BAD_IMAGE_PATTERN.matcher(content).replaceAll("")
		content = EMPTY_IMAGE_PATTERN.matcher(content).replaceAll("")
		// remove empty links
		content = EMPTY_LINK_PATTERN.matcher(content).replaceAll("")
		// fix non http image paths
		content = NON_HTTP_IMAGE_PATTERN.matcher(content).replaceAll(" $1=$2http://")
		// remove trailing BR & too much BR
		content = START_BR_PATTERN.matcher(content).replaceAll("")
		content = END_BR_PATTERN.matcher(content).replaceAll("")
		content = MULTIPLE_BR_PATTERN.matcher(content).replaceAll("<br><br>")

		return content
	}

	fun getImageURLs(content: String): ArrayList<String> {
		val images = ArrayList<String>()

		if (!TextUtils.isEmpty(content)) {
			val matcher = IMG_PATTERN.matcher(content)

			while (matcher.find()) {
				matcher.group(1)?.replace(" ", URL_SPACE)?.let { images.add(it) }
			}
		}

		return images
	}

	fun replaceImageURLs(content: String, itemId: String): String {
		@Suppress("NAME_SHADOWING")
		var content = content

		if (!TextUtils.isEmpty(content)) {
			val needDownloadPictures = FetcherService.shouldDownloadPictures()
			val imagesToDl = ArrayList<String>()

			val matcher = IMG_PATTERN.matcher(content)
			while (matcher.find()) {
				matcher.group(1)?.replace(" ", URL_SPACE)?.let { match ->
					val imgPath = FetcherService.getDownloadedImagePath(itemId, match)
					if (File(imgPath).exists()) {
						content = content.replace(match, FILE_SCHEME + imgPath)
					} else if (needDownloadPictures) {
						imagesToDl.add(match)
					}
				}
			}

			// Download the images if needed
			if (!imagesToDl.isEmpty()) {
				doAsync {
					FetcherService.addImagesToDownload(mapOf(itemId to imagesToDl))
					App.context.startService(Intent(App.context, FetcherService::class.java).setAction(FetcherService.ACTION_DOWNLOAD_IMAGES))
				}
			}
		}

		return content
	}

	fun getMainImageURL(content: String): String? {
		if (!TextUtils.isEmpty(content)) {
			val matcher = IMG_PATTERN.matcher(content)

			while (matcher.find()) {
				val imgUrl = matcher.group(1)?.replace(" ", URL_SPACE)
				if (isCorrectImage(imgUrl)) {
					return imgUrl
				}
			}
		}

		return null
	}

	fun getMainImageURL(imgUrls: ArrayList<String>): String? {
		return imgUrls.firstOrNull { isCorrectImage(it) }
	}

	private fun isCorrectImage(imgUrl: String?): Boolean {
		if (imgUrl == null) {
			return false
		}

		if (!imgUrl.endsWith(".gif") && !imgUrl.endsWith(".GIF") && !imgUrl.endsWith(".img") && !imgUrl.endsWith(".IMG")) {
			return true
		}

		return false
	}
}

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

package net.frju.flym.ui.feeds

import android.os.Bundle
import android.view.View
import net.frju.flym.data.entities.Feed
import net.frju.flym.data.entities.FeedWithCount
import org.jetbrains.anko.sdk21.listeners.onClick


private const val STATE_SELECTED_ID = "STATE_SELECTED_ID"

open class FeedAdapter(groups: List<FeedGroup>) : BaseFeedAdapter(groups) {

	var selectedItemId = Feed.ALL_ENTRIES_ID
		set(newValue) {
			notifyParentDataSetChanged(true)
			field = newValue
		}

	override fun bindItem(itemView: View, feedWithCount: FeedWithCount) {
		itemView.isSelected = selectedItemId == feedWithCount.feed.id

		itemView.onClick {
			selectedItemId = feedWithCount.feed.id
			feedClickListener?.invoke(itemView, feedWithCount)
		}
	}

	override fun bindItem(itemView: View, group: FeedGroup) {
		itemView.isSelected = selectedItemId == group.feedWithCount.feed.id

		itemView.onClick {
			selectedItemId = group.feedWithCount.feed.id
			feedClickListener?.invoke(itemView, group.feedWithCount)
		}
	}

	override fun onSaveInstanceState(savedInstanceState: Bundle) {
		savedInstanceState.putLong(STATE_SELECTED_ID, selectedItemId)
	}

	override fun onRestoreInstanceState(savedInstanceState: Bundle?) {
		selectedItemId = savedInstanceState?.getLong(STATE_SELECTED_ID) ?: Feed.ALL_ENTRIES_ID
	}
}
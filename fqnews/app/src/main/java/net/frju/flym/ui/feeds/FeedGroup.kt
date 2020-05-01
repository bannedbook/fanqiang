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

import com.bignerdranch.expandablerecyclerview.model.Parent
import net.frju.flym.data.entities.FeedWithCount


class FeedGroup(val feedWithCount: FeedWithCount, val subFeeds: List<FeedWithCount>) : Parent<FeedWithCount> {

    override fun getChildList(): List<FeedWithCount> {
        return subFeeds
    }

    override fun isInitiallyExpanded(): Boolean {
        return false
    }

    // needed to preserve expansion state
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (javaClass != other?.javaClass) return false

        other as FeedGroup

        if (feedWithCount.feed.id != other.feedWithCount.feed.id) return false

        return true
    }

    override fun hashCode(): Int {
        return feedWithCount.feed.id.hashCode()
    }

    fun getEntryCountString(): String {
        val entryCount = if (subFeeds.isNotEmpty()) subFeeds.sumBy { it.entryCount } else feedWithCount.entryCount
        return if (entryCount > 0) entryCount.toString() else ""
    }
}

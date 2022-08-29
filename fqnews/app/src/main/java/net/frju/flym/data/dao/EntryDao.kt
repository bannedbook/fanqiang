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

package net.frju.flym.data.dao

import androidx.lifecycle.LiveData
import androidx.paging.DataSource
import androidx.room.*
import net.frju.flym.data.entities.Entry
import net.frju.flym.data.entities.EntryWithFeed

private const val LIGHT_SELECT = "id, entries.feedId, feedLink, feedTitle, fetchDate, publicationDate, title, link, description, imageLink, read, favorite"
private const val ORDER_BY = "ORDER BY publicationDate DESC, id"
private const val JOIN = "entries INNER JOIN feeds ON entries.feedId = feeds.feedId"
private const val OLDER = "fetchDate <= :maxDate"
private const val FEED_ID = "entries.feedId IS :feedId"
private const val LIKE_SEARCH = "LIKE '%' || :searchText || '%'"

@Dao
abstract class EntryDao {

	@Query("SELECT $LIGHT_SELECT FROM $JOIN WHERE title $LIKE_SEARCH OR description $LIKE_SEARCH OR mobilizedContent $LIKE_SEARCH $ORDER_BY")
	abstract fun observeSearch(searchText: String): DataSource.Factory<Int, EntryWithFeed>

	@Query("SELECT $LIGHT_SELECT FROM $JOIN WHERE $OLDER $ORDER_BY")
	abstract fun observeAll(maxDate: Long): DataSource.Factory<Int, EntryWithFeed>

	@Query("SELECT $LIGHT_SELECT FROM $JOIN WHERE $OLDER AND read = 0 $ORDER_BY")
	abstract fun observeAllUnreads(maxDate: Long): DataSource.Factory<Int, EntryWithFeed>

	@Query("SELECT $LIGHT_SELECT FROM $JOIN WHERE $OLDER AND favorite = 1 $ORDER_BY")
	abstract fun observeAllFavorites(maxDate: Long): DataSource.Factory<Int, EntryWithFeed>

	@Query("SELECT id FROM entries WHERE $OLDER $ORDER_BY")
	abstract fun observeAllIds(maxDate: Long): LiveData<List<String>>

	@Query("SELECT id FROM entries WHERE $OLDER AND read = 0 $ORDER_BY")
	abstract fun observeAllUnreadIds(maxDate: Long): LiveData<List<String>>

	@Query("SELECT id FROM entries WHERE $OLDER AND favorite = 1 $ORDER_BY")
	abstract fun observeAllFavoriteIds(maxDate: Long): LiveData<List<String>>

	@Query("SELECT $LIGHT_SELECT FROM $JOIN WHERE $FEED_ID AND $OLDER $ORDER_BY")
	abstract fun observeByFeed(feedId: Long, maxDate: Long): DataSource.Factory<Int, EntryWithFeed>

	@Query("SELECT $LIGHT_SELECT FROM $JOIN WHERE $FEED_ID AND $OLDER AND read = 0 $ORDER_BY")
	abstract fun observeUnreadsByFeed(feedId: Long, maxDate: Long): DataSource.Factory<Int, EntryWithFeed>

	@Query("SELECT $LIGHT_SELECT FROM $JOIN WHERE $FEED_ID AND $OLDER AND favorite = 1 $ORDER_BY")
	abstract fun observeFavoritesByFeed(feedId: Long, maxDate: Long): DataSource.Factory<Int, EntryWithFeed>

	@Query("SELECT id FROM entries WHERE title $LIKE_SEARCH OR description $LIKE_SEARCH OR mobilizedContent $LIKE_SEARCH $ORDER_BY")
	abstract fun observeIdsBySearch(searchText: String): LiveData<List<String>>

	@Query("SELECT id FROM entries WHERE feedId IS :feedId AND $OLDER $ORDER_BY")
	abstract fun observeIdsByFeed(feedId: Long, maxDate: Long): LiveData<List<String>>

	@Query("SELECT id FROM entries WHERE feedId IS :feedId AND $OLDER AND read = 0 $ORDER_BY")
	abstract fun observeUnreadIdsByFeed(feedId: Long, maxDate: Long): LiveData<List<String>>

	@Query("SELECT id FROM entries WHERE feedId IS :feedId AND $OLDER AND favorite = 1 $ORDER_BY")
	abstract fun observeFavoriteIdsByFeed(feedId: Long, maxDate: Long): LiveData<List<String>>

	@Query("SELECT $LIGHT_SELECT FROM $JOIN WHERE groupId IS :groupId AND $OLDER $ORDER_BY")
	abstract fun observeByGroup(groupId: Long, maxDate: Long): DataSource.Factory<Int, EntryWithFeed>

	@Query("SELECT $LIGHT_SELECT FROM $JOIN WHERE groupId IS :groupId AND $OLDER AND read = 0 $ORDER_BY")
	abstract fun observeUnreadsByGroup(groupId: Long, maxDate: Long): DataSource.Factory<Int, EntryWithFeed>

	@Query("SELECT $LIGHT_SELECT FROM $JOIN WHERE groupId IS :groupId AND $OLDER AND favorite = 1 $ORDER_BY")
	abstract fun observeFavoritesByGroup(groupId: Long, maxDate: Long): DataSource.Factory<Int, EntryWithFeed>

	@Query("SELECT id FROM $JOIN WHERE groupId IS :groupId AND $OLDER $ORDER_BY")
	abstract fun observeIdsByGroup(groupId: Long, maxDate: Long): LiveData<List<String>>

	@Query("SELECT id FROM $JOIN WHERE groupId IS :groupId AND $OLDER AND read = 0 $ORDER_BY")
	abstract fun observeUnreadIdsByGroup(groupId: Long, maxDate: Long): LiveData<List<String>>

	@Query("SELECT id FROM $JOIN WHERE groupId IS :groupId AND $OLDER AND favorite = 1 $ORDER_BY")
	abstract fun observeFavoriteIdsByGroup(groupId: Long, maxDate: Long): LiveData<List<String>>

	@Query("SELECT COUNT(*) FROM entries WHERE read = 0 AND fetchDate > :minDate")
	abstract fun observeNewEntriesCount(minDate: Long): LiveData<Long>

	@Query("SELECT COUNT(*) FROM entries WHERE $FEED_ID AND read = 0 AND fetchDate > :minDate")
	abstract fun observeNewEntriesCountByFeed(feedId: Long, minDate: Long): LiveData<Long>

	@Query("SELECT COUNT(*) FROM $JOIN WHERE groupId IS :groupId AND read = 0 AND fetchDate > :minDate")
	abstract fun observeNewEntriesCountByGroup(groupId: Long, minDate: Long): LiveData<Long>

	@get:Query("SELECT id FROM entries WHERE favorite = 1")
	abstract val favoriteIds: List<String>

	@get:Query("SELECT COUNT(*) FROM entries WHERE read = 0")
	abstract val countUnread: Long

	@Query("SELECT * FROM entries WHERE id IS :id LIMIT 1")
	abstract fun findById(id: String): Entry?

	@Query("SELECT * FROM $JOIN WHERE id IS :id LIMIT 1")
	abstract fun findByIdWithFeed(id: String): EntryWithFeed?

	@Query("SELECT title FROM entries WHERE title IN (:titles)")
	abstract fun findAlreadyExistingTitles(titles: List<String>): List<String>

	@Query("SELECT id FROM entries WHERE feedId IS (:feedId)")
	abstract fun idsForFeed(feedId: Long): List<String>

	@Query("UPDATE entries SET read = 1 WHERE id IN (:ids)")
	abstract fun markAsRead(ids: List<String>)

	@Query("UPDATE entries SET read = 0 WHERE id IN (:ids)")
	abstract fun markAsUnread(ids: List<String>)

	@Query("UPDATE entries SET read = 1 WHERE feedId = :feedId")
	abstract fun markAsRead(feedId: Long)

	@Query("UPDATE entries SET read = 1 WHERE feedId IN (SELECT feedId FROM feeds WHERE groupId = :groupId)")
	abstract fun markGroupAsRead(groupId: Long)

	@Query("UPDATE entries SET read = 1")
	abstract fun markAllAsRead()

	@Query("UPDATE entries SET favorite = 1 WHERE id IS :id")
	abstract fun markAsFavorite(id: String)

	@Query("UPDATE entries SET favorite = 0 WHERE id IS :id")
	abstract fun markAsNotFavorite(id: String)

	@Query("DELETE FROM entries WHERE fetchDate < :keepDateBorderTime AND favorite = 0 AND read = :read")
	abstract fun deleteOlderThan(keepDateBorderTime: Long, read: Long)

	@Insert(onConflict = OnConflictStrategy.IGNORE) // Ignore because we don't want to delete previously starred entries
	abstract fun insert(vararg entries: Entry)

	@Update
	abstract fun update(vararg entries: Entry)

	@Delete
	abstract fun delete(vararg entries: Entry)
}
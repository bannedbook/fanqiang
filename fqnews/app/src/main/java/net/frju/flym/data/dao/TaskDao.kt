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
import androidx.room.*
import net.frju.flym.data.entities.Task


@Dao
abstract class TaskDao {
	@get:Query("SELECT * FROM tasks")
	abstract val all: List<Task>

	@get:Query("SELECT * FROM tasks")
	abstract val observeAll: LiveData<List<Task>>

	@get:Query("SELECT * FROM tasks WHERE imageLinkToDl = ''")
	abstract val mobilizeTasks: List<Task>

	@Query("SELECT COUNT(*) FROM tasks WHERE imageLinkToDl = '' AND entryId = :itemId")
	abstract fun observeItemMobilizationTasksCount(itemId: String): LiveData<Int>

	@get:Query("SELECT * FROM tasks WHERE imageLinkToDl != ''")
	abstract val downloadTasks: List<Task>

	@Insert(onConflict = OnConflictStrategy.REPLACE)
	protected abstract fun insertInternal(task: Task)

	@Transaction
	open fun insert(vararg tasks: Task) {
		for (task in tasks) {
			try {
				insertInternal(task) // Needed to avoid failing on all insert if a single one is failing
			} catch (t: Throwable) {
			}
		}
	}

	@Update
	abstract fun update(vararg tasks: Task)

	@Delete
	abstract fun delete(vararg tasks: Task)
}
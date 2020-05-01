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

package net.frju.flym.data.entities

import android.os.Parcelable
import androidx.room.Entity
import androidx.room.ForeignKey
import androidx.room.Index
import kotlinx.android.parcel.Parcelize

@Parcelize
@Entity(tableName = "tasks",
		primaryKeys = ["entryId", "imageLinkToDl"],
		indices = [(Index(value = ["entryId"]))],
		foreignKeys = [(ForeignKey(entity = Entry::class,
				parentColumns = ["id"],
				childColumns = ["entryId"],
				onDelete = ForeignKey.CASCADE))])
data class Task(
		var entryId: String = "",
		var imageLinkToDl: String = "",
		var numberAttempt: Int = 0) : Parcelable

/*******************************************************************************
 *                                                                             *
 *  Copyright (C) 2020 by Max Lv <max.c.lv@gmail.com>                          *
 *  Copyright (C) 2020 by Mygod Studio <contact-shadowsocks-android@mygod.be>  *
 *                                                                             *
 *  This program is free software: you can redistribute it and/or modify       *
 *  it under the terms of the GNU General Public License as published by       *
 *  the Free Software Foundation, either version 3 of the License, or          *
 *  (at your option) any later version.                                        *
 *                                                                             *
 *  This program is distributed in the hope that it will be useful,            *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 *  GNU General Public License for more details.                               *
 *                                                                             *
 *  You should have received a copy of the GNU General Public License          *
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

package com.github.shadowsocks.utils

import androidx.recyclerview.widget.SortedList
import java.net.URL

abstract class BaseSorter<T> : SortedList.Callback<T>() {
    override fun onInserted(position: Int, count: Int) { }
    override fun areContentsTheSame(oldItem: T?, newItem: T?): Boolean = oldItem == newItem
    override fun onMoved(fromPosition: Int, toPosition: Int) { }
    override fun onChanged(position: Int, count: Int) { }
    override fun onRemoved(position: Int, count: Int) { }
    override fun areItemsTheSame(item1: T?, item2: T?): Boolean = item1 == item2
    override fun compare(o1: T?, o2: T?): Int =
            if (o1 == null) if (o2 == null) 0 else 1 else if (o2 == null) -1 else compareNonNull(o1, o2)
    abstract fun compareNonNull(o1: T, o2: T): Int
}

object URLSorter : BaseSorter<URL>() {
    private val ordering = compareBy<URL>({ it.host }, { it.port }, { it.file }, { it.protocol })
    override fun compareNonNull(o1: URL, o2: URL): Int = ordering.compare(o1, o2)
}

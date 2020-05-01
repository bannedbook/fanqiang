/*******************************************************************************
 *                                                                             *
 *  Copyright (C) 2018 by Max Lv <max.c.lv@gmail.com>                          *
 *  Copyright (C) 2018 by Mygod Studio <contact-shadowsocks-android@mygod.be>  *
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

import android.content.ClipData
import androidx.recyclerview.widget.SortedList

private sealed class ArrayIterator<out T> : Iterator<T> {
    abstract val size: Int
    abstract operator fun get(index: Int): T
    private var count = 0
    override fun hasNext() = count < size
    override fun next(): T = if (hasNext()) this[count++] else throw NoSuchElementException()
}

private class ClipDataIterator(private val data: ClipData) : ArrayIterator<ClipData.Item>() {
    override val size get() = data.itemCount
    override fun get(index: Int) = data.getItemAt(index)
}
fun ClipData.asIterable() = Iterable { ClipDataIterator(this) }

private class SortedListIterator<out T>(private val list: SortedList<T>) : ArrayIterator<T>() {
    override val size get() = list.size()
    override fun get(index: Int) = list[index]
}
fun <T> SortedList<T>.asIterable() = Iterable { SortedListIterator(this) }

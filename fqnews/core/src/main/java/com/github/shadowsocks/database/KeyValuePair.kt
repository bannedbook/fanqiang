/*******************************************************************************
 *                                                                             *
 *  Copyright (C) 2017 by Max Lv <max.c.lv@gmail.com>                          *
 *  Copyright (C) 2017 by Mygod Studio <contact-shadowsocks-android@mygod.be>  *
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

package com.github.shadowsocks.database

import androidx.room.*
import java.io.ByteArrayOutputStream
import java.nio.ByteBuffer

@Entity
class KeyValuePair() {
    companion object {
        const val TYPE_UNINITIALIZED = 0
        const val TYPE_BOOLEAN = 1
        const val TYPE_FLOAT = 2
        @Deprecated("Use TYPE_LONG.")
        const val TYPE_INT = 3
        const val TYPE_LONG = 4
        const val TYPE_STRING = 5
        const val TYPE_STRING_SET = 6
    }

    @androidx.room.Dao
    interface Dao {
        @Query("SELECT * FROM `KeyValuePair` WHERE `key` = :key")
        operator fun get(key: String): KeyValuePair?

        @Insert(onConflict = OnConflictStrategy.REPLACE)
        fun put(value: KeyValuePair): Long

        @Query("DELETE FROM `KeyValuePair` WHERE `key` = :key")
        fun delete(key: String): Int
    }

    @PrimaryKey
    var key: String = ""
    var valueType: Int = TYPE_UNINITIALIZED
    var value: ByteArray = ByteArray(0)

    val boolean: Boolean?
        get() = if (valueType == TYPE_BOOLEAN) ByteBuffer.wrap(value).get() != 0.toByte() else null
    val float: Float?
        get() = if (valueType == TYPE_FLOAT) ByteBuffer.wrap(value).float else null
    @Suppress("DEPRECATION")
    @Deprecated("Use long.", ReplaceWith("long"))
    val int: Int?
        get() = if (valueType == TYPE_INT) ByteBuffer.wrap(value).int else null
    val long: Long? get() = when (valueType) {
        @Suppress("DEPRECATION")
        TYPE_INT -> ByteBuffer.wrap(value).int.toLong()
        TYPE_LONG -> ByteBuffer.wrap(value).long
        else -> null
    }
    val string: String?
        get() = if (valueType == TYPE_STRING) String(value) else null
    val stringSet: Set<String>?
        get() = if (valueType == TYPE_STRING_SET) {
            val buffer = ByteBuffer.wrap(value)
            val result = HashSet<String>()
            while (buffer.hasRemaining()) {
                val chArr = ByteArray(buffer.int)
                buffer.get(chArr)
                result.add(String(chArr))
            }
            result
        } else null

    @Ignore
    constructor(key: String) : this() {
        this.key = key
    }

    // putting null requires using DataStore
    fun put(value: Boolean): KeyValuePair {
        valueType = TYPE_BOOLEAN
        this.value = ByteBuffer.allocate(1).put((if (value) 1 else 0).toByte()).array()
        return this
    }
    fun put(value: Float): KeyValuePair {
        valueType = TYPE_FLOAT
        this.value = ByteBuffer.allocate(4).putFloat(value).array()
        return this
    }
    @Suppress("DEPRECATION")
    @Deprecated("Use long.")
    fun put(value: Int): KeyValuePair {
        valueType = TYPE_INT
        this.value = ByteBuffer.allocate(4).putInt(value).array()
        return this
    }
    fun put(value: Long): KeyValuePair {
        valueType = TYPE_LONG
        this.value = ByteBuffer.allocate(8).putLong(value).array()
        return this
    }
    fun put(value: String): KeyValuePair {
        valueType = TYPE_STRING
        this.value = value.toByteArray()
        return this
    }
    fun put(value: Set<String>): KeyValuePair {
        valueType = TYPE_STRING_SET
        val stream = ByteArrayOutputStream()
        val intBuffer = ByteBuffer.allocate(4)
        for (v in value) {
            intBuffer.rewind()
            stream.write(intBuffer.putInt(v.length).array())
            stream.write(v.toByteArray())
        }
        this.value = stream.toByteArray()
        return this
    }
}

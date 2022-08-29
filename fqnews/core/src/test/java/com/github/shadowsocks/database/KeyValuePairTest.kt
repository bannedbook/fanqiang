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

import org.junit.Assert
import org.junit.Test

class KeyValuePairTest {
    @Test
    fun putAndGet() {
        val kvp = KeyValuePair()
        Assert.assertEquals(true, kvp.put(true).boolean)
        Assert.assertEquals(3f, kvp.put(3f).float)
        @Suppress("DEPRECATION")
        Assert.assertEquals(3L, kvp.put(3).long)
        Assert.assertEquals(3L, kvp.put(3L).long)
        Assert.assertEquals("3", kvp.put("3").string)
        val set = (0 until 3).map(Int::toString).toSet()
        Assert.assertEquals(set, kvp.put(set).stringSet)
        Assert.assertEquals(null, kvp.boolean)
    }
}

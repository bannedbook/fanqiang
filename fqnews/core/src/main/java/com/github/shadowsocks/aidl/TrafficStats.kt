/*******************************************************************************
 *                                                                             *
 *  Copyright (C) 2019 by Max Lv <max.c.lv@gmail.com>                          *
 *  Copyright (C) 2019 by Mygod Studio <contact-shadowsocks-android@mygod.be>  *
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

package com.github.shadowsocks.aidl

import android.os.Parcel
import android.os.Parcelable

data class TrafficStats(
        // Bytes per second
        var txRate: Long = 0L,
        var rxRate: Long = 0L,

        // Bytes for the current session
        var txTotal: Long = 0L,
        var rxTotal: Long = 0L
) : Parcelable {
    operator fun plus(other: TrafficStats) = TrafficStats(
            txRate + other.txRate, rxRate + other.rxRate,
            txTotal + other.txTotal, rxTotal + other.rxTotal)

    constructor(parcel: Parcel) : this(parcel.readLong(), parcel.readLong(), parcel.readLong(), parcel.readLong())
    override fun writeToParcel(parcel: Parcel, flags: Int) {
        parcel.writeLong(txRate)
        parcel.writeLong(rxRate)
        parcel.writeLong(txTotal)
        parcel.writeLong(rxTotal)
    }
    override fun describeContents() = 0

    companion object CREATOR : Parcelable.Creator<TrafficStats> {
        override fun createFromParcel(parcel: Parcel) = TrafficStats(parcel)
        override fun newArray(size: Int): Array<TrafficStats?> = arrayOfNulls(size)
    }
}

package com.github.shadowsocks.database

import SpeedUpVPN.VpnEncrypt
import androidx.room.*

@Entity
class SSRSub(
        @PrimaryKey(autoGenerate = true)
        var id: Long = 0,
        var url: String,
        var url_group: String,
        var status: Long = NORMAL
) {
    companion object {
        const val NORMAL: Long = 0
        const val EMPTY: Long = 1
        const val NETWORK_ERROR: Long = 2
        const val NAME_CHANGED: Long = 3
    }

    @androidx.room.Dao
    interface Dao {
        @Insert
        fun create(value: SSRSub): Long

        @Update
        fun update(value: SSRSub): Int

        @Query("SELECT * FROM `SSRSub` WHERE `id` = :id")
        operator fun get(id: Long): SSRSub?

        @Query("SELECT * FROM `SSRSub` WHERE `url_group` = :url_group")
        fun getByGroup(url_group: String): SSRSub?

        @Query("DELETE FROM `SSRSub` WHERE `id` = :id")
        fun delete(id: Long): Int

        @Query("SELECT * FROM `SSRSub`")
        fun getAll(): List<SSRSub>
    }

    val displayName
        get() = when (status) {
            NORMAL -> url_group
            EMPTY -> "Invalid Link($url_group)"
            NETWORK_ERROR -> "Network Error($url_group)"
            NAME_CHANGED->"Name Changed(old name: $url_group) Stop Update"
            else -> throw IllegalArgumentException("status: $status")
        }
    fun isBuiltin(): Boolean {
        return VpnEncrypt.vpnGroupName == url_group
    }
}
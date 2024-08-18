package com.nononsenseapps.feeder.db.room

import androidx.room.Dao
import androidx.room.Delete
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.Transaction
import kotlinx.coroutines.flow.Flow

@Dao
interface SyncDeviceDao {
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insert(device: SyncDevice): Long

    @Delete
    suspend fun delete(device: SyncDevice): Int

    @Query(
        """
            SELECT *
            FROM sync_device
            ORDER BY device_name
        """,
    )
    fun getDevices(): Flow<List<SyncDevice>>

    @Query(
        """
            DELETE FROM sync_device
        """,
    )
    fun deleteAll(): Int

    @Transaction
    suspend fun replaceDevices(devices: List<SyncDevice>) {
        deleteAll()
        devices.forEach {
            insert(it)
        }
    }
}

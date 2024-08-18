package com.nononsenseapps.feeder.db.room

import android.os.Build
import androidx.room.Dao
import androidx.room.Delete
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.Transaction
import androidx.room.Update
import com.nononsenseapps.feeder.crypto.AesCbcWithIntegrity
import kotlinx.coroutines.flow.Flow
import java.time.Instant

@Dao
interface SyncRemoteDao {
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insert(syncRemote: SyncRemote): Long

    @Update
    suspend fun update(syncRemote: SyncRemote): Int

    @Delete
    suspend fun delete(syncRemote: SyncRemote): Int

    @Query(
        """
            SELECT *
            FROM sync_remote
            WHERE id IS 1
        """,
    )
    suspend fun getSyncRemote(): SyncRemote?

    @Query(
        """
            SELECT *
            FROM sync_remote
            WHERE id IS 1
        """,
    )
    fun getSyncRemoteFlow(): Flow<SyncRemote?>

    @Query(
        """
            UPDATE sync_remote
            SET latest_message_timestamp = :timestamp
            WHERE id IS 1 AND latest_message_timestamp < :timestamp
        """,
    )
    suspend fun updateLastMessageTimestamp(timestamp: Instant): Int

    @Query(
        """
            DELETE FROM sync_remote WHERE id IS 1
        """,
    )
    suspend fun deleteSyncRemote(): Int

    @Transaction
    suspend fun replaceWithDefaultSyncRemote() {
        deleteSyncRemote()
        insert(
            SyncRemote(
                id = 1L,
                deviceName = Build.PRODUCT.ifBlank { Build.MODEL.ifBlank { Build.BRAND } },
                secretKey = AesCbcWithIntegrity.generateKey().toString(),
            ),
        )
    }
}

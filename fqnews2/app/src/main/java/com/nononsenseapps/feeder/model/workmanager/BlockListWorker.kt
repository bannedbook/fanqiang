package com.nononsenseapps.feeder.model.workmanager

import android.content.Context
import androidx.core.app.NotificationManagerCompat
import androidx.work.CoroutineWorker
import androidx.work.ForegroundInfo
import androidx.work.WorkerParameters
import com.nononsenseapps.feeder.db.room.BlocklistDao
import com.nononsenseapps.feeder.db.room.ID_UNSET
import com.nononsenseapps.feeder.ui.ARG_FEED_ID
import com.nononsenseapps.feeder.ui.ARG_ONLY_NEW
import com.nononsenseapps.feeder.util.logDebug
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.android.closestDI
import org.kodein.di.instance
import java.time.Instant

class BlockListWorker(val context: Context, workerParams: WorkerParameters) :
    CoroutineWorker(context, workerParams), DIAware {
    override val di: DI by closestDI(context)

    private val notificationManager: NotificationManagerCompat by instance()
    private val blocklistDao: BlocklistDao by instance()

    override suspend fun getForegroundInfo(): ForegroundInfo {
        return createForegroundInfo(context, notificationManager)
    }

    override suspend fun doWork(): Result {
        logDebug(LOG_TAG, "Doing work...")

        val onlyNew = inputData.getBoolean(ARG_ONLY_NEW, false)
        val feedId = inputData.getLong(ARG_FEED_ID, ID_UNSET)

        when {
            feedId != ID_UNSET -> blocklistDao.setItemBlockStatusForNewInFeed(feedId, Instant.now())
            onlyNew -> blocklistDao.setItemBlockStatusWhereNull(Instant.now())
            else -> blocklistDao.setItemBlockStatus(Instant.now())
        }

        logDebug(LOG_TAG, "Work done!")
        return Result.success()
    }

    companion object {
        const val LOG_TAG = "FEEDER_BLOCKLIST"
        const val UNIQUE_BLOCKLIST_NAME = "feeder_blocklist_worker"
    }
}

package com.nononsenseapps.feeder.model.workmanager

import android.content.Context
import android.util.Log
import androidx.core.app.NotificationManagerCompat
import androidx.work.CoroutineWorker
import androidx.work.ForegroundInfo
import androidx.work.WorkerParameters
import com.nononsenseapps.feeder.sync.SyncRestClient
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.android.closestDI
import org.kodein.di.instance

class SyncServiceSendReadWorker(val context: Context, workerParams: WorkerParameters) :
    CoroutineWorker(context, workerParams), DIAware {
    override val di: DI by closestDI(context)

    private val notificationManager: NotificationManagerCompat by instance()
    private val syncClient: SyncRestClient by di.instance()

    override suspend fun getForegroundInfo(): ForegroundInfo {
        return createForegroundInfo(context, notificationManager)
    }

    override suspend fun doWork(): Result {
        return try {
            Log.d(LOG_TAG, "Doing work")
            syncClient.markAsRead()
                .onLeft {
                    Log.e(
                        LOG_TAG,
                        "Error when sending readmarks ${it.code}, ${it.body}",
                        it.throwable,
                    )
                }
                .fold(
                    ifLeft = { Result.failure() },
                    ifRight = { Result.success() },
                )
        } catch (e: Exception) {
            Log.e(LOG_TAG, "Error when sending read marks", e)
            Result.failure()
        }
    }

    companion object {
        private const val LOG_TAG = "FEEDER_SENDREAD"
        const val UNIQUE_SENDREAD_NAME = "feeder_sendread_onetime"
    }
}

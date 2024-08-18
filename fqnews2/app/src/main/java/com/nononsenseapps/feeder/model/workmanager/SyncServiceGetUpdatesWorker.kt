package com.nononsenseapps.feeder.model.workmanager

import android.content.Context
import android.util.Log
import androidx.core.app.NotificationManagerCompat
import androidx.work.Constraints
import androidx.work.CoroutineWorker
import androidx.work.ExistingWorkPolicy
import androidx.work.ForegroundInfo
import androidx.work.NetworkType
import androidx.work.OneTimeWorkRequestBuilder
import androidx.work.WorkManager
import androidx.work.WorkerParameters
import com.nononsenseapps.feeder.archmodel.Repository
import com.nononsenseapps.feeder.model.workmanager.SyncServiceGetUpdatesWorker.Companion.UNIQUE_GETUPDATES_NAME
import com.nononsenseapps.feeder.sync.SyncRestClient
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.android.closestDI
import org.kodein.di.instance
import java.util.concurrent.TimeUnit

class SyncServiceGetUpdatesWorker(val context: Context, workerParams: WorkerParameters) :
    CoroutineWorker(context, workerParams), DIAware {
    override val di: DI by closestDI(context)

    private val notificationManager: NotificationManagerCompat by instance()
    private val syncClient: SyncRestClient by instance()
    private val repository: Repository by instance()

    override suspend fun getForegroundInfo(): ForegroundInfo {
        return createForegroundInfo(context, notificationManager)
    }

    override suspend fun doWork(): Result {
        return try {
            Log.d(LOG_TAG, "Doing work")
            syncClient.getRead()
            repository.applyRemoteReadMarks()
            syncClient.getDevices()
            Result.success()
        } catch (e: Exception) {
            Log.e(LOG_TAG, "Error when getting updates", e)
            Result.failure()
        }
    }

    companion object {
        const val LOG_TAG = "FEEDER_GETUPDATES"
        const val UNIQUE_GETUPDATES_NAME = "feeder_getupdates_worker"
    }
}

fun scheduleGetUpdates(di: DI) {
    Log.d(SyncServiceGetUpdatesWorker.LOG_TAG, "Scheduling work")
    val repository by di.instance<Repository>()

    val constraints =
        Constraints.Builder()
            // This prevents expedited if true
            .setRequiresCharging(repository.syncOnlyWhenCharging.value)

    if (repository.syncOnlyOnWifi.value) {
        constraints.setRequiredNetworkType(NetworkType.UNMETERED)
    } else {
        constraints.setRequiredNetworkType(NetworkType.CONNECTED)
    }

    val workRequest =
        OneTimeWorkRequestBuilder<SyncServiceGetUpdatesWorker>()
            .addTag("feeder")
            .keepResultsForAtLeast(5, TimeUnit.MINUTES)
            .setConstraints(constraints.build())

    val workManager by di.instance<WorkManager>()
    workManager.enqueueUniqueWork(
        UNIQUE_GETUPDATES_NAME,
        ExistingWorkPolicy.REPLACE,
        workRequest.build(),
    )
}

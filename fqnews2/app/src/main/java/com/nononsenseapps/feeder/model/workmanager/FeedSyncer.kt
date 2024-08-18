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
import androidx.work.OutOfQuotaPolicy
import androidx.work.WorkManager
import androidx.work.WorkerParameters
import androidx.work.workDataOf
import com.nononsenseapps.feeder.archmodel.Repository
import com.nononsenseapps.feeder.db.room.ID_UNSET
import com.nononsenseapps.feeder.model.RssLocalSync
import com.nononsenseapps.feeder.model.notify
import com.nononsenseapps.feeder.ui.ARG_FEED_ID
import com.nononsenseapps.feeder.ui.ARG_FEED_TAG
import io.nekohasekai.sagernet.database.SagerDatabase
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.android.closestDI
import org.kodein.di.instance
import java.util.concurrent.TimeUnit

const val ARG_FORCE_NETWORK = "force_network"

const val UNIQUE_PERIODIC_NAME = "feeder_periodic_3"

// Clear this for scheduler
val oldPeriodics =
    listOf(
        "feeder_periodic",
        "feeder_periodic_2",
    )
private const val UNIQUE_FEEDSYNC_NAME = "feeder_sync_onetime"
private const val MIN_FEED_AGE_MINUTES = "min_feed_age_minutes"

class FeedSyncer(val context: Context, workerParams: WorkerParameters) :
    CoroutineWorker(context, workerParams), DIAware {
    override val di: DI by closestDI(context)

    private val notificationManager: NotificationManagerCompat by instance()
    private val rssLocalSync: RssLocalSync by instance()

    override suspend fun getForegroundInfo(): ForegroundInfo {
        return createForegroundInfo(context, notificationManager)
    }

    override suspend fun doWork(): Result {
        var success: Boolean
        var feedId: Long =0
        var feedTag =""
        try {
            feedId = inputData.getLong(ARG_FEED_ID, ID_UNSET)
            feedTag = inputData.getString(ARG_FEED_TAG) ?: ""
            val forceNetwork = inputData.getBoolean(ARG_FORCE_NETWORK, false)
            val minFeedAgeMinutes = inputData.getInt(MIN_FEED_AGE_MINUTES, 5)

            success =
                rssLocalSync.syncFeeds(
                    feedId = feedId,
                    feedTag = feedTag,
                    forceNetwork = forceNetwork,
                    minFeedAgeMinutes = minFeedAgeMinutes,
                )
        } catch (e: Exception) {
            success = false
            Log.e("FeederFeedSyncer", "Failure during sync for feedId: ${feedId}, feedTag: $feedTag", e)
        } finally {
            // Send notifications for configured feeds
            notify(applicationContext)
        }

        return when (success) {
            true -> Result.success()
            false -> Result.failure()
        }
    }
}

fun requestFeedSync(
    di: DI,
    feedId: Long = ID_UNSET,
    feedTag: String = "",
    forceNetwork: Boolean = false,
) {
    Log.d("FeederFeedSyncer", "requestFeedSync 1")
    val repository: Repository by di.instance()
    Log.d("FeederFeedSyncer", "requestFeedSync 2")
    if (!repository.settingsStore.addedFeederNews.value)return
    Log.d("FeederFeedSyncer", "requestFeedSync 3")
    if (SagerDatabase.proxyDao.getAvailable().isEmpty())return
    Log.d("FeederFeedSyncer", "requestFeedSync 4")
    //if(!DataStore.serviceState.started)return
    Log.d("FeederFeedSyncer", "requestFeedSync 5")

    val constraints = Constraints.Builder()

    if (!forceNetwork && repository.syncOnlyOnWifi.value) {
        constraints.setRequiredNetworkType(NetworkType.UNMETERED)
    } else {
        constraints.setRequiredNetworkType(NetworkType.CONNECTED)
    }

    val data =
        workDataOf(
            ARG_FEED_ID to feedId,
            ARG_FEED_TAG to feedTag,
            ARG_FORCE_NETWORK to forceNetwork,
        )

    val workRequest = OneTimeWorkRequestBuilder<FeedSyncer>()
            .addTag("feeder")
            .setExpedited(OutOfQuotaPolicy.RUN_AS_NON_EXPEDITED_WORK_REQUEST)
            .keepResultsForAtLeast(5, TimeUnit.MINUTES)
            .setConstraints(constraints.build())
            .setInputData(data)
            .build()
    val workManager by di.instance<WorkManager>()
    Log.d("FeederFeedSyncer", "requestFeedSync 6")
    workManager.enqueueUniqueWork(
        UNIQUE_FEEDSYNC_NAME,
        ExistingWorkPolicy.KEEP,
        workRequest,
    )
    Log.d("FeederFeedSyncer", "requestFeedSync 7")
}

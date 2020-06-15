package com.github.shadowsocks.work

import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.net.Uri
import androidx.core.app.NotificationCompat
import androidx.core.content.ContextCompat
import androidx.core.content.getSystemService
import androidx.work.*
import com.github.shadowsocks.Core
import com.github.shadowsocks.Core.app
import com.github.shadowsocks.core.BuildConfig
import com.github.shadowsocks.core.R
import com.github.shadowsocks.preference.DataStore
import com.github.shadowsocks.utils.printLog
import com.github.shadowsocks.utils.useCancellable
import com.google.gson.JsonStreamParser
import java.net.HttpURLConnection
import java.net.URL
import java.util.concurrent.TimeUnit


class UpdateCheck(context: Context, workerParams: WorkerParameters) : CoroutineWorker(context, workerParams) {
    val url =context.resources.getString(R.string.update_check_url)
    companion object {
        fun enqueue() = WorkManager.getInstance(Core.deviceStorage).enqueueUniquePeriodicWork(
                "UpdateCheck", ExistingPeriodicWorkPolicy.KEEP,
                PeriodicWorkRequestBuilder<UpdateCheck>(1, TimeUnit.DAYS).run {
                    setConstraints(Constraints.Builder()
                            .setRequiredNetworkType(NetworkType.UNMETERED)
                            .setRequiresCharging(false)
                            .build())
                    build()
                })
    }

    override suspend fun doWork(): Result = try {
        val connection = URL(url).openConnection() as HttpURLConnection
        val json = connection.useCancellable { inputStream.bufferedReader() }
        val info = JsonStreamParser(json).asSequence().single().asJsonObject
        if (info["version"].asInt > BuildConfig.VERSION_CODE) {
            val nm = app.getSystemService<NotificationManager>()!!
            val intent = Intent(Intent.ACTION_VIEW).setData(Uri.parse(info["uri"].asString))
            val builder = NotificationCompat.Builder(app as Context, "update")
                    .setColor(ContextCompat.getColor(app, R.color.material_primary_500))
                    .setContentIntent(PendingIntent.getActivity(app, 0, intent, 0))
                    .setVisibility(if (DataStore.canToggleLocked) NotificationCompat.VISIBILITY_PUBLIC
                    else NotificationCompat.VISIBILITY_PRIVATE)
                    .setSmallIcon(R.drawable.ic_service_active)
                    .setCategory(NotificationCompat.CATEGORY_STATUS)
                    .setContentTitle(info["title"].asString)
                    .setContentText(info["text"].asString)
                    .setAutoCancel(true)
            nm.notify(62, builder.build())
        }
        Result.success()
    } catch (e: Exception) {
        printLog(e)
        if (runAttemptCount > 5) Result.failure() else Result.retry()
    }
}

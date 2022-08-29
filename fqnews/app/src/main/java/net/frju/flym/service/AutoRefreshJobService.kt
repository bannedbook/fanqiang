/*
 * Copyright (c) 2012-2018 Frederic Julian
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http:></http:>//www.gnu.org/licenses/>.
 */

package net.frju.flym.service

import android.app.job.JobInfo
import android.app.job.JobParameters
import android.app.job.JobScheduler
import android.app.job.JobService
import android.content.ComponentName
import android.content.Context
import android.os.Build
import android.util.Log
import net.frju.flym.data.utils.PrefConstants
import net.frju.flym.utils.getPrefBoolean
import net.frju.flym.utils.getPrefString
import org.jetbrains.anko.doAsync

class AutoRefreshJobService : JobService() {
	companion object {
		var ignoreNextJob = false

		private const val TWO_HOURS = "7200"
		private const val JOB_ID = 0

		fun initAutoRefresh(context: Context) {
			Log.e("Refresh init", "......")
			// DO NOT USE ANKO TO RETRIEVE THE SERVICE HERE (crash on API 21)
			val jobSchedulerService = context.getSystemService(Context.JOB_SCHEDULER_SERVICE) as JobScheduler

            val time = Math.max(300, context.getPrefString(PrefConstants.REFRESH_INTERVAL, TWO_HOURS)!!.toInt())

			if (context.getPrefBoolean(PrefConstants.REFRESH_ENABLED, true)) {
				val builder = JobInfo.Builder(JOB_ID, ComponentName(context, AutoRefreshJobService::class.java))
						.setPeriodic(time * 1000L)
						.setPersisted(true)
						.setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY)

				if (Build.VERSION.SDK_INT >= 26) {
					builder.setRequiresBatteryNotLow(true)
							.setRequiresStorageNotLow(true)
				}

				ignoreNextJob = true // We can't add a initial delay with JobScheduler, so let's do this little hack instead
				jobSchedulerService.schedule(builder.build())
			} else {
				jobSchedulerService.cancel(JOB_ID)
			}
		}
	}

	override fun onStartJob(params: JobParameters): Boolean {
		val isRefreshing = getPrefBoolean(PrefConstants.IS_REFRESHING, false)
		Log.e("Refresh onStartJob", "ignoreNextJob:$ignoreNextJob, isFreshing:$isRefreshing")
		if (!ignoreNextJob && !isRefreshing) {
			doAsync {
				FetcherService.fetch(this@AutoRefreshJobService, true, FetcherService.ACTION_REFRESH_FEEDS)
				jobFinished(params, false)
			}
			return true
		}

		ignoreNextJob = false

		return false
	}

	override fun onStopJob(params: JobParameters): Boolean {
		return false
	}
}

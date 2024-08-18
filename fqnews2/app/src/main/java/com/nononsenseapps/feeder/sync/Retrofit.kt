package com.nononsenseapps.feeder.sync

import android.util.Log
import com.nononsenseapps.feeder.db.room.SyncRemote
import okhttp3.Credentials
import okhttp3.OkHttpClient
import retrofit2.Retrofit
import retrofit2.converter.moshi.MoshiConverterFactory
import retrofit2.create
import java.net.URL

fun getFeederSyncClient(
    syncRemote: SyncRemote,
    okHttpClient: OkHttpClient,
): FeederSync {
    val moshi = getMoshi()

    val retrofit =
        Retrofit.Builder()
            .client(
                okHttpClient.newBuilder()
                    // Auth only used to prevent automatic scanning of the API
                    .addInterceptor { chain ->
                        chain.proceed(
                            chain.request().newBuilder()
                                .header(
                                    "Authorization",
                                    Credentials.basic(HARDCODED_USER, HARDCODED_PASSWORD),
                                )
                                .build(),
                        )
                    }
                    .addInterceptor { chain ->
                        val response = chain.proceed(chain.request())
                        val isCachedResponse =
                            response.cacheResponse != null && (response.networkResponse == null || response.networkResponse?.code == 304)
                        Log.v(
                            "FEEDER_SYNC_CLIENT",
                            "Response cached: $isCachedResponse, ${response.networkResponse?.code}, cache-Control: ${response.cacheControl}",
                        )
                        response
                    }
                    .build(),
            )
            .baseUrl(URL(syncRemote.url, "/api/v1/"))
            .addConverterFactory(MoshiConverterFactory.create(moshi))
            .build()

    return retrofit.create()
}

private const val HARDCODED_USER = "feeder_user"
private const val HARDCODED_PASSWORD = "feeder_secret_1234"

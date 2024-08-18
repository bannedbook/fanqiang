package com.nononsenseapps.feeder.model

import com.nononsenseapps.feeder.util.logDebug
import okhttp3.Interceptor
import okhttp3.Response

object ForceCacheOnSomeFailuresInterceptor : Interceptor {
    override fun intercept(chain: Interceptor.Chain): Response {
        val response = chain.proceed(chain.request())
        // If server already set cache-control, don't override it
        if (response.headers("cache-control").isNotEmpty()) {
            return response
        }
        return when (response.code) {
            in 400..499 -> {
                logDebug("FEEDER", "cache-control forced for code ${response.code}")
                // Cache for 60 minutes
                // The intent is primarily to cache 404s for incorrect favicons
                // but all 4xx errors are probably wise not to hammer
                response
                    .newBuilder()
                    .header("cache-control", "max-age=3600")
                    // Remove any headers that might conflict with caching
                    .removeHeader("pragma")
                    .removeHeader("expires")
                    .removeHeader("x-cache")
                    .build()
            }
            else -> response
        }
    }
}

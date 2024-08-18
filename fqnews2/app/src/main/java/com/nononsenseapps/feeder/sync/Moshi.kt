package com.nononsenseapps.feeder.sync

import com.squareup.moshi.FromJson
import com.squareup.moshi.JsonAdapter
import com.squareup.moshi.Moshi
import com.squareup.moshi.ToJson
import com.squareup.moshi.kotlin.reflect.KotlinJsonAdapterFactory
import java.net.URL
import java.time.Instant

fun getMoshi(): Moshi =
    Moshi.Builder()
        .add(InstantAdapter())
        .add(URLAdapter())
        .addLast(KotlinJsonAdapterFactory())
        .build()

class InstantAdapter {
    @ToJson
    fun toJSon(value: Instant): Long = value.toEpochMilli()

    @FromJson
    fun fromJson(value: Long): Instant = Instant.ofEpochMilli(value)
}

class URLAdapter {
    @ToJson
    fun toJSon(value: URL): String = value.toString()

    @FromJson
    fun fromJson(value: String): URL = URL(value)
}

inline fun <reified T> Moshi.adapter(): JsonAdapter<T> = adapter(T::class.java)

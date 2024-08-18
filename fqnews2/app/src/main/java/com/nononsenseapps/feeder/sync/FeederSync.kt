package com.nononsenseapps.feeder.sync

import retrofit2.Response
import retrofit2.http.Body
import retrofit2.http.DELETE
import retrofit2.http.GET
import retrofit2.http.Header
import retrofit2.http.POST
import retrofit2.http.Path
import retrofit2.http.Query

interface FeederSync {
    @POST("create")
    suspend fun create(
        @Body request: CreateRequest,
    ): Response<JoinResponse>

    @POST("join")
    suspend fun join(
        @Header("X-FEEDER-ID") syncChainId: String,
        @Body request: JoinRequest,
    ): Response<JoinResponse>

    @GET("devices")
    suspend fun getDevices(
        @Header("X-FEEDER-ID") syncChainId: String,
        @Header("X-FEEDER-DEVICE-ID") currentDeviceId: Long,
    ): Response<DeviceListResponse>

    @DELETE("devices/{deviceId}")
    suspend fun removeDevice(
        @Header("X-FEEDER-ID") syncChainId: String,
        @Header("X-FEEDER-DEVICE-ID") currentDeviceId: Long,
        @Path("deviceId") deviceId: Long,
    ): Response<DeviceListResponse>

    @GET("readmark")
    suspend fun getReadMarks(
        @Header("X-FEEDER-ID") syncChainId: String,
        @Header("X-FEEDER-DEVICE-ID") currentDeviceId: Long,
        @Query("since") sinceMillis: Long,
    ): Response<GetReadMarksResponse>

    @POST("readmark")
    suspend fun sendReadMarks(
        @Header("X-FEEDER-ID") syncChainId: String,
        @Header("X-FEEDER-DEVICE-ID") currentDeviceId: Long,
        @Body request: SendReadMarkBulkRequest,
    ): Response<SendReadMarkResponse>

    @GET("ereadmark")
    suspend fun getEncryptedReadMarks(
        @Header("X-FEEDER-ID") syncChainId: String,
        @Header("X-FEEDER-DEVICE-ID") currentDeviceId: Long,
        @Query("since") sinceMillis: Long,
    ): Response<GetEncryptedReadMarksResponse>

    @POST("ereadmark")
    suspend fun sendEncryptedReadMarks(
        @Header("X-FEEDER-ID") syncChainId: String,
        @Header("X-FEEDER-DEVICE-ID") currentDeviceId: Long,
        @Body request: SendEncryptedReadMarkBulkRequest,
    ): Response<SendReadMarkResponse>

    @GET("feeds")
    suspend fun getFeeds(
        @Header("X-FEEDER-ID") syncChainId: String,
        @Header("X-FEEDER-DEVICE-ID") currentDeviceId: Long,
    ): Response<GetFeedsResponse>

    @POST("feeds")
    suspend fun updateFeeds(
        @Header("X-FEEDER-ID") syncChainId: String,
        @Header("X-FEEDER-DEVICE-ID") currentDeviceId: Long,
        @Header("If-Match") etagValue: String,
        @Body request: UpdateFeedsRequest,
    ): Response<UpdateFeedsResponse>
}

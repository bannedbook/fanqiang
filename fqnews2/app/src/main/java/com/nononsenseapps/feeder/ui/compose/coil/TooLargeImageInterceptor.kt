package com.nononsenseapps.feeder.ui.compose.coil

import coil.intercept.Interceptor
import coil.request.ErrorResult
import coil.request.ImageResult
import coil.request.SuccessResult

/**
 * Ensures an error is returned instead of rendering images that are likely to trigger memory errors
 * onDraw - but are not SO large as too cause a OOM exception during decode.
 */
class TooLargeImageInterceptor : Interceptor {
    override suspend fun intercept(chain: Interceptor.Chain): ImageResult {
        return when (val result = chain.proceed(chain.request)) {
            is ErrorResult -> result
            is SuccessResult -> {
                val sumPixels = result.drawable.intrinsicWidth * result.drawable.intrinsicHeight

                if (sumPixels > MAX_PIXELS) {
                    return ErrorResult(
                        chain.request.error,
                        chain.request,
                        RuntimeException(
                            "Image was (probably) too large to render within memory constraints: ${result.drawable.intrinsicWidth} x ${result.drawable.intrinsicHeight} > 2500 x 2500",
                        ),
                    )
                } else {
                    result
                }
            }
        }
    }

    companion object {
        const val MAX_PIXELS = 2500 * 2500
    }
}

package com.nononsenseapps.feeder.util

import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.SmallTest
import coil.ImageLoader
import coil.decode.DataSource
import coil.decode.ImageSource
import coil.fetch.SourceResult
import coil.request.Options
import com.danielrampelt.coil.ico.IcoDecoder
import kotlinx.coroutines.runBlocking
import okio.buffer
import okio.source
import org.junit.Test
import org.junit.runner.RunWith
import kotlin.test.assertNotNull

@RunWith(AndroidJUnit4::class)
@SmallTest
class IcoDecoderTest {
    private val factory = IcoDecoder.Factory(ApplicationProvider.getApplicationContext())

    @Test
    fun testPngFavicon() {
        val decoder =
            factory.create(
                pngIco,
                Options(ApplicationProvider.getApplicationContext()),
                ImageLoader(ApplicationProvider.getApplicationContext()),
            )

        assertNotNull(decoder)
        val result =
            runBlocking {
                decoder.decode()
            }
        assertNotNull(result)
    }

    @Test
    fun testGitlabIco() {
        val decoder =
            factory.create(
                pngIco,
                Options(ApplicationProvider.getApplicationContext()),
                ImageLoader(ApplicationProvider.getApplicationContext()),
            )

        assertNotNull(decoder)
        val result =
            runBlocking {
                decoder.decode()
            }
        assertNotNull(result)
    }

    companion object {
        private val gitlabIco: SourceResult
            get() {
                val buf =
                    Companion::class.java.getResourceAsStream("gitlab.ico")!!
                        .source()
                        .buffer()

                val imageSource =
                    ImageSource(
                        source = buf,
                        context = ApplicationProvider.getApplicationContext(),
                    )

                return SourceResult(
                    source = imageSource,
                    mimeType = null,
                    DataSource.DISK,
                )
            }

        private val pngIco: SourceResult
            get() {
                val buf =
                    Companion::class.java.getResourceAsStream("png.ico")!!
                        .source()
                        .buffer()

                val imageSource =
                    ImageSource(
                        source = buf,
                        context = ApplicationProvider.getApplicationContext(),
                    )

                return SourceResult(
                    source = imageSource,
                    mimeType = null,
                    DataSource.DISK,
                )
            }
    }
}

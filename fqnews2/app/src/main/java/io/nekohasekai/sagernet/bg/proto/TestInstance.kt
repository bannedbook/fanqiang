package io.nekohasekai.sagernet.bg.proto

import android.util.Log
import com.nononsenseapps.feeder.BuildConfig
import io.nekohasekai.sagernet.bg.GuardedProcessPool
import io.nekohasekai.sagernet.database.ProxyEntity
import io.nekohasekai.sagernet.fmt.buildConfig
import io.nekohasekai.sagernet.ktx.Logs
import io.nekohasekai.sagernet.ktx.runOnDefaultDispatcher
import io.nekohasekai.sagernet.ktx.tryResume
import io.nekohasekai.sagernet.ktx.tryResumeWithException
import kotlinx.coroutines.delay
import libcore.Libcore
import kotlin.coroutines.suspendCoroutine

class TestInstance(profile: ProxyEntity, val link: String, val timeout: Int) :
    BoxInstance(profile) {

    suspend fun doTest(): Int {
        return suspendCoroutine { c ->
            processes = GuardedProcessPool {
                Logs.w(it)
                c.tryResumeWithException(it)
            }
            runOnDefaultDispatcher {
                use {
                    try {
                        init()
                        launch()
                        if (processes.processCount > 0) {
                            // wait for plugin start
                            delay(500)
                        }
                        Log.e("urlTest","5-1")
                        if (box == null) {
                            Log.e("TestInstance", "Box is null before urlTest")
                            return@runOnDefaultDispatcher
                        }
                        val result = Libcore.urlTest(box, link, timeout)
                        Log.e("urlTest","5-2")
                        if (result == null) {
                            Log.e("TestInstance", "urlTest returned null")
                            c.tryResumeWithException(IllegalStateException("urlTest returned null"))
                            return@runOnDefaultDispatcher
                        }
                        c.tryResume(result)
                        Log.e("urlTest","5-3")
                    } catch (e: Exception) {
                        c.tryResumeWithException(e)
                    }
                }
            }
        }
    }

    override fun buildConfig() {
        config = buildConfig(profile, true)
    }

    override suspend fun loadConfig() {
        // don't call destroyAllJsi here
        //if (BuildConfig.DEBUG)
        Log.e("loadConfig", config.config)
        box = Libcore.newSingBoxInstance(config.config)
        box.forTest = true
    }

}

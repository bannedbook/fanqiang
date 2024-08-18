package app

import android.content.Context
import android.util.Log
import jww.app.FeederApplication
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.util.ToastMaker
import io.nekohasekai.sagernet.bg.proto.UrlTest
import io.nekohasekai.sagernet.database.DataStore
import io.nekohasekai.sagernet.database.ProfileManager
import io.nekohasekai.sagernet.database.SagerDatabase
import io.nekohasekai.sagernet.group.RawUpdater
import io.nekohasekai.sagernet.ktx.Logs
import io.nekohasekai.sagernet.ktx.onMainDispatcher
import io.nekohasekai.sagernet.ktx.readableMessage
import io.nekohasekai.sagernet.ktx.runOnDefaultDispatcher
import io.nekohasekai.sagernet.plugin.PluginManager
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.joinAll
import kotlinx.coroutines.launch
import kotlinx.coroutines.newFixedThreadPoolContext
import java.util.concurrent.ConcurrentLinkedQueue

class NodeImporter(
    private val context: Context,
    private val coroutineScope: CoroutineScope,
    private val toastMaker: ToastMaker
) {
    fun importNodes() {
        val text = (context.applicationContext as FeederApplication).getClipboardText()
        if (text.isBlank()) {
            coroutineScope.launch { toastMaker.makeToast(R.string.clipboard_empty) }
        } else {
            runOnDefaultDispatcher {
                try {
                    val proxies = RawUpdater.parseRaw(text)
                    if (proxies.isNullOrEmpty()) {
                        onMainDispatcher {
                            coroutineScope.launch { toastMaker.makeToast(R.string.no_proxies_found_in_clipboard) }
                        }
                    } else {
                        val targetId = DataStore.selectedGroupForImport()
                        for (proxy in proxies) {
                            ProfileManager.createProfile(targetId, proxy)
                        }
                        onMainDispatcher {
                            DataStore.editingGroup = targetId
                            coroutineScope.launch { toastMaker.makeToast(R.string.added) }
                        }
                        urlTest()
                        println("URL Test started, but we are not waiting for it to finish.")

/*                        val testJob = urlTest()
                        testJob.invokeOnCompletion { throwable ->
                            if (throwable == null) {
                                println("All tests completed successfully.")
                            } else {
                                println("Error occurred: ${throwable.message}")
                            }
                        }
                        println("Main thread continues without waiting for URL Test.")*/

                    }
                } catch (e: Exception) {
                    Logs.w(e)
                    onMainDispatcher {
                        coroutineScope.launch { toastMaker.makeToast(e.readableMessage) }
                    }
                }
            }
        }
    }

    fun urlTest(): Job {
        return GlobalScope.launch(Dispatchers.Default) { // 启动一个新协程，返回Job
            if (DataStore.serviceState.started) {
                FeederApplication.instance.stopService()
                delay(500) // wait for service stop
            }
            Log.e("urlTest", "1")
            //val group = DataStore.currentGroup()
            //val profilesUnfiltered = SagerDatabase.proxyDao.getByGroup(group.id)
            //val profiles = ConcurrentLinkedQueue(profilesUnfiltered)
            val profiles = ConcurrentLinkedQueue(SagerDatabase.proxyDao.getAll())

            // 使用新的协程上下文作为测试线程池
            val testPool = newFixedThreadPoolContext(
                DataStore.connectionTestConcurrent,
                "urlTest"
            )

            val tasks = mutableListOf<Job>()

            Log.e("urlTest", "2")
            repeat(DataStore.connectionTestConcurrent) {
                val job = launch(testPool) { // 使用launch而不是async
                    Log.e("urlTest", "3")
                    val urlTest = UrlTest() // note: this is NOT in bg process
                    Log.e("urlTest", "4")
                    while (isActive) {
                        val profile = profiles.poll() ?: break
                        Log.e("urlTest", "5 ${profile.displayName()} - ${profile.status}  - ${profile.ping}")
                        profile.status = 0
                        try {
                            val result = urlTest.doTest(profile)
                            Log.e("urlTest", "6")
                            profile.status = 1
                            profile.ping = result
                            SagerDatabase.proxyDao.updateProxy(profile)
                            Log.e("urlTest ping", profile.displayName() + " " + result.toString())
                            Log.e("urlTest avaible", SagerDatabase.proxyDao.getAvailable().size.toString())
                        } catch (e: PluginManager.PluginNotFoundException) {
                            Log.e("urlTest ", profile.displayName() + " " + e.toString())
                            e.printStackTrace()
                            profile.status = 2
                            profile.error = e.readableMessage
                            SagerDatabase.proxyDao.deleteProxy(profile)
                        } catch (e: Exception) {
                            Log.e("urlTest ", profile.displayName() + " " + e.toString())
                            e.printStackTrace()
                            profile.status = 3
                            profile.error = e.readableMessage
                            SagerDatabase.proxyDao.deleteProxy(profile)
                        }
                        Log.e("urlTest", "7")
                    }
                }
                tasks.add(job)
            }
            tasks.joinAll() // 等待所有任务完成
            //ConcurrentLinkedQueue 是一种线程安全的队列实现，但在多个线程对其进行操作时，其迭代结果可能不包含所有修改过的数据。因为在 tasks 的多个协程中，profiles.poll() 会不断移除元素，这意味着最终 profiles 中可能没有剩余的元素或者元素的状态没有更新到最新。
            //SagerDatabase.proxyDao.updateProxy(profiles.toList())
            Log.e("urlTest", "All tests completed. there are ${SagerDatabase.proxyDao.getAvailable().size} good nodes.")
        }
    }

    fun urlTestOld() {
        val mainJob = runOnDefaultDispatcher {
            if (DataStore.serviceState.started) {
                FeederApplication.instance.stopService()
                delay(500) // wait for service stop
            }
            Log.e("urlTest", "1")
            val group = DataStore.currentGroup()
            val profilesUnfiltered = SagerDatabase.proxyDao.getByGroup(group.id)
            val profiles = ConcurrentLinkedQueue(profilesUnfiltered)
            val testPool = newFixedThreadPoolContext(
                DataStore.connectionTestConcurrent,
                "urlTest"
            )
            Log.e("urlTest", "2")
            repeat(DataStore.connectionTestConcurrent) {
                Log.e("urlTest", "3")
                val urlTest = UrlTest() // note: this is NOT in bg process
                Log.e("urlTest", "4")
                while (isActive) {
                    val profile = profiles.poll() ?: break
                    profile.status = 0
                    Log.e("urlTest", "5")
                    try {
                        val result = urlTest.doTest(profile)
                        Log.e("urlTest", "6")
                        profile.status = 1
                        profile.ping = result
                        Log.e("urlTest ping", profile.displayName() + " " + result.toString())
                    } catch (e: PluginManager.PluginNotFoundException) {
                        profile.status = 2
                        profile.error = e.readableMessage
                    } catch (e: Exception) {
                        profile.status = 3
                        profile.error = e.readableMessage
                    }
                    Log.e("urlTest", "7")
                }
            }
        }
    }
}

package com.nononsenseapps.feeder.ui

import android.app.AlertDialog
import android.content.Intent
import android.os.Bundle
import android.util.Log
import android.view.Gravity
import android.widget.Toast
import androidx.activity.compose.setContent
import androidx.compose.animation.ExperimentalAnimationApi
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.core.util.Consumer
import androidx.core.view.WindowCompat
import androidx.lifecycle.lifecycleScope
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.rememberNavController
import app.Constants
import app.NodeImporter
import com.nononsenseapps.feeder.ApplicationCoroutineScope
import jww.app.FeederApplication
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.archmodel.Repository
import com.nononsenseapps.feeder.base.DIAwareComponentActivity
import com.nononsenseapps.feeder.model.workmanager.requestFeedSync
import com.nononsenseapps.feeder.model.workmanager.scheduleGetUpdates
import com.nononsenseapps.feeder.notifications.NotificationsWorker
import com.nononsenseapps.feeder.ui.compose.navigation.AddFeedDestination
import com.nononsenseapps.feeder.ui.compose.navigation.ArticleDestination
import com.nononsenseapps.feeder.ui.compose.navigation.EditFeedDestination
import com.nononsenseapps.feeder.ui.compose.navigation.FeedDestination
import com.nononsenseapps.feeder.ui.compose.navigation.SearchFeedDestination
import com.nononsenseapps.feeder.ui.compose.navigation.SettingsDestination
import com.nononsenseapps.feeder.ui.compose.navigation.SyncScreenDestination
import com.nononsenseapps.feeder.ui.compose.utils.withAllProviders
import com.nononsenseapps.feeder.util.ToastMaker
import io.nekohasekai.sagernet.database.DataStore
import io.nekohasekai.sagernet.database.SagerDatabase
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.launch
import org.kodein.di.direct
import org.kodein.di.instance
import java.io.IOException
import java.net.Socket
import kotlin.concurrent.thread


class MainActivity : DIAwareComponentActivity() {
    private val notificationsWorker: NotificationsWorker by instance()
    private val mainActivityViewModel: MainActivityViewModel by instance(arg = this)
    private val repository: Repository by instance()
    override fun onStart() {
        super.onStart()
        notificationsWorker.runForever()
    }

    override fun onStop() {
        notificationsWorker.stopForever()
        super.onStop()
    }

    override fun onResume() {
        super.onResume()
        mainActivityViewModel.setResumeTime()

        coroutineScope = di.direct.instance<ApplicationCoroutineScope>()
        // 直接创建 ToastMaker 实例
        toastMaker = di.direct.instance<ToastMaker>()
        // 创建 NodeImporter 实例
        if (! ::nodeImporter.isInitialized) {
            nodeImporter = NodeImporter(FeederApplication.instance, coroutineScope, toastMaker)
        }
        doInit()
        startPortMonitor()

        scheduleGetUpdates(di)
        maybeRequestSync()
        nodeImporter.urlTest()
    }
    private fun doInit(){
        if (!repository.settingsStore.addedFeederNews.value) {
            Constants.initializeFeeds(this) {
                // 这个回调在选择语言或取消对话框后被调用
                repository.addFeederNewsIfInitialStart()
                requestFeedSync(
                    di = di,
                    forceNetwork = true,
                )
            }
        }

        if (SagerDatabase.proxyDao.getAvailable().isEmpty()) {
            importNodesDialog()
        } else {
            nodeImporter.urlTest()
            FeederApplication.instance.startService()
        }
    }
    private fun maybeRequestSync() =
        lifecycleScope.launch {
            if (mainActivityViewModel.shouldSyncOnResume) {
                if (mainActivityViewModel.isOkToSyncAutomatically()) {
                    requestFeedSync(
                        di = di,
                        forceNetwork = false,
                    )
                }
            }
        }

    private lateinit var nodeImporter: NodeImporter
    private lateinit var coroutineScope: CoroutineScope
    private lateinit var toastMaker: ToastMaker
    private fun importNodesDialog() {
        AlertDialog.Builder(this)
            .setTitle("提醒-无可用节点")
            .setMessage(
                "本APP需要导入翻墙节点才能正常运行，请将节点拷贝到剪贴板后，然后点 确定 ，每行一个，数量不限，支持节点类型及格式如下:\n" +
                        "ss://...\n" +
                        "vmess://...\n" +
                        "vless://...\n" +
                        "trojan://..."
            )
            .setPositiveButton("导入节点") { dialog, _ ->
                dialog.dismiss() // 关闭对话框
                val text = (applicationContext as FeederApplication).getClipboardText()
                if (text.isBlank()) {
                    // 如果剪贴板为空
                    Toast.makeText(this, R.string.empty_feed_top, Toast.LENGTH_SHORT).show()
                    importNodesDialog() // 重新显示对话框
                } else {
                    // 如果剪贴板非空
                    nodeImporter.importNodes()
                    FeederApplication.instance.reloadService()
                    requestFeedSync(
                        di,
                        forceNetwork = true,
                    )
                }
            }
            .create()
            .show()
    }

    private fun startPortMonitor() {
        thread(start = true) {
            while (true) {
                if (SagerDatabase.proxyDao.getAvailable().isEmpty()) {
                    runOnUiThread {
                        val toast = Toast.makeText(this , "无可用节点，请导入节点", Toast.LENGTH_SHORT)
                        toast.setGravity(Gravity.TOP or Gravity.CENTER_HORIZONTAL, 0, 100) // 在屏幕顶部中央显示，向下偏移100像素
                        toast.show()
                    }
                    Thread.sleep(5000) // 每3秒检查一次
                    continue
                }
                var socket: Socket? = null
                try {
                    socket = Socket("127.0.0.1", DataStore.mixedPort)
                    // 检查连接是否成功
                    println("Port ${DataStore.mixedPort} is open.")
                } catch (e: IOException) {
                    FeederApplication.instance.startService()
                    println("Port ${DataStore.mixedPort} is not open. start proxy...")
                } finally {
                    socket?.close() // 确保在finally中关闭socket
                }
                // 休眠一段时间以避免频繁检测，单位为毫秒
                Thread.sleep(3000) // 每5秒检查一次
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        installExceptionHandler()
        mainActivityViewModel.ensurePeriodicSyncConfigured()
        WindowCompat.setDecorFitsSystemWindows(window, false)
        setContent {
            withAllProviders {
                AppContent()
            }
        }
    }

    @OptIn(ExperimentalAnimationApi::class)
    @Composable
    fun AppContent() {
        val navController = rememberNavController()
        val navDrawerListState = rememberLazyListState()

        NavHost(navController, startDestination = FeedDestination.route) {
            FeedDestination.register(this, navController, navDrawerListState)
            ArticleDestination.register(this, navController, navDrawerListState)
            // Feed editing
            EditFeedDestination.register(this, navController, navDrawerListState)
            SearchFeedDestination.register(this, navController, navDrawerListState)
            AddFeedDestination.register(this, navController, navDrawerListState)
            // Settings
            SettingsDestination.register(this, navController, navDrawerListState)
            // Sync settings
            SyncScreenDestination.register(this, navController, navDrawerListState)
        }

        DisposableEffect(navController) {
            val listener =
                Consumer<Intent> { intent ->
                    if (!navController.handleDeepLink(intent)) {
                        Log.e(LOG_TAG, "NavController rejected intent: $intent")
                    }
                }
            addOnNewIntentListener(listener)
            onDispose { removeOnNewIntentListener(listener) }
        }
    }

    companion object {
        private const val LOG_TAG = "FEEDER_MAIN"
    }
}

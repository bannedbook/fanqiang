package jww.app

import android.app.Application
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.ClipboardManager
import android.content.ContentResolver
import android.content.Context
import android.content.Intent
import android.content.SharedPreferences
import android.net.ConnectivityManager
import android.net.Network
import android.os.Build
import android.os.Build.VERSION.SDK_INT
import android.os.PowerManager
import android.os.UserManager
import android.util.Log
import android.widget.Toast
import androidx.annotation.RequiresApi
import androidx.core.app.NotificationManagerCompat
import androidx.core.content.ContextCompat
import androidx.core.content.getSystemService
import androidx.preference.PreferenceManager
import androidx.work.Configuration
import androidx.work.WorkManager
import coil.ImageLoader
import coil.ImageLoaderFactory
import coil.decode.GifDecoder
import coil.decode.ImageDecoderDecoder
import coil.decode.SvgDecoder
import coil.disk.DiskCache
import com.danielrampelt.coil.ico.IcoDecoder
import com.nononsenseapps.feeder.ApplicationCoroutineScope
import com.nononsenseapps.feeder.BuildConfig
import com.nononsenseapps.feeder.R
import com.nononsenseapps.feeder.archmodel.Repository
import com.nononsenseapps.feeder.db.room.AppDatabase
import com.nononsenseapps.feeder.db.room.BlocklistDao
import com.nononsenseapps.feeder.db.room.FeedDao
import com.nononsenseapps.feeder.db.room.FeedItemDao
import com.nononsenseapps.feeder.db.room.ReadStatusSyncedDao
import com.nononsenseapps.feeder.db.room.RemoteFeedDao
import com.nononsenseapps.feeder.db.room.RemoteReadMarkDao
import com.nononsenseapps.feeder.db.room.SyncDeviceDao
import com.nononsenseapps.feeder.db.room.SyncRemoteDao
import com.nononsenseapps.feeder.di.androidModule
import com.nononsenseapps.feeder.di.archModelModule
import com.nononsenseapps.feeder.di.networkModule
import com.nononsenseapps.feeder.model.ForceCacheOnSomeFailuresInterceptor
import com.nononsenseapps.feeder.model.TTSStateHolder
import com.nononsenseapps.feeder.model.UserAgentInterceptor
import com.nononsenseapps.feeder.notifications.NotificationsWorker
import com.nononsenseapps.feeder.ui.MainActivity
import com.nononsenseapps.feeder.ui.compose.coil.TooLargeImageInterceptor
import com.nononsenseapps.feeder.util.FilePathProvider
import com.nononsenseapps.feeder.util.ToastMaker
import com.nononsenseapps.feeder.util.currentlyUnmetered
import com.nononsenseapps.feeder.util.filePathProvider
import com.nononsenseapps.feeder.util.logDebug
import com.nononsenseapps.jsonfeed.cachingHttpClient
import go.Seq
import io.nekohasekai.sagernet.Action
import io.nekohasekai.sagernet.bg.SagerConnection
import io.nekohasekai.sagernet.database.DataStore
import io.nekohasekai.sagernet.ktx.runOnDefaultDispatcher
import io.nekohasekai.sagernet.utils.DefaultNetworkListener
import io.nekohasekai.sagernet.utils.PackageCache
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.cancel
import kotlinx.coroutines.withContext
import okhttp3.CacheControl
import okhttp3.OkHttpClient
import org.conscrypt.Conscrypt
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.bind
import org.kodein.di.direct
import org.kodein.di.instance
import org.kodein.di.singleton
import java.io.File
import java.security.Security
import java.util.concurrent.TimeUnit
import libcore.Libcore
import moe.matsuri.nb4a.NativeInterface
import moe.matsuri.nb4a.utils.JavaUtil
import moe.matsuri.nb4a.utils.cleanWebview

class FeederApplication : Application(), DIAware, ImageLoaderFactory {
    private val applicationCoroutineScope = ApplicationCoroutineScope()
    private val ttsStateHolder = TTSStateHolder(this, applicationCoroutineScope)

    override val di by DI.lazy {
        // import(androidXModule(this@FeederApplication))

        bind<FilePathProvider>() with
            singleton {
                filePathProvider(cacheDir = cacheDir, filesDir = filesDir)
            }
        bind<Application>() with singleton { this@FeederApplication }
        bind<AppDatabase>() with singleton { AppDatabase.getInstance(this@FeederApplication) }
        bind<FeedDao>() with singleton { instance<AppDatabase>().feedDao() }
        bind<FeedItemDao>() with singleton { instance<AppDatabase>().feedItemDao() }
        bind<SyncRemoteDao>() with singleton { instance<AppDatabase>().syncRemoteDao() }
        bind<ReadStatusSyncedDao>() with singleton { instance<AppDatabase>().readStatusSyncedDao() }
        bind<RemoteReadMarkDao>() with singleton { instance<AppDatabase>().remoteReadMarkDao() }
        bind<RemoteFeedDao>() with singleton { instance<AppDatabase>().remoteFeedDao() }
        bind<SyncDeviceDao>() with singleton { instance<AppDatabase>().syncDeviceDao() }
        bind<BlocklistDao>() with singleton { instance<AppDatabase>().blocklistDao() }

        import(androidModule)

        import(archModelModule)

        bind<WorkManager>() with singleton { WorkManager.getInstance(this@FeederApplication) }
        bind<ContentResolver>() with singleton { contentResolver }
        bind<ToastMaker>() with
            singleton {
                object : ToastMaker {
                    override suspend fun makeToast(text: String) =
                        withContext(Dispatchers.Main) {
                            Toast.makeText(this@FeederApplication, text, Toast.LENGTH_SHORT).show()
                        }

                    override suspend fun makeToast(resId: Int) =
                        withContext(Dispatchers.Main) {
                            Toast.makeText(this@FeederApplication, resId, Toast.LENGTH_SHORT).show()
                        }
                }
            }
        bind<NotificationManagerCompat>() with singleton { NotificationManagerCompat.from(this@FeederApplication) }
        bind<SharedPreferences>() with
            singleton {
                PreferenceManager.getDefaultSharedPreferences(
                    this@FeederApplication,
                )
            }

        bind<OkHttpClient>() with
            singleton {
                val filePathProvider = instance<FilePathProvider>()
                cachingHttpClient(
                    cacheDirectory = (filePathProvider.httpCacheDir),
                ) {
                    addNetworkInterceptor(UserAgentInterceptor)
                    addNetworkInterceptor(ForceCacheOnSomeFailuresInterceptor)
                    if (BuildConfig.DEBUG) {
                        addInterceptor { chain ->
                            val request = chain.request()
                            logDebug(
                                "FEEDER",
                                "Request ${request.url} headers [${request.headers}]",
                            )

                            chain.proceed(request).also {
                                logDebug(
                                    "FEEDER",
                                    "Response ${it.request.url} code ${it.networkResponse?.code} cached ${it.cacheResponse != null}",
                                )
                            }
                        }
                    }
                }
            }
        bind<ImageLoader>() with
            singleton {
                val filePathProvider = instance<FilePathProvider>()
                val repository = instance<Repository>()
                val okHttpClient =
                    instance<OkHttpClient>()
                        .newBuilder()
                        .addInterceptor { chain ->
                            chain.proceed(
                                when (!repository.loadImageOnlyOnWifi.value || currentlyUnmetered(this@FeederApplication)) {
                                    true -> chain.request()
                                    false -> {
                                        // Forces only cached responses to be used - if no cache then 504 is thrown
                                        chain.request().newBuilder()
                                            .cacheControl(
                                                CacheControl.Builder()
                                                    .onlyIfCached()
                                                    .maxStale(Int.MAX_VALUE, TimeUnit.SECONDS)
                                                    .maxAge(Int.MAX_VALUE, TimeUnit.SECONDS)
                                                    .build(),
                                            )
                                            .build()
                                    }
                                },
                            )
                        }
                        .build()

                ImageLoader.Builder(instance())
                    .dispatcher(Dispatchers.Default)
                    .okHttpClient(okHttpClient = okHttpClient)
                    .diskCache(
                        DiskCache.Builder()
                            .directory(filePathProvider.imageCacheDir)
                            .maxSizeBytes(250L * 1024 * 1024)
                            .build(),
                    )
                    .components {
                        add(TooLargeImageInterceptor())
                        add(SvgDecoder.Factory())
                        if (SDK_INT >= 28) {
                            add(ImageDecoderDecoder.Factory())
                        } else {
                            add(GifDecoder.Factory())
                        }
                        add(IcoDecoder.Factory(this@FeederApplication))
                    }
                    .build()
            }
        bind<ApplicationCoroutineScope>() with instance(applicationCoroutineScope)
        import(networkModule)
        bind<TTSStateHolder>() with instance(ttsStateHolder)
        bind<NotificationsWorker>() with singleton { NotificationsWorker(di) }
    }

    init {
        // Install Conscrypt to handle TLSv1.3 pre Android10
        // This crashes if app is installed to SD-card. Since it should still work for many
        // devices, try to ignore errors
        try {
            Security.insertProviderAt(Conscrypt.newProvider(), 1)
        } catch (t: Throwable) {
            Log.e(LOG_TAG, "Failed to insert Conscrypt. Attempt to ignore.", t)
        }
    }

    private val nativeInterface = NativeInterface()
    val externalAssets: File by lazy { getExternalFilesDir(null) ?: filesDir }
    override fun onCreate() {
        super.onCreate()
        instance = this
        @Suppress("DEPRECATION")
        staticFilesDir = filesDir

        // 手动初始化 WorkManager
        val configuration = Configuration.Builder()
            .setMinimumLoggingLevel(Log.INFO)
            .build()

        WorkManager.initialize(this, configuration)

        Seq.setContext(this)
        updateNotificationChannels()

        // nb4a: init core
        externalAssets.mkdirs()
        Libcore.initCore(
            process,
            cacheDir.absolutePath + "/",
            filesDir.absolutePath + "/",
            externalAssets.absolutePath + "/",
            DataStore.logBufSize,
            DataStore.logLevel > 0,
            nativeInterface, nativeInterface
        )

        if (isMainProcess || isBgProcess) {
            // fix multi process issue in Android 9+
            JavaUtil.handleWebviewDir(this)
            runOnDefaultDispatcher {
                DefaultNetworkListener.start(this) {
                    underlyingNetwork = it
                }
                PackageCache.register()
                cleanWebview()
            }
        }
    }

    override fun onTerminate() {
        applicationCoroutineScope.cancel("Application is being terminated")
        ttsStateHolder.shutdown()
        super.onTerminate()
    }

    companion object {
        @Deprecated("Only used by an old database migration")
        lateinit var staticFilesDir: File

        private const val LOG_TAG = "FEEDER_APP"

        lateinit var instance: FeederApplication
            private set
    }

    override fun newImageLoader(): ImageLoader {
        return di.direct.instance()
    }

    val user by lazy { instance.getSystemService<UserManager>()!! }
    val power by lazy { instance.getSystemService<PowerManager>()!! }
    val connectivity by lazy { instance.getSystemService<ConnectivityManager>()!! }
    val clipboard by lazy { instance.getSystemService<ClipboardManager>()!! }
    var underlyingNetwork: Network? = null
    val process: String = JavaUtil.getProcessName()
    val notification by lazy { instance.getSystemService<NotificationManager>()!! }
    private val isMainProcess = process == BuildConfig.APPLICATION_ID
    val isBgProcess = process.endsWith(":bg")
    fun getClipboardText(): String {
        return clipboard.primaryClip?.takeIf { it.itemCount > 0 }
            ?.getItemAt(0)?.text?.toString() ?: ""
    }

    val configureIntent: (Context) -> PendingIntent by lazy {
        {
            PendingIntent.getActivity(
                it,
                0,
                Intent(
                    instance, MainActivity::class.java
                ).setFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT),
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) PendingIntent.FLAG_IMMUTABLE else 0
            )
        }
    }
    fun updateNotificationChannels() {
        if (Build.VERSION.SDK_INT >= 26) @RequiresApi(26) {
            notification.createNotificationChannels(
                listOf(
                    NotificationChannel(
                        "service-vpn",
                        instance.getText(R.string.service_vpn),
                        if (Build.VERSION.SDK_INT >= 28) NotificationManager.IMPORTANCE_MIN
                        else NotificationManager.IMPORTANCE_LOW
                    ),   // #1355
                    NotificationChannel(
                        "service-proxy",
                        instance.getText(R.string.service_proxy),
                        NotificationManager.IMPORTANCE_LOW
                    ), NotificationChannel(
                        "service-subscription",
                        instance.getText(R.string.service_subscription),
                        NotificationManager.IMPORTANCE_DEFAULT
                    )
                )
            )
        }
    }

    fun startService() {
        val intent = Intent(instance, SagerConnection.serviceClass)
        // 添加日志记录
        Log.d("startService", "Starting service with Intent: $intent")
        try {
            ContextCompat.startForegroundService(instance, intent)
            Log.d("startService", "Service started successfully.")
        }
        catch (e: Exception){
            e.printStackTrace()
        }
    }

    fun reloadService() =
        instance.sendBroadcast(Intent(Action.RELOAD).setPackage(instance.packageName))

    fun stopService() =
        instance.sendBroadcast(Intent(Action.CLOSE).setPackage(instance.packageName))
}

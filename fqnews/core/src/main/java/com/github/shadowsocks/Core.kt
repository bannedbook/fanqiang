/*******************************************************************************
 *                                                                             *
 *  Copyright (C) 2018 by Max Lv <max.c.lv@gmail.com>                          *
 *  Copyright (C) 2018 by Mygod Studio <contact-shadowsocks-android@mygod.be>  *
 *                                                                             *
 *  This program is free software: you can redistribute it and/or modify       *
 *  it under the terms of the GNU General Public License as published by       *
 *  the Free Software Foundation, either version 3 of the License, or          *
 *  (at your option) any later version.                                        *
 *                                                                             *
 *  This program is distributed in the hope that it will be useful,            *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 *  GNU General Public License for more details.                               *
 *                                                                             *
 *  You should have received a copy of the GNU General Public License          *
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

package com.github.shadowsocks
import SpeedUpVPN.VpnEncrypt
import android.app.*
import android.app.admin.DevicePolicyManager
import android.content.Context
import android.content.DialogInterface
import android.content.Intent
import android.content.pm.PackageInfo
import android.content.pm.PackageManager
import android.net.ConnectivityManager
import android.net.NetworkCapabilities
import android.os.Build
import android.os.SystemClock
import android.os.UserManager
import android.util.Log
import android.view.Gravity
import android.widget.Toast
import androidx.annotation.RequiresApi
import androidx.annotation.VisibleForTesting
import androidx.core.content.ContextCompat
import androidx.core.content.getSystemService
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import androidx.work.Configuration
import androidx.work.WorkManager
import com.github.shadowsocks.acl.Acl
import com.github.shadowsocks.bg.ProxyService
import com.github.shadowsocks.bg.V2RayTestService
import com.github.shadowsocks.core.R
import com.github.shadowsocks.database.Profile
import com.github.shadowsocks.database.ProfileManager
import com.github.shadowsocks.database.SSRSubManager
import com.github.shadowsocks.net.TcpFastOpen
import com.github.shadowsocks.preference.DataStore
import com.github.shadowsocks.utils.*
import com.google.firebase.FirebaseApp
import com.google.firebase.analytics.FirebaseAnalytics
import kotlinx.coroutines.*
import me.dozen.dpreference.DPreference
import java.io.File
import java.io.IOException
import java.net.*
import kotlin.reflect.KClass

object Core {
    const val TAG = "Core"
    lateinit var app: Application
        @VisibleForTesting set
    val defaultDPreference by lazy { DPreference(app, app.packageName + "_preferences") }
    lateinit var configureIntent: (Context) -> PendingIntent
    val activity by lazy { app.getSystemService<ActivityManager>()!! }
    val connectivity by lazy { app.getSystemService<ConnectivityManager>()!! }
    val notification by lazy { app.getSystemService<NotificationManager>()!! }
    val packageInfo: PackageInfo by lazy { getPackageInfo(app.packageName) }
    val deviceStorage by lazy { if (Build.VERSION.SDK_INT < 24) app else DeviceStorageApp(app) }
    val directBootSupported by lazy {
        Build.VERSION.SDK_INT >= 24 && app.getSystemService<DevicePolicyManager>()?.storageEncryptionStatus ==
                DevicePolicyManager.ENCRYPTION_STATUS_ACTIVE_PER_USER
    }

    val activeProfileIds get() = ProfileManager.getProfile(DataStore.profileId).let {
        if (it == null) emptyList() else listOfNotNull(it.id, it.udpFallback)
    }
    val currentProfile: Pair<Profile, Profile?>? get() {
        if (DataStore.directBootAware) DirectBoot.getDeviceProfile()?.apply { return this }
        var theOne=ProfileManager.getProfile(DataStore.profileId)
        if (theOne==null){
            theOne=ProfileManager.getRandomVPNServer()
            if (theOne!=null)DataStore.profileId=theOne.id
        }
        return ProfileManager.expand(theOne ?: return null)
    }

    fun switchProfile(id: Long): Profile {
        val result = ProfileManager.getProfile(id) ?: ProfileManager.createProfile()
        DataStore.profileId = result.id
        return result
    }

    fun isInternetAvailable(context: Context): Boolean {
        return true
        //isInternetAvailable 此函数失效，原因不明，正常网络判断为网络不可用2020-11-12日之前

        var result = false
        val connectivityManager =
                context.getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            val networkCapabilities = connectivityManager.activeNetwork ?: return false
            val actNw =
                    connectivityManager.getNetworkCapabilities(networkCapabilities) ?: return false
            result = when {
                actNw.hasTransport(NetworkCapabilities.TRANSPORT_WIFI) -> true
                actNw.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR) -> true
                actNw.hasTransport(NetworkCapabilities.TRANSPORT_ETHERNET) -> true
                else -> false
            }
        } else {
            connectivityManager.run {
                connectivityManager.activeNetworkInfo?.run {
                    result = when (type) {
                        ConnectivityManager.TYPE_WIFI -> true
                        ConnectivityManager.TYPE_MOBILE -> true
                        ConnectivityManager.TYPE_ETHERNET -> true
                        else -> false
                    }

                }
            }
        }
        return result
    }

    fun checkLocalProxy(wait:Long=5000L){
        var ttt = 0
        while (tcping("127.0.0.1", DataStore.portProxy) < 0 || tcping("127.0.0.1", VpnEncrypt.HTTP_PROXY_PORT) < 0) {
            if (ttt == 10) break;
            Thread.sleep(500)
            ttt++
        }
        //Thread.sleep(wait)
    }
    //Import built-in subscription
    fun pickSingleServer(activity:Activity) = GlobalScope.async{
        Log.e("pickSingleServer ","...")

        if (!isInternetAvailable(app)){
            Log.e("isConnectedToNetwork ","not")
            activity.runOnUiThread(){alertMessage("网络不可用仅限离线阅读，连接互联网后，请重起本APP",activity)}
            Thread.sleep(5_000)
            LocalBroadcastManager.getInstance(activity).sendBroadcast(Intent(VpnEncrypt.ACTION_PROXY_START_COMPLETE))
            return@async
        }

        var testMsg="搜索服务器，请稍候"
        activity.runOnUiThread{showMessage(testMsg)}
        var  builtinSubUrls  = app.resources.getStringArray(R.array.builtinSubUrls)
        for (element in builtinSubUrls) {
            var builtinSub=SSRSubManager.createBuiltInSub(element)
            if (builtinSub != null) break
        }
        val profiles = ProfileManager.getAllProfilesByGroup(VpnEncrypt.vpnGroupName) ?: emptyList()
        if (profiles.isNullOrEmpty()) {
            Log.e("------","profiles empty, return@async")
            activity.runOnUiThread(){alertMessage("网络连接异常仅限离线阅读，连接互联网后，请重起本APP",activity)}
            Thread.sleep(5_000)
            LocalBroadcastManager.getInstance(activity).sendBroadcast(Intent(VpnEncrypt.ACTION_PROXY_START_COMPLETE))
            return@async
        }

        var i=(0..profiles.size).random()
        var theProfile= profiles[i] //随机取一个，避免集中压力到第一个
        switchProfile(theProfile.id)
        startService()
        checkLocalProxy()
        var selectedProfileDelay = testConnection2(theProfile)
        Log.e("test proxy:",theProfile.name+", delay:"+selectedProfileDelay)

        if(selectedProfileDelay>0 && selectedProfileDelay<3600000 && profiles.size==1){
            LocalBroadcastManager.getInstance(activity).sendBroadcast(Intent(VpnEncrypt.ACTION_PROXY_START_COMPLETE))
            return@async
        }

        var kkk=0
        var selecti=i
        while (i < profiles.size) {
            if (kkk==profiles.size)break
            kkk++
            i++
            if (i==profiles.size)i=0
            if (tcping(profiles[i].host, profiles[i].remotePort) <0)continue
            switchProfile(profiles[i].id)
            reloadService()

            testMsg+="."
            activity.runOnUiThread(){showMessage(testMsg)}
            checkLocalProxy()

            var delay = testConnection2(profiles[i])
            Log.e("test proxy:", profiles[i].name+", delay:"+delay)
            if (delay<selectedProfileDelay){
                selectedProfileDelay=delay
                selecti=i
            }

            if(selectedProfileDelay>0 && selectedProfileDelay<3600000 && kkk>=2){
                if (selecti!=i){
                    Log.e("select quik one",profiles[selecti].name.toString())
                    switchProfile(profiles[selecti].id)
                    reloadService()
                    checkLocalProxy()
                }
                else Log.e("last one is quik",profiles[selecti].name.toString())

                LocalBroadcastManager.getInstance(activity).sendBroadcast(Intent(VpnEncrypt.ACTION_PROXY_START_COMPLETE))
                return@async
            }
        }

        activity.runOnUiThread(){alertMessage("暂无可用服务器，仅限离线阅读，请检查网络状态后重启APP",activity)}
        Thread.sleep(5_000)
        LocalBroadcastManager.getInstance(activity).sendBroadcast(Intent(VpnEncrypt.ACTION_PROXY_START_COMPLETE))
    }

    private val URLConnection.responseLength: Long
        get() = if (Build.VERSION.SDK_INT >= 24) contentLengthLong else contentLength.toLong()

    fun testConnection2(server:Profile): Long {
        var result : Long = 3600000  // 1 hour
        var conn: HttpURLConnection? = null

        try {
            val url = URL("https",
                    "www.google.com",
                    "/generate_204")
            //Log.e("start test server",server.name + "...")
            //conn = url.openConnection(Proxy(Proxy.Type.HTTP,DataStore.httpProxyAddress)) as HttpURLConnection
            conn = url.openConnection(
                    Proxy(Proxy.Type.HTTP,
                            InetSocketAddress("127.0.0.1", VpnEncrypt.HTTP_PROXY_PORT))) as HttpURLConnection
            conn.setRequestProperty("User-Agent", "Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/79.0.3945.117 Safari/537.36")
            conn.connectTimeout = 5000
            conn.readTimeout = 5000
            conn.setRequestProperty("Connection", "close")
            conn.instanceFollowRedirects = false
            conn.useCaches = false

            val start = SystemClock.elapsedRealtime()
            val code = conn.responseCode
            val elapsed = SystemClock.elapsedRealtime() - start
            //Log.e("test server",server.name + " delay is "+ elapsed.toString())
            if (code == 204 || code == 200 && conn.responseLength == 0L) {
                result = elapsed
            } else {
                throw IOException(app.getString(R.string.connection_test_error_status_code, code))
            }
        }
        catch (e: IOException) {
            // network exception
            Log.e("Core:","testConnection2:"+e.toString())
        } catch (e: Exception) {
            // library exception, eg sumsung
            Log.e("Core-","testConnection Exception: "+Log.getStackTraceString(e))
        } finally {
            conn?.disconnect()
        }
        return result
    }
    /**
     * import free sub
     */
    fun importFreeSubs(): Boolean {
        try {
            GlobalScope.launch {
                var  freesuburl  = app.resources.getStringArray(R.array.freesuburl)
                for (i in freesuburl.indices) {
                    var freeSub=SSRSubManager.createSSSub(freesuburl[i])
                    if (freeSub != null) break
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
            return false
        }
        return true
    }

    fun init(app: Application, configureClass: KClass<out Any>) {
        this.app = app
        this.configureIntent = {
            PendingIntent.getActivity(it, 0, Intent(it, configureClass.java)
                    .setFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT), 0)
        }

        if (Build.VERSION.SDK_INT >= 24) {  // migrate old files
            deviceStorage.moveDatabaseFrom(app, Key.DB_PUBLIC)
            val old = Acl.getFile(Acl.CUSTOM_RULES, app)
            if (old.canRead()) {
                Acl.getFile(Acl.CUSTOM_RULES).writeText(old.readText())
                old.delete()
            }
        }

        // overhead of debug mode is minimal: https://github.com/Kotlin/kotlinx.coroutines/blob/f528898/docs/debugging.md#debug-mode
        System.setProperty(DEBUG_PROPERTY_NAME, DEBUG_PROPERTY_VALUE_ON)
        FirebaseApp.initializeApp(deviceStorage)
        WorkManager.initialize(deviceStorage, Configuration.Builder().apply {
            setExecutor { GlobalScope.launch { it.run() } }
            setTaskExecutor { GlobalScope.launch { it.run() } }
        }.build())

        // handle data restored/crash
        if (Build.VERSION.SDK_INT >= 24 && DataStore.directBootAware &&
                app.getSystemService<UserManager>()?.isUserUnlocked == true) DirectBoot.flushTrafficStats()
        if (DataStore.tcpFastOpen && !TcpFastOpen.sendEnabled) TcpFastOpen.enableTimeout()
        if (DataStore.publicStore.getLong(Key.assetUpdateTime, -1) != packageInfo.lastUpdateTime) {
            val assetManager = app.assets
            try {
                for (file in assetManager.list("acl")!!) assetManager.open("acl/$file").use { input ->
                    File(deviceStorage.noBackupFilesDir, file).outputStream().use { output -> input.copyTo(output) }
                }
            } catch (e: IOException) {
                printLog(e)
            }
            DataStore.publicStore.putLong(Key.assetUpdateTime, packageInfo.lastUpdateTime)
        }
        updateNotificationChannels()
    }

    fun updateNotificationChannels() {
        if (Build.VERSION.SDK_INT >= 26) @RequiresApi(26) {
            notification.createNotificationChannels(listOf(
                    NotificationChannel("service-vpn", app.getText(R.string.service_vpn),
                            if (Build.VERSION.SDK_INT >= 28) NotificationManager.IMPORTANCE_MIN
                            else NotificationManager.IMPORTANCE_LOW),   // #1355
                    NotificationChannel("service-proxy", app.getText(R.string.service_proxy),
                            NotificationManager.IMPORTANCE_LOW),
                    NotificationChannel("service-transproxy", app.getText(R.string.service_transproxy),
                            NotificationManager.IMPORTANCE_LOW)))
            notification.deleteNotificationChannel("service-nat")   // NAT mode is gone for good
        }
    }

    fun getPackageInfo(packageName: String) = app.packageManager.getPackageInfo(packageName,
            if (Build.VERSION.SDK_INT >= 28) PackageManager.GET_SIGNING_CERTIFICATES
            else @Suppress("DEPRECATION") PackageManager.GET_SIGNATURES)!!

    fun startService() = ContextCompat.startForegroundService(app, Intent(app, V2RayTestService::class.java))
    fun reloadService() = app.sendBroadcast(Intent(Action.RELOAD).setPackage(app.packageName))
    fun stopService() = app.sendBroadcast(Intent(Action.CLOSE).setPackage(app.packageName))
    fun startServiceForTest() = app.startService(Intent(app, ProxyService::class.java).putExtra("test","go"))
    fun showMessage(msg: String) {
        var toast = Toast.makeText(app, msg, Toast.LENGTH_LONG)
        toast.setGravity(Gravity.BOTTOM or Gravity.CENTER_HORIZONTAL, 0, 150)
        toast.show()
    }

    fun alertMessage(msg: String,activity:Context) {
        try {
            if(activity==null || (activity as Activity).isFinishing)return
        val builder: AlertDialog.Builder? = activity.let {
            AlertDialog.Builder(activity)
        }
        builder?.setMessage(msg)?.setTitle("Alert")?.setPositiveButton("ok", DialogInterface.OnClickListener {
            _, _ ->
        })
        val dialog: AlertDialog? = builder?.create()
        dialog?.show()
        }
        catch (t:Throwable){}
    }

    /**
     * tcping
     */
    fun tcping(url: String, port: Int,timeout:Int=5000): Long {
        var time = -1L
        for (k in 0 until 1) {
            val one = socketConnectTime(url, port,timeout)
            if (one != -1L  )
                if(time == -1L || one < time) {
                    time = one
                }
        }
        return time
    }
    private fun socketConnectTime(url: String, port: Int,timeout:Int=5000): Long {
        try {
            val start = System.currentTimeMillis()
            val socket = Socket()
            var socketAddress = InetSocketAddress(url, port)
            socket.connect(socketAddress,timeout)
            val time = System.currentTimeMillis() - start
            socket.close()
            return time
        } catch (e: UnknownHostException) {
            Log.e("tcping",e.readableMessage)
        } catch (e: IOException) {
            Log.e("tcping",e.readableMessage)
        } catch (e: Exception) {
            Log.e("tcping",e.readableMessage)
        }
        return -1
    }
}

package com.github.shadowsocks.bg
import SpeedUpVPN.VpnEncrypt
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.PackageManager
import android.net.*
import android.net.VpnService
import android.os.Build
import android.os.ParcelFileDescriptor
import android.os.StrictMode
import android.util.Log
import com.github.shadowsocks.BootReceiver
import com.github.shadowsocks.Core
import com.github.shadowsocks.Core.defaultDPreference
import com.github.shadowsocks.aidl.ShadowsocksConnection
import com.github.shadowsocks.core.R
import com.github.shadowsocks.database.AppConfig
import com.github.shadowsocks.database.Profile
import com.github.shadowsocks.database.ProfileManager
import com.github.shadowsocks.database.VmessBean
import com.github.shadowsocks.net.DefaultNetworkListener
import com.github.shadowsocks.preference.DataStore
import com.github.shadowsocks.utils.*
import go.Seq
import kotlinx.coroutines.*
import libv2ray.Libv2ray
import libv2ray.V2RayVPNServiceSupportsSet
import java.io.File
import java.net.UnknownHostException

class V2RayTestService : Service() , BaseService.Interface {
    //for BaseService.Interface start
    override val data = BaseService.Data(this)
    override val tag: String get() = "SSV2Service"
    override fun createNotification(profileName: String): ServiceNotification =
            ServiceNotification(this, profileName, "service-proxy", true)
    override fun onBind(intent: Intent) = super.onBind(intent)
    override fun startRunner() {
        //Log.e("startRunner","...")
        this as Context
        if (Build.VERSION.SDK_INT >= 26) startForegroundService(Intent(this, javaClass))
        else startService(Intent(this, javaClass))
    }
    //for BaseService.Interface stop

    private val v2rayPoint = Libv2ray.newV2RayPoint(V2RayCallback())
    private lateinit var configContent: String
    private lateinit var mInterface: ParcelFileDescriptor
    private var listeningForDefaultNetwork = false
    lateinit var activeProfile: Profile
    override fun onCreate() {
        super.onCreate()
        val policy = StrictMode.ThreadPolicy.Builder().permitAll().build()
        StrictMode.setThreadPolicy(policy)
        v2rayPoint.packageName = packagePath(applicationContext)
        Seq.setContext(applicationContext)
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
                data.connectingJob = GlobalScope.launch(Dispatchers.Main) {
                    try {
                        activeProfile = ProfileManager.getProfile(DataStore.profileId)!!
                        genStoreV2rayConfig()
                        startV2ray()
                    } catch (_: CancellationException) {
                        // if the job was cancelled, it is canceller's responsibility to call stopRunner
                    } catch (_: UnknownHostException) {
                        stopRunner(false, getString(R.string.invalid_server))
                    } catch (exc: Throwable) {
                        if (exc is BaseService.ExpectedException) exc.printStackTrace() else printLog(exc)
                        stopRunner(false, "${getString(R.string.service_failed)}: ${exc.readableMessage}")
                    } finally {
                        data.connectingJob = null
                    }
                }
                return START_NOT_STICKY
    }
    fun profileToVmessBean(profile: Profile): VmessBean {
        var vmess = VmessBean()
        vmess.guid=profile.id.toString()
        vmess.remoteDns=profile.remoteDns
        vmess.address=profile.host
        vmess.alterId=profile.alterId
        vmess.headerType=profile.headerType
        vmess.id=profile.password
        vmess.network=profile.network
        vmess.path=profile.path
        vmess.port=profile.remotePort
        vmess.remarks= profile.name.toString()
        vmess.requestHost=profile.requestHost
        vmess.security=profile.method
        vmess.streamSecurity=profile.streamSecurity
        vmess.subid=profile.url_group
        vmess.testResult=profile.elapsed.toString()

        if(profile.route=="all")vmess.route="0"
        else if(profile.route=="bypass-lan")vmess.route="1"
        else if(profile.route=="bypass-china")vmess.route="2"
        else if(profile.route=="bypass-lan-china")vmess.route="3"
        else vmess.route="0"

        return vmess
    }
    /**
     * gen and store v2ray config file
     */
    fun genStoreV2rayConfig(): Boolean {
        try {
            val result = V2rayConfigUtil.getV2rayConfig(Core.app, profileToVmessBean(activeProfile))
            if (result.status) {
                defaultDPreference.setPrefString(AppConfig.PREF_CURR_CONFIG, result.content)
                defaultDPreference.setPrefString(AppConfig.PREF_CURR_CONFIG_GUID, activeProfile.id.toString())
                defaultDPreference.setPrefString(AppConfig.PREF_CURR_CONFIG_NAME, activeProfile.name)
                return true
            } else {
                return false
            }
        } catch (e: Exception) {
            e.printStackTrace()
            return false
        }
    }

    override fun onLowMemory() {
        stopV2Ray()
        super.onLowMemory()
    }

    override fun onDestroy() {
        super.onDestroy()
        data.binder.close()
        //android.os.Process.killProcess(android.os.Process.myPid())
    }

    fun shutdown() {
        stopV2Ray(true)
    }

    private fun startV2ray() {
        if (!v2rayPoint.isRunning) {
            val broadname="$packageName.SERVICE"
            //Log.e("broadname",broadname)
            if (!data.closeReceiverRegistered) {
                registerReceiver(data.closeReceiver, IntentFilter().apply {
                    addAction(Action.RELOAD)
                    addAction(Intent.ACTION_SHUTDOWN)
                    addAction(Action.CLOSE)
                }, broadname, null)
                data.closeReceiverRegistered = true
            }

            data.notification = createNotification("FQNews-"+activeProfile.formattedName)
            data.changeState(BaseService.State.Connecting)


            configContent = defaultDPreference.getPrefString(AppConfig.PREF_CURR_CONFIG, "")
            v2rayPoint.configureFileContent = configContent
            v2rayPoint.enableLocalDNS = VpnEncrypt.enableLocalDns
            v2rayPoint.forwardIpv6 = activeProfile.ipv6
            v2rayPoint.domainName = defaultDPreference.getPrefString(AppConfig.PREF_CURR_CONFIG_DOMAIN, "")

            try {
                v2rayPoint.runLoop()
            } catch (e: Exception) {
                Log.d(packageName, e.toString())
            }

            if (v2rayPoint.isRunning) {
                data.changeState(BaseService.State.Connected)
            } else {
                //MessageUtil.sendMsg2UI(this, AppConfig.MSG_STATE_START_FAILURE, "")
                //cancelNotification()
            }
        }
        else{
            Log.e("startV2ray","v2rayPoint isRunning already.")
        }
    }
    override fun stopRunner(restart: Boolean , msg: String? ) {
        Log.e("v2stopRunner",msg.toString())
        if (data.state == BaseService.State.Stopping) return
        // channge the state
        data.changeState(BaseService.State.Stopping)
        GlobalScope.launch(Dispatchers.Main.immediate) {
            data.connectingJob?.cancelAndJoin() // ensure stop connecting first
            stopV2Ray()
            // clean up receivers
            val data = data
            if (data.closeReceiverRegistered) {
                unregisterReceiver(data.closeReceiver)
                data.closeReceiverRegistered = false
            }
            data.notification?.destroy()
            data.notification = null
            // change the state
            data.changeState(BaseService.State.Stopped, msg)

            // stop the service if nothing has bound to it
            if (restart) startRunner() else {
                BootReceiver.enabled = false
                stopSelf()
            }
        }
    }
    private fun stopV2Ray(isForced: Boolean = true) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            if (listeningForDefaultNetwork) {
                listeningForDefaultNetwork = false
            }
        }
        if (v2rayPoint.isRunning) {
            try {
                v2rayPoint.stopLoop()
            } catch (e: Exception) {
                Log.d(packageName, e.toString())
            }
        }

        if (isForced) {
            //stopSelf has to be called ahead of mInterface.close(). otherwise v2ray core cannot be stooped
            //It's strage but true.
            //This can be verified by putting stopself() behind and call stopLoop and startLoop
            //in a row for several times. You will find that later created v2ray core report port in use
            //which means the first v2ray core somehow failed to stop and release the port.
            stopSelf()
            try {
                mInterface.close()
            } catch (ignored: Exception) {
            }

        }
    }

    private inner class V2RayCallback : V2RayVPNServiceSupportsSet {
        override fun shutdown(): Long {
            // called by go
            // shutdown the whole vpn service
            try {
                this@V2RayTestService.shutdown()
                return 0
            } catch (e: Exception) {
                Log.d(packageName, e.toString())
                return -1
            }
        }

        override fun prepare(): Long {
            return 0
        }

        override fun protect(l: Long) = 1.toLong()

        override fun onEmitStatus(l: Long, s: String?): Long {
            return 0
        }

        override fun setup(s: String): Long {
                return 0
        }
        override fun sendFd(): Long {
            return 0
        }
    }
}


package net.frju.flym.ui.main
import SpeedUpVPN.VpnEncrypt
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Bundle
import android.os.Handler
import android.os.Process.myPid
import android.os.SystemClock
import android.util.Log
import android.view.Window
import android.view.WindowManager
import androidx.appcompat.app.AppCompatActivity
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.github.shadowsocks.Core
import net.fred.feedex.R
import kotlin.system.exitProcess


class SplashActivity : AppCompatActivity() {
    private val SPLASH_DISPLAY_LENGHT = 1000L
    //Splash至少显示这么长时间，然后等待Core.pickSingleServer完成，即使Core.pickSingleServer先完成，也要等这么久
    private var localBroadcastManager: LocalBroadcastManager? = null
    private var dataLoadingComplete = false
    private lateinit var intancce:SplashActivity
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        intancce=this
        Core.pickSingleServer(this)

        //hiding title bar of this activity
        window.requestFeature(Window.FEATURE_NO_TITLE)
        //making this activity full screen
        window.setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN)
        setContentView(R.layout.activity_splash)

        localBroadcastManager = LocalBroadcastManager.getInstance(this)

        localBroadcastManager!!.registerReceiver(object : BroadcastReceiver() {
            override fun onReceive(context: Context?, intent: Intent?) {
                Log.e("onReceive0", "internet fail")
                //finish()
                //moveTaskToBack(true);
                android.os.Process.killProcess(android.os.Process.myPid())
                exitProcess(1);
            }
        }, IntentFilter(VpnEncrypt.ACTION_INTERNET_FAIL))

        localBroadcastManager!!.registerReceiver(object : BroadcastReceiver() {
            override fun onReceive(context: Context?, intent: Intent?) {
                //Log.e("onReceive1", SystemClock.elapsedRealtime().toString()+" set dataLoadingComplete to true")
                dataLoadingComplete = true
            }
        }, IntentFilter(VpnEncrypt.ACTION_PROXY_START_COMPLETE))

        Handler().postDelayed({
            localBroadcastManager!!.registerReceiver(object : BroadcastReceiver() {
                override fun onReceive(context: Context, intent: Intent) {
                    Log.e("onReceive2", SystemClock.elapsedRealtime().toString()+" startMainActivity")
                    startActivity(Intent(this@SplashActivity, MainActivity::class.java))
                    finish()
                }
            }, IntentFilter(VpnEncrypt.ACTION_PROXY_START_COMPLETE))

            //Log.e("dataLoadingComplete",SystemClock.elapsedRealtime().toString()+" "+dataLoadingComplete.toString())
            if (dataLoadingComplete) {
                localBroadcastManager!!.sendBroadcast(Intent(VpnEncrypt.ACTION_PROXY_START_COMPLETE))
            }

        }, SPLASH_DISPLAY_LENGHT)

        /*
        //4second splash time
        Handler().postDelayed({
            //start main activity
            startActivity(Intent(this@SplashActivity, MainActivity::class.java))
            //finish this activity
            finish()
        },4000)
        */

    }
}
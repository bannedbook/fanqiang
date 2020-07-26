package net.frju.flym.ui.main
import android.content.Intent
import android.os.Bundle
import android.os.Handler
import android.view.Window
import android.view.WindowManager
import androidx.appcompat.app.AppCompatActivity
import net.fred.feedex.R


class SplashActivity : AppCompatActivity() {
    private val SPLASH_DISPLAY_LENGHT = 1000L
    //Splash至少显示这么长时间，然后等待Core.pickSingleServer完成，即使Core.pickSingleServer先完成，也要等这么久
    private lateinit var intancce:SplashActivity
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        intancce=this

        //hiding title bar of this activity
        window.requestFeature(Window.FEATURE_NO_TITLE)
        //making this activity full screen
        window.setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN)
        setContentView(R.layout.activity_splash)

        //4second splash time
        Handler().postDelayed({
            //start main activity
            startActivity(Intent(this@SplashActivity, MainActivity::class.java))
            //finish this activity
            finish()
        },SPLASH_DISPLAY_LENGHT)


    }
}
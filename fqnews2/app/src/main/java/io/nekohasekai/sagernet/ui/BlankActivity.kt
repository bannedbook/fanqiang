package io.nekohasekai.sagernet.ui

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import moe.matsuri.nb4a.utils.SendLog

class BlankActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // process crash log
        intent?.getStringExtra("sendLog")?.apply {
            SendLog.sendLog(this@BlankActivity, this)
        }

        finish()
    }

}
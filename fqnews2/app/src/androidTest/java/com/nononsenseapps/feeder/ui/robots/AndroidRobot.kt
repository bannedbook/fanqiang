package com.nononsenseapps.feeder.ui.robots

import androidx.test.espresso.Espresso
import androidx.test.espresso.NoActivityResumedException

abstract class AndroidRobot {
    var isAppRunning: Boolean = true
        private set

    fun pressBackButton() {
        try {
            Espresso.pressBack()
        } catch (_: NoActivityResumedException) {
            isAppRunning = false
        }
    }
}

package com.nononsenseapps.feeder.ui.activity

import android.content.Intent
import androidx.lifecycle.Lifecycle
import androidx.test.core.app.ApplicationProvider
import androidx.test.core.app.launchActivity
import androidx.test.ext.junit.runners.AndroidJUnit4
import com.nononsenseapps.feeder.ui.AddFeedFromShareActivity
import org.junit.Test
import org.junit.runner.RunWith
import kotlin.test.assertEquals

@RunWith(AndroidJUnit4::class)
class AddFeedFromShareActivityTest {
    @Test
    fun activityShouldStart() {
        val intent =
            Intent(
                ApplicationProvider.getApplicationContext(),
                AddFeedFromShareActivity::class.java,
            )
        launchActivity<AddFeedFromShareActivity>(intent).use { scenario ->
            assertEquals(Lifecycle.State.RESUMED, scenario.state)
        }
    }
}

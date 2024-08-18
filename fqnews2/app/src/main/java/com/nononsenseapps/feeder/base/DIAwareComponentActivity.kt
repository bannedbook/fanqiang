package com.nononsenseapps.feeder.base

import android.view.MenuInflater
import androidx.activity.ComponentActivity
import com.nononsenseapps.feeder.util.ActivityLauncher
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.android.closestDI
import org.kodein.di.bind
import org.kodein.di.direct
import org.kodein.di.instance
import org.kodein.di.provider
import org.kodein.di.singleton

abstract class DIAwareComponentActivity : ComponentActivity(), DIAware {
    private val parentDI: DI by closestDI()
    override val di: DI by DI.lazy {
        extend(parentDI)
        bind<MenuInflater>() with provider { menuInflater }
        bind<DIAwareComponentActivity>() with instance(this@DIAwareComponentActivity)
        bind<ActivityLauncher>() with
            singleton {
                ActivityLauncher(
                    this@DIAwareComponentActivity,
                    di.direct.instance(),
                )
            }
    }
}

package com.nononsenseapps.feeder.notifications

import com.nononsenseapps.feeder.ApplicationCoroutineScope
import com.nononsenseapps.feeder.archmodel.Repository
import io.mockk.MockKAnnotations
import io.mockk.Runs
import io.mockk.coEvery
import io.mockk.coVerify
import io.mockk.confirmVerified
import io.mockk.impl.annotations.MockK
import io.mockk.just
import io.mockk.spyk
import kotlinx.coroutines.runBlocking
import org.junit.Before
import org.junit.Test
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.bind
import org.kodein.di.instance
import org.kodein.di.singleton

class NotificationsWorkerTest : DIAware {
    @MockK
    private lateinit var repository: Repository

    override val di by DI.lazy {
        bind<Repository>() with instance(repository)
        bind<ApplicationCoroutineScope>() with singleton { ApplicationCoroutineScope() }
        bind<NotificationsWorker>() with singleton { spyk(NotificationsWorker(di)) }
    }

    private val notificationsWorker: NotificationsWorker by instance()

    @Before
    fun setup() {
        MockKAnnotations.init(this)
    }

    @Test
    fun cancelsNotificationsForItemsNoLongerPresent() {
        coEvery { notificationsWorker.cancelNotification(any()) } just Runs

        runBlocking {
            notificationsWorker.unNotifyForMissingItems(
                prev = listOf(1L, 2L, 3L, 4L),
                current = listOf(1L, 3L),
            )
        }

        coVerify {
            notificationsWorker.unNotifyForMissingItems(any(), any())
            notificationsWorker.cancelNotification(2L)
            notificationsWorker.cancelNotification(4L)
        }
        confirmVerified(notificationsWorker)
    }
}

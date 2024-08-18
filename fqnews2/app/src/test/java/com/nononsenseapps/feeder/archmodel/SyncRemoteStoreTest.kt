package com.nononsenseapps.feeder.archmodel

import com.nononsenseapps.feeder.db.room.ReadStatusSynced
import com.nononsenseapps.feeder.db.room.ReadStatusSyncedDao
import com.nononsenseapps.feeder.db.room.RemoteReadMarkDao
import com.nononsenseapps.feeder.db.room.SyncRemote
import com.nononsenseapps.feeder.db.room.SyncRemoteDao
import com.nononsenseapps.feeder.util.minusMinutes
import io.mockk.MockKAnnotations
import io.mockk.coEvery
import io.mockk.coVerify
import io.mockk.confirmVerified
import io.mockk.impl.annotations.MockK
import kotlinx.coroutines.flow.emptyFlow
import kotlinx.coroutines.flow.toList
import kotlinx.coroutines.runBlocking
import org.junit.Before
import org.junit.Test
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.bind
import org.kodein.di.instance
import org.kodein.di.singleton
import java.time.Instant
import kotlin.test.assertEquals

class SyncRemoteStoreTest : DIAware {
    private val store: SyncRemoteStore by instance()

    @MockK
    private lateinit var dao: SyncRemoteDao

    @MockK
    private lateinit var readStatusDao: ReadStatusSyncedDao

    @MockK
    private lateinit var remoteReadMarkDao: RemoteReadMarkDao

    override val di by DI.lazy {
        bind<SyncRemoteDao>() with instance(dao)
        bind<ReadStatusSyncedDao>() with instance(readStatusDao)
        bind<RemoteReadMarkDao>() with instance(remoteReadMarkDao)
        bind<SyncRemoteStore>() with singleton { SyncRemoteStore(di) }
    }

    @Before
    fun setup() {
        MockKAnnotations.init(this, relaxUnitFun = true, relaxed = true)
    }

    @Test
    fun getNewSyncRemoteInsertsDefault() {
        coEvery { dao.getSyncRemote() } returns null

        val result =
            runBlocking {
                store.getSyncRemote()
            }

        coVerify {
            dao.getSyncRemote()
            dao.insert(any())
        }

        assertEquals(1L, result.id)

        confirmVerified(dao)
    }

    @Test
    fun getExistingSyncRemote() {
        val expected = SyncRemote(id = 1L)
        coEvery { dao.getSyncRemote() } returns expected

        val result =
            runBlocking {
                store.getSyncRemote()
            }

        coVerify {
            dao.getSyncRemote()
        }

        assertEquals(expected, result)

        confirmVerified(dao)
    }

    @Test
    fun getSyncRemoteFlow() {
        coEvery { dao.getSyncRemoteFlow() } returns emptyFlow()

        runBlocking {
            store.getSyncRemoteFlow()
        }

        coVerify { dao.getSyncRemoteFlow() }
        confirmVerified(dao)
    }

    @Test
    fun deleteAllReadStatusSyncs() {
        coEvery { readStatusDao.deleteAll() } returns 1

        runBlocking {
            store.deleteAllReadStatusSyncs()
        }

        coVerify { readStatusDao.deleteAll() }
        confirmVerified(readStatusDao, dao)
    }

    @Test
    fun getNextFeedItemWithoutSyncedReadMark() {
        coEvery { readStatusDao.getNextFeedItemWithoutSyncedReadMark() } returns emptyFlow()

        val result =
            runBlocking {
                store.getNextFeedItemWithoutSyncedReadMark().toList()
            }

        assertEquals(emptyList(), result)

        coVerify { readStatusDao.getNextFeedItemWithoutSyncedReadMark() }
        confirmVerified(readStatusDao)
    }

    @Test
    fun setSynced() {
        coEvery { readStatusDao.insert(any()) } returns 10L

        runBlocking {
            store.setSynced(5L)
        }

        coVerify { readStatusDao.insert(ReadStatusSynced(sync_remote = 1L, feed_item = 5L)) }
        confirmVerified(readStatusDao)
    }

    @Test
    fun deleteStaleRemoteReadMarks() {
        coEvery { remoteReadMarkDao.deleteStaleRemoteReadMarks(any()) } returns 50

        val now = Instant.now()

        runBlocking {
            store.deleteStaleRemoteReadMarks(now)
        }

        coVerify { remoteReadMarkDao.deleteStaleRemoteReadMarks(now.minusMinutes(7 * 24 * 60)) }
        confirmVerified(remoteReadMarkDao)
    }

    @Test
    fun deleteReadStatusSyncs() {
        coEvery { readStatusDao.deleteReadStatusSyncs(any()) } returns 50

        runBlocking {
            store.deleteReadStatusSyncs(listOf(5L, 12L))
        }

        coVerify { readStatusDao.deleteReadStatusSyncs(listOf(5L, 12L)) }
        confirmVerified(readStatusDao)
    }
}

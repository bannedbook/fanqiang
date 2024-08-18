package io.nekohasekai.sagernet.database

import io.nekohasekai.sagernet.GroupType
import io.nekohasekai.sagernet.ktx.applyDefaultValues

object GroupManager {

    interface Listener {
        suspend fun groupAdd(group: ProxyGroup)
        suspend fun groupUpdated(group: ProxyGroup)

        suspend fun groupRemoved(groupId: Long)
        suspend fun groupUpdated(groupId: Long)
    }

    interface Interface {
        suspend fun confirm(message: String): Boolean
        suspend fun alert(message: String)
        suspend fun onUpdateSuccess(
            group: ProxyGroup,
            changed: Int,
            added: List<String>,
            updated: Map<String, String>,
            deleted: List<String>,
            duplicate: List<String>,
            byUser: Boolean
        )

        suspend fun onUpdateFailure(group: ProxyGroup, message: String)
    }

    private val listeners = ArrayList<Listener>()
    var userInterface: Interface? = null

    suspend fun iterator(what: suspend Listener.() -> Unit) {
        synchronized(listeners) {
            listeners.toList()
        }.forEach { listener ->
            what(listener)
        }
    }

    fun addListener(listener: Listener) {
        synchronized(listeners) {
            listeners.add(listener)
        }
    }

    fun removeListener(listener: Listener) {
        synchronized(listeners) {
            listeners.remove(listener)
        }
    }

    suspend fun clearGroup(groupId: Long) {
        DataStore.selectedProxy = 0L
        SagerDatabase.proxyDao.deleteAll(groupId)
        iterator { groupUpdated(groupId) }
    }

    fun rearrange(groupId: Long) {
        val entities = SagerDatabase.proxyDao.getByGroup(groupId)
        for (index in entities.indices) {
            entities[index].userOrder = (index + 1).toLong()
        }
        SagerDatabase.proxyDao.updateProxy(entities)
    }

    suspend fun postUpdate(group: ProxyGroup) {
        iterator { groupUpdated(group) }
    }

    suspend fun postUpdate(groupId: Long) {
        postUpdate(SagerDatabase.groupDao.getById(groupId) ?: return)
    }

    suspend fun postReload(groupId: Long) {
        iterator { groupUpdated(groupId) }
    }

    suspend fun createGroup(group: ProxyGroup): ProxyGroup {
        group.userOrder = SagerDatabase.groupDao.nextOrder() ?: 1
        group.id = SagerDatabase.groupDao.createGroup(group.applyDefaultValues())
        iterator { groupAdd(group) }
        return group
    }

    suspend fun updateGroup(group: ProxyGroup) {
        SagerDatabase.groupDao.updateGroup(group)
        iterator { groupUpdated(group) }
    }

    suspend fun deleteGroup(groupId: Long) {
        SagerDatabase.groupDao.deleteById(groupId)
        SagerDatabase.proxyDao.deleteByGroup(groupId)
        iterator { groupRemoved(groupId) }
    }

    suspend fun deleteGroup(group: List<ProxyGroup>) {
        SagerDatabase.groupDao.deleteGroup(group)
        SagerDatabase.proxyDao.deleteByGroup(group.map { it.id }.toLongArray())
        for (proxyGroup in group) iterator { groupRemoved(proxyGroup.id) }
    }

}

package moe.matsuri.nb4a.utils

import io.nekohasekai.sagernet.database.DataStore
import io.nekohasekai.sagernet.database.ProxyEntity
import io.nekohasekai.sagernet.database.SagerDatabase
import libcore.Libcore

object LibcoreUtil {
    fun resetAllConnections(system: Boolean) {
        if (DataStore.serviceState.started) {
            val proxy = SagerDatabase.proxyDao.getById(DataStore.currentProfile)
            if (proxy?.type == ProxyEntity.TYPE_TUIC) {
                return
            }
        }
        Libcore.resetAllConnections(system)
    }
}

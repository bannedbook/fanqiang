package moe.matsuri.nb4a.proxy.neko

import io.nekohasekai.sagernet.database.DataStore
import io.nekohasekai.sagernet.ktx.Logs
import io.nekohasekai.sagernet.ktx.getStr
import io.nekohasekai.sagernet.ktx.runOnIoDispatcher
import libcore.Libcore
import moe.matsuri.nb4a.Protocols
import moe.matsuri.nb4a.plugin.NekoPluginManager
import org.json.JSONObject
import kotlin.coroutines.resume
import kotlin.coroutines.suspendCoroutine

suspend fun parseShareLink(plgId: String, protocolId: String, link: String): NekoBean =
    suspendCoroutine {
        runOnIoDispatcher {
            val jsi = NekoJSInterface.Default.requireJsi(plgId)
            jsi.lock()

            try {
                jsi.init()

                val jsip = jsi.switchProtocol(protocolId)
                val sharedStorage = jsip.parseShareLink(link)

                // NekoBean from link
                val bean = NekoBean()
                bean.plgId = plgId
                bean.protocolId = protocolId
                bean.sharedStorage = NekoBean.tryParseJSON(sharedStorage)
                bean.onSharedStorageSet()

                it.resume(bean)
            } catch (e: Exception) {
                Logs.e(e)
                it.resume(NekoBean().apply {
                    this.plgId = plgId
                    this.protocolId = protocolId
                })
            }

            jsi.unlock()
            // destroy when all link parsed
        }
    }

fun NekoBean.shareLink(): String {
    return sharedStorage.optString("shareLink")
}

// Only run in bg process
// seems no concurrent
suspend fun NekoBean.updateAllConfig(port: Int) = suspendCoroutine<Unit> {
    allConfig = null

    runOnIoDispatcher {
        val jsi = NekoJSInterface.Default.requireJsi(plgId)
        jsi.lock()

        try {
            jsi.init()
            val jsip = jsi.switchProtocol(protocolId)

            // runtime arguments
            val otherArgs = mutableMapOf<String, Any>()
            otherArgs["finalAddress"] = finalAddress
            otherArgs["finalPort"] = finalPort
            otherArgs["muxEnabled"] = Protocols.shouldEnableMux(protocolId)
            otherArgs["muxConcurrency"] = DataStore.muxConcurrency

            val ret = jsip.buildAllConfig(port, this@updateAllConfig, otherArgs)

            // result
            allConfig = JSONObject(ret)
        } catch (e: Exception) {
            Logs.e(e)
        }

        jsi.unlock()
        it.resume(Unit)
        // destroy when config generated / all tests finished
    }
}

fun NekoBean.cacheGet(id: String): String? {
    return DataStore.profileCacheStore.getString("neko_${hash()}_$id")
}

fun NekoBean.cacheSet(id: String, value: String) {
    DataStore.profileCacheStore.putString("neko_${hash()}_$id", value)
}

fun NekoBean.hash(): String {
    var a = plgId
    a += protocolId
    a += sharedStorage.toString()
    return Libcore.sha256Hex(a.toByteArray())
}

// must call it to update something like serverAddress
fun NekoBean.onSharedStorageSet() {
    serverAddress = sharedStorage.getStr("serverAddress")
    serverPort = sharedStorage.getStr("serverPort")?.toInt() ?: 1080
    if (serverAddress == null || serverAddress.isBlank()) {
        serverAddress = "127.0.0.1"
    }
    name = sharedStorage.optString("name")
}

fun NekoBean.needBypassRootUid(): Boolean {
    val p = NekoPluginManager.findProtocol(protocolId) ?: return false
    return p.protocolConfig.optBoolean("needBypassRootUid")
}

fun NekoBean.haveStandardLink(): Boolean {
    val p = NekoPluginManager.findProtocol(protocolId) ?: return false
    return p.protocolConfig.optBoolean("haveStandardLink")
}

fun NekoBean.canShare(): Boolean {
    val p = NekoPluginManager.findProtocol(protocolId) ?: return false
    return p.protocolConfig.optBoolean("canShare")
}

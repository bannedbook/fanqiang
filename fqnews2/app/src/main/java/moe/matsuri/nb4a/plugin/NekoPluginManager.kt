package moe.matsuri.nb4a.plugin

import com.nononsenseapps.feeder.R
import jww.app.FeederApplication
import io.nekohasekai.sagernet.bg.BaseService
import io.nekohasekai.sagernet.database.DataStore
import io.nekohasekai.sagernet.ktx.forEach
import io.nekohasekai.sagernet.utils.PackageCache
import moe.matsuri.nb4a.proxy.neko.NekoJSInterface
import okhttp3.internal.closeQuietly
import org.json.JSONObject
import java.io.File
import java.util.zip.CRC32
import java.util.zip.ZipFile

object NekoPluginManager {
    const val managerVersion = 2

    val plugins get() = DataStore.nekoPlugins.split("\n").filter { it.isNotBlank() }

    // plgID to plgConfig object
    fun getManagedPlugins(): Map<String, JSONObject> {
        val ret = mutableMapOf<String, JSONObject>()
        plugins.forEach {
            tryGetPlgConfig(it)?.apply {
                ret[it] = this
            }
        }
        return ret
    }

    class Protocol(
        val protocolId: String, val plgId: String, val protocolConfig: JSONObject
    )

    fun getProtocols(): List<Protocol> {
        val ret = mutableListOf<Protocol>()
        getManagedPlugins().forEach { (t, u) ->
            u.optJSONArray("protocols")?.forEach { _, any ->
                if (any is JSONObject) {
                    val name = any.optString("protocolId")
                    ret.add(Protocol(name, t, any))
                }
            }
        }
        return ret
    }

    fun findProtocol(protocolId: String): Protocol? {
        getManagedPlugins().forEach { (t, u) ->
            u.optJSONArray("protocols")?.forEach { _, any ->
                if (any is JSONObject) {
                    if (protocolId == any.optString("protocolId")) {
                        return Protocol(protocolId, t, any)
                    }
                }
            }
        }
        return null
    }

    fun removeManagedPlugin(plgId: String) {
        DataStore.configurationStore.remove(plgId)
        val dir = File(FeederApplication.instance.filesDir.absolutePath + "/plugins/" + plgId)
        if (dir.exists()) {
            dir.deleteRecursively()
        }
    }

    fun extractPlugin(plgId: String, install: Boolean) {
        val app = PackageCache.installedApps[plgId] ?: return
        val apk = File(app.publicSourceDir)
        if (!apk.exists()) {
            return
        }
        if (!install && !plugins.contains(plgId)) {
            return
        }

        val zipFile = ZipFile(apk)
        val unzipDir = File(FeederApplication.instance.filesDir.absolutePath + "/plugins/" + plgId)
        unzipDir.mkdirs()
        for (entry in zipFile.entries()) {
            if (entry.name.startsWith("assets/")) {
                val relativePath = entry.name.removePrefix("assets/")
                val outFile = File(unzipDir, relativePath)
                if (entry.isDirectory) {
                    outFile.mkdirs()
                    continue
                }

                if (outFile.isDirectory) {
                    outFile.delete()
                } else if (outFile.exists()) {
                    val checksum = CRC32()
                    checksum.update(outFile.readBytes())
                    if (checksum.value == entry.crc) {
                        continue
                    }
                }

                val input = zipFile.getInputStream(entry)
                outFile.outputStream().use {
                    input.copyTo(it)
                }
            }
        }
        zipFile.closeQuietly()
    }

    suspend fun installPlugin(plgId: String) {
        if (plgId == "moe.matsuri.plugin.singbox" || plgId == "moe.matsuri.plugin.xray") {
            throw Exception("This plugin is deprecated")
        }
        extractPlugin(plgId, true)
        NekoJSInterface.Default.destroyJsi(plgId)
        NekoJSInterface.Default.requireJsi(plgId).init()
        NekoJSInterface.Default.destroyJsi(plgId)
    }

    const val PLUGIN_APP_VERSION = "_v_vc"
    const val PLUGIN_APP_VERSION_NAME = "_v_vn"

    // Return null if not managed
    fun tryGetPlgConfig(plgId: String): JSONObject? {
        return try {
            JSONObject(DataStore.configurationStore.getString(plgId)!!)
        } catch (e: Exception) {
            null
        }
    }

    fun updatePlgConfig(plgId: String, plgConfig: JSONObject) {
        PackageCache.installedPluginPackages[plgId]?.apply {
            // longVersionCode requires API 28
//            plgConfig.put(PLUGIN_APP_VERSION, versionCode)
            plgConfig.put(PLUGIN_APP_VERSION_NAME, versionName)
        }
        DataStore.configurationStore.putString(plgId, plgConfig.toString())
    }

    fun htmlPath(plgId: String): String {
        val htmlFile = File(FeederApplication.instance.filesDir.absolutePath + "/plugins/" + plgId)
        return htmlFile.absolutePath
    }

    class PluginInternalException(val protocolId: String) : Exception(),
        BaseService.ExpectedException {
        override fun getLocalizedMessage() =
            FeederApplication.instance.getString(R.string.neko_plugin_internal_error, protocolId)
    }

}

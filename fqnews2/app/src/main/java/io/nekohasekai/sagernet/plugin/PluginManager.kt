package io.nekohasekai.sagernet.plugin

import android.content.pm.ComponentInfo
import android.content.pm.ProviderInfo
import com.nononsenseapps.feeder.R
import jww.app.FeederApplication
import io.nekohasekai.sagernet.bg.BaseService
import io.nekohasekai.sagernet.ktx.Logs
import moe.matsuri.nb4a.plugin.Plugins
import java.io.File
import java.io.FileNotFoundException

object PluginManager {

    class PluginNotFoundException(val plugin: String) : FileNotFoundException(plugin),
        BaseService.ExpectedException {
        override fun getLocalizedMessage() =
            FeederApplication.instance.getString(R.string.plugin_unknown, plugin)
    }

    data class InitResult(
        val path: String,
        val info: ProviderInfo,
    )

    @Throws(Throwable::class)
    fun init(pluginId: String): InitResult? {
        if (pluginId.isEmpty()) return null
        var throwable: Throwable? = null

        try {
            val result = initNative(pluginId)
            if (result != null) return result
        } catch (t: Throwable) {
            throwable = t
            Logs.w(t)
        }

        throw throwable ?: PluginNotFoundException(pluginId)
    }

    private fun initNative(pluginId: String): InitResult? {
        val info = Plugins.getPlugin(pluginId) ?: return null

        // internal so
        if (info.applicationInfo == null) {
            try {
                initNativeInternal(pluginId)?.let { return InitResult(it, info) }
            } catch (t: Throwable) {
                Logs.w("initNativeInternal failed", t)
            }
            return null
        }

        try {
            initNativeFaster(info)?.let { return InitResult(it, info) }
        } catch (t: Throwable) {
            Logs.w("initNativeFaster failed", t)
        }

        Logs.w("Init native returns empty result")
        return null
    }

    private fun initNativeInternal(pluginId: String): String? {
        fun soIfExist(soName: String): String? {
            val f = File(FeederApplication.instance.applicationInfo.nativeLibraryDir, soName)
            if (f.canExecute()) {
                return f.absolutePath
            }
            return null
        }
        return when (pluginId) {
            "hysteria-plugin" -> soIfExist("libhysteria.so")
            "hysteria2-plugin" -> soIfExist("libhysteria2.so")
            else -> null
        }
    }

    private fun initNativeFaster(provider: ProviderInfo): String? {
        return provider.loadString(Plugins.METADATA_KEY_EXECUTABLE_PATH)
            ?.let { relativePath ->
                File(provider.applicationInfo.nativeLibraryDir).resolve(relativePath).apply {
                    check(canExecute())
                }.absolutePath
            }
    }

    fun ComponentInfo.loadString(key: String) = when (val value = metaData.get(key)) {
        is String -> value
        is Int -> FeederApplication.instance.packageManager.getResourcesForApplication(applicationInfo)
            .getString(value)

        null -> null
        else -> error("meta-data $key has invalid type ${value.javaClass}")
    }
}

package moe.matsuri.nb4a

import android.content.Context
import com.nononsenseapps.feeder.R
import io.nekohasekai.sagernet.database.DataStore
import io.nekohasekai.sagernet.database.ProxyEntity.Companion.TYPE_NEKO
import io.nekohasekai.sagernet.fmt.AbstractBean
import io.nekohasekai.sagernet.fmt.v2ray.StandardV2RayBean
import io.nekohasekai.sagernet.fmt.v2ray.isTLS
import io.nekohasekai.sagernet.ktx.app
import io.nekohasekai.sagernet.ktx.getColorAttr
import moe.matsuri.nb4a.plugin.NekoPluginManager

// Settings for all protocols, built-in or plugin
object Protocols {
    // Mux

    fun isProfileNeedMux(bean: StandardV2RayBean): Boolean {
        return when (bean.type) {
            "tcp", "ws" -> true
            "http" -> !bean.isTLS()
            else -> false
        }
    }

    fun shouldEnableMux(protocol: String): Boolean {
        return DataStore.muxProtocols.contains(protocol)
    }

    fun getCanMuxList(): List<String> {
        // built-in and support mux
        val list = mutableListOf("vmess", "trojan", "trojan-go", "shadowsocks", "vless", "padding")

        NekoPluginManager.getProtocols().forEach {
            if (it.protocolConfig.optBoolean("canMux")) {
                list.add(it.protocolId)
            }
        }

        return list
    }

    // Deduplication

    class Deduplication(
        val bean: AbstractBean, val type: String
    ) {

        fun hash(): String {
            return bean.serverAddress + bean.serverPort + type
        }

        override fun hashCode(): Int {
            return hash().toByteArray().contentHashCode()
        }

        override fun equals(other: Any?): Boolean {
            if (this === other) return true
            if (javaClass != other?.javaClass) return false

            other as Deduplication

            return hash() == other.hash()
        }

    }

    // Display

    fun Context.getProtocolColor(type: Int): Int {
        return when (type) {
            TYPE_NEKO -> getColorAttr(android.R.attr.textColorPrimary)
            else -> getColor(android.R.color.holo_blue_light)
        }
    }

    // Test

    fun genFriendlyMsg(msg: String): String {
        val msgL = msg.lowercase()
        return when {
            msgL.contains("timeout") || msgL.contains("deadline") -> {
                app.getString(R.string.connection_test_timeout)
            }

            msgL.contains("refused") || msgL.contains("closed pipe") -> {
                app.getString(R.string.connection_test_refused)
            }

            else -> msg
        }
    }

}

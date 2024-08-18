package io.nekohasekai.sagernet.group

import android.annotation.SuppressLint
import android.net.Uri
import com.nononsenseapps.feeder.R
import io.nekohasekai.sagernet.database.*
import io.nekohasekai.sagernet.fmt.AbstractBean
import io.nekohasekai.sagernet.fmt.http.HttpBean
import io.nekohasekai.sagernet.fmt.hysteria.HysteriaBean
import io.nekohasekai.sagernet.fmt.hysteria.parseHysteria1Json
import io.nekohasekai.sagernet.fmt.shadowsocks.ShadowsocksBean
import io.nekohasekai.sagernet.fmt.shadowsocks.parseShadowsocks
import io.nekohasekai.sagernet.fmt.socks.SOCKSBean
import io.nekohasekai.sagernet.fmt.trojan.TrojanBean
import io.nekohasekai.sagernet.fmt.trojan_go.parseTrojanGo
import io.nekohasekai.sagernet.fmt.tuic.TuicBean
import io.nekohasekai.sagernet.fmt.v2ray.StandardV2RayBean
import io.nekohasekai.sagernet.fmt.v2ray.VMessBean
import io.nekohasekai.sagernet.fmt.v2ray.isTLS
import io.nekohasekai.sagernet.fmt.v2ray.setTLS
import io.nekohasekai.sagernet.fmt.wireguard.WireGuardBean
import io.nekohasekai.sagernet.ktx.*
import libcore.Libcore
import moe.matsuri.nb4a.Protocols
import moe.matsuri.nb4a.proxy.config.ConfigBean
import org.ini4j.Ini
import org.json.JSONArray
import org.json.JSONObject
import org.json.JSONTokener
import org.yaml.snakeyaml.TypeDescription
import org.yaml.snakeyaml.Yaml
import org.yaml.snakeyaml.error.YAMLException
import java.io.StringReader

@Suppress("EXPERIMENTAL_API_USAGE")
object RawUpdater : GroupUpdater() {

    @SuppressLint("Recycle")
    override suspend fun doUpdate(
        proxyGroup: ProxyGroup,
        subscription: SubscriptionBean,
        userInterface: GroupManager.Interface?,
        byUser: Boolean
    ) {

        val link = subscription.link
        var proxies: List<AbstractBean>
        if (link.startsWith("content://")) {
            val contentText = app.contentResolver.openInputStream(Uri.parse(link))
                ?.bufferedReader()
                ?.readText()

            proxies = contentText?.let { parseRaw(contentText) }
                ?: error(app.getString(R.string.no_proxies_found_in_subscription))
        } else {

            val response = Libcore.newHttpClient().apply {
                trySocks5(DataStore.mixedPort)
                when (DataStore.appTLSVersion) {
                    "1.3" -> restrictedTLS()
                }
            }.newRequest().apply {
                if (DataStore.allowInsecureOnRequest) {
                    allowInsecure()
                }
                setURL(subscription.link)
                setUserAgent(subscription.customUserAgent.takeIf { it.isNotBlank() } ?: USER_AGENT)
            }.execute()

            proxies = parseRaw(response.contentString)
                ?: error(app.getString(R.string.no_proxies_found))

            subscription.subscriptionUserinfo = response.getHeader("Subscription-Userinfo")
        }

        val proxiesMap = LinkedHashMap<String, AbstractBean>()
        for (proxy in proxies) {
            var index = 0
            var name = proxy.displayName()
            while (proxiesMap.containsKey(name)) {
                println("Exists name: $name")
                index++
                name = name.replace(" (${index - 1})", "")
                name = "$name ($index)"
                proxy.name = name
            }
            proxiesMap[proxy.displayName()] = proxy
        }
        proxies = proxiesMap.values.toList()

        if (subscription.forceResolve) forceResolve(proxies, proxyGroup.id)

        val exists = SagerDatabase.proxyDao.getByGroup(proxyGroup.id)
        val duplicate = ArrayList<String>()
        if (subscription.deduplication) {
            Logs.d("Before deduplication: ${proxies.size}")
            val uniqueProxies = LinkedHashSet<Protocols.Deduplication>()
            val uniqueNames = HashMap<Protocols.Deduplication, String>()
            for (_proxy in proxies) {
                val proxy = Protocols.Deduplication(_proxy, _proxy.javaClass.toString())
                if (!uniqueProxies.add(proxy)) {
                    val index = uniqueProxies.indexOf(proxy)
                    if (uniqueNames.containsKey(proxy)) {
                        val name = uniqueNames[proxy]!!.replace(" ($index)", "")
                        if (name.isNotBlank()) {
                            duplicate.add("$name ($index)")
                            uniqueNames[proxy] = ""
                        }
                    }
                    duplicate.add(_proxy.displayName() + " ($index)")
                } else {
                    uniqueNames[proxy] = _proxy.displayName()
                }
            }
            uniqueProxies.retainAll(uniqueNames.keys)
            proxies = uniqueProxies.toList().map { it.bean }
        }

        Logs.d("New profiles: ${proxies.size}")

        val nameMap = proxies.associateBy { bean ->
            bean.displayName()
        }

        Logs.d("Unique profiles: ${nameMap.size}")

        val toDelete = ArrayList<ProxyEntity>()
        val toReplace = exists.mapNotNull { entity ->
            val name = entity.displayName()
            if (nameMap.contains(name)) name to entity else let {
                toDelete.add(entity)
                null
            }
        }.toMap()

        Logs.d("toDelete profiles: ${toDelete.size}")
        Logs.d("toReplace profiles: ${toReplace.size}")

        val toUpdate = ArrayList<ProxyEntity>()
        val added = mutableListOf<String>()
        val updated = mutableMapOf<String, String>()
        val deleted = toDelete.map { it.displayName() }

        var userOrder = 1L
        var changed = toDelete.size
        for ((name, bean) in nameMap.entries) {
            if (toReplace.contains(name)) {
                val entity = toReplace[name]!!
                val existsBean = entity.requireBean()
                existsBean.applyFeatureSettings(bean)
                when {
                    existsBean != bean -> {
                        changed++
                        entity.putBean(bean)
                        toUpdate.add(entity)
                        updated[entity.displayName()] = name

                        Logs.d("Updated profile: $name")
                    }

                    entity.userOrder != userOrder -> {
                        entity.putBean(bean)
                        toUpdate.add(entity)
                        entity.userOrder = userOrder

                        Logs.d("Reordered profile: $name")
                    }

                    else -> {
                        Logs.d("Ignored profile: $name")
                    }
                }
            } else {
                changed++
                SagerDatabase.proxyDao.addProxy(ProxyEntity(
                    groupId = proxyGroup.id, userOrder = userOrder
                ).apply {
                    putBean(bean)
                })
                added.add(name)
                Logs.d("Inserted profile: $name")
            }
            userOrder++
        }

        SagerDatabase.proxyDao.updateProxy(toUpdate).also {
            Logs.d("Updated profiles: $it")
        }

        SagerDatabase.proxyDao.deleteProxy(toDelete).also {
            Logs.d("Deleted profiles: $it")
        }

        val existCount = SagerDatabase.proxyDao.countByGroup(proxyGroup.id).toInt()

        if (existCount != proxies.size) {
            Logs.e("Exist profiles: $existCount, new profiles: ${proxies.size}")
        }

        subscription.lastUpdated = (System.currentTimeMillis() / 1000).toInt()
        SagerDatabase.groupDao.updateGroup(proxyGroup)
        finishUpdate(proxyGroup)

        userInterface?.onUpdateSuccess(
            proxyGroup, changed, added, updated, deleted, duplicate, byUser
        )
    }

    @Suppress("UNCHECKED_CAST")
    suspend fun parseRaw(text: String, fileName: String = ""): List<AbstractBean>? {

        val proxies = mutableListOf<AbstractBean>()

        if (text.contains("proxies:")) {

            // clash & meta

            try {

                val yaml = Yaml().apply {
                    addTypeDescription(TypeDescription(String::class.java, "str"))
                }.loadAs(text, Map::class.java)

                val globalClientFingerprint = yaml["global-client-fingerprint"]?.toString() ?: ""

                for (proxy in (yaml["proxies"] as? (List<Map<String, Any?>>) ?: error(
                    app.getString(R.string.no_proxies_found_in_file)
                ))) {
                    // Note: YAML numbers parsed as "Long"

                    when (proxy["type"] as String) {
                        "socks5" -> {
                            proxies.add(SOCKSBean().apply {
                                serverAddress = proxy["server"] as String
                                serverPort = proxy["port"].toString().toInt()
                                username = proxy["username"]?.toString()
                                password = proxy["password"]?.toString()
                                name = proxy["name"]?.toString()
                            })
                        }

                        "http" -> {
                            proxies.add(HttpBean().apply {
                                serverAddress = proxy["server"] as String
                                serverPort = proxy["port"].toString().toInt()
                                username = proxy["username"]?.toString()
                                password = proxy["password"]?.toString()
                                setTLS(proxy["tls"]?.toString() == "true")
                                sni = proxy["sni"]?.toString()
                                name = proxy["name"]?.toString()
                            })
                        }

                        "ss" -> {
                            val ssPlugin = mutableListOf<String>()
                            if (proxy.contains("plugin")) {
                                val opts = proxy["plugin-opts"] as Map<String, Any?>
                                when (proxy["plugin"]) {
                                    "obfs" -> {
                                        ssPlugin.apply {
                                            add("obfs-local")
                                            add("obfs=" + (opts["mode"]?.toString() ?: ""))
                                            add("obfs-host=" + (opts["host"]?.toString() ?: ""))
                                        }
                                    }

                                    "v2ray-plugin" -> {
                                        ssPlugin.apply {
                                            add("v2ray-plugin")
                                            add("mode=" + (opts["mode"]?.toString() ?: ""))
                                            if (opts["mode"]?.toString() == "true") add("tls")
                                            add("host=" + (opts["host"]?.toString() ?: ""))
                                            add("path=" + (opts["path"]?.toString() ?: ""))
                                            if (opts["mux"]?.toString() == "true") add("mux=8")
                                        }
                                    }
                                }
                            }
                            proxies.add(ShadowsocksBean().apply {
                                serverAddress = proxy["server"] as String
                                serverPort = proxy["port"].toString().toInt()
                                password = proxy["password"]?.toString()
                                method = clashCipher(proxy["cipher"] as String)
                                plugin = ssPlugin.joinToString(";")
                                name = proxy["name"]?.toString()
                            })
                        }

                        "vmess", "vless" -> {
                            val isVLESS = proxy["type"].toString() == "vless"
                            val bean = VMessBean().apply {
                                if (isVLESS) {
                                    alterId = -1 // make it VLESS
                                    packetEncoding = 2 // clash meta default XUDP
                                }
                            }
                            for (opt in proxy) {
                                if (opt.value == null) continue
                                when (opt.key.replace("_", "-")) {
                                    "name" -> bean.name = opt.value.toString()
                                    "server" -> bean.serverAddress = opt.value as String
                                    "port" -> bean.serverPort = opt.value.toString().toInt()
                                    "uuid" -> bean.uuid = opt.value as String

                                    "alterId" -> if (!isVLESS) bean.alterId =
                                        opt.value.toString().toInt()

                                    "cipher" -> if (!isVLESS) bean.encryption = opt.value as String

                                    "flow" -> if (isVLESS) bean.encryption = opt.value as String

                                    "packet-addr" -> if (opt.value.toString() == "true") {
                                        bean.packetEncoding = 1
                                    }

                                    "xudp" -> if (opt.value.toString() == "true") {
                                        bean.packetEncoding = 2
                                    }

                                    "network" -> {
                                        bean.type = opt.value as String
                                        // Clash "network" fix
                                        when (bean.type) {
                                            "h2" -> bean.type = "http"
                                        }
                                    }

                                    "client-fingerprint" -> bean.utlsFingerprint =
                                        opt.value as String

                                    "tls" -> bean.security =
                                        if (opt.value.toString() == "true") "tls" else ""

                                    "servername" -> bean.sni = opt.value.toString()

                                    "skip-cert-verify" -> bean.allowInsecure =
                                        opt.value.toString() == "true"

                                    "alpn" -> {
                                        val alpn = (opt.value as? (List<String>))
                                        bean.alpn = alpn?.joinToString("\n")
                                    }

                                    "ws-path" -> bean.path = opt.value.toString()
                                    "ws-headers" -> for (wsHeader in (opt.value as Map<String, Any>)) {
                                        when (wsHeader.key.lowercase()) {
                                            "host" -> bean.host = wsHeader.value.toString()
                                        }
                                    }

                                    "ws-opts", "ws-opt" -> for (wsOpt in (opt.value as Map<String, Any>)) {
                                        when (wsOpt.key.lowercase()) {
                                            "headers" -> for (wsHeader in (wsOpt.value as Map<String, Any>)) {
                                                when (wsHeader.key.lowercase()) {
                                                    "host" -> bean.host = wsHeader.value.toString()
                                                }
                                            }

                                            "path" -> {
                                                bean.path = wsOpt.value.toString()
                                            }

                                            "max-early-data" -> {
                                                bean.wsMaxEarlyData = wsOpt.value.toString().toInt()
                                            }

                                            "early-data-header-name" -> {
                                                bean.earlyDataHeaderName = wsOpt.value.toString()
                                            }
                                        }
                                    }

                                    // The format of the VMessBean is wrong, so the `host` `path` has some strange transformations here.
                                    "h2-opts", "h2-opt" -> for (h2Opt in (opt.value as Map<String, Any>)) {
                                        when (h2Opt.key.lowercase()) {
                                            "host" -> bean.host =
                                                (h2Opt.value as List<String>).first()

                                            "path" -> bean.path = h2Opt.value.toString()
                                        }
                                    }

                                    "http-opts", "http-opt" -> for (httpOpt in (opt.value as Map<String, Any>)) {
                                        when (httpOpt.key.lowercase()) {
                                            "path" -> bean.path =
                                                (httpOpt.value as List<String>).first()

                                            "headers" -> for (hdr in (httpOpt.value as Map<String, Any>)) {
                                                when (hdr.key.lowercase()) {
                                                    "host" -> bean.host =
                                                        (hdr.value as List<String>).first()
                                                }
                                            }
                                        }
                                    }

                                    "grpc-opts", "grpc-opt" -> for (grpcOpt in (opt.value as Map<String, Any>)) {
                                        when (grpcOpt.key.lowercase()) {
                                            "grpc-service-name" -> bean.path =
                                                grpcOpt.value.toString()
                                        }
                                    }

                                    "reality-opts" -> for (realityOpt in (opt.value as Map<String, Any>)) {
                                        when (realityOpt.key.lowercase()) {
                                            "public-key" -> bean.realityPubKey =
                                                realityOpt.value.toString()

                                            "short-id" -> bean.realityShortId =
                                                realityOpt.value.toString()
                                        }
                                    }
                                }
                            }
                            proxies.add(bean)
                        }

                        "trojan" -> {
                            val bean = TrojanBean()
                            bean.security = "tls"
                            for (opt in proxy) {
                                if (opt.value == null) continue
                                when (opt.key.replace("_", "-")) {
                                    "name" -> bean.name = opt.value.toString()
                                    "server" -> bean.serverAddress = opt.value as String
                                    "port" -> bean.serverPort = opt.value.toString().toInt()
                                    "password" -> bean.password = opt.value.toString()
                                    "client-fingerprint" -> bean.utlsFingerprint =
                                        opt.value as String

                                    "sni" -> bean.sni = opt.value.toString()
                                    "skip-cert-verify" -> bean.allowInsecure =
                                        opt.value.toString() == "true"

                                    "alpn" -> {
                                        val alpn = (opt.value as? (List<String>))
                                        bean.alpn = alpn?.joinToString("\n")
                                    }

                                    "network" -> when (opt.value) {
                                        "ws", "grpc" -> bean.type = opt.value.toString()
                                    }

                                    "ws-opts", "ws-opt" -> for (wsOpt in (opt.value as Map<String, Any>)) {
                                        when (wsOpt.key.lowercase()) {
                                            "headers" -> for (wsHeader in (wsOpt.value as Map<String, Any>)) {
                                                when (wsHeader.key.lowercase()) {
                                                    "host" -> bean.host = wsHeader.value.toString()
                                                }
                                            }

                                            "path" -> {
                                                bean.path = wsOpt.value.toString()
                                            }
                                        }
                                    }

                                    "grpc-opts", "grpc-opt" -> for (grpcOpt in (opt.value as Map<String, Any>)) {
                                        when (grpcOpt.key.lowercase()) {
                                            "grpc-service-name" -> bean.path =
                                                grpcOpt.value.toString()
                                        }
                                    }
                                }
                            }
                            proxies.add(bean)
                        }

                        "hysteria" -> {
                            val bean = HysteriaBean()
                            bean.protocolVersion = 1
                            var hopPorts = ""
                            for (opt in proxy) {
                                if (opt.value == null) continue
                                when (opt.key.replace("_", "-")) {
                                    "name" -> bean.name = opt.value.toString()
                                    "server" -> bean.serverAddress = opt.value as String
                                    "port" -> bean.serverPorts = opt.value.toString()
                                    "ports" -> hopPorts = opt.value.toString()

                                    "obfs" -> bean.obfuscation = opt.value.toString()

                                    "auth-str" -> {
                                        bean.authPayloadType = HysteriaBean.TYPE_STRING
                                        bean.authPayload = opt.value.toString()
                                    }

                                    "sni" -> bean.sni = opt.value.toString()

                                    "skip-cert-verify" -> bean.allowInsecure =
                                        opt.value.toString() == "true"

                                    "up" -> bean.uploadMbps =
                                        opt.value.toString().substringBefore(" ").toIntOrNull()
                                            ?: 100

                                    "down" -> bean.downloadMbps =
                                        opt.value.toString().substringBefore(" ").toIntOrNull()
                                            ?: 100

                                    "recv-window-conn" -> bean.connectionReceiveWindow =
                                        opt.value.toString().toIntOrNull() ?: 0

                                    "recv-window" -> bean.streamReceiveWindow =
                                        opt.value.toString().toIntOrNull() ?: 0

                                    "disable-mtu-discovery" -> bean.disableMtuDiscovery =
                                        opt.value.toString() == "true" || opt.value.toString() == "1"

                                    "alpn" -> {
                                        val alpn = (opt.value as? (List<String>))
                                        bean.alpn = alpn?.joinToString("\n") ?: "h3"
                                    }
                                }
                            }
                            if (hopPorts.isNotBlank()) {
                                bean.serverPorts = hopPorts
                            }
                            proxies.add(bean)
                        }

                        "hysteria2" -> {
                            val bean = HysteriaBean()
                            bean.protocolVersion = 2
                            var hopPorts = ""
                            for (opt in proxy) {
                                if (opt.value == null) continue
                                when (opt.key.replace("_", "-")) {
                                    "name" -> bean.name = opt.value.toString()
                                    "server" -> bean.serverAddress = opt.value as String
                                    "port" -> bean.serverPorts = opt.value.toString()
                                    "ports" -> hopPorts = opt.value.toString()

                                    "obfs-password" -> bean.obfuscation = opt.value.toString()

                                    "password" -> bean.authPayload = opt.value.toString()

                                    "sni" -> bean.sni = opt.value.toString()

                                    "skip-cert-verify" -> bean.allowInsecure =
                                        opt.value.toString() == "true"

                                    "up" -> bean.uploadMbps =
                                        opt.value.toString().substringBefore(" ").toIntOrNull() ?: 0

                                    "down" -> bean.downloadMbps =
                                        opt.value.toString().substringBefore(" ").toIntOrNull() ?: 0
                                }
                            }
                            if (hopPorts.isNotBlank()) {
                                bean.serverPorts = hopPorts
                            }
                            proxies.add(bean)
                        }

                        "tuic" -> {
                            val bean = TuicBean()
                            var ip = ""
                            for (opt in proxy) {
                                if (opt.value == null) continue
                                when (opt.key.replace("_", "-")) {
                                    "name" -> bean.name = opt.value.toString()
                                    "server" -> bean.serverAddress = opt.value.toString()
                                    "ip" -> ip = opt.value.toString()
                                    "port" -> bean.serverPort = opt.value.toString().toInt()

                                    "token" -> {
                                        bean.protocolVersion = 4
                                        bean.token = opt.value.toString()
                                    }

                                    "uuid" -> bean.uuid = opt.value.toString()

                                    "password" -> bean.token = opt.value.toString()

                                    "skip-cert-verify" -> bean.allowInsecure =
                                        opt.value.toString() == "true"

                                    "disable-sni" -> bean.disableSNI =
                                        opt.value.toString() == "true"

                                    "reduce-rtt" -> bean.reduceRTT =
                                        opt.value.toString() == "true"

                                    "sni" -> bean.sni = opt.value.toString()

                                    "alpn" -> {
                                        val alpn = (opt.value as? (List<String>))
                                        bean.alpn = alpn?.joinToString("\n")
                                    }

                                    "congestion-controller" -> bean.congestionController =
                                        opt.value.toString()

                                    "udp-relay-mode" -> bean.udpRelayMode = opt.value.toString()

                                }
                            }
                            if (ip.isNotBlank()) {
                                bean.serverAddress = ip
                                if (bean.sni.isNullOrBlank() && !bean.serverAddress.isNullOrBlank() && !bean.serverAddress.isIpAddress()) {
                                    bean.sni = bean.serverAddress
                                }
                            }
                            proxies.add(bean)
                        }
                    }
                }

                // Fix ent
                proxies.forEach {
                    it.initializeDefaultValues()
                    if (it is StandardV2RayBean) {
                        // 1. SNI
                        if (it.isTLS() && it.sni.isNullOrBlank() && !it.host.isNullOrBlank() && !it.host.isIpAddress()) {
                            it.sni = it.host
                        }
                        // 2. globalClientFingerprint
                        if (!it.realityPubKey.isNullOrBlank() && it.utlsFingerprint.isNullOrBlank()) {
                            it.utlsFingerprint = globalClientFingerprint
                            if (it.utlsFingerprint.isNullOrBlank()) it.utlsFingerprint = "chrome"
                        }
                    }
                }
                return proxies
            } catch (e: YAMLException) {
                Logs.w(e)
            }
        } else if (text.contains("[Interface]")) {
            // wireguard
            try {
                proxies.addAll(parseWireGuard(text).map {
                    if (fileName.isNotBlank()) it.name = fileName.removeSuffix(".conf")
                    it
                })
                return proxies
            } catch (e: Exception) {
                Logs.w(e)
            }
        }

        try {
            val json = JSONTokener(text).nextValue()
            return parseJSON(json)
        } catch (ignored: Exception) {
        }

        try {
            return parseProxies(text.decodeBase64UrlSafe()).takeIf { it.isNotEmpty() }
                ?: error("Not found")
        } catch (e: Exception) {
            Logs.w(e)
        }

        try {
            return parseProxies(text).takeIf { it.isNotEmpty() } ?: error("Not found")
        } catch (e: SubscriptionFoundException) {
            throw e
        } catch (ignored: Exception) {
        }

        return null
    }

    fun clashCipher(cipher: String): String {
        return when (cipher) {
            "dummy" -> "none"
            else -> cipher
        }
    }

    fun parseWireGuard(conf: String): List<WireGuardBean> {
        val ini = Ini(StringReader(conf))
        val iface = ini["Interface"] ?: error("Missing 'Interface' selection")
        val bean = WireGuardBean().applyDefaultValues()
        val localAddresses = iface.getAll("Address")
        if (localAddresses.isNullOrEmpty()) error("Empty address in 'Interface' selection")
        bean.localAddress = localAddresses.flatMap { it.split(",") }.joinToString("\n")
        bean.privateKey = iface["PrivateKey"]
        bean.mtu = iface["MTU"]?.toIntOrNull()
        val peers = ini.getAll("Peer")
        if (peers.isNullOrEmpty()) error("Missing 'Peer' selections")
        val beans = mutableListOf<WireGuardBean>()
        for (peer in peers) {
            val endpoint = peer["Endpoint"]
            if (endpoint.isNullOrBlank() || !endpoint.contains(":")) {
                continue
            }

            val peerBean = bean.clone()
            peerBean.serverAddress = endpoint.substringBeforeLast(":")
            peerBean.serverPort = endpoint.substringAfterLast(":").toIntOrNull() ?: continue
            peerBean.peerPublicKey = peer["PublicKey"] ?: continue
            peerBean.peerPreSharedKey = peer["PresharedKey"]
            beans.add(peerBean.applyDefaultValues())
        }
        if (beans.isEmpty()) error("Empty available peer list")
        return beans
    }

    fun parseJSON(json: Any): List<AbstractBean> {
        val proxies = ArrayList<AbstractBean>()

        if (json is JSONObject) {
            when {
                json.has("server") && (json.has("up") || json.has("up_mbps")) -> {
                    return listOf(json.parseHysteria1Json())
                }

                json.has("method") -> {
                    return listOf(json.parseShadowsocks())
                }

                json.has("remote_addr") -> {
                    return listOf(json.parseTrojanGo())
                }

                json.has("outbounds") -> {
                    return listOf(ConfigBean().applyDefaultValues().apply {
                        config = json.toStringPretty()
                    })
                }

                json.has("server") && json.has("server_port") -> {
                    return listOf(ConfigBean().applyDefaultValues().apply {
                        type = 1
                        config = json.toStringPretty()
                    })
                }
            }
        } else {
            json as JSONArray
            json.forEach { _, it ->
                if (isJsonObjectValid(it)) {
                    proxies.addAll(parseJSON(it))
                }
            }
        }

        proxies.forEach { it.initializeDefaultValues() }
        return proxies
    }

}

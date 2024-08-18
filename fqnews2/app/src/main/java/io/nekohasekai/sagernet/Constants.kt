package io.nekohasekai.sagernet

const val CONNECTION_TEST_URL = "http://cp.cloudflare.com/"

object Key {

    const val DB_PUBLIC = "configuration.db"
    const val DB_PROFILE = "sager_net.db"

    const val PERSIST_ACROSS_REBOOT = "isAutoConnect"

    const val APP_EXPERT = "isExpert"
    const val APP_THEME = "appTheme"
    const val NIGHT_THEME = "nightTheme"
    const val SERVICE_MODE = "serviceMode"
    const val MODE_VPN = "vpn"
    const val MODE_PROXY = "proxy"

    const val REMOTE_DNS = "remoteDns"
    const val DIRECT_DNS = "directDns"
    const val ENABLE_DNS_ROUTING = "enableDnsRouting"
    const val ENABLE_FAKEDNS = "enableFakeDns"

    const val IPV6_MODE = "ipv6Mode"

    const val PROXY_APPS = "proxyApps"
    const val BYPASS_MODE = "bypassMode"
    const val INDIVIDUAL = "individual"
    const val METERED_NETWORK = "meteredNetwork"

    const val TRAFFIC_SNIFFING = "trafficSniffing"
    const val RESOLVE_DESTINATION = "resolveDestination"

    const val BYPASS_LAN = "bypassLan"
    const val BYPASS_LAN_IN_CORE = "bypassLanInCore"

    const val MIXED_PORT = "mixedPort"
    const val ALLOW_ACCESS = "allowAccess"
    const val SPEED_INTERVAL = "speedInterval"
    const val SHOW_DIRECT_SPEED = "showDirectSpeed"
    const val LOCAL_DNS_PORT = "portLocalDns"

    const val APPEND_HTTP_PROXY = "appendHttpProxy"

    const val CONNECTION_TEST_URL = "connectionTestURL"

    const val TCP_KEEP_ALIVE_INTERVAL = "tcpKeepAliveInterval"
    const val RULES_PROVIDER = "rulesProvider"
    const val LOG_LEVEL = "logLevel"
    const val LOG_BUF_SIZE = "logBufSize"
    const val MTU = "mtu"
    const val ALWAYS_SHOW_ADDRESS = "alwaysShowAddress"

    // Protocol Settings
    const val MUX_TYPE = "muxType"
    const val MUX_PROTOCOLS = "mux"
    const val MUX_CONCURRENCY = "muxConcurrency"
    const val GLOBAL_ALLOW_INSECURE = "globalAllowInsecure"

    const val ACQUIRE_WAKE_LOCK = "acquireWakeLock"
    const val SHOW_BOTTOM_BAR = "showBottomBar"

    const val ALLOW_INSECURE_ON_REQUEST = "allowInsecureOnRequest"

    const val TUN_IMPLEMENTATION = "tunImplementation"
    const val PROFILE_TRAFFIC_STATISTICS = "profileTrafficStatistics"

    const val PROFILE_DIRTY = "profileDirty"
    const val PROFILE_ID = "profileId"
    const val PROFILE_NAME = "profileName"
    const val PROFILE_GROUP = "profileGroup"
    const val PROFILE_CURRENT = "profileCurrent"

    const val SERVER_ADDRESS = "serverAddress"
    const val SERVER_PORT = "serverPort"
    const val SERVER_USERNAME = "serverUsername"
    const val SERVER_PASSWORD = "serverPassword"
    const val SERVER_METHOD = "serverMethod"
    const val SERVER_PASSWORD1 = "serverPassword1"

    const val PROTOCOL_VERSION = "protocolVersion"

    const val SERVER_PROTOCOL = "serverProtocol"
    const val SERVER_OBFS = "serverObfs"

    const val SERVER_NETWORK = "serverNetwork"
    const val SERVER_HOST = "serverHost"
    const val SERVER_PATH = "serverPath"
    const val SERVER_SNI = "serverSNI"
    const val SERVER_ENCRYPTION = "serverEncryption"
    const val SERVER_ALPN = "serverALPN"
    const val SERVER_CERTIFICATES = "serverCertificates"
    const val SERVER_MTU = "serverMTU"

    const val SERVER_CONFIG = "serverConfig"
    const val SERVER_CUSTOM = "serverCustom"
    const val SERVER_CUSTOM_OUTBOUND = "serverCustomOutbound"

    const val SERVER_SECURITY_CATEGORY = "serverSecurityCategory"
    const val SERVER_TLS_CAMOUFLAGE_CATEGORY = "serverTlsCamouflageCategory"
    const val SERVER_ECH_CATEORY = "serverECHCategory"
    const val SERVER_WS_CATEGORY = "serverWsCategory"
    const val SERVER_SS_CATEGORY = "serverSsCategory"
    const val SERVER_HEADERS = "serverHeaders"
    const val SERVER_ALLOW_INSECURE = "serverAllowInsecure"

    const val SERVER_AUTH_TYPE = "serverAuthType"
    const val SERVER_UPLOAD_SPEED = "serverUploadSpeed"
    const val SERVER_DOWNLOAD_SPEED = "serverDownloadSpeed"
    const val SERVER_STREAM_RECEIVE_WINDOW = "serverStreamReceiveWindow"
    const val SERVER_CONNECTION_RECEIVE_WINDOW = "serverConnectionReceiveWindow"
    const val SERVER_DISABLE_MTU_DISCOVERY = "serverDisableMtuDiscovery"
    const val SERVER_HOP_INTERVAL = "hopInterval"

    const val SERVER_PRIVATE_KEY = "serverPrivateKey"
    const val SERVER_INSECURE_CONCURRENCY = "serverInsecureConcurrency"

    const val SERVER_UDP_RELAY_MODE = "serverUDPRelayMode"
    const val SERVER_CONGESTION_CONTROLLER = "serverCongestionController"
    const val SERVER_DISABLE_SNI = "serverDisableSNI"
    const val SERVER_REDUCE_RTT = "serverReduceRTT"

    const val ROUTE_NAME = "routeName"
    const val ROUTE_DOMAIN = "routeDomain"
    const val ROUTE_IP = "routeIP"
    const val ROUTE_PORT = "routePort"
    const val ROUTE_SOURCE_PORT = "routeSourcePort"
    const val ROUTE_NETWORK = "routeNetwork"
    const val ROUTE_SOURCE = "routeSource"
    const val ROUTE_PROTOCOL = "routeProtocol"
    const val ROUTE_OUTBOUND = "routeOutbound"
    const val ROUTE_PACKAGES = "routePackages"

    const val GROUP_NAME = "groupName"
    const val GROUP_TYPE = "groupType"
    const val GROUP_ORDER = "groupOrder"
    const val GROUP_IS_SELECTOR = "groupIsSelector"
    const val GROUP_FRONT_PROXY = "groupFrontProxy"
    const val GROUP_LANDING_PROXY = "groupLandingProxy"

    const val GROUP_SUBSCRIPTION = "groupSubscription"
    const val SUBSCRIPTION_LINK = "subscriptionLink"
    const val SUBSCRIPTION_FORCE_RESOLVE = "subscriptionForceResolve"
    const val SUBSCRIPTION_DEDUPLICATION = "subscriptionDeduplication"
    const val SUBSCRIPTION_UPDATE = "subscriptionUpdate"
    const val SUBSCRIPTION_UPDATE_WHEN_CONNECTED_ONLY = "subscriptionUpdateWhenConnectedOnly"
    const val SUBSCRIPTION_USER_AGENT = "subscriptionUserAgent"
    const val SUBSCRIPTION_AUTO_UPDATE = "subscriptionAutoUpdate"
    const val SUBSCRIPTION_AUTO_UPDATE_DELAY = "subscriptionAutoUpdateDelay"

    //

    const val NEKO_PLUGIN_MANAGED = "nekoPlugins"
    const val APP_TLS_VERSION = "appTLSVersion"
    const val ENABLE_CLASH_API = "enableClashAPI"
}

object TunImplementation {
    const val GVISOR = 0
    const val SYSTEM = 1
    const val MIXED = 2
}

object IPv6Mode {
    const val DISABLE = 0
    const val ENABLE = 1
    const val PREFER = 2
    const val ONLY = 3
}

object GroupType {
    const val BASIC = 0
    const val SUBSCRIPTION = 1
}

object GroupOrder {
    const val ORIGIN = 0
    const val BY_NAME = 1
    const val BY_DELAY = 2
}

object Action {
    const val SERVICE = "io.nekohasekai.sagernet.SERVICE"
    const val CLOSE = "io.nekohasekai.sagernet.CLOSE"
    const val RELOAD = "io.nekohasekai.sagernet.RELOAD"

    // const val SWITCH_WAKE_LOCK = "io.nekohasekai.sagernet.SWITCH_WAKELOCK"
    const val RESET_UPSTREAM_CONNECTIONS = "free.v2ray.proxy.VPN.RESET_UPSTREAM_CONNECTIONS"
}

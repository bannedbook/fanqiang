package com.github.shadowsocks.database

data class V2rayConfig(
        val stats: Any?=null,
        val log: LogBean = LogBean(),
        val policy: PolicyBean = PolicyBean(),
        val inbounds: ArrayList<InboundBean> = arrayListOf(InboundBean()),
        var outbounds: ArrayList<OutboundBean> = arrayListOf(OutboundBean()),
        var dns: DnsBean = DnsBean(),
        val routing: RoutingBean = RoutingBean()) {

    data class LogBean(val access: String="",
                       val error: String="",
                       val loglevel: String="")

    data class InboundBean(
            var tag: String ="",
            var port: Int?=null,
            var protocol: String ="",
            var listen: String?=null,
            val settings: InSettingsBean?=InSettingsBean(),
            val sniffing: SniffingBean?=SniffingBean()) {

        data class InSettingsBean(val auth: String? = null,
                                  val udp: Boolean? = null,
                                  val userLevel: Int? =null,
                                  val address: String? = null,
                                  val port: Int? = null,
                                  val network: String? = null)

        data class SniffingBean(var enabled: Boolean?=null,
                                val destOverride: List<String>?=null)
    }

    data class OutboundBean(val tag: String="",
                            var protocol: String="",
                            var settings: OutSettingsBean?=OutSettingsBean(),
                            var streamSettings: StreamSettingsBean?=null,
                            var mux: MuxBean?=MuxBean()) {

        data class OutSettingsBean(var vnext: List<VnextBean>?= listOf(VnextBean()),
                                   var servers: List<ServersBean>?=listOf(ServersBean()),
                                   var response: Response?=Response()) {

            data class VnextBean(var address: String ="",
                                 var port: Int = 0,
                                 var users: List<UsersBean> = listOf(UsersBean())) {

                data class UsersBean(var id: String="",
                                     var alterId: Int=2,
                                     var encryption: String="none",
                                     var security: String="",
                                     var level: Int=2,
                                     var flow:String?="")
            }

            data class ServersBean(var address: String="",
                                   var method: String="",
                                   var ota: Boolean=false,
                                   var password: String="",
                                   var port: Int=0,
                                   var level: Int=0)

            data class Response(var type: String="")
        }

        data class StreamSettingsBean(var network: String="",
                                      var security: String="",
                                      var tcpSettings: TcpsettingsBean?=TcpsettingsBean(),
                                      var kcpSettings: KcpsettingsBean?=KcpsettingsBean(),
                                      var wsSettings: WssettingsBean?=WssettingsBean(),
                                      var httpsettings: HttpsettingsBean?=HttpsettingsBean(),
                                      var tlsSettings: TlssettingsBean?=TlssettingsBean(),
                                      var xtlsSettings: TlssettingsBean?=TlssettingsBean(),
                                      var quicsettings: QuicsettingBean?=QuicsettingBean()
        ) {

            data class TcpsettingsBean(var connectionReuse: Boolean = true,
                                       var header: HeaderBean = HeaderBean()) {
                data class HeaderBean(var type: String = "none",
                                      var request: Any? = null,
                                      var response: Any? = null)
            }

            data class KcpsettingsBean(var mtu: Int = 1350,
                                       var tti: Int = 20,
                                       var uplinkCapacity: Int = 12,
                                       var downlinkCapacity: Int = 100,
                                       var congestion: Boolean = false,
                                       var readBufferSize: Int = 1,
                                       var seed:String = "",
                                       var writeBufferSize: Int = 1,
                                       var header: HeaderBean = HeaderBean()) {
                data class HeaderBean(var type: String = "none")
            }

            data class WssettingsBean(var path: String = "",
                                      var headers: HeadersBean = HeadersBean()) {
                data class HeadersBean(var Host: String = "")
            }

            data class HttpsettingsBean(var host: List<String> = ArrayList<String>(), var path: String = "")

            data class TlssettingsBean(var allowInsecure: Boolean = false,
                                       var serverName: String = "")

            data class QuicsettingBean(var security: String = "none",
                                        var key: String = "",
                                        var header: HeaderBean = HeaderBean()) {
                data class HeaderBean(var type: String = "none")
            }
        }

        data class MuxBean(var enabled: Boolean = false, var concurrency:Int = -1)
    }

    //data class DnsBean(var servers: List<String>)
    data class DnsBean(var servers: List<Any>?=null,
                       var hosts: Map<String, String>?=null
    ) {
        data class ServersBean(var address: String = "",
                               var port: Int = 0,
                               var domains: List<String>?)
    }

    data class RoutingBean(var domainStrategy: String="",
                           var rules: ArrayList<RulesBean> =arrayListOf(RulesBean())) {

        data class RulesBean(var type: String = "",
                             var ip: ArrayList<String>? = null,
                             var domain: ArrayList<String>? = null,
                             var outboundTag: String = "",
                             var port: String? = null,
                             var inboundTag: ArrayList<String>? = null)
    }

    data class PolicyBean(var levels: Map<String, LevelBean>?= mapOf("" to LevelBean()),
                          var system: Any?=null) {
        data class LevelBean(
                  var handshake: Int? = null,
                  var connIdle: Int? = null,
                  var uplinkOnly: Int? = null,
                  var downlinkOnly: Int? = null)
    }
}
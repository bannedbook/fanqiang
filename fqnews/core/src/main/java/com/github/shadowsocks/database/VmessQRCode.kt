package com.github.shadowsocks.database

data class VmessQRCode(var v: String = "",
                       var ps: String = "",
                       var add: String = "",
                       var port: String = "",
                       var id: String = "",
                       var aid: String = "",
                       var net: String = "",
                       var type: String = "",
                       var host: String = "",
                       var path: String = "",
                       var tls: String = "",
                       var sni: String = "",
                       var allowInsecure: String = "false")
package com.github.shadowsocks.database

data class VmessBean(var guid: String = "123456",
                     var address: String = "v2ray.cool",
                     var port: Int = 10086,
                     var id: String = "a3482e88-686a-4a58-8126-99c9df64b7bf",
                     var alterId: Int = 64,
                     var security: String = "aes-128-cfb",
                     var network: String = "tcp",
                     var remarks: String = "def",
                     var headerType: String = "",
                     var requestHost: String = "",
                     var path: String = "",
                     var streamSecurity: String = "",
                     var allowInsecure: String = "false", //跳过证书验证
                     var SNI: String ="",
                     var flow: String ="",
                     var configType: String = AppConfig.EConfigType.vmess,
                     var configVersion: Int = 2,
                     var testResult: String = "",
                     var subid: String = "",
                     var remoteDns: String = "1.1.1.1",
                     var route:String = "3"  // 3,绕过局域网和大陆地址
)
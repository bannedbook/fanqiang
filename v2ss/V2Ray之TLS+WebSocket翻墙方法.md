# V2Ray之TLS+WebSocket翻墙方法

接前文，[自建V2Ray+TLS翻墙配置方法](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2Ray%2BTLS%E7%BF%BB%E5%A2%99%E9%85%8D%E7%BD%AE%E6%96%B9%E6%B3%95.md) , 本文将在V2Ray+TLS翻墙配置方法基础上再启用WebSocket搭配翻墙。

WebSocket是什么，这里不多解释，感兴趣的网友可以自行Google，反正一句话，就是比传统HTTP/TCP 更快更好用的协议。

WebSocket 的配置其实很简单，基于前文 V2Ray+TLS 基础上， 我们 "network": "tcp" 改成 "network": "ws" 就行了，注意，服务器和客户端要同时修改哦。

话不多说了，直接上配置。

### 服务器

```javascript
{
  "inbounds": [
    {
      "port": 443, // 建议使用 443 端口
      "protocol": "vmess",    
      "settings": {
        "clients": [
          {
            "id": "23ad6b10-8d1a-40f7-8ad0-e3e35cd38297",  
            "alterId": 64
          }
        ]
      },
      "streamSettings": {
        "network": "ws",
        "security": "tls", // security 要设置为 tls 才会启用 TLS
        "tlsSettings": {
          "certificates": [
            {
              "certificateFile": "/etc/v2ray/v2ray.crt", // 证书文件
              "keyFile": "/etc/v2ray/v2ray.key" // 密钥文件
            }
          ]
        }
      }
    }
  ],
  "outbounds": [
    {
      "protocol": "freedom",
      "settings": {}
    }
  ]
}
```

### 客户端

```javascript
{
  "inbounds": [
    {
      "port": 1080,
      "protocol": "socks",
      "sniffing": {
        "enabled": true,
        "destOverride": ["http", "tls"]
      },
      "settings": {
        "auth": "noauth"
      }
    }
  ],
  "outbounds": [
    {
      "protocol": "vmess",
      "settings": {
        "vnext": [
          {
            "address": "mydomain.me", // tls 需要域名，所以这里应该填自己的域名。如果前面配置了子域名，可以使用其中一个子域名，子域名被封可换另一个子域名
            "port": 443,
            "users": [
              {
                "id": "23ad6b10-8d1a-40f7-8ad0-e3e35cd38297",
                "alterId": 64
              }
            ]
          }
        ]
      },
      "streamSettings": {
        "network": "ws",
        "security": "tls" // 客户端的 security 也要设置为 tls
      }
    }
  ]
}
```

改完服务器、客户端配置后，重启服务器，然后重启客户端，开始冲浪，是不是感觉更快了一些呢？

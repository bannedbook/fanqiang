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
            "alterId": 0
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
                "alterId": 0
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

### 结语

改完服务器、客户端配置后，重启服务器，然后重启客户端，开始冲浪，是不是感觉更快了一些呢？如果还不够快，还可以套CDN【详见：[V2Ray之TLS+WebSocket+Nginx+CDN配置方法](https://github.com/bannedbook/fanqiang/blob/master/v2ss/V2Ray%E4%B9%8BTLS+WebSocket+Nginx+CDN%E9%85%8D%E7%BD%AE%E6%96%B9%E6%B3%95.md)】，套CDN不是一个必须的步骤，但套CDN可以有效保护IP，甚至被墙的ip也能复活，如果套国内CDN，据说速度可以飞起来。

### 相关教程

<a title="自建V2ray服务器简明教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md">自建V2ray服务器简明教程</a>

<a title="自建V2Ray+TLS翻墙配置方法" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2Ray%2BTLS%E7%BF%BB%E5%A2%99%E9%85%8D%E7%BD%AE%E6%96%B9%E6%B3%95.md">自建V2Ray+TLS翻墙配置方法</a>

<a title="自建Shadowsocks服务器简明教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAShadowsocks%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md">自建Shadowsocks服务器简明教程</a>

<a title="SSH连接VPS教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/SSH%E8%BF%9E%E6%8E%A5VPS%E6%95%99%E7%A8%8B.md">SSH连接VPS教程</a>

<a title="V2ray官方一键安装脚本" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/V2ray%E5%AE%98%E6%96%B9%E4%B8%80%E9%94%AE%E5%AE%89%E8%A3%85%E8%84%9A%E6%9C%AC.md">V2ray官方一键安装脚本</a>

<a title="Windows版V2ray客户端安装配置指南" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/Windows%E7%89%88V2ray%E5%AE%A2%E6%88%B7%E7%AB%AF%E5%AE%89%E8%A3%85%E9%85%8D%E7%BD%AE%E6%8C%87%E5%8D%97.md">Windows版V2ray客户端安装配置指南.md</a>

<a title="使用FileZilla和VPS传输文件教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E4%BD%BF%E7%94%A8FileZilla%E5%92%8CVPS%E4%BC%A0%E8%BE%93%E6%96%87%E4%BB%B6%E6%95%99%E7%A8%8B.md">使用FileZilla和VPS传输文件教程</a>

<a title="最简单的Google BBR 一键加速VPS教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E6%9C%80%E7%AE%80%E5%8D%95%E7%9A%84Google%20BBR%20%E4%B8%80%E9%94%AE%E5%8A%A0%E9%80%9FVPS%E6%95%99%E7%A8%8B.md">最简单的Google BBR 一键加速VPS教程</a>

<a title="翻墙VPS推荐：搬瓦工VPS购买教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E7%BF%BB%E5%A2%99VPS%E6%8E%A8%E8%8D%90%EF%BC%9A%E6%90%AC%E7%93%A6%E5%B7%A5VPS%E8%B4%AD%E4%B9%B0%E6%95%99%E7%A8%8B.md">翻墙VPS推荐：搬瓦工VPS购买教程</a>

<a  href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%B4%AD%E4%B9%B0Vultr%20VPS%E5%9B%BE%E6%96%87%E6%95%99%E7%A8%8B.md">购买Vultr VPS图文教程【新用户赠送100美元】</a>



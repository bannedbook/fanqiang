<h1>自建Shadowsocks服务器简明教程</h1>

**自建Shadowsocks教程很简单，整个教程分简单几步**：

购买VPS服务器、一键加速VPS服务器、安装Shadowsocks服务端

虽然很简单，但是如果你懒得折腾，那就用我们提供的免费翻墙软件吧：<br>
<a href="https://github.com/bannedbook/fanqiang/wiki/%E5%AE%89%E5%8D%93%E7%BF%BB%E5%A2%99%E8%BD%AF%E4%BB%B6">安卓手机翻墙</a>、<a href="https://github.com/bannedbook/fanqiang/wiki/Chrome%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" class="wiki-page-link">Chrome一键翻墙包</a>、<a href="https://github.com/bannedbook/fanqiang/wiki/%E7%81%AB%E7%8B%90firefox%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" class="wiki-page-link">火狐firefox一键翻墙包</a> <br>

<b>或者也可以购买现成的翻墙服务(跟本库无关哦)：</b><br>
<a href="https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA"><img src="https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2free.jpg" height="300" alt="V2free翻墙-不限流量、高速稳定、性价比超强"></a>

***

**【前言】**

***

**第一步：购买VPS服务器**

详见： [购买Vultr VPS图文教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%B4%AD%E4%B9%B0Vultr%20VPS%E5%9B%BE%E6%96%87%E6%95%99%E7%A8%8B.md)
或 [搬瓦工VPS购买教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E7%BF%BB%E5%A2%99VPS%E6%8E%A8%E8%8D%90%EF%BC%9A%E6%90%AC%E7%93%A6%E5%B7%A5VPS%E8%B4%AD%E4%B9%B0%E6%95%99%E7%A8%8B.md)

***

**第二步：SSH连接服务器**

详见： [SSH连接VPS教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/SSH%E8%BF%9E%E6%8E%A5VPS%E6%95%99%E7%A8%8B.md)

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/ss/xshell2.png)

SSH连接VPS成功后，会出现如上图所示，之后就可以复制粘贴linux命令脚本来执行了。

***

**第三步：Google BBR 一键加速VPS服务器**

详见： [最简单的Google BBR 一键加速VPS教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E6%9C%80%E7%AE%80%E5%8D%95%E7%9A%84Google%20BBR%20%E4%B8%80%E9%94%AE%E5%8A%A0%E9%80%9FVPS%E6%95%99%E7%A8%8B.md)

***

**第四步：安装Shadowsocks服务端**

这里我们采用V2ray官方的一键安装脚本，见教程：[V2ray官方一键安装脚本](https://github.com/bannedbook/fanqiang/blob/master/v2ss/V2ray%E5%AE%98%E6%96%B9%E4%B8%80%E9%94%AE%E5%AE%89%E8%A3%85%E8%84%9A%E6%9C%AC.md)

什么？V2ray？，这不是安装Shadowsocks吗，是我看错了，还是你写错了？哈哈，你没看错，我也没写错！

V2ray 的实现中已内置了Shadowsocks支持，包括最新的AEAD加密协议。同时经过笔者的试用，感觉V2ray 的 Shadowsocks实现，用起来挺稳定。而Shadowsocks官方的Shadowsocks libev有一个问题，我一直没法解决，就是Shadowsocks libev的服务端程序会死掉，错误日志报的错误信息却是DNS解析错误，此时重启一下Shadowsocks libev就好了，所以判断是Shadowsocks libev 服务端程序莫名其妙死掉了。所以，这里我们推荐采用V2ray 服务端程序来实现自建Shadowsocks翻墙服务器。

使用 [V2ray官方一键安装脚本](https://github.com/bannedbook/fanqiang/blob/master/v2ss/V2ray%E5%AE%98%E6%96%B9%E4%B8%80%E9%94%AE%E5%AE%89%E8%A3%85%E8%84%9A%E6%9C%AC.md) 安装V2ray后，我们需要修改一下V2ray配置文件，以启用Shadowsocks服务。

熟悉Linux的网友可以直接在VPS服务端编辑： /etc/v2ray/config.json 这个文件，一般网友推荐把这个文件下载到本地，编辑修改后，再上传到VPS服务器上。

VPS传输文件教程请见： [使用FileZilla和VPS传输文件教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E4%BD%BF%E7%94%A8FileZilla%E5%92%8CVPS%E4%BC%A0%E8%BE%93%E6%96%87%E4%BB%B6%E6%95%99%E7%A8%8B.md)

在本地编辑/etc/v2ray/config.json 文件推荐使用 [Notepad++](https://notepad-plus-plus.org/downloads/) 这个开源免费的编辑器，非常好用。

话不多说了，直接给出V2ray实现Shadowsocks的配置文件config.json例子：

```
{
  "log": {
    "loglevel": "warning",
    "access": "/dev/null",
    "error": "/dev/null"
  },
  "inbounds": [
    {
      "port": 51888,
      "protocol": "shadowsocks",
      "settings": {
        "method": "aes-256-gcm",
        "password": "www.bannedbook.org",
        "network": "tcp,udp",
        "level": 0
      }
    }
  ],
  "outbounds": [
    {
      "protocol": "freedom",
      "settings": {},
      "tag": "allowed"
    },
    {
      "protocol": "blackhole",
      "settings": {},
      "tag": "blocked"
    }
  ],
  "routing": {
    "rules": [
      {
        "domain": [
          "google.com",
          "apple.com",
          "oppomobile.com"
        ],
        "type": "field",
        "outboundTag": "allowed"
      },
      {
        "type": "field",
        "ip": [
          "geoip:private"
        ],
        "outboundTag": "blocked"
      }
    ]
  }
}
```
你只需要完整复制这个config.json例子，使用Notepad++将里面的 port 、password 修改成你想要的服务器端口和密码，端口小于65535 , 密码任意设置，跟客户端一致即可。

Shadowsocks加密算法有很多种，可以粗略的分为AEAD加密和非AEAD加密，具体技术细节有兴趣了解的可以自行Google。现在请务必使用AEAD加密算法（chacha20-ietf-poly1305、xchacha20-ietf-poly1305、aes-128-gcm、aes-192-gcm、aes-256-gcm），目前xchacha20-ietf-poly1305和aes-256-gcm是最佳的选择，由于各大平台的cpu现在对aes算法都有较好的优化，这里个人推荐aes-256-gcm 。见上面配置文件中的 `"method": "aes-256-gcm"`  。

然后上传config.json文件，覆盖vps上的/etc/v2ray/config.json 文件。

然后在vps上执行命令检查下配置文件： `/usr/bin/v2ray/v2ray -test -config /etc/v2ray/config.json`

如过配置文件没有错误的话，以上命令会输出：`......Configuration OK.`

然后重启v2ray服务端命令： `service v2ray restart`

接下来配置Shadowsocks客户端即可。

***

**第五步：Shadowsocks客户端配置**

我们以<a href="https://github.com/shadowsocks/shadowsocks-windows/releases">Windows版的Shadowsocks官方客户端</a>为例，简单示范客户端配置如下图:

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/ss/ss-win1.jpg)

注意：这里的端口，填写上面 “第四步 安装Shadowsocks服务器” 中config.json文件中 "port": 后面的数字，密码 和 加密 也必须和第四步中config.json里面保持一致。
配置好客户端，就可以自由冲浪了！ 其它客户端的配置也都差不多与此类似。

<b>关于客户端的选择，请特别注意：绝大多数SSR客户端不支持AEAD加密，所以必须要使用 [SS客户端](https://shadowsocks.org/en/download/clients.html) ，而不是SSR客户端。</b>

***
<b>相关教程</b>： [自建V2ray服务器简明教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md)
***

有问题可以<a href="https://github.com/bannedbook/fanqiang/issues">发Issue</a>交流。

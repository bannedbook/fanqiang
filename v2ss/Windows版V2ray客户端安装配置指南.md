# Windows版V2ray客户端安装配置指南

## V2ray官方Windows客户端安装配置

<b>广告插播，如果你觉得自己折腾VPS太麻烦，可以考虑这个服务哦（非本库服务）：</b><br>
<a href="https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA"><img src="https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2free.jpg" height="300" alt="V2free翻墙-不限流量、高速稳定、性价比超强"></a>

点<a href="https://github.com/v2ray/v2ray-core/releases" target="_blank" rel="noopener">这里</a>下载 V2Ray 的 Windows 压缩包，如果是 32 位系统，下载 v2ray-windows-32.zip，如果是 64 位系统，下载 v2ray-windows-64.zip。 下载解压之后会有 v2ray.exe 和 config.json 这两个文件，v2ray.exe 是运行 v2ray 的文件，config.json 是配置文件。你可以通过记事本或其它的文本编辑器打开查看。

浏览器里设置代理。以火狐（Firefox）为例，点菜单 -&gt; 选项 -&gt; 高级 -&gt; 设置 -&gt; 手动代理设置，在 SOCKS Host 填上 127.0.0.1，后面的 Port 填 1080，再勾上使用 SOCKS v5 时代理 DNS (这个勾选项在旧的版本里叫做远程 DNS)。操作图见下：

<img src="https://toutyrater.github.io/resource/images/firefox_proxy_setting1.png" alt="" />

<img src="https://toutyrater.github.io/resource/images/firefox_proxy_setting2.png" alt="" />

<img src="https://toutyrater.github.io/resource/images/firefox_proxy_setting3.png" alt="" />

<img src="https://toutyrater.github.io/resource/images/firefox_proxy_setting4.png" alt="" />

如果使用的是其它的浏览器，请自行在网上搜一下怎么设置 SOCKS 代理。或者也可以使用浏览器插件，如 SwitchyOmega 等。

以下是官方客户端配置，将客户端的 config.json 文件修改成下面的内容，<b>修改完成后要重启 V2Ray 才会使修改的配置生效</b>。
```javascript
{
  "inbounds": [
    {
      "port": 1080, // 监听端口
      "protocol": "socks", // 入口协议为 SOCKS 5
      "sniffing": {
        "enabled": true,
        "destOverride": ["http", "tls"]
      },
      "settings": {
        "auth": "noauth"  //socks的认证设置，noauth 代表不认证，由于 socks 通常在客户端使用，所以这里不认证
      }
    }
  ],
  "outbounds": [
    {
      "protocol": "vmess", // 出口协议
      "settings": {
        "vnext": [
          {
            "address": "serveraddr.com", // 服务器地址，请修改为你自己的服务器 IP 或域名
            "port": 33333,  // 服务器端口
            "users": [
              {
                "id": "b9a7e7ac-e9f2-4ac2-xxxx-xxxxxxxxxx",  // 用户 ID，必须与服务器端配置相同
                "alterId": 0
              }
            ]
          }
        ]
      }
    }
  ]
}
```

在配置当中，有一个 id (在这里的例子是 b9a7e7ac-e9f2-4ac2-xxxx-xxxxxxxxxx)，作用类似于 Shadowsocks 的密码(password), VMess 的 id 的格式必须与 UUID 格式相同。关于 id 或者 UUID 没必要了解很多，在这里只要清楚以下几点就足够了：
* 相对应的 VMess 传入传出的 id 必须相同（如果你不是很明白这句话，那么可以简单理解成服务器与客户端的 id 必须相同）
* 由于 id 使用的是 UUID 的格式，我们可以使用任何 UUID 生成工具生成 UUID 作为这里的 id（<b>一般我们配置客户端时直接使用安装服务器时自动生成的id即可</b>）。比如 [UUID Generator](https://www.uuidgenerator.net/) 这个网站，只要一打开或者刷新这个网页就可以得到一个 UUID，如下图。或者可以在 Linux 使用命令 `cat /proc/sys/kernel/random/uuid` 生成。

![](/resource/images/generate_uuid.png)

本文属于bannedbook系列翻墙教程的一部分，相关教程如下：
* [自建V2ray服务器简明教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md)	
* [自建Shadowsocks服务器简明教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAShadowsocks%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md) 

## V2ray客户端配置文件(绕过国内域名及IP)

上面的客户端配置是全局的，也就是说访问所有的网站都将翻墙。但v2ray可以配置路由，绕过国内域名及IP，将配置文件改成如下即可（服务器端无需修改）
```javascript
{
    "inbounds": [
        {
            "port": 1080,
            "protocol": "socks",
            "sniffing": {
                "enabled": true,
                "destOverride": [
                    "http",
                    "tls"
                ]
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
                        "address": "serveraddr.com",
                        "port": 33333,
                        "users": [
                            {
                                "id": "b9a7e7ac-e9f2-4ac2-xxxx-xxxxxxxxxx",
                                "alterId": 0
                            }
                        ]
                    }
                ]
            },
            "streamSettings": {
                "network": "tcp",
                "security": "tls"
            }
        },
        {
            "protocol": "freedom",
            "settings": {},
            "tag": "direct" //如果要使用路由，这个 tag 是一定要有的
        }
    ],
    "routing": {
        "domainStrategy": "IPOnDemand",
        "rules": [
            {
                "type": "field",
                "outboundTag": "direct",
                "domain": [
                    "geosite:cn"
                ] // 中国大陆主流网站的域名
            },
            {
                "type": "field",
                "outboundTag": "direct",
                "ip": [
                    "geoip:cn", // 中国大陆的 IP
                    "geoip:private" // 私有地址 IP，如路由器等
                ]
            }
        ]
    }
}
```

说明：配置文件很容易出错，最好还是使用 V2Ray 提供的配置检查功能（test 选项），因为可以检查 JSON 语法错误外的问题。

`v2ray -test -config config.json`

如果是配置文件没问题，则是这样的：
```
V2Ray 4.21.3 (V2Fly, a community-driven edition of V2Ray.) Custom
A unified platform for anti-censorship.
Configuration OK.
```

## Windows下的第三方V2ray客户端

也可以使用Windows下第三方的V2ray客户端<a href="https://github.com/2dust/v2rayN/releases/latest">v2rayN</a>，v2rayN的客户端配置简单示范如下图:

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2ray/client1.jpg)

注意：这里的端口，填写上面 “第四步 [安装V2ray服务器](https://github.com/bannedbook/fanqiang/blob/master/v2ss/V2ray%E5%AE%98%E6%96%B9%E4%B8%80%E9%94%AE%E5%AE%89%E8%A3%85%E8%84%9A%E6%9C%AC.md)” 安装完成后显示的Port:后面的数字，用户ID填写第四步显示的UUID:后面的一串字符即可。
配置好客户端，就可以自由冲浪了！至此为止，是不是很简单，有人说V2ray配置复杂，我们怎么没觉得呢？

本文属于bannedbook系列翻墙教程的一部分，欢迎体验我们提供的免费翻墙软件和教程：

  
* [自建V2ray服务器简明教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md)	
  
* [自建Shadowsocks服务器简明教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAShadowsocks%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md) 
  <ul>
<li><a href="https://github.com/bannedbook/fanqiang/wiki/%E5%AE%89%E5%8D%93%E7%BF%BB%E5%A2%99%E8%BD%AF%E4%BB%B6">安卓手机翻墙</a></li>
 <li><a href="https://github.com/bannedbook/fanqiang/wiki/Chrome%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" >Chrome一键翻墙包</a></li>
 <li><a href="https://github.com/bannedbook/fanqiang/wiki/%E7%81%AB%E7%8B%90firefox%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" >火狐firefox一键翻墙包</a></li>
 <li><a href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md" >自建V2ray服务器简明教程</a></li>
</ul>


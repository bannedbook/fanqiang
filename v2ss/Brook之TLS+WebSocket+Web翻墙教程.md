# Brook之TLS+WebSocket+Web服务器翻墙教程

接前文 [Brook之TLS+WebSocket翻墙教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/Brook%E4%B9%8BTLS%2BWebSocket%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B.md) ，本文将介绍Brook之TLS+WebSocket+Web服务器的翻墙方法。

首先把VPS上的Brook进程停掉，然后以下面的命令重新启动：

`setsid ./brook wsserver -l :18000 -p yourPassword`

然后在VPS上下载安装Caddy web服务器，简略过程如下：

```
wget https://github.com/caddyserver/caddy/releases/download/v1.0.4/caddy_v1.0.4_linux_amd64.tar.gz
tar -xzf caddy*.tar.gz caddy
mv ./caddy /usr/local/bin
mkdir mycaddy
cd mycaddy
vi Caddyfile
```

然后把以下内容保存到Caddyfile里面：

```
www.mydomain.com
{
  log ./caddy.log
  proxy / localhost:18000 {
    websocket
    header_upstream -Origin
  }
}
```
如果你不会vi编辑文件，那么可以在本地编辑好这个文件，再上传到VPS服务器上。VPS传输文件教程请见： [使用FileZilla和VPS传输文件教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E4%BD%BF%E7%94%A8FileZilla%E5%92%8CVPS%E4%BC%A0%E8%BE%93%E6%96%87%E4%BB%B6%E6%95%99%E7%A8%8B.md)

在本地编辑配置文件推荐使用 [Notepad++](https://notepad-plus-plus.org/downloads/) 这个开源免费的编辑器，非常好用。

然后启动Caddy，不熟悉Caddy的网友可以到这里看一下：

https://caddyserver.com/v1/tutorial/beginner

这样，我们的Caddy web服务器就以TLS+WebSocket监听在443端口，然后会转发数据包到后端的Brook，本文中我们的Brook监听在18000端口，你可以改变这个端口，但要注意，Brook 和 Caddy 需要同时改变这个端口，二者保持一致。

客户端和前文一样，无需改变。
还是这个命令：

`brook_windows_386.exe wsclient -l 127.0.0.1:2080 -i 127.0.0.1 -s wss://www.mydomain.com:443 -p yourPassword`

## 相关翻墙教程


* [Brook之TLS+WebSocket+CDN翻墙教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/Brook%E4%B9%8BTLS%2BWebSocket%2BCDN%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B.md)

* [Brook之TLS+WebSocket+Web服务器翻墙教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/Brook%E4%B9%8BTLS%2BWebSocket%2BWeb%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B.md)

* [Brook之TLS+WebSocket翻墙教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/Brook%E4%B9%8BTLS%2BWebSocket%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B.md)

* <a href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md">自建V2ray服务器简明教程</a>

* <a href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2Ray%2BTLS%E7%BF%BB%E5%A2%99%E9%85%8D%E7%BD%AE%E6%96%B9%E6%B3%95.md">自建V2Ray+TLS翻墙配置方法</a>

* <a href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/V2Ray%E4%B9%8BTLS%2BWebSocket%E7%BF%BB%E5%A2%99%E6%96%B9%E6%B3%95.md">V2Ray之TLS+WebSocket翻墙方法</a>

* <a title="自建Shadowsocks服务器简明教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAShadowsocks%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md">自建Shadowsocks服务器简明教程</a>

* <a title="SSH连接VPS教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/SSH%E8%BF%9E%E6%8E%A5VPS%E6%95%99%E7%A8%8B.md">SSH连接VPS教程</a>

* <a title="V2ray官方一键安装脚本" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/V2ray%E5%AE%98%E6%96%B9%E4%B8%80%E9%94%AE%E5%AE%89%E8%A3%85%E8%84%9A%E6%9C%AC.md">V2ray官方一键安装脚本</a>

* <a title="Windows版V2ray客户端安装配置指南" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/Windows%E7%89%88V2ray%E5%AE%A2%E6%88%B7%E7%AB%AF%E5%AE%89%E8%A3%85%E9%85%8D%E7%BD%AE%E6%8C%87%E5%8D%97.md">Windows版V2ray客户端安装配置指南.md</a>

* <a title="使用FileZilla和VPS传输文件教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E4%BD%BF%E7%94%A8FileZilla%E5%92%8CVPS%E4%BC%A0%E8%BE%93%E6%96%87%E4%BB%B6%E6%95%99%E7%A8%8B.md">使用FileZilla和VPS传输文件教程</a>

* <a title="最简单的Google BBR 一键加速VPS教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E6%9C%80%E7%AE%80%E5%8D%95%E7%9A%84Google%20BBR%20%E4%B8%80%E9%94%AE%E5%8A%A0%E9%80%9FVPS%E6%95%99%E7%A8%8B.md">最简单的Google BBR 一键加速VPS教程</a>

* <a title="翻墙VPS推荐：搬瓦工VPS购买教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E7%BF%BB%E5%A2%99VPS%E6%8E%A8%E8%8D%90%EF%BC%9A%E6%90%AC%E7%93%A6%E5%B7%A5VPS%E8%B4%AD%E4%B9%B0%E6%95%99%E7%A8%8B.md">翻墙VPS推荐：搬瓦工VPS购买教程</a>

* <a  href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%B4%AD%E4%B9%B0Vultr%20VPS%E5%9B%BE%E6%96%87%E6%95%99%E7%A8%8B.md">购买Vultr VPS图文教程【新用户赠送100美元】</a>





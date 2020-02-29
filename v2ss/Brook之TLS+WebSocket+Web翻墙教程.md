# Brook之TLS+WebSocket+Web翻墙教程

接前文 [Brook之TLS+WebSocket+Web翻墙教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/Brook%E4%B9%8BTLS%2BWebSocket%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B.md) ，本文将介绍Brook之TLS+WebSocket+Web的翻墙方法。

首先在VPS上下载安装Candy web服务器，简略过程如下：

```
wget https://github.com/caddyserver/caddy/releases/download/v1.0.4/caddy_v1.0.4_linux_amd64.tar.gz
tar -xzf caddy*.tar.gz caddy
mv ./caddy /usr/local/bin
mkdir mycandy
cd mycandy
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

在本地编辑/etc/v2ray/config.json 文件推荐使用 [Notepad++](https://notepad-plus-plus.org/downloads/) 这个开源免费的编辑器，非常好用。

然后启动Candy，不熟悉Candy的网友可以到这里看一下：

https://caddyserver.com/v1/tutorial/beginner




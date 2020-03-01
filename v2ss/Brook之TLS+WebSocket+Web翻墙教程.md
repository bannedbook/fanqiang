# Brook之TLS+WebSocket+Web服务器翻墙教程

接前文 [Brook之TLS+WebSocket翻墙教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/Brook%E4%B9%8BTLS%2BWebSocket%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B.md) ，本文将介绍Brook之TLS+WebSocket+Web服务器的翻墙方法。

首先把VPS上的Brook进程停掉，然后以下面的命令重新启动：

`setsid ./brook wsserver -l :18000 -p yourPassword`

然后在VPS上下载安装Candy web服务器，简略过程如下：

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

这样，我们的Caddy web服务器就以TLS+WebSocket监听在443端口，然后会转发数据包到后端的Brook，本文中我们的Brook监听在18000端口，建议你改变这个端口，不要完全照搬哦。注意，Brook 和 Candy 需要同时改变这个端口，二者保持一致。

客户端和前文一样，无需改变。
还是这个命令：

`brook_windows_386.exe wsclient -l 127.0.0.1:2080 -i 127.0.0.1 -s wss://www.mydomain.com:443 -p yourPassword`






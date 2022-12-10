<h1>Brook之TLS+WebSocket翻墙教程</h1>

**Brook之TLS+WebSocket翻墙，整个教程分简单几步**：

购买VPS服务器、一键加速VPS服务器、安装Brook服务器、启动客户端。

虽然很简单，但是如果你懒得折腾，那就用我们提供的免费翻墙软件吧：<br>
<a href="https://github.com/bannedbook/fanqiang/wiki/%E5%AE%89%E5%8D%93%E7%BF%BB%E5%A2%99%E8%BD%AF%E4%BB%B6">安卓手机翻墙</a>、<a href="https://github.com/bannedbook/fanqiang/wiki/Chrome%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" class="wiki-page-link">Chrome一键翻墙包</a>、<a href="https://github.com/bannedbook/fanqiang/wiki/%E7%81%AB%E7%8B%90firefox%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" class="wiki-page-link">火狐firefox一键翻墙包</a><br>

<b>或者也可以购买现成的翻墙服务(跟本库无关哦)：</b><br>
<a href="https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA"><img src="https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2free.jpg" height="300" alt="V2free翻墙-不限流量、高速稳定、性价比超强"></a>

***

**第一步：购买VPS服务器**

[购买Vultr VPS图文教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%B4%AD%E4%B9%B0Vultr%20VPS%E5%9B%BE%E6%96%87%E6%95%99%E7%A8%8B.md)
或 [搬瓦工VPS购买教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E7%BF%BB%E5%A2%99VPS%E6%8E%A8%E8%8D%90%EF%BC%9A%E6%90%AC%E7%93%A6%E5%B7%A5VPS%E8%B4%AD%E4%B9%B0%E6%95%99%E7%A8%8B.md)

***

**第二步：SSH连接服务器**

[SSH连接VPS教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/SSH%E8%BF%9E%E6%8E%A5VPS%E6%95%99%E7%A8%8B.md)

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/ss/xshell2.png)

SSH连接VPS成功后，会出现如上图所示，之后就可以复制粘贴linux命令脚本来执行了。

***

**第三步：Google BBR 一键加速VPS服务器**

[最简单的Google BBR 一键加速VPS教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E6%9C%80%E7%AE%80%E5%8D%95%E7%9A%84Google%20BBR%20%E4%B8%80%E9%94%AE%E5%8A%A0%E9%80%9FVPS%E6%95%99%E7%A8%8B.md)

***

**第四步：安装Brook服务端**

***注册一个域名***

如果已经注册有域名了可以跳过。
TLS 需要一个域名，域名有免费的和有付费的，如果你不舍得为一个域名每年花点钱，用个免费域名也可以，但总体来说付费的会优于免费的，我看到有网友因为用了免费域名，结果出现域名解析故障而翻墙失败。所以，还是推荐买个便宜点的域名。关于域名注册商，推荐[namesilo](https://www.namesilo.com/register.php?rid=43ac240wm) ,这家域名商有不少便宜的域名选择，比如.xyz域名，一年才0.99美元，很便宜，而且可选择免费域名隐私保护。为了方便，在本文中我就忽略如何注册购买域名了。关于如何获取域名，具体可搜索相关文章教程。

以下假设注册的域名为 mydomain.com ，请将之替换成你自己的域名。
注册好域名之后务必记得添加一个 A 记录指向你的 VPS! 一般将主域名 mydomain.com 和 `www.mydomain.com` 都做A记录指向你的VPS。
进行下一步之前确保A记录生效，可以ping一下域名测试确保解析正确。

前面我安装的debian 10 64位VPS, 这里我们下载这个当前（20200229）最新版本（下面三行每次拷贝一行执行，共三个命令）：

```
wget https://github.com/txthinking/brook/releases/download/v20200201/brook
chmod +x brook
setsid ./brook wsserver --domain www.mydomain.com -p yourPassword
```
这样Brook服务器端就启动起来了，Brook会自动申请免费TLS证书，保存在当前目录的子目录.letsencrypt下， Brook此时同时监听在 TCP 80/443 端口.

***如果更改域名的DNS配置或其它内容，或者是更换域名，可能必须# rm -rf .letsencrypt ，并重新启动brook wsserver 。***

***

**第五步：Brook客户端使用方法**

本文主要以Windows客户端为例，介绍Windows版Brook客户端的安装配置
到这里 

https://github.com/txthinking/brook/releases 

下载适合你最新的版本，当前是

https://github.com/txthinking/brook/releases/download/v20200201/brook_windows_386.exe

然后启动windows命令行，进入brook_windows_386.exe所在目录，然后执行(注意，有折行，请完整拷贝)：

`brook_windows_386.exe wsclient -l 127.0.0.1:2080 -i 127.0.0.1 -s wss://www.mydomain.com:443 -p yourPassword`

这样客户端就以本地Socks5代理的形式，启动监听在2080端口，启动你的浏览器，设置浏览器代理为Socks5代理：127.0.0.1:2080 就可以自由冲浪了。

**注意事项**
客户端和服务器时间必须保持一致。

***

**高级玩法**

我们可以在TLS+WebSocket基础上，再套上一个Web服务器，把Brook稍稍隐藏一下，详见：

[Brook之TLS+WebSocket+Web服务器翻墙教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/Brook%E4%B9%8BTLS%2BWebSocket%2BWeb%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B.md)

当封锁特别厉害的时候，或者IP被墙，可以再套上CDN，复活被墙VPS，详见：

[Brook之TLS+WebSocket+CDN翻墙教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/Brook%E4%B9%8BTLS+WebSocket+CDN%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B.md)

***

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


***

有问题可以<a href="https://github.com/bannedbook/fanqiang/issues">发Issue</a>交流。

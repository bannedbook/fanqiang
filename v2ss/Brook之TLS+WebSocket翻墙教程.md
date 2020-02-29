<h1>自建Brook服务器翻墙教程</h1>

**自建Brook服务器翻墙很简单，整个教程分简单几步**：

购买VPS服务器、一键加速VPS服务器、安装Brook服务器

虽然很简单，但是如果你懒得折腾，那就用我们提供的免费翻墙软件吧：<br>
<a href="https://github.com/bannedbook/fanqiang/wiki/%E5%AE%89%E5%8D%93%E7%BF%BB%E5%A2%99%E8%BD%AF%E4%BB%B6">安卓手机翻墙</a>、<a href="https://github.com/bannedbook/fanqiang/wiki/Chrome%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" class="wiki-page-link">Chrome一键翻墙包</a>、<a href="https://github.com/bannedbook/fanqiang/wiki/%E7%81%AB%E7%8B%90firefox%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" class="wiki-page-link">火狐firefox一键翻墙包</a>、<a href="https://github.com/bannedbook/xxjw" class="wiki-page-link">XX-Net禁闻版</a> <br>

<b>或者也可以购买现成的搬瓦工翻墙服务(跟本库无关哦)：</b><br>
<a href="https://github.com/killgcd/justmysocks/blob/master/README.md"><img src="https://raw.githubusercontent.com/killgcd/justmysocks/master/images/bwgss.jpg" alt="搬瓦工翻墙 Just My Socks"></a>

***

**第一步：购买VPS服务器**

Vultr VPS目前正举办优惠活动，新用户注册可获赠100美元。

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

注册好域名之后务必记得添加一个 A 记录指向你的 VPS!
以下假设注册的域名为 mydomain.com ，请将之替换成你自己的域名。

前面我安装的debian 10 64位VPS, 这里我们下载这个当前（20200229）最新版本：

```
wget https://github.com/txthinking/brook/releases/download/v20200201/brook
chmod +x brook
setsid ./brook wsserver --domain www.mydomain.com -p yourPassword
```
这样Brook服务器端就启动起来了，Brook会自动申请免费TLS证书，保存在当前目录的子目录.letsencrypt下， Brook此时同时监听在 TCP 80/443 端口.

***

**第五步：Brook客户端使用方法**

本文主要以Windows客户端为例，介绍Windows版Brook客户端的安装配置
到这里 https://github.com/txthinking/brook/releases 下载适合你最新的版本，当前是
https://github.com/txthinking/brook/releases/download/v20200201/brook_windows_386.exe
然后启动windows命令行，进入brook_windows_386.exe所做在目录
然后执行：

`brook_windows_386.exe wsclient -l 127.0.0.1:2080 -i 127.0.0.1 -s wss://www.mydomain.com:443 -p yourPassword`

***

**高级玩法**

当封锁特别厉害的时候，或者IP被墙，则可以在TLS+WebSocket基础上，再套上CDN，稍候推出Brook套CDN教程。

***

<b>相关教程</b>： 

<a title="自建V2ray服务器简明教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md">自建V2ray服务器简明教程</a>

<a title="自建V2Ray+TLS翻墙配置方法" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2Ray%2BTLS%E7%BF%BB%E5%A2%99%E9%85%8D%E7%BD%AE%E6%96%B9%E6%B3%95.md">自建V2Ray+TLS翻墙配置方法</a>

<a title="自建Shadowsocks服务器简明教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAShadowsocks%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md">自建Shadowsocks服务器简明教程</a>

<a title="SSH连接VPS教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/SSH%E8%BF%9E%E6%8E%A5VPS%E6%95%99%E7%A8%8B.md">SSH连接VPS教程</a>

<a title="V2ray官方一键安装脚本" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/V2ray%E5%AE%98%E6%96%B9%E4%B8%80%E9%94%AE%E5%AE%89%E8%A3%85%E8%84%9A%E6%9C%AC.md">V2ray官方一键安装脚本</a>

<a title="Windows版V2ray客户端安装配置指南" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/Windows%E7%89%88V2ray%E5%AE%A2%E6%88%B7%E7%AB%AF%E5%AE%89%E8%A3%85%E9%85%8D%E7%BD%AE%E6%8C%87%E5%8D%97.md">Windows版V2ray客户端安装配置指南.md</a>

<a title="使用FileZilla和VPS传输文件教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E4%BD%BF%E7%94%A8FileZilla%E5%92%8CVPS%E4%BC%A0%E8%BE%93%E6%96%87%E4%BB%B6%E6%95%99%E7%A8%8B.md">使用FileZilla和VPS传输文件教程</a>

<a title="最简单的Google BBR 一键加速VPS教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E6%9C%80%E7%AE%80%E5%8D%95%E7%9A%84Google%20BBR%20%E4%B8%80%E9%94%AE%E5%8A%A0%E9%80%9FVPS%E6%95%99%E7%A8%8B.md">最简单的Google BBR 一键加速VPS教程</a>

<a title="翻墙VPS推荐：搬瓦工VPS购买教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E7%BF%BB%E5%A2%99VPS%E6%8E%A8%E8%8D%90%EF%BC%9A%E6%90%AC%E7%93%A6%E5%B7%A5VPS%E8%B4%AD%E4%B9%B0%E6%95%99%E7%A8%8B.md">翻墙VPS推荐：搬瓦工VPS购买教程</a>

<a title="购买Vultr VPS图文教程" href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%B4%AD%E4%B9%B0Vultr%20VPS%E5%9B%BE%E6%96%87%E6%95%99%E7%A8%8B.md">购买Vultr VPS图文教程.md</a>

***

有问题可以<a href="https://github.com/bannedbook/fanqiang/issues">发Issue</a>交流。

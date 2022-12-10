<h1>自建V2ray服务器简明教程</h1>

**自建v2ray教程很简单，整个教程分简单几步**：

购买VPS服务器、一键加速VPS服务器、安装V2ray服务端

虽然很简单，但是如果你懒得折腾，那就用我们提供的免费翻墙软件吧：<br>
<a href="https://github.com/bannedbook/fanqiang/wiki/%E5%AE%89%E5%8D%93%E7%BF%BB%E5%A2%99%E8%BD%AF%E4%BB%B6">安卓手机翻墙</a>、<a href="https://github.com/bannedbook/fanqiang/wiki/Chrome%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" class="wiki-page-link">Chrome一键翻墙包</a>、<a href="https://github.com/bannedbook/fanqiang/wiki/%E7%81%AB%E7%8B%90firefox%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" class="wiki-page-link">火狐firefox一键翻墙包</a><br>

<b>或者也可以购买现成的翻墙服务(跟本库无关哦)：</b><br>
<a href="https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA"><img src="https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2free.jpg" height="300" alt="V2free翻墙-不限流量、高速稳定、性价比超强"></a>

***

**【前言】**

**v2ray的优势**

v2ray支持的传输方式有很多，包括：普通TCP、HTTP伪装、WebSocket流量、普通mKCP、mKCP伪装FaceTime通话、mKCP伪装BT下载流量、mKCP伪装微信视频流量，不同的传输方式其效果会不同，有可能会遇到意想不到的效果哦！当然国内不同的地区、不同的网络环境，效果也会不同，所以具体可以自己进行测试。现在v2ray客户端也很多了，有windows、MAC、linux和安卓版、IOS。

注意：如果你选择使用 V2ray，建议关闭并删除所有的ss/ssr服务端，避开互相干扰。

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

**第四步：安装V2ray服务端**

这里我们采用v2ray官方的一键安装脚本，见教程：[V2ray官方一键安装脚本](https://github.com/bannedbook/fanqiang/blob/master/v2ss/V2ray%E5%AE%98%E6%96%B9%E4%B8%80%E9%94%AE%E5%AE%89%E8%A3%85%E8%84%9A%E6%9C%AC.md)

注意：vultr debian 10 现在防火墙好像默认只开了 ssh 端口，所以需要[手工开通vultr debian 10防火墙端口](https://github.com/bannedbook/fanqiang/issues/669#issuecomment-871351813)

***

**第五步：V2ray客户端配置**

本文主要以Windows客户端为例，介绍Windows版V2ray客户端的安装配置，请参见：[Windows版V2ray客户端安装配置指南](https://github.com/bannedbook/fanqiang/blob/master/v2ss/Windows%E7%89%88V2ray%E5%AE%A2%E6%88%B7%E7%AB%AF%E5%AE%89%E8%A3%85%E9%85%8D%E7%BD%AE%E6%8C%87%E5%8D%97.md)

关于更多平台的客户端使用，请参考：[v2ray各平台图文使用教程](https://github.com/bannedbook/fanqiang#windows-v2rayssssr-%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B)

***

**高级玩法**

当封锁特别厉害的时候，常规的v2ray配置可能已经无法满足需求，这个时候我们可以尝试下[V2Ray+TLS翻墙](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2Ray%2BTLS%E7%BF%BB%E5%A2%99%E9%85%8D%E7%BD%AE%E6%96%B9%E6%B3%95.md)以及[V2Ray之TLS+WebSocket翻墙](https://github.com/bannedbook/fanqiang/blob/master/v2ss/V2Ray%E4%B9%8BTLS%2BWebSocket%E7%BF%BB%E5%A2%99%E6%96%B9%E6%B3%95.md)的方式，甚至搭建好后还可以套CDN【详见：[V2Ray之TLS+WebSocket+Nginx+CDN配置方法](https://github.com/bannedbook/fanqiang/blob/master/v2ss/V2Ray%E4%B9%8BTLS+WebSocket+Nginx+CDN%E9%85%8D%E7%BD%AE%E6%96%B9%E6%B3%95.md)】，套CDN不是一个必须的步骤，但套CDN可以有效保护IP，甚至被墙的ip也能复活，如果套国内CDN，据说速度可以飞起来。

***

**常见问题参考解决方法**：

1、账号无法使用，可能原因一：**客户端与服务端的设备系统时间相差过大。**

当vps服务器与本地设备系统时间相差过大，会导致客户端无法与服务端建立链接。请修改服务器时区，再手动修改服务器系统时间（注意也要校准自己本地设备时间）！

先修改vps的时区为中国上海时区：cp -f /usr/share/zoneinfo/Asia/Shanghai /etc/localtime 

再手动修改vps系统时间命令的格式为（数字改为和自己电脑时间一致，误差30秒以内）：date -s "2018-11-02 19:14:00"   

修改后再输入date命令检查下。

2、账号无法使用，可能原因二：**Windows 防火墙、杀毒软件阻挡代理软件。**

如果以上问题都已排查，可以关闭 Windows 自带的防火墙、杀毒软件再尝试。

3、账号无法使用，可能原因三：**端口没有开放**


***
<b>相关教程</b>： [自建Shadowsocks服务器简明教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAShadowsocks%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md)
***

有问题可以<a href="https://github.com/bannedbook/fanqiang/issues">发Issue</a>交流。

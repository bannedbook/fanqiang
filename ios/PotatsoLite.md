# Potatso Lite 教程

1\. 简介
------

Potatso Lite 是一款免费的iOS翻墙工具，功能简单，稳定性尚可，支持的代理协议有：Shadowsocks, ShadowsocksR，适合新手入门使用。

2\. 下载
------

需要使用非中国大陆区AppleID下载。没有的话可以点击：
[注册苹果美区 Apple ID 帐号并购买APP指南](AppleID.md) 

https://apps.apple.com/us/app/potatso-lite/id1239860606

3\. 整理教程时的系统环境
--------------

iOS 14 

Potatso Lite 2.5.0

文档中的某些内容可能会随软件版本迭代而失效。

4\. 快速上手
--------

### 4.1 添加节点

首次打开软件时，启动页会出现“添加代理”的选项。软件首页右上角也有“+”号，点击即可添加节点。我们可以看到Potatso Lite支持的导入方式有：扫描二维码、节点链接、订阅。

![IMG_4605.jpg](https://v2free.org/docs/SSPanel/iOS/PotatsoLite_files/2252186481.jpg)  
![IMG_4592.jpg](https://v2free.org/docs/SSPanel/iOS/PotatsoLite_files/1558299922.jpg)

#### 4.1.1 扫描二维码

点击“添加代理”（或右上角+号），再点击“二维码”即可扫描SS节点二维码添加节点。

#### 4.1.2 节点链接

注册机场以获取节点URL或者找免费的节点。

这里我们推荐一个[V2ray机场，有免费V2ray和SS节点](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)，[点击注册](https://go.runba.cyou/auth/register?code=cd79)，注册后在该机场用户中心拷贝全部SS节点URL。

教育网的网友如果打不开上面的链接，请使用这个链接：
https://cdn.runba.cyou/auth/register?code=cd79

注册后免费获得1024M初始流量，每日[签到](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/checkin.jpg)可获得300-500M免费流量。
注册登录后，用个人邀请链接 邀请新用户注册还可获得流量奖励，如果新用户成为付费用户，你还可以赚取高达20%终生佣金奖励。

拷贝SS节点URL后，

点击“添加代理”（或右上角+号），再点击“代理地址”。在新的窗口里粘贴节点链接（ss://或ssr://开头的那种）。  

![IMG_4601.PNG](https://v2free.org/docs/SSPanel/iOS/PotatsoLite_files/2897544874.png)  

当软件检测到已经复制好了节点链接，也会提示是否需要自动导入，如图：  

![IMG_4600.PNG](https://v2free.org/docs/SSPanel/iOS/PotatsoLite_files/131924680.png)

#### 4.1.3 订阅(本站暂时不支持)

点击“添加代理”（或右上角+号），再点击“订阅”。然后在新的窗口里粘贴订阅链接并输入订阅名称（为了区分不同的订阅链接，可以随便起名字）。点击“完成”以保存设置。 
 
![IMG_4594.PNG](https://v2free.org/docs/SSPanel/iOS/PotatsoLite_files/3732450977.png)  

添加后会从订阅链接加载节点，如图：  

![IMG_4595.PNG](https://v2free.org/docs/SSPanel/iOS/PotatsoLite_files/2308989632.png)

### 4.2 更新订阅链接

有时节点提供商（机场）可能会修改节点配置信息，此时可以通过更新订阅链接来同步更改。  
点击软件主界面右上角的“管理”，进入“管理代理”界面。  

![IMG_4597.PNG](https://v2free.org/docs/SSPanel/iOS/PotatsoLite_files/2284973646.png) 
 
点开你添加的订阅，然后在右上角点击“完成”，即可更新。  
也可以在添加订阅的时候就选择“自动更新”。

### 4.3 其他设置

如果需要分流功能，可以点击软件右下角的“设置”，然后启用“智能路由”。  

![IMG_4599.PNG](https://v2free.org/docs/SSPanel/iOS/PotatsoLite_files/2559756792.png)

### 4.4 启用代理

设置完毕后在软件主页选一个节点，然后点击右下角的圆形按钮即可开启代理。首次启动代理时软件会申请创建本地VPN隧道，请选择允许。

## 相关阅读
*   [V2ray机场，V2ray/SS免费翻墙节点](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)

*   [安卓翻墙软件](https://github.com/bannedbook/fanqiang/wiki/%E5%AE%89%E5%8D%93%E7%BF%BB%E5%A2%99%E8%BD%AF%E4%BB%B6)
*   [安卓翻墙APP教程](https://github.com/bannedbook/fanqiang/tree/master/android)
*   [Chrome一键翻墙包](https://github.com/bannedbook/fanqiang/wiki/Chrome%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85)
*   [火狐firefox一键翻墙包](https://github.com/bannedbook/fanqiang/wiki/%E7%81%AB%E7%8B%90firefox%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85)
*   [自建V2ray服务器翻墙简明教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md)
*   [自建Shadowsocks服务器翻墙简明教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAShadowsocks%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md)
*   [免费ss账号](https://github.com/bannedbook/fanqiang/wiki/%E5%85%8D%E8%B4%B9ss%E8%B4%A6%E5%8F%B7)
*   [v2ray免费账号](https://github.com/bannedbook/fanqiang/wiki/v2ray%E5%85%8D%E8%B4%B9%E8%B4%A6%E5%8F%B7)
*   [苹果电脑MAC翻墙](https://github.com/bannedbook/fanqiang/wiki/%E8%8B%B9%E6%9E%9C%E7%94%B5%E8%84%91MAC%E7%BF%BB%E5%A2%99)
*   [iphone翻墙](https://github.com/bannedbook/fanqiang/wiki/iphone%E7%BF%BB%E5%A2%99)
*   [TorBrowser一键翻墙包](https://github.com/bannedbook/fanqiang/wiki/TorBrowser%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85)
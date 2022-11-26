V2rayN配置使用教程
============

应用概述
----

V2rayN 是在 WIN 平台上的客户端软件，支持 VMess 协议。 

V2rayN 要求系统安装有 [Microsoft .NET Framework 4.8](https://dotnet.microsoft.com/download/dotnet-framework/thank-you/net48-web-installer) 或更高版本。如果程序启动不了，请先安装Microsoft .NET Framework 。

应用下载
----

以下是各平台该应用的下载地址。 **下载链接：** [点击下载软件](https://v2free.org/ssr-download/v2rayn.zip)

获取订阅
----

注册机场以获取节点订阅链接或者找免费的订阅链接。

这里我们推荐一个[V2ray机场，有免费V2ray和SS节点](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)，[点击注册](https://go.runba.cyou/auth/register?code=cd79)，注册后在该机场用户中心拷贝订阅链接。

教育网的网友如果打不开上面的链接，请使用这个链接：
https://cdn.runba.cyou/auth/register?code=cd79

注册后免费获得1024M初始流量，每日[签到](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/checkin.jpg)可获得300-500M免费流量。
注册登录后，用个人邀请链接 邀请新用户注册还可获得流量奖励，如果新用户成为付费用户，你还可以赚取高达20%终生佣金奖励。

机场的 **订阅链接** 非常重要，你应当把它当做密码一样妥善保管。

配置 V2rayN
---------

解压 v2rayN-Core 压缩包到硬盘，启动v2rayn.exe 点击左上角的菜单图标打开侧边栏，随后点击 **订阅设置**。 

![1](https://v2free.org/docs/SSPanel/Windows/V2RayN_files/v2rayN1.png) 

点击订阅按钮，在新页面点击添加，粘贴订阅号，点击确定 **√**。 

![2](https://v2free.org/docs/SSPanel/Windows/V2RayN_files/v2rayN2.png)

然后点：订阅/更新订阅

![2.5](https://v2free.org/docs/SSPanel/Windows/V2RayN_files/v2rayN2.5.jpg)

然后，在windows右下角托盘区，右键点V2rayn的图标，勾选：自动配置系统代理，如下图：

![3](https://v2free.org/docs/SSPanel/Windows/V2RayN_files/v2rayN3.png)

开始使用
----
在V2rayN主界面，按Ctrl+A选择全部节点，点右键，点“测试服务器真连接延迟（多选）”，测速完毕后，
点击任务栏图标，右键选择您中意的节点，勾选后点“系统代理/自动配置系统代理”。 
这样IE、Edge、Chrome等使用系统代理的浏览器就可以翻出去了。

## 相关阅读
*   [V2ray机场，V2ray/SS免费翻墙节点](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)
*   [安卓翻墙APP](https://github.com/bannedbook/fanqiang/wiki/%E5%AE%89%E5%8D%93%E7%BF%BB%E5%A2%99%E8%BD%AF%E4%BB%B6)
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

**V2rayN的三种代理模式**

“清除系统代理”: 禁止使用Windows系统的代理,不设置任何代理。

“自动配置系统代理”：设置使用V2rayn的代理。

“不改变系统代理”：保持Windows原有的代理设置，不做任何改变。
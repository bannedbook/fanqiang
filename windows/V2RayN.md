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

这里我们推荐一个[V2ray机场，有免费V2ray和SS节点](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)，[点击注册](https://w1.v2dns.xyz/auth/register?code=cd79)，注册后在该机场用户中心拷贝订阅链接。
注册后免费获得1024M初始流量，每日[签到](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/checkin.jpg)可获得300-500M免费流量。更有不限流量节点可用。
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

然后继续：

![3](https://v2free.org/docs/SSPanel/Windows/V2RayN_files/v2rayN3.png)

开始使用
----
在V2rayN主界面，按Ctrl+A选择全部节点，点右键，点“测试服务器真连接延迟（多选）”，测速完毕后，
点击任务栏图标，右键选择您中意的节点，勾选后点“系统代理/自动配置系统代理”。 
这样IE、Edge、Chrome等使用系统代理的浏览器就可以翻出去了。

**V2rayN的三种代理模式**

“清除系统代理”: 禁止使用Windows系统的代理,不设置任何代理。

“自动配置系统代理”：设置使用V2rayn的代理。

“不改变系统代理”：保持Windows原有的代理设置，不做任何改变。
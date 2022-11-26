# ClashX 翻墙教程

## 应用概述

ClashX 是一个拥有 GUI 界面基于 Clash 可自定义规则的 macOS 代理应用。

支持 Shadowsocks 协议和其 simple-obfs 插件、v2ray-plugin 插件以及 V2ray/VMess 协议和其 TCP、WebSocket 等传输方式。

## 应用下载

以下是ClashX的下载地址。

- 官方下载：[ClashX](https://github.com/yichengchen/clashX/releases)

下载ClashX的安装文件，文件格式为”dmg”格式，相当于一个光盘镜像文件。
下载的文件一般放置于用户的”下载”文件夹，使用 Finder找到下载文件。

## 获取订阅

注册机场以获取 Clash 订阅链接或者找免费的订阅链接。

这里我们推荐一个[V2ray机场，有免费V2ray和SS节点](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)，[点击注册](https://go.runba.cyou/auth/register?code=cd79)，注册后在该机场拷贝Clash订阅链接。

教育网的网友如果打不开上面的链接，请使用这个链接：
https://cdn.runba.cyou/auth/register?code=cd79

注册后免费获得1024M初始流量，每日[签到](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2sshttps://v2free.org/docs/SSPanel/macOS/images/checkin.jpg)可获得300-500M免费流量。
注册登录后，用个人邀请链接 邀请新用户注册还可获得流量奖励，如果新用户成为付费用户，你还可以赚取高达20%终生佣金奖励。

机场的 **订阅链接** 非常重要，你应当把它当做密码一样妥善保管。

## 配置 ClashX

双击ClashX.dmg，打开 ClashX安装程序

![1](https://v2free.org/docs/SSPanel/macOS/images/ClashX-1.png)

图：运行 ClashX 安装程序

此时，桌面上会生成一个虚拟光盘，并将下载的镜像文件装载到该光盘，并弹出一个窗口，按照提示将窗口左侧的”ClashX”图标拖拽到窗口右侧的”Applications”文件夹，即完成了 ClashX 的安装。

安装过程其实就相当于把 ClashX 的程序文件夹复制到 Mac 电脑中，放置在”Applicationes”目录是方便应用程序的访问和使用。

复制完成后，就可以在应用程序中看到 ClashX 应用图标，表示安装已经成功。我们就可以把虚拟光盘弹出，然后删除下载目录中的 dmg 文件。

第一次启动ClashX时，依次点击：打开、安装、输入密码，点击“安装帮助程序”，即可启动ClashX了（如下图）。

![1](https://v2free.org/docs/SSPanel/macOS/images/clashx1.jpg)

![1](https://v2free.org/docs/SSPanel/macOS/images/clashx2.jpg)

如果程序打不开，请参考：[打开来自身份不明开发者的 Mac App1](https://support.apple.com/zh-cn/guide/mac-help/mh40616/mac)或[https://www.jianshu.com/p/3a5ceb412f15](https://www.jianshu.com/p/3a5ceb412f15)

点击菜单栏中 ClashX 的图标，选择 配置 => 托管配置 => 管理，

![2](https://v2free.org/docs/SSPanel/macOS/images/ClashX-2.png)

然后点击 添加 ，粘贴上方 **[获取订阅](#获取订阅)** 中的拷贝的订阅链接（注意，粘贴后如果看不到url，可能是因为多了一个空行，按一次“Backspace删除键”即可）。

![3](https://v2free.org/docs/SSPanel/macOS/images/ClashX-3.png)
![3](https://v2free.org/docs/SSPanel/macOS/images/ClashX-3a.png)

点击菜单栏中 ClashX 的图标，出站模式选择 **规则**，勾选下方的 **设置为系统代理** 以及 **开机启动（可选）**。

注意，ClashX目前不支持v2ray的vless协议，所以vless节点显示为失败。

![4](https://v2free.org/docs/SSPanel/macOS/images/ClashX-4.png)

## 开始使用

点击菜单栏中 ClashX 的图标，然后在下方的 **Proxy 策略组** 中 节点选择 选 自动选择 或者你中意的节点即可。

## 其他注意

请不要修改 `~/.config/clash/config.yml` 中的端口配置，否则会导致应用异常。

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
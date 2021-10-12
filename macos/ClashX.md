# ClashX 翻墙教程

## 应用概述

ClashX 是一个拥有 GUI 界面基于 Clash 可自定义规则的 macOS 代理应用。

支持 Shadowsocks 协议和其 simple-obfs 插件、v2ray-plugin 插件以及 VMess 协议和其 TCP、WebSocket 等传输方式。

## 应用下载

以下是ClashX的下载地址。

- 官方下载：[ClashX](https://github.com/yichengchen/clashX/releases)
- 本站下载：[ClashX](https://v2free.org/ssr-download/ClashX.dmg)

下载ClashX的安装文件，文件格式为”dmg”格式，相当于一个光盘镜像文件。
下载的文件一般放置于用户的”下载”文件夹，使用 Finder找到下载文件。

## 获取订阅


注册机场以获取节点订阅链接或者找免费的订阅链接。

这里我们推荐一个[V2ray机场，有免费V2ray和SS节点](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)，[点击注册](https://w1.v2dns.xyz/auth/register?code=cd79)，注册后在该机场用户中心拷贝订阅链接。
注册后免费获得1024M初始流量，每日[签到](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2sshttps://v2free.org/docs/SSPanel/macOS/images/checkin.jpg)可获得300-500M免费流量。更有不限流量节点可用。
注册登录后，用个人邀请链接 邀请新用户注册还可获得流量奖励，如果新用户成为付费用户，你还可以赚取高达20%终生佣金奖励。

机场的 **订阅链接** 非常重要，你应当把它当做密码一样妥善保管。

## 配置 ClashX

双击ClashX.dmg，打开 ClashX安装程序

![1](https://v2free.org/docs/SSPanel/macOS/images/ClashX-1.png ':size=400')

图：运行 ClashX 安装程序

此时，桌面上会生成一个虚拟光盘，并将下载的镜像文件装载到该光盘，并弹出一个窗口，按照提示将窗口左侧的”ClashX”图标拖拽到窗口右侧的”Applications”文件夹，即完成了 ClashX 的安装。

安装过程其实就相当于把 ClashX 的程序文件夹复制到 Mac 电脑中，放置在”Applicationes”目录是方便应用程序的访问和使用。

复制完成后，就可以在应用程序中看到 ClashX 应用图标，表示安装已经成功。我们就可以把虚拟光盘弹出，然后删除下载目录中的 dmg 文件。

第一次启动ClashX时，依次点击：打开、安装、输入密码，点击“安装帮助程序”，即可启动ClashX了（如下图）。

![1](https://v2free.org/docs/SSPanel/macOS/images/clashx1.jpg ':size=400')

![1](https://v2free.org/docs/SSPanel/macOS/images/clashx2.jpg ':size=400')

点击菜单栏中 ClashX 的图标，选择 配置 => 托管配置 => 管理，

![2](https://v2free.org/docs/SSPanel/macOS/images/ClashX-2.png ':size=400')

然后点击 添加 ，粘贴上方 **[获取订阅](#获取订阅)** 中的拷贝的订阅链接（注意，粘贴后如果看不到url，可能是因为多了一个空行，按一次“Backspace删除键”即可）。

![3](https://v2free.org/docs/SSPanel/macOS/images/ClashX-3.png ':size=400')
![3](https://v2free.org/docs/SSPanel/macOS/images/ClashX-3a.png ':size=400')

点击菜单栏中 ClashX 的图标，出站模式选择 **规则**，勾选下方的 **设置为系统代理** 以及 **开机启动（可选）**。

注意，ClashX目前不支持v2ray的vless协议，所以vless节点显示为失败。

![4](https://v2free.org/docs/SSPanel/macOS/images/ClashX-4.png ':size=730')

## 开始使用

点击菜单栏中 ClashX 的图标，然后在下方的 **Proxy 策略组** 中 节点选择 选 自动选择 或者你中意的节点即可。

## 其他注意

请不要修改 `~/.config/clash/config.yml` 中的端口配置，否则会导致应用异常。


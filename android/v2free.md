# 安卓手机 V2free 翻墙教程

## 应用概述

V2free 是在 Android 平台上的客户端软件，支持 VMess/Vless 及 Shadowsocks 协议。

## 应用下载

[V2free下载](https://v2free.org/ssr-download/v2free.apk)

## 通过订阅链接将V2ray节点导入V2free

### 获取订阅链接

注册机场以获取节点订阅链接或者找免费的订阅链接。

这里我们推荐一个[V2ray机场，有免费V2ray和SS节点](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)，[点击注册](https://go.runba.cyou/auth/register?code=cd79)，注册后在该机场用户中心拷贝订阅链接。

教育网的网友如果打不开上面的链接，请使用这个链接：
https://cdn.runba.cyou/auth/register?code=cd79

注册后免费获得1024M初始流量，每日[签到](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/checkin.jpg)可获得300-500M免费流量。
注册登录后，用个人邀请链接 邀请新用户注册还可获得流量奖励。

机场 **订阅链接** 非常重要，你应当把它当做密码一样妥善保管。

### V2free导入订阅链接

打开 V2free 点击左上角的菜单图标打开侧边栏，随后点击 **订阅** （如下图）。

![1](https://v2free.org/docs/SSPanel/Android/images/v2free1.png)
![2](https://v2free.org/docs/SSPanel/Android/images/v2free2.png)

点击右上角的加号按钮，在 编辑订阅 的 地址输入框中粘贴上方 **获取订阅** 中的订阅链接，然后点确定按钮（如下图）。

![3](https://v2free.org/docs/SSPanel/Android/images/v2free3.png)
![4](https://v2free.org/docs/SSPanel/Android/images/v2free4.png)

随后从侧边栏回到 **服务器** 页面，点击右上角的 刷新图标 更新订阅（节点不定期更新，发现不好用时请更新订阅）（如下图）

![5](https://v2free.org/docs/SSPanel/Android/images/v2free5.png)
![6](https://v2free.org/docs/SSPanel/Android/images/v2free6.png)

## 节点测速

点右上角的 三个点 按钮，再点 测试全部配置真连接（如下图）。

![7](https://v2free.org/docs/SSPanel/Android/images/v2free7.png)
![8](https://v2free.org/docs/SSPanel/Android/images/v2free8.png)

## 路由设置
V2free APP的全局路由默认选项为：代理所有流量。如需设置国内外分流，请从侧边栏进入 **设置选项** 页面，点击 **V2ray路由** 和 **Shadowsocks路由** ，将其更改为 **绕过局域网及中国大陆地址**。（如下图）

路由设置分为V2ray路由和SS路由2个选项，新添加/更新的服务器的路由选项将采用全局路由设置值；改变全局路由设置时会重置所有相关类型节点的路由选项，即改变全局V2ray路由设置将重置所有v2ray节点，改变全局SS路由设置将重置所有SS节点。 

单个节点的路由设置（点节点右侧的铅笔图标设置）可以覆盖全局路由设置。

![9](https://v2free.org/docs/SSPanel/Android/images/v2free9.png)
![10](https://v2free.org/docs/SSPanel/Android/images/v2free10.png)
![11](https://v2free.org/docs/SSPanel/Android/images/v2free11.png)

## 分应用设置
也可以使用分应用设置进行分流，点节点右侧的铅笔图标，打开节点编辑界面，往下拉，点击：**分应用VPN** 进行设置。（如下图）
在分应用设置界面，点右上角三个点，可批量设置所有节点的分应用设置。

![12](https://v2free.org/docs/SSPanel/Android/images/v2free12.png)

## 开始使用

点击选择您中意的节点，点击主界面底部的小飞机按钮即可启动VPN连接。（如下图）
如操作系统提示添加 VPN 配置，请点击 **允许** 并验证您的 密码、指纹等。
启动vpn后，点选另一节点即可切换节点；再次点小飞机按钮可断开vpn。

![12](https://v2free.org/docs/SSPanel/Android/images/v2free13.png)

## 通过vmess url导入V2free

此方法更新节点不太方便，推荐首选上文介绍的订阅链接导入方法。

拷贝全部节点URL后(不是订阅链接)，在V2free主界面点右上角 "+" 按钮，再点：从剪贴板导入。

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
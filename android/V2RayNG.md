# 安卓手机 V2rayNG 翻墙教程

## 应用概述

V2rayNG 是在 Android 平台上的客户端软件，支持 VMess 及 Shadowsocks 协议。

## 应用下载

[V2rayNG 官方下载](https://github.com/2dust/v2rayNG/releases)

## 通过订阅链接将V2ray节点导入V2rayNG

### 获取订阅

注册机场以获取节点订阅链接或者找免费的订阅链接。

这里我们推荐一个[V2ray机场，有免费V2ray和SS节点](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)，[点击注册](https://go.runba.cyou/auth/register?code=cd79)，注册后在该机场用户中心拷贝订阅链接。

教育网的网友如果打不开上面的链接，请使用这个链接：
https://cdn.runba.cyou/auth/register?code=cd79

注册后免费获得1024M初始流量，每日[签到](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/checkin.jpg)可获得300-500M免费流量。
注册登录后，用个人邀请链接 邀请新用户注册还可获得流量奖励。

机场 **订阅链接** 非常重要，你应当把它当做密码一样妥善保管。

### 配置 V2rayNG

打开 V2rayNG 点击左上角的菜单图标打开侧边栏，随后点击 **订阅设置**。

![1](https://i.loli.net/2019/02/13/5c62fd8327c0e.png)

点击右上角的加号按钮，在新页面的备注中填写机场名称，地址输入框中粘贴上方 **获取订阅** 中的订阅链接并点击右上角的 **√**。

![2](https://i.loli.net/2019/02/13/5c62fef253cd4.jpg)

再次从侧边栏进入 **设置** 页面，点击 **路由模式** 将其更改为 **绕过局域网及大陆地址**。

![3](https://i.loli.net/2019/02/13/5c62ffab506fb.jpeg)

随后从侧边栏回到 **配置文件** 页面，点击右上角的省略号图标选择更新订阅、测试全部配置真连接（建议每日更新一次订阅）。

![4](https://i.loli.net/2019/02/13/5c630072445ec.jpeg)

### 开始使用

点击选择您中意的节点，点击右下角的按钮即可连接。

如操作系统提示添加 VPN 配置，请点击 运行 并验证您的 密码、指纹等。

### 相关阅读
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

## 通过vmess url导入V2rayNG

此方法更新节点不太方便，推荐首选上文介绍的订阅链接导入方法。

拷贝节点URL后(不是订阅链接)，在V2RayNG主界面点右上角 "+" 按钮，再点：从剪贴板导入。


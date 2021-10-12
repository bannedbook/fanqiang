# Quantumult 教程

## 应用概述

Quantumult 是在 iOS 平台上的客户端软件，支持 Shadowsocks、ShadowsocksR 以及 VMess 协议。

目前 Quantumult 已经被 Apple 根据政府要求从中国大陆区的 App Store 移除，之前在中国大陆商店购买此软件的用户将不能获得更新或重新下载。

!> 这是一个付费软件，你需要购买才能使用。

## 应用下载

以下是应用的下载地址。

- Apple iOS：[App Store](https://itunes.apple.com/us/app/quantumult/id1252015438?ls=1&mt=8)

## 配置方案

此文中讲述在 Quantumult 中配置 SSR、V2ray、分流规则订阅。

如您需要更细化的配置，如使用策略组等，请 **[点此查阅](/iOS/Quantumult_conf.md)** 相关的使用方案，否则请往下继续。

## 获取订阅

注册机场以获取节点订阅链接或者找免费的订阅链接。

这里我们推荐一个[V2ray机场，有免费V2ray和SS节点](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)，[点击注册](https://w1.v2dns.xyz/auth/register?code=cd79)，注册后在该机场用户中心拷贝订阅链接。
注册后免费获得1024M初始流量，每日[签到](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/checkin.jpg)可获得300-500M免费流量。更有不限流量节点可用。
注册登录后，用个人邀请链接 邀请新用户注册还可获得流量奖励，如果新用户成为付费用户，你还可以赚取高达20%终生佣金奖励。

机场的 **订阅链接** 非常重要，你应当把它当做密码一样妥善保管。

## 配置 Quantumult

打开 Quantumult，点击底部导航栏的「设置」进入设置页面。

![1](https://v2free.org/docs/SSPanel/iOS/images/quantumult_sub-1.jpg ':size=200')

进入「订阅」子页面并点击右上角的加号，从弹出菜单中选择第一个「服务器」。

![2](https://v2free.org/docs/SSPanel/iOS/images/quantumult_sub-2.jpg ':size=200')

在「名称」中输入本站名称并保存，随后在「链接」中粘贴上方 **[获取订阅](#获取订阅)** 中您需要使用的订阅类型并保存。

![3](https://v2free.org/docs/SSPanel/iOS/images/quantumult_sub-3.jpg ':size=600')

随后点击右上角保存，此时会自动更新获取服务器。

### 分流规则

同样在「订阅」页面，点击右上角加号，从弹出菜单中选择第二个「分流」。

在名称中输入「Hackl0us Rules」，链接中输入 [此链接网址](https://raw.githubusercontent.com/Hackl0us/Surge-Rule-Snippets/master/LAZY_RULES/Quantumult.conf) 【电脑：右键点链接->复制链接地址；手机长按链接，然后复制链接地址；或点击打开链接后从浏览器地址栏复制链接地址】。

![4](https://v2free.org/docs/SSPanel/iOS/images/quantumult_sub-4.jpg ':size=600')

随后左划「Hackl0us Rules」分流规则并点击替换。

![5](https://v2free.org/docs/SSPanel/iOS/images/quantumult_sub-5.jpg ':size=400')

## 开始使用

回到 Quantumult 主页，点击底部导航栏的 **圆 logo** 图标，选择您需要的节点，随后打开右上角开关即可。

如提示添加 VPN 配置，请点击 Allow 并验证您的 密码、Touch ID、Face ID 。

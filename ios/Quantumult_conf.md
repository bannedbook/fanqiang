# 导入 Quantumult 完整配置

## 应用概述

Quantumult 是在 iOS 平台上的客户端软件，支持 Shadowsocks、ShadowsocksR 以及 VMess 协议。

目前 Quantumult 已经被 Apple 根据政府要求从中国大陆区的 App Store 移除，之前在中国大陆商店购买此软件的用户将不能获得更新或重新下载。

!> 这是一个付费软件，你需要购买才能使用。

## 应用下载

以下是各平台该应用的下载地址。

- Apple iOS：[App Store](https://itunes.apple.com/us/app/quantumult/id1252015438?ls=1&mt=8)

## 配置方案

此文中讲述在 Quantumult 中使用策略组等更为细化的分流方案。

如您想使用更简化的配置，请 **[点此查阅](Quantumult_sub.md)** 相关的使用方案，否则请往下继续。

## 获取订阅

注册机场以获取节点订阅链接或者找免费的订阅链接。

这里我们推荐一个[V2ray机场，有免费V2ray和SS节点](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)，[点击注册](https://w1.v2dns.xyz/auth/register?code=cd79)，注册后在该机场用户中心拷贝订阅链接。
注册后免费获得1024M初始流量，每日[签到](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/checkin.jpg)可获得300-500M免费流量。更有不限流量节点可用。
注册登录后，用个人邀请链接 邀请新用户注册还可获得流量奖励，如果新用户成为付费用户，你还可以赚取高达20%终生佣金奖励。

机场的 **订阅链接** 非常重要，你应当把它当做密码一样妥善保管。

## 配置 Quantumult

在机场的 **个人中心** 页面中一般提供一键导入的方法，您可以在用户中心点击如下两个按钮一键导入。

![1](https://v2free.org/docs/SSPanel/iOS/images/quantumult_conf-1.jpg)

**下方是手动配置方法：**

打开 Quantumult，点击底部导航栏的「设置」进入设置页面。

划动到下方，点击 **下载配置文件**，在弹出的输入框中粘贴上方 **[获取订阅](#获取订阅)** 中您需要使用的订阅类型并保存。

![2](https://v2free.org/docs/SSPanel/iOS/images/quantumult_conf-2.jpg)

待配置文件下载完成后点击右上角的保存，随后将可看到已导入的节点以及分流规则。

进入「运行模式」将其切换为 **自动分流**。

![3](https://v2free.org/docs/SSPanel/iOS/images/quantumult_conf-3.jpg)

再进入「策略」点击 **主页显示** 将其更改为 **自定义策略**。

![4](https://v2free.org/docs/SSPanel/iOS/images/quantumult_conf-4.jpg)

## 开始使用

回到 Quantumult 主页，点击底部导航栏的 **圆 logo** 图标，选择 🏃 Auto 或您中意的节点，随后打开右上角开关即可。

如提示添加 VPN 配置，请点击 Allow 并验证您的 密码、Touch ID、Face ID 。

# 安卓手机 V2rayNG 翻墙教程

## 应用概述

V2rayNG 是在 Android 平台上的客户端软件，支持 VMess 及 Shadowsocks 协议。

## 应用下载

[V2rayNG 官方下载](https://github.com/2dust/v2rayNG/releases)

## 通过订阅链接将V2ray节点导入V2rayNG

### 获取订阅

从[V2ray机场](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)获取[V2ray订阅链接](https://w1.v2dns.xyz/auth/register?code=cd79)，或者也可以找[免费订阅链接](https://w1.v2dns.xyz/auth/register?code=cd79)

机场 **订阅链接** 非常重要，你应当把它当做密码一样妥善保管。

### 配置 V2rayNG

打开 V2rayNG 点击左上角的菜单图标打开侧边栏，随后点击 **订阅设置**。

![1](https://i.loli.net/2019/02/13/5c62fd8327c0e.png)

点击右上角的加号按钮，在新页面的备注中填写本站名称，地址输入框中粘贴上方 **获取订阅** 中的订阅链接并点击右上角的 **√**。

![2](https://i.loli.net/2019/02/13/5c62fef253cd4.jpg)

再次从侧边栏进入 **设置** 页面，点击 **路由模式** 将其更改为 **绕过局域网及大陆地址**。

![3](https://i.loli.net/2019/02/13/5c62ffab506fb.jpeg)

随后从侧边栏回到 **配置文件** 页面，点击右上角的省略号图标选择更新订阅、测试全部配置真连接（建议每日更新一次订阅）。

![4](https://i.loli.net/2019/02/13/5c630072445ec.jpeg)

### 开始使用

点击选择您中意的节点，点击右下角的按钮即可连接。

如操作系统提示添加 VPN 配置，请点击 运行 并验证您的 密码、指纹等。

## 通过vmess url导入V2rayNG

此方法更新节点不太方便，推荐首选上文介绍的订阅链接导入方法。

从[V2free用户中心](https://v2free.org/user ':ignore')，拷贝全部节点URL后(不是订阅链接)，在V2RayNG主界面点右上角 "+" 按钮，再点：从剪贴板导入。

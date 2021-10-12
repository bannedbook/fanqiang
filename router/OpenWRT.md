# OpenWRT路由器翻墙教程

**前情：已在openwrt中安装了shadowsocksR-plus插件，现在具体设置**

**操作分为两部分：**

**一，获得SSR或V2RAY机场；**

**二，在shadowsocksR-plus插件中设置相应参数**

## 一，获得SSR或V2RAY机场节点

注册机场以获取节点订阅链接或者找免费的订阅链接。

这里我们推荐一个[V2ray机场，有免费V2ray和SS节点](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)，[点击注册](https://w1.v2dns.xyz/auth/register?code=cd79)，注册后在该机场用户中心拷贝订阅链接。
注册后免费获得1024M初始流量，每日[签到](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/checkin.jpg)可获得300-500M免费流量。更有不限流量节点可用。
注册登录后，用个人邀请链接 邀请新用户注册还可获得流量奖励，如果新用户成为付费用户，你还可以赚取高达20%终生佣金奖励。

机场的 **订阅链接** 非常重要，你应当把它当做密码一样妥善保管。

复制上面的订阅链接。

## 二，在shadowsocksR-plus插件中设置相应参数

1.登录进路由器，点击服务器节点页面。

![](https://v2free.org/docs/SSPanel/Router/OpenWRT2.jpg)

2.在订阅URL栏目粘贴进之前在V2free复制的订阅链接。（如图所示），并点击 **“保存 & 应用”**。大约几分钟后刷新页面就可以看到节点列表了（**需联网操作**）。

![](https://v2free.org/docs/SSPanel/Router/OpenWRT3.jpg)

3.出现节点列表后，返回 “**客户端**” 页面选择任意一个**可用**节点 并 “保存&应用”。

![](https://v2free.org/docs/SSPanel/Router/OpenWRT4.jpg)

当页面出现绿色的 “ShadowsocksR Plus+ 运行中”字样即表示设置成功。

在运行模式那里，选择 绕过中国大陆IP模式或者GFW列表模式，按需设置即可。

至此就可以实现路由端科学上网设置。
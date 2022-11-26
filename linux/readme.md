# Linux 翻墙教程

## 安装 clash for linux

下载 clash：[https://github.com/Dreamacro/clash/releases](https://github.com/Dreamacro/clash/releases)

![](https://v2free.org/docs/SSPanel/linux/clash_files/1946477.png)

根据你的Linux版本选择相应的下载，我这里直接通过 wget 下载 clash-linux-amd64 版本。

    wget -O clash.gz https://github.com/Dreamacro/clash/releases/download/v1.10.6/clash-linux-amd64-v1.10.6.gz

解压到当前文件夹

    gzip -f clash.gz -d 

授权可执行权限

    chmod +x clash

初始化执行 clash

    ./clash 

初始化执行 clash 会默认在 `~/.config/clash/` 目录下生成配置文件和全球IP地址库：`config.yaml` 和 `Country.mmdb`

然后按 Ctrl+c 退出clash程序。

## 下载 clash 配置文件

登录您的机场获取Clash订阅配置文件，推荐免费公益机场-V2free机场：

点击注册链接：<a href="https://go.runba.cyou/auth/register?code=cd79" target="_blank">go.runba.cyou</a>，注册后在该网站用户中心获取clash订阅链接.

教育网的网友如果打不开上面的链接，请使用这个链接：
https://cdn.runba.cyou/auth/register?code=cd79

注册后免费获得1024M初始流量，每日[签到](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/checkin.jpg)可获得300-500M免费流量。
注册登录后，用个人邀请链接 邀请新用户注册还可获得流量奖励，如果新用户成为付费用户，你还可以赚取高达20%终生佣金奖励。

**安全提示： V2free机场的Clash订阅** 已默认实现国内外流量分流，一般国内网站不走代理。

用wget下载clash配置文件，替换默认的配置文件，下面的wget命令后面的 `你的Clash订阅链接网址`  ，用上面的实际的clash订阅链接替换。

    wget -U "Mozilla/6.0" -O ~/.config/clash/config.yaml  你的Clash订阅链接网址

然后，再次启动clash

    ./clash

## 配置Linux 或者 浏览器使用Clash代理，以 ubunutu 为例

同時启用 HTTP 代理和 Socks5 代理。

clash 默认 http 端口默认监听 7890 , socks5 端口默认监听 7891

打开 设置 -> 网络 -> 网络代理

配置 HTTP 代理和 socket 代理 分别为上面的端口号

![](https://v2free.org/docs/SSPanel/linux/clash_files/574938345.png)

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
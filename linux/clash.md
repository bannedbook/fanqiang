#Clash for linux 翻墙教程

##安装 clash for linux

下载最新版本 clash：[https://github.com/Dreamacro/clash/releases](https://github.com/Dreamacro/clash/releases)

![](https://v2free.org/docs/SSPanel/linux/clash_files/1946477.png)

根据你的Linux版本选择相应的下载，我这里直接通过 wget 下载 clash-linux-amd64 版本。**建议不要使用1.9.0及更新版本。**

    wget -O clash.gz https://github.com/Dreamacro/clash/releases/download/v1.7.1/clash-linux-amd64-v1.7.1.gz

解压到当前文件夹

    gzip -f clash.gz -d 

授权可执行权限

    chmod +x clash

初始化执行 clash

    ./clash 

初始化执行 clash 会默认在 `~/.config/clash/` 目录下生成配置文件和全球IP地址库：`config.yaml` 和 `Country.mmdb`

然后按 Ctrl+c 退出clash程序。

##下载 clash 配置文件

登录您的机场获取Clash订阅配置文件，推荐免费公益机场-V2free机场：

[![免费公益机场-不限流量](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/freenode.jpg)](https://w1.v2dns.xyz/auth/register?code=cd79)

点击注册链接：<a href="https://w1.v2dns.xyz/auth/register?code=cd79" target="_blank">w1.v2dns.xyz</a>，注册后在该网站用户中心获取clash订阅链接.

教育网的网友如果打不开上面的链接，请使用这个链接：
https://cdn.v2free.net/auth/register?code=cd79

注册后免费获得1024M初始流量，每日[签到](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/checkin.jpg)可获得300-500M免费流量。
注册登录后，用个人邀请链接 邀请新用户注册还可获得流量奖励，如果新用户成为付费用户，你还可以赚取高达20%终生佣金奖励。

!> 安全提示： **V2free机场的Clash订阅** 已默认实现国内外流量分流，一般国内网站不走代理。

用wget下载clash配置文件，替换默认的配置文件，下面的wget命令后面的 `你的Clash订阅链接网址`  ，用上面的实际的clash订阅链接替换。

    wget -U "Mozilla/6.0" -O ~/.config/clash/config.yaml  你的Clash订阅链接网址

然后，再次启动clash

    ./clash

##配置Linux 或者 浏览器使用Clash代理，以 ubunutu 为例

同時启用 HTTP 代理和 Socks5 代理。

clash 默认 http 端口默认监听 7890 , socks5 端口默认监听 7891

打开 设置 -> 网络 -> 网络代理

配置 HTTP 代理和 socket 代理 分别为上面的端口号

![](https://v2free.org/docs/SSPanel/linux/clash_files/574938345.png)
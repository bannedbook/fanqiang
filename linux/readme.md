# Linux 翻墙教程

## 创建并进入程序目录

打开linux命令行，依次执行下列命令

```
mkdir  ~/.config/
mkdir  ~/.config/mihomo/
cd     ~/.config/mihomo/
```

## 下载最新版本 clash

https://github.com/MetaCubeX/mihomo/releases

![39509-crg2bid6yj.png](https://v2free.org/docs/SSPanel/linux/clash_files/1946477.png "39509-crg2bid6yj.png")

根据你的Linux版本选择相应的下载，一般下载`linux-amd64`版本即可。如果 wget 下载不了的话，就用浏览器手工下载也行

    wget -O clash-linux.gz https://github.com/MetaCubeX/mihomo/releases/download/v1.17.0/mihomo-linux-amd64-v1.17.0.gz
	
如果用浏览器下载，下载后移动文件到  `~/.config/mihomo/` 并改名为 `clash-linux.gz`

解压到当前文件夹

    gzip -f clash-linux.gz -d 

授权可执行权限

    chmod +x clash-linux

初始化执行 clash

    ./clash-linux 
	
等几分钟，然后按 Ctrl+c 退出clash程序。

初始化执行 clash 会默认在 `~/.config/mihomo/` 目录下生成配置文件和全球IP地址库：`config.yaml` 、`Country.mmdb`、`GeoSite.dat`

    ls -rtl ~/.config/mihomo/

## 下载`Country.mmdb`

如果这一步`Country.mmdb`不能自动完成下载，请手工下载：

https://github.com/Dreamacro/maxmind-geoip/releases/latest/download/Country.mmdb

下载后放到 `~/.config/mihomo/` 目录。

## 下载`GeoSite.dat`

如果`GeoSite.dat`不能自动完成下载，请手工下载[GeoSite.dat](https://github.com/ewigl/mihomo/raw/master/GeoSite.dat) 后放到 `~/.config/mihomo/` 目录.

## 下载 clash 配置文件(更新订阅更新节点)

注册机场以获取节点订阅链接或者找免费的订阅链接。

这里我们推荐一个[V2ray机场，有免费V2ray节点](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)，[点击注册](https://w1.v2ai.top/auth/register?code=cd79)，注册后在该机场用户中心拷贝clash订阅链接。

教育网的网友如果打不开上面的链接，请使用这个链接：
https://cdn.v2ai.top/auth/register?code=cd79

注册后免费获得1024M初始流量，每日[签到](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/checkin.jpg)可获得300-500M免费流量。
注册登录后，用个人邀请链接 邀请新用户注册还可获得流量奖励，如果新用户成为付费用户，你还可以赚取高达20%终生佣金奖励。

机场的 **订阅链接** 非常重要，你应当把它当做密码一样妥善保管。

用wget下载clash配置文件（**重复执行就是更新订阅更新节点**），替换默认的配置文件，下面的wget命令后面的 `你的Clash订阅链接网址`  ，用你的机场的实际的clash订阅链接替换。**当然，你也可以用浏览器打开订阅链接，下载后拷贝或移动到~/.config/mihomo/目录替换覆盖config.yaml文件。**

	wget -U "Mozilla/6.0" -O ~/.config/mihomo/config.yaml  你的Clash订阅链接网址

然后，再次启动clash

    ./clash-linux
	
提示：机场节点信息可能会不定时更新，若出现大面积节点不可用现象，或者从免费用户升级为VIP用户，请手工更新订阅更新节点。 

## 配置Linux 或者 浏览器使用Clash代理，以 ubunutu 为例

同時启用 HTTP 代理和 Socks5 代理。

clash 默认 http 和 socks5 端口都默认监听 7890

打开 设置 -> 网络 -> 网络代理

配置 HTTP 代理和 socket 代理 分别为上面的端口号(**注意：Linux命令行的程序或shell脚本不一定遵循此处代理设置，设置命令行的代理请看后文**)

![69564-fy7u3i5sqhl.png](https://v2free.org/docs/SSPanel/linux/clash_files/574938345.png "69564-fy7u3i5sqhl.png")

## Linux命令行设置代理

Linux命令行的程序或shell脚本不一定遵循上述代理设置，因此需要单独设置命令行的代理。

!> 注意，ping 不支持代理，命令行测试外网网址请使用 curl 测试。

!> clash启动已占用的终端窗口无法再输入命令，请新开一个终端窗口执行下列命令。

!> 下列命令只对当前终端窗口有效，如果希望永久性的设置代理，可以将以上命令添加到.bashrc文件中。

在Linux命令行中设置代理，可以通过设置环境变量http_proxy和https_proxy来实现：

	export http_proxy="http://127.0.0.1:7890"
	export https_proxy="http://127.0.0.1:7890"

输入 echo $http_proxy 和 echo $https_proxy 命令，然后回车查看，以确保代理已经正确设置。

如果需要取消代理，可以使用以下命令：

	unset http_proxy
	unset https_proxy

## 相关阅读
*   [V2ray机场，V2ray/SS免费翻墙节点](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)

*   [安卓翻墙软件](https://github.com/bannedbook/fanqiang/wiki/%E5%AE%89%E5%8D%93%E7%BF%BB%E5%A2%99%E8%BD%AF%E4%BB%B6)
*   [安卓翻墙APP教程](https://github.com/bannedbook/fanqiang/tree/master/android)
*   [Chrome一键翻墙包](https://github.com/bannedbook/fanqiang/wiki/Chrome%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85)
*   [EdgeGo-Edge一键翻墙包](https://github.com/bannedbook/fanqiang/tree/master/EdgeGo)
*   [火狐firefox一键翻墙包](https://github.com/bannedbook/fanqiang/wiki/%E7%81%AB%E7%8B%90firefox%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85)
*   [自建V2ray服务器翻墙简明教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md)
*   [自建Shadowsocks服务器翻墙简明教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAShadowsocks%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md)
*   [免费ss账号](https://github.com/bannedbook/fanqiang/wiki/%E5%85%8D%E8%B4%B9ss%E8%B4%A6%E5%8F%B7)
*   [v2ray免费账号](https://github.com/bannedbook/fanqiang/wiki/v2ray%E5%85%8D%E8%B4%B9%E8%B4%A6%E5%8F%B7)
*   [苹果电脑MAC翻墙](https://github.com/bannedbook/fanqiang/wiki/%E8%8B%B9%E6%9E%9C%E7%94%B5%E8%84%91MAC%E7%BF%BB%E5%A2%99)
*   [iphone翻墙](https://github.com/bannedbook/fanqiang/wiki/iphone%E7%BF%BB%E5%A2%99)
*   [TorBrowser一键翻墙包](https://github.com/bannedbook/fanqiang/wiki/TorBrowser%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85)
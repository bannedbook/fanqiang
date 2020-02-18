**2020年2月12日更新（未完成，修改中...）。**
转载修订自：<a href="https://github.com/Alvin9999/new-pac/wiki/%E8%87%AA%E5%BB%BAv2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E6%95%99%E7%A8%8B" target="_blank"> Alvin9999的自建v2ray服务器教程</a>
***

**自建v2ray教程很简单，整个教程分三步**：

第一步：购买VPS服务器 

第二步：一键部署VPS服务器

第三步：一键加速VPS服务器 

***

**【前言】**

**v2ray的优势**

v2ray支持的传输方式有很多，包括：普通TCP、HTTP伪装、WebSocket流量、普通mKCP、mKCP伪装FaceTime通话、mKCP伪装BT下载流量、mKCP伪装微信视频流量，不同的传输方式其效果会不同，有可能会遇到意想不到的效果哦！当然国内不同的地区、不同的网络环境，效果也会不同，所以具体可以自己进行测试。现在v2ray客户端也很多了，有windows、MAC、linux和安卓版。

注意：如果你选择使用 V2ray，建议关闭并删除所有的ss/ssr服务端，避开互相干扰。

***

**第一步：购买VPS服务器**

VPS服务器需要选择国外的，首选国际知名的vultr，速度不错、稳定且性价比高，按小时计费，能够随时开通和删除服务器，新服务器即是新ip。

Vultr注册地址：https://www.vultr.com/?ref=8442917-6G
（新用户通过此活动链接注册，可获赠100美元，有效期1个月，为了获得100美元赠送必须绑定信用卡或Paypal。Vultr 云主机在全球15个数据中心可选，KVM框架，最低2.5美元/月。）如果以后这个vultr注册地址被墙了，那么就用<a href="https://github.com/bannedbook/fanqiang/wiki/Chrome%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85">翻墙软件</a>打开，或者用[ss/ssr免费账号](https://github.com/bannedbook/fanqiang/wiki/%E5%85%8D%E8%B4%B9ss%E8%B4%A6%E5%8F%B7)

<a href="https://www.vultr.com/?ref=8442917-6G"><img src="https://www.vultr.com/media/banner_2.png" width="468" height="60"></a>

虽然是英文界面，但是现在的浏览器都有网页翻译功能，鼠标点击右键，选择网页翻译即可翻译成中文。

注册并邮件激活账号，充值后即可购买服务器。充值方式是支付宝或paypal，使用paypal有银行卡（包括信用卡）即可。paypal注册地址：https://www.paypal.com （paypal是国际知名的第三方支付服务商，注册一下账号，绑定银行卡即可购买国外商品）

2.5美元/月的服务器配置信息：单核   512M内存  20G SSD硬盘   带宽峰值100M    500G流量/月   (**仅提供ipv6 ip**)

3.5美元/月的服务器配置信息：单核   512M内存  20G SSD硬盘   带宽峰值100M    500G流量/月    (**推荐**) 

5美元/月的服务器配置信息：  单核   1G内存    25G SSD硬盘   带宽峰值100M    1000G流量/月   (**推荐**) 
 
10美元/月的服务器配置信息： 单核   2G内存    40G SSD硬盘   带宽峰值100M    2000G流量/月  

20美元/月的服务器配置信息： 2cpu   4G内存   60G SSD硬盘    带宽峰值100M    3000G流量/月  

40美元/月的服务器配置信息： 4cpu   8G内存   100G SSD硬盘   带宽峰值100M    4000G流量/月  

**注意：2.5美元套餐只提供ipv6，如果你用不了ipv6，那么你可以买3.5美元的套餐。另外，并非所有地区都有3.5美元的套餐，需要自己去看。由于资源的短缺，有的地区有时候有3.5美元的套餐，有时候没有。**

vultr实际上是折算成小时来计费的，比如服务器是5美元1个月，那么每小时收费为5/30/24=0.0069美元 会自动从账号中扣费，只要保证账号有钱即可。如果你部署的服务器实测后速度不理想，你可以把它删掉（destroy），重新换个地区的服务器来部署，方便且实用。因为新的服务器就是新的ip，所以当ip被墙时这个方法很有用。当ip被墙时，为了保证新开的服务器ip和原先的ip不一样，先开新服务器，开好后再删除旧服务器即可。在账号的Billing选项里可以看到账户余额。

**账号充值如图**：

![](https://raw.githubusercontent.com/Alvin9999/pac2/master/pp100.png)

![](https://raw.githubusercontent.com/Alvin9999/pac2/master/pp101.png)

**开通服务器步骤如图**

![](https://raw.githubusercontent.com/Alvin9999/pac2/master/vultr/vultr1.PNG)

![](https://raw.githubusercontent.com/Alvin9999/pac2/master/vultr/vultr2.PNG)

**vps操作系统选择**

vps操作系统推荐选择Debian 10 x 64，因为这里以Debian 10为例讲解，其实V2ray 在 Linux 2.6.23 及之后版本（x86 / amd64 / arm / arm64 / mips64 / mips），包括但不限于 Debian 7 / 8、Ubuntu 12.04 / 14.04 及后续版本、CentOS 6 / 7、Arch Linux 下都可以安装。

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/vps/vultr-debian.jpg)


![](https://raw.githubusercontent.com/Alvin9999/pac2/master/vultr/vultr5.PNG)

![](https://raw.githubusercontent.com/Alvin9999/pac2/master/vultr/vultr6.PNG)

**开通服务器时，当出现了ip，不要立马去ping或者用xshell去连接，再等5分钟之后，有个系统安装启动的时间。完成购买后，找到系统的密码记下来，部署服务器时需要用到。vps系统的密码获取方法如下图：**

![](https://raw.githubusercontent.com/Alvin9999/crp_up/master/pac教程05.png)

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/ss/Debian2.png)

**删掉服务器步骤如下图**：

删除服务器时，先开新的服务器后再删除旧服务器，这样可以保证新服务器的ip与旧ip不同。

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/ss/de4.PNG)

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/ss/de2.PNG)

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/ss/de5.png)

一个被墙ip的vps被删掉后，其ip并不会消失，会随机分配给下一个在这个数据中心新建服务器的人，这就是为什么开新服务器会有一定几率开到被墙的ip。被墙是指在国内地区无法ping通服务器，但在国外是可以ping通的，vultr是面向全球服务，如果这个被墙ip被国外的人开到了，它是可以被正常使用的，一般一段时间后就会被解封，那么这就是一个良性循环。

***

**第二步：部署VPS服务器**

购买服务器后，需要部署一下。因为你买的是虚拟东西，而且又远在国外，我们需要一个叫Xshell的软件来远程部署。Xshell windows版下载地址：

[国外云盘下载](http://45.32.39.221/xshell.exe) 

如果你是苹果电脑操作系统，更简单，无需下载xshell，系统可以直接连接VPS。打开**终端**（Terminal），输入ssh root@ip  其中“ip”替换成你VPS的ip, 按回车键，然后复制粘贴密码，按回车键即可登录。粘贴密码时有可能不显示密码，但不影响， [参考设置方法](http://www.cnblogs.com/ghj1976/archive/2013/04/19/3030159.html)  如果不能用MAC自带的终端连接的话，直接网上搜“MAC连接SSH的软件”，有很多，然后通过软件来连接vps服务器就行，具体操作方式参考windows xshell。

***

部署教程：

下载windows xshell软件并安装后，打开软件

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/xshell11.png)

选择文件，新建

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/xshell12.png)

随便取个名字，然后把你的服务器ip填上

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/xshell13.png)

连接国外ip即服务器时，软件会先后提醒你输入用户名和密码，用户名默认都是root，密码是你购买的服务器系统的密码。

在ssh连接服务器之前我们检查一下，看看vps服务器是否已经成功启动，看下图：
![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/vps/vultr-console.jpg)

点服务器右侧的3个点，然后点 View Console ，然后就会弹出一个浏览器窗口如左边，显示：......Login: ,这就说明启动成功了。如果是没有这个Login:的提示，就说明还没有启动成功，这时候肯定是连不上的。

### 确保服务器启动成功的前提下，如果xshell连不上服务器，没有弹出让你输入用户名和密码的输入框，表明你开到的ip是一个被墙的ip，遇到这种情况，重新开新的服务器，直到能用xshell连上为止，耐心点哦！如果同一个地区开了多台服务器还是不行的话，可以换其它地区。

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/xshell14.png)

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/ss/xshell2.png)

连接成功后，会出现如上图所示，之后就可以复制粘贴代码部署了。

这里我们采用v2ray官方的一键安装脚本（官方脚本更加安全可靠）：

安装脚本命令：
`bash <(curl -L -s https://install.direct/go.sh)`

> 如果提示 curl: command not found 的错误，这是你的系统精简的太干净了，curl都没有安装，所以需要安装curl。CentOS系统安装curl命令: yum install -y curl  Debian/Ubuntu系统安装wget命令:apt-get install -y curl

———————————————————代码分割线————————————————

复制上面的安装脚本命令到VPS服务器里，复制代码用鼠标右键的复制，然后在vps里面右键粘贴进去，因为ctrl+c和ctrl+v无效。接着按回车键，脚本会自动安装。

安装成功后输出提示大略如下：<br>
`PORT:33333`<br>
`UUID:b9a7e7ac-e9f2-4ac2-xxxx-xxxxxxxxxx`<br>
`Created symlink /etc/systemd/system/multi-user.target.wants/v2ray.service → /etc/systemd/system/v2ray.service.`<br>
`V2Ray v4.22.1 is installed.`<br>

### 注意：以上直接下载并替换config文件的方法只适合v2ray官方客户端，像第三方开发的客户端，比如v2rayN、v2rayX是不适合用直接替换config文件的方法。如果用第三方开发的客户端的话，可以按照要求填写账号信息，如果部署的账号信息没有额外ID，在第三方客户端的额外ID那一栏一般是可以随便填写一个数字，比如1～10选1个。参考教程：[v2ray各平台图文使用教程](https://github.com/Alvin9999/new-pac/wiki/v2ray%E5%90%84%E5%B9%B3%E5%8F%B0%E5%9B%BE%E6%96%87%E4%BD%BF%E7%94%A8%E6%95%99%E7%A8%8B)


***

**第三步：Google BBR 一键加速VPS服务器**
分别执行以下2个命令即可：
`wget https://raw.githubusercontent.com/bannedbook/fanqiang/master/hosts/temp/sysctl.conf  -O -> /etc/sysctl.conf`<br>
`sysctl -p`<br>
执行成功后大致会输出：
`#sysctl -p `<br>
`fs.file-max = 51200 `<br>
`net.ipv4.conf.lo.accept_redirects = 0 `<br>
`net.ipv4.conf.all.accept_redirects = 0 `<br>
`net.ipv4.conf.default.accept_redirects = 0 `<br>
`net.ipv4.ip_local_port_range = 10000 65000 `<br>
`net.ipv4.tcp_fin_timeout = 15 `<br>
`net.ipv4.tcp_fastopen = 3 `<br>
`net.ipv4.tcp_keepalive_time = 1200 `<br>
`net.ipv4.tcp_rmem = 32768 436600 873200 `<br>
`net.ipv4.tcp_syncookies = 1 `<br>
`net.ipv4.tcp_synack_retries = 2 `<br>
`net.ipv4.tcp_syn_retries = 2 `<br>
`net.ipv4.tcp_timestamps = 0 `<br>
`net.ipv4.tcp_max_tw_buckets = 9000 `<br>
`net.ipv4.tcp_max_syn_backlog = 65536 `<br>
`net.ipv4.tcp_mem = 94500000 91500000 92700000 `<br>
`net.ipv4.tcp_max_orphans = 3276800 `<br>
`net.ipv4.tcp_mtu_probing = 1 `<br>
`net.ipv4.tcp_wmem = 8192 436600 873200 `<br>
`net.core.netdev_max_backlog = 250000 `<br>
`net.core.somaxconn = 32768 `<br>
`net.core.wmem_default = 8388608 `<br>
`net.core.rmem_default = 8388608 `<br>
`net.core.rmem_max = 67108864 `<br>
`net.core.wmem_max = 67108864 `<br>
`net.core.default_qdisc = fq `<br>
`net.ipv4.tcp_congestion_control = bbr `<br>
***

### 需要注意的是：不管是重启服务器，还是以后想修改之前vps里面的v2ray配置信息，当你重启好服务器或者修改好了v2ray配置信息后，都需要启动v2ray服务端。方式是：输入v2ray，选择1，然后选择1（启动服务）。

***

【v2ray客户端下载】

**2019年12月11日更新。**

[v2ray各平台客户端下载地址](https://github.com/v2ray/v2ray-core/releases)

[v2ray iOS客户端下载地址](https://itunes.apple.com/cn/app/kitsunebi/id1275446921?mt=8)

[v2ray 安卓客户端下载地址](https://play.google.com/store/apps/details?id=com.github.dawndiy.bifrostv) [v2rayNG下载地址](https://github.com/2dust/v2rayNG/releases)

以v2ray windows版为例：

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/ss/Debian17.png)

下载windows版客户端后解压出来，然后替换config.json配置文件。

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/ss/Debian18.png)

运行上图中的v2ray.exe启动软件，浏览器代理设置成Socks(5) 127.0.0.1 和1080 即可通过v2ray代理上网。

谷歌浏览器chrome可配合switchyomega插件来使用，下载插件：[switchyomega](https://github.com/atrandys/trojan/releases/download/1.0.0/SwitchyOmega_Chromium.crx)

安装插件，打开chrome，打开扩展程序，将下载的插件拖动到扩展程序页面，添加到扩展。
![20181116000534](https://user-images.githubusercontent.com/12132898/70548725-0461d000-1bae-11ea-9d1e-4577e36ac46e.png)

完成添加，会跳转到switchyomega页面，点跳过教程，然后点击proxy，如图填写，最后点击应用选项。
![20181116001438](https://user-images.githubusercontent.com/12132898/70548727-04fa6680-1bae-11ea-99da-568af4fd6f5f.png)

### 注意：以上直接下载并替换config文件的方法只适合v2ray官方客户端，像第三方开发的客户端，比如v2rayN、v2rayX是不适合用直接替换config文件的方法。如果用第三方开发的客户端的话，可以按照要求填写账号信息，参考教程：[v2ray各平台图文使用教程](https://github.com/Alvin9999/new-pac/wiki/v2ray%E5%90%84%E5%B9%B3%E5%8F%B0%E5%9B%BE%E6%96%87%E4%BD%BF%E7%94%A8%E6%95%99%E7%A8%8B)

***

**常见问题参考解决方法**：

1、账号无法使用，可能原因一：**客户端与服务端的设备系统时间相差过大。**

当vps服务器与本地设备系统时间相差过大，会导致客户端无法与服务端建立链接。请修改服务器时区，再手动修改服务器系统时间（注意也要校准自己本地设备时间）！

先修改vps的时区为中国上海时区：\cp -f /usr/share/zoneinfo/Asia/Shanghai /etc/localtime 

再手动修改vps系统时间命令的格式为（数字改为和自己电脑时间一致，误差30秒以内）：date -s "2018-11-02 19:14:00"   

修改后再输入date命令检查下。

2、账号无法使用，可能原因二：**Windows 防火墙、杀毒软件阻挡代理软件。**

如果以上问题都已排查，可以关闭 Windows 自带的防火墙、杀毒软件再尝试。

3、高阶篇

当封锁特别厉害的时候，常规的v2ray配置可能已经无法满足需求，这个时候我们可以尝试下ws+tls的方式，甚至搭建好后还可以套CDN，套CDN不是一个必须的步骤，但套CDN可以有效保护IP，甚至被墙的ip也能复活。套CDN的方法可以自行网络搜索。提前准备好域名，并将域名指定vps的ip，然后根据脚本来搭建就好了。

支持系统Debian 8+ / Ubuntu 16.04+ / Centos7，一键部署Nginx+ws+tls脚本(2020.2.8 )：

***

bash <(curl -L -s https://raw.githubusercontent.com/wulabing/V2Ray_ws-tls_bash_onekey/master/install.sh) | tee v2ray_ins.log

***

![v2ray](https://user-images.githubusercontent.com/12132898/74119025-c22e2c80-4bf8-11ea-81fe-614133dcc713.PNG)

注意：ws+tls和http/2只能安装1个，不能同时安装。比如ws+tls安装后不好用，那么你可以卸载后安装http/2。

启动 V2ray：systemctl start v2ray

启动 Nginx：systemctl start nginx

安装完成后会得到v2ray账号信息，然后把账号信息填写到客户端里面，推荐用v2rayN客户端，参考[v2ray各平台图文使用教程
](https://github.com/Alvin9999/new-pac/wiki/v2ray%E5%90%84%E5%B9%B3%E5%8F%B0%E5%9B%BE%E6%96%87%E4%BD%BF%E7%94%A8%E6%95%99%E7%A8%8B)

浏览器代理设置成Socks(5) 127.0.0.1 和1080

***

有问题可以发邮件至海外邮箱kebi2014@gmail.com

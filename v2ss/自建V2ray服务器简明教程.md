<h1>自建V2ray服务器简明教程</h1>

转载修订自：<a href="https://github.com/Alvin9999/new-pac/wiki/%E8%87%AA%E5%BB%BAv2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E6%95%99%E7%A8%8B" target="_blank"> Alvin9999的自建v2ray服务器教程</a>，主要修改部分：<br>
1、改用V2ray官方一键安装脚本<br>
2、简化网络加速方案<br>
3、SSH客户端改为使用Git for Windows。

***

**自建v2ray教程很简单，整个教程分简单几步**：

购买VPS服务器、一键加速VPS服务器、安装V2ray服务端

虽然很简单，但是如果你懒得折腾，那就用我们提供的免费翻墙软件吧：<br>
<a href="https://github.com/bannedbook/fanqiang/wiki/%E5%AE%89%E5%8D%93%E7%BF%BB%E5%A2%99%E8%BD%AF%E4%BB%B6">安卓手机翻墙</a>、<a href="/bannedbook/fanqiang/wiki/Chrome%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" class="wiki-page-link">Chrome一键翻墙包</a>、<a href="/bannedbook/fanqiang/wiki/%E7%81%AB%E7%8B%90firefox%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" class="wiki-page-link">火狐firefox一键翻墙包</a>、<a href="/bannedbook/xxjw" class="wiki-page-link">XX-Net禁闻版</a> <br>

<b>或者也可以购买现成的搬瓦工翻墙服务(跟本库无关哦)：</b><br>
<a href="https://github.com/killgcd/justmysocks/blob/master/README.md"><img src="https://raw.githubusercontent.com/killgcd/justmysocks/master/images/bwgss.jpg" alt="搬瓦工翻墙 Just My Socks"></a>

***

**【前言】**

**v2ray的优势**

v2ray支持的传输方式有很多，包括：普通TCP、HTTP伪装、WebSocket流量、普通mKCP、mKCP伪装FaceTime通话、mKCP伪装BT下载流量、mKCP伪装微信视频流量，不同的传输方式其效果会不同，有可能会遇到意想不到的效果哦！当然国内不同的地区、不同的网络环境，效果也会不同，所以具体可以自己进行测试。现在v2ray客户端也很多了，有windows、MAC、linux和安卓版、IOS。

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

服务器位置首选美国西海岸的 Los Angeles、或者亚洲的东京、新加坡，当然其它国家地区都可以尝试。

![](https://raw.githubusercontent.com/Alvin9999/pac2/master/vultr/vultr2.PNG)

**vps操作系统选择**

**vps操作系统这里选择Debian 10 x 64**

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/vps/vultr-debian.jpg)


![](https://raw.githubusercontent.com/Alvin9999/pac2/master/vultr/vultr5.PNG)

![](https://raw.githubusercontent.com/Alvin9999/pac2/master/vultr/vultr6.PNG)

**开通服务器时，当出现了ip，不要立马去ping或者用xshell去连接，再等5分钟之后，有个系统安装启动的时间。完成购买后，找到系统的密码记下来，部署服务器时需要用到。vps系统的密码获取方法如下图：**

![](https://raw.githubusercontent.com/Alvin9999/crp_up/master/pac教程05.png)

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/ss/Debian2.png)

**如果需要删掉服务器时，参考下图**：

删除服务器时，先开新的服务器后再删除旧服务器，这样可以保证新服务器的ip与旧ip不同。

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/ss/de4.PNG)

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/ss/de2.PNG)

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/ss/de5.png)

一个被墙ip的vps被删掉后，其ip并不会消失，会随机分配给下一个在这个数据中心新建服务器的人，这就是为什么开新服务器会有一定几率开到被墙的ip。被墙是指在国内地区无法ping通服务器，但在国外是可以ping通的，vultr是面向全球服务，如果这个被墙ip被国外的人开到了，它是可以被正常使用的，一般一段时间后就会被解封，那么这就是一个良性循环。

***

**第二步：SSH连接服务器**

购买服务器后，需要SSH连接服务器来执行几个简单的Linux命令（很简单，照着拷贝粘贴即可）。

如果你是苹果电脑操作系统，更简单，无需下载安装任何软件，系统可以直接连接VPS。打开**终端**（Terminal），输入：ssh root@ip  其中“ip”替换成你VPS的ip, 按回车键，然后复制粘贴密码，按回车键即可登录。粘贴密码时有可能不显示密码，但不影响， [参考设置方法](http://www.cnblogs.com/ghj1976/archive/2013/04/19/3030159.html)  如果不能用MAC自带的终端连接的话，直接网上搜“MAC连接SSH的软件”，有很多，然后通过软件来连接vps服务器就行。

Windows用户推荐使用 Git for Windows 中 Git Bash 提供的 SSH 工具。Git for Windows 还提供有完整Linux/Unix 环境，可以顺便在Windows下学习Linux。</p>

<a href="https://git-for-windows.github.io/" rel="nofollow">下载Git for Windows</a>，下载安装后，可以直接<a href="https://zh.wikihow.com/%E6%89%93%E5%BC%80Windows%E7%B3%BB%E7%BB%9F%E7%9A%84%E5%91%BD%E4%BB%A4%E6%8F%90%E7%A4%BA%E7%AC%A6" rel="nofollow">打开Windows命令行</a>，然后输入：ssh root@ip 来连接服务器。 也可以使用Git for Windows的Git Bash终端窗口执行ssh命令。

在ssh连接服务器之前我们检查一下，看看vps服务器是否已经成功启动，看下图：
![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/vps/vultr-console.jpg)

在my.vultr.com主界面，点服务器右侧的3个点，然后点 View Console ，然后就会弹出一个浏览器窗口如左边，显示：......Login: ,这就说明启动成功了。如果是没有这个Login:的提示，按几下回车键，还是没有，则说明还没有启动成功，这时候肯定是连不上的，可以再稍等几分钟。在这里检查确认vps已经启动成功后，为了验证VPS没有被墙，可以在windows 命令行ping ip检查一下<br>
`ping ip`<br>
上面命令中的 ip 换成你的vps的ip地址，然后回车。如果vps已经成功启动，但是却ping不通，则说明ip被墙了，遇到这种情况，重新开新的服务器，直到能ping通为止，耐心点哦！如果同一个地区开了多台服务器还是不行的话，可以换其它地区。下图是一个ping的通的例子：
![](https://docs.microsoft.com/en-us/windows-server/identity/ad-fs/troubleshooting/media/ad-fs-tshoot-dns/dns1.png)

ping 通vps后，就可以ssh连接了。当然你也可以跳过ping的步骤，直接连接试试看。

在windows 命令窗口，或 Git for Windows 的 Git Bash 窗口，输入：
`ssh root@your-vps-ip` ，然后回车，然后输入vps的root密码，密码可以点鼠标右键复制粘贴。

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/ss/xshell2.png)

连接成功后，会出现如上图所示，之后就可以复制粘贴linux命令脚本来执行了。

***

**第三步：Google BBR 一键加速VPS服务器**

分别执行以下2个命令即可（鼠标选中高亮后，点鼠标右键复制粘贴到上图的#后面，然后回车）。<br>
命令1(比较长，有折行，请完整拷贝)：<br> `wget https://raw.githubusercontent.com/bannedbook/fanqiang/master/hosts/temp/sysctl.conf  -O -> /etc/sysctl.conf`<br>
> 如果提示 wget: command not found 的错误，这是你的系统精简的太干净了，wget都没有安装，所以需要安先装 wget:<br>
`apt-get install -y wget`

命令2：<br> `sysctl -p`<br>

执行成功后大致会输出：<br>
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

**第四步：安装V2ray服务端**


这里我们采用v2ray官方的一键安装脚本（官方脚本更加安全可靠）：

安装脚本命令：<br>
`bash <(curl -L -s https://install.direct/go.sh)`

> 如果提示 curl: command not found 的错误，这是你的系统精简的太干净了，curl都没有安装，所以需要安先装 curl:<br>
`apt-get install -y curl`

复制上面的安装脚本命令到VPS服务器里，复制代码用鼠标右键的复制，然后在vps里面右键粘贴进去，因为ctrl+c和ctrl+v无效。接着按回车键，脚本会自动安装。

安装成功后输出提示大略如下：<br>
`PORT:33333`<br>
`UUID:b9a7e7ac-e9f2-4ac2-xxxx-xxxxxxxxxx`<br>
`Created symlink /etc/systemd/system/multi-user.target.wants/v2ray.service → /etc/systemd/system/v2ray.service.`<br>
`V2Ray v4.22.1 is installed.`<br>

### 
然后我们以Windows下V2ray客户端<a href="https://github.com/2dust/v2rayN/releases/latest">v2rayN</a>为例，简单示范客户端配置如下图:

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2ray/client1.jpg)

注意：这里的端口，填写上面服务器安装完成后显示的Port:后面的数字，用户ID填写上面的UUID:后面的一串字符即可。
配置好客户端，就可以自由冲浪了！至此为止，是不是很简单，有人说V2ray配置复杂，我们怎么没觉得呢？

关于客户端的更详细帮助，请参考[v2ray各平台图文使用教程](https://github.com/Alvin9999/new-pac/wiki/v2ray%E5%90%84%E5%B9%B3%E5%8F%B0%E5%9B%BE%E6%96%87%E4%BD%BF%E7%94%A8%E6%95%99%E7%A8%8B)

***

**常见问题参考解决方法**：

1、账号无法使用，可能原因一：**客户端与服务端的设备系统时间相差过大。**

当vps服务器与本地设备系统时间相差过大，会导致客户端无法与服务端建立链接。请修改服务器时区，再手动修改服务器系统时间（注意也要校准自己本地设备时间）！

先修改vps的时区为中国上海时区：cp -f /usr/share/zoneinfo/Asia/Shanghai /etc/localtime 

再手动修改vps系统时间命令的格式为（数字改为和自己电脑时间一致，误差30秒以内）：date -s "2018-11-02 19:14:00"   

修改后再输入date命令检查下。

2、账号无法使用，可能原因二：**Windows 防火墙、杀毒软件阻挡代理软件。**

如果以上问题都已排查，可以关闭 Windows 自带的防火墙、杀毒软件再尝试。

3、高阶篇

当封锁特别厉害的时候，常规的v2ray配置可能已经无法满足需求，这个时候我们可以尝试下ws+tls的方式，甚至搭建好后还可以套CDN，套CDN不是一个必须的步骤，但套CDN可以有效保护IP，甚至被墙的ip也能复活。套CDN的方法可以自行网络搜索。提前准备好域名，并将域名指定vps的ip，然后根据脚本来搭建就好了。

***

有问题可以<a href="https://github.com/bannedbook/fanqiang/issues">发Issue</a>交流。

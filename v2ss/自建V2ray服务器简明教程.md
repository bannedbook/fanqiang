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

[购买Vultr VPS图文教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%B4%AD%E4%B9%B0Vultr%20VPS%E5%9B%BE%E6%96%87%E6%95%99%E7%A8%8B.md)

***

**第二步：SSH连接服务器**

[SSH连接VPS教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/SSH%E8%BF%9E%E6%8E%A5VPS%E6%95%99%E7%A8%8B.md)

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/ss/xshell2.png)

SSH连接VPS成功后，会出现如上图所示，之后就可以复制粘贴linux命令脚本来执行了。

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

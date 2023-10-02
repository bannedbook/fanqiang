<h1>Windows如何共享Wifi无线网卡翻墙热点给其它设备翻墙</h1>

<h2>Netch简介</h2>

Netch是一款开源的网络游戏工具，支持Socks5、55R、V2等协议，UDP NAT FullCone及指定进程加速（不需要麻烦的IP规则）。功能上和SSTAP差不多，不过听说加速效果比后者要更好，甚至堪比一些付费的加速器，当然前提需要你的线路给力，不然加速就没意义了。
<img src="https://raw.githubusercontent.com/Qiyuan-Z/blog-image/main/img/netch/windows-netch-1.jpg" alt="" />
<h2>使用方法</h2>
下载 Netch 客户端，解压后以管理员身份运行 Netch.exe。若系统提示需要安装 .NET Framework，请<a href="https://www.microsoft.com/net/download/dotnet-framework-runtime" target="_blank" rel="noopener">点此</a>访问微软官网下载安装。

<strong>Github地址：</strong><a href="https://github.com/netchx/Netch" target="_blank" rel="noopener">https://github.com/netchx/Netch</a>

<strong>下载地址：</strong><a href="https://github.com/netchx/Netch/releases" target="_blank" rel="noopener">https://github.com/netchx/Netch/releases</a>

打开程序后，选中 “订阅” &gt; “管理订阅链接”

**推荐：**

<a href="https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA"><img src="https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2free.jpg" height="300" alt="V2free翻墙-不限流量、高速稳定、性价比超强"></a>

<img src="https://raw.githubusercontent.com/Qiyuan-Z/blog-image/main/img/netch/windows-netch-2.jpg" alt="" />

粘贴服务商提供的订阅链接到左下角的链接，备注随便填写，点击添加，然后点击保存

<img src="https://raw.githubusercontent.com/Qiyuan-Z/blog-image/main/img/netch/windows-netch-3.jpg" alt="" />

也可以从剪切板添加节点

<img src="https://raw.githubusercontent.com/Qiyuan-Z/blog-image/main/img/netch/Snipaste_2019-06-24_10-48-31.png" alt="" />

选择<code>Bypass LAN and China #绕过局域网和大陆</code>，点击启动，即可

<img src="https://raw.githubusercontent.com/Qiyuan-Z/blog-image/main/img/netch/Snipaste_2020-02-16_12-45-58.jpg" alt="" />

模式中也可以选择相应的游戏进行加速，也支持给Xshell和Xftp连接国外服务器进行加速

<img src="https://raw.githubusercontent.com/Qiyuan-Z/blog-image/main/img/netch/windows-netch-7.jpg" alt="" />

如果需要加速的游戏不在列表里面，那么就选中 “模式” &gt; “创建进程模式”

<img src="https://raw.githubusercontent.com/Qiyuan-Z/blog-image/main/img/netch/windows-netch-4.jpg" alt="" />

点击 “扫描” 选取你游戏安装目录后选择确定即可添加，它将自动加载其中的exe文件，点击保存即可选择应用此模式

<img src="https://raw.githubusercontent.com/Qiyuan-Z/blog-image/main/img/netch/windows-netch-6.jpg" alt="" />

若需开启热点，给其他设备也翻墙，则需要Tap虚拟网卡的支持，自行查看自己电脑是否装有

<img src="https://raw.githubusercontent.com/Qiyuan-Z/blog-image/main/img/netch/Snipaste_2020-02-16_12-58-05.jpg" alt="" />

若未安装则去<a href="https://build.openvpn.net/downloads/releases/tap-windows-9.21.2.exe" target="_blank" rel="noopener">TAP-Windows</a>下载安装，Clash、OpenVPN等软件中也提供了安装

<img src="https://raw.githubusercontent.com/Qiyuan-Z/blog-image/main/img/netch/Snipaste_2020-02-16_12-57-35.jpg" alt="" />

确认Tap网卡ip为自动获取

<img src="https://raw.githubusercontent.com/Qiyuan-Z/blog-image/main/img/netch/Snipaste_2020-02-16_13-21-22.jpg" alt="" />

选择<code>Bypass LAN and China (TUN/TAP)</code>，点击启动

<img src="https://raw.githubusercontent.com/Qiyuan-Z/blog-image/main/img/netch/Snipaste_2020-02-16_12-52-59.jpg" alt="" />

启动热点，会发现在网络适配器中多出一个本地连接

<img src="https://raw.githubusercontent.com/Qiyuan-Z/blog-image/main/img/netch/Snipaste_2020-02-16_13-26-48.jpg" alt="" />

右键Tap网卡，属性—共享，选择Internet连接共享，添加热点所对应的本地连接

<img src="https://raw.githubusercontent.com/Qiyuan-Z/blog-image/main/img/netch/Snipaste_2020-02-16_13-28-32.jpg" alt="" />

此时，热点即可成功翻墙

<img src="https://raw.githubusercontent.com/Qiyuan-Z/blog-image/main/img/netch/TIM%E5%9B%BE%E7%89%8720200216133203.jpg" alt="" />

本文采用CC BY-NC-SA 4.0</a> 许可协议, 文章作者: Qiyuan-Z, 原文链接: <a href="https://qiyuan-z.github.io/2020/02/16/%E5%A6%82%E4%BD%95%E7%BB%99%E7%83%AD%E7%82%B9%E7%BF%BB%E5%A2%99/">https://qiyuan-z.github.io/2020/02/16/%E5%A6%82%E4%BD%95%E7%BB%99%E7%83%AD%E7%82%B9%E7%BF%BB%E5%A2%99/</a>
<h1>SSH连接VPS教程</h1>

一般自己部署翻墙服务器的话，购买VPS服务器后，需要SSH连接服务器来执行几个简单的Linux命令（很简单，照着拷贝粘贴即可）。这里我们介绍一下SSH连接VPS的方法，以Vultr VPS为例，其它VPS也大同小异。[购买Vultr VPS图文教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%B4%AD%E4%B9%B0Vultr%20VPS%E5%9B%BE%E6%96%87%E6%95%99%E7%A8%8B.md)

如果你是苹果电脑操作系统，更简单，无需下载安装任何软件，系统可以直接连接VPS。打开**终端**（Terminal），输入：ssh root@ip  其中“ip”替换成你VPS的ip, 按回车键，然后复制粘贴密码，按回车键即可登录。粘贴密码时有可能不显示密码，但不影响， [参考设置方法](http://www.cnblogs.com/ghj1976/archive/2013/04/19/3030159.html)  如果不能用MAC自带的终端连接的话，直接网上搜“MAC连接SSH的软件”，有很多，然后通过软件来连接vps服务器就行。

Windows用户推荐使用 Git for Windows 中 Git Bash 提供的 SSH 工具。Git for Windows 还提供有完整Linux/Unix 环境，可以顺便在Windows下学习Linux。</p>

<b>广告插播，如果你觉得自己折腾VPS太麻烦，可以考虑这个服务哦（非本库服务）：</b><br>
<a href="https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA"><img src="https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2free.jpg" height="300" alt="V2free翻墙-不限流量、高速稳定、性价比超强"></a>

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

本文属于bannedbook系列翻墙教程的一部分，欢迎体验我们提供的免费翻墙软件和教程：
<ul>
<li><a href="https://github.com/bannedbook/fanqiang/wiki/%E5%AE%89%E5%8D%93%E7%BF%BB%E5%A2%99%E8%BD%AF%E4%BB%B6">安卓手机翻墙</a></li>
 <li><a href="https://github.com/bannedbook/fanqiang/wiki/Chrome%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" >Chrome一键翻墙包</a></li>
 <li><a href="https://github.com/bannedbook/fanqiang/wiki/%E7%81%AB%E7%8B%90firefox%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" >火狐firefox一键翻墙包</a></li>
 <li><a href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md" >自建V2ray服务器简明教程</a></li>

版权所有，转载必须保留文章所有链接。

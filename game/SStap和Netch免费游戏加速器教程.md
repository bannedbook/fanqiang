<h1>SStap和Netch免费游戏加速器教程</h1>
<h2>首先了解一下SStap和Netch都是什么？</h2>
SStap和Netch两款游戏加速器都是Github上的开源工具，用来加速海外游戏，使用虚拟网卡技术从网卡物理层实现海外游戏的UDP连接。下面对比一下两款游戏加速器哪个更好一些。通过ss订阅链接测试两款免费游戏加速器的整体性能。
<h3>简单介绍一下SStap和Netch</h3>
<h4>SStap是谁？</h4>
SSTap 全称 SOCKSTap, 是一款利用虚拟网卡技术在网络层实现的代理工具。SSTap 能在网络层拦截所有连接并转发给 HTTP, SOCKS4/5, SHADOWSOCKS(R) 代理，而无需对被代理的应用程序做任何修改或设置。它能同时转发 TCP, UDP 数据包，非常适合于游戏玩家使用。

SStap项目方已经不再维护 SSTap 和 SocksCap64，SSTap 和 SocksCap64 只是一个加速器，旨在减少网络游戏的 ping，这不是绕过防火墙的工具，所以即使你使用这个工具，你也无法打开那些被中国屏蔽的网站，如谷歌、Youtube、Twitter 等。而且，他们是免费的！它从不提供任何代理节点。如果其他人销售代理节点并将其用作游戏加速器，这与我无关。最后，在使用本软件时，请遵守国家法律法规。用户将对因使用本软件而造成的任何损害和责任承担全部责任。
<ul>
 	<li>SSTap和SocksCap64已于2017年11月19日停止开发及维护。</li>
</ul>
SStap软件官网： <a href="https://www.sockscap64.com/" target="_blank" rel="noopener">https://www.sockscap64.com/</a>
<h4>Netch是谁？</h4>
Netch 是一款运行在 Windows 系统上的开源游戏加速工具，简单易上手。也可以用于日常的网页浏览等。支持Socks5，Shadowsocks，ShadowsocksR，特洛伊木马，VMess，VLess代理。UDP NAT FullCone，Netch是一个简单的代理客户端。

Netch是一个开源游戏网络加速器。与需要添加规则以用作黑名单代理的SSTap不同，Netch更类似于SocksCap64，后者可以扫描游戏目录以专门获取其进程名称并通过代理服务器转发其网络流量。
<h3>SStap和Netch的下载与安装</h3>
<h4>SStap下载与安装</h4>
由于作者的原因，停止了更新SSTap，但SStap所用到的修改系统路由表的方法依旧可用。技术无罪，只因太优秀，SStap已于2017年末停止了新的开发以及维护，虽然2017年就已经停止更新，但是SStap的使用功能，时至今日仍然可以满足海外游戏加速的大部分用户需求。SStap的软件官网只能缅怀一下SStap曾经的辉煌，官网已经停止SStap的下载，如果您需要下载SStap，请从万能的Github上获取下载链接。

Github上SStap的下载链接：<a href="https://github.com/solikethis/SSTap-backup">https://github.com/solikethis/SSTap-backup</a>
<ul>
 	<li><a href="https://github.com/solikethis/SSTap-backup/blob/master/SSTap-KYO-%E5%8E%BB%E5%B9%BF%E5%91%8A%E5%85%8D%E5%AE%89%E8%A3%85%E7%89%88.zip" data-pjax="#repo-content-pjax-container">SSTap-KYO-去广告免安装版.zip</a></li>
 	<li><a href="https://github.com/solikethis/SSTap-backup/blob/master/SSTap-beta-setup-1.0.9.7.exe.7z" data-pjax="#repo-content-pjax-container">SSTap-beta-setup-1.0.9.7.exe.7z</a></li>
</ul>
游戏加速器SStap推荐使用1.0.9.7版本，也可以安装KYO去广告免安装版，这是一个免安装的修改版，下载完毕后解压到sstap根目录覆盖即可。使用风险由您自行承担。

<a href="https://github.com/FQrabbit/SSTap-Rule/releases/tag/SSTap%E5%B8%B8%E7%94%A8%E7%89%88%E6%9C%AC%E5%8F%8A%E5%8E%BB%E5%B9%BF%E5%91%8A" target="_blank" rel="noopener">SSTap常用版本及去广告版本下载链接</a>

下载完毕后需要.7z解压缩软件进行解压，如果电脑中没有安装.7z解压缩软件，请先下载安装。
<h5>安装SSTap</h5>
下面进行安装<a href="https://github.com/solikethis/SSTap-backup/blob/master/SSTap-beta-setup-1.0.9.7.exe.7z" data-pjax="#repo-content-pjax-container">SSTap-beta-setup-1.0.9.7.exe</a>，下载完毕后解压缩，之后双击打开安装程序，选择简体中文后，进行安装。

<img src="https://uzbox.com/wp-content/uploads/2021/11/bb2a04cb9fc9f975fba63fa56b451789-1024x651.png"  alt="" width="731" height="465" />

在安装过程中，会默认安装一个虚拟网卡的应用程序，在网络连接中可以看到一个SSTAP1的网卡。

<img src="https://uzbox.com/wp-content/uploads/2021/11/abec123c6b422be24b5e0add30709c6d-1024x415.jpg"  alt="" width="731" height="296" />

<img src="https://uzbox.com/wp-content/uploads/2021/11/396d9870a985eec27e9d14bff08bdf1e.png"  alt="" width="753" height="249" />

安装完毕后自动打开SSTap，进行下一步配置。

<img src="https://uzbox.com/wp-content/uploads/2021/11/08e7c8e4ada83838e1a288e93d070146.png"  alt="" width="651" height="787" />
<h5>添加SSTap代理</h5>
SSTap软件页面比较简洁，下面内嵌的广告页面已经无法正常显示，代理目前为空，需要先添加代理。

<img src="https://uzbox.com/wp-content/uploads/2021/11/dac5aa5685d04960a094bfcd72d84cc8.png" alt="" width="628" height="84" />

点击+加号，进行添加代理节点，支持HTTP/SOCKS4/SOCKS5/SS/SSR几种模式。这几种模式又是什么意思呢？

代理（英语：Proxy）：也称网络代理，是一种特殊的网络服务，允许一个终端（一般为客户端）通过这个服务与另一个终端（一般为服务器）进行非直接的连接。一些网关、路由器等网络设备具备网络代理功能。一般认为代理服务有利于保障网络终端的隐私或安全，在一定程度上能够阻止网络攻击。
<ul>
 	<li>HTTP代理：主要用于访问网页，一般有内容过滤和缓存功能。端口一般为80、8080、3128等。</li>
 	<li>SOCKS代理：只是单纯传递数据包，不关心具体协议和用法，所以速度快很多。端口一般为1080。SOCKS5比SOCKS4多了验证、IPv6、UDP支持。创建与SOCKS5服务器的TCP连接后客户端需要先发送请求来确认协议版本及认证方式。</li>
 	<li>SS：Shadowsocks（简称SS）是一种基于Socks5代理方式的加密传输协议，也可以指实现这个协议的各种开发包。目前包使用Python、C、C++、C#、Go语言、Rust等编程语言开发，大部分主要实现（iOS平台的除外）采用Apache许可证、GPL、MIT许可证等多种自由软件许可协议开放源代码。Shadowsocks分为服务器端和客户端，在使用之前，需要先将服务器端程序部署到服务器上面，然后通过客户端连接并创建本地代理。</li>
 	<li>SSR：ShadowsocksR（简称SSR）是网名为breakwa11的用户发起的Shadowsocks分支，在Shadowsocks的基础上增加了一些资料混淆方式，称修复了部分安全问题并可以提高QoS优先级。</li>
</ul>
<strong>创建代理服务器，请访问：</strong><a href="https://github.com/bannedbook/fanqiang/wiki#%E8%87%AA%E5%BB%BA%E7%BF%BB%E5%A2%99%E6%9C%8D%E5%8A%A1%E5%99%A8%E6%95%99%E7%A8%8B">自建翻墙服务器教程</a>

代理节点支持手工添加，和订阅添加。

<strong>SSTap手工添加新的代理服务器：</strong>

下面添加的内容只是一个示例，你需要搭建自己的代理服务器，才可以进行添加。

<img src="https://uzbox.com/wp-content/uploads/2021/11/8ddc9b395f0d2bb3796bf3ee647b434b.png"  alt="" width="504" height="796" />

添加SS的代理服务器完毕后，勾选添加并激活使用，然后点击保存按钮。

<img src="https://uzbox.com/wp-content/uploads/2021/11/ef1a021af7b325f1585fb4bb98a4635f.png"  alt="" width="637" height="775" />

代理服务器创建完毕后，选择代理模式，之后点击连接按钮。

<img src="https://uzbox.com/wp-content/uploads/2021/11/0b4155f1c8b0f4a9a4fa839679991038.png"  alt="" width="633" height="766" />

现在已经连接成功了，打开游戏进行连接，游戏会从SSTap的虚拟网卡上进行连接。SSTap已经安装完毕了。不过1.0.9.7版本的代理规则有限，可以选择安装KYO的去广告免安装版本，KYO版本的代理规则相对要多一些，当然你也可以自己添加代理规则。
<h5>KYO的去广告免安装版本</h5>
安装KYO的去广告免安装版本，将免安装版本下载后解压缩，然后覆盖到SSTap的安装目录。然后打开KYO去广告免安装版。

<img src="https://uzbox.com/wp-content/uploads/2021/11/13a3768e74f59626f1ae9465c5c2191e.png"  alt="" width="648" height="648" />

KYO去广告免安装版，新增添了很多代理模式。下面看一下如何添加代理模式。
<h5>添加代理规则</h5>
SSTap代理规则下载链接：<a href="https://github.com/FQrabbit/SSTap-Rule">https://github.com/FQrabbit/SSTap-Rule</a>

支持更多游戏规则，让SSTap成为真正的网游加速器。

<img src="https://uzbox.com/wp-content/uploads/2021/11/b4d1a21a082132d4b52dbce99c67d130-1024x719.png"  alt="" width="731" height="513" />

最近的规则存档是2021年5月14日的，点击下载最新的规则存档。

<strong>代理规则使用方法：</strong>
<ol>
 	<li>下载规则存档的zip包后解压缩。( 规则包的更新速度较慢，见谅。)</li>
 	<li>打开你 SSTap 所在文件夹：在桌面的快捷方式：右击点击属性，点击打开文件所在的位置。</li>
 	<li>打开 SSTap-bate 文件夹下的 rules 文件夹。</li>
 	<li>打开规则存档内的 rules 文件夹，选择需要的文件，将它们复制到 SSTap rules 文件夹窗口内。你也可以全选，或者清除原来的规则文件，直接将本项目的规则拖入 SSTap rules 文件夹。</li>
 	<li>注意：解压缩之前请注意备份之前的规则文件，以防文件被覆盖造成的麻烦。</li>
</ol>
<img src="https://uzbox.com/wp-content/uploads/2021/11/ae6ca2777534c820073332ec3ca618ee-1024x456.png"  alt="" width="731" height="326" />
<h5>SSTap订阅</h5>

推荐购买机场套餐获取SS订阅链接。

**广告插入：**  
[![V2free翻墙-不限流量、高速稳定、性价比超强](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2free.jpg)](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)

<strong>如何添加SSTap订阅：</strong>

点击SSTap工具右下方的设置图标，弹出设置窗口，在SSR订阅中点击SSR订阅管理。

<img src="https://uzbox.com/wp-content/uploads/2021/11/a59e9036c0f976df8d224e1546966e71.png"  alt="" width="991" height="826" />

点击SSR订阅管理后，进入到SSR订阅管理界面，接下来添加一条新的订阅。复制上面的订阅链接，然后粘贴在URL中。

<img src="https://uzbox.com/wp-content/uploads/2021/11/b53acef2110ac0f7947351b99d3de1a6.png"  alt="" width="817" height="532" />

在SSR订阅管理中添加订阅URL，输入完毕后，点击URL后面的添加，添加成功后，点击关闭按钮。

SSR订阅管理关闭之后点击设置中的SSR订阅，点击手动更新SSR订阅。

<img src="https://uzbox.com/wp-content/uploads/2021/11/b027777c3994435a74755d4488d4b7bb.png"  alt="" width="654" height="940" />

点击代理服务器后面的闪电标志，可以测试当前服务器状态。

<img src="https://uzbox.com/wp-content/uploads/2021/11/3f6b5ad6319839ddc3dd1b5a4b13041c-1024x573.png"  alt="" width="731" height="409" />

SSTap已经设置完毕了！

代理连接后，可以在IP地址查询网站<a href="https://ip.skk.moe/" target="_blank" rel="noopener">https://ip.skk.moe/</a>中进行IP地址查询。
<h4>Netch下载与安装</h4>
Netch避免了由SSTap引起的受限NAT问题。您可以使用NATTypeTester来测试您的NAT类型。当使用SSTap加速某些P2P游戏连接或该开放NAT类型需要游戏时，您可能会遇到一些不好的情况，例如无法加入游戏。
<ul>
 	<li>不同于SSTap那样需要通过添加规则来实现黑名单代理，Netch原理更类似Sockscap64，通过扫描游戏目录获得需要代理的进程名进行代理。 也可以实现 SSTap 那样的全局 TUN/TAP 代理，和 shadowsocks-windows 那样的本地 Socks5，HTTP 和系统代理。</li>
 	<li>在日常网页浏览方面，可以进行分流设置。</li>
 	<li>支持的代理协议：Socks5 / Shadowsocks / ShadowsocksR / Trojan / Vmess / VLess</li>
 	<li>UDP NAT FullCone</li>
 	<li>指定进程加速</li>
</ul>
<h5>Netch下载</h5>
Github 项目地址：<a href="https://github.com/NetchX/Netch" target="_blank" rel="noopener">https://github.com/NetchX/Netch</a>

<a href="https://aka.ms/dotnet/5.0/windowsdesktop-runtime-win-x64.exe" rel="nofollow"><strong>第一次使用Netch请先安装 .NET 5.0 运行库</strong></a>

最新稳定版本：<a href="https://github.com/netchx/netch/releases/tag/1.9.2">Netch1.9.2</a>

预发行版本：<a href="https://github.com/netchx/netch/releases/tag/1.9.4">Netch1.9.4</a>

首先下载安装.NET 5.0运行库，如果你想安装预发行版本<a href="https://github.com/netchx/netch/releases/tag/1.9.4">Netch1.9.4</a>，需要<a href="https://aka.ms/dotnet/6.0/windowsdesktop-runtime-win-x64.exe" rel="nofollow"><strong>先安装 .NET 6.0 运行库</strong></a>。

<img src="https://uzbox.com/wp-content/uploads/2021/11/79624730e494e3cbc77462c20ab0f6b0.png"  alt="" width="960" height="687" />

.NET 5.0运行库安装完毕后，请重新启动电脑否则代理服务器模式启动时会报错。
<h5>Netch安装</h5>
电脑重新启动后，下载安装2021年10月15日发布的稳定版<a href="https://github.com/netchx/netch/releases/tag/1.9.2">Netch1.9.2</a>。目前的Netch软件为免安装版，解压缩后即可使用，如果之前没有安装虚拟网卡软件，还需要安装 <a href="https://build.openvpn.net/downloads/releases/tap-windows-9.21.2.exe" target="_blank" rel="noopener">TAP-Windows</a>软件。

<img src="https://uzbox.com/wp-content/uploads/2021/11/517d7800809df79d8b9f91f396d0df2c-1024x731.png"  alt="" width="731" height="522" />

下载完毕后，解压缩，在Netch文件夹内直接运行Netch.exe主程序文件。Netch打开后，先添加服务器。

<img src="https://uzbox.com/wp-content/uploads/2021/11/ef8020abb638907e99aed62615a861b9-1024x457.png"  alt="" width="731" height="326" />

下面以添加Shadowsocks服务器为例，点击服务器菜单，选择添加[Shadowsocks]服务器。手动输入Shadowsocks服务器配置信息。

<img src="https://uzbox.com/wp-content/uploads/2021/11/77e5e5a51a94786c390bed29dee31209.png" alt="" width="694" height="463" />

填写好代理服务器信息后，点击保存即可。

<strong>Netch添加服务器的方式主要有三种：</strong>
<ul>
 	<li>复制节点链接后从剪贴板导入。</li>
 	<li>手动填写服务器配置。</li>
 	<li>从订阅链接导入。</li>
</ul>
添加Netch订阅链接

推荐购买机场套餐获取SS订阅链接。

**广告插入：**  
[![V2free翻墙-不限流量、高速稳定、性价比超强](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2free.jpg)](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)

<img src="https://uzbox.com/wp-content/uploads/2021/11/d9c5e1db2b3e94a8472916c9fa34f05c-1024x456.png"  alt="" width="731" height="326" />

<img src="https://uzbox.com/wp-content/uploads/2021/11/ac6bb3223afe1094a2c9354e9a4791c4-1024x633.png"  alt="" width="731" height="452" />

<img src="https://uzbox.com/wp-content/uploads/2021/11/0dddd29c67f62dfa1d65d237c900cd9a-1024x637.png"  alt="" width="731" height="455" />

订阅链接添加完毕后，在订阅菜单中点击更新服务器后，订阅链接中的服务器就被添加成功了。

<img src="https://uzbox.com/wp-content/uploads/2021/11/765a6e30b19634e876909c658786fe25-1024x542.png"  alt="" width="731" height="387" />

服务器添加完毕后，下一步选择代理模式，在模式中选择Bypass LAN and China。

<img src="https://uzbox.com/wp-content/uploads/2021/11/ecb826de94a8c274bb3f3f6847397960-1024x461.png"  alt="" width="731" height="329" />

Bypass LAN and China模式是直连国内IP，代理海外IP。点击启动后，您的电脑已经可以访问国际网络了。
<p dir="auto"><strong>Netch的代理模式有两种:</strong></p>

<ul dir="auto">
 	<li>创建进程模式</li>
 	<li>创建路由表规则</li>
</ul>
在模式中你也可以自定义添加路由表的访问规则，也可以指定某一款游戏进程使用代理服务器。配置灵活方便且多样性，总有一款规则适合你。

Netch启动后，在网络连接中会多出一个aioCloud的网卡连接，这个aioCloud是Netch的虚拟网卡。电脑中所有的网络访问流量会通过这个网卡中转。

<img src="https://uzbox.com/wp-content/uploads/2021/11/799b458e24ad0bc33031f1618dee4b82.png" alt="" width="697" height="103" />
<h2>SSTap和Netch性能对比</h2>
目前来说，SStap无法解决NAT问题。如果游戏里会有提示NAT类型严格的话，说明此游戏不适用于使用SStap进行加速。也有些游戏采用P2P方式进行联机，一般为某玩家作为房间主机，其余玩家通过与房主间建立连接来进行游戏。继续使用SStap前请知悉。而<a href="https://github.com/NetchX/Netch">Netch</a>则能解决以上问题。

选择使用SStap这种基于ip地址对游戏进行代理，还是使用基于游戏进程进行代理的Netch，亦或者选择商用加速器，悉听尊便。

Netch是一款开源的游戏加速工具，不同于SSTap那样需要通过添加规则来实现黑名单代理，通过扫描游戏目录获得需要代理的进程名进行代理。与此同时Netch避免了SSTap的NAT问题，使用SSTap加速部分P2P联机，对NAT类型有要求的游戏时，可能会因为NAT类型严格遇到无法加入联机，或者其他影响游戏体验的情况。
<h3>新手入门推荐使用SSTap，进阶玩家推荐使用Netch！</h3>

<h3>常见问题</h3>

netch 报 aioDNS start fialled , 找到解决方法了，按文档里说的，设置自定义DNS就行了

本文采用 CC BY-NC-SA 4.0 许可协议。原文链接：<a target="_blank" href="https://uzbox.com/tech/sstap-netch.html">SStap和Netch哪个免费游戏加速器好？</a>
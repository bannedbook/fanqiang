<h1>Switch、 PlayStation、Xbox等游戏机翻墙教程，利用MAC电脑做旁路由加速</h1>
<h2>前言</h2>
作为一个不怎么打游戏的人，做梦也不会想到我会在 2022 年入坑一台 2017 年发布的游戏机，并死心塌地了给任地狱上供了不少钱，但是随着玩的越发深入，非国行 Switch 的许多问题也暴露出来（为什么我一定要强调非国行，难道真的有人买国行吗……），总之，本文旨在针对其中最大的痛点之一，也就是网络连接的问题提出一些解决方法，希望对各位有所帮助。

## 相关阅读

  * [PS4-PS5游戏机通过局域网翻墙，加速游戏，以及下载游戏教程](https://github.com/bannedbook/fanqiang/blob/master/game/PS4-PS5%E6%B8%B8%E6%88%8F%E6%9C%BA%E9%80%9A%E8%BF%87%E5%B1%80%E5%9F%9F%E7%BD%91%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B.md)
  * [SStap和Netch免费游戏加速器教程](https://github.com/bannedbook/fanqiang/blob/master/game/SStap%E5%92%8CNetch%E5%85%8D%E8%B4%B9%E6%B8%B8%E6%88%8F%E5%8A%A0%E9%80%9F%E5%99%A8%E6%95%99%E7%A8%8B.md)
  * [Switch、 PlayStation、Xbox等游戏机翻墙教程，利用MAC电脑做旁路由加速](https://github.com/bannedbook/fanqiang/blob/master/game/Switch%E3%80%81%20PlayStation%E3%80%81Xbox%E7%AD%89%E6%B8%B8%E6%88%8F%E6%9C%BA%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B%EF%BC%8C%E5%88%A9%E7%94%A8MAC%E7%94%B5%E8%84%91%E5%81%9A%E6%97%81%E8%B7%AF%E7%94%B1%E5%8A%A0%E9%80%9F.md)
  * [Windows如何共享Wifi无线网卡翻墙热点给其它设备翻墙](https://github.com/bannedbook/fanqiang/blob/master/game/Windows%E5%A6%82%E4%BD%95%E5%85%B1%E4%BA%ABWifi%E6%97%A0%E7%BA%BF%E7%BD%91%E5%8D%A1%E7%BF%BB%E5%A2%99%E7%83%AD%E7%82%B9%E7%BB%99%E5%85%B6%E5%AE%83%E8%AE%BE%E5%A4%87%E7%BF%BB%E5%A2%99.md)
  * [Mac电脑使用ClashX Pro作为网关旁路由给其它设备翻墙](https://github.com/bannedbook/fanqiang/blob/master/game/Mac%E7%94%B5%E8%84%91%E4%BD%BF%E7%94%A8ClashX%20Pro%E4%BD%9C%E4%B8%BA%E7%BD%91%E5%85%B3%E6%97%81%E8%B7%AF%E7%94%B1%E7%BB%99%E5%85%B6%E5%AE%83%E8%AE%BE%E5%A4%87%E7%BF%BB%E5%A2%99.md)
  * [在Mac上使用clashx pro给switch开启游戏加速](https://github.com/bannedbook/fanqiang/blob/master/game/%E5%9C%A8Mac%E4%B8%8A%E4%BD%BF%E7%94%A8clashx%20pro%E7%BB%99switch%E5%BC%80%E5%90%AF%E6%B8%B8%E6%88%8F%E5%8A%A0%E9%80%9F.md)

<h2>原因</h2>
原因其实不必多说，作为一款国行与非国行有着明显差异的主机，自然没有考虑到国内的网络环境，再加上任地狱的联机方式基本都是P2P（为了省服务器的钱真是太拼了），导致体验非常差，这种体验差主要体现在两方面：
<ul>
 	<li>游戏下载速度过慢</li>
 	<li>线上联机体验差</li>
</ul>
大部分国内玩家遇到这种问题基本都是花钱买加速器解决，但是迫于囊中羞涩，我肯定是不会买<s>买不起</s>加速器的，不过我很快找到了方法解决这个问题。
<h2>分析</h2>
想要解决这两个问题还是比较简单的，思路就是只要能让电脑的代理工具代理 Switch 的网络，那不就跟在国外用 Switch 一个体验了吗，也就是说，只要我们让担当旁路由一职就可以了。

在开始之前多说两句，线上联机体验如何，一个重要指标就是 NAT（Network address translation），Switch 设置里用等级表示，例如我家中的辣鸡移动网直连，NAT 类型显示为 C
<img src="https://kokurasona.github.io/post-images/1660210226386.jpg" alt="" />
根据说明，C 型属于能连但是体验不好的等级。一般来说，想要有良好的联机体验需要 NAT 类型为 A 型或 B 型（A 型最佳），具体可以看<a href="https://www.nintendoswitch.com.cn/support/faq/336.html">官方说明（腾讯）</a>，这也就是我们今天的目标，本文不做原理说明，只展示如何操作。

首先需要确保你的节点支持 UDP 转发，本文用 Mac 上的 ClashX Pro 进行演示，如果你是 Windows 用户，据说 <a href="https://github.com/bannedbook/fanqiang/blob/master/game/SStap%E5%92%8CNetch%E5%85%8D%E8%B4%B9%E6%B8%B8%E6%88%8F%E5%8A%A0%E9%80%9F%E5%99%A8%E6%95%99%E7%A8%8B.md">Netch</a> 是很好的选择。

**推荐：**  
[![V2free翻墙-不限流量、高速稳定、性价比超强](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2free.jpg)](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)

<h2>启用 TUN 模式下的 UDP 转发</h2>
想要提升联机体验，最关键的便是实现 UDP 转发，为此我们先启用 ClashX Pro 的增强模式（Enhanced Mode）。

<em>注：如果只是想把电脑作为旁路由，并不需要修改配置文件，这样做的具体原因会在后文说明。</em>

打开配置文件夹，添加 yaml 文件头（如果机场的给定的配置文件里有类似内容则只需确保添加 tun: 及其以下内容即可）：
<pre class=" language-yaml"><code class=" language-yaml"><span class="token key atrule">dns</span><span class="token punctuation">:</span>
  <span class="token key atrule">enable</span><span class="token punctuation">:</span> <span class="token boolean important">true</span>
  <span class="token key atrule">ipv6</span><span class="token punctuation">:</span> <span class="token boolean important">false</span>
  <span class="token key atrule">listen</span><span class="token punctuation">:</span> 0.0.0.0<span class="token punctuation">:</span><span class="token number">53</span>
  <span class="token key atrule">enhanced-mode</span><span class="token punctuation">:</span> fake<span class="token punctuation">-</span>ip
  <span class="token key atrule">nameserver</span><span class="token punctuation">:</span>
    <span class="token punctuation">-</span> 119.29.29.29
    <span class="token punctuation">-</span> 223.5.5.5
    <span class="token punctuation">-</span> 1.1.1.1
    <span class="token punctuation">-</span> tls<span class="token punctuation">:</span>//dns.rubyfish.cn<span class="token punctuation">:</span><span class="token number">853</span>
  <span class="token key atrule">fallback</span><span class="token punctuation">:</span>
    <span class="token punctuation">-</span> tls<span class="token punctuation">:</span>//1.1.1.1<span class="token punctuation">:</span><span class="token number">853</span>
    <span class="token punctuation">-</span> tls<span class="token punctuation">:</span>//1.0.0.1<span class="token punctuation">:</span><span class="token number">853</span>
    <span class="token punctuation">-</span> tls<span class="token punctuation">:</span>//dns.google<span class="token punctuation">:</span><span class="token number">853</span>

<span class="token key atrule">tun</span><span class="token punctuation">:</span>
  <span class="token key atrule">enable</span><span class="token punctuation">:</span> <span class="token boolean important">true</span>
  <span class="token key atrule">macOS-auto-route</span><span class="token punctuation">:</span> <span class="token boolean important">true</span>
  <span class="token key atrule">macOS-auto-detect-interface</span><span class="token punctuation">:</span> <span class="token boolean important">true</span>
</code></pre>
下拉到规则部分，添加一行规则：

<code>- SRC-IP-CIDR,ns-lan-ip/32,Proxy</code>

将 ns-lan-ip 部分替换为 Switch 的 IP 地址，可以在设置-互联网页面看到：
<img src="https://kokurasona.github.io/post-images/1660210692167.jpg" alt="" />
Proxy 部分则替换为对应的代理即可，保存，更新一下配置文件，此时在 Connections 下应该可以看到出现了一个 198.18.0.1 的地址，这里是因为我们使用 fake-ip 的结果，重点看 Type 一栏，显示为 TUN，这证明我们已经成功启用了 TUN 模式。
<h2>Switch 下的配置</h2>
Switch 下打开设置-互联网-互联网设置-选择对应 Wi-Fi -更改设置，将 IP 地址设置、DNS 设置两项改为手动，其中 IP 地址填 <strong>Switch 的 IP 地址</strong>，子网掩码保持默认的 255.255.255.0，网关和首选 DNS 均填<strong>电脑的 IP 地址</strong>，备选 DNS 随意填（因为用不到）。
<img src="https://kokurasona.github.io/post-images/1660210717298.png" alt="" />
如果你不知道电脑的 IP 地址，可以从 ClashX Pro 下查看
<img src="https://kokurasona.github.io/post-images/1660210749910.png" alt="" />
保存，退出，这时 Switch 应该已经自动连接 Wi-Fi 成功，如果没自动连上手动选择连接此网络。连接成功后看电脑端 ClashX Pro 的 Connections 页面，出现了 Switch 的 IP 地址，Host 部分可见 nintendo.net 字样，可见 Clash 已经成功接管了 Switch 流量，Type 一栏同样显示为 TUN，到这里代表我们应该已经成功。我们跑一下 Switch 的连接测试，此时 NAT 应该已经变为 B 以上……
<img src="https://kokurasona.github.io/post-images/1660210772488.jpg" alt="" />
嗯？怎么 NAT 从 C 变成 F 了……我们一通操作，联机体验怎么反而还下降了……
<h2>vmess 协议背大锅</h2>
这里我们的操作理论上是没有问题的，但是为什么 NAT 等级下降了？经过一番查找，答案是如果你的节点是 vmess 协议，在这个环节就会出问题，据说是 UDP 转发有问题，看上去除了 vmess 的其他几个协议（Trojan、SS、SSR）都是可以的，于是我立刻换用 SS 协议的节点，再次跑一下 Switch 连接测试：
<img src="https://kokurasona.github.io/post-images/1660210809773.jpg" alt="" />
可以看到 NAT 成功变为最高的 A 级，这也代表我们成功改善了联机体验，无论是 NAT 等级还是上传下载速度皆有很大提升。

这就是为什么在上文我有一个修改 config 文件的操作，主要是因为我的常用节点都是 vmess 协议的，同时用特定规则 SRC-IP-CIDR 实现了让 Switch 一个设备走 SSR + fake-ip（ 规则 SRC-IP-CIDR 后加 Switch IP 地址，通过这样的方式指定 Switch 设备。具体见下图，可以看到 Switch 流量走了 SRC-IP-CIDR 规则），当然如果你觉得改 config 文件太麻烦可以跳过不做，只要满足上述条件同样可以实现 NAT B+。
<img src="https://kokurasona.github.io/post-images/1660355322821.png" alt="" />

哦对了，拿 Mac 做旁路由还有一个好处，那就是性能管够，这一部分应该是不会掉链子的，如果体验不畅大概率是节点背锅，不过也有可能是无线连接不太行，可以给 Mac 接网线试试。以及说白了加速器大致也是类似的原理嘛，没必要花两份钱，这不是冤大头嘛。
<h2>另一种方法</h2>
感觉内容稍微有点少，这里补充另一种更简单的方法，也就是设置代理服务器的方法，这也是网上提到最多的方法，思路很简单，且同样可以通过 ClashX Pro 实现，但是它也是有缺点的，这种方法只能改善网络的传输速度，并不能挽救你的联机体验。

同样打开 ClashX Pro，这时打开的不是增强模式（Enhanced Mode），而是打开允许通过 LAN 连接（Allow connect from Lan），电脑端的操作就结束了，这时打开 Switch，同样进入互联网设置，将代理服务器设置改为开启，填入<strong>电脑的 IP 地址和端口</strong>。
<img src="https://kokurasona.github.io/post-images/1660210851322.jpg" alt="" />
同样，不知道电脑的 IP 地址和端口号的可以从 ClashX Pro 下查看。
<img src="https://kokurasona.github.io/post-images/1660210749910.png" alt="" />
跑一下网络测试，可以看到上传下载速度均有提升，但是 NAT 等级岿然不动。
<img src="https://kokurasona.github.io/post-images/1660210933086.jpg" alt="" />
因此这种方法只能用来提升游戏的下载速度，个人是不太推荐的，不过这个方法对节点没什么要求，节点不合要求的可以用这种方法。
<h2>总结</h2>
虽然这篇文章是 8 月 10 号写的，但是因为忘了写惯例的总结，所以这里是 11 号写的，说实话……感觉没有什么可以总结的，昨晚任地狱也是开了 Splatoon 3 的迷你直面会，还有大概两周多的时间 Splatoon 3 的前夜祭（试玩）就要开了<s>现在感觉全身有鱿鱼在爬</s>，感觉这篇文章写的时间很不错，总之希望本文能够帮到你。

<strong>2023 年 5 月 9 日更新：</strong> 最近逛论坛的时候看到有人提到优化 Splatoon 3 的一种思路，可以有效优化联机体验，操作如下：

打开配置文件夹，修改对应配置文件，在规则部分添加如下规则：
<pre class=" language-yaml"><code class=" language-yaml"><span class="token punctuation">-</span> DOMAIN<span class="token punctuation">-</span>SUFFIX<span class="token punctuation">,</span>npln.srv.nintendo.net<span class="token punctuation">,</span>DIRECT
<span class="token punctuation">-</span> DOMAIN<span class="token punctuation">-</span>SUFFIX<span class="token punctuation">,</span>n.n.srv.nintendo.net<span class="token punctuation">,</span>DIRECT
<span class="token punctuation">-</span> DOMAIN<span class="token punctuation">-</span>SUFFIX<span class="token punctuation">,</span>nintendo.net<span class="token punctuation">,</span>Proxy
</code></pre>
其实思路很简单，联网检查（nintendo.net）流量走代理，而 P2P 流量不走代理，这样可以既保证不掉线又不提高游戏延迟，确实是很巧妙的做法。而且本方法应该不需要节点的 UDP 转发，不挑节点。

最后骂一句任地狱，猜到喷 3 联机体验会差，没有想到这么差，修修你那破网吧任天堂！

参考：

<a href="https://github.com/Dreamacro/clash/issues/971">请教TUN模式下UDP转发 · Issue #971 · Dreamacro/clash</a>

<a href="https://github.com/vernesong/OpenClash/issues/1997">TUN下以及兼容模式打开UDP转发下，Switch NAT F · Issue #1997 · vernesong/OpenClash</a>

<a href="https://github.com/vernesong/OpenClash/issues/1076">PS4 怪物猎人 无法联机 · Issue #1076 · vernesong/OpenClash</a>

本文采用 CC BY-NC-SA 4.0 许可协议。原文链接：<a target="_blank" href="https://sonatta.top/post/Oa-JnB-qx/">利用 ClashX Pro，加速你的 Switch</a>
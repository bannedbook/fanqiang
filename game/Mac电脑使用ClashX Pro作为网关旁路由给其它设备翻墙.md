<h1>Mac电脑使用ClashX Pro作为网关旁路由给其它设备翻墙</h1>

<p><strong>这个教程将会指导你用 Mac 电脑使用免费的 ClashX Pro 软件作为家庭的网关，让全家设备连上 Wi-Fi 就能科学上网。</strong></p>

Windows类似方法参考: [Windows如何共享Wifi无线网卡翻墙热点给其它设备翻墙](https://github.com/bannedbook/fanqiang/blob/master/game/Windows%E5%A6%82%E4%BD%95%E5%85%B1%E4%BA%ABWifi%E6%97%A0%E7%BA%BF%E7%BD%91%E5%8D%A1%E7%BF%BB%E5%A2%99%E7%83%AD%E7%82%B9%E7%BB%99%E5%85%B6%E5%AE%83%E8%AE%BE%E5%A4%87%E7%BF%BB%E5%A2%99.md)

<p>之前有分享过 Surge 作为网关 DHCP 接管家里网络的教程，这几个月家里的设备一直这么使用非常稳定，电视手机直接科学上网也非常方便。但是：Surge 价格有点贵会劝退人，之前的教程也需要手动设置下使用 Surge 作为网关走代理（那是我设置问题）所以这次写下 ClashX Pro 的教程。</p>
<p>使用 Clash 或者 Surge 软件作为网关我再赘述下我个人觉得的优点：</p>
<ol>
<li>性能非常非常好，因为是直接使用你的电脑 CPU 加密解密，性能比软路由、或者啥啥路由器都会好很多。</li>
<li>使用非常便利，软件鼠标直接点下就能修改规则选择节点，在状态栏也能直接看到整体的网络状态。</li>
<li>直接使用你的 Mac 电脑，不用单独购买设备。</li>
<li>稳定性不错、还有就一个软件更新非常也便利。</li>
</ol>
<h2>教程</h2>
<h3>下载安装</h3>
<p>下载链接：<a href="https://install.appcenter.ms/users/clashx/apps/clashx-pro/distribution_groups/public">ClashX Pro</a></p>
<p>第一次打开会有这个提示，选择 install 安装，输入电脑密码即可。</p>
<figure><img src="https://i.loli.net/2021/03/29/Kfdgz3MwRuE6cOI.jpg" alt ></figure>
<h3>准备 Clash 的配置文件</h3>

**广告插入：**  
[![V2free翻墙-不限流量、高速稳定、性价比超强](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2free.jpg)](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)

<p>一般你订阅的服务应该会给 Clash 的<strong>订阅链接</strong>，如果没有也没关系，你可以通过第三方 <a href="https://subconverter.speedupvpn.com/" rel="nofollow">机场翻墙订阅转换器-V2ray,Clash,SSR,SS等订阅链接在线转换</a> 生成：</p>
<figure><img src="https://oss.qust.me/img/%E6%88%AA%E5%B1%8F2021-03-29%20%E4%B8%8B%E5%8D%8811.45.13.jpg" alt ></figure>
<ol>
<li>订阅链接填上你的 v2ray、ss 、trojan 的订阅（非 SSR 的订阅）</li>
<li>客户端选择 Clash</li>
<li>选择生成订阅链接就好了</li>
</ol>
<h3>添加配置</h3>
<p>打开有 ClashX Pro 后，右上角会有一只小猫咪🐱的图标。右键选择 Config 配置 — Remote 托管配置 — Manage 管理。</p>
<figure><img src="https://oss.qust.me/img/%E6%88%AA%E5%B1%8F2021-03-29%20%E4%B8%8B%E5%8D%8811.53.40.jpg" alt ></figure>
<p>然后选择 Add 添加，URL 填上你上一步生成的 Clash 订阅地址，Config name 可以给它取一个名字备注，然后选择 OK 确定。</p>
<figure><img src="https://oss.qust.me/img/%E6%88%AA%E5%B1%8F2021-03-29%20%E4%B8%8B%E5%8D%8811.56.52.jpg" alt ></figure>
<h3>打开系统代理和增强模式</h3>
<p>再次右键打开小猫咪选项，应该就能看到你的配置文件规则（你可以通过节点选择来配置默认的上网节点）。</p>
<p>我们还需打开</p>
<ul>
<li>Set as system proxy 设置为系统代理</li>
<li>Enhanced Mode 增强模式（正是这个起到了网关路由器的作用）</li>
</ul>
<figure><img src="https://oss.qust.me/img/%E6%88%AA%E5%B1%8F2021-03-30%20%E4%B8%8A%E5%8D%8812.18.50.jpg" alt ></figure>
<p>这个时候你这一台电脑应该就能科学上网了。</p>
<h3>将 Mac 设置为固定</h3>
<p><strong>强烈建议使用网线连接到电脑 ，Wi-Fi 其实也是 OK 的不过作为路由器，Wi-Fi 不会稳定。</strong></p>
<p>打开 系统偏好设置 — 网络 — 以太网：</p>
<ul>
<li>
<p>配置 IPv4 选择手动</p>
</li>
<li>
<p>IP地址填写你局域网段里的一个就行（我的路由器是 192.168.88.1 所以 IP 地址我就填 192.168.88.2，只用修改最后一位在 2-225 之间就行。如果你的路由器是 192.168.31.1 你的 IP 地址就可以填 192.168.31.2，以此类推）</p>
</li>
<li>
<p>路由器就填写你路由器后台 IP 地址 （我的路由器是 192.168.88.1，你的如果是 192.168.31.1 你就填你路由器后台地址）</p>
</li>
</ul>
<p>修改好选择应用</p>
<figure><img src="https://oss.qust.me/img/%E6%88%AA%E5%B1%8F2021-03-30%20%E4%B8%8A%E5%8D%8812.10.32.png" alt ></figure>
<h3>修改路由器的 DHCP 选项</h3>
<p>打开路由器后台，选择 内部网络（LAN）— DHCP 服务器 ，将默认网关由路由器的后台地址改为刚才你设置你 Mac 的 IP 地址</p>
<p>。（我的这里默认网关原本是 192.168.88.1 我改为了我 Mac 的 IP地址 192.168.88.2）,DNS 服务器也得同样修改为 Mac 的 IP 地址。</p>
<p>我这里的是 padavan 系统的路由器，华硕等大部分品牌路由器应该都是有修改默认网关的选项。</p>
<figure><img src="https://oss.qust.me/img/%E6%88%AA%E5%B1%8F2021-03-30%20%E4%B8%8A%E5%8D%8810.10.54.jpg" alt ></figure>
<p>然后重启你的路由器或者设备重新连接 Wi-Fi 就能让 ClashX Pro 接管网络实现科学上网了。<br>
在 Clash 的面板 连接 里你也能看到接管设备的数量状态，只是没 Surge 那么美观。</p>
<figure><img src="https://oss.qust.me/img/%E6%88%AA%E5%B1%8F2021-03-30%20%E4%B8%8A%E5%8D%8812.35.03.png" alt ></figure>
<h3>如果你的路由器真的不支持修改 DHCP 网关</h3>
<p>你可以手动修改 Wi-Fi 里想要科学上网的设备，配置 IP 地址为手动，将路由器即网关改为 Mac 的 IP 地址。（当然还是更建议你选择修改路由器 DHCP 选项会方便很多很多）</p>
<figure><img src="https://oss.qust.me/img/IMG_0975.PNG" alt ></figure>
<h2>总结</h2>
<p><strong>删除退出 ClashX Pro 后如果不能上网得恢复网络设置</strong>：https://jingyan.baidu.com/article/020278113fc2511bcc9ce5e8.html</p>
<p>ClashX Pro 完全免费这点真的太棒了，使用软件作为网关接管网络体验真的非常非常棒，建议你也试试。</p>

<figure><img src="images/mac-route-reviews.gif"></figure>

转载自：[Mac 电脑使用 ClashX Pro 作为网关旁路由](https://qust.me/post/clashxProMac/)
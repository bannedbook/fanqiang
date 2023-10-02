<h1>在Mac上使用clashx pro给switch开启游戏加速</h1>

虽然我家的WiFi裸连也能达到nat type B，可以凑合用，但我发现使用了uu加速器可以达到nat type a，所以还是想折腾。

参照<a href="https://github.com/bannedbook/fanqiang/blob/master/game/Mac%E7%94%B5%E8%84%91%E4%BD%BF%E7%94%A8ClashX%20Pro%E4%BD%9C%E4%B8%BA%E7%BD%91%E5%85%B3%E6%97%81%E8%B7%AF%E7%94%B1%E7%BB%99%E5%85%B6%E5%AE%83%E8%AE%BE%E5%A4%87%E7%BF%BB%E5%A2%99.md" target="_blank" rel="nofollow noopener">Mac 电脑使用 ClashX Pro 作为网关旁路由</a>，好像还真能把我那些用不完的节点当成加速器使用。

首先你需要：
<ol>
 	<li>一台mac电脑；</li>
 	<li>支持导入clash的节点列表（机场）</li>
 	<li>确定你的节点支持udp协议</li>
</ol>

**推荐：**  
[![V2free翻墙-不限流量、高速稳定、性价比超强](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2free.jpg)](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)

然后，具体步骤如下：

1 下载安装clash x pro（注意，一定是pro才行，clashx没有增强模式，下载链接：<a href="https://install.appcenter.ms/users/clashx/apps/clashx-pro/distribution_groups/public" target="_blank" rel="nofollow noopener">https://install.appcenter.ms/users/clashx/apps/clashx-pro/distribution_groups/public</a>）；导入节点后，开启“设为系统代理” 和 “增强模式”

<figure ><img  src="images/clashns1.jpg"    width="379" height="161"  /></figure>

## 相关阅读

  * [PS4-PS5游戏机通过局域网翻墙，加速游戏，以及下载游戏教程](https://github.com/bannedbook/fanqiang/blob/master/game/PS4-PS5%E6%B8%B8%E6%88%8F%E6%9C%BA%E9%80%9A%E8%BF%87%E5%B1%80%E5%9F%9F%E7%BD%91%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B.md)
  * [SStap和Netch免费游戏加速器教程](https://github.com/bannedbook/fanqiang/blob/master/game/SStap%E5%92%8CNetch%E5%85%8D%E8%B4%B9%E6%B8%B8%E6%88%8F%E5%8A%A0%E9%80%9F%E5%99%A8%E6%95%99%E7%A8%8B.md)
  * [Switch、 PlayStation、Xbox等游戏机翻墙教程，利用MAC电脑做旁路由加速](https://github.com/bannedbook/fanqiang/blob/master/game/Switch%E3%80%81%20PlayStation%E3%80%81Xbox%E7%AD%89%E6%B8%B8%E6%88%8F%E6%9C%BA%E7%BF%BB%E5%A2%99%E6%95%99%E7%A8%8B%EF%BC%8C%E5%88%A9%E7%94%A8MAC%E7%94%B5%E8%84%91%E5%81%9A%E6%97%81%E8%B7%AF%E7%94%B1%E5%8A%A0%E9%80%9F.md)
  * [Windows如何共享Wifi无线网卡翻墙热点给其它设备翻墙](https://github.com/bannedbook/fanqiang/blob/master/game/Windows%E5%A6%82%E4%BD%95%E5%85%B1%E4%BA%ABWifi%E6%97%A0%E7%BA%BF%E7%BD%91%E5%8D%A1%E7%BF%BB%E5%A2%99%E7%83%AD%E7%82%B9%E7%BB%99%E5%85%B6%E5%AE%83%E8%AE%BE%E5%A4%87%E7%BF%BB%E5%A2%99.md)
  * [Mac电脑使用ClashX Pro作为网关旁路由给其它设备翻墙](https://github.com/bannedbook/fanqiang/blob/master/game/Mac%E7%94%B5%E8%84%91%E4%BD%BF%E7%94%A8ClashX%20Pro%E4%BD%9C%E4%B8%BA%E7%BD%91%E5%85%B3%E6%97%81%E8%B7%AF%E7%94%B1%E7%BB%99%E5%85%B6%E5%AE%83%E8%AE%BE%E5%A4%87%E7%BF%BB%E5%A2%99.md)
  * [在Mac上使用clashx pro给switch开启游戏加速](https://github.com/bannedbook/fanqiang/blob/master/game/%E5%9C%A8Mac%E4%B8%8A%E4%BD%BF%E7%94%A8clashx%20pro%E7%BB%99switch%E5%BC%80%E5%90%AF%E6%B8%B8%E6%88%8F%E5%8A%A0%E9%80%9F.md)

2 在本机的网络设置中查看本机ip

<figure ><img  src="images/clashns2.jpg"    width="642" height="366"  /></figure>

3 在switch的网络设置中，连接上和mac相同的WiFi，然后修改网络设置

把“IP地址设置”改为“手动”，随便填一个IP地址（如果你的电脑ip是192.168.1.XX，这里需要填192.168.1.NN，即前三段一样）

把网关和首选DNS改为电脑的IP，我的是192.168.3.2

<figure ><img  src="images/clashns4.jpg"    width="1280" height="720"  /></figure>

4 点击保存，然后重新连接到网络，检测网络

<figure ><img  src="images/clashns3.jpg"    width="1280" height="720"  /></figure>

可以看到我的IP地址变成了我选择的香港节点，NAT类型也变成了A。网速还是那样没变……


本文采用知识共享署名-非商业性使用-相同方式共享 4.0 国际许可协议，原文链接：

https://www.imtrq.com/archives/2908
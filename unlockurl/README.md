# 解封禁闻直连网址

本文的目的是解封已经被封锁和无法使用的<a href="https://github.com/bannedbook/fanqiang/wiki#jwurl">禁闻直连网址</a>，包括当前尚能使用的禁闻直连网址，也可以使用这种方法来防止被封，保持一直可用。

<ol type=disc>
<li ><b>获得一个直连网址的域名</b><br/>
如果您还没有禁闻直连网址，请点<a href="https://github.com/bannedbook/fanqiang/wiki#jwurl">禁闻代理在线-镜像</a>, 然后点“禁闻代理在线-镜像1” ，即可打开一个禁闻直连网址，把这个网址拷贝下来，比如得到网址：https://ya.dnstogo.xyz/ ，然后我们把前面的https:// 和 后面的一个斜杠去掉，就得到了一个域名：ya.dnstogo.xyz<br/></li>

<li ><b>获取直连域名的真实IP地址</b><br/>点击打开 https://www.whatsmydns.net/ (如果这个网址打不开，可以自行百度搜索：online dns lookup ，找一个国外的在线DNS查询网站)，在左上角的域名输入框输入域名，然后点 右侧的 "Search" 按钮，即可得到2个ip地址：<br/>
104.27.172.153 <br/>
104.27.173.153<br/>
如下图<br/>
<img src="https://raw.githubusercontent.com/kgfw/fg/master/hosts/findip.jpg"/>

</li>


<li ><b>修改hosts文件</b><br/>任选其中一个ip地址（另一个备用），在自己的hosts文件中添加一行:<br/>
104.27.172.153   ya.dnstogo.xyz<br/>
如何修改hosts文件，请参考：<strong><a href="https://github.com/bannedbook/fanqiang/wiki/hosts%E7%BF%BB%E5%A2%99" class="wiki-page-link">hosts翻墙</a></strong>、<strong><a href="https://github.com/bannedbook/fanqiang/blob/master/unlockurl/hostsmodify.md">如何修改hosts文件</a></strong>
<br/><br/>
修改并保存hosts文件后，用浏览器打开： https://ya.dnstogo.xyz/    ，注意：必须是<b>https</b>, s不能少，否则会打不开。
</li>

<li ><b>问答交流</b><br/>
教程到这里基本结束了，如果哪一步还有疑问，请点下面的翻墙交流链接发帖交流。</li>
</ol>

<h1>使用FileZilla和VPS传输文件教程</h1>
<p>这里我们介绍一下本地计算机和VPS之间传输文件的方法，准确的说，其实是使用 SFTP 的方法。VPS购买后很多朋友都想往 VPS 上面传输文件，上传更新翻墙软件的配置文件或者一些当网盘使用的。</p>
<p>SFTP 是 Secure File Transfer Protocol 的缩写，安全文件传送协议，SFTP与 FTP 有着几乎一样的语法和功能。SFTP 也是 SSH协议 的一部分，在 SSH 协议中，已经包含了一个叫作 SFTP 的安全文件信息传输子系统。SFTP 采用加密传输，所以，使用 SFTP 是传输安全的。当然这是针对普通网友而说，对于大量传播翻墙技术的网友，不建议直接连接自己的公开VPS。</p>
<p>简而言之，SFTP 不需要在 VPS 上额外安装任何东西，我们购买VPS后就能直接使用 SFTP 进行文件传输，非常方便，所以这里介绍基于 SFTP 协议的文件传输方法。</p>

<b>广告插播，如果你觉得自己折腾VPS太麻烦，可以考虑这个服务哦（非本库服务）：</b><br>
<a href="https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA"><img src="https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2free.jpg" height="300" alt="V2free翻墙-不限流量、高速稳定、性价比超强"></a>

<h2>一、下载安装 FileZilla</h2>
<p>FileZilla 是一个开源的 FTP 客户端，也是比较推荐的一个 FTP 客户端，全平台兼容，支持 Windows、Mac 和 Linux 平台，不管你用什么系统，都能使用 FileZilla。除了 FTP 之外，还支持 FTPS 和 SFTP，今天我们要用到的是 SFTP，是目前最方便的向 VPS 传文件的解决方法。</p>
<p>首先下载一个 FileZilla，下载地址：</p>
<p><a href="https://filezilla-project.org/download.php?type=client" target="_blank" rel="noopener">https://filezilla-project.org/download.php?type=client</a></p>
<p>选择自己系统对应的版本进行下载即可。怎么安装就不多说了。</p>
<h2>二、登陆使用 FileZilla</h2>
<p>FileZilla 自带中文语言支持，所以用起来非常方便。</p>
<p>安装之后，点击菜单的 文件 -&gt; 站点管理器，如下图所示。然后点击新站点，填写相关信息。</p>	

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/vps/filezilla1.jpg)

<p>需要填写的信息如下：</p>
<ol>
<li>协议选择 SFTP 协议；</li>
<li>主机填写 VPS 的 IP 地址；</li>
<li>端口填写 VPS 的端口地址；</li>
<li>用户填写 root；</li>
<li>密码填写 VPS 的 root 密码。</li>
</ol>

获取上面的 IP、端口(默认是22，可以不填)、密码等信息，可以参考： [购买Vultr VPS图文教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%B4%AD%E4%B9%B0Vultr%20VPS%E5%9B%BE%E6%96%87%E6%95%99%E7%A8%8B.md)
<p><strong>这里填写的信息其实就是 SSH 的端口和密码，跟SSH登录完全一样。</strong></p>
<p>填写完成后，点击连接。</p>

<p>点击连接后，会看到弹出一个确认页面。勾上 总是信任该主机 前面的勾，然后点确定。</p>

<p>确定之后，就能成功连上 VPS 主机了。如下图所示，左边是本地文件，右边是远程文件，连接成功之后远程VPS默认在/root目录。</p>

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/vps/filezilla2.jpg)

<p><strong>需要提醒的是，注意远程文件的目录，确保文件上传到正确的目录下面。</strong></p>
<p>比如我们需要更新VPS上的V2RAY 服务端配置文件，我们可以把远端目录定位到/etc/v2ray目录（如下图），在右侧窗口右键点击config.json文件，点下载，下载编辑之后，在左边窗口，右键点config.json文件，点上传。<strong>再次提醒一下，先切换到 VPS 上想要存放的文件目录再进行上传。切换目录很简单，单击对应的目录名即可（Windows 上可能需要双击），也可以在“远程站点：”处手工输入后回车。</strong></p>

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/vps/filezilla3.jpg)

至此，我们已经知道了怎么向 VPS 上传文件，或者从VPS下载文件，都还算简单。

本文属于bannedbook系列翻墙教程的一部分，欢迎体验我们提供的免费翻墙软件和教程：
<ul>
<li><a href="https://github.com/bannedbook/fanqiang/wiki/%E5%AE%89%E5%8D%93%E7%BF%BB%E5%A2%99%E8%BD%AF%E4%BB%B6">安卓手机翻墙</a></li>
 <li><a href="https://github.com/bannedbook/fanqiang/wiki/Chrome%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" >Chrome一键翻墙包</a></li>
 <li><a href="https://github.com/bannedbook/fanqiang/wiki/%E7%81%AB%E7%8B%90firefox%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" >火狐firefox一键翻墙包</a></li>
 <li><a href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md" >自建V2ray服务器简明教程</a></li>

版权所有，转载必须保留文章所有链接。

<h1>V2ray官方一键安装脚本（新版）</h1>

网上流传一些v2ray一键安装脚本，有的用起来挺方便，但个人更喜欢采用官方脚本，官方脚本更加安全可靠，你懂得。

V2RAY官方的安装脚本命令为(SSH连接VPS后执行)：<br>
```
apt-get install -y curl
bash <(curl -L https://raw.githubusercontent.com/v2fly/fhs-install-v2ray/master/install-release.sh)
```

复制上面的安装脚本命令到VPS服务器里，复制代码用鼠标右键的复制，然后在vps里面右键粘贴进去，因为ctrl+c和ctrl+v无效。接着按回车键，脚本会自动安装。

<b>广告插播，如果你觉得自己折腾VPS太麻烦，可以考虑这个服务哦（非本库服务）：</b><br>
<a href="https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA"><img src="https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/v2free.jpg" height="300" alt="V2free翻墙-不限流量、高速稳定、性价比超强"></a>

安装成功后需要自己准备一个配置文件，可以参考我的配置文件，请自行修改端口、uuid等参数(执行下面的命令下载参考配置文件)：<br>
```
wget https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/server-cfg/v2/config.json  -O -> /usr/local/etc/v2ray/config.json
```

安装完成后，请执行：
`service v2ray restart` ,以确保v2ray启动成功。

控制 V2Ray 的运行的常用命令：

`service v2ray restart | force-reload  |start|stop|status|reload `

测试V2Ray配置文件：

`/usr/local/bin/v2ray test -config /usr/local/etc/v2ray/config.json`

本文属于bannedbook系列翻墙教程的一部分，请继续阅读<a href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md" >自建V2ray服务器简明教程</a>的其它章节。也欢迎体验我们提供的免费翻墙软件和教程：
<ul>
  <li><a href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md" >自建V2ray服务器简明教程</a></li>
  <li><a href="https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAShadowsocks%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md">自建Shadowsocks服务器简明教程</a></li>
<li><a href="https://github.com/bannedbook/fanqiang/wiki/%E5%AE%89%E5%8D%93%E7%BF%BB%E5%A2%99%E8%BD%AF%E4%BB%B6">安卓手机翻墙</a></li>
 <li><a href="https://github.com/bannedbook/fanqiang/wiki/Chrome%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" >Chrome一键翻墙包</a></li>
 <li><a href="https://github.com/bannedbook/fanqiang/wiki/%E7%81%AB%E7%8B%90firefox%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85" >火狐firefox一键翻墙包</a></li>
</ul>

版权所有，转载必须保留文章所有链接。

很多的翻墙软件基本都属于未知来源, 因此 MAC电脑设置 允许未知来源的应用 比较方便翻墙。

终端里输入

`sudo spctl --master-disable`

然后确认enter，密码是系统开机密码。

然后你就发现系统偏好设置⇨安全性和隐私⇨通用⇨出现了任何来源了，勾选即可。

![MAC允许未知来源的应用](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/mac-allow-unkown-app.webp)

查看状态

`sudo spctl --status`

关闭

`sudo spctl --master-disable`

打开

`sudo spctl --master-enable`

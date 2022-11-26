# macOS翻墙 V2rayX 教程

前言：V2rayX很久没有更新了，一些采用新技术的节点用不了，不推荐使用此软件了。

## 相关阅读
*   [V2ray机场，V2ray/SS免费翻墙节点](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)

*   [安卓翻墙软件](https://github.com/bannedbook/fanqiang/wiki/%E5%AE%89%E5%8D%93%E7%BF%BB%E5%A2%99%E8%BD%AF%E4%BB%B6)
*   [安卓翻墙APP教程](https://github.com/bannedbook/fanqiang/tree/master/android)
*   [Chrome一键翻墙包](https://github.com/bannedbook/fanqiang/wiki/Chrome%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85)
*   [火狐firefox一键翻墙包](https://github.com/bannedbook/fanqiang/wiki/%E7%81%AB%E7%8B%90firefox%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85)
*   [自建V2ray服务器翻墙简明教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md)
*   [自建Shadowsocks服务器翻墙简明教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAShadowsocks%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md)
*   [免费ss账号](https://github.com/bannedbook/fanqiang/wiki/%E5%85%8D%E8%B4%B9ss%E8%B4%A6%E5%8F%B7)
*   [v2ray免费账号](https://github.com/bannedbook/fanqiang/wiki/v2ray%E5%85%8D%E8%B4%B9%E8%B4%A6%E5%8F%B7)
*   [苹果电脑MAC翻墙](https://github.com/bannedbook/fanqiang/wiki/%E8%8B%B9%E6%9E%9C%E7%94%B5%E8%84%91MAC%E7%BF%BB%E5%A2%99)
*   [iphone翻墙](https://github.com/bannedbook/fanqiang/wiki/iphone%E7%BF%BB%E5%A2%99)
*   [TorBrowser一键翻墙包](https://github.com/bannedbook/fanqiang/wiki/TorBrowser%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85)

您需要确保网络畅通。下载软件，配置节点信息，开始使用。  
仔细阅读并遵从所有步骤，一般仅需几分钟。

* * *

第一步，下载用于Mac设备的V2RayX软件
----------------------

可在本站下载，也可以前往Github下载

|描述/Description|链接/Links|
|--- |--- |
|V2RayX （Github所有release版本）|[Github](https://github.com/Cenmrev/V2RayX/releases)|

第二步，解压下载的zip文件，得到V2RayX文件
-------------------------

将V2RayX文件[拷贝到应用程式](https://www.google.com/search?q=Mac+%E6%8B%B7%E8%B4%9D%E7%A8%8B%E5%BA%8F%E5%88%B0%E5%BA%94%E7%94%A8%E7%A8%8B%E5%BA%8F)，然后直接打开，在弹出的窗口点"install"进行安装，此过程可能需要输入系统密码，授予相关权限，请放心。如果安装或打开程序有困难，参考：[打开来自身份不明开发者的 Mac App1](https://support.apple.com/zh-cn/guide/mac-help/mh40616/mac)或[https://www.jianshu.com/p/3a5ceb412f15](https://www.jianshu.com/p/3a5ceb412f15)

![](https://v2free.org/docs/SSPanel/macOS/V2rayX_files/mac-03.png)

如果打开程序，出现如下提示

![](https://v2free.org/docs/SSPanel/macOS/V2rayX_files/mac-01.png)

请前往“系统偏好设置”－“安全性和隐私”，点“仍要打开”，再点"install"安装

![](https://v2free.org/docs/SSPanel/macOS/V2rayX_files/mac-02.png)

安装完成后，可进入launchpad，找到V2RayX图标打开。

第三步，打开程序，配置节点
-------------

右上角出现V2RayX图标，点击图标调出菜单，点Configure，进行节点设置,建议首选方法1:订阅方式

|方法|描述|
|--- |--- |
|1.订阅方式|通过v2ray订阅地址批量导入节点|
|2.URL方式|输入一个vmess://...的URL链接，即可完成一个节点配置，从[节点列表](/user/node)，点击节点展开节点信息，然后复制vmess链接|
|3.手动配置|增加新节点，并逐一配置相关节点信息|

### 方法一，V2rayX设置节点订阅

1\. 首先需要保证你的 V2RayX 客户端版本 >= 1.5.1，通过上面就可下载

2\. 复制你的v2ray订阅链接网址


注册机场以获取节点订阅链接或者找免费的订阅链接。

这里我们推荐一个[V2ray机场，有免费V2ray和SS节点](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)，[点击注册](https://go.runba.cyou/auth/register?code=cd79)，注册后在该机场用户中心拷贝订阅链接。

教育网的网友如果打不开上面的链接，请使用这个链接：
https://cdn.runba.cyou/auth/register?code=cd79

注册后免费获得1024M初始流量，每日[签到](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/checkin.jpg)可获得300-500M免费流量。
注册登录后，用个人邀请链接 邀请新用户注册还可获得流量奖励，如果新用户成为付费用户，你还可以赚取高达20%终生佣金奖励。

机场的 **订阅链接** 非常重要，你应当把它当做密码一样妥善保管。

3\. 启动V2RayX,注意，V2RayX启动后并不会弹出窗口，而是在右上角出现个小图标，如下图：

![](https://v2free.org/docs/SSPanel/macOS/V2rayX_files/v2rayx1.jpg)

点击这个小图标，弹出菜单，再点Configure...，如下图：

![](https://v2free.org/docs/SSPanel/macOS/V2rayX_files/v2rayx2.jpg)

进入 V2RayX 的配置界面，点击 Advanced （没有这个按钮说明你使用的是老版本）

![](https://v2free.org/docs/SSPanel/macOS/V2rayX_files/v2rayx3.jpg)

4\. 切换到 Subscription 面板，再点击左下方“+”号

![](https://v2free.org/docs/SSPanel/macOS/V2rayX_files/v2rayx4.jpg)

鼠标双击”enter your subscription link here“，粘贴v2ray订阅链接网址替换掉”enter your subscription link here“，然后**点击一下空白处**，然后点击“Finish”保存 **（注意：填写后，需点击一下空白处，否则无法保存设置）**

![](https://v2free.org/docs/SSPanel/macOS/V2rayX_files/v2rayx5.png)

设置成功后，会自动更新订阅，再次点击屏幕右上方图标，勾选PAC模式（国内外网站自动分流），勾选一个节点，比如下图中的"台湾-NF..."节点，并点击“Load core”

![](https://v2free.org/docs/SSPanel/macOS/V2rayX_files/v2rayx6.png)

OK，设置成功，打开浏览器即可开始使用。

提示：点击“Update subscription”可手动更新订阅信息，建议隔几天可手动更新一次订阅，或者发现有节点不好用时也可更新一下。

### v2rayx的几种代理模式

*   pac模式，根据pac文件里的网站列表判断是否走代理
*   manual 顾名思义，手动模式，不配置系统级代理。可以自行使用浏览器插件或其他软件配置需要的代理模式
*   globle 全局模式，不管哪国域名，不管哪国ip，统统走代理
*   v2ray rules是由`geoip.dat`和`geosite.dat`两个文件控制，该模式会将`geosite:cn`和`geoip:cn`（中国域名和中国ip）直连，除此之外走代理

### 方法二，URL添加

打开程序，右上角出现V2RayX图标，点击图标调出菜单，点Configure，进行节点设置

![](https://v2free.org/docs/SSPanel/macOS/V2rayX_files/mac-04.png)

点击图中鼠标指向的import按钮，选择import from other links

![](https://v2free.org/docs/SSPanel/macOS/V2rayX_files/mac-07.jpg)

复制节点URL（url格式：vmess://...），然后粘贴入弹出的输入框，点OK

![](https://v2free.org/docs/SSPanel/macOS/V2rayX_files/mac-08.png)

回到节点配置页面，可以看到已自动导入该节点信息，点OK

重複上述步骤即可导入多个节点

再次调出菜单，确保servers已勾选可用节点，点"Load core"开启V2RayX，一般选择PAC Mode

![](https://v2free.org/docs/SSPanel/macOS/V2rayX_files/mac-10.png)

完成，可打开浏览器访问网址测试

### 方法三，手动配置(本站v2free.org的节点一般不需要手动，上面2种方法就可以了)

打开程序，右上角出现V2RayX图标，点击图标调出菜单，点Configure，进行节点设置

![](https://v2free.org/docs/SSPanel/macOS/V2rayX_files/mac-04.png)

先点左下角"+"，增加节点。然后根据提供商提供的节点信息，填入address(服务器地址），后面是端口号，User ID对应UUID，以及alterID，其他如果没有相关参数，无须修改

DNS处填8.8.8.8,8.8.4.4，点OK保存此节点

![](https://v2free.org/docs/SSPanel/macOS/V2rayX_files/mac-05.png)

再次调出菜单，确保servers已勾选可用节点，点"Load core"开启V2RayX，一般选择PAC Mode

![](https://v2free.org/docs/SSPanel/macOS/V2rayX_files/mac-06.png)

完成，可打开浏览器访问网址测试

如果某些网址无法访问，可尝试Global Mode


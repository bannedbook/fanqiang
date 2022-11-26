# macOS翻墙 V2rayU教程

##### 步骤1 下载安装 V2rayU

[V2rayU下载](https://github.com/yanue/V2rayU/releases)

下载V2rayU的安装文件，文件格式为”dmg”格式，相当于一个光盘镜像文件。

![](https://v2free.org/docs/SSPanel/macOS/V2RayU_files/v2rayu1.jpg)

图：下载 V2rayU 程序文件

下载文件一般放置于用户的”下载”文件夹，使用 Finder找到下载文件，然后双击运行：

![](https://v2free.org/docs/SSPanel/macOS/V2RayU_files/macOS-V2rayU-course-2-install-app.jpg)

图：运行 V2rayU 安装程序

此时，桌面上会生成一个虚拟光盘，并将下载的镜像文件装载到该光盘，并弹出一个窗口，提示拖拽完成安装。

按照提示将窗口左侧的”V2rayU”图标拖拽到窗口右侧的”Applications”文件夹，即完成了 V2rayU 的安装：

![](https://v2free.org/docs/SSPanel/macOS/V2RayU_files/macOS-V2rayU-course-2-copy-app.jpg)

图：拷贝 V2rayU 至应用程序目录

安装过程其实就相当于把 V2rayU 的程序文件夹复制到 Mac 电脑中，放置在”Applicationes”目录是方便应用程序的访问和使用。

复制完成后，就可以在应用程序中看到 V2rayU 应用图标，表示安装已经成功。我们就可以把虚拟光盘弹出，然后删除下载目录中的 dmg 文件。

![](https://v2free.org/docs/SSPanel/macOS/V2RayU_files/macOS-V2rayU-course-2-finish-install.jpg)

图：V2rayU 完成安装

mac OS 安装应用程序相比 Windows 程序就容易的多，每个应用可以理解为 Windows 的绿色软件，可以直接执行而不需要过多配置。

在”应用程序”目录看到 V2rayU 图标，就完成了软件安装。

##### 步骤2 启动运行V2rayU

mac OS 的每个程序都可以理解为独立的应用，需要使用的话直接双击即运行了该应用。

现在双击 V2rayU 图标打开该应用，如果打开程序打不开，请参考：[打开来自身份不明开发者的 Mac App1](https://support.apple.com/zh-cn/guide/mac-help/mh40616/mac)或[https://www.jianshu.com/p/3a5ceb412f15](https://www.jianshu.com/p/3a5ceb412f15)

![](https://v2free.org/docs/SSPanel/macOS/V2RayU_files/macOS-V2rayU-course-3-excute-confirm.jpg)

图：确认运行 V2rayU 应用

由于应用不是从苹果应用商店获得的，所以在运行 V2rayU 前系统会提示用户确认。不用过分纠结这个提示，直接点击打开运行 V2rayU 应用。

![](https://v2free.org/docs/SSPanel/macOS/V2RayU_files/macOS-V2rayU-course-3-input-password.jpg)

图：输入管理员用户及密码

之后，输入mac OS 的管理员用户名及密码，就可以真正打开 V2rayU 应用了。

![](https://v2free.org/docs/SSPanel/macOS/V2RayU_files/macOS-V2rayU-course-3-open-app.jpg)

图：V2rayU 应用运行界面

可以看到，V2rayU 已经成功运行。

但是此时还不能代理上网，因为并没有给 V2rayU 配置服务器。

##### 步骤3 配置V2RayU应用

点击任务栏该应用图标-打开服务器设置(如下图)

![](https://v2free.org/docs/SSPanel/macOS/V2RayU_files/v2rayu_1.png)

##### 步骤4 导入V2ray节点

###### 方法一：通过订阅链接，导入V2ray服务器

获取订阅链接网址
----


注册机场以获取节点订阅链接或者找免费的订阅链接。

这里我们推荐一个[V2ray机场，有免费V2ray和SS节点](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)，[点击注册](https://go.runba.cyou/auth/register?code=cd79)，注册后在该机场用户中心拷贝订阅链接。

教育网的网友如果打不开上面的链接，请使用这个链接：
https://cdn.runba.cyou/auth/register?code=cd79

注册后免费获得1024M初始流量，每日[签到](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/checkin.jpg)可获得300-500M免费流量。
注册登录后，用个人邀请链接 邀请新用户注册还可获得流量奖励，如果新用户成为付费用户，你还可以赚取高达20%终生佣金奖励。

机场的 **订阅链接** 非常重要，你应当把它当做密码一样妥善保管。

**如果订阅链接无法更新节点,请按下文方法二:vmess url导入 先导入1、2个节点，启用节点能翻后再用订阅链接更新节点。**

右键点击出现在顶部栏中的「V2RayU」图标，在弹出菜单中选择「订阅设置」。

![v2rayu_5](https://v2free.org/docs/SSPanel/macOS/V2RayU_files/v2rayu_5.png)

将订阅链接粘贴到「地址」栏中，点击「添加」，待订阅地址中出现订阅后，点击「更新」获取节点。

![v2rayu_5](https://v2free.org/docs/SSPanel/macOS/V2RayU_files/v2rayu_6.png)

###### 方法二：vmess url导入

这种方式只能一个一个节点的导入，比较麻烦，推荐首选上面的订阅链接方式。如果订阅网址被封，则可以用这种方法先导入一两个节点，启用节点后成功后，再用订阅链接导入节点。
从[V2free节点列表](/user/node)，点任一节点的问号图标，然后复制vmess链接
然后参考下图导入服务器
![v2rayu_2](https://v2free.org/docs/SSPanel/macOS/V2RayU_files/v2rayu_2.png)

##### 步骤5 连接上网

点击任务栏应用图标 --- 服务器列表--勾选刚配置的服务器

选择"Pac模式"

### V2rayU的几种代理模式

*   pac模式，根据pac文件里的网站列表判断是否走代理
*   manual 顾名思义，手动模式，不配置系统级代理。可以自行使用浏览器插件或其他软件配置需要的代理模式
*   globle 全局模式，不管哪国域名，不管哪国ip，统统走代理

点击 "Turn v2ray-core On" ，这时候就可以使用VPN上网了

![v2rayu_3](https://v2free.org/docs/SSPanel/macOS/V2RayU_files/v2rayu_3.png)

最后，通过浏览器验证代理是否成功即可。

![](https://v2free.org/docs/SSPanel/macOS/V2RayU_files/macOS-V2rayU-course-4-verify-app.jpg)

图：验证 V2rayU 代理效果。

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

### V2rayU崩溃的解决办法

V2RayU 更新订阅 之后打不开了，打开终端，输入下面的命令

```
rm -rf ~/Library/LaunchAgents/yanue.v2rayu.v2ray-core.plist
rm -rf ~/Library/Preferences/net.yanue.V2rayU.plist
```

### V2rayU 端口被占用的问题

错误信息：

failed to listen on address: 127.0.0.1:1087 > listen tcp 127.0.0.1:1087: bind: address already in use

解决方法参考(换个端口)：

https://github.com/yanue/V2rayU/issues/380


# Clash for Windows 翻墙教程

1\. 简介
------

ClashDotNetFramework 是基于.NET5的图形化 Clash 分支，似乎并非是开源软件，请根据您的情况选择是否使用。

**支持的协议：** Vmess, Shadowsocks, Snell , SOCKS5 , ShadowsocksR，注意：目前不支持vless协议，vless节点会被显示为vmess并连接超时。

2\. 您的V2free Clash订阅链接
--------------

注册机场以获取节点订阅链接或者找免费的订阅链接。

这里我们推荐一个[V2ray机场，有免费V2ray和SS节点](https://github.com/bannedbook/fanqiang/wiki/V2ray%E6%9C%BA%E5%9C%BA)，[点击注册](https://go.runba.cyou/auth/register?code=cd79)，注册后在该机场用户中心拷贝Clash订阅链接。

教育网的网友如果打不开上面的链接，请使用这个链接：
https://cdn.runba.cyou/auth/register?code=cd79

注册后免费获得1024M初始流量，每日[签到](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/checkin.jpg)可获得300-500M免费流量。
注册登录后，用个人邀请链接 邀请新用户注册还可获得流量奖励，如果新用户成为付费用户，你还可以赚取高达20%终生佣金奖励。

机场的 **订阅链接** 非常重要，你应当把它当做密码一样妥善保管。

3\. 下载与安装
---------
[点击下载ClashDotNetFramework](https://github.com/bannedbook/fanqiang/releases)

下载后解压全部文件，然后双击运行主程序即可。 
 
![explorer_ntyMxVTeQ7.png](https://v2free.org/docs/SSPanel/Windows/ClashDotNetFramework_files/3002202166.png)  
 
**注意：** 软件运行需要本机有.NET5依赖，如果首次启动的时候有如下提示，请点击“是”，然后下载并安装对应的依赖包。 
 
![ClashDotNet_ImGYJ3GVxX.png](https://v2free.org/docs/SSPanel/Windows/ClashDotNetFramework_files/2374135324.png)  

根据自己的系统类型进行选择，这里选择X86：  
![chrome_qrhkpt1ZC4.png](https://v2free.org/docs/SSPanel/Windows/ClashDotNetFramework_files/442620522.png)  


4\. 语言设置
--------

该软件原生支持简体中文（如果已正常显示简体中文，可跳过此步骤），具体操作步骤如下：  
点击 `Settings` → `Display` → `Language`  
![ClashDotNet_k1uLG5mSB9.png](https://v2free.org/docs/SSPanel/Windows/ClashDotNetFramework_files/3188293196.png)  
![ClashDotNet_RZdUtP28g6.png](https://v2free.org/docs/SSPanel/Windows/ClashDotNetFramework_files/1446880618.png)  
如果出现如上图这种比较奇怪的中文字体，说明本机缺少部分字体库，可以回到官方下载页面下载对应的文字库，解压后双击运行，点击安装。然后重启一次ClashDotNetFramework客户端。  
![chrome_5VvMtqmPbl.png](https://v2free.org/docs/SSPanel/Windows/ClashDotNetFramework_files/2192815081.png)  
![explorer_5op1h4saIV.png](https://v2free.org/docs/SSPanel/Windows/ClashDotNetFramework_files/2410481344.png)  
![fontview_c65D5CJZOU.png](https://v2free.org/docs/SSPanel/Windows/ClashDotNetFramework_files/202185407.png)  
然后字体就可以正常显示了：  
![ClashDotNet_tssLVb0XBf.png](https://v2free.org/docs/SSPanel/Windows/ClashDotNetFramework_files/955605122.png)

5\. 添加订阅
--------

首先从本页面上方第二节，复制好自己的Clash订阅链接。  
然后点击 `主页` → `配置`，点击加号位置，  
![ClashDotNet_7DwXUwA2dd.png](https://v2free.org/docs/SSPanel/Windows/ClashDotNetFramework_files/1599691601.png)  
然后在弹出的窗口中粘贴自己的Clash订阅链接并点击`订阅`。
![ClashDotNet_WOyNbPVM6i.png](https://v2free.org/docs/SSPanel/Windows/ClashDotNetFramework_files/2428975405.png)  
然后点击一下自己刚添加的这个订阅，显示为绿色高亮，才是选中状态。  
![ClashDotNet_1EOnnD8acq.png](https://v2free.org/docs/SSPanel/Windows/ClashDotNetFramework_files/3224149311.png)  
然后在`主页`上点击`代理`，即可看到订阅中的节点。小闪电按钮可以快速检测可用性。  
![ClashDotNet_DCbAY2rOda.png](https://v2free.org/docs/SSPanel/Windows/ClashDotNetFramework_files/2120685303.png)  
注意：Clash使用 HTTP HEAD 方法对测试网址（server\_check\_url）进行网页相应测试，以确认节点的可用性。数值在5000以内均为正常值，超出则显示为超时。数值大小和网速快慢没有什么关系。

6\. 启动代理
--------

在上一步中选好节点，点击`设置`，然后勾选`系统代理`，即可科学上网。  
![ClashDotNet_XoT2QeheHM.png](https://v2free.org/docs/SSPanel/Windows/ClashDotNetFramework_files/3961348341.png)  
代理启动以后，主页右侧的表格会有所波动。  
![ClashDotNet_DCbAY2rOda.png](https://v2free.org/docs/SSPanel/Windows/ClashDotNetFramework_files/1970949877.png)

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
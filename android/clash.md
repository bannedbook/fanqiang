# 安卓手机翻墙APP: Clash for Android教程

1\. 简介
------
Clash for Android 是安卓系统上的一款综合翻墙软件，支持v2ray/vmess、SS和SSR协议。 
 


**特性：**

*   可随时切换代理模式及节点
*   支持节点批量延迟测试
*   通过订阅链接一键配置
*   规则命中分析
*   日志输出

**系统要求：**

- Android 5.0+ (minimum)
- Android 7.0+ (recommend)
- `armeabi-v7a` , `arm64-v8a`, `x86` or `x86_64` Architecture


2\. 下载安装
--------
Clash for Android 为免费 app ，已于 2019.12.10 上架 Google Play 。

[Clash for Android下载](https://github.com/Kr328/ClashForAndroid) 
 
安卓手机使用 Chrome 浏览器可能遇到无法下载的情况，可复制教程链接到其它浏览器尝试下载。
  
语言设置路径：`Settings` → `Interface` → `Language` → `Simplified Chinese`  
`设置` 👉 `界面` 👉 `语言` 👉 `简体中文`  


3\. 快速上手
--------

Clash for Android 支持两种导入节点及配置文档的方式：

*   URL （订阅）
*   本地导入  
    首先讲一下订阅。

### 3.1 快速订阅

登录您的机场获取Clash订阅，推荐免费公益机场-V2free机场：

[![免费公益机场-不限流量](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/freenode.jpg)](https://w1.v2dns.xyz/auth/register?code=cd79)

点击注册链接：<a href="https://w1.v2dns.xyz/auth/register?code=cd79" target="_blank">w1.v2dns.xyz</a>，注册后在该网站用户中心获取订阅链接或拷贝全部节点 URL，根据其用户中心的快速使用帮助下的相应操作系统的帮助文档操作即可。
注册后免费获得1024M初始流量，每日[签到](https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/images/checkin.jpg)可获得300-500M免费流量。更有不限流量节点可用。
注册登录后，用个人邀请链接 邀请新用户注册还可获得流量奖励，如果新用户成为付费用户，你还可以赚取高达20%终生佣金奖励。
  
复制好了Clash订阅之后打开Clash for Android应用程序。请点击`配置`。  
![IMG_7843.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/1072376875.jpg)  
请在新弹出的窗口中点击`新配置`。  
![IMG_7844.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/1118270140.jpg)  
然后选择`从URL导入`。在对应地方填写订阅地址并保存。  
![IMG_7845.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/2169466048.jpg)  
![IMG_7847.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/3843008539.jpg)  
![IMG_7849.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/932669651.jpg)  
点击节点右侧的三个点按钮，可以修改订阅设置。如图：  
![IMG_7850.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/1168337143.jpg)  
提示：机场服务器信息可能会不定时更新，若出现大面积节点超时现象，可尝试刷新订阅。  
返回首页。点击开关，即可进行代理。会提示是否同意创建VPN，请点击`允许`。  
![IMG_7851.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/2409469823.jpg)  
开启代理后，可以点击中间的代理选项卡，进入策略组面板，在这里可以切换节点。直接点击你想要的节点即可进行切换。  
![IMG_7852.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/2753500055.jpg)  
点击⚡图标可以进行延迟测试，测试结果将显示在节点名称右侧。  

Clash for Android 测试延迟的方法为从目标 policy 返回 http response header 数据包的时间，并不是简单的 ping 。  
测试延迟会导致机场网页上显示的在线设备数异常飙升，这是正常现象，等一等就好了。

  
点击右上角的三个点按钮，可以进入更多设置：

*   刷新订阅
*   更改代理模式
*   改变代理组（策略组）排序
*   改变代理（节点）排序
*   前缀合并（即节点名字前缀相同的进行归类显示）  
    ![IMG_7853.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/550116317.jpg)  
    ![IMG_7854.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/838427780.jpg)

### 3.2 本地文件导入

点击 `配置` 👉 `新配置` 👉 `从文件导入`，然后从本地文件夹选择自己要导入的配置文档。  
某些厂商的ROM可能报如下错误，请选择其他文件管理器导入。  
![10](/Clash_files/romfault.png)

### 3.3 查询日志

点击`日志`面板，然后选择`Clash日志捕捉工具`即可抓取日志。默认是关闭日志的，以防内存溢出。  
![IMG_7857.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/1404728428.jpg)  
![IMG_7858.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/1836797657.jpg)  
![IMG_7859.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/2336060890.jpg)

### 3.4 其它设置

![IMG_7860.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/3789481771.jpg)  
![IMG_7862.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/454476014.jpg)

### 3.5 分应用代理

点击`设置` 👉 `网络`，最下方可以设置`分应用代理`。  
点击`访问控制模式`可以切换黑白名单。自行理解即可。  
点击`访问控制应用包列表`即可选择应用。  
![IMG_7863.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/1278124387.jpg)

### 3.6 切换代理模式

#### 3.6.1 2.0.18及之前版本

如果是2.0.18及之前的版本，代理模式设置路径为`代理` → `模式`，如图：  
![IMG_7853.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/550116317.jpg)  
![IMG_7854.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/838427780.jpg)

#### 3.6.2 2.1.1之后的版本

如果是2.1.1之后的版本，代理模式设置路径为`设置` → `覆写` → `模式`，如图：  
![IMG_4577.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/1118590743.jpg)  
![IMG_4579.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/3855851416.jpg)  
![IMG_4578.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/1753717891.jpg)

### 3.7 语言设置

语言设置路径：`Settings` → `Interface` → `Language` → `Simplified Chinese`  
`设置` 👉 `界面` 👉 `语言` 👉 `简体中文`  
![IMG_7864.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/3362312117.jpg)  
若当前系统语言为简体中文，则软件默认显示简体中文。  
若当前系统语言为繁體中文或其它语言，则默认显示English。

### 3.8 暗黑模式

Clash for Android现已适配暗黑模式。  
入口：`设置` 👉 `界面` 👉 `暗黑模式`  
开启后效果如图：  
![IMG_7865.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/59156606.jpg)  
![IMG_7866.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/382223072.jpg)

4\. 常见的订阅错误报告
-------------

如果遇到以下提示：

    Invalid Config:yaml:unmarshal errors: line 1:cannot unmarshal !!str c3M6Ly9...

说明用错了订阅链接，请检查自己是不是复制错了或者多了空格之类的。

如果遇到此类提示：

    Invalid Config:Value for 'Proxy' is invalid:Unexpected null or empty

说明你还没买套餐，或者订阅为空。请联系你所在机场的管理员。

5\. 易用性设置
---------

大部分安卓ROM都会因为电池策略导致Clash for Andorid应用程序被杀掉导致无法连接网络。出现这种情况的特征是通知栏中VPN连接仍然存在，但实际上无法访问网络（有时也包括国内网络）。这是因为Clash for Android主程序和VPN框架是独立存在的，主程序被系统清理后会导致流量仍然通过VPN路由到本地，但此时没有应用程序来处理这些流量，导致无法上网。

### 5.1 始终开启VPN

以RealmeX(Android 10)为例，点击`设定` 👉 `其他无线连接` 👉 `VPN`,打开`一律与VPN保持连线`。  
![IMG_7868.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/1483525553.jpg)  
![IMG_7869.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/2937852562.jpg)  
![IMG_7872.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/2475363784.jpg)  
![IMG_7873.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/1266218543.jpg)

### 5.2 省电策略/允许后台运行

长按Clash图标，选择`应用程式资讯`。  
勾选`允许自动啓动`和`允许其他应用程式关联啓动`。  
点击`耗电保护`,选择`允许背景执行`。  
![IMG_7874.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/722228769.jpg)  
![IMG_7875.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/1110606495.jpg)  
![IMG_7876.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/74665642.jpg)

### 5.3 通知栏快速启动

![IMG_7878.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/1192405662.jpg)  
![IMG_7879.JPG](https://v2free.org/docs/SSPanel/Android/Clash_files/2936823915.jpg)

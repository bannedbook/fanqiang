# readme

**备注，RESILIO SYNC 的 Tracker Server被墙，如果无法下载翻墙软件，请看**：   
[使用 IPFS 来下载和传播翻墙软件](https://www.bannedbook.org/bnews/fanqiang/20171119/847566.html)   
[聊聊 GFW 如何封杀 Resilio Sync（BTSync）？以及如何【免翻墙】继续使用？](https://www.bannedbook.org/bnews/renquan/minyun/20190115/1194998.html)

一、RESILIO SYNC是啥？ BT 下载，相信大伙儿都知道的。今儿个要介绍的 Resilio Sync，跟 BT下载一样，都是采用 P2P协议来进行传输。简而言之，Resilio Sync是一个文件同步工具，让你在几台不同的设备之间，同步文件。

二、RESILIO SYNC有啥优点？

\(一\)作为“同步工具”的优点 首先来说说 Resilio Sync作为同步工具的优点。至少有如下几个：

1.不需要有自己的服务器

1. 不需要有公网 IP——如果两台设备都在【内网】，只要这两台设备都能访问到公网，就可以相互同步

3.文件数量【无】限制，文件大小【无】限制

4.支持多种网络形态——可以“公网上互相同步”，也可以是“局域网内相互同步”。

1. 支持各种操作系统（以下列表摘自洋文维基百科）

Microsoft Windows \(XP SP3 or later\)

Mac OS X \(10.6 or later\)

Linux

FreeBSD

NAS Devices

Android

Amazon Kindle

iOS

Windows Phone

\(二\)作为“分布式网盘”的优点 再来说说 Resilio Sync 作为“分布式网盘”的优点——这也就是为啥，俺决定用它来分享 “[翻墙工具](https://github.com/bannedbook/fanqiang/wiki/Chrome%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85)”。

1. 【没有】存储空间的限制——真要说空间限制，那就是参与的节点，贡献的硬盘尺寸（如今 TB

   级的硬盘已经不稀奇了）

2.【没有】下载流量的限制——大部分商业网盘都有这个限制。

3.【没有】文件大小限制——大部分商业网盘对“单个文件大小”都作了限制。

1. 【没有】审查——俺想在上面分享啥，就分享啥——咱们朝廷管不了（一想到这点，心里那个爽啊）。

5.【没有】费用——老读者都明白，俺是很讨厌付费服务的——其实俺不缺钱，俺是担心身份暴露（即使“比特币”支付，也【不是】彻底“匿名”的）

1. 【很难】被封杀——与之对比，国外的商业网盘，GFW说封杀就封杀（比如

   “微软网盘”和“Dropbox网盘”都撞墙了）

（看完这些优点，或许你就明白——为啥 Resilio Sync被称为“Dropbox终结者”）

另外，开源的 Resilio Sync替代品已经出现了——名叫 Syncthing。可惜还不够成熟，而且也不适合用来做大范围分享。

\(三\)“安全方面”的优点 不同的 Resilio Sync节点之间进行数据传输时，会采用“强加密”的方式，以防止数据传输流量被嗅探（偷窥）。只有参与同步的节点，才能解密；而那些帮你中转的“中转服务器”，是没有办法解密的。因此，即使你的 ISP（电信运营商）监视你的流量，也【无法】知道你通过 Resilio Sync传输了啥文件。

三、RESILIO SYNC的下载 要下载 Resilio Sync，请点击[https://github.com/killgcd/fg/raw/master/fqb/Resilio-Sync.exe](https://github.com/killgcd/fg/raw/master/fqb/Resilio-Sync.exe) 。如果你下载的是 Windows上运行的 exe，会自带“数字签名”。为了保险起见，可以查看一下如下图。

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/RESILIO-SYNC/img/image001.jpg)

如果使用Windows XP系统，请从这里下：[https://github.com/killgcd/fg/raw/master/fqb/BTSync-1.4.111.exe](https://github.com/killgcd/fg/raw/master/fqb/BTSync-1.4.111.exe)

四、RESILIO SYNC的安装 （考虑到大部分人用的是 Windows，俺就以这个系统为例），安装很简单，启动安装程序后，一路点击“下一步（Next）”即可。

安装完毕，程序运行之后，Resilio Sync会弹出如下窗口：

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/RESILIO-SYNC/img/image002.jpg)

在上面的窗口中，需要选择一个名字，默认会显示你的Windows用户名，建议修改成一个可以保持匿名的名称，然后把下面的2个框打勾，然后再点击“入门”按钮。

然后弹出下面的画面，如果不想订阅直接点右上角的X关掉窗口即可：

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/RESILIO-SYNC/img/image003.jpg)

然后就出现了程序主界面：

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/RESILIO-SYNC/img/image004.jpg)

五、RESILIO SYNC的使用 前面说了好多屁话，现在终于说到重点部分啦。

\(一\)“同步密钥”的概念 要使用 Resilio Sync的功能，首先要了解“同步密钥”的概念。

每个参与同步的目录，都有其密钥。你只有拿到这个密钥，才能同步该目录的文件。

对于普通的使用场景，每个同步目录对应两个密钥：一个是“读写密钥”，另一个是“只读密钥”。顾名思义，拥有读写密钥的节点，可以修改同步目录的内容；反之，拥有“只读密钥”的节点，只能读取，无法修改——所谓的“无法修改”，就是说：即使你修改了同步目录的内容，修改结果也【不会】同步给其它节点（所以这种修改是【无】意义的）。

至于如何得到密钥，请看下面的介绍。

\(二\)如何“接受同步”（下载[翻墙软件](https://github.com/bannedbook/fanqiang/wiki/%E7%81%AB%E7%8B%90firefox%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85)） 先从比较简单的“接受同步”说起。

俺已经共享了一个“[翻墙软件](https://github.com/bannedbook/fanqiang/wiki/Chrome%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85)”的同步目录，然后俺把只读密钥公布如下：

BG2G54AGRNPC5YXSTLQV5PNWL7ULYRIPC

当你拿到这个密钥之后，可以通过如下步骤，导入密钥，并在你本机创建一个同步目录。（截图如下），点击“添加文件夹”右边的倒三角图标，然后点“输入密钥或链接”：

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/RESILIO-SYNC/img/image005.jpg)

输入密钥后，点击 下一步：

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/RESILIO-SYNC/img/image006.jpg)

然后会弹出选择文件夹的对话框，请选择一个有读写和修改权限的文件夹，完成上述步骤之后，Resilio Sync 就把你选择的目录作为同步目录。今后俺如果往自己的“翻墙软件”目录增加了新的软件，或者更新了原来的软件，你的Resilio Sync会自动同步并保存到你的这个目录。

在这个同步目录里面会创建一个名为 .sync 的子目录。这个.sync目录会包含 Resilio Sync的一些信息（比如密钥的信息），你可别把它给删喽。

\(三\)使用代理服务器 如果你让 Resilio Sync 走代理（proxy）的方式联网，最好勾选“使用代理服务器”这个复选框。

![](https://raw.githubusercontent.com/bannedbook/fanqiang/master/RESILIO-SYNC/img/image007.jpg)

点击上图“首选项”，再点“高级”按钮，即可看到“使用代理服务器”的复选框了。

六、时间同步 运行 Resilio Sync的系统，最好是开启自动时间同步。否则的话，如果系统时间严重不准，会导致 Resilio Sync无法正常工作。

七、俺分享的资源

\(一\)翻墙工具 在刚才示范的时候，已经提到——俺用Resilio Sync来分享翻墙工具。

俺分享的翻墙工具的分享密钥：BG2G54AGRNPC5YXSTLQV5PNWL7ULYRIPC

用 Resilio Sync 分享翻墙工具，最大的好处是——可以绕过 GFW。只要有一个【墙内的】 Resilio Sync 节点拿到翻墙工具，那么其它的【墙内节点】也可以同步并得到。而GFW是部署在天朝的国际出口。墙内两台电脑之间的传输，不会经过 GFW。

\(二\)补充说明 1.同步下载时不需要翻墙——因为翻墙会导致你的传输速度变慢（会慢多少，取决于你用的翻墙工具）。

2.大伙儿没事儿就把你的 Resilio Sync 开着\(注：敏感人士不建议这样做\)。同时运行的节点越多，同步速度就越快。而且运行的节点越多，朝廷越难封锁。


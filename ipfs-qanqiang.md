使用 IPFS 来下载和传播翻墙软件<br>
<br>
星际文件系统（InterPlanetary File System，缩写IPFS）是一个旨在创建持久且分布式存储和共享文件的网络传输协议。它是一种内容可寻址的对等超媒体分发协议。在IPFS网络中的节点将构成一个分布式文件系统。它是一个开放源代码项目，自2014年开始由Protocol Labs在开源社区的帮助下发展。<br>
network<br>
下载IPFS：https://dist.ipfs.io/#go-ipfs<br>
从这里选择windows Binary，下载windows Binary对应的amd64版的ipfs,如果你的系统是32位的，则下载386版本<br>
如果是其它操作系统，就选择其它的下载链接。<br>
<br>
下载后，解压程序，解压到不含中文和空格的路径中，比如：D:\go-ipfs<br>
然后启动windows 命令行工具，点“开始”，在“搜索程序和文件”框中输入：cmd ，然后点击搜索结果中的：cmd.exe<br>
输入<br>
d:<br>
回车<br>
再输入：<br>
cd D:\go-ipfs<br>
回车<br>
然后再输入：<br>
ipfs init<br>
回车<br>
然后会输出结果：<br>
initializing IPFS node at C:\Users\tom\.ipfs<br>
generating 2048-bit RSA keypair...done<br>
peer identity:（一串字符串）<br>
to get started, enter:<br>
ipfs cat /ipfs/（一串字符串）/readme<br>
然后下载一些常用的翻墙软件<br>
执行命令：<br>
start ipfs daemon<br>
这样会单独启动一个窗口：<br>
窗口中最后显示：<br>
API server listening on /ip4/127.0.0.1/tcp/5001<br>
Gateway (readonly) server listening on /ip4/127.0.0.1/tcp/8080<br>
Daemon is ready<br>
<br>
 <br>
然后再执行：<br>
ipfs get /ipns/QmdRaB7BPW4hE8UdNN5M6VdCTPG6c8AN269UrqjVYc42xC -o fanqiang<br>
就会开始下载翻墙软件了，如果黑色窗口没有反应，可以按回车键迫使画面更新。<br>
画面会显示：<br>
Saving file(s) to fanqiang<br>
下面有下载的进度条，直到进度条显示100%，则下载完毕，下载的翻墙软件保存在：D:\go-ipfs\fanqiang目录下。<br>
做种传播翻墙软件<br>
如果您希望做种，帮助传播翻墙软件，大陆网友可能有安全问题，推荐海外网友来做种。<br>
执行命令：<br>
ipfs pin add -r /ipns/QmdRaB7BPW4hE8UdNN5M6VdCTPG6c8AN269UrqjVYc42xC<br>
会输出<br>
pinned QmXtNPPScbSRnY38Ns4PZYRqyGoic1hY9v4MC4r4Aeh4my recursively<br>
查看做种情况<br>
ipfs pin ls /ipfs/QmXtNPPScbSRnY38Ns4PZYRqyGoic1hY9v4MC4r4Aeh4my

# 最简单的Google BBR 一键加速VPS教程

BBR是Google的一套网络拥塞控制算法，用在VPS服务器上，可以有效减少拥堵丢包，大幅提高网络连接和翻墙速度。

目前很多Linux类系统的最新内核，都已内置BBR。所以，不再需要第三方的安装脚本了。直接修改系统配置即可。

**广告插播，如果你觉得自己折腾VPS太麻烦，可以考虑这个服务哦（非本库服务）：**  
 

本文的系统要求为 Debian 9或更高版本的 Debian Linux，其它操作系统所知不详，不知是否适合本文的方法。

注意，本文的配置参数不仅仅是启用Google BBR，还包括一系列网络参数的优化，具体的不解释了，直接拷贝执行使用即可。

Google BBR 一键加速VPS服务器很简单，SSH登录VPS后，分别执行以下2个命令即可（鼠标选中高亮后，点鼠标右键复制粘贴到root用户的\#后面，然后回车）。  


命令1\(比较长，有折行，请完整拷贝\)：  
 `wget https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/server-cfg/sysctl.conf -O -> /etc/sysctl.conf`  


> 如果提示 wget: command not found 的错误，这是你的系统精简的太干净了，wget都没有安装，所以需要安先装 wget:  
>  `apt-get install -y wget`

命令2：  
 `sysctl -p`  


执行成功后大致会输出：  
 `fs.file-max = 51200`  
 `net.ipv4.conf.lo.accept_redirects = 0`  
 `net.ipv4.conf.all.accept_redirects = 0`  
 `net.ipv4.conf.default.accept_redirects = 0`  
 `net.ipv4.ip_local_port_range = 10000 65000`  
 `net.ipv4.tcp_fin_timeout = 15`  
 `net.ipv4.tcp_fastopen = 3`  
 `net.ipv4.tcp_keepalive_time = 1200`  
 `net.ipv4.tcp_rmem = 32768 436600 873200`  
 `net.ipv4.tcp_syncookies = 1`  
 `net.ipv4.tcp_synack_retries = 2`  
 `net.ipv4.tcp_syn_retries = 2`  
 `net.ipv4.tcp_timestamps = 0`  
 `net.ipv4.tcp_max_tw_buckets = 9000`  
 `net.ipv4.tcp_max_syn_backlog = 65536`  
 `net.ipv4.tcp_mem = 94500000 91500000 92700000`  
 `net.ipv4.tcp_max_orphans = 3276800`  
 `net.ipv4.tcp_mtu_probing = 1`  
 `net.ipv4.tcp_wmem = 8192 436600 873200`  
 `net.core.netdev_max_backlog = 250000`  
 `net.core.somaxconn = 32768`  
 `net.core.wmem_default = 8388608`  
 `net.core.rmem_default = 8388608`  
 `net.core.rmem_max = 67108864`  
 `net.core.wmem_max = 67108864`  
 `net.core.default_qdisc = fq`  
 `net.ipv4.tcp_congestion_control = bbr`  


本文属于bannedbook系列翻墙教程的一部分，欢迎体验我们提供的免费翻墙软件和教程：

[安卓手机翻墙](https://github.com/bannedbook/fanqiang/wiki/%E5%AE%89%E5%8D%93%E7%BF%BB%E5%A2%99%E8%BD%AF%E4%BB%B6)

[Chrome一键翻墙包](https://github.com/bannedbook/fanqiang/wiki/Chrome%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85)

[火狐firefox一键翻墙包](https://github.com/bannedbook/fanqiang/wiki/%E7%81%AB%E7%8B%90firefox%E4%B8%80%E9%94%AE%E7%BF%BB%E5%A2%99%E5%8C%85)

[XX-Net禁闻版](https://github.com/bannedbook/xxjw)

[自建V2ray服务器简明教程](https://github.com/bannedbook/fanqiang/blob/master/v2ss/%E8%87%AA%E5%BB%BAV2ray%E6%9C%8D%E5%8A%A1%E5%99%A8%E7%AE%80%E6%98%8E%E6%95%99%E7%A8%8B.md)

版权所有，转载必须保留文章所有链接。


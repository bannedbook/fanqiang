[PAC](https://github.com/bannedbook/fanqiang/wiki/pacfq) 
=======
本项目主要介绍如何利用国外VPS搭建多协议代理服务。

[这里]https://github.com/bannedbook/fanqiang/wiki/pacfq 提供了我搭建的PAC公共代理.

搭建代理服务器
==============
推荐服务器，速度飞快，SLA可用性99.99% ： http://www.vultr.com/?ref=7036138-3B
赠送的20刀期限30天，注意使用。
注意使用我们的推荐链接（不要省略?ref=7036138-3B），才能享受到赠送20刀优惠哦。
搭配锐速，网页打开速度翻十倍，效果更加。
==============
在 25 端口搭建 http/https 代理。

Debian 7/8 （需要一行一行复制安装，推荐用此系统）:
-------
apt-get update
lsof -i:25  #查看25端口被什么程序占用
如果25端口被sendmail占用
service sendmail stop
apt-get remove sendmail
cd /etc/init.d
rm sendmail
如果25端口被exim4占用
apt-get remove exim4 exim4-base exim4-config exim4-daemon-light
rm -r /var/log/exim4/
如果25端口被Postfix占用
apt-get remove Postfix

lsof -i:25  #查看占用25端口的进程是否卸载干净

apt-get install curl
apt-get -y install squid3 
curl https://raw.githubusercontent.com/kgfw/fg/master/jwpac/debian-squid.conf > /etc/squid3/squid.conf
mkdir  /var/cache/squid
chmod 777 /var/cache/squid
/etc/init.d/squid3 stop
squid3 -z
/etc/init.d/squid3 start


Ubuntu 16.04 x64（需要一行一行复制安装，默认密码admin123）:
-------
apt-get -y install squid
curl https://raw.githubusercontent.com/kgfw/fg/master/jwpac/ubuntu-squid.conf > /etc/squid/squid.conf
echo "root:ssbiY3prCJLxU" >> /etc/squid/passwd
mkdir -p /var/cache/squid
chmod -R 777 /var/cache/squid
service squid stop
squid -z
service squid restart

CentOS 6.7 x64:
-------
setenforce 0
ulimit -n 800000
echo "* soft nofile 800000" >> /etc/security/limits.conf
echo "* hard nofile 800000" >> /etc/security/limits.conf
echo "alias net-pf-10 off" >> /etc/modprobe.d/dist.conf
echo "alias ipv6 off" >> /etc/modprobe.d/dist.conf
killall sendmail
/etc/init.d/postfix stop
chkconfig --level 2345 postfix off
yum -y install squid wget
wget https://raw.githubusercontent.com/kgfw/fg/master/jwpac/centos-squid.conf -O /etc/squid/squid.conf
echo "root:W10fM8VWO04aM" >> /etc/squid/passwd
mkdir -p /var/cache/squid
chmod -R 777 /var/cache/squid
squid -z
service squid restart
chkconfig --level 2345 squid on
iptables -t nat -F
iptables -t nat -X
iptables -t nat -P PREROUTING ACCEPT
iptables -t nat -P POSTROUTING ACCEPT
iptables -t nat -P OUTPUT ACCEPT
iptables -t mangle -F
iptables -t mangle -X
iptables -t mangle -P PREROUTING ACCEPT
iptables -t mangle -P INPUT ACCEPT
iptables -t mangle -P FORWARD ACCEPT
iptables -t mangle -P OUTPUT ACCEPT
iptables -t mangle -P POSTROUTING ACCEPT
iptables -F
iptables -X
iptables -P FORWARD ACCEPT
iptables -P INPUT ACCEPT
iptables -P OUTPUT ACCEPT
iptables -t raw -F
iptables -t raw -X
iptables -t raw -P PREROUTING ACCEPT
iptables -t raw -P OUTPUT ACCEPT
service iptables save


装完后记得reboot重启下服务器确保生效。

然后使用 [PAC](https://raw.githubusercontent.com/kgfw/fg/master/jwpac/jw.pac) 右键另存为 PAC 文件后修改其中的server1.pac.dtwang.org为你的服务器IP即可。

推荐服务器，速度飞快，SLA可用性99.99% ： http://www.vultr.com/?ref=7036138-3B
赠送的20刀期限30天，注意使用。
注意使用我们的推荐链接（不要省略?ref=7036138-3B），才能享受到赠送20刀优惠哦。
搭配锐速，网页打开速度翻十倍，效果更加。

转载注明出处：https://github.com/bannedbook/fanqiang/wiki/pacfq


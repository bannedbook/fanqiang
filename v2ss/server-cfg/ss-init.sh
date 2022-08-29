apt-get update
apt-get install curl wget -y
iptables -A OUTPUT -p tcp --dport 25 -j REJECT
apt-get install -y iptables-persistent
wget https://raw.githubusercontent.com/bannedbook/blockporn/master/hosts.txt  -O ->> /etc/hosts
echo '* soft nofile 51200' >> /etc/security/limits.conf
echo '* hard nofile 51200' >> /etc/security/limits.conf
ulimit -n 51200
cp -f /usr/share/zoneinfo/Asia/Shanghai /etc/localtime
wget https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/server-cfg/sysctl.conf  -O -> /etc/sysctl.conf
sysctl -p
wget https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/server-cfg/resolv.conf  -O -> resolv.conf
cp resolv.conf /etc/resolv.conf
wget --no-check-certificate -O shadowsocks-all.sh https://raw.githubusercontent.com/bannedbook/shadowsocks_install/master/shadowsocks-all.sh
chmod +x shadowsocks-all.sh
./shadowsocks-all.sh 2>&1 | tee shadowsocks-all.log
wget https://github.com/shadowsocks/v2ray-plugin/releases/download/v1.3.0/v2ray-plugin-linux-amd64-v1.3.0.tar.gz
tar zxvf v2ray-plugin-linux-amd64-v1.3.0.tar.gz
cp v2ray-plugin_linux_amd64 /usr/bin/v2ray-plugin
apt-get -y install nginx
wget https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/server-cfg/ss_nginx_check.sh  -O -> /etc/shadowsocks-libev/ss_nginx_check.sh
chmod +x /etc/shadowsocks-libev/ss_nginx_check.sh
wget https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/server-cfg/ss-crontab.txt  -O -> /var/spool/cron/crontabs/root

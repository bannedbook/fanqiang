#!/usr/bin/env bash
set -x
#trap read debug
apt-get update
apt-get install curl wget -y
curl -L -s https://raw.githubusercontent.com/v2fly/fhs-install-v2ray/master/install-release.sh | bash
curl -L -s https://raw.githubusercontent.com/v2fly/fhs-install-v2ray/master/install-dat-release.sh | bash
systemctl enable v2ray
iptables -A OUTPUT -p tcp --dport 25 -j REJECT
hostbakfile=/etc/hosts_bak
if [ -f "$hostbakfile" ]; then
    cp -f /etc/hosts_bak /etc/hosts
else 
    cp -f /etc/hosts /etc/hosts_bak
fi
wget https://raw.githubusercontent.com/bannedbook/blockporn/master/hosts.txt  -O ->> /etc/hosts
echo '* soft nofile 65535' >> /etc/security/limits.conf
echo '* hard nofile 65535' >> /etc/security/limits.conf
ulimit -n 65535
wget https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/server-cfg/sysctl.conf  -O -> /etc/sysctl.conf
sysctl -p
wget https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/server-cfg/v2ray.service  -O -> /etc/systemd/system/v2ray.service
systemctl daemon-reload
timedatectl set-timezone Asia/Singapore
wget https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/server-cfg/resolv.conf  -O -> resolv.conf
cp resolv.conf /etc/resolv.conf
#below need step to step
apt-get install -y iptables-persistent
#apt-get -y install nginx
#above need step to step
wget https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/server-cfg/v2ss-nginx.conf  -O -> /etc/nginx/nginx.conf
#nginx -t
#nginx -s reload
service sendmail stop
apt -y remove exim4 exim4-base exim4-config exim4-daemon-light
apt -y remove Postfix
apt -y remove  sendmail
rm /etc/init.d/sendmail
/usr/sbin/service v2ray restart
/sbin/service v2ray restart
wget https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/server-cfg/v2ray_check.sh  -O -> /etc/v2ray/v2ray_check.sh
chmod +x /etc/v2ray/v2ray_check.sh
cat /etc/v2ray/v2ray_check.sh
wget https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/server-cfg/crontab.txt  -O -> /var/spool/cron/crontabs/root
#crontab -e to active cron



apt-get update
apt-get install curl wget -y
iptables -A OUTPUT -p tcp --dport 25 -j REJECT
apt-get install -y iptables-persistent
wget https://raw.githubusercontent.com/bannedbook/blockporn/master/hosts.txt  -O ->> /etc/hosts
echo '* soft nofile 51200' >> /etc/security/limits.conf
echo '* hard nofile 51200' >> /etc/security/limits.conf
ulimit -n 51200
wget https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/server-cfg/sysctl.conf  -O -> /etc/sysctl.conf
sysctl -p
bash <(curl -L -s https://install.direct/go.sh)
wget https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/server-cfg/v2ray.service  -O -> /etc/systemd/system/v2ray.service
systemctl daemon-reload
cp -f /usr/share/zoneinfo/Asia/Shanghai /etc/localtime
wget https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/server-cfg/resolv.conf  -O -> resolv.conf
cp resolv.conf /etc/resolv.conf
service v2ray start
apt-get -y install nginx
wget https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/server-cfg/v2ss-nginx.conf  -O -> /etc/nginx/nginx.conf
nginx -t
nginx -s reload
wget https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/server-cfg/v2ray_check.sh  -O -> /etc/v2ray/v2ray_check.sh
chmod +x /etc/v2ray/v2ray_check.sh
cat /etc/v2ray/v2ray_check.sh
wget https://raw.githubusercontent.com/bannedbook/fanqiang/master/v2ss/server-cfg/crontab.txt  -O -> /var/spool/cron/crontabs/root
/etc/init.d/cron restart


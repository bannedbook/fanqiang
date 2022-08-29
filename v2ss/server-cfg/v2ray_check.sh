DATE=`date -d "today" +"%Y-%m-%d-%H:%M:%S"`
MM=`ps -ef |grep /usr/bin/v2ray/v2ray |grep -v grep |wc -l`
if [ $MM -eq 0 ]; then 
/usr/sbin/service v2ray restart
/sbin/service v2ray restart
echo "$DATE: The v2ray is down and restart" >> /var/log/v2ray_check.log 
else 
echo "$DATE: The v2ray is ok" >> /dev/null
fi

MM=`ps -ef |grep nginx |grep -v grep |wc -l`
if [ $MM -eq 0 ]; then 
/usr/sbin/nginx
echo "$DATE: The nginx is down and restart" >> /var/log/ss_nginx_check.log 
else 
echo "$DATE: The nginx is ok" >> /dev/null
fi
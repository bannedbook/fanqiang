DATE=`date -d "today" +"%Y-%m-%d-%H:%M:%S"`
MM=`ps -ef |grep /usr/bin/v2ray/v2ray |grep -v grep |wc -l`
if [ "$MM" == "0" ]; then 
/sbin/reboot
/usr/sbin/reboot
echo "$DATE: The v2ray is down and restart" >> /var/log/v2ray_check.log 
else 
echo "$DATE: The v2ray is ok" >> /dev/null
fi

MM=`ps -ef |grep /usr/sbin/nginx |grep -v grep |wc -l`
if [ "$MM" == "0" ]; then 
/sbin/reboot
/usr/sbin/reboot
echo "$DATE: The nginx is down and restart" >> /var/log/ss_nginx_check.log 
else 
echo "$DATE: The nginx is ok" >> /dev/null
fi
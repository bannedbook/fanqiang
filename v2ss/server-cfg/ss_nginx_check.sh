DATE=`date -d "today" +"%Y-%m-%d-%H:%M:%S"`
MM=`ps -ef |grep /usr/local/bin/ss-server |grep -v grep |wc -l`
if [ $MM -eq 0 ]; then 
/sbin/reboot
/usr/sbin/reboot
echo "$DATE: The ss-server is down and restart" >> /var/log/ss_nginx_check.log 
else 
echo "$DATE: The ss-server is ok" >> /dev/null
fi

MM=`ps -ef |grep nginx |grep -v grep |wc -l`
if [ $MM -eq 0 ]; then 
/usr/sbin/nginx
echo "$DATE: The nginx is down and restart" >> /var/log/ss_nginx_check.log 
else 
echo "$DATE: The nginx is ok" >> /dev/null
fi
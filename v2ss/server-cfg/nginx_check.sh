DATE=`date -d "today" +"%Y-%m-%d-%H:%M:%S"`
MM=`ps -ef |grep nginx |grep -v grep |wc -l`
if [ $MM -eq 0 ]; then 
/usr/sbin/nginx
echo "$DATE: The nginx is down and restart" >> /var/log/nginx_check.log 
else 
echo "$DATE: The nginx is ok" >> /var/log/nginx_check.log 
fi

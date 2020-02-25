DATE=`date -d "today" +"%Y-%m-%d-%H:%M:%S"`
MM=`ps -ef |grep /usr/bin/v2ray/v2ray |grep -v grep |wc -l`
if [ "$MM" == "0" ]; then 
/usr/sbin/reboot
echo "$DATE: The v2ray is down and restart" >> /var/log/v2ray_check.log 
else 
echo "$DATE: The v2ray is ok" >> /dev/null
fi

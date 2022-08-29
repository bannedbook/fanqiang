DATE=`/usr/bin/date -d "today" +"%Y-%m-%d-%H:%M:%S"`
MM=`ps -ef |grep /usr/local/bin/xray |grep -v grep |wc -l`
echo -n "$DATE : $MM "
if [ $MM -eq 0 ]; then 
/bin/systemctl restart au
echo "The xray is down and restart"
else 
echo "The xray is ok"
fi
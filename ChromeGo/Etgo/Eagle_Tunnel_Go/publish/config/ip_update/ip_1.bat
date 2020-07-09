@Echo Off
Title 从GitHub云端更新 Trojan 最新配置
cd /d %~dp0
..\..\..\..\..\wget --no-check-certificate https://gitlab.com/free9999/ipupdate/-/raw/master/etgo/client.conf

if exist client.conf goto startcopy
echo ip更新失败，请试试ip_2更新
pause
exit
:startcopy

del "..\client.conf_backup"
ren "..\client.conf"  client.conf_backup
copy /y "%~dp0client.conf" ..\client.conf
del "%~dp0client.conf"

ECHO.&ECHO.已更新完成最新可用etgo配置,请按回车键或空格键启动程序！ &PAUSE >NUL 2>NUL
exit
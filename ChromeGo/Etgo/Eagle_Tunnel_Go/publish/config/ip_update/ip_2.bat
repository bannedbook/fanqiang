@Echo Off
Title 从Coding.net云端更新 Trojan 最新配置
cd /d %~dp0
..\..\..\..\..\wget --no-check-certificate https://cdn.jsdelivr.net/gh/Alvin9999/pac2@latest/etgo/client.conf

if exist client.conf goto startcopy
echo ip更新失败，请试试另外一个，如果都不行，请反馈kebi2014@gmail.com
pause
exit
:startcopy

del "..\client.conf_backup"
ren "..\client.conf"  client.conf_backup
copy /y "%~dp0client.conf" ..\client.conf
del "%~dp0client.conf"

ECHO.&ECHO.已更新完成最新可用etgo配置,请按回车键或空格键启动程序！ &PAUSE >NUL 2>NUL
exit
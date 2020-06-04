@Echo Off
Title 从GitHub云端更新 Trojan 最新配置
cd /d %~dp0
..\..\wget --ca-certificate=ca-bundle.crt -c https://gitlab.com/free9999/ipupdate/-/raw/master/SSROT/config.json

if exist config.json goto startcopy
echo ip更新失败，请试试ip_2更新
pause
exit
:startcopy

del "..\config.json_backup"
ren "..\config.json"  config.json_backup
copy /y "%~dp0config.json" ..\config.json
del "%~dp0config.json"

ECHO.&ECHO.已更新完成最新SSROT配置,请按回车键或空格键启动程序！ &PAUSE >NUL 2>NUL
exit
@Echo Off
Title 从GitHub云端更新 Trojan 最新配置
cd /d %~dp0
..\..\wget --ca-certificate=ca-bundle.crt -c https://gitlab.com/free9999/ipupdate/-/raw/master/trojan/config.json
..\..\wget --ca-certificate=ca-bundle.crt -c https://gitlab.com/free9999/ipupdate/-/raw/master/trojan/private.crt

if exist config.json goto startcopy
echo ip更新失败，请试试ip_2更新
pause
exit
:startcopy

del "..\config.json_backup"
ren "..\config.json"  config.json_backup
copy /y "%~dp0config.json" ..\config.json
del "%~dp0config.json"
ECHO.&ECHO.已更新IP配置信息～接下来更新证书文件～

del "..\private.crt_backup"
ren "..\private.crt"  private.crt_backup
copy /y "%~dp0private.crt" ..\private.crt
del "%~dp0private.crt"
ECHO.&ECHO.已更新证书文件～

ECHO.&ECHO.已更新完成最新可用trojan配置,请按回车键或空格键启动程序！ &PAUSE >NUL 2>NUL
exit
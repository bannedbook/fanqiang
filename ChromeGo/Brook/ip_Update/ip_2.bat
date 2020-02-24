@Echo Off
Title 从COD云端更新 brook 最新配置
cd /d %~dp0
..\..\wget --ca-certificate=ca-bundle.crt -c https://cdn.jsdelivr.net/gh/Alvin9999/PAC@latest/config.ini

if exist config.ini goto startcopy
echo ip更新失败，请试试ip_1更新
pause
exit
:startcopy

del "..\config.ini_backup"
ren "..\config.ini"  config.ini_backup
copy /y "%~dp0config.ini" ..\config.ini
del "%~dp0config.ini"
ECHO.&ECHO.已更新完成最新可用brook配置,请按任意键退出,并重启程序. &PAUSE >NUL 2>NUL
exit
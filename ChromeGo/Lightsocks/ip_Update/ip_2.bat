@Echo Off
Title 从COD云端更新 Lightsocks 最新配置
cd /d %~dp0
..\..\wget --ca-certificate=ca-bundle.crt -c https://cdn.jsdelivr.net/gh/Alvin9999/PAC@latest/lightsocks2/config.ini

if exist config.ini goto startcopy
echo ip更新失败，请试试另外一个，如果都不行，请反馈kebi2014@gmail.com
pause
exit
:startcopy

del "..\config.ini_backup"
ren "..\config.ini"  config.ini_backup
copy /y "%~dp0config.ini" ..\config.ini
del "%~dp0config.ini"
ECHO.&ECHO.已更新完成最新可用Lightsocks配置，请按任意键，软件会自动启动浏览器和代理软件. &PAUSE >NUL 2>NUL
exit

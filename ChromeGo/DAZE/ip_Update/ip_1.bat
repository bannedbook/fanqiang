@Echo Off
Title 从GitHub云端更新 DAZE 最新配置
cd /d %~dp0
..\..\wget --ca-certificate=ca-bundle.crt -c https://gitlab.com/free9999/ipupdate/-/raw/master/DAZE/daze.bat

if exist daze.bat goto startcopy
echo ip更新失败，请试试ip_2更新
pause
exit
:startcopy

del "..\daze.bat_backup"
ren "..\daze.bat"  daze.bat_backup
copy /y "%~dp0daze.bat" ..\daze.bat
del "%~dp0daze.bat"
ECHO.&ECHO.已更新完成最新可用DAZE配置,请按回车键或空格键启动程序！ &PAUSE >NUL 2>NUL
exit
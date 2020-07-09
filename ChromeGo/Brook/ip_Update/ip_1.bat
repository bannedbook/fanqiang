@Echo Off
Title 从GitHub云端更新 brook 最新配置
cd /d %~dp0
..\..\wget --no-check-certificate https://gitlab.com/free9999/ipupdate/-/raw/master/brook/brook.bat

if exist brook.bat goto startcopy
echo ip更新失败，请试试ip_2更新
pause
exit
:startcopy

del "..\brook.bat_backup"
ren "..\brook.bat"  brook.bat_backup
copy /y "%~dp0brook.bat" ..\brook.bat
del "%~dp0brook.bat"
ECHO.&ECHO.已更新完成最新brook配置,请按回车键或空格键启动程序！ &PAUSE >NUL 2>NUL
exit
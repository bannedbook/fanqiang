@Echo Off
Title 从GitHub云端更新 Goflyway 最新配置
cd /d %~dp0
..\..\wget --no-check-certificate https://gitlab.com/free9999/ipupdate/-/raw/master/goflyway/goflyway.bat

if exist goflyway.bat goto startcopy
echo ip更新失败，请试试其它ip更新
pause
exit
:startcopy

del "..\goflyway.bat_backup"
ren "..\goflyway.bat"  goflyway.bat_backup
copy /y "%~dp0goflyway.bat" ..\goflyway.bat
del "%~dp0goflyway.bat"
ECHO.&ECHO.已更新完成最新Goflyway配置,请按回车键或空格键启动程序！ &PAUSE >NUL 2>NUL
exit
@Echo Off
Title 从COD云端更新 Goflyway 最新配置
cd /d %~dp0
..\..\wget --ca-certificate=ca-bundle.crt -c https://cdn.jsdelivr.net/gh/Alvin9999/pac2@latest/goflyway.ini

if exist goflyway.ini goto startcopy
echo ip更新失败，请试试ip_1更新
pause
exit
:startcopy

del "..\goflyway.bat_backup"
ren "..\goflyway.bat"  goflyway.bat_backup
copy /y "%~dp0goflyway.ini" ..\goflyway.bat
del "%~dp0goflyway.ini"
ECHO.&ECHO.已更新完成最新可用Goflyway配置,请按回车键或空格键启动程序！ &PAUSE >NUL 2>NUL
exit
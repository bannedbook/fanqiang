@Echo Off
Title 从COD云端更新 brook 最新配置
cd /d %~dp0
..\..\wget --no-check-certificate https://cdn.jsdelivr.net/gh/Alvin9999/PAC@latest/brook/brook.ini

if exist brook.ini goto startcopy
echo ip更新失败，请试试ip_1更新
pause
exit
:startcopy

del "..\brook.bat_backup"
ren "..\brook.bat"  brook.bat_backup
copy /y "%~dp0brook.ini" ..\brook.bat
del "%~dp0brook.ini"
ECHO.&ECHO.已更新完成最新brook配置,请按回车键或空格键启动程序！ &PAUSE >NUL 2>NUL
exit
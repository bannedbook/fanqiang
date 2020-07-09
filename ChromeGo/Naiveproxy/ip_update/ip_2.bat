@Echo Off
Title 从Coding.net云端更新 naiveproxy 最新配置
cd /d %~dp0
..\..\wget --no-check-certificate https://cdn.jsdelivr.net/gh/Alvin9999/PAC@latest/naiveproxy/config.json

if exist config.json goto startcopy
echo ip更新失败，请试试另外一个，如果都不行，请反馈kebi2014@gmail.com
pause
exit
:startcopy

del "..\config.json_backup"
ren "..\config.json"  config.json_backup
copy /y "%~dp0config.json" ..\config.json
del "%~dp0config.json"

ECHO.&ECHO.已更新完成最新naiveproxy配置,请按回车键或空格键启动程序！ &PAUSE >NUL 2>NUL
exit
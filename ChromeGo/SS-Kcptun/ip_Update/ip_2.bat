@Echo Off
Title 从GitHub云端更新 SS 配置文件
cd /d %~dp0
..\..\wget --no-check-certificate https://cdn.jsdelivr.net/gh/Alvin9999/pac2@latest/SS-Kcptun/ssconfig.txt

if exist ssconfig.txt goto startcopy
echo ip更新失败，请试试另外一个，如果都不行，请反馈kebi2014@gmail.com
pause
exit
:startcopy

del "..\gui-config.json_backup"
ren "..\gui-config.json"  gui-config.json_backup
b64 -d ssconfig.txt gui-config.json
copy /y "%~dp0gui-config.json" ..\gui-config.json
del "%~dp0ssconfig.txt"
del "%~dp0gui-config.json"
ECHO.&ECHO.已更新SS配置文件,请按回车键或空格键启动程序！ &PAUSE >NUL 2>NUL
exit
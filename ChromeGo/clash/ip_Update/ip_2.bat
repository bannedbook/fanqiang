@Echo Off
Title ip2云端更新 clash 最新配置
cd /d %~dp0
..\..\wget --no-check-certificate https://raw.githubusercontent.com/fqnews/bnews/master/baitai/20200329/1302338.md

if exist 1302338.md goto startcopy
echo ip更新失败，请试试ip_1更新
pause
exit
:startcopy

del "..\config.yaml_backup"
ren "..\config.yaml"  config.yaml_backup
copy /y "%~dp01302338.md" ..\config.yaml
del "%~dp01302338.md"
ECHO.&ECHO.已更新完成最新clash配置,请按回车键或空格键启动程序！ &PAUSE >NUL 2>NUL
exit
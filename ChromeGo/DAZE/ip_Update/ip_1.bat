@Echo Off
setlocal enableDelayedExpansion
Title 从GitHub云端更新 DAZE 最新配置
cd /d %~dp0

set filename=daze.bat
..\..\wget -t 3 --ca-certificate=ca-bundle.crt  https://killgcd.github.io/FirefoxFQ/FirefoxFQ/DAZE/daze.bat
rem 检查文件是否存在，不存在则下载失败，存在则下载成功
if exist %filename% goto startcopy

set filename=daze.bat
echo download ip1 failed,try download ip2 ...
..\..\wget -t 1 --ca-certificate=ca-bundle.crt https://raw.githubusercontent.com/killgcd/FirefoxFQ/master/FirefoxFQ/DAZE/daze.bat
if exist %filename% goto startcopy

rem 2次下载都失败，则提示用户反馈并退出
echo ip更新失败，请帮我们反馈，谢谢！反馈邮箱kebi2014@gmail.com
pause
exit

:startcopy
del "..\daze.bat_backup"
ren "..\daze.bat"  daze.bat_backup
copy /y "%~dp0%filename%" ..\daze.bat
del "%~dp0%filename%"
ECHO.&ECHO.已更新完成最新可用DAZE配置,请按任意键退出,并重启程序. &PAUSE >NUL 2>NUL
exit
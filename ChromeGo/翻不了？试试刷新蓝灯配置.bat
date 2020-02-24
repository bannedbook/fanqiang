@echo off
echo "请关闭浏览器并退出蓝灯,然后按回车键继续..."
pause
del /F %appdata%\Lantern\*
exit

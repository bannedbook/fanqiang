@echo off
SET PYTHONPATH=
SET PYTHONHOME=
echo 由于封锁严重，因此可能需要数分钟到数小时的初始化IP扫描，方能正常运行。如果刚启动时打不开网页，请耐心等待初始化IP扫描，运行一段时间后，使用效果会更好...
echo ---
echo 退出时，先关闭浏览器，然后在xxnet dos窗口按Ctrl+C键或者右键点击托盘区XXNET图标，然后点退出。不要直接关闭xxnet dos窗口，否则程序会自动重启。
"%~dp0%start.vbs" console
exit

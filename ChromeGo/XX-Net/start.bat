@echo off
SET PYTHONPATH=
SET PYTHONHOME=
echo 由于封锁严重，因此可能需要数分钟到数小时的初始化IP扫描，方能正常运行。如果刚启动时打不开网页，请耐心等待初始化IP扫描，运行一段时间后，使用效果会更好...
"%~dp0%start.vbs" console
exit

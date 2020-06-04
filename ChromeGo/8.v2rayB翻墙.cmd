%%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a %%a 
cls
@echo off
%1 start "" mshta vbscript:CreateObject("Shell.Application").ShellExecute("cmd.exe","/c %~s0 ::","","runas",1)(window.close)&&exit
CD /D "%~dp0"
start "" "%~dp0v2rayB\v2rayN.exe"
IF EXIST %~dp0Browser\chrome.exe (
    start %~dp0Browser\chrome.exe --user-data-dir=%~dp0chrome-user-data --proxy-server="socks5://127.0.0.1:10808" --host-resolver-rules="MAP * ~NOTFOUND , EXCLUDE 127.0.0.1" https://www.bannedbook.org/bnews/fq/?utm_source=chgo-v2ray
) ELSE (
	%SystemRoot%\System32\reg.exe query "HKLM\Software\Microsoft\Windows\CurrentVersion\App Paths\chrome.exe" >nul 2>&1
	IF  not errorlevel 1 (
    start chrome.exe --user-data-dir=%~dp0chrome-user-data  --proxy-server="socks5://127.0.0.1:10808" --host-resolver-rules="MAP * ~NOTFOUND , EXCLUDE 127.0.0.1"  https://www.bannedbook.org/bnews/fq/?utm_source=chgo-v2ray
	) else (
		echo Chrome浏览器不存在或没有正确安装，请尝试重新安装Chrome浏览器
		echo 或者采用以下办法：
		echo 右键点桌面的Google Chrome图标，再点属性，找到chrome.exe文件的路径，然后打开那个目录，把chrome.exe 连同那个目录下的所有子文件夹和文件，一起拷贝到ChromeGo文件夹下的Browser目录里面，然后重新启动ChromeGo的翻墙脚本。
	)
)
echo ------注意注意注意，必读必读必读------
echo 1、启动后如果Chrome页面打不开，点桌面右下角托盘区蓝色V图标，打开V2rayN软件；
echo 2、然后在V2rayN主界面，按Ctrl+A键选中所有服务器，然后点右键，再点：测试服务器真连接延迟（多选）
echo 3、测试完毕后，测试结果为数字的是可用服务器，测试结果数字越小越快,点测试结果列头可排序;
echo 4、鼠标点击选中一个可用服务器，按回车键激活，然后回到已打开的chrome浏览器刷新页面；如果还打不开页面可从最快到最慢的服务器挨个尝试；
echo 5、如果第4步挨个测试所有可用服务器都不行，那么点V2rayN左上角“订阅/更新订阅”，然后重复第2-4步。
echo ------注意注意注意，必读必读必读------
pause
%1 start "" mshta vbscript:CreateObject("Shell.Application").ShellExecute("cmd.exe","/c %~s0 ::","","runas",1)(window.close)&&exit
CD /D "%~dp0"
git config --global core.autocrlf true
git pull origin master
git add -A
git commit -m "update"
git push origin master
pause
@echo off
echo Get Admin
::ver|findstr "[6,10]\.[0-9]\.[0-9][0-9]*" > nul && (goto Main)
::ver|findstr "[3-5]\.[0-9]\.[0-9][0-9]*" > nul && (goto isBelowNT6)

:: :isBelowNT6

:Main
@echo off
cd /d "%~dp0"
cacls.exe "%SystemDrive%\System Volume Information" >nul 2>nul
if %errorlevel%==0 goto Admin
if exist "%temp%\getadmin.vbs" del /f /q "%temp%\getadmin.vbs"
echo Set RequestUAC = CreateObject^("Shell.Application"^)>"%temp%\getadmin.vbs"
echo RequestUAC.ShellExecute "%~s0","","","runas",1 >>"%temp%\getadmin.vbs"
echo WScript.Quit >>"%temp%\getadmin.vbs"
"%temp%\getadmin.vbs" /f
if exist "%temp%\getadmin.vbs" del /f /q "%temp%\getadmin.vbs"
exit

:Admin

netsh interface teredo set state disable
netsh interface 6to4 set state disabled
netsh interface isatap set state disabled

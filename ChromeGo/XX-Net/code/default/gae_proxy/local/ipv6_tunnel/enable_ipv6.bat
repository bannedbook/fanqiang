:: XX-Net
:: Enable IPV6
:: https://github.com/XX-net/XX-Net-dev/issues/53

:: https://www.zhihu.com/question/34541107/answer/137174053
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
@echo off


sc config RpcEptMapper start= auto
sc start RpcEptMapper

sc config DcomLaunch start= auto
sc start DcomLaunch

sc config RpcSs start= auto
sc start RpcSs

sc config nsi start= auto
sc start nsi

sc config Winmgmt start= auto
sc start Winmgmt

sc config Dhcp start= auto
sc start Dhcp

sc config WinHttpAutoProxySvc start= auto
sc start WinHttpAutoProxySvc

sc config iphlpsvc start= auto
sc start iphlpsvc

:: Reset IPv6
netsh interface ipv6 reset

:: Reset Group Policy Teredo
..\..\..\python27\1.0\pythonw.exe win_reset_gp.py

netsh interface teredo set state type=enterpriseclient servername=teredo.remlab.net.

:: Keep teredo interface route (not needed with reset?)
:: route DELETE ::/0
:: netsh interface ipv6 add route ::/0 "Teredo Tunneling Pseudo-Interface"

:: Set IPv6 prefixpolicies
:: See https://tools.ietf.org/html/rfc3484
:: 2002::/16 6to4 tunnel
:: 2001::/32 teredo tunnel; not default
netsh interface ipv6 add prefixpolicy ::1/128 50 0
netsh interface ipv6 set prefixpolicy ::1/128 50 0
netsh interface ipv6 add prefixpolicy ::/0 40 1
netsh interface ipv6 set prefixpolicy ::/0 40 1
netsh interface ipv6 add prefixpolicy 2002::/16 30 2
netsh interface ipv6 set prefixpolicy 2002::/16 30 2
netsh interface ipv6 add prefixpolicy 2001::/32 25 5
netsh interface ipv6 set prefixpolicy 2001::/32 25 5
netsh interface ipv6 add prefixpolicy ::/96 20 3
netsh interface ipv6 set prefixpolicy ::/96 20 3
netsh interface ipv6 add prefixpolicy ::ffff:0:0/96 10 4
netsh interface ipv6 set prefixpolicy ::ffff:0:0/96 10 4

:: Fix look up AAAA on teredo
:: http://technet.microsoft.com/en-us/library/bb727035.aspx
:: http://ipv6-or-no-ipv6.blogspot.com/2009/02/teredo-ipv6-on-vista-no-aaaa-resolving.html
Reg add HKLM\SYSTEM\CurrentControlSet\services\Dnscache\Parameters /v AddrConfigControl /t REG_DWORD /d 0 /f

:: Enable all IPv6 parts
Reg add HKLM\SYSTEM\CurrentControlSet\Services\Tcpip6\Parameters /v DisabledComponents /t REG_DWORD /d 0 /f


ipconfig /flushdns

set time=%date:~0,4%-%date:~5,2%-%date:~8,2%_%time:~0,2%%time:~3,2%%time:~6,2%
@call :output>..\..\..\..\..\data\gae_proxy\ipv6-state%time%.txt

@echo Over
@echo Reboot system at first time!
exit

:output
@echo off
ipconfig /all
netsh interface ipv6 show teredo
netsh interface ipv6 show route
netsh interface ipv6 show interface
netsh interface ipv6 show prefixpolicies
netsh interface ipv6 show address
route print
notepad ..\..\..\..\..\data\gae_proxy\ipv6-state%time%.txt

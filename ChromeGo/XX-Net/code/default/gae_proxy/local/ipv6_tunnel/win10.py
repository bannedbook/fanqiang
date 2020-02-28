#!/usr/bin/env python2
# coding:utf-8

import os
import sys
import time
import socket
import platform
from .common import *
from . import win32runas
from .pteredor import local_ip_startswith

current_path = os.path.dirname(os.path.abspath(__file__))
python_path = os.path.abspath( os.path.join(current_path, os.pardir, os.pardir, 'python27', '1.0'))
root_path = os.path.abspath(os.path.join(current_path, os.pardir, os.pardir))
data_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir, 'data', "gae_proxy"))
if not os.path.isdir(data_path):
    data_path = current_path

if __name__ == "__main__":
    noarch_lib = os.path.abspath( os.path.join(python_path, 'lib', 'noarch'))
    sys.path.append(noarch_lib)

    if sys.platform == "win32":
        win32_lib = os.path.abspath( os.path.join(python_path, 'lib', 'win32'))
        sys.path.append(win32_lib)


# TODO：Help to reset ipv6 if system Teredo Interface not exist
# download http://download.microsoft.com/download/3/F/3/3F3CA0F7-2FAF-4C51-8DDF-3516B4D91975/MicrosoftEasyFix20164.mini.diagcab


# TODO: Win7 need to change the interface name as the real one.
# can use ifconfig to get it.

# TODO; Win10 Home and Win10 Profession is different.

enable_ipv6_temp = os.path.join(current_path, 'enable_ipv6_temp.bat')
disable_ipv6_temp = os.path.join(current_path, 'disable_ipv6_temp.bat')
set_best_server_temp = os.path.join(current_path, 'set_best_server_temp.bat')

enable_cmds = """
@echo Starting...
@set log_file="{}"

@echo Config servers...
@call:[config servers]>>%log_file%

@echo Reset IPv6...
@call:[reset ipv6]>>%log_file%

@echo Set IPv6 Tunnel...
@call:[set ipv6]>>%log_file%

@call:[print state]>>%log_file%

@echo Over
@echo Reboot system at first time!
@pause
exit


:[config servers]
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

goto :eof


:[reset ipv6]
netsh interface ipv6 reset
ipconfig /flushdns
goto :eof


:[set ipv6]
:: Reset Group Policy Teredo
SET PYTHONPATH=
SET PYTHONHOME=
"{}" "{}\\win_reset_gp.py"

""".format(log_file, sys.executable, current_path) + \
"""
netsh interface teredo set state type={} servername={}.

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

goto :eof


:[print state]
:: Show state
ipconfig /all
netsh interface ipv6 show teredo
netsh interface ipv6 show route
netsh interface ipv6 show interface
netsh interface ipv6 show prefixpolicies
netsh interface ipv6 show address
route print
goto :eof
"""


disable_cmds="""
netsh interface teredo set state disable
netsh interface 6to4 set state disabled
netsh interface isatap set state disabled
"""

has_admin = win32runas.is_admin()

# Use this if need admin
# Don't hide the console window
def elevate(script_path, clear_log=True):
    global script_is_running
    if not script_is_running:
        script_is_running = True

        if clear_log and os.path.isfile(log_file):
            try:
                os.remove(log_file)
            except Exception as e:
                xlog.warn("remove %s fail:%r", log_file, e)

        try:
            win32runas.runas(None, script_path)
            return True
        except Exception as e:
            xlog.warning('elevate e:%r', e)
        finally:
            script_is_running = False

script_is_running = False
last_get_state_time = 0
last_set_server_time = 0
last_state = "unknown"

client_ext = 'natawareclient' if platform.version()[0] > '6' else 'enterpriseclient'

def client_type():
    try:
        ip = [line for line in run("route print").split("\r\n") if " 0.0.0.0 " in line][0].split()[-2]
    except:
        xlog.warning('route setting may wrong, please check.')
        return 'client'
    return client_ext if ip.startswith(local_ip_startswith) else 'client'


def get_teredo_interface():
    r = run("ipconfig /all")
    return "Developing"


def state():
    global last_get_state_time, last_state
    if time.time() - last_get_state_time < 5:
        return last_state

    last_get_state_time = time.time()
    r = run("netsh interface teredo show state")
    xlog.debug("netsh state: %s", r)
    type = get_line_value(r, 2)
    if type == "disabled":
        last_state = "disabled"
    else:
        last_state = get_line_value(r, 6) or "unknown"
        if "probe" in last_state:
            last_state = "probe"

    return last_state


def state_pp():
    return "Developing"


def switch_pp():
    return "Developing"


def enable(is_local=False):
    if not is_local:
        return "Please operating on local host."

    if script_is_running or pteredor_is_running:
        return "Script is running, please retry later."
    else:
        new_enable_cmds = enable_cmds.format(client_type(), best_server())
        with open(enable_ipv6_temp, 'w') as fp:
            fp.write(new_enable_cmds)
        done = elevate(enable_ipv6_temp, False)

        if done:
            global last_set_server_time
            last_set_server_time = time.time()
            return "IPv6 tunnel is enabled, please reboot system."
        else:
            return "Enable IPv6 tunnel fail, you must authorized as admin."


def disable(is_local=False):
    if not is_local:
        return "Please operating on local host."

    if script_is_running or pteredor_is_running:
        return "Script is running, please retry later."
    else:
        with open(disable_ipv6_temp, 'w') as fp:
            fp.write(disable_cmds)
        done = elevate(disable_ipv6_temp)

        if done:
            return "IPv6 tunnel is disabled."
        else:
            return "Disable IPv6 tunnel fail, you must authorized as admin."


def set_best_server(is_local=False):
    # Allow remote if has admin
    if not is_local and not has_admin:
        return "Please operating on local host."

    if script_is_running or pteredor_is_running:
        return "Script is running, please retry later."
    else:
        global last_set_server_time
        now = time.time()
        wait_time = 3 - (now - last_set_server_time) // 60
        if wait_time > 0:
            return "Don't do this repeated, please retry in %d minutes later." % wait_time

        set_server_cmds = ("netsh interface teredo set state %s %s. default default default"
                           % (client_type(), best_server()))
        with open(set_best_server_temp, 'w') as fp:
            fp.write(set_server_cmds)
        done = elevate(set_best_server_temp, False)


        if done:
            last_set_server_time = now
            return "Set teredo server is completed."
        else:
            return "Set teredo server fail, you must authorized as admin."


if __name__ == '__main__':
    enable(True)

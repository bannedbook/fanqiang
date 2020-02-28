#!/usr/bin/env python
"""A simple crossplatform autostart helper"""
from __future__ import with_statement

import os
import sys

from xlog import getLogger
xlog = getLogger("launcher")

current_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.abspath(os.path.join(current_path, os.pardir))
top_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir))

if sys.platform == 'win32':
    import _winreg
    _registry = _winreg.ConnectRegistry(None, _winreg.HKEY_CURRENT_USER)

    def get_runonce():
        return _winreg.OpenKey(_registry, r"Software\Microsoft\Windows\CurrentVersion\Run", 0, _winreg.KEY_ALL_ACCESS)

    def add(name, application):
        """add a new autostart entry"""
        key = get_runonce()
        _winreg.SetValueEx(key, name, 0, _winreg.REG_SZ, application)
        _winreg.CloseKey(key)

    def exists(name):
        """check if an autostart entry exists"""
        key = get_runonce()
        exists = True
        try:
            _winreg.QueryValueEx(key, name)
        except:  # WindowsError
            exists = False
        _winreg.CloseKey(key)
        return exists

    def remove(name):
        if not exists(name):
            return

        """delete an autostart entry"""
        key = get_runonce()
        _winreg.DeleteValue(key, name)
        _winreg.CloseKey(key)

    run_cmd = "\"" + os.path.join(top_path, "start.vbs") + "\""
elif sys.platform.startswith('linux'):
    _xdg_config_home = os.environ.get("XDG_CONFIG_HOME", "~/.config")
    home_config_path = os.path.expanduser(_xdg_config_home)
    _xdg_user_autostart = os.path.join(home_config_path, "autostart")

    def getfilename(name):
        """get the filename of an autostart (.desktop) file"""
        return os.path.join(_xdg_user_autostart, name + ".desktop")

    def add(name, application):
        if not os.path.isdir(home_config_path):
            xlog.warn("autorun linux config path not found:%s", home_config_path)
            return

        if not os.path.isdir(_xdg_user_autostart):
            try:
                os.mkdir(_xdg_user_autostart)
            except Exception as e:
                xlog.warn("Enable auto start, create path:%s fail:%r", _xdg_user_autostart, e)
                return

        """add a new autostart entry"""
        desktop_entry = "[Desktop Entry]\n" \
                        "Name=%s\n" \
                        "Exec=%s\n" \
                        "Type=Application\n" \
                        "Terminal=false\n" \
                        "X-GNOME-Autostart-enabled=true" % (name, application)
        with open(getfilename(name), "w") as f:
            f.write(desktop_entry)

    def exists(name):
        """check if an autostart entry exists"""
        return os.path.exists(getfilename(name))

    def remove(name):
        """delete an autostart entry"""
        if (exists(name)):
            os.unlink(getfilename(name))

    run_cmd = os.path.join(top_path, "start")
elif sys.platform == 'darwin':
    plist_template = """<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>Label</key>
	<string>com.xxnet.launcher</string>

	<key>LimitLoadToSessionType</key>
	<string>Aqua</string>

	<key>ProgramArguments</key>
	<array>
	  <string>%s</string>
	  <string>-hungup</string>
	</array>

	<key>RunAtLoad</key>
	<true/>

	<key>StandardErrorPath</key>
	<string>/dev/null</string>
	<key>StandardOutPath</key>
	<string>/dev/null</string>
</dict>
</plist>"""

    run_cmd = os.path.join(top_path, "start")
    from os.path import expanduser
    home = expanduser("~")
    launch_path = os.path.join(home, "Library/LaunchAgents")

    plist_file_path = os.path.join(launch_path, "com.xxnet.launcher.plist")

    def add(name, cmd):
        file_content = plist_template % cmd
        xlog.info("create file:%s", plist_file_path)

        if not os.path.isdir(launch_path):
            os.mkdir(launch_path, 0o755)

        with open(plist_file_path, "w") as f:
            f.write(file_content)

    def remove(name):
        if (os.path.isfile(plist_file_path)):
            os.unlink(plist_file_path)
            xlog.info("remove file:%s", plist_file_path)
else:
    def add(name, cmd):
        pass

    def remove(name):
        pass

def enable():
    add("XX-Net", run_cmd)

def disable():
    remove("XX-Net")

def test():
    assert not exists("test_xxx")
    try:
        add("test_xxx", "test")
        assert exists("test_xxx")
    finally:
        remove("test_xxx")
    assert not exists("test_xxx")


if __name__ == "__main__":
    test()

#!/usr/bin/env python
# coding:utf-8

import os
import json
import time
import threading
import zipfile
import sys
import platform
import uuid
from distutils.version import LooseVersion

from xlog import getLogger
xlog = getLogger("launcher")
import config
import update_from_github
import urllib2

try:
    reduce         # Python 2
except NameError:  # Python 3
    from functools import reduce

update_url = "https://xxnet-update.appspot.com/update.json"

update_content = ""
update_dict = {}
new_gae_proxy_version = ""
gae_proxy_path = ""

current_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.abspath(os.path.join(current_path, os.pardir))
data_root = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir, 'data'))


def get_opener():
    autoproxy = '127.0.0.1:8087'

    import ssl
    if getattr(ssl, "create_default_context", None):
        cafile = os.path.join(data_root, "gae_proxy", "CA.crt")
        if not os.path.isfile(cafile):
            cafile = None
        context = ssl.create_default_context(purpose=ssl.Purpose.SERVER_AUTH,
                                             cafile=cafile)
        https_handler = urllib2.HTTPSHandler(context=context)

        opener = urllib2.build_opener(urllib2.ProxyHandler({'http': autoproxy, 'https': autoproxy}), https_handler)
    else:
        opener = urllib2.build_opener(urllib2.ProxyHandler({'http': autoproxy, 'https': autoproxy}))
    return opener


def version_to_bin(s):
    return reduce(lambda a, b: a << 8 | b, map(int, s.split(".")))


def download_file(url, file):
    try:
        xlog.info("download %s to %s", url, file)
        opener = get_opener()
        req = opener.open(url, cafile="")
        CHUNK = 16 * 1024
        with open(file, 'wb') as fp:
            while True:
                chunk = req.read(CHUNK)
                if not chunk: break
                fp.write(chunk)
        return True
    except:
        xlog.info("download %s to %s fail", url, file)
        return False


def sha1_file(filename):
    import hashlib

    BLOCKSIZE = 65536
    hasher = hashlib.sha1()
    try:
        with open(filename, 'rb') as afile:
            buf = afile.read(BLOCKSIZE)
            while len(buf) > 0:
                hasher.update(buf)
                buf = afile.read(BLOCKSIZE)
        return hasher.hexdigest()
    except:
        return False


def install_module(module, new_version):
    import module_init
    import os, subprocess, sys

    current_path = os.path.dirname(os.path.abspath(__file__))
    new_module_version_path = os.path.abspath(os.path.join(current_path, os.pardir, os.pardir, module, new_version))

    # check path exist
    if not os.path.isdir(new_module_version_path):
        xlog.error("install module %s dir %s not exist", module, new_module_version_path)
        return

    # call setup.py
    setup_script = os.path.join(new_module_version_path, "setup.py")
    if not os.path.isfile(setup_script):
        xlog.warn("update %s fail. setup script %s not exist", module, setup_script)
        return

    config.set(["modules", module, "current_version"], str(new_version))
    config.save()

    if module == "launcher":
        module_init.stop_all()
        import web_control
        web_control.stop()

        subprocess.Popen([sys.executable, setup_script], shell=False)

        os._exit(0)

    else:
        xlog.info("Setup %s version %s ...", module, new_version)
        try:
            module_init.stop(module)

            subprocess.call([sys.executable, setup_script], shell=False)
            xlog.info("Finished new version setup.")

            xlog.info("Restarting new version ...")
            module_init.start(module)
        except Exception as e:
            xlog.error("install module %s %s fail:%s", module, new_version, e)


def download_module(module, new_version):
    import os
    global update_content, update_dict

    current_path = os.path.dirname(os.path.abspath(__file__))
    download_path = os.path.abspath(os.path.join(current_path, os.pardir, os.pardir, 'data', 'downloads'))
    if not os.path.isdir(download_path):
        os.mkdir(download_path)

    try:
        for source in update_dict["modules"][module]["versions"][new_version]["sources"]:
            url = source["url"]
            filename = module + "-" + new_version + ".zip"

            file_path = os.path.join(download_path, filename)

            if os.path.isfile(file_path) and sha1_file(file_path) == update_dict["modules"][module]["versions"][new_version]["sha1"]:
                pass
            elif not download_file(url, file_path):
                xlog.warn("download %s fail", url)
                continue

            sha1 = sha1_file(file_path)
            if update_dict["modules"][module]["versions"][new_version]["sha1"] != sha1:
                xlog.warn("download %s sha1 wrong", url)
                continue

            module_path = os.path.abspath(os.path.join(current_path, os.pardir, os.pardir, module))
            if not os.path.isdir(module_path):
                os.path.mkdir(module_path, "755")

            version_path = os.path.join(module_path, new_version)
            if os.path.isdir(version_path):
                xlog.error("module dir exist:%s, download exist.", version_path)
                return

            with zipfile.ZipFile(file_path, "r") as dz:
                dz.extractall(module_path)
                dz.close()

            import shutil
            unzip_path = os.path.abspath(os.path.join(module_path, module + "-" + new_version))
            tag_path = os.path.abspath(os.path.join(module_path, new_version))
            shutil.move(unzip_path, tag_path)

            msg = "Module %s new version %s downloaded, Install?" % (module, new_version)
            if sys.platform == "linux" or sys.platform == "linux2":
                from gtk_tray import sys_tray
                data_install = "%s|%s|install" % (module, new_version)
                data_ignore = "%s|%s|ignore" % (module, new_version)
                buttons = {1: {"data": data_install, "label": "Install", 'callback': general_gtk_callback},
                           2: {"data": data_ignore, "label": "Ignore", 'callback': general_gtk_callback}}
                sys_tray.notify_general(msg=msg, title="Install", buttons=buttons)
            elif sys.platform == "win32":
                from win_tray import sys_tray
                if sys_tray.dialog_yes_no(msg, u"Install", None, None) == 1:
                    install_module(module, new_version)
                else:
                    ignore_module(module, new_version)
            elif sys.platform == "darwin":
                from  mac_tray import sys_tray
                if sys_tray.presentAlert_withTitle_(msg, "Install"):
                    install_module(module, new_version)
                else:
                    ignore_module(module, new_version)

            else:
                install_module(module, new_version)

            break

    except Exception as e:
        xlog.warn("get gae_proxy source fail, content:%s err:%s", update_content, e)


def ignore_module(module, new_version):
    config.set(["modules", module, "ignore_version"], str(new_version))
    config.save()


def general_gtk_callback(widget=None, data=None):
    args = data.split('|')
    if len(args) != 3:
        xlog.error("general_gtk_callback data:%s", data)
        return

    module = args[0]
    new_version = args[1]
    action = args[2]

    if action == "download":
        download_module(module, new_version)
    elif action == "install":
        install_module(module, new_version)
    elif action == "ignore":
        ignore_module(module, new_version)


def check_update():
    try:
        if update_from_github.update_info == "dont-check":
            return

        check_push_update()

        update_rule = config.get(["update", "check_update"], "notice-stable")
        if update_rule not in ("stable", "notice-stable", "test", "notice-test"):
            return

        versions = update_from_github.get_github_versions()
        current_version = update_from_github.current_version()
        test_version, stable_version = versions[0][1], versions[1][1]
        if test_version != config.get(["update", "skip_test_version"]):
            if update_rule == "notice-test":
                if LooseVersion(current_version) < LooseVersion(test_version):
                    xlog.info("checked new test version %s", test_version)
                    update_from_github.update_info = '{"type":"test", "version":"%s"}' % test_version
            elif update_rule == "test":
                if LooseVersion(current_version) < LooseVersion(test_version):
                    xlog.info("update to test version %s", test_version)
                    update_from_github.update_version(test_version)
        if stable_version != config.get(["update", "skip_stable_version"]):
            if update_rule == "notice-stable":
                if LooseVersion(current_version) < LooseVersion(stable_version):
                    xlog.info("checked new stable version %s", stable_version)
                    update_from_github.update_info = '{"type":"stable", "version":"%s"}' % stable_version
            elif update_rule == "stable":
                if LooseVersion(current_version) < LooseVersion(stable_version):
                    xlog.info("update to stable version %s", stable_version)
                    update_from_github.update_version(stable_version)
    except IOError as e:
        xlog.warn("check update fail:%r", e)
    except Exception as e:
        xlog.exception("check_update fail:%r", e)
    finally:
        if update_from_github.update_info == "init":
            update_from_github.update_info = ""


def check_push_update():
    global update_content, update_dict
    try:
        opener = get_opener()

        req_url = update_url + "?uuid=" + get_uuid() \
                  + "&version=" + update_from_github.current_version() \
                  + "&platform=" + platform.platform()
        try:
            update_content = opener.open(req_url).read()
        except Exception as e:
            xlog.warn("check_update fail:%r", e)
            return False

        update_dict = json.loads(update_content)
        return True

        for module in update_dict["modules"]:
            new_version = str(update_dict["modules"][module]["last_version"])
            describe = update_dict["modules"][module]["versions"][new_version]["describe"]

            if update_dict["modules"][module]["versions"][new_version]["notify"] != "true":
                continue

            if not module in config.config["modules"]:
                ignore_version = 0
                current_version = 0
                config.config["modules"][module] = {}
                config.config["modules"][module]["current_version"] = '0.0.0'
            else:
                current_version = config.get(["modules", module, "current_version"])
                if "ignore_version" in config.config["modules"][module]:
                    ignore_version = config.config["modules"][module]["ignore_version"]
                else:
                    ignore_version = current_version

            if version_to_bin(new_version) <= version_to_bin(ignore_version):
                continue

            if version_to_bin(new_version) > version_to_bin(current_version):
                xlog.info("new %s version:%s", module, new_version)

                if sys.platform == "linux" or sys.platform == "linux2":
                    from gtk_tray import sys_tray
                    msg = "Module %s new version: %s, Download?\nNew:%s" % (module, new_version, describe)
                    data_download = "%s|%s|download" % (module, new_version)
                    data_ignore = "%s|%s|ignore" % (module, new_version)
                    buttons = {1: {"data": data_download, "label": "Download", 'callback': general_gtk_callback},
                               2: {"data": data_ignore, "label": "Ignore", 'callback': general_gtk_callback}}
                    sys_tray.notify_general(msg=msg, title="New Version", buttons=buttons)
                elif sys.platform == "win32":
                    from win_tray import sys_tray
                    msg = "Module %s new version: %s, Download?" % (module, new_version)
                    if sys_tray.dialog_yes_no(msg, u"Download", None, None) == 1:
                        download_module(module, new_version)
                    else:
                        ignore_module(module, new_version)
                elif sys.platform == "darwin":
                    from mac_tray import sys_tray
                    msg = "Module %s new version: %s, Download?" % (module, new_version)
                    if sys_tray.presentAlert_withTitle_(msg, "Download"):
                        download_module(module, new_version)
                    else:
                        ignore_module(module, new_version)

                else:
                    download_module(module, new_version)

    except Exception as e:
        xlog.exception("check_update except:%s", e)
        return


def create_desktop_shortcut():
    import sys
    import subprocess

    work_path = os.path.dirname(os.path.abspath(__file__))
    os.chdir(work_path)

    if sys.platform.startswith("linux"):
        if os.getenv("DESKTOP_SESSION", "unknown") != "unknown":  # make sure this is desktop linux
            xxnet_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir))
            cmd = 'env XXNETPATH="' + xxnet_path + '" "' + work_path + '/create_shortcut_linux.sh"'
            os.system(cmd)

    elif sys.platform == "win32":
        # import ctypes
        # msg = u"是否在桌面创建图标？"
        # title = u"XX-Net 叉叉网"
        #res = ctypes.windll.user32.MessageBoxW(None, msg, title, 1)
        # Yes:1 No:2
        #if res == 2:
        #    return

        subprocess.call(["Wscript.exe", "//E:JScript", "create_shortcut.js"], shell=False)


def notify_install_tcpz_for_winXp():
    import ctypes
    ctypes.windll.user32.MessageBoxW(None, u"请使用tcp-z对 tcpip.sys 打补丁，解决链接并发限制！", u"Patch XP needed", 0)


def check_new_machine():
    current_path = os.path.dirname(os.path.abspath(__file__))
    if current_path != config.get(["update", "last_path"], ""):
        config.set(["update", "last_path"], current_path)
        config.save()

        if sys.platform == "win32" and platform.release() == "XP":
            notify_install_tcpz_for_winXp()

        if os.getenv("XXNET_NO_MESS_SYSTEM", "0") == "0":
            xlog.info("generate desktop shortcut")
            create_desktop_shortcut()


def check_loop():
    check_new_machine()

    # wait gae_proxy to start
    # update need gae_proxy as proxy
    time.sleep(1)

    while True:
        check_update()
        time.sleep(3600 * 24)


def start():
    p = threading.Thread(target=check_loop)
    p.setDaemon(True)
    p.start()


def need_new_uuid():
    if not config.get(["update", "uuid"]):
        xlog.info("need_new_uuid: uuid is empty")
        return True
    return False


def generate_new_uuid():
    xx_net_uuid = str(uuid.uuid4())
    config.set(["update", "uuid"], xx_net_uuid)
    xlog.info("generate uuid:%s", xx_net_uuid)
    config.save()


def get_uuid():
    if need_new_uuid():
        generate_new_uuid()

    xx_net_uuid = config.get(["update", "uuid"])
    xlog.info("get uuid:%s", xx_net_uuid)
    return xx_net_uuid


if __name__ == "__main__":
    #get_uuid()
    #check_update()
    #sys_tray.serve_forever()
    create_desktop_shortcut()

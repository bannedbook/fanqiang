#!/usr/bin/env python2
# coding:utf-8

import sys
import os
import threading

current_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.abspath( os.path.join(current_path, os.pardir, os.pardir, os.pardir))
data_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir, 'data'))
module_data_path = os.path.join(data_path, 'x_tunnel')
python_path = os.path.abspath( os.path.join(root_path, 'python27', '1.0'))

sys.path.append(root_path)

noarch_lib = os.path.abspath( os.path.join(python_path, 'lib', 'noarch'))
sys.path.append(noarch_lib)

if sys.platform == "win32":
    win32_lib = os.path.abspath( os.path.join(python_path, 'lib', 'win32'))
    sys.path.append(win32_lib)
elif sys.platform.startswith("linux"):
    linux_lib = os.path.abspath( os.path.join(python_path, 'lib', 'linux'))
    sys.path.append(linux_lib)
elif sys.platform == "darwin":
    darwin_lib = os.path.abspath( os.path.join(python_path, 'lib', 'darwin'))
    sys.path.append(darwin_lib)
    extra_lib = "/System/Library/Frameworks/Python.framework/Versions/2.7/Extras/lib/python"
    sys.path.append(extra_lib)


import xlog
logger = xlog.getLogger("check_ip")
logger.set_buffer(500)

from front_base.openssl_wrap import SSLContext
from front_base.host_manager import HostManagerBase
from front_base.connect_creator import ConnectCreator
from front_base.check_ip import CheckIp


from config import Config


class CheckAllIp(object):

    def __init__(self):
        config_path = os.path.join(module_data_path, "heroku_front.json")
        config = Config(config_path)

        openssl_context = SSLContext(logger)

        host_manager = HostManagerBase()
        connect_creator = ConnectCreator(logger, config, openssl_context, host_manager,
                                         debug=True)
        self.check_ip = CheckIp(logger, config, connect_creator)

        self.lock = threading.Lock()

        self.in_fd = open("good_ip.txt", "r")
        self.out_fd = open(
            os.path.join(module_data_path, "heroku_checked_ip.txt"),
            "w"
        )

    def get_ip(self):
        with self.lock:
            while True:
                line = self.in_fd.readline()
                if not line:
                    raise Exception()

                try:
                    ip = line.split()[0]
                    return ip
                except:
                    continue

    def write_ip(self, ip, host, handshake):
        with self.lock:
            self.out_fd.write("%s %s gws %d 0 0\n" % (ip, host, handshake))
            self.out_fd.flush()

    def checker(self):
        while True:
            try:
                ip = self.get_ip()
            except Exception as e:
                xlog.info("no ip left")
                return

            try:
                res = self.check_ip.check_ip(ip)
            except Exception as e:
                xlog.warn("check except:%r", e)
                continue

            if not res or not res.ok:
                xlog.debug("ip:%s fail", ip)
                continue

            self.write_ip(ip, res.domain, res.handshake_time)

    def run(self):
        for i in range(0, 10):
            threading.Thread(target=self.checker).run()


def check_all():
    check = CheckAllIp()
    check.run()
    exit(0)


def check_one(ip, top_domain, wait_time):
    config_path = os.path.join(module_data_path, "heroku_front.json")
    config = Config(config_path)

    openssl_context = SSLContext(logger)

    host_manager = HostManagerBase()
    connect_creator = ConnectCreator(logger, config, openssl_context, host_manager,
                                     debug=True)
    check_ip = CheckIp(logger, config, connect_creator)

    res = check_ip.check_ip(ip, host=top_domain, wait_time=wait_time)
    if not res:
        print("connect fail")
    elif res.ok:
        print("success, domain:%s handshake:%d" % (res.domain, res.handshake_time))
    else:
        print("not support")


if __name__ == "__main__":
    check_all()

    # case 1: only ip
    # case 2: ip + domain
    #    connect use domain

    if len(sys.argv) > 1:
        ip = sys.argv[1]
    else:
        ip = "54.225.129.54"
        print("Usage: check_ip.py [ip] [top_domain] [wait_time=0]")
    print("test ip:%s" % ip)

    if len(sys.argv) > 2:
        top_domain = sys.argv[2]
    else:
        top_domain = None

    if len(sys.argv) > 3:
        wait_time = int(sys.argv[3])
    else:
        wait_time = 0

    check_one(ip, top_domain, wait_time)
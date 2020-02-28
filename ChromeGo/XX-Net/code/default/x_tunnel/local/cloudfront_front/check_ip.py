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


import utils
import xlog
logger = xlog.getLogger("cloudfront_front")
logger.set_buffer(500)

from front_base.openssl_wrap import SSLContext
from front_base.connect_creator import ConnectCreator
from front_base.check_ip import CheckIp
from front_base.host_manager import HostManagerBase

from config import Config


class CheckAllIp(object):
    def __init__(self, check_ip, host):
        self.check_ip = check_ip
        self.host = host
        self.lock = threading.Lock()

        self.in_fd = open("good_ip.txt", "r")
        self.out_fd = open(
            os.path.join(module_data_path, "cloudfront_checked_ip.txt"),
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
                res = self.check_ip.check_ip(ip, host=host)
            except Exception as e:
                xlog.warn("check fail:%s except:%r", e)
                continue

            if not res or not res.ok:
                xlog.debug("check fail:%s fail", ip)
                continue

            self.write_ip(ip, res.domain, res.handshake_time)

    def run(self):
        for i in range(0, 10):
            threading.Thread(target=self.checker).start()


def check_all_ip(check_ip):
    check = CheckAllIp(check_ip, "scan1.xx-net.net")
    check.run()


if __name__ == "__main__":
    # case 1: only ip
    # case 2: ip + domain
    #    connect use domain

    default_ip = "54.192.37.196"

    sni = "www.binance.com"
    host = "scan1.xx-net.net"
    if len(sys.argv) > 1:
        ip = sys.argv[1]
        if not utils.check_ip_valid(ip):
            ip = default_ip
            host = sys.argv[1]
    else:
        ip = default_ip
        print("Usage: check_ip.py [ip] [top_domain] [wait_time=0]")
    xlog.info("test ip:%s", ip)

    if len(sys.argv) > 2:
        host = sys.argv[2]
    xlog.info("host:%s", host)

    if len(sys.argv) > 3:
        wait_time = int(sys.argv[3])
    else:
        wait_time = 0

    config_path = os.path.join(module_data_path, "cloudfront_front.json")
    config = Config(config_path)

    openssl_context = SSLContext(logger, support_http2=True)
    host_manager = HostManagerBase()
    connect_creator = ConnectCreator(logger, config, openssl_context, host_manager, debug=True)
    check_ip = CheckIp(logger, config, connect_creator)

    #check_all_ip(check_ip)
    #exit(0)

    res = check_ip.check_ip(ip, sni=sni, host=host, wait_time=wait_time)
    if not res:
        xlog.warn("connect fail")
    elif res.ok:
        xlog.info("success, domain:%s handshake:%d", res.host, res.handshake_time)
    else:
        xlog.warn("not support")
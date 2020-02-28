#!/usr/bin/env python2
# coding:utf-8
import sys
import os
import threading
import json

current_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.abspath( os.path.join(current_path, os.pardir, os.pardir, os.pardir))
data_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir, 'data'))
module_data_path = os.path.join(data_path, 'x_tunnel')
python_path = os.path.abspath( os.path.join(root_path, 'python27', '1.0'))


if __name__ == "__main__":
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
logger = xlog.getLogger("cloudflare_front")
logger.set_buffer(500)

from front_base.openssl_wrap import SSLContext
from front_base.connect_creator import ConnectCreator
from front_base.check_ip import CheckIp
from front_base.host_manager import HostManagerBase

from config import Config


def check_all_domain(check_ip):
    with open(os.path.join(current_path, "front_domains.json"), "r") as fd:
        content = fd.read()
        cs = json.loads(content)
        for host in cs:
            host = "scan1." + host
            res = check_ip.check_ip(ip, host=host, wait_time=wait_time)
            if not res or not res.ok:
                xlog.warn("host:%s fail", host)
            else:
                xlog.info("host:%s ok", host)


class CheckAllIp(object):
    def __init__(self, check_ip, host):
        self.check_ip = check_ip
        self.host = host
        self.lock = threading.Lock()

        self.in_fd = open("good_ip.txt", "r")
        self.out_fd = open(
            os.path.join(module_data_path, "cloudflare_checked_ip.txt"),
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
                res = self.check_ip.check_ip(ip, sni=host, host=host)
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
    check = CheckAllIp(check_ip, "scan1.movistar.gq")
    check.run()


if __name__ == "__main__":
    # format: [ip] [domain [sni] ]

    # case 1: only ip
    # case 2: ip domain
    # case 3: ip domain sni
    # case 4: domain
    # case 5: domain sni

    ip = "141.101.120.131"
    host = "xx-net.net"
    sni = host

    args = list(sys.argv[1:])

    if len(args):
        if utils.check_ip_valid(args[0]):
            ip = args.pop(0)

    if len(args):
        host = args.pop(0)
        sni = host

    if len(args):
        sni = args.pop(0)

    # print("Usage: check_ip.py [ip] [top_domain] [wait_time=0]")
    xlog.info("test ip:%s", ip)
    xlog.info("host:%s", host)
    xlog.info("sni:%s", sni)

    wait_time = 0

    config_path = os.path.join(module_data_path, "cloudflare_front.json")
    config = Config(config_path)

    ca_certs = os.path.join(current_path, "cacert.pem")
    openssl_context = SSLContext(logger, ca_certs=ca_certs)
    host_manager = HostManagerBase()
    connect_creator = ConnectCreator(logger, config, openssl_context, host_manager, debug=True)
    check_ip = CheckIp(logger, config, connect_creator)

    #check_all_domain(check_ip)
    #check_all_ip(check_ip)
    #exit(0)

    res = check_ip.check_ip(ip, sni=sni, host=host, wait_time=wait_time)
    if not res:
        xlog.warn("connect fail")
    elif res.ok:
        xlog.info("success, domain:%s handshake:%d", res.host, res.handshake_time)
    else:
        xlog.warn("not support")
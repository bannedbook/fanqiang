#!/usr/bin/env python2
# coding:utf-8

import sys
import os
import threading

current_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.abspath( os.path.join(current_path, os.pardir, os.pardir))
data_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir, 'data'))
module_data_path = os.path.join(data_path, 'gae_proxy')
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


import xlog
logger = xlog.getLogger("gae_proxy")

from front_base.openssl_wrap import SSLContext
from front_base.connect_creator import ConnectCreator
from front_base.host_manager import HostManagerBase
import front_base.check_ip
from config import config


class CheckIp(front_base.check_ip.CheckIp):
    def check_response(self, response):
        server_type = response.headers.get('Server', "")
        self.logger.debug("status:%d", response.status)
        self.logger.debug("Server type:%s", server_type)

        if response.status not in self.config.check_ip_accept_status:
            return False

        if response.status == 503:
            # out of quota
            if "gws" not in server_type and "Google Frontend" not in server_type and "GFE" not in server_type:
                xlog.warn("503 but server type:%s", server_type)
                return False
            else:
                return True

        content = response.read()
        if self.config.check_ip_content not in content:
            self.logger.warn("app check content:%s", content)
            return False

        return True


class CheckAllIp(object):

    def __init__(self):
        ca_certs = os.path.join(current_path, "cacert.pem")
        openssl_context = SSLContext(
            logger, ca_certs=ca_certs,
            cipher_suites=['ALL', "!RC4-SHA", "!ECDHE-RSA-RC4-SHA", "!ECDHE-RSA-AES128-GCM-SHA256",
                           "!AES128-GCM-SHA256", "!ECDHE-RSA-AES128-SHA", "!AES128-SHA"]
        )
        host_manager = HostManagerBase()
        connect_creator = ConnectCreator(logger, config, openssl_context, host_manager,
                                         debug=True)
        self.check_ip = CheckIp(logger, config, connect_creator)

        self.lock = threading.Lock()

        self.in_fd = open("ipv6_list.txt", "r")
        self.out_fd = open(
            os.path.join(module_data_path, "ipv6_list.txt"),
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

            if res.h2:
                self.write_ip(ip, res.domain, res.handshake_time)

    def run(self):
        for i in range(0, 100):
            threading.Thread(target=self.checker).start()


def check_all():
    check = CheckAllIp()
    check.run()
    exit(0)


if __name__ == "__main__":
    #check_all()

    # case 1: only ip
    # case 2: ip + domain
    #    connect use domain

    if len(sys.argv) > 1:
        ip = sys.argv[1]
    else:
        print("Usage: check_ip.py [ip] [top_domain] [wait_time=0]")
        exit(0)

    print("test ip:%s" % ip)

    if len(sys.argv) > 2:
        top_domain = sys.argv[2]
    else:
        top_domain = None

    if len(sys.argv) > 3:
        wait_time = int(sys.argv[3])
    else:
        wait_time = 0

    ca_certs = os.path.join(current_path, "cacert.pem")
    openssl_context = SSLContext(
        logger, ca_certs=ca_certs,
        cipher_suites=['ALL', "!RC4-SHA", "!ECDHE-RSA-RC4-SHA", "!ECDHE-RSA-AES128-GCM-SHA256",
                       "!AES128-GCM-SHA256", "!ECDHE-RSA-AES128-SHA", "!AES128-SHA"]
    )
    host_manager = HostManagerBase()
    connect_creator = ConnectCreator(logger, config, openssl_context, host_manager,
                                     debug=True)
    check_ip = CheckIp(logger, config, connect_creator)

    res = check_ip.check_ip(ip, host=top_domain, wait_time=wait_time)
    if not res:
        print("connect fail")
    elif res.ok:
        print("success, domain:%s handshake:%d" % (res.host, res.handshake_time))
    else:
        print("not support")
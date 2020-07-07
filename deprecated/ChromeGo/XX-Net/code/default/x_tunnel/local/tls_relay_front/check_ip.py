#!/usr/bin/env python2
# coding:utf-8

import sys
import os


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
logger = xlog.getLogger("tls_relay")
logger.set_buffer(500)

from front_base.openssl_wrap import SSLContext
from front_base.connect_creator import ConnectCreator
from front_base.check_ip import CheckIp


from config import Config


if __name__ == "__main__":
    # case 1: only ip
    # case 2: ip + domain
    #    connect use domain

    if len(sys.argv) > 1:
        ip = sys.argv[1]
    else:
        ip = "207.246.89.177"
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

    config_path = os.path.join(module_data_path, "tls_relay.json")
    config = Config(config_path)

    openssl_context = SSLContext(logger)

    connect_creator = ConnectCreator(logger, config, openssl_context)
    check_ip = CheckIp(logger, config, connect_creator)

    res = check_ip.check_ip(ip, top_domain=top_domain, wait_time=wait_time)
    if not res:
        print("connect fail")
    elif res.ok:
        print("success, domain:%s handshake:%d" % (res.top_domain, res.handshake_time))
    else:
        print("not support")
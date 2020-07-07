#!/usr/bin/env python2
# coding:utf-8
# Based on GAppProxy 2.0.0 by Du XiaoGang <dugang.2008@gmail.com>
# Based on WallProxy 0.4.0 by Hust Moon <www.ehust@gmail.com>
# Contributor:
#      Phus Lu           <phus.lu@gmail.com>
#      Hewig Xu          <hewigovens@gmail.com>
#      Ayanamist Yang    <ayanamist@gmail.com>
#      V.E.O             <V.E.O@tom.com>
#      Max Lv            <max.c.lv@gmail.com>
#      AlsoTang          <alsotang@gmail.com>
#      Christopher Meng  <cickumqt@gmail.com>
#      Yonsm Guo         <YonsmGuo@gmail.com>
#      Parkman           <cseparkman@gmail.com>
#      Ming Bai          <mbbill@gmail.com>
#      Bin Yu            <yubinlove1991@gmail.com>
#      lileixuan         <lileixuan@gmail.com>
#      Cong Ding         <cong@cding.org>
#      Zhang Youfu       <zhangyoufu@gmail.com>
#      Lu Wei            <luwei@barfoo>
#      Harmony Meow      <harmony.meow@gmail.com>
#      logostream        <logostream@gmail.com>
#      Rui Wang          <isnowfy@gmail.com>
#      Wang Wei Qiang    <wwqgtxx@gmail.com>
#      Felix Yan         <felixonmars@gmail.com>
#      QXO               <qxodream@gmail.com>
#      Geek An           <geekan@foxmail.com>
#      Poly Rabbit       <mcx_221@foxmail.com>
#      oxnz              <yunxinyi@gmail.com>
#      Shusen Liu        <liushusen.smart@gmail.com>
#      Yad Smood         <y.s.inside@gmail.com>
#      Chen Shuang       <cs0x7f@gmail.com>
#      cnfuyu            <cnfuyu@gmail.com>
#      cuixin            <steven.cuixin@gmail.com>



import sys
import os

current_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.abspath( os.path.join(current_path, os.pardir, os.pardir))
data_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir, 'data'))
data_gae_proxy_path = os.path.join(data_path, 'gae_proxy')
python_path = os.path.abspath( os.path.join(root_path, 'python27', '1.0'))

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

import traceback
import platform

__file__ = os.path.abspath(__file__)
if os.path.islink(__file__):
    __file__ = getattr(os, 'readlink', lambda x: x)(__file__)
work_path = os.path.dirname(os.path.abspath(__file__))
os.chdir(work_path)


def check_create_data_path():
    if not os.path.isdir(data_path):
        os.mkdir(data_path)

    if not os.path.isdir(data_gae_proxy_path):
        os.mkdir(data_gae_proxy_path)


from xlog import getLogger
xlog = getLogger("gae_proxy")
xlog.set_buffer(1000)

from cert_util import CertUtil
import simple_http_server
import proxy_handler
import env_info
from front import front, direct_front


proxy_server = None
# launcher/module_init will check this value for start/stop finished
ready = False


def log_info():
    xlog.info('------------------------------------------------------')
    xlog.info('Python Version     : %s', platform.python_version())
    xlog.info('OS                 : %s', env_info.os_detail())
    xlog.info('Listen Address     : %s:%d', front.config.listen_ip, front.config.listen_port)
    if front.config.PROXY_ENABLE:
        xlog.info('%s Proxy    : %s:%s', front.config.PROXY_TYPE, front.config.PROXY_HOST, front.config.PROXY_PORT)

    if len(front.config.GAE_APPIDS):
        xlog.info('GAE APPID          : %s', '|'.join(front.config.GAE_APPIDS))
    else:
        xlog.info("Using public APPID")
    xlog.info('------------------------------------------------------')


def main(args):
    global ready, proxy_server

    check_create_data_path()

    log_info()

    CertUtil.init_ca()

    allow_remote = args.get("allow_remote", 0)

    listen_ips = front.config.listen_ip
    if isinstance(listen_ips, basestring):
        listen_ips = [listen_ips]
    else:
        listen_ips = list(listen_ips)
    if allow_remote and ("0.0.0.0" not in listen_ips or "::" not in listen_ips):
        listen_ips.append("0.0.0.0")
    addresses = [(listen_ip, front.config.listen_port) for listen_ip in listen_ips]

    front.start()
    direct_front.start()

    proxy_server = simple_http_server.HTTPServer(
        addresses, proxy_handler.GAEProxyHandler, logger=xlog)

    ready = True  # checked by launcher.module_init
    
    proxy_server.serve_forever()


# called by launcher/module/stop
def terminate():
    global ready, proxy_server

    xlog.info("start to terminate GAE_Proxy")
    ready = False
    front.stop()
    direct_front.stop()
    proxy_server.shutdown()


if __name__ == '__main__':
    try:
        main({})
    except Exception:
        traceback.print_exc(file=sys.stdout)
    except KeyboardInterrupt:
        terminate()
        sys.exit()

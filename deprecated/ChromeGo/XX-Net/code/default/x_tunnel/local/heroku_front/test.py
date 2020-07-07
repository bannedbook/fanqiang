#!/usr/bin/env python2
# coding:utf-8

import os
import sys
import time
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



from front import front
from xlog import getLogger
xlog = getLogger("heroku_front")
xlog.set_buffer(2000)


def get():
    start_time = time.time()
    content, status, response = front.request("GET", "dns.xx-net.net", path="/query?domain=www.google.com")
    time_cost = time.time() - start_time
    xlog.info("GET cost:%f", time_cost)
    xlog.info("status:%d content:%s", status, content.tobytes())
    front.stop()


if __name__ == '__main__':
    import traceback

    try:
        get()
    except Exception:
        traceback.print_exc(file=sys.stdout)
    except KeyboardInterrupt:
        front.stop()
        sys.exit()

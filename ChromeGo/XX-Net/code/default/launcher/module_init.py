import subprocess
import threading
import os
import sys
import config

from xlog import getLogger
xlog = getLogger("launcher")
import web_control
import time

proc_handler = {}

xargs = {}

current_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.abspath(os.path.join(current_path, os.pardir))
if root_path not in sys.path:
    sys.path.append(root_path)

data_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir, 'data'))
data_launcher_path = os.path.join(data_path, 'launcher')
running_file = os.path.join(data_launcher_path, "Running.Lck")


def start(module):
    if not os.path.isdir(os.path.join(root_path, module)):
        return

    try:
        if module not in config.config["modules"]:
            xlog.error("module not exist %s", module)
            raise Exception()

        if module in proc_handler:
            xlog.error("module %s is running", module)
            return "module is running"

        if module not in proc_handler:
            proc_handler[module] = {}

        if os.path.isfile(os.path.join(root_path, module, "__init__.py")):
            if "imp" not in proc_handler[module]:
                proc_handler[module]["imp"] = __import__(module, globals(), locals(), ['local', 'start'], -1)

            _start = proc_handler[module]["imp"].start
            p = threading.Thread(target=_start.main, args=([xargs]))
            p.daemon = True
            p.start()
            proc_handler[module]["proc"] = p

            while not _start.client.ready:
                time.sleep(0.1)
        else:
            script_path = os.path.join(root_path, module, 'start.py')
            if not os.path.isfile(script_path):
                xlog.warn("start module script not exist:%s", script_path)
                return "fail"

            proc_handler[module]["proc"] = subprocess.Popen([sys.executable, script_path], shell=False)

        xlog.info("module %s started", module)

    except Exception as e:
        xlog.exception("start module %s fail:%s", module, e)
        raise
    return "start success."


def stop(module):
    try:
        if module not in proc_handler:
            xlog.error("module %s not running", module)
            return

        if os.path.isfile(os.path.join(root_path, module, "__init__.py")):

            _start = proc_handler[module]["imp"].start
            xlog.debug("start to terminate %s module", module)
            _start.client.terminate()
            xlog.debug("module %s stopping", module)
            while _start.client.ready:
                time.sleep(0.1)
        else:
            proc_handler[module]["proc"].terminate()  # Sends SIGTERM
            proc_handler[module]["proc"].wait()

        del proc_handler[module]

        xlog.info("module %s stopped", module)
    except Exception as e:
        xlog.exception("stop module %s fail:%s", module, e)
        return "Except:%s" % e
    return "stop success."


def call_each_module(api_name, args):
    for module in proc_handler:
        try:
            apis = proc_handler[module]["imp"].local.apis
            if not hasattr(apis, api_name):
                continue
            api = getattr(apis, api_name)
            api(args)
        except Exception as e:
            xlog.exception("call %s api:%s, except:%r", module, api_name, e)


def start_all_auto():    
    global running_file
    if not os.path.isfile(running_file):
        open(running_file, 'a').close()

    for module in config.modules:
        if module in ["launcher"]:
            continue
        if not os.path.isdir(os.path.join(root_path, module)):
            continue
        if "auto_start" in config.config['modules'][module] and config.config['modules'][module]["auto_start"]:
            start_time = time.time()
            start(module)
            # web_control.confirm_module_ready(config.get(["modules", module, "control_port"], 0))
            finished_time = time.time()
            xlog.info("start %s time cost:%d ms", module, (finished_time - start_time) * 1000)


def stop_all():
    global running_file
    running_modules = [k for k in proc_handler]
    for module in running_modules:
        stop(module)

    os.remove(running_file)
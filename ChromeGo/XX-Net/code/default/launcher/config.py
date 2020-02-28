#!/usr/bin/env python

import os
import yaml
from xlog import getLogger
xlog = getLogger("launcher")

current_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.abspath(os.path.join(current_path, os.pardir))
data_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir, 'data'))
config_path = os.path.join(data_path, 'launcher', 'config.yaml')

config = {}
need_save_config = False
modules = ["gae_proxy", "launcher", "x_tunnel", "smart_router"]


def load():
    global config, config_path
    try:
        config = yaml.load(open(config_path, 'r'))
        if config is None:
            config = {}
        # print(yaml.dump(config))
    except Exception as exc:
        print("Error in configuration file:", exc)


def save():
    global config, config_path
    global need_save_config
    try:
        yaml.dump(config, open(config_path, "w"))
        need_save_config = False
    except Exception as e:
        xlog.warn("save config %s fail %s", config_path, e)


def get(path, default_val=""):
    global config
    try:
        cmd = "config"
        for p in path:
            cmd += '["%s"]' % p
        value = eval(cmd)
        return value
    except:
        return default_val


def _set(m, k_list, v):
    k0 = k_list[0]
    if len(k_list) == 1:
        m[k0] = v
        return
    if k0 not in m:
        m[k0] = {}
    _set(m[k0], k_list[1:], v)


def set(path, val):
    global config
    global need_save_config
    _set(config, path, val)
    need_save_config = True


def recheck_module_path():
    global config
    global need_save_config

    for module in modules:
        if module not in ["launcher"]:
            if not os.path.isdir(os.path.join(root_path, module)):
                del config[module]
                continue

            if get(["modules", module, "auto_start"], -1) == -1:
                set(["modules", module, "auto_start"], 1)

    if get(["modules", "launcher", "control_port"], 0) == 0:
        set(["modules", "launcher", "control_port"], 8085)
        set(["modules", "launcher", "allow_remote_connect"], 0)

    if get(["no_mess_system"], 0) == 1 or os.getenv("XXNET_NO_MESS_SYSTEM","0") != "0" :
        xlog.debug("no_mess_system")
        os.environ["XXNET_NO_MESS_SYSTEM"] = "1"
        set(["no_mess_system"], 1)

    return need_save_config


def init():
    if os.path.isfile(config_path):
        load()

    if recheck_module_path():
        save()

init()

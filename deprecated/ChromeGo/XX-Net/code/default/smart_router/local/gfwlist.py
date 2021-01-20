#!/usr/bin/env python
# coding:utf-8


import os

current_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.abspath(os.path.join(current_path, os.pardir, os.pardir))
data_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir, 'data', "smart_router"))

from xlog import getLogger
xlog = getLogger("smart_router")


class GfwList(object):
    def __init__(self):
        self.gfw_black_list = self.load("gfw_black_list.txt")
        self.gfw_white_list = self.load("gfw_white_list.txt")
        self.advertisement_list = self.load("advertisement_list.txt")

    def load(self, name):
        user_file = os.path.join(data_path, name)
        if os.path.isfile(user_file):
            list_file = user_file
        else:
            list_file = os.path.join(current_path, name)

        xlog.info("Load file:%s", list_file)

        fd = open(list_file, "r")
        gfwdict = {}
        for line in fd.readlines():
            line = line.strip()
            if not line:
                continue

            gfwdict[line] = 1

        gfwlist = [h for h in gfwdict]
        return tuple(gfwlist)

    def check(self, host):
        if host.endswith(self.gfw_white_list):
            return False

        if not host.endswith(self.gfw_black_list):
            return False

        # check avoid wrong match like xgoogle.com
        dpl = host.split(".")
        for i in range(0, len(dpl)):
            h = ".".join(dpl[i:])
            if h in self.gfw_black_list:
                return True

        return False

    def is_white(self, host):
        if host.endswith(self.gfw_white_list):
            return True
        else:
            return False

    def is_advertisement(self, host):
        if host.endswith(self.advertisement_list):
            return True
        else:
            return False

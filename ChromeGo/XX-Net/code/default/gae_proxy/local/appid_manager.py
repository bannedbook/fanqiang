#!/usr/bin/env python
# coding:utf-8

import random
import threading
import time
import os
from front_base.random_get_slice import RandomGetSlice


current_path = os.path.dirname(os.path.abspath(__file__))


class AppidManager(object):
    lock = threading.Lock()

    def __init__(self, config, logger):
        self.config = config
        self.logger = logger
        self.check_api = None
        self.ip_manager = None

        fn = os.path.join(current_path, "appids.txt")
        self.public_appid = RandomGetSlice(fn, 60)

        self.reset_appid()

    def reset_appid(self):
        # called by web_control
        with self.lock:
            self.working_appid_list = list()
            for appid in self.config.GAE_APPIDS:
                if not appid:
                    self.config.GAE_APPIDS.remove(appid)
                    continue
                self.working_appid_list.append(appid)
            self.not_exist_appids = []
            self.out_of_quota_appids = []
        self.last_reset_time = time.time()

    def get(self):
        if len(self.config.GAE_APPIDS):
            if len(self.working_appid_list) == 0:
                time_to_reset = 600 - (time.time() - self.last_reset_time)
                if time_to_reset > 0:
                    self.logger.warn("all appid out of quota, wait %d seconds to reset", time_to_reset)
                    time.sleep(time_to_reset)
                    return None
                else:
                    self.logger.warn("reset appid")
                    self.reset_appid()

            appid = random.choice(self.working_appid_list)
            return str(appid)
        else:
            for _ in xrange(0, 10):
                appid = self.public_appid.get()
                if appid in self.out_of_quota_appids or appid in self.not_exist_appids:
                    continue
                else:
                    return appid
            return None

    def report_out_of_quota(self, appid):
        self.logger.warn("report_out_of_quota:%s", appid)
        with self.lock:
            if appid not in self.out_of_quota_appids:
                self.out_of_quota_appids.append(appid)
            try:
                self.working_appid_list.remove(appid)
            except:
                pass

    def report_not_exist(self, appid, ip):
        self.logger.debug("report_not_exist:%s %s", appid, ip)
        th = threading.Thread(target=self.process_appid_not_exist, args=(appid, ip))
        th.start()

    def process_appid_not_exist(self, appid, ip):
        ret = self.check_api(ip, "xxnet-1")
        if ret and ret.ok:
            self.set_appid_not_exist(appid)
        else:
            self.logger.warn("process_appid_not_exist, remove ip:%s", ip)

            self.ip_manager.report_connect_fail(ip, force_remove=True)

    def set_appid_not_exist(self, appid):
        self.logger.warn("APPID_manager, set_appid_not_exist %s", appid)
        with self.lock:
            if appid not in self.not_exist_appids:
                self.not_exist_appids.append(appid)
            try:
                self.config.GAE_APPIDS.remove(appid)
            except:
                pass

            try:
                self.working_appid_list.remove(appid)
            except:
                pass

    def appid_exist(self, appids):
        for appid in appids.split('|'):
            if appid == "":
                continue
            if appid in self.config.GAE_APPIDS:
                return True
        return False


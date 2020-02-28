import random
import time

from front_base.host_manager import HostManagerBase


class HostManager(HostManagerBase):
    def __init__(self, logger, appids=[]):
        self.logger = logger
        self.appids = appids

    def set_appids(self, appids):
        self.appids = appids

    def get_sni_host(self, ip):
        if len(self.appids) == 0:
            self.logger.warn("no appid left")
            time.sleep(1)
            raise Exception("no appid left")

        return "", random.choice(self.appids)

    def remove(self, appid):
        try:
            self.appids.remove(appid)
        except:
            pass
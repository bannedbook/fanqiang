import time

from front_base.host_manager import HostManagerBase
from sni_manager import SniManager


class HostManager(HostManagerBase):
    def __init__(self, config, logger):
        self.config = config
        self.logger = logger
        self.appid_manager = None

        self.sni_manager = SniManager(logger)

    def get_host(self):
        if not self.appid_manager:
            return ""

        appid = self.appid_manager.get()
        if not appid:
            self.logger.warn("no appid")
            time.sleep(10)
            raise Exception()

        return appid + ".appspot.com"

    def get_sni_host(self, ip):
        sni = self.sni_manager.get()
        host = self.get_host()

        return sni, host


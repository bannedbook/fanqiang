import os
import json

from front_base.host_manager import HostManagerBase


class HostManager(HostManagerBase):
    def __init__(self, config_fn):
        self.config_fn = config_fn
        self.info = {}
        self.load()

    def load(self):
        if not os.path.isfile(self.config_fn):
            return

        try:
            with open(self.config_fn, "r") as fd:
                self.info = json.load(fd)
        except:
            pass

    def save(self):
        with open(self.config_fn, "w") as fd:
            json.dump(self.info, fd, indent=2)

    def set_host(self, info):
        self.info = info
        self.save()

    def get_sni_host(self, ip):
        if ip not in self.info:
            return "", ""

        return self.info[ip]["sni"], ""

    def reset(self):
        self.info = {}
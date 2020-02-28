
from front_base.random_get_slice import RandomGetSlice


class HostManager(RandomGetSlice):
    def __init__(self, fn, max_size):
        super(HostManager, self).__init__(fn, max_size)

    def get_sni_host(self, ip):
        sni = self.get()
        top_domain = ""
        return sni, top_domain


import sys
import os

import time
import threading

current_path = os.path.dirname(os.path.abspath(__file__))

if __name__ == "__main__":
    python_path = os.path.abspath( os.path.join(current_path, os.pardir, os.pardir, 'python27', '1.0'))

    noarch_lib = os.path.abspath( os.path.join(python_path, 'lib', 'noarch'))
    sys.path.append(noarch_lib)

    if sys.platform == "win32":
        win32_lib = os.path.abspath( os.path.join(python_path, 'lib', 'win32'))
        sys.path.append(win32_lib)
    elif sys.platform.startswith("linux"):
        linux_lib = os.path.abspath( os.path.join(python_path, 'lib', 'linux'))
        sys.path.append(linux_lib)


#try:
#    from code.default.gae_proxy.local.config import config
#except:
#    from code.default.gae_proxy.local.config import config
from config import config

import simple_http_client
from xlog import getLogger
xlog = getLogger("gae_proxy")


max_timeout = 5


class CheckNetwork(object):
    def __init__(self, type="IPv4"):
        self.type = type
        self.urls = []
        self._checking_lock = threading.Lock()
        self._checking_num = 0
        self.network_stat = "unknown"
        self.last_check_time = 0
        self.continue_fail_count = 0

        if config.PROXY_ENABLE:
            if config.PROXY_USER:
                self.proxy = "%s://%s:%s@%s:%d" % \
                    (config.PROXY_TYPE, config.PROXY_USER, config.PROXY_PASSWD, config.PROXY_HOST, config.PROXY_PORT)
            else:
                self.proxy = "%s://%s:%d" % \
                    (config.PROXY_TYPE, config.PROXY_HOST, config.PROXY_PORT)
        else:
            self.proxy = None

        self.http_client = simple_http_client.Client(self.proxy, timeout=10)

    def report_ok(self):
        self.network_stat = "OK"
        self.last_check_time = time.time()
        self.continue_fail_count = 0

    def report_fail(self):
        self.continue_fail_count += 1
        # don't record last_check_time here, it's not a real check
        # last_check_time = time.time()

        if self.continue_fail_count > 10:
            # don't set network_stat to "unknown", wait for check
            # network_stat = "unknown"
            xlog.debug("report_connect_fail %s continue_fail_count:%d",
                       self.type, self.continue_fail_count)
            self.triger_check_network(True)

    def get_stat(self):
        return self.network_stat

    def is_ok(self):
        return self.network_stat == "OK"

    def _test_host(self, url):
        try:
            header = {
                "user-agent": "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Safari/537.36",
                "accept": "application/json, text/javascript, */*; q=0.01",
                "accept-encoding": "gzip, deflate, sdch",
                "accept-language": 'en-US,en;q=0.8,ja;q=0.6,zh-CN;q=0.4,zh;q=0.2',
                "connection": "keep-alive"
                }
            response = self.http_client.request("HEAD", url, header, "", read_payload=False)
            if response:
                return True
        except Exception as e:
            if __name__ == "__main__":
                xlog.exception("test %s e:%r", url, e)

        return False

    def _simple_check_worker(self):
        time_now = time.time()

        self._checking_lock.acquire()
        self._checking_num += 1
        self._checking_lock.release()

        network_ok = False
        for url in self.urls:
            if self._test_host(url):
                network_ok = True
                break
            else:
                if __name__ == "__main__":
                    xlog.warn("test %s fail", url)
                time.sleep(1)

        if network_ok:
            self.last_check_time = time.time()
            self.report_ok()
            xlog.debug("network %s is ok, cost:%d ms", self.type, 1000 * (time.time() - time_now))
        else:
            xlog.warn("network %s fail", self.type)
            self.network_stat = "Fail"
            self.last_check_time = time.time()

        self._checking_lock.acquire()
        self._checking_num -= 1
        self._checking_lock.release()

    def triger_check_network(self, fail=False, force=False):
        time_now = time.time()
        if not force:
            if self._checking_num > 0:
                return

            if fail or self.network_stat != "OK":
                # Fail or unknown
                if time_now - self.last_check_time < 3:
                    return
            else:
                if time_now - self.last_check_time < 10:
                    return

        self.last_check_time = time_now
        threading.Thread(target=self._simple_check_worker).start()


IPv4 = CheckNetwork("IPv4")
IPv4.urls = [
            "https://www.microsoft.com",
            "https://www.apple.com",
            "https://code.jquery.com",
            "https://cdn.bootcss.com",
            "https://cdnjs.cloudflare.com"]
IPv4.triger_check_network()

IPv6 = CheckNetwork("IPv6")
IPv6.urls = ["http://[2001:41d0:8:e8ad::1]",
             "http://[2001:260:401:372::5f]",
             "http://[2a02:188:3e00::32]",
             "http://[2804:10:4068::202:82]"
             ]
IPv6.triger_check_network()


def report_ok(ip):
    if "." in ip:
        IPv4.report_ok()
    else:
        IPv6.report_ok()


def report_fail(ip):
    if "." in ip:
        IPv4.report_fail()
    else:
        IPv6.report_fail()


def is_ok(ip=None):
    if not ip:
        return IPv4.is_ok() or IPv6.is_ok()
    elif "." in ip:
        return IPv4.is_ok()
    else:
        return IPv6.is_ok()


if __name__ == "__main__":
    print IPv6._test_host("http://[2804:10:4068::202:82]")
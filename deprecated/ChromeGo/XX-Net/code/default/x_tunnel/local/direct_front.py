
# This front is for debug

import time
import threading
import collections
import simple_http_client
import random

import global_var as g
from xlog import getLogger
xlog = getLogger("x_tunnel")

name = "direct_front"
last_success_time = 0
last_fail_time = 0
continue_fail_num = 0
success_num = 0
fail_num = 0

def init():
    global last_success_time, last_fail_time, continue_fail_num


class FakeWorker(object):
    def update_debug_data(self, rtt, send_data_len, dlen, speed):
        pass

    def get_trace(self):
        return ""


class FakeDispatcher(object):
    def __init__(self):

        self.success_num = 0
        self.fail_num = 0
        self.continue_fail_num = 0
        self.last_fail_time = 0
        self.rtts = []
        self.last_sent = self.total_sent = 0
        self.last_received = self.total_received = 0
        #self.second_stats = Queue.deque()
        self.second_stat = {
            "rtt": 0,
            "sent": 0,
            "received": 0
        }
        self.minute_stat = {
            "rtt": 0,
            "sent": 0,
            "received": 0
        }

    def get_score(self, host=""):
        return 1

    def worker_num(self):
        return 1

    def statistic(self):
        pass


fake_dispatcher = FakeDispatcher()


def get_dispatcher(host=None):
    return fake_dispatcher


def request(method, host, schema="http", path="/", headers={}, data="", timeout=60):
    global last_success_time, last_fail_time, continue_fail_num, success_num, fail_num

    timeout = 30
    # use http to avoid cert fail
    url = "http://" + host + path
    if data:
        headers["Content-Length"] = str(len(data))

    # xlog.debug("gae_proxy %s %s", method, url)
    try:
        response = simple_http_client.request(method, url, headers, data, timeout=timeout)
        if response.status != 200:
            raise Exception("Direct request fail")
    except Exception as e:
        fail_num += 1
        continue_fail_num += 1
        last_fail_time = time.time()
        return "", 602, {}

    last_success_time = time.time()
    continue_fail_num = 0
    success_num += 1
    response.worker = FakeWorker()
    response.task = response.worker
    return response.text, response.status, response


def stop():
    pass


def set_proxy(args):
    pass


init()
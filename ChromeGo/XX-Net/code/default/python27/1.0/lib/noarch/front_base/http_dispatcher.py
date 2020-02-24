#!/usr/bin/env python
# coding:utf-8


"""
This file manage the ssl connection dispatcher
Include http/1.1 and http/2 workers.

create ssl socket, then run worker on ssl.
if ssl suppport http/2, run http/2 worker.

provide simple https request block api.
 caller don't need to known ip/ssl/http2/appid.

performance:
 get the fastest worker to process the request.
 sorted by rtt and pipeline task on load.
"""


import Queue
import operator
import threading
import time
import traceback

from utils import SimpleCondition
import simple_queue

import http_common
from http1 import Http1Worker
from http2_connection import Http2Worker


class HttpsDispatcher(object):
    idle_time = 2 * 60

    def __init__(self, logger, config, ip_manager, connection_manager,
                 http1worker=Http1Worker,
                 http2worker=Http2Worker):
        self.logger = logger
        self.config = config
        self.ip_manager = ip_manager
        self.connection_manager = connection_manager
        self.connection_manager.set_ssl_created_cb(self.on_ssl_created_cb)

        self.http1worker = http1worker
        self.http2worker = http2worker

        self.request_queue = Queue.Queue()
        self.workers = []
        self.working_tasks = {}
        self.h1_num = 0
        self.h2_num = 0
        self.last_request_time = time.time()
        self.task_count_lock = threading.Lock()
        self.task_count = 0
        self.running = True

        # for statistic
        self.success_num = 0
        self.fail_num = 0
        self.continue_fail_num = 0
        self.last_fail_time = 0
        self.rtts = []
        self.last_sent = self.total_sent = 0
        self.last_received = self.total_received = 0
        self.second_stats = Queue.deque()
        self.last_statistic_time = time.time()
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

        self.trigger_create_worker_cv = SimpleCondition()
        self.wait_a_worker_cv = simple_queue.Queue()

        threading.Thread(target=self.dispatcher).start()
        threading.Thread(target=self.create_worker_thread).start()
        threading.Thread(target=self.connection_checker).start()

    def stop(self):
        self.running = False
        self.request_queue.put(None)
        self.close_all_worker("stop")

    def on_ssl_created_cb(self, ssl_sock, check_free_work=True):
        # self.logger.debug("on_ssl_created_cb %s", ssl_sock.ip)
        if not self.running:
            ssl_sock.close()
            return

        if not ssl_sock:
            raise Exception("on_ssl_created_cb ssl_sock None")

        if ssl_sock.h2:
            worker = self.http2worker(
                self.logger, self.ip_manager, self.config, ssl_sock,
              self.close_cb, self.retry_task_cb, self._on_worker_idle_cb, self.log_debug_data)
            self.h2_num += 1
        else:
            worker = self.http1worker(
                self.logger, self.ip_manager, self.config, ssl_sock,
                self.close_cb, self.retry_task_cb, self._on_worker_idle_cb, self.log_debug_data)
            self.h1_num += 1

        self.workers.append(worker)

        if check_free_work:
            self.check_free_worker()

    def _on_worker_idle_cb(self):
        self.wait_a_worker_cv.notify()

    def create_worker_thread(self):
        while self.running:
            self.trigger_create_worker_cv.wait()

            try:
                ssl_sock = self.connection_manager.get_ssl_connection()
            except Exception as e:
                continue

            if not ssl_sock:
                # self.logger.warn("create_worker_thread get ssl_sock fail")
                continue

            try:
                self.on_ssl_created_cb(ssl_sock, check_free_work=False)
            except:
                time.sleep(10)

            idle_num = 0
            acceptable_num = 0
            for worker in self.workers:
                if worker.accept_task:
                    acceptable_num += 1

                if worker.version == "1.1":
                    if worker.accept_task:
                        idle_num += 1
                else:
                    if len(worker.streams) == 0:
                        idle_num += 1

    def get_worker(self, nowait=False):
        while self.running:
            best_score = 99999999
            best_worker = None
            idle_num = 0
            now = time.time()
            for worker in self.workers:
                if not worker.accept_task:
                    # self.logger.debug("not accept")
                    continue

                if worker.version == "1.1":
                    idle_num += 1
                else:
                    if len(worker.streams) == 0:
                        idle_num += 1

                score = worker.get_score()

                if best_score > score:
                    best_score = score
                    best_worker = worker

            if len(self.workers) < self.config.dispather_max_workers and \
                    (best_worker is None or
                    idle_num < self.config.dispather_min_idle_workers or
                    len(self.workers) < self.config.dispather_min_workers or
                    (now - best_worker.last_recv_time) < self.config.dispather_work_min_idle_time or
                    best_score > self.config.dispather_work_max_score or
                     (best_worker.version == "2" and len(best_worker.streams) >= self.config.http2_target_concurrent)):
                # self.logger.debug("trigger get more worker")
                self.trigger_create_worker_cv.notify()

            if nowait or \
                    (best_worker and (now - best_worker.last_recv_time) >= self.config.dispather_work_min_idle_time):
                # self.logger.debug("return worker")
                return best_worker

            self.wait_a_worker_cv.wait(time.time() + 1)
            # self.logger.debug("get wait_a_worker_cv")
            #time.sleep(0.1)

    def check_free_worker(self):
        # close slowest worker,
        # give change for better worker
        while True:
            slowest_score = 9999
            slowest_worker = None
            idle_num = 0
            for worker in self.workers:
                if not worker.accept_task:
                    continue

                if worker.version == "2" and len(worker.streams) > 0:
                    continue

                score = worker.get_score()
                if score < 1000:
                    idle_num += 1

                if score > slowest_score:
                    slowest_score = score
                    slowest_worker = worker

            if idle_num < self.config.dispather_max_idle_workers or \
                    idle_num < int(len(self.workers) * 0.3) or \
                    len(self.workers) < self.config.dispather_max_workers:
                return

            if slowest_worker is None:
                return
            self.close_cb(slowest_worker)

    def request(self, method, host, path, headers, body, url="", timeout=60):
        if self.task_count > self.config.max_task_num:
            self.logger.warn("task num exceed")
            time.sleep(1)
            return None

        with self.task_count_lock:
            self.task_count += 1

        try:
            # self.logger.debug("task start request")
            if not url:
                url = "%s %s%s" % (method, host, path)
            self.last_request_time = time.time()
            q = simple_queue.Queue()
            task = http_common.Task(self.logger, self.config, method, host, path, headers, body, q, url, timeout)
            task.set_state("start_request")
            self.request_queue.put(task)

            response = q.get(timeout=timeout)
            if response and response.status == 200:
                self.success_num += 1
                self.continue_fail_num = 0
            else:
                self.logger.warn("task %s %s %s timeout", method, host, path)
                self.fail_num += 1
                self.continue_fail_num += 1
                self.last_fail_time = time.time()

            task.set_state("get_response")
            return response
        finally:
            with self.task_count_lock:
                self.task_count -= 1

    def retry_task_cb(self, task, reason=""):
        self.fail_num += 1
        self.continue_fail_num += 1
        self.last_fail_time = time.time()
        self.logger.warn("retry_task_cb: %s", task.url)

        if task.responsed:
            self.logger.warn("retry but responsed. %s", task.url)
            st = traceback.extract_stack()
            stl = traceback.format_list(st)
            self.logger.warn("stack:%r", repr(stl))
            task.finish()
            return

        if task.retry_count > 10:
            task.response_fail("retry time exceed 10")
            return

        if time.time() - task.start_time > task.timeout:
            task.response_fail("retry timeout:%d" % (time.time() - task.start_time))
            return

        if not self.running:
            task.response_fail("retry but stopped.")
            return

        task.set_state("retry(%s)" % reason)
        task.retry_count += 1
        self.request_queue.put(task)

    def dispatcher(self):
        while self.running:
            start_time = time.time()
            try:
                task = self.request_queue.get(True)
                if task is None:
                    # exit
                    break
            except Exception as e:
                self.logger.exception("http_dispatcher dispatcher request_queue.get fail:%r", e)
                continue
            get_time = time.time()
            get_cost = get_time - start_time

            task.set_state("get_task(%d)" % get_cost)
            try:
                worker = self.get_worker()
            except Exception as e:
                self.logger.warn("get worker fail:%r", e)
                task.response_fail(reason="get worker fail:%r" % e)
                continue

            if worker is None:
                # can send because exit.
                self.logger.warn("http_dispatcher get None worker")
                task.response_fail("get worker fail.")
                continue

            get_worker_time = time.time()
            get_cost = get_worker_time - get_time
            task.set_state("get_worker(%d):%s" % (get_cost, worker.ip))
            task.worker = worker
            try:
                worker.request(task)
            except Exception as e:
                self.logger.exception("dispatch request:%r", e)

        # wait up threads to exit.
        self.wait_a_worker_cv.notify()
        self.trigger_create_worker_cv.notify()

    def connection_checker(self):
        while self.running:
            now = time.time()
            try:
                for worker in list(self.workers):
                    if worker.version == "1.1":
                        continue

                    worker.check_active(now)
            except Exception as e:
                self.logger.exception("check worker except:%r")

            time.sleep(1)

    def is_idle(self):
        return time.time() - self.last_request_time > self.idle_time

    def close_cb(self, worker):
        try:
            self.workers.remove(worker)
            if worker.version == "2":
                self.h2_num -= 1
            else:
                self.h1_num -= 1
        except:
            pass

    def close_all_worker(self, reason="close all worker"):
        for w in list(self.workers):
            if w.accept_task:
                w.close(reason)

        self.workers = []
        self.h1_num = 0
        self.h2_num = 0

    def log_debug_data(self, rtt, sent, received):
        self.rtts.append(rtt)
        self.total_sent += sent
        self.total_received += received

    def statistic(self):
        now = time.time()
        if now > self.last_statistic_time + 60:
            rtt = 0
            sent = 0
            received = 0
            for stat in self.second_stats:
                rtt = max(rtt, stat["rtt"])
                sent += stat["sent"]
                received += stat["received"]
            self.minute_stat = {
                "rtt": rtt,
                "sent": sent,
                "received": received
            }
            self.second_stats = Queue.deque()
            self.last_statistic_time = now

        if len(self.rtts):
            rtt = max(self.rtts)
        else:
            rtt = 0

        self.second_stat = {
            "rtt": rtt,
            "sent": self.total_sent - self.last_sent,
            "received": self.total_received - self.last_received
        }
        self.rtts = []
        self.last_sent = self.total_sent
        self.last_received = self.total_received
        self.second_stats.append(self.second_stat)

    def worker_num(self):
        return len(self.workers)

    def get_score(self):
        now = time.time()
        if now - self.last_fail_time < 60 and \
                self.continue_fail_num > 10:
            return None

        worker = self.get_worker(nowait=True)
        if not worker:
            return None

        return worker.get_score() * self.config.dispather_score_factor

    def to_string(self):
        now = time.time()
        worker_rate = {}
        for w in self.workers:
            worker_rate[w] = w.get_rtt_rate()

        w_r = sorted(worker_rate.items(), key=operator.itemgetter(1))

        out_str = 'thread num:%d\r\n' % threading.activeCount()
        for w, r in w_r:
            out_str += "%s score:%d rtt:%d running:%d accept:%d live:%d inactive:%d processed:%d" % \
                       (w.ip, w.get_score(), w.rtt, w.keep_running,  w.accept_task,
                        (now-w.ssl_sock.create_time), (now-w.last_recv_time), w.processed_tasks)
            if w.version == "2":
                out_str += " continue_timeout:%d streams:%d ping_on_way:%d remote_win:%d send_queue:%d\r\n" % \
                   (w.continue_timeout, len(w.streams), w.ping_on_way, w.remote_window_size, w.send_queue.qsize())

            elif w.version == "1.1":
                out_str += " Trace:%s" % w.get_trace()

            out_str += "\r\n Speed:"
            for speed in w.speed_history:
               out_str += "%d," % speed

            out_str += "\r\n"

        out_str += "\r\n working_tasks:\r\n"
        for unique_id in self.working_tasks:
            task = self.working_tasks[unique_id]
            out_str += task.to_string()

        return out_str

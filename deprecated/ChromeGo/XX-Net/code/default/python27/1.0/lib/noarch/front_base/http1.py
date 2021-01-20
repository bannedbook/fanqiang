
import Queue
import threading

from http_common import *
import simple_http_client


class Http1Worker(HttpWorker):
    version = "1.1"

    def __init__(self, logger, ip_manager, config, ssl_sock, close_cb, retry_task_cb, idle_cb, log_debug_data):
        super(Http1Worker, self).__init__(logger, ip_manager, config, ssl_sock,
                                          close_cb, retry_task_cb, idle_cb, log_debug_data)

        self.task = None
        self.request_onway = False
        self.transfered_size = 0
        self.trace_time = []
        self.trace_time.append([ssl_sock.create_time, "connect"])
        self.record_active("init")

        self.task_queue = Queue.Queue()
        threading.Thread(target=self.work_loop).start()
        self.idle_cb()

        if self.config.http1_first_ping_wait or \
            self.config.http1_ping_interval or \
            self.config.http1_idle_time:
            threading.Thread(target=self.keep_alive_thread).start()

    def record_active(self, active=""):
        self.trace_time.append([time.time(), active])
        # self.logger.debug("%s stat:%s", self.ip, active)

    def get_trace(self):
        out_list = []
        last_time = self.trace_time[0][0]
        for t, stat in self.trace_time:
            time_diff = int((t - last_time) * 1000)
            last_time = t
            out_list.append(" %d:%s" % (time_diff, stat))
        out_list.append(":%d" % ((time.time() - last_time) * 1000))
        out_list.append(" processed:%d" % self.processed_tasks)
        out_list.append(" transfered:%d" % self.transfered_size)
        out_list.append(" sni:%s" % self.ssl_sock.sni)
        return ",".join(out_list)

    def get_rtt_rate(self):
        return self.rtt + 100

    def request(self, task):
        self.accept_task = False
        self.task = task
        self.task_queue.put(task)

    def keep_alive_thread(self):
        while time.time() - self.ssl_sock.create_time < self.config.http1_first_ping_wait:
            if not self.keep_running:
                self.close("exit ")
                return
            time.sleep(1)

        if self.config.http1_first_ping_wait and self.processed_tasks == 0:
            self.task_queue.put("ping")

        if self.config.http1_ping_interval:
            while self.keep_running:
                time_to_ping = max(self.config.http1_ping_interval - (time.time() - self.last_recv_time), 0.2)
                time.sleep(time_to_ping)

                if not self.request_onway and \
                        time.time() - self.last_recv_time > self.config.http1_ping_interval - 1:
                    self.task_queue.put("ping")
                    time.sleep(1)

        elif self.config.http1_idle_time:
            while self.keep_running:
                time_to_sleep = max(self.config.http1_idle_time - (time.time() - self.last_recv_time), 0.2)
                time.sleep(time_to_sleep)

                if not self.request_onway and time.time() - self.last_recv_time > self.config.http1_idle_time:
                    self.close("idle timeout")
                    return

    def work_loop(self):
        while self.keep_running:
            task = self.task_queue.get(True)
            if not task:
                # None task means exit
                self.accept_task = False
                self.keep_running = False
                return

            if task == "ping":
                if not self.head_request():
                    self.ip_manager.recheck_ip(self.ssl_sock.ip)
                    self.close("keep alive")
                    return

                self.last_recv_time = time.time()
                continue

            # self.logger.debug("http1 get task")
            time_now = time.time()
            if time_now - self.last_recv_time > self.config.http1_idle_time:
                self.logger.warn("get task but inactive time:%d", time_now - self.last_recv_time)
                self.task = task
                self.close("inactive timeout %d" % (time_now - self.last_recv_time))
                return

            self.request_task(task)
            self.request_onway = False
            self.last_send_time = time_now
            self.last_recv_time = time_now

            if self.processed_tasks > self.config.http1_max_process_tasks:
                self.close("lift end.")
                return

    def request_task(self, task):
        timeout = task.timeout
        self.request_onway = True
        start_time = time.time()

        self.record_active("request")
        task.set_state("h1_req")

        task.headers['Host'] = self.get_host(task.host)

        task.headers["Content-Length"] = len(task.body)
        request_data = '%s %s HTTP/1.1\r\n' % (task.method, task.path)
        request_data += ''.join('%s: %s\r\n' % (k, v) for k, v in task.headers.items())
        request_data += '\r\n'

        try:
            self.ssl_sock.send(request_data.encode())
            payload_len = len(task.body)
            start = 0
            while start < payload_len:
                send_size = min(payload_len - start, 65535)
                sended = self.ssl_sock.send(task.body[start:start+send_size])
                start += sended

            task.set_state("h1_req_sended")

            response = simple_http_client.Response(self.ssl_sock)

            response.begin(timeout=timeout)
            task.set_state("response_begin")

        except Exception as e:
            self.logger.warn("%s h1_request:%r inactive_time:%d task.timeout:%d",
                             self.ip, e, time.time() - self.last_recv_time, task.timeout)
            self.logger.warn('%s trace:%s', self.ip, self.get_trace())

            self.retry_task_cb(task)
            self.task = None
            self.close("down fail")
            return

        task.set_state("h1_get_head")

        time_left = timeout - (time.time() - start_time)

        if task.method == "HEAD" or response.status in [204, 304]:
            response.content_length = 0

        response.ssl_sock = self.ssl_sock
        response.task = task
        response.worker = self
        task.content_length = response.content_length
        task.responsed = True
        task.queue.put(response)

        try:
            read_target = int(response.content_length)
        except:
            read_target = 0

        data_len = 0
        while True:
            try:
                data = response.read(timeout=time_left)
                if not data:
                    break
            except Exception as e:
                self.logger.warn("read fail, ip:%s, chunk:%d url:%s task.timeout:%d e:%r",
                               self.ip, response.chunked, task.url, task.timeout, e)
                self.logger.warn('%s trace:%s', self.ip, self.get_trace())
                self.close("down fail")
                return

            task.put_data(data)
            length = len(data)
            data_len += length
            if read_target and data_len >= read_target:
                break

        if read_target > data_len:
            self.logger.warn("read fail, ip:%s, chunk:%d url:%s task.timeout:%d e:%r",
                           self.ip, response.chunked, task.url, task.timeout, e)
            self.ip_manager.recheck_ip(self.ssl_sock.ip)
            self.close("down fail")

        task.finish()

        self.ssl_sock.received_size += data_len
        time_cost = (time.time() - start_time)
        if time_cost != 0:
            speed = data_len / time_cost
            task.set_state("h1_finish[SP:%d]" % speed)

        self.transfered_size += len(request_data) + data_len
        self.task = None
        self.accept_task = True
        self.idle_cb()
        self.processed_tasks += 1
        self.last_recv_time = time.time()
        self.record_active("Res")

    def head_request(self):
        if not self.ssl_sock.host:
            # self.logger.warn("try head but no host set")
            return True

        # for keep alive, not work now.
        self.request_onway = True
        self.record_active("head")
        start_time = time.time()
        # self.logger.debug("head request %s", self.ip)
        request_data = 'GET / HTTP/1.1\r\nHost: %s\r\n\r\n' % self.ssl_sock.host

        try:
            data = request_data.encode()
            ret = self.ssl_sock.send(data)
            if ret != len(data):
                self.logger.warn("h1 head send len:%r %d %s", ret, len(data), self.ip)
                self.logger.warn('%s trace:%s', self.ip, self.get_trace())
                return False
            response = simple_http_client.Response(self.ssl_sock)
            response.begin(timeout=5)

            status = response.status
            if status != 200:
                self.logger.warn("%s host:%s head fail status:%d", self.ip, self.ssl_sock.host, status)
                return False

            content = response.readall(timeout=5)
            self.record_active("head end")
            self.rtt = (time.time() - start_time) * 1000
            #self.ip_manager.update_ip(self.ip, self.rtt)
            return True
        except Exception as e:
            self.logger.warn("h1 %s HEAD keep alive request fail:%r", self.ssl_sock.ip, e)
            self.logger.warn('%s trace:%s', self.ip, self.get_trace())
            self.close("down fail")
        finally:
            self.request_onway = False

    def close(self, reason=""):
        # Notify loop to exit
        # This function may be call by out side http2
        # When gae_proxy found the appid or ip is wrong
        self.accept_task = False
        self.keep_running = False
        self.task_queue.put(None)

        if self.task is not None:
            if self.task.responsed:
                self.task.finish()
            else:
                self.retry_task_cb(self.task)
            self.task = None

        super(Http1Worker, self).close(reason)
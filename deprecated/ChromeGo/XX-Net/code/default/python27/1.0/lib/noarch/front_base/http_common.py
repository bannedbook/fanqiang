import time

import simple_queue
import simple_http_client


class Task(object):
    def __init__(self, logger, config, method, host, path, headers, body, queue, url, timeout):
        self.logger = logger
        self.config = config
        self.method = method
        self.host = host
        self.path = path
        self.headers = headers
        self.body = body
        self.queue = queue
        self.url = url
        self.timeout = timeout
        self.start_time = time.time()
        self.unique_id = "%s:%f" % (url, self.start_time)
        self.trace_time = []
        self.body_queue = simple_queue.Queue()
        self.body_len = 0
        self.body_readed = 0
        self.content_length = None
        self.worker = None
        self.read_buffers = []
        self.read_buffer_len = 0

        self.responsed = False
        self.finished = False
        self.retry_count = 0

    def to_string(self):
        out_str = " Task:%s\r\n" % self.url
        out_str += "   responsed:%d" % self.responsed
        out_str += "   retry_count:%d" % self.retry_count
        out_str += "   start_time:%d" % (time.time() - self.start_time)
        out_str += "   body_readed:%d\r\n" % self.body_readed
        out_str += "   Trace:%s" % self.get_trace()
        out_str += "\r\n"
        return out_str

    def put_data(self, data):
        # hyper H2
        if isinstance(data, memoryview):
            data = data.tobytes()
        self.body_queue.put(data)
        self.body_len += len(data)

    def read(self, size=None):
        # fail or cloe if return ""
        if self.body_readed == self.content_length:
            return b''

        if size:
            while self.read_buffer_len < size:
                data = self.body_queue.get(self.timeout)
                if not data:
                    return b''

                self.read_buffers.append(data)
                self.read_buffer_len += len(data)

            if len(self.read_buffers[0]) == size:
                data = self.read_buffers[0]
                self.read_buffers.pop(0)
                self.read_buffer_len -= size
            elif len(self.read_buffers[0]) > size:
                data = self.read_buffers[0][:size]
                self.read_buffers[0] = self.read_buffers[0][size:]
                self.read_buffer_len -= size
            else:
                buff = bytearray(self.read_buffer_len)
                buff_view = memoryview(buff)
                p = 0
                for data in self.read_buffers:
                    buff_view[p:p+len(data)] = data
                    p += len(data)

                if self.read_buffer_len == size:
                    self.read_buffers = []
                    self.read_buffer_len = 0
                    data = buff_view.tobytes()
                else:
                    data = buff_view[:size].tobytes()

                    self.read_buffers = [buff_view[size:].tobytes()]
                    self.read_buffer_len -= size

        else:
            if self.read_buffers:
                data = self.read_buffers.pop(0)
                self.read_buffer_len -= len(data)
            else:
                data = self.body_queue.get(self.timeout)
                if not data:
                    return b''

        self.body_readed += len(data)
        return data

    def read_all(self):
        if self.content_length:
            left_body = int(self.content_length) - self.body_readed

            buff = bytearray(left_body)
            buff_view = memoryview(buff)
            p = 0
            for data in self.read_buffers:
                buff_view[p:p+len(data)] = data
                p += len(data)

            self.read_buffers = []
            self.read_buffer_len = 0

            while p < left_body:
                data = self.read()
                if not data:
                    break

                buff_view[p:p + len(data)] = data[0:len(data)]
                p += len(data)

            self.body_readed += p
            return buff_view[:p].tobytes()
        else:
            out = list()
            while True:
                data = self.read()
                if not data:
                    break
                out.append(data)
            return "".join(out)

    def set_state(self, stat):
        # for debug trace
        time_now = time.time()
        self.trace_time.append((time_now, stat))
        if self.config.show_state_debug:
            self.logger.debug("%s stat:%s", self.unique_id, stat)
        return time_now

    def get_trace(self):
        out_list = []
        last_time = self.start_time
        for t, stat in self.trace_time:
            time_diff = int((t - last_time) * 1000)
            last_time = t
            out_list.append("%d:%s" % (time_diff, stat))
        out_list.append(":%d" % ((time.time()-last_time)*1000))
        return ",".join(out_list)

    def response_fail(self, reason=""):
        if self.responsed:
            self.logger.error("http_common responsed_fail but responed.%s", self.url)
            self.put_data("")
            return

        self.responsed = True
        err_text = "response_fail:%s" % reason
        self.logger.warn("%s %s", self.url, err_text)
        res = simple_http_client.BaseResponse(body=err_text)
        res.task = self
        res.worker = self.worker
        self.queue.put(res)
        self.finish()

    def finish(self):
        if self.finished:
            return

        self.put_data("")
        self.finished = True


class HttpWorker(object):
    def __init__(self, logger, ip_manager, config, ssl_sock, close_cb, retry_task_cb, idle_cb, log_debug_data):
        self.logger = logger
        self.ip_manager = ip_manager
        self.config = config
        self.ssl_sock = ssl_sock
        self.init_rtt = ssl_sock.handshake_time / 3
        self.rtt = self.init_rtt
        self.speed = 1
        self.ip = ssl_sock.ip
        self.close_cb = close_cb
        self.retry_task_cb = retry_task_cb
        self.idle_cb = idle_cb
        self.log_debug_data = log_debug_data
        self.accept_task = True
        self.keep_running = True
        self.processed_tasks = 0
        self.speed_history = []
        self.last_recv_time = self.ssl_sock.create_time
        self.last_send_time = self.ssl_sock.create_time

    def update_debug_data(self, rtt, sent, received, speed):
        self.rtt = rtt
        self.speed = speed
        self.speed_history.append(speed)
        self.log_debug_data(rtt, sent, received)

    def close(self, reason):
        self.accept_task = False
        self.keep_running = False
        self.ssl_sock.close()
        if reason not in ["idle timeout"]:
            self.logger.debug("%s worker close:%s", self.ip, reason)
        self.ip_manager.report_connect_closed(self.ssl_sock.ip, reason)
        self.close_cb(self)

    def get_score(self):
        now = time.time()
        inactive_time = now - self.last_recv_time

        rtt = self.rtt
        if inactive_time > 30:
            if rtt > 1000:
                rtt = 1000

        if self.version == "1.1":
            rtt += 100
        else:
            rtt += len(self.streams) * 500

        if inactive_time > 1:
            score = rtt
        elif inactive_time < 0.1:
            score = rtt + 1000
        else:
            # inactive_time < 2
            score = rtt + (1 / inactive_time) * 1000

        return score

    def get_host(self, task_host):
        if task_host:
            return task_host
        else:
            return self.ssl_sock.host

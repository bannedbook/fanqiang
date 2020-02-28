import os
import sys
import socket
import operator
import time
import threading
import struct

if __name__ == '__main__':
    current_path = os.path.dirname(os.path.abspath(__file__))
    root_path = os.path.abspath(os.path.join(current_path, os.pardir))
    python_path = os.path.join(root_path, 'python27', '1.0')
    noarch_lib = os.path.join(python_path, 'lib', 'noarch')
    sys.path.append(noarch_lib)


from socket_wrap import SocketWrap
import simple_queue
import socks
import global_var as g
from xlog import getLogger
xlog = getLogger("smart_router")


def load_proxy_config():
    global default_socket
    if g.config.PROXY_ENABLE:

        if g.config.PROXY_TYPE == "HTTP":
            proxy_type = socks.HTTP
        elif g.config.PROXY_TYPE == "SOCKS4":
            proxy_type = socks.SOCKS4
        elif g.config.PROXY_TYPE == "SOCKS5":
            proxy_type = socks.SOCKS5
        else:
            xlog.error("proxy type %s unknown, disable proxy", g.config.PROXY_TYPE)
            raise Exception()

        socks.set_default_proxy(proxy_type, g.config.PROXY_HOST, g.config.PROXY_PORT,
                                g.config.PROXY_USER, g.config.PROXY_PASSWD)


class ConnectManager(object):
    def __init__(self, connection_timeout=15, connect_threads=3, connect_timeout=5):
        self.lock = threading.Lock()
        self.cache = {}
        # host => [ { "conn":.., "create_time" }
        #    ... ]
        self.connection_timeout = connection_timeout
        self.connect_timeout = connect_timeout
        self.connect_threads = connect_threads

        self.running = True
        threading.Thread(target=self.check_thread).start()

    def stop(self):
        self.running = False

    def check_thread(self):
        while self.running:
            time_now = time.time()
            with self.lock:
                for host_port in list(self.cache.keys()):
                    try:
                        cache = self.cache[host_port]
                        for cc in list(cache):
                            if time_now - cc["create_time"] > self.connection_timeout:
                                cache.remove(cc)
                                cc["conn"].close()
                    except:
                        pass

            time.sleep(1)

    def add_sock(self, host_port, sock):
        with self.lock:
            if host_port not in self.cache:
                self.cache[host_port] = []
            self.cache[host_port].append({"create_time": time.time(), "conn": sock})

    def get_sock_from_cache(self, host_port):
        time_now = time.time()
        with self.lock:
            if host_port in self.cache:
                cache = self.cache[host_port]
                while len(cache):
                    try:
                        cc = cache.pop(0)
                        if time_now - cc["create_time"] > self.connection_timeout:
                            cc["conn"].close()
                            continue

                        return cc["conn"]
                    except Exception as e:
                        xlog.exception("get_conn:%r", e)
                        break

    def create_connect(self, queue, host, ip, port, timeout=5):
        if int(g.config.PROXY_ENABLE):
            sock = socks.socksocket(socket.AF_INET if ':' not in ip else socket.AF_INET6)
        else:
            sock = socket.socket(socket.AF_INET if ':' not in ip else socket.AF_INET6)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        # set struct linger{l_onoff=1,l_linger=0} to avoid 10048 socket error
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack('ii', 1, 0))
        # resize socket recv buffer 8K->32K to improve browser releated application performance
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 32 * 1024)
        sock.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, True)
        sock.settimeout(timeout)

        start_time = time.time()
        try:
            sock.connect((ip, port))
            time_cost = (time.time() - start_time) * 1000
            # xlog.debug("connect %s %s:%d time:%d", host, ip, port, time_cost)
            g.ip_cache.update_connect_time(ip, port, time_cost)
            s = SocketWrap(sock, ip, port, host)
            host_port = "%s:%d" % (host, port)
            self.add_sock(host_port, s)
            queue.put(True)
        except Exception as e:
            # xlog.debug("connect %s %s:%d fail:%r", host, ip, port, e)
            g.ip_cache.report_connect_fail(ip, port)
            queue.put(False)

    def get_conn(self, host, ips, port, timeout=5):
        # xlog.debug("connect to %s:%d %r", host, port, ips)
        end_time = time.time() + timeout
        host_port = "%s:%d" % (host, port)
        sock = self.get_sock_from_cache(host_port)
        if sock:
            return sock

        ip_rate = {}
        for ipd in ips:
            ipl = ipd.split("|")
            ip = ipl[0]
            connect_time = g.ip_cache.get_connect_time(ip, port)
            if connect_time >= 8000:
                continue

            ip_rate[ip] = connect_time

        if not ip_rate:
            return None

        ip_time = sorted(ip_rate.items(), key=operator.itemgetter(1))
        ordered_ips = [ip for ip, rate in ip_time]

        wait_queue = simple_queue.Queue()
        wait_t = 0.2
        for ip in ordered_ips:
            threading.Thread(target=self.create_connect, args=(wait_queue, host, ip, port)).start()
            status = wait_queue.get(wait_t)
            if status:
                sock = self.get_sock_from_cache(host_port)
                if sock:
                    return sock
            else:
                time.sleep(wait_t)
                wait_t += 0.1

        while True:
            time_left = end_time - time.time()
            if time_left <= 0:
                return self.get_sock_from_cache(host_port)

            status = wait_queue.get(time_left)
            if status:
                sock = self.get_sock_from_cache(host_port)
                if sock:
                    return sock
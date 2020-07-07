import time
import socket
import struct
import urlparse
import select

import pac_server
import global_var as g
from socket_wrap import SocketWrap
import utils
from smart_route import handle_ip_proxy, handle_domain_proxy, netloc_to_host_port
from xlog import getLogger
xlog = getLogger("smart_router")

SO_ORIGINAL_DST = 80


class ProxyServer():
    handle_num = 0

    def __init__(self, sock, client, args):
        self.conn = sock
        self.rfile = socket._fileobject(self.conn, "rb", -1)
        self.wfile = socket._fileobject(self.conn, "wb", 0)
        self.client_address = client

        self.read_buffer = ""
        self.buffer_start = 0
        self.support_redirect = True

    def try_redirect(self):
        if not self.support_redirect:
            return False

        try:
            dst = self.conn.getsockopt(socket.SOL_IP, SO_ORIGINAL_DST, 16)
        except:
            self.support_redirect = False
            return False

        try:
            dst_port, srv_ip = struct.unpack("!2xH4s8x", dst)
            ip_str = socket.inet_ntoa(srv_ip)
            if dst_port != g.config.proxy_port and not utils.is_private_ip(ip_str):
                xlog.debug("Redirect to:%s:%d from:%s", ip_str, dst_port, self.client_address)
                handle_ip_proxy(self.conn, ip_str, dst_port, self.client_address)
                return True
            else:
                return False
        except Exception as e:
            xlog.exception("redirect except:%r", e)

        return True

    def handle(self):
        self.__class__.handle_num += 1

        if self.try_redirect():
            return

        sockets = [self.conn]
        try:
            r, w, e = select.select(sockets, [], [])
            socks_version = self.conn.recv(1, socket.MSG_PEEK)
            if not socks_version:
                return

            if socks_version == "\x04":
                self.socks4_handler()
            elif socks_version == "\x05":
                self.socks5_handler()
            elif socks_version == "C":
                self.https_handler()
            elif socks_version in ["G", "P", "D", "O", "H", "T"]:
                self.http_handler()
            else:
                xlog.warn("socks version:%s[%s] not supported", socks_version, utils.str2hex(socks_version))
                return

        except socket.error as e:
            xlog.warn('socks handler read error:%r', e)
        except Exception as e:
            xlog.exception("any err:%r", e)

    def read_null_end_line(self):
        sock = self.conn
        sock.setblocking(0)
        try:
            while True:
                n1 = self.read_buffer.find("\x00", self.buffer_start)
                if n1 > -1:
                    line = self.read_buffer[self.buffer_start:n1]
                    self.buffer_start = n1 + 1
                    return line

                try:
                    data = sock.recv(8192)
                except socket.error as e:
                    # logging.exception("e:%r", e)
                    if e.errno in [2, 11, 10035]:
                        time.sleep(0.01)
                        continue
                    else:
                        raise e

                self.read_buffer += data
        finally:
            sock.setblocking(1)

    def read_crlf_line(self):
        sock = self.conn
        sock.setblocking(0)
        try:
            while True:
                n1 = self.read_buffer.find("\r\n", self.buffer_start)
                if n1 > -1:
                    line = self.read_buffer[self.buffer_start:n1]
                    self.buffer_start = n1 + 2
                    return line

                try:
                    data = sock.recv(8192)
                except socket.error as e:
                    # logging.exception("e:%r", e)
                    if e.errno in [2, 11, 10035]:
                        time.sleep(0.01)
                        continue
                    else:
                        raise e

                self.read_buffer += data
        finally:
            sock.setblocking(1)

    def read_headers(self):
        sock = self.conn
        sock.setblocking(0)
        try:
            while True:
                if self.read_buffer[self.buffer_start:] == "\r\n":
                    self.buffer_start += 2
                    return ""

                n1 = self.read_buffer.find("\r\n\r\n", self.buffer_start)
                if n1 > -1:
                    block = self.read_buffer[self.buffer_start:n1]
                    self.buffer_start = n1 + 4
                    return block

                try:
                    data = sock.recv(8192)
                except socket.error as e:
                    # logging.exception("e:%r", e)
                    if e.errno in [2, 11, 10035]:
                        time.sleep(0.01)
                        continue
                    else:
                        raise e

                self.read_buffer += data
        finally:
            sock.setblocking(1)

    def read_bytes(self, size):
        sock = self.conn
        sock.setblocking(1)
        try:
            while True:
                left = len(self.read_buffer) - self.buffer_start
                if left >= size:
                    break

                need = size - left

                try:
                    data = sock.recv(need)
                except socket.error as e:
                    # logging.exception("e:%r", e)
                    if e.errno in [2, 11, 10035]:
                        time.sleep(0.01)
                        continue
                    else:
                        raise e

                if len(data):
                    self.read_buffer += data
                else:
                    raise socket.error("recv fail")
        finally:
            sock.setblocking(1)

        data = self.read_buffer[self.buffer_start:self.buffer_start + size]
        self.buffer_start += size
        return data

    def socks4_handler(self):
        # Socks4 or Socks4a
        sock = self.conn
        socks_version = ord(self.read_bytes(1))
        cmd = ord(self.read_bytes(1))
        if cmd != 1:
            xlog.warn("Socks4 cmd:%d not supported", cmd)
            return

        data = self.read_bytes(6)
        port = struct.unpack(">H", data[0:2])[0]
        addr_pack = data[2:6]
        if addr_pack[0:3] == '\x00\x00\x00' and addr_pack[3] != '\x00':
            domain_mode = True
        else:
            ip = socket.inet_ntoa(addr_pack)
            domain_mode = False

        user_id = self.read_null_end_line()
        if len(user_id):
            xlog.debug("Socks4 user_id:%s", user_id)

        if domain_mode:
            addr = self.read_null_end_line()
        else:
            addr = ip

        reply = b"\x00\x5a" + addr_pack + struct.pack(">H", port)
        sock.send(reply)

        # xlog.debug("Socks4:%r to %s:%d", self.client_address, addr, port)
        handle_ip_proxy(sock, addr, port, self.client_address)

    def socks5_handler(self):
        sock = self.conn
        socks_version = ord(self.read_bytes(1))
        auth_mode_num = ord(self.read_bytes(1))
        data = self.read_bytes(auth_mode_num)

        sock.send(b"\x05\x00")  # socks version 5, no auth needed.
        try:
            data = self.read_bytes(4)
        except Exception as e:
            xlog.debug("socks5 auth num:%d, list:%s", auth_mode_num, utils.str2hex(data))
            xlog.warn("socks5 protocol error:%r", e)
            return

        socks_version = ord(data[0])
        if socks_version != 5:
            xlog.warn("request version:%d error", socks_version)
            return

        command = ord(data[1])
        if command != 1:  # 1. Tcp connect
            xlog.warn("request not supported command mode:%d", command)
            sock.send(b"\x05\x07\x00\x01")  # Command not supported
            return

        addrtype_pack = data[3]
        addrtype = ord(addrtype_pack)
        if addrtype == 1:  # IPv4
            addr_pack = self.read_bytes(4)
            addr = socket.inet_ntoa(addr_pack)
        elif addrtype == 3:  # Domain name
            domain_len_pack = self.read_bytes(1)[0]
            domain_len = ord(domain_len_pack)
            domain = self.read_bytes(domain_len)
            addr_pack = domain_len_pack + domain
            addr = domain
        elif addrtype == 4:  # IPv6
            addr_pack = self.read_bytes(16)
            addr = socket.inet_ntop(socket.AF_INET6, addr_pack)
        else:
            xlog.warn("request address type unknown:%d", addrtype)
            sock.send(b"\x05\x07\x00\x01")  # Command not supported
            return

        port = struct.unpack('>H', self.rfile.read(2))[0]

        # xlog.debug("socks5 %r connect to %s:%d", self.client_address, addr, port)
        reply = b"\x05\x00\x00" + addrtype_pack + addr_pack + struct.pack(">H", port)
        sock.send(reply)

        if addrtype in [1, 4]:
            handle_ip_proxy(sock, addr, port, self.client_address)
        else:
            handle_domain_proxy(sock, addr, port, self.client_address)

    def https_handler(self):
        line = self.read_crlf_line()
        line = line.decode('iso-8859-1')
        words = line.split()
        if len(words) == 3:
            command, path, version = words
        elif len(words) == 2:
            command, path = words
            version = "HTTP/1.1"
        else:
            xlog.warn("https req line fail:%s", line)
            return

        if command != "CONNECT":
            xlog.warn("https req line fail:%s", line)
            return

        host, _, port = path.rpartition(':')
        host = host.encode()
        port = int(port)

        header_block = self.read_headers()
        sock = self.conn

        # xlog.debug("https %r connect to %s:%d", self.client_address, host, port)
        sock.send(b'HTTP/1.1 200 OK\r\n\r\n')

        handle_domain_proxy(sock, host, port, self.client_address)

    def http_handler(self):
        req_data = self.conn.recv(65537, socket.MSG_PEEK)
        rp = req_data.split("\r\n")
        req_line = rp[0]

        words = req_line.split()
        if len(words) == 3:
            method, url, http_version = words
        elif len(words) == 2:
            method, url = words
            http_version = "HTTP/1.1"
        else:
            xlog.warn("http req line fail:%s", req_line)
            return

        if url.lower().startswith("http://"):
            o = urlparse.urlparse(url)
            host, port = netloc_to_host_port(o.netloc)

            url_prex_len = url[7:].find("/")
            if url_prex_len >= 0:
                url_prex_len += 7
                path = url[url_prex_len:]
            else:
                url_prex_len = len(url)
                path = "/"
        else:
            # not proxy request, should be PAC
            xlog.debug("PAC %s %s from:%s", method, url, self.client_address)
            handler = pac_server.PacHandler(self.conn, self.client_address, None, xlog)
            return handler.handle()

        #req_d = self.conn.recv(len(req_line))
        #req_d = req_d.replace(url, path)

        sock = SocketWrap(self.conn, self.client_address[0], self.client_address[1])
        sock.replace_pattern = [url[:url_prex_len], ""]

        xlog.debug("http %r connect to %s:%d %s %s", self.client_address, host, port, method, path)
        handle_domain_proxy(sock, host, port, self.client_address)

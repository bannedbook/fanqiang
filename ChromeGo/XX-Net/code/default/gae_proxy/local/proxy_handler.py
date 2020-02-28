#!/usr/bin/env python
# coding:utf-8


"""
GAEProxyHandler is the handler of http proxy port. default to 8087

    if HTTP request:
        do_METHOD()

    elif HTTPS request:
        do_CONNECT()


What is Direct mode:
    if user access google site like www.google.com, client.google.com,
    we don't need forward request to GAE server.
    we can send the original request to google ip directly.
    because most google ip act as general front server.

    Youtube content server do not support direct mode.

    look direct_handler.py for more detail.

What GAE mode:
    Google App Engine support urlfetch for proxy.
    every google account can apply 12 appid.
    after deploy server code under gae_proxy/server/gae to GAE server, user can
    use GAE server as http proxy.

    Here is the global link view:

     Browser => GAE_proxy => GAE server => target http/https server.

    look gae_hander.py for more detail.
"""

import errno
import socket
import ssl
import urlparse
import OpenSSL
NetWorkIOError = (socket.error, ssl.SSLError, OpenSSL.SSL.Error, OSError)


from xlog import getLogger
xlog = getLogger("gae_proxy")
import simple_http_client
import simple_http_server
from cert_util import CertUtil
import gae_handler
import direct_handler
import web_control
import check_local_network
from front import front


class GAEProxyHandler(simple_http_server.HttpServerHandler):
    gae_support_methods = tuple(["GET", "POST", "HEAD", "PUT", "DELETE", "PATCH"])
    # GAE don't support command like OPTION

    bufsize = 65535
    local_names = []
    self_check_response_data = "HTTP/1.1 200 OK\r\n" \
                               "Access-Control-Allow-Origin: *\r\n" \
                               "Cache-Control: no-cache, no-store, must-revalidate\r\n" \
                               "Pragma: no-cache\r\n" \
                               "Expires: 0\r\n" \
                               "Content-Type: text/plain\r\n" \
                               "Keep-Alive:\r\n" \
                               "Persist:\r\n" \
                               "Connection: Keep-Alive, Persist\r\n" \
                               "Content-Length: 2\r\n\r\nOK"
    fake_host = web_control.get_fake_host()

    def setup(self):
        self.__class__.do_GET = self.__class__.do_METHOD
        self.__class__.do_PUT = self.__class__.do_METHOD
        self.__class__.do_POST = self.__class__.do_METHOD
        self.__class__.do_HEAD = self.__class__.do_METHOD
        self.__class__.do_DELETE = self.__class__.do_METHOD
        self.__class__.do_OPTIONS = self.__class__.do_METHOD

    def forward_local(self):
        """
        If browser send localhost:xxx request to GAE_proxy,
        we forward it to localhost.
        """
        request_headers = dict((k.title(), v) for k, v in self.headers.items())
        payload = b''
        if 'Content-Length' in request_headers:
            try:
                payload_len = int(request_headers.get('Content-Length', 0))
                payload = self.rfile.read(payload_len)
            except Exception as e:
                xlog.warn('forward_local read payload failed:%s', e)
                return

        response = simple_http_client.request(self.command, self.path, request_headers, payload)
        if not response:
            xlog.warn("forward_local fail, command:%s, path:%s, headers: %s, payload: %s",
                self.command, self.path, request_headers, payload)
            return

        out_list = []
        out_list.append("HTTP/1.1 %d\r\n" % response.status)
        for key in response.headers:
            key = key.title()
            out_list.append("%s: %s\r\n" % (key, response.headers[key]))
        out_list.append("\r\n")
        out_list.append(response.text)

        self.wfile.write("".join(out_list))

    def send_method_allows(self, headers, payload):
        xlog.debug("send method allow list for:%s %s", self.command, self.path)
        # Refer: https://developer.mozilla.org/en-US/docs/Web/HTTP/Access_control_CORS#Preflighted_requests

        response = \
                "HTTP/1.1 200 OK\r\n"\
                "Access-Control-Allow-Credentials: true\r\n"\
                "Access-Control-Allow-Methods: GET, POST, HEAD, PUT, DELETE, PATCH\r\n"\
                "Access-Control-Max-Age: 1728000\r\n"\
                "Content-Length: 0\r\n"

        req_header = headers.get("Access-Control-Request-Headers", "")
        if req_header:
            response += "Access-Control-Allow-Headers: %s\r\n" % req_header

        origin = headers.get("Origin", "")
        if origin:
            response += "Access-Control-Allow-Origin: %s\r\n" % origin
        else:
            response += "Access-Control-Allow-Origin: *\r\n"

        response += "\r\n"

        self.wfile.write(response)

    def is_local(self, hosts):
        if 0 == len(self.local_names):
            self.local_names.append('localhost')
            self.local_names.append(socket.gethostname().lower())
            try:
                self.local_names.append(socket.gethostbyname_ex(socket.gethostname())[-1])
            except socket.gaierror:
                # TODO Append local IP address to local_names
                pass

        for s in hosts:
            s = s.lower()
            if s.startswith('127.') \
                    or s.startswith('192.168.') \
                    or s.startswith('10.') \
                    or s.startswith('169.254.') \
                    or s in self.local_names:
                print s
                return True

        return False

    def do_CONNECT(self):
        """deploy fake cert to client"""
        host, _, port = self.path.rpartition(':')
        port = int(port)
        if port not in (80, 443):
            xlog.warn("CONNECT %s port:%d not support", host, port)
            return

        certfile = CertUtil.get_cert(host)
        self.wfile.write(b'HTTP/1.1 200 Connection Established\r\n\r\n')
        #self.conntunnel = True
 
        leadbyte = self.connection.recv(1, socket.MSG_PEEK)
        if leadbyte in ('\x80', '\x16'):
            try:
                ssl_sock = ssl.wrap_socket(self.connection, keyfile=CertUtil.cert_keyfile, certfile=certfile, server_side=True)
            except ssl.SSLError as e:
                xlog.info('ssl error: %s, create full domain cert for host:%s', e, host)
                certfile = CertUtil.get_cert(host, full_name=True)
                return
            except Exception as e:
                if e.args[0] not in (errno.ECONNABORTED, errno.ECONNRESET):
                    xlog.exception('ssl.wrap_socket(self.connection=%r) failed: %s path:%s, errno:%s', self.connection, e, self.path, e.args[0])
                return

            self.__realwfile = self.wfile
            self.__realrfile = self.rfile
            self.connection = ssl_sock
            self.rfile = self.connection.makefile('rb', self.bufsize)
            self.wfile = self.connection.makefile('wb', 0)

        self.close_connection = 0

    def do_METHOD(self):
        self.req_payload = None
        host = self.headers.get('Host', '')
        host_ip, _, port = host.rpartition(':')

        if self.is_local([host, host_ip]):
            xlog.debug("Browse localhost by proxy")
            return self.forward_local()
        elif host == self.fake_host:
            # xlog.debug("%s %s", self.command, self.path)
            # for web_ui status page
            # auto detect browser proxy setting is work
            return self.wfile.write(self.self_check_response_data)

        if not (front.config.use_ipv6 == "force_ipv6" and \
                check_local_network.IPv6.is_ok() or \
                front.config.use_ipv6 != "force_ipv6" and \
                check_local_network.is_ok()):
            self.close_connection = 1
            return

        if isinstance(self.connection, ssl.SSLSocket):
            schema = "https"
        else:
            schema = "http"

        if self.path[0] == '/':
            self.host = self.headers['Host']
            self.url = '%s://%s%s' % (schema, host, self.path)
        else:
            self.url = self.path
            self.parsed_url = urlparse.urlparse(self.path)
            self.host = self.parsed_url[1]
            if len(self.parsed_url[4]):
                self.path = '?'.join([self.parsed_url[2], self.parsed_url[4]])
            else:
                self.path = self.parsed_url[2]

        if len(self.url) > 2083 and self.host.endswith(front.config.GOOGLE_ENDSWITH):
            return self.go_DIRECT()

        if self.host in front.config.HOSTS_GAE:
            return self.go_AGENT()

        # redirect http request to https request
        # avoid key word filter when pass through GFW
        if host in front.config.HOSTS_DIRECT:
            return self.go_DIRECT()

        if host.endswith(front.config.HOSTS_GAE_ENDSWITH):
            return self.go_AGENT()

        if host.endswith(front.config.HOSTS_DIRECT_ENDSWITH):
            return self.go_DIRECT()

        return self.go_AGENT()

    # Called by do_METHOD and do_CONNECT_AGENT
    def go_AGENT(self):
        request_headers = dict((k.title(), v) for k, v in self.headers.items())
        payload = self.read_payload()

        if self.command == "OPTIONS":
            return self.send_method_allows(request_headers, payload)

        if self.command not in self.gae_support_methods:
            xlog.warn("Method %s not support in GAEProxy for %s", self.command, self.path)
            return self.wfile.write(('HTTP/1.1 404 Not Found\r\n\r\n').encode())

        xlog.debug("GAE %s %s from:%s", self.command, self.url, self.address_string())
        if gae_handler.handler(self.command, self.host, self.url, request_headers, payload, self.wfile, self.go_DIRECT) != "ok":
            self.close_connection = 1

    def go_DIRECT(self):
        if not self.url.startswith("https"):
            xlog.debug("Host:%s Direct redirect to https", self.host)
            return self.wfile.write(('HTTP/1.1 301\r\nLocation: %s\r\nContent-Length: 0\r\n\r\n' % self.url.replace('http://', 'https://', 1)).encode())

        request_headers = dict((k.title(), v) for k, v in self.headers.items())
        payload = self.read_payload()

        xlog.debug("DIRECT %s %s from:%s", self.command, self.url, self.address_string())
        if direct_handler.handler(self.command, self.host, self.path, request_headers, payload, self.wfile) != "ok":
            self.close_connection = 1

    def read_payload(self):
        def get_crlf(rfile):
            crlf = rfile.readline(2)
            if crlf != "\r\n":
                xlog.warn("chunk header read fail crlf")

        if self.req_payload is not None:
            return self.req_payload

        payload = b''
        if 'Content-Length' in self.headers:
            try:
                payload_len = int(self.headers.get('Content-Length', 0))
                #xlog.debug("payload_len:%d %s %s", payload_len, self.command, self.path)
                payload = self.rfile.read(payload_len)
            except NetWorkIOError as e:
                xlog.error('handle_method_urlfetch read payload failed:%s', e)
                return
        elif 'Transfer-Encoding' in self.headers:
            # chunked, used by facebook android client
            payload = ""
            while True:
                chunk_size_str = self.rfile.readline(65537)
                chunk_size_list = chunk_size_str.split(";")
                chunk_size = int("0x"+chunk_size_list[0], 0)
                if len(chunk_size_list) > 1 and chunk_size_list[1] != "\r\n":
                    xlog.warn("chunk ext: %s", chunk_size_str)
                if chunk_size == 0:
                    while True:
                        line = self.rfile.readline(65537)
                        if line == "\r\n":
                            break
                        else:
                            xlog.warn("entity header:%s", line)
                    break
                payload += self.rfile.read(chunk_size)
                get_crlf(self.rfile)

        self.req_payload = payload
        return payload

# called by smart_router
def wrap_ssl(sock, host, port, client_address):
    certfile = CertUtil.get_cert(host or 'www.google.com')
    ssl_sock = ssl.wrap_socket(sock, keyfile=CertUtil.cert_keyfile,
                               certfile=certfile, server_side=True)
    return ssl_sock


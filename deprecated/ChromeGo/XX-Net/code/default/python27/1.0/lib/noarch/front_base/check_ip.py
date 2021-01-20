import socket
import time

import hyper
import simple_http_client


class CheckIp(object):
    def __init__(self, logger, config, connect_creator):
        self.logger = logger
        self.config = config
        self.connect_creator = connect_creator

    def check_http1(self, ssl_sock, host):
        self.logger.info("ip:%s use http/1.1", ssl_sock.ip)

        try:
            request_data = 'GET %s HTTP/1.1\r\nHost: %s\r\n\r\n' % (self.config.check_ip_path, host)
            ssl_sock.send(request_data.encode())

            response = simple_http_client.Response(ssl_sock)
            response.begin(timeout=5)
            return response
        except Exception as e:
            self.logger.debug("check ip %s http1 e:%r", ssl_sock.ip, e)
            return False

    def check_http2(self, ssl_sock, host):
        self.logger.debug("ip:%s use http/2", ssl_sock.ip)
        try:
            conn = hyper.HTTP20Connection(ssl_sock, host=host, ip=ssl_sock.ip, port=443)
            conn.request('GET', self.config.check_ip_path)
            response = conn.get_response()
            return response
        except Exception as e:
            self.logger.exception("http2 get response fail:%r", e)
            return False

    def check_ip(self, ip, sni=None, host=None, wait_time=0):
        try:
            ssl_sock = self.connect_creator.connect_ssl(ip, sni=sni)
            self.connect_creator.get_ssl_cert_domain(ssl_sock)
        except socket.timeout:
            self.logger.warn("connect timeout")
            return False
        except Exception as e:
            self.logger.exception("check_ip:%s create_ssl except:%r", ip, e)
            return False

        ssl_sock.ok = False

        if host:
            pass
        elif self.config.check_ip_host:
            host = self.config.check_ip_host
        else:
            host = ssl_sock.host
        self.logger.info("host:%s", host)

        time.sleep(wait_time)
        start_time = time.time()

        if not ssl_sock.h2:
            response = self.check_http1(ssl_sock, host)
        else:
            response = self.check_http2(ssl_sock, host)

        if not response:
            return False

        if not self.check_response(response):
            return False

        time_cost = (time.time() - start_time) * 1000
        ssl_sock.request_time = time_cost
        self.logger.info("check ok, time:%d", time_cost)
        ssl_sock.ok = True
        return ssl_sock

    def check_response(self, response):
        server_type = response.headers.get('Server', "")
        self.logger.debug("status:%d", response.status)
        self.logger.debug("Server type:%s", server_type)

        if response.status not in self.config.check_ip_accept_status:
            return False

        content = response.read()

        if self.config.check_ip_content not in content:
            self.logger.warn("app check content:%s", content)
            return False

        return True

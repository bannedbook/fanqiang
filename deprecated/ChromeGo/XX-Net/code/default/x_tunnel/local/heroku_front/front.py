import os
import struct

import xlog
logger = xlog.getLogger("heroku_front")
logger.set_buffer(500)

import simple_http_client
from config import Config
import host_manager
from front_base.openssl_wrap import SSLContext
from front_base.connect_creator import ConnectCreator
from front_base.ip_manager import IpManager
from front_base.http_dispatcher import HttpsDispatcher
from front_base.connect_manager import ConnectManager
from front_base.check_ip import CheckIp
from gae_proxy.local import check_local_network


current_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.abspath(os.path.join(current_path, os.pardir, os.pardir, os.pardir))
data_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir, 'data'))
module_data_path = os.path.join(data_path, 'x_tunnel')


class Front(object):
    name = "heroku_front"

    def __init__(self):
        self.logger = logger
        config_path = os.path.join(module_data_path, "heroku_front.json")
        self.config = Config(config_path)

        ca_certs = os.path.join(current_path, "cacert.pem")
        self.host_manager = host_manager.HostManager(self.logger, self.config.appids)

        openssl_context = SSLContext(logger, ca_certs=ca_certs)
        self.connect_creator = ConnectCreator(logger, self.config, openssl_context, self.host_manager)
        self.check_ip = CheckIp(xlog.null, self.config, self.connect_creator)

        ip_source = None
        default_ip_list_fn = os.path.join(current_path, "good_ip.txt")
        ip_list_fn = os.path.join(module_data_path, "heroku_ip_list.txt")
        self.ip_manager = IpManager(logger, self.config, ip_source, check_local_network,
                    self.check_ip.check_ip,
                 default_ip_list_fn, ip_list_fn, scan_ip_log=None)

        self.connect_manager = ConnectManager(logger, self.config, self.connect_creator, self.ip_manager, check_local_network)
        self.http_dispatcher = HttpsDispatcher(logger, self.config, self.ip_manager, self.connect_manager)

    def get_dispatcher(self, host=None):
        if len(self.host_manager.appids) == 0:
            return None

        return self.http_dispatcher

    def set_ips(self, ips):
        self.ip_manager.set_ips(ips)

    def _request(self, method, host, path="/", headers={}, data="", timeout=40):
        try:
            response = self.http_dispatcher.request(method, host, path, dict(headers), data, timeout=timeout)
            if not response:
                return "", 500, {}

            status = response.status
            if status != 200:
                self.logger.warn("front request %s %s%s fail, status:%d", method, host, path, status)

            content = response.task.read_all()
            # self.logger.debug("%s %s%s trace:%s", method, response.ssl_sock.host, path, response.task.get_trace())
            return content, status, response
        except Exception as e:
            self.logger.exception("front request %s %s%s fail:%s", method, host, path, e)
            return "", 500, {}

    def request(self, method, host, schema="http", path="/", headers={}, data="", timeout=40):
        schema = "http"
        # force schema to http, avoid cert fail on heroku curl.
        # and all x-server provide ipv4 access

        url = schema + "://" + host + path
        payloads = ['%s %s HTTP/1.1\r\n' % (method, url)]
        for k in headers:
            v = headers[k]
            payloads.append('%s: %s\r\n' % (k, v))
        head_payload = "".join(payloads)

        request_body = '%s%s%s%s' % \
                       ((struct.pack('!H', len(head_payload)),  head_payload,
                         struct.pack('!I', len(data)), data))
        request_headers = {'Content-Length': len(data), 'Content-Type': 'application/octet-stream'}

        heroku_host = ""
        content, status, response = self._request(
                                            "POST", heroku_host, "/2/",
                                            request_headers, request_body, timeout)

        # self.logger.info('%s "PHP %s %s %s" %s %s', handler.address_string(), handler.command, url, handler.protocol_version, response.status, response.getheader('Content-Length', '-'))
        # self.logger.debug("status:%d", status)

        if hasattr(response, "task"):
            self.logger.debug("%s %s%s status:%d trace:%s", method, host, path, status, response.task.get_trace())
        else:
            self.logger.debug("%s %s%s status:%d", method, host, path, status)

        if status == 404:
            heroku_host = response.ssl_sock.host
            self.logger.warn("heroku appid:%s fail", heroku_host)
            try:
                self.host_manager.remove(heroku_host)
            except:
                pass
            return "", 501, {}

        if not content:
            return "", 501, {}

        try:
            res = simple_http_client.TxtResponse(content)
        except Exception as e:
            self.logger.warn("decode %s response except:%r", content, e)
            return "", 501, {}

        res.worker = response.worker
        res.task = response.task
        return res.body, res.status, res

    def stop(self):
        logger.info("terminate")
        self.connect_manager.set_ssl_created_cb(None)
        self.http_dispatcher.stop()
        self.connect_manager.stop()
        self.ip_manager.stop()

        self.running = False

    def set_proxy(self, args):
        logger.info("set_proxy:%s", args)

        self.config.PROXY_ENABLE = args["enable"]
        self.config.PROXY_TYPE = args["type"]
        self.config.PROXY_HOST = args["host"]
        self.config.PROXY_PORT = args["port"]
        self.config.PROXY_USER = args["user"]
        self.config.PROXY_PASSWD = args["passwd"]

        self.config.save()

        self.connect_creator.update_config()


front = Front()
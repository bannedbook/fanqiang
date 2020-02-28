import os

import xlog
logger = xlog.getLogger("cloudfront_front")
logger.set_buffer(500)

from front_base.openssl_wrap import SSLContext
from front_base.ip_manager import IpManager
from front_base.ip_source import Ipv4RangeSource
from front_base.http_dispatcher import HttpsDispatcher
from front_base.connect_manager import ConnectManager
from front_base.check_ip import CheckIp

from connect_creator import ConnectCreator
from config import Config
import host_manager
from gae_proxy.local import check_local_network


current_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.abspath(os.path.join(current_path, os.pardir, os.pardir, os.pardir))
data_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir, 'data'))
module_data_path = os.path.join(data_path, 'x_tunnel')


class Front(object):
    name = "cloudfront_front"

    def __init__(self):
        self.running = True
        self.last_host = "center.xx-net.net"

        self.logger = logger
        config_path = os.path.join(module_data_path, "cloudfront_front.json")
        self.config = Config(config_path)

        sni_fn = os.path.join(current_path, "sni_list.txt")
        self.host_manager = host_manager.HostManager(fn=sni_fn, max_size=120)

        ca_certs = os.path.join(current_path, "cacert.pem")
        openssl_context = SSLContext(logger, ca_certs=ca_certs, support_http2=True)
        self.connect_creator = ConnectCreator(logger, self.config, openssl_context, self.host_manager)
        self.ip_checker = CheckIp(xlog.null, self.config, self.connect_creator)

        ip_source = Ipv4RangeSource(
            logger, self.config,
            os.path.join(current_path, "ip_range.txt"),
            os.path.join(module_data_path, "cloudfront_ip_range.txt")
        )
        self.ip_manager = IpManager(
            logger, self.config, ip_source, check_local_network,
            self.check_ip,
            os.path.join(current_path, "good_ip.txt"),
            os.path.join(module_data_path, "cloudfront_ip_list.txt"),
            scan_ip_log=None)

        self.connect_manager = ConnectManager(
            logger, self.config, self.connect_creator, self.ip_manager, check_local_network)

        self.dispatchs = {}

    def check_ip(self, ip):
        sni, host = self.host_manager.get_sni_host(ip)
        host = "scan1.xx-net.net"
        return self.ip_checker.check_ip(ip, sni=sni, host=host)

    def get_dispatcher(self, host=None):
        if host is None:
            host = self.last_host
        else:
            self.last_host = host

        if host not in self.dispatchs:
            http_dispatcher = HttpsDispatcher(
                logger, self.config, self.ip_manager, self.connect_manager
            )
            self.dispatchs[host] = http_dispatcher

        dispatcher = self.dispatchs[host]
        return dispatcher

    def request(self, method, host, path="/", headers={}, data="", timeout=120):
        dispatcher = self.get_dispatcher(host)
        headers = dict(headers)
        response = dispatcher.request(method, host, path, headers, data, timeout=timeout)
        if not response:
            self.logger.warn("req %s get response timeout", path)
            return "", 602, {}

        status = response.status
        content = response.task.read_all()
        if status == 200:
            self.logger.debug("%s %s%s status:%d trace:%s", method, response.worker.ssl_sock.host, path, status,
                       response.task.get_trace())
        else:
            self.logger.warn("%s %s%s status:%d trace:%s", method, response.worker.ssl_sock.host, path, status,
                       response.task.get_trace())
        return content, status, response

    def stop(self):
        logger.info("terminate")
        self.connect_manager.set_ssl_created_cb(None)
        for host in self.dispatchs:
            dispatcher = self.dispatchs[host]
            dispatcher.stop()
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
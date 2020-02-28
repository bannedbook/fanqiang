import os

import xlog
logger = xlog.getLogger("tls_relay")
logger.set_buffer(500)

from config import Config
import host_manager
from front_base.openssl_wrap import SSLContext
from front_base.connect_creator import ConnectCreator
from front_base.ip_manager import IpManager
from front_base.http_dispatcher import HttpsDispatcher
from front_base.connect_manager import ConnectManager
from front_base.ip_source import IpSimpleSource
from front_base.check_ip import CheckIp
from gae_proxy.local import check_local_network

current_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.abspath(os.path.join(current_path, os.pardir, os.pardir, os.pardir))
data_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir, 'data'))
module_data_path = os.path.join(data_path, 'x_tunnel')


class Front(object):
    name = "tls_relay_front"

    def __init__(self):
        self.running = True

        self.logger = logger
        config_path = os.path.join(module_data_path, "tls_relay.json")
        self.config = Config(config_path)

        self.ca_cert_fn = os.path.join(module_data_path, "tls_relay_CA.crt")
        self.openssl_context = SSLContext(logger)
        if os.path.isfile(self.ca_cert_fn):
            self.openssl_context.set_ca(self.ca_cert_fn)

        host_fn = os.path.join(module_data_path, "tls_host.json")
        self.host_manager = host_manager.HostManager(host_fn)

        self.connect_creator = ConnectCreator(logger, self.config, self.openssl_context, self.host_manager)
        self.check_ip = CheckIp(xlog.null, self.config, self.connect_creator)

        ip_source = IpSimpleSource(self.config.ip_source_ips)

        default_ip_list_fn = ""
        ip_list_fn = os.path.join(module_data_path, "tls_relay_ip_list.txt")
        self.ip_manager = IpManager(logger, self.config, ip_source, check_local_network, self.check_ip.check_ip,
                 default_ip_list_fn, ip_list_fn, scan_ip_log=None)
        for ip in self.config.ip_source_ips:
            self.ip_manager.add_ip(ip, 100)

        self.connect_manager = ConnectManager(logger, self.config, self.connect_creator, self.ip_manager, check_local_network)
        self.http_dispatcher = HttpsDispatcher(logger, self.config, self.ip_manager, self.connect_manager)

        self.account = ""
        self.password = ""

    def get_dispatcher(self, host=None):
        return self.http_dispatcher

    def set_x_tunnel_account(self, account, password):
        self.account = account
        self.password = password

    def set_ips(self, ips):
        if not ips:
            return

        host_info = {}
        ca_certs = []
        ipss = []
        for ip in ips:
            dat = ips[ip]
            ca_cert = dat["ca_crt"]
            sni = dat["sni"]

            host_info[ip] = {"sni":sni, "ca_crt":ca_cert}
            if ca_cert not in ca_certs:
                ca_certs.append(ca_cert)
            ipss.append(ip)

        self.ip_manager.update_ips(ipss)
        self.ip_manager.save(True)
        self.host_manager.set_host(host_info)

        ca_content = "\n\n".join(ca_certs)
        with open(self.ca_cert_fn, "w") as fd:
            fd.write(ca_content)
        self.openssl_context.set_ca(self.ca_cert_fn)
        self.logger.info("set_ips:%s", ",".join(ipss))

    def request(self, method, host, path="/", headers={}, data="", timeout=120):
        headers = dict(headers)
        headers["XX-Account"] = self.account

        response = self.http_dispatcher.request(method, host, path, dict(headers), data, timeout=timeout)
        if not response:
            logger.warn("req %s get response timeout", path)
            return "", 602, {}

        status = response.status

        content = response.task.read_all()
        if status == 200:
            logger.debug("%s %s%s status:%d trace:%s", method, host, path, status,
                       response.task.get_trace())
        else:
            logger.warn("%s %s%s status:%d trace:%s", method, host, path, status,
                       response.task.get_trace())
        return content, status, response

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
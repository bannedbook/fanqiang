
import global_var as g
import connect_manager
from xlog import getLogger
xlog = getLogger("smart_router")


def set_proxy(args):
    xlog.info("set_proxy:%s", args)

    g.config.PROXY_ENABLE = args["enable"]
    g.config.PROXY_TYPE = args["type"]
    g.config.PROXY_HOST = args["host"]
    try:
        g.config.PROXY_PORT = int(args["port"])
    except:
        g.config.PROXY_POT = 0

        g.config.PROXY_USER = args["user"]
    g.config.PROXY_PASSWD = args["passwd"]

    g.config.save()

    connect_manager.load_proxy_config()


def set_bind_ip(args):
    xlog.info("set_bind_ip:%s", args)

    g.config.proxy_bind_ip = args["ip"]
    g.config.dns_bind_ip = args["ip"]
    g.config.save()

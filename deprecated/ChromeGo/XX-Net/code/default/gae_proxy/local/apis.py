
from front import front, direct_front
from xlog import getLogger
xlog = getLogger("gae_proxy")


def set_proxy(args):
    front.set_proxy(args)
    direct_front.set_proxy(args)


def _count_conn_num():
    return len(front.connect_manager.new_conn_pool.pool) +\
           front.http_dispatcher.h1_num + \
           front.http_dispatcher.h2_num


def is_workable():
    #if front.http_dispatcher.is_idle():
    #    return True

    if _count_conn_num() > 0:
        return True
    else:
        front.http_dispatcher.get_worker()
        return _count_conn_num() > 0


def set_bind_ip(args):
    xlog.info("set_bind_ip:%s", args)

    front.config.listen_ip = args["ip"]
    front.config.save()

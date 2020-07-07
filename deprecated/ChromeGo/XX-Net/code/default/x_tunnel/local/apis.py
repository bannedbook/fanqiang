import time

import global_var as g
import front_dispatcher

from xlog import getLogger
xlog = getLogger("x_tunnel")


workable_call_times = 0


def set_proxy(args):
    xlog.info("set_proxy:%s", args)
    for front in front_dispatcher.all_fronts:
        try:
            front.set_proxy(args)
        except Exception as e:
            xlog.exception("set_proxy except:%r", e)


def is_workable():
    global workable_call_times
    if workable_call_times == 0:
        loop_num = 8
    else:
        loop_num = 1

    workable_call_times += 1
    for i in xrange(0, loop_num):
        for front in front_dispatcher.session_fronts:
            score = front.get_dispatcher().get_score()
            if score is None:
                continue
            else:
                return True

        if loop_num > 1:
            time.sleep(1)

    return False


def set_bind_ip(args):
    xlog.info("set_bind_ip:%s", args)

    g.config.socks_host = args["ip"]
    g.config.save()

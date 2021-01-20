import os
import shlex
import subprocess
from .pteredor import teredo_prober

from xlog import getLogger
xlog = getLogger("gae_proxy")

current_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.abspath(os.path.join(current_path, os.pardir, os.pardir, os.pardir))
data_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir, 'data', "gae_proxy"))
if not os.path.isdir(data_path):
    data_path = current_path

log_file = os.path.join(data_path, "ipv6_tunnel.log")

if os.path.isfile(log_file):
    os.remove(log_file)


class Log(object):
    def __init__(self):
        self.fd = open(log_file, "a")

    def write(self, content):
        self.fd.write(content + "\n")
        self.fd.flush()

    def close(self):
        self.fd.close()


pteredor_is_running = False


def new_pteredor(probe_nat=True):
    if os.path.isfile(log_file):
        try:
            os.remove(log_file)
        except Exception as e:
            xlog.warn("remove %s fail:%r", log_file, e)

    global pteredor_is_running, usable
    pteredor_is_running = probe_nat
    prober = teredo_prober(probe_nat=probe_nat)

    if prober.nat_type in ('cone', 'restricted'):
        usable = 'usable'
    elif prober.nat_type == 'offline':
        usable = 'unusable'
    else:
        usable = 'unknown'

    if probe_nat:
        pteredor_is_running = False
        log = Log()
        log.write('qualified: %s\nNAT type: %s' % (prober.qualified, prober.nat_type))
        log.close()
    return prober


def test_teredo(probe_nat=True, probe_server=True):
    if pteredor_is_running:
        return "Script is running, please retry later."

    server = ''
    if probe_server:
        server = ' and the best server is %s.' % best_server(probe_nat=probe_nat)
    else:
        new_pteredor()

    return 'teredor test result is %s.%s' % (usable, server)


def best_server(probe_nat=False):
    best_server = None
    prober = new_pteredor(probe_nat=probe_nat)
    prober.qualified = True
    if not probe_nat:
        prober.nat_type = 'unknown'
        prober.rs_cone_flag = 0

    global pteredor_is_running
    pteredor_is_running = True
    server_list = prober.eval_servers()
    pteredor_is_running = False

    for qualified, server, _, _ in server_list:
        if qualified:
            best_server = server[0]
            break
    log = Log()
    if best_server:
        log.write('best server is: %s.' % best_server)
    else:
        xlog.warning('no server detected, return default: teredo.remlab.net.')
        log.write('no server detected, return default: teredo.remlab.net.')
        best_server = "teredo.remlab.net"
    log.close()
    return best_server


def run(cmd):
    cmd = shlex.split(cmd)

    try:
        # hide console in MS windows
        startupinfo = subprocess.STARTUPINFO()
        startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startupinfo.wShowWindow = subprocess.SW_HIDE

        #out = subprocess.check_output(cmd, startupinfo=startupinfo)
        process = subprocess.Popen(cmd, stdout=subprocess.PIPE, startupinfo=startupinfo)
        out, unused_err = process.communicate()
        retcode = process.poll()
        if retcode:
            return out + "\n retcode:%s\n unused_err:%s\n" % (retcode, unused_err)
    except Exception as e:
        out = "Exception:%r" % e

    return out


def run_cmds(cmds):
    log = Log()
    cmd_pl = cmds.split("\n")
    outs = []
    for cmd in cmd_pl:
        if not cmd:
            continue

        if cmd.startswith("#"):
            log.write("%s" % cmd)
            continue

        log.write("\n>: %s\n------------------------------------" % cmd)
        out = run(cmd)
        log.write(out)
        outs.append(out)
    log.close()
    return "\r\n".join(outs)


def get_line_value(r, n):
    rls = r.split("\r\n")
    if len(rls) < n + 1:
        return None

    lp = rls[n].split(":")
    if len(lp) < 2:
        return None

    value = lp[1].strip()
    return value

import os
import threading
import time
import collections
import operator

import global_var as g

from xlog import getLogger
xlog = getLogger("smart_router")


def is_network_ok(ip):
    if not g.gae_proxy:
        return False

    return g.gae_proxy.check_local_network.is_ok(ip)


class DomainRecords(object):
    # File Format:
    # ===============
    # host $rule gae_acceptable [ip|CN,ip|CN ip_update_time]
    # rule: direct, gae, socks, black, other
    # gae_acceptable: 0 or 1, default 1
    # CN: country name

    # Cache struct
    # ==============
    # "r": rule
    # "g": gae_acceptable
    # "ip": { ip: cn, ..}
    # "update": $ip_update_time

    def __init__(self, file_path, capacity=1000, ttl=3600):
        self.file_path = file_path
        self.capacity = capacity
        self.ttl = ttl
        self.cache = collections.OrderedDict()
        self.last_save_time = time.time()
        self.lock = threading.Lock()
        self.last_update_time = 0
        self.need_save = False
        self.load()

        self.running = True

    def get(self, domain):
        with self.lock:
            try:
                record = self.cache.pop(domain)

                time_now = time.time()
                if time_now - record["update"] > self.ttl:
                   record = None
            except KeyError:
                record = None

            if not record:
                record = {"r": "unknown", "ip": {}, "g": 1, "query_count": 0}
            self.cache[domain] = record
            return record

    def set(self, domain, record):
        with self.lock:
            try:
                self.cache.pop(domain)
            except KeyError:
                if len(self.cache) >= self.capacity:
                    self.cache.popitem(last=False)

            self.cache[domain] = record
            self.need_save = True
            self.last_update_time = time.time()

    def clean(self):
        with self.lock:
            self.cache = collections.OrderedDict()
        self.save(True)

    def get_content(self):
        socks_lines = []
        gae_lines = []
        direct_lines = []
        with open(self.file_path, "r") as fd:
            for line in fd.readlines():
                if not line:
                    continue
                try:
                    lp = line.split()
                    if len(lp) < 2:
                        continue

                    rule = lp[1]
                    if rule == "socks":
                        socks_lines.append(line)
                    elif rule == "gae":
                        gae_lines.append(line)
                    else:
                        direct_lines.append(line)
                except Exception as e:
                    xlog.warn("rule line:%s fail:%r", line, e)
                    continue
        return "".join(socks_lines + gae_lines + direct_lines)

    def load(self):
        if not os.path.isfile(self.file_path):
            return

        with self.lock:
            with open(self.file_path, "r") as fd:
                for line in fd.readlines():
                    if not line:
                        continue

                    try:
                        lp = line.split()
                        if len(lp) == 3:
                            domain = lp[0]
                            rule = lp[1]
                            gae_acceptable = int(lp[2])
                            record = {"r": rule, "ip": {}, "g":gae_acceptable}
                        elif len(lp) == 5:
                            domain = lp[0]
                            rule = lp[1]
                            gae_acceptable = int(lp[2])
                            ips = lp[3].split(",")
                            update_time = int(lp[4])
                            record = {"r": rule, "ip":{}, "update": update_time, "g":gae_acceptable}
                            for ipd in ips:
                                ipl = ipd.split("|")
                                ip = ipl[0]
                                cn = ipl[1]

                                record["ip"][ip] = cn
                        else:
                            xlog.warn("rule line:%s fail", line)
                            continue
                    except Exception as e:
                        xlog.warn("rule line:%s fail:%r", line, e)
                        continue

                    self.cache[domain] = record

    def save(self, force=False):
        time_now = time.time()
        if not force:
            if not self.need_save:
                return

            if time_now - self.last_save_time < 10:
                return

        with self.lock:
            with open(self.file_path, "w") as fd:
                for host in self.cache:
                    record = self.cache[host]
                    line = host + " " + record["r"] + " " + str(record["g"]) + " "
                    if len(record["ip"]) and time_now - record["update"] < self.ttl:
                        for ip in record["ip"]:
                            cn = record["ip"][ip]
                            line += ip + "|" + cn + ","

                        line = line[:-1]
                        line += " " + str(int(record["update"]))

                    fd.write(line + "\n")

        self.last_save_time = time.time()
        self.need_save = False

    def get_ips(self, domain, type=None):
        record = self.get(domain)
        if len(record["ip"]) == 0:
            return []

        ips = []
        for ip in record["ip"]:
            if type and ("." in ip and type != 1) or (":" in ip and type != 28):
                continue

            cn = record["ip"][ip]
            s = "%s|%s" % (ip, cn)
            ips.append(s)

        return ips

    def get_ordered_ips(self, domain, type=None):
        record = self.get(domain)
        if len(record["ip"]) == 0:
            return []

        ip_rate = {}
        for ip in record["ip"]:
            connect_time = g.ip_cache.get_connect_time(ip)
            if not connect_time:
                ip_rate[ip] = 9999
            else:
                ip_rate[ip] = connect_time

        if not ip_rate:
            return []

        ip_time = sorted(ip_rate.items(), key=operator.itemgetter(1))

        ordered_ips = []
        for ip, rate in ip_time:
            cn = record["ip"][ip]
            s = "%s|%s" % (ip, cn)
            ordered_ips.append(s)

        return ordered_ips

    def set_ips(self, domain, ips, type=None, rule="direct"):
        record = self.get(domain)
        if rule != "direct":
            record["r"] = rule

        for ipd in ips:
            ipl = ipd.split("|")
            ip = ipl[0]
            cn = ipl[1]
            record["ip"][ip] = cn

        record["update"] = time.time()

        self.set(domain, record)

    def update_rule(self, domain, port, rule):
        record = self.get(domain)
        record["r"] = rule
        return self.set(domain, record)

    def report_gae_deny(self, domain, port=None):
        record = self.get(domain)
        record["g"] = 0
        return self.set(domain, record)

    def accept_gae(self, domain, port=None):
        record = self.get(domain)
        return record["g"]

    def get_query_count(self, domain):
        record = self.get(domain)
        return record["query_count"]

    def add_query_count(self, domain):
        record = self.get(domain)
        record["query_count"] += 1
        self.set(domain, record)


class IpRecord(object):
    # File Format:
    # =============
    # ip $rule $connect_time $update_time
    # rule: direct, gae, socks, black, other
    # connect_time: -1(fail times), n>=0 connect time cost in ms.
    # default: IPv4 6000, IPv6 4000
    # fail: 7000 + (fail time * 1000)

    # IPv6 will try first if IPv6 exist.

    # Cache struct
    # ==============
    # "r": rule
    # "c": $connect_time
    # "update": $update_time

    def __init__(self, file_path, capacity=1000, ttl=3600):
        self.file_path = file_path
        self.capacity = capacity
        self.ttl = ttl
        self.cache = collections.OrderedDict()
        self.last_save_time = time.time()
        self.lock = threading.Lock()
        self.last_update_time = 0
        self.need_save = False
        self.load()

        self.running = True

    def get(self, ip):
        with self.lock:
            record = None
            try:
                record = self.cache.pop(ip)
                self.cache[ip] = record
            except KeyError:
                pass
            return record

    def set(self, ip, record):
        with self.lock:
            try:
                self.cache.pop(ip)
            except KeyError:
                if len(self.cache) >= self.capacity:
                    self.cache.popitem(last=False)

            self.cache[ip] = record
            self.need_save = True
            self.last_update_time = time.time()

    def clean(self):
        with self.lock:
            self.cache = collections.OrderedDict()
        self.save(True)

    def get_content(self):
        with open(self.file_path, "r") as fd:
            content = fd.read()
            return content

    def load(self):
        if not os.path.isfile(self.file_path):
            return

        with self.lock:
            with open(self.file_path, "r") as fd:
                for line in fd.readlines():
                    if not line:
                        continue

                    lp = line.split()
                    if len(lp) != 4:
                        xlog.warn("rule line:%s fail", line)
                        continue

                    ip = lp[0]
                    rule = lp[1]
                    connect_time = int(lp[2])
                    update_time = int(lp[3])
                    self.cache[ip] = {"r": rule, "c": connect_time, "update": update_time}

    def save(self, force=False):
        if not force:
            if not self.need_save:
                return

            if time.time() - self.last_save_time < 10:
                return

        with self.lock:
            with open(self.file_path, "w") as fd:
                for ip in self.cache:
                    record = self.cache[ip]
                    rule = record["r"]
                    connect_time = record["c"]
                    update_time = record["update"]

                    fd.write("%s %s %d %d\n" % (ip, rule, connect_time, update_time))

        self.last_save_time = time.time()
        self.need_save = False

    def get_connect_time(self, ip, port=None):
        record = self.get(ip)
        if not record or time.time() - record["update"] > self.ttl:
            if "." in ip:
                return 6000
            else:
                return 4000
        else:
            return record["c"]

    def update_rule(self, ip, port, rule):
        record = self.get(ip)
        if "." in ip:
            connect_time = 6000
        else:
            connect_time = 4000

        if not record:
            record = {"r": rule, "c": connect_time}
        else:
            record["r"] = rule

        record["update"] = time.time()
        return self.set(ip, record)

    def update_connect_time(self, ip, port, connect_time):
        record = self.get(ip)
        if not record:
            record = {"r": "direct", "c": connect_time}
        else:
            record["c"] = connect_time
        record["update"] = time.time()
        return self.set(ip, record)

    def report_connect_fail(self, ip, port):
        if not is_network_ok(ip):
            return

        record = self.get(ip)
        if not record:
            record = {"r": "direct", "c": 7000}
        else:
            if record["c"] <= 7000:
                record["c"] = 7000
            else:
                record["c"] += 1000
        record["update"] = time.time()
        return self.set(ip, record)
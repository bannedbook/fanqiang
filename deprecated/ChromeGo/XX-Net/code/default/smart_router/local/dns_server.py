#!/usr/bin/env python
# coding:utf-8

import json
import os
import sys
import threading
import socket
import time
import re
import select
import collections
import random

current_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.abspath(os.path.join(current_path, os.pardir, os.pardir))
top_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir))
data_path = os.path.join(top_path, "data", 'smart_router')

python_path = os.path.join(root_path, 'python27', '1.0')
noarch_lib = os.path.join(python_path, 'lib', 'noarch')
sys.path.append(noarch_lib)

import simple_queue
import utils
from dnslib import DNSRecord, DNSHeader, A, AAAA, RR, DNSQuestion, QTYPE

import global_var as g
from xlog import getLogger
xlog = getLogger("smart_router")


def remote_query_dns(domain, type=None):
    if not g.x_tunnel:
        return []

    content, status, response = g.x_tunnel.front_dispatcher.request(
        "GET", "dns.xx-net.net", path="/query?domain=%s" % (domain), timeout=5)

    if status != 200:
        xlog.warn("remote_query_dns fail status:%d", status)
        return []

    if isinstance(content, memoryview):
        content = content.tobytes()

    try:
        rs = json.loads(content)
        ips = rs["ip"]
        g.domain_cache.set_ips(domain, ips)
        return ips
    except Exception as e:
        xlog.warn("remote_query_dns json:%s parse fail:%s", content, e)
        return []


allowed = re.compile("(?!-)[A-Z\d-]{1,63}(?<!-)$")


def is_valid_hostname(hostname):
    hostname = hostname.upper()
    if len(hostname) > 255:
        return False
    if hostname.endswith("."):
        hostname = hostname[:-1]

    return all(allowed.match(x) for x in hostname.split("."))


class DnsServerList(object):
    def __init__(self):
        self.local_list = self.get_dns_server()

        if g.config.country_code == "CN":
            self.public_list = ['114.114.114.114', "180.76.76.76", "198.15.67.245", "202.46.32.19", "64.214.116.84"]
        else:
            self.public_list = ['8.8.8.8', "208.67.222.222", "209.244.0.3", "8.26.56.26", "37.235.1.174", "91.239.100.100"]

        self.public_servers = {}
        for ip in self.public_list:
            self.public_servers[ip] = {"query_time":0.01, "last_query": time.time()}

        self.i = 0

    def get_dns_server(self):
        iplist = []
        if os.name == 'nt':
            import ctypes, ctypes.wintypes, struct, socket
            DNS_CONFIG_DNS_SERVER_LIST = 6
            buf = ctypes.create_string_buffer(2048)
            ctypes.windll.dnsapi.DnsQueryConfig(DNS_CONFIG_DNS_SERVER_LIST, 0, None, None, ctypes.byref(buf),
                                                ctypes.byref(ctypes.wintypes.DWORD(len(buf))))
            ipcount = struct.unpack('I', buf[0:4])[0]
            iplist = [socket.inet_ntoa(buf[i:i + 4]) for i in xrange(4, ipcount * 4 + 4, 4)]
        elif os.path.isfile('/etc/resolv.conf'):
            with open('/etc/resolv.conf', 'rb') as fp:
                iplist = re.findall(r'(?m)^nameserver\s+(\S+)', fp.read())

        out_list = []
        for ip in iplist:
            if ip == "127.0.0.1":
                continue
            out_list.append(ip)
            xlog.debug("use local DNS server:%s", ip)

        return out_list

    def get_local(self):
        return self.local_list[self.i]

    def reset_server(self):
        self.i = 0

    def next_server(self):
        self.i += 1
        if self.i >= len(self.local_list):
            self.i = self.i % len(self.local_list)

        xlog.debug("next dns server:%s", self.get_local())

    def get_public(self):
        return random.choice(self.public_list)

    def get_fastest_public(self):
        fastest_ip = ""
        fastest_time = 10000
        for ip in self.public_servers:
            info = self.public_servers[ip]
            if info["query_time"] < fastest_time:
                if info["query_time"] > 2 and time.time() - info["last_query"] < 60:
                    continue
                fastest_ip = ip
                fastest_time = info["query_time"]

        return fastest_ip

    def update_public_server(self, ip, cost_time):
        self.public_servers[ip]["query_time"] = cost_time
        self.public_servers[ip]["last_query"] = time.time()


class DnsClient(object):
    def __init__(self):
        self.start()

    def start(self):
        self.waiters = {}

        self.dns_server = DnsServerList()

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.settimeout(1)

        self.running = True
        self.th = threading.Thread(target=self.recv_worker)
        self.th.start()

    def stop(self):
        self.running = False

    def query_over_tcp(self, domain, type=None, loop_count=0):
        if loop_count > 10:
            return []

        if type is None:
            types = [1, 28]
        else:
            types = [type]

        ips = []
        for t in types:
            query_time = 0
            for i in range(0, 3):
                server_ip = self.dns_server.get_fastest_public()
                if not server_ip:
                    return []

                query_time += 1
                if query_time > 3:
                    break

                t0 = time.time()
                try:
                    d = DNSRecord(DNSHeader())
                    d.add_question(DNSQuestion(domain, t))

                    a_pkt = d.send(server_ip, 53, tcp=True, timeout=2)
                    t1 = time.time()
                    self.dns_server.update_public_server(server_ip, t1-t0)
                    p = DNSRecord.parse(a_pkt)
                    if len(p.rr) == 0:
                        xlog.warn("query_over_tcp for %s type:%d server:%s return none",
                                  domain, t, server_ip)
                        continue

                    for r in p.rr:
                        ip = str(r.rdata)
                        if utils.check_ip_valid(ip):
                            if "." in ip and g.ip_region.check_ip(ip):
                                cn = g.ip_region.cn
                            else:
                                cn = "XX"
                            ips.append(ip + "|" + cn)
                        else:
                            # It is domain, loop search it.
                            ipss = self.query_over_tcp(ip, type, loop_count+1)
                            if not ipss:
                                continue
                            ips += ipss

                    break
                except Exception as e:
                    t1 = time.time()
                    self.dns_server.update_public_server(server_ip, t1 - t0)

                    xlog.warn("query_over_tcp %s type:%s server:%s except:%r", domain, type, server_ip,e)

        if ips:
            g.domain_cache.set_ips(domain, ips, type)

        return ips

    def recv_worker(self):
        while self.running:
            try:
                try:
                    response, server = self.sock.recvfrom(8192)
                    server, port = server
                except Exception as e:
                    # xlog.exception("sock.recvfrom except:%r", e)
                    continue

                if not response:
                    continue

                try:
                    p = DNSRecord.parse(response)
                except Exception as e:
                    xlog.exception("dns client parse response fail:%r", e)
                    continue

                if len(p.questions) == 0:
                    xlog.warn("received response without question")
                    continue

                id = p.header.id

                if id not in self.waiters:
                    continue

                que = self.waiters[id]
                org_domain = que.domain
                domain = str(p.questions[0].qname)
                xlog.debug("recev %s from:%s domain:%s org:%s", len(p.rr), server, domain, org_domain)
                ips = []
                for r in p.rr:
                    ip = str(r.rdata)
                    if r.rtype == 5:
                        # CNAME
                        if ip.endswith("."):
                            ip = ip[:-1]

                        if ip == domain:
                            xlog.warn("recv domain[%s] == ip[%s]", domain, ip)
                            continue

                        query_count = g.domain_cache.get_query_count(domain)
                        if query_count >= 50:
                            xlog.warn("%s ip:%s query_count:%d", domain, ip, query_count)
                            continue

                        g.domain_cache.add_query_count(domain)

                        xlog.debug("local dns %s recv %s cname:%s from:%s", org_domain, domain, ip, server)
                        d = DNSRecord(DNSHeader(id))
                        d.add_question(DNSQuestion(ip, QTYPE.A))
                        req_pack = d.pack()

                        self.sock.sendto(req_pack, (server, 53))

                        d = DNSRecord(DNSHeader(id))
                        d.add_question(DNSQuestion(ip, QTYPE.AAAA))
                        req_pack = d.pack()

                        self.sock.sendto(req_pack, (server, 53))
                        continue

                    if "." in ip and g.ip_region.check_ip(ip):
                        cn = g.ip_region.cn
                    else:
                        cn = "XX"
                    ips.append(ip+"|"+cn)

                if ips:
                    que.put(ips)
            except Exception as e:
                xlog.exception("dns recv_worker except:%r", e)

        xlog.info("DNS Client recv worker exit.")
        self.sock.close()

    def send_request(self, id, domain, server):
        try:
            d = DNSRecord(DNSHeader(id))
            d.add_question(DNSQuestion(domain, QTYPE.A))
            req4_pack = d.pack()

            d = DNSRecord(DNSHeader(id))
            d.add_question(DNSQuestion(domain, QTYPE.AAAA))
            req6_pack = d.pack()

            self.sock.sendto(req4_pack, (server, 53))
            # xlog.debug("send req:%s to:%s", domain, server)

            self.sock.sendto(req6_pack, (server, 53))
            # xlog.debug("send req:%s to:%s", domain, server)
        except Exception as e:
            xlog.warn("send_request except:%r", e)

    def query(self, domain, timeout=3, use_local=False):
        end_time = time.time() + timeout
        while True:
            id = random.randint(0, 65535)
            if id not in self.waiters:
                break

        que = simple_queue.Queue()
        que.domain = domain

        ips = []
        if use_local:
            server_list = self.dns_server.local_list
        else:
            server_list = self.dns_server.public_list

        for ip in server_list:
            new_time = time.time()
            if new_time > end_time:
                break

            self.send_request(id, domain, ip)

            self.waiters[id] = que
            ips += que.get(1) or []
            ips += que.get(new_time + 1 - time.time()) or []
            if ips:
                ips = list(set(ips))
                g.domain_cache.set_ips(domain, ips)
                break

        if id in self.waiters:
            del self.waiters[id]

        return ips


class DnsServer(object):
    def __init__(self, bind_ip="127.0.0.1", port=53, backup_port=5353, ttl=24*3600):
        self.sockets = []
        self.running = False
        if isinstance(bind_ip, str):
            self.bind_ip = [bind_ip]
        else:
            # server can listen multi-port
            self.bind_ip = bind_ip
        self.port = port
        self.backup_port = backup_port
        self.ttl = ttl
        self.th = None
        self.init_socket()

    def init_socket(self):
        ips = set(self.bind_ip)
        listen_all_v4 = "0.0.0.0" in ips
        listen_all_v6 = "::" in ips
        for ip in ips:
            if ip not in ("0.0.0.0", "::") and (
                    listen_all_v4 and '.' in ip or
                    listen_all_v6 and ':' in ip):
                continue
            self.bing_linsten(ip)

    def bing_linsten(self, bind_ip):
        if ":" in bind_ip:
            sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
        else:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            sock.bind((bind_ip, self.port))
            xlog.info("start DNS server at %s:%d", bind_ip, self.port)
            self.running = True
            self.sockets.append(sock)
            return
        except:
            xlog.warn("bind DNS %s:%d fail", bind_ip, self.port)
            pass

        try:
            sock.bind((bind_ip, self.backup_port))
            xlog.info("start DNS server at %s:%d", bind_ip, self.backup_port)
            self.running = True
            self.sockets.append(sock)
            return
        except Exception as e:
            xlog.warn("bind DNS %s:%d fail", bind_ip, self.backup_port)

            if sys.platform.startswith("linux"):
                xlog.warn("You can try: install libcap2-bin")
                xlog.warn("Then: sudo setcap 'cap_net_bind_service=+ep' /usr/bin/python2.7")
                xlog.warn("Or run XX-Net as root")

    def in_country(self, ips):
        for ip_cn in ips:
            ipcn_p = ip_cn.split("|")
            ip = ipcn_p[0]
            cn = ipcn_p[1]
            if cn == g.config.country_code:
                return True
        return False

    def query(self, domain, type=None):
        if utils.check_ip_valid(domain):
            return [domain]

        if not is_valid_hostname(domain):
            xlog.warn("DNS query:%s not valid, type:%d", domain, type)
            return []

        ips = g.domain_cache.get_ordered_ips(domain, type)
        if ips:
            return ips

        rule = g.user_rules.check_host(domain, 0)
        if rule == "black":
            ips = ["127.0.0.1|XX"]
            xlog.debug("DNS query:%s in black", domain)
            return ips

        if g.gfwlist.check(domain) or rule in ["gae", "socks"]:
            ips = remote_query_dns(domain, type)
            if not ips and g.config.auto_direct:
                ips = g.dns_client.query_over_tcp(domain, type)

            return ips

        if "." not in domain:
            ips = g.dns_client.query(domain, timeout=1, use_local=True)
            if not ips:
                ips = ["127.0.0.1|XX"]
                g.domain_cache.set_ips(domain, ips, type)
            return ips

        # case: normal domain, not in black list
        if g.config.auto_direct:
            ips = g.dns_client.query(domain, timeout=1)
            if not ips:
                ips = g.dns_client.query(domain, timeout=1, use_local=True)

        if not ips:
            ips = remote_query_dns(domain, type)

        return ips

    def direct_query(self, rsock, request, client_addr):
        start_time = time.time()
        for server_ip in g.dns_client.dns_server.public_list:
            try:
                a_pkt = request.send(server_ip, 53, tcp=True, timeout=1)
                # a = DNSRecord.parse(a_pkt)
                rsock.sendto(a_pkt, client_addr)
                return
            except:
                if time.time() - start_time > 5:
                    xlog.warn("direct_query %s timeout", request)
                    break

    def on_udp_query(self, rsock, req_data, addr):
        start_time = time.time()
        try:
            request = DNSRecord.parse(req_data)
            if len(request.questions) != 1:
                xlog.warn("query num:%d %s", len(request.questions), request)
                return

            domain = str(request.questions[0].qname)

            if domain.endswith("."):
                domain = domain[:-1]

            type = request.questions[0].qtype
            if type not in [1, 28]:
                xlog.info("direct_query:%s type:%d", domain, type)
                return self.direct_query(rsock, request, addr)

            xlog.debug("DNS query:%s type:%d from %s", domain, type, addr)

            ips = self.query(domain, type)
            if not ips:
                xlog.debug("query:%s type:%d from:%s, get fail, cost:%d", domain, type, addr,
                           (time.time() - start_time) * 1000)

            reply = DNSRecord(DNSHeader(id=request.header.id, qr=1, aa=1, ra=1, auth=1), q=request.q)
            for ip_cn in ips:
                ipcn_p = ip_cn.split("|")
                ip = ipcn_p[0]
                if "." in ip and type == 1:
                    reply.add_answer(RR(domain, ttl=60, rdata=A(ip)))
                elif ":" in ip and type == 28:
                    reply.add_answer(RR(domain, rtype=type, ttl=60, rdata=AAAA(ip)))
            res_data = reply.pack()

            rsock.sendto(res_data, addr)
            xlog.debug("query:%s type:%d from:%s, return ip num:%d cost:%d", domain, type, addr,
                       len(reply.rr), (time.time()-start_time)*1000)
        except Exception as e:
            xlog.exception("on_query except:%r", e)

    def server_forever(self):
        while self.running:
            r, w, e = select.select(self.sockets, [], [], 1)
            for rsock in r:
                data, addr = rsock.recvfrom(1024)
                threading.Thread(target=self.on_udp_query, args=(rsock, data, addr)).start()

        self.th = None

    def start(self):
        self.th = threading.Thread(target=self.server_forever)
        self.th.start()

    def stop(self):
        self.running = False
        while self.th:
            time.sleep(1)
        for sock in self.sockets:
            sock.close()
        self.sockets = []
        xlog.info("dns_server stop")


if __name__ == '__main__':
    r = remote_query_dns("apple.com")
    print(r)

#!/usr/bin/env python
# coding:utf-8

# A tool to help evaluate the teredo servers.
# Thanks XndroidDev
# Author: SeaHOH <seahoh@gmail.com>
# Compatible: Python 2.7 & 3.4 & 3.5 & 3.6
# References:
#   https://tools.ietf.org/html/rfc4380 5.1 5.2
#   https://tools.ietf.org/html/rfc4861 4.1 4.2
#   https://tools.ietf.org/html/rfc2460 8.1
#   https://github.com/XndroidDev/Xndroid/blob/master/fqrouter/manager/teredo.py

__version__ = '0.1.0'

import sys

if sys.platform == 'win32' and sys.version_info[0] < 3:
    import win_inet_pton

import os
import socket
import random
import struct
import collections
import time
import logging
import select
import errno

try:
    import queue
except:
    import Queue as queue

try:
    import thread
except:
    import _thread as thread

try:
    _real_raw_input = raw_input
    def raw_input(s='', file=sys.stdout):
        if isinstance(s, unicode):
            file.write(s.encode(sys.getfilesystemencoding(), 'replace'))
            return _real_raw_input()
        else:
            return _real_raw_input(s)
except:
    raw_input = input

logger = logging.getLogger('pteredor')


teredo_timeout = 4
teredo_port = 3544
link_local_addr = 'fe80::ffff:ffff:ffff'
all_router_multicast = 'ff02::2'
teredo_server_list = [
    # limited
    #'teredo.ginzado.ne.jp',
    # disuse
    #'debian-miredo.progsoc.org',
    #'teredo.autotrans.consulintel.com',
    #'teredo.ngix.ne.kr',
    #'teredo.managemydedi.com',
    #'teredo.ipv6.microsoft.com',
    #'win8.ipv6.microsoft.com',
    'teredo.remlab.net',
    'teredo2.remlab.net',
    'teredo-debian.remlab.net',
    'teredo.trex.fi',
    'teredo.iks-jena.de',
    'win10.ipv6.microsoft.com',
    'win1710.ipv6.microsoft.com',
    'win1711.ipv6.microsoft.com'
    ]

def creat_rs_nonce():
    return struct.pack('d', random.randint(0, 1<<62))

def creat_ipv6_rs_msg(checksum=None):
    return struct.pack('!2BH4x', 133, 0, checksum or 0)

def in_checksum(data):
    n = len(data)
    f = '%dH' % (n // 2)
    if n % 2:
        f += 'B'
    s = sum(struct.unpack(f, data))
    while (s >> 16):
        s = (s & 0xffff) + (s >> 16)
    s = ~s & 0xffff
    return socket.ntohs(s)

class teredo_rs_packet(object):
    def __init__(self, nonce=None):
        self.rs_src = socket.inet_pton(socket.AF_INET6, link_local_addr)
        self.rs_dst = socket.inet_pton(socket.AF_INET6, all_router_multicast)
        self._icmpv6_rs_msg = creat_ipv6_rs_msg()
        self.nonce = nonce or creat_rs_nonce()
        self.rs_src = bytearray(self.rs_src)
        self.teredo_header = self.creat_teredo_header()

    def creat_teredo_header(self):
        return struct.pack('!H2x8sx', 1, self.nonce)

    def creat_ipv6_pseudo_header(self):
        return struct.pack('!16s16sI3xB',
                           bytes(self.rs_src),
                           self.rs_dst,
                           58,
                           len(self._icmpv6_rs_msg)
                           )

    def creat_rs_packet(self, cone=None):
        self.rs_src[8] = 0x80 if cone else 0
        pseudo_header = self.creat_ipv6_pseudo_header()
        checksum = in_checksum(self._icmpv6_rs_msg + pseudo_header)
        rs_msg = creat_ipv6_rs_msg(checksum)
        return self.teredo_header + struct.pack('!B4x3B16s16s',
                                                0x60,
                                                len(rs_msg),
                                                58,
                                                255,
                                                bytes(self.rs_src),
                                                self.rs_dst
                                                ) + rs_msg

    @property
    def type_cone(self):
        if not hasattr(self, '_type_cone'):
            self._type_cone = self.creat_rs_packet(True)
        return self._type_cone

    @property
    def type_restricted(self):
        if not hasattr(self, '_type_restricted'):
            self._type_restricted = self.creat_rs_packet()
        return self._type_restricted

def get_sock(port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    while True:
        try:
            _port = port or random.randint(1025, 5000)
            print('try bind local port:', _port)
            sock.bind(('0.0.0.0', _port))
            return sock
        except socket.error as e:
            if port:
                print('bind local port %d fail: %r' % (_port, e))
                return
            if e.args[0] == errno.EADDRINUSE:
                pass

def is_ipv4(ip):
    try:
        socket.inet_aton(ip)
    except:
        return False
    else:
        return True

def resolve(host):
   try:
       return socket.gethostbyname_ex(host)[-1]
   except:
       return []

def ip2int(ip):
    return struct.unpack('>I', socket.inet_aton(ip))[0]

def int2ip(int):
    return socket.inet_ntoa(struct.pack('>I', int))

def get_second_server_ip(ip):
    return int2ip(ip2int(ip) + 1)

def remove_same_server(server_ip_list):
    logger.debug('input ip: %s' % server_ip_list)
    cleared_list = set()
    for ip1 in server_ip_list:
        _ip1 = ip2int(ip1)
        for ip2 in server_ip_list:
            b = _ip1 - ip2int(ip2)
            if b in (1, -1):
                cleared_list.add(ip1 if b < 0 else ip2)
                break
        if b not in (1, -1):
            cleared_list.add(ip1)
    logger.debug('cleared ip: %s' % cleared_list)
    return cleared_list

def str2hex(str):
    str = bytearray(str)
    h = ['']
    for c in str:
        if c > 0xf:
            h.append(hex(c)[2:])
        else:
            h.append('0' + hex(c)[2:])
    return '\\x'.join(h)

class deque(collections.deque):

    def put(self, v):
        self.append(v)

    def get(self):
        try:
            return self.popleft()
        except:
            return None

class default_prober_dict(dict):

    def __init__(self):
        self['nonce'] = None
        self['rs_packet'] = None
        self['ra_packets'] = deque()

class teredo_prober(object):

    _stoped = None
    nat_type = 'null'
    qualified = False
    rs_cone_flag = 1
    timeout = teredo_timeout
    teredo_port = teredo_port

    def __init__(self, server_list=teredo_server_list, local_port=None,
                 remote_port=None, probe_nat=True):
        self.teredo_sock = get_sock(local_port)
        if remote_port:
            self.teredo_port = remote_port
        self.prober_dict = collections.defaultdict(default_prober_dict)
        self.ip2server = collections.defaultdict(list)
        server_ip_list = []
        if isinstance(server_list, str):
            server_list = [server_list]
        for server in server_list:
            if is_ipv4(server):
                server_ip_list.append(server)
            else:
                ip_list = resolve(server)
                for ip in ip_list:
                    self.ip2server[ip].append(server)
                server_ip_list += ip_list
        self.server_ip_list = remove_same_server(server_ip_list)
        if len(self.server_ip_list) < 1:
            msg = 'Servers could not be resolved, %r.' % server_list
            print(msg)
            raise Exception(msg)
        elif len(self.server_ip_list) < 2:
            print('Need input more teredo servers, now is %d.'
                  % len(self.server_ip_list))
        thread.start_new_thread(self.receive_loop, ())
        if probe_nat:
            self.nat_type = self.nat_type_probe()

    def unpack_indication(self, data):
        return struct.unpack('!2s4s', data[2:8])

    def handle_ra_packet(self, ipv6_pkt):
        server_ip = socket.inet_ntoa(ipv6_pkt[76:80])
        cone_flag = bytearray(ipv6_pkt)[32] >> 7 & 1
        logger.debug('ipv6_pkt ; RA_cone = %s\nsrc:%s\ndst:%s' % (
                cone_flag,
                str2hex(ipv6_pkt[8:24]),
                str2hex(ipv6_pkt[24:40])))
        return server_ip, cone_flag

    def receive_ra_packet(self):
        data, addr = self.teredo_sock.recvfrom(10240)
        received_ip, port = addr
        if port != self.teredo_port or len(data) < 40:
            logger.debug('ipv6_pkt ;1 drop:\n%s' % str2hex(data))
            return
        auth_pkt = indicate_pkt = ipv6_pkt = None
        if data[0:2] == b'\x00\x01':
            auth_len = 13 + sum(struct.unpack('2B', data[2:4]))
            auth_pkt = data[0:auth_len]
            if data[auth_len:auth_len + 2] == b'\x00\x00':
                indicate_pkt = data[auth_len:auth_len + 8]
                ipv6_pkt = data[auth_len + 8:]
        if (auth_pkt is None or
            indicate_pkt is None or
            ipv6_pkt is None or
            bytearray(ipv6_pkt)[0] & 0xf0 != 0x60 or
            bytearray(ipv6_pkt)[40] != 134 or
            struct.unpack('!H', ipv6_pkt[4:6])[0] + 40 != len(ipv6_pkt)
            ):
            logger.debug('ipv6_pkt ;2 drop:\n%s' % str2hex(data))
            return
        server_ip, ra_cone_flag = self.handle_ra_packet(ipv6_pkt)
        logger.debug('server ip: %s ; received ip: %s' % (server_ip, received_ip))
        if (received_ip != server_ip and
            received_ip != get_second_server_ip(server_ip) or
            auth_pkt[4:12] != self.prober_dict[server_ip]['rs_packet'].nonce
            ):
            logger.debug('ipv6_pkt ;3 drop:\n%s' % str2hex(data))
            return
        qualified = ra_cone_flag, indicate_pkt
        self.prober_dict[received_ip]['ra_packets'].put(qualified)

    def receive_loop(self):
        while not self._stoped:
            try:
                rd, _, _ = select.select([self.teredo_sock], [], [], 0.5)
                if rd and not self._stoped:
                    self.receive_ra_packet()
            except Exception as e:
                logger.exception('receive procedure fail once: %r', e)
                pass

    def send_rs_packet(self, rs_packet, dst_ip):
        rs_packet = rs_packet.type_cone if self.rs_cone_flag else rs_packet.type_restricted
        logger.debug('send ; RS_cone = %s\n%s' % (self.rs_cone_flag, str2hex(rs_packet)))
        self.teredo_sock.sendto(rs_packet, (dst_ip, self.teredo_port))

    def qualify(self, server_ip, second_server_ip=None):
        rs_packet = self.prober_dict[server_ip]['rs_packet']
        if rs_packet is None:
            self.prober_dict[server_ip]['rs_packet'] = rs_packet = teredo_rs_packet()
        dst_ip = second_server_ip or server_ip
        self.send_rs_packet(rs_packet, dst_ip)

        begin_recv = time.time()
        while time.time() < self.timeout + begin_recv:
            qualified = self.prober_dict[dst_ip]['ra_packets'].get()
            if qualified:
                return qualified
            time.sleep(0.01)

    def qualify_loop(self, server_ip, second_server=None):
        if second_server:
            self.rs_cone_flag = 0
            second_server_ip = get_second_server_ip(server_ip) 
        else:
            second_server_ip = None
        for i in range(3):
            try:
                return self.qualify(server_ip, second_server_ip)
            except Exception as e:
                logger.exception('qualify procedure fail once: %r', e)

    def nat_type_probe(self):
        print('Starting probe NAT type...')
        self.nat_type = 'probing'
        server_ip_list = self.server_ip_list.copy()
        self.rs_cone_flag = 1
        for server_ip in server_ip_list:
            qualified = self.qualify_loop(server_ip)
            if qualified:
                break
        if qualified is None:
            self.rs_cone_flag = 0
            while server_ip_list:
                server_ip = server_ip_list.pop()
                qualified = self.qualify_loop(server_ip)
                if qualified:
                    break
        if qualified is None:
            self.qualified = False
            return 'offline'
        ra_cone_flag, first_indicate = qualified
        if ra_cone_flag:
            self.qualified = True
            return 'cone'
        qualified = None
        qualified = self.qualify_loop(server_ip, second_server=True)
        if qualified is None:
            self.last_server_ip = server_ip
            self.qualified = True
            return 'unknown'
        ra_cone_flag, second_indicate = qualified
        if first_indicate == second_indicate:
            self.qualified = True
            return 'restricted'
        else:
            self.qualified = False
            return 'symmetric'

    def _eval_servers(self, server_ip, queue_obj):
        start = time.time()
        qualified = self.qualify_loop(server_ip)
        cost = int((time.time() - start) * 1000)
        queue_obj.put((bool(qualified), self.ip2server[server_ip], server_ip, cost))

    def eval_servers(self):
        if self.nat_type is 'null':
            self.nat_type = self.nat_type_probe()
        elif self.nat_type is 'probing':
            print('Is probing NAT type now, pleace wait...')
        while self.nat_type is 'probing':
            time.sleep(0.1)
        if not self.qualified:
            print('This device can not use teredo tunnel, the NAT type is %s!'
                  % prober.nat_type)
            return []
        print('Starting evaluate servers...')
        self.clear()
        eval_list = []
        queue_obj = queue.Queue()
        for server_ip in self.server_ip_list:
            thread.start_new_thread(self._eval_servers, (server_ip, queue_obj))
        for _ in self.server_ip_list:
            eval_list.append(queue_obj.get())
        return eval_list

    def close(self):
        self._stoped = True
        self.clear()
        if self.teredo_sock:
            self.teredo_sock.close()
            self.teredo_sock = None

    def clear(self):
        for server_ip in self.server_ip_list:
            second_server_ip = get_second_server_ip(server_ip)
            self.prober_dict.pop(server_ip, None)
            self.prober_dict.pop(second_server_ip, None)

local_ip_startswith = tuple(
    ['192.168', '10.'] +
    ['100.%d.' % (64 + n) for n in range(1 << 6)] +
    ['172.%d.' % (16 + n) for n in range(1 << 4)]
    )

import locale

zh_locale = None
try:
    zh_locale = locale.getdefaultlocale()[0] == 'zh_CN'
except:
    if sys.platform == "darwin":
        try:
            oot = os.pipe()
            p = subprocess.Popen(['/usr/bin/defaults',
                                  'read',
                                  'NSGlobalDomain',
                                  'AppleLanguages'], stdout=oot[1])
            p.communicate()
            zh_locale = b'zh' in os.read(oot[0], 10000)
        except:
            pass

if zh_locale:
    help_info = u'''
pteredor [-p <port>] [-P <port>] [-h] [<server1> [<server2> [...]]]
      -p  \u8bbe\u7f6e\u672c\u5730\u5ba2\u6237\u7aef\u7aef\u53e3\u3002
      -P  \u8bbe\u7f6e\u8fdc\u7a0b\u670d\u52a1\u7aef\u7aef\u53e3\u3002
      -h  \u663e\u793a\u672c\u5e2e\u52a9\u4fe1\u606f\u3002

          Teredo server \u662f\u4e00\u4e2a\u4e3b\u673a\u540d\uff0c\u53ef\u4ee5\u4f7f\u7528\u57df\u540d\u6216 IP\u3002

'''
    result_info = u'\n\u7ecf\u68c0\u6d4b\uff0c\u63a8\u8350\u670d\u52a1\u5668\u662f %r.'
    wait_info = u'\u8bf7\u7b49\u5f85 10 \u79d2\u949f\u2026\u2026'
    resume_info = u'Teredo \u5ba2\u6237\u7aef\u5df2\u6062\u590d\u8fd0\u884c\u3002'
    warn_1 = u'\u53c2\u6570 "-p" \u9519\u8bef\uff1a\u7aef\u53e3\u5fc5\u987b\u662f\u4e00\u4e2a\u6570\u5b57\u3002'
    warn_2 = u'\u53c2\u6570 "-P" \u9519\u8bef\uff1a\u7aef\u53e3\u5fc5\u987b\u662f\u4e00\u4e2a\u6570\u5b57\u3002'
    warn_3 = u'\u5f53\u524d\u8bbe\u5907\u53ef\u80fd\u65e0\u6cd5\u6b63\u5e38\u4f7f\u7528 teredo \u96a7\u9053\uff0cNAT \u7c7b\u578b\u662f %s\uff01'
    warn_4 = u'\u65e0\u6cd5\u5224\u65ad NAT \u7c7b\u578b\u3002'
    confirm_stop = u'\u662f\u5426\u5148\u6682\u65f6\u5173\u95ed teredo \u5ba2\u6237\u7aef\uff08IPv6\uff09\u518d\u8fdb\u884c\u6d4b\u8bd5\uff1f\uff08Y/N\uff09'
    confirm_set = u'\u4f60\u60f3\u8981\u5c06 teredo \u670d\u52a1\u5668\u8bbe\u7f6e\u4e3a\u672c\u6d4b\u8bd5\u7684\u63a8\u8350\u503c\u5417\uff1f\uff08Y/N\uff09'
    confirm_reset = u'\u4f60\u60f3\u8981\u91cd\u7f6e teredo \u5ba2\u6237\u7aef\u7684\u5237\u65b0\u95f4\u9694\u5417\uff1f\uff08Y/N\uff09'
    confirm_over = u'\u6309\u56de\u8f66\u952e\u7ed3\u675f\u2026\u2026'
    confirm_force = u'\u4f60\u60f3\u8981\u7ee7\u7eed\u8fdb\u884c\u6d4b\u8bd5\u5417\uff1f\uff08Y/N\uff09'
    nat_type_result = u'NAT \u7c7b\u578b\u662f %s\u3002'
else:
    help_info = '''
pteredor [-p <port>] [-P <port>] [-h] [<server1> [<server2> [...]]]
      -p  Set the local port num. (client)
      -P  Set the remote port num. (server)
      -h  Show this help.

          The teredo server is a host name (domain or IP).

'''
    result_info = '\nThe recommend server is %r.'
    wait_info = 'Please wait 10 seconds...'
    resume_info = 'The teredo cilent has resumed.'
    warn_1 = 'The value of parameter "-p" error: local port must be a number.'
    warn_2 = 'The value of parameter "-P" error: remote port must be a number.'
    warn_3 = 'This device may not be able to use teredo tunnel, the NAT type is %s!'
    warn_4 = 'We can not judge the NAT type.'
    confirm_stop = 'Stop teredo cilent for run prober, Y/N? '
    confirm_set = 'Do you want to set recommend teredo server, Y/N? '
    confirm_reset = 'Do you want to reset refreshinterval to the default value, Y/N? '
    confirm_over = 'Press enter to over...'
    confirm_force = 'Do you want to force probe and set the teredo servers, Y/N? '
    nat_type_result = 'The NAT type is %s.'

if os.name == 'nt':
    import win32runas
    if win32runas.is_admin():
        runas = os.system
    else:
        def runas(cmd):
            cmd = tuple(cmd.split(None, 1))
            if len(cmd) == 1:
                cmd += None,
            win32runas.runas(cmd[1], cmd[0])

def main(local_port=None, remote_port=None, *args):
    server_list = [] + teredo_server_list
    for arg in args:
        if isinstance(arg, str):
            server_list.append(arg)
        elif isinstance(arg, list):
            server_list += arg
        elif isinstance(arg, tuple):
            server_list += list(arg)
    prober = teredo_prober(server_list, local_port=local_port, remote_port=remote_port)
    need_probe = recommend = None
    if not prober.qualified:
        print(warn_3 % prober.nat_type)
        if (prober.nat_type is 'symmetric' and 
                raw_input(confirm_force).lower() == 'y'):
            need_probe = True
            prober.qualified = True
    elif prober.nat_type is 'unknown':
        print(warn_4)
        recommend = prober.ip2server[prober.last_server_ip]
    else:
        print(nat_type_result % prober.nat_type)
        need_probe = True
    if need_probe:
        qualified_list = prober.eval_servers()
        for qualified, server, server_ip, cost in qualified_list:
            print('%s %s %s' % (server_ip, server, '%sms' % cost if qualified else 'timedout'))
        recommend = qualified_list[0][1]
    prober.close()
    return recommend, prober.nat_type

def test():
    logging.basicConfig(level=logging.DEBUG)
    blank_rs_packet = bytearray(
        b'\x00\x01\x00\x00\x8a\xde\xb0\xd0\x2e\xea\x0b\xfc\x00'
        b'\x60\x00\x00\x00\x00\x08\x3a\xff\xfe\x80\x00\x00\x00\x00\x00\x00'
        b'\x00\x00\xff\xff\xff\xff\xff\xff\xff\x02\x00\x00\x00\x00\x00\x00'
        b'\x00\x00\x00\x00\x00\x00\x00\x02\x85\x00\x7d\x37\x00\x00\x00\x00')
    nonce = creat_rs_nonce()
    blank_rs_packet[4:12] = nonce
    assert(teredo_rs_packet(nonce).type_restricted == bytes(blank_rs_packet))

    server_list = ['teredo.remlab.net','win1710.ipv6.microsoft.com']
    prober = teredo_prober(server_list, probe_nat=False)
    prober.timeout = 4
    server_ip_list = prober.server_ip_list.copy()
    server_ip = server_ip_list.pop()
    for _ in range(2):
        print(prober.qualify_loop(server_ip))
        prober.rs_cone_flag = prober.rs_cone_flag ^ 1
    server_ip = server_ip_list.pop()
    for _ in range(2):
        print(prober.qualify_loop(server_ip))
        prober.rs_cone_flag = prober.rs_cone_flag ^ 1
#    prober.close()

    print(main())
    raw_input(confirm_over)
    sys.exit(0)

if '__main__' == __name__:
#    test()
    args = sys.argv[1:]
    if '-h' in args:
        args.remove('-h')
        print(help_info)
        if not args:
            raw_input(confirm_over)
            sys.exit(0)
    try:
        local_port = args[args.index('-p') + 1]
        args.remove('-p')
        args.remove(local_port)
        try:
            local_port = int(local_port)
        except:
            local_port = None
            print(warn_1)
    except:
        local_port = None
    try:
        remote_port = args[args.index('-P') + 1]
        args.remove('-P')
        args.remove(remote_port)
        try:
            remote_port = int(remote_port)
        except:
            remote_port = None
            print(warn_2)
    except:
        remote_port = None
    done_disabled = False
    if os.name == 'nt':
        if raw_input(confirm_stop).lower() == 'y':
            if runas('netsh interface teredo set state disable'):
                done_disabled = True
        win32runas.runas("win_reset_gp.py")
        print(os.system('netsh interface teredo show state'))
    recommend, nat_type = main(*args, local_port=local_port, remote_port=remote_port)
    print(result_info % recommend)
    if os.name == 'nt':
        ip = [a for a in os.popen('route print').readlines() if ' 0.0.0.0 ' in a][0].split()[-2]
        if nat_type == 'cone':
            client = 'client'
        else:
            import platform
            client_ext = 'natawareclient' if platform.version()[0] > '6' else 'enterpriseclient'
            client = client_ext if ip.startswith(local_ip_startswith) else 'client'
    if recommend:
        if os.name == 'nt' and \
                raw_input(confirm_set).lower() == 'y':
            cmd = 'netsh interface teredo set state type=%s servername=%s.'
            if raw_input(confirm_reset).lower() == 'y':
                cmd += ' refreshinterval=default'
            if not remote_port:
                cmd += ' clientport=default'
            runas(cmd % (client, recommend[0]))
            print(wait_info)
            time.sleep(10)
            print(os.system('netsh interface teredo show state'))
            done_disabled = False
    if done_disabled:
        if runas('netsh interface teredo set state type=%s' % client):
            print(resume_info)
    raw_input(confirm_over)

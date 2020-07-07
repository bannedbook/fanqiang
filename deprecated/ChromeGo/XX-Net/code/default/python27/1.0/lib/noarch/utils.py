#!/usr/bin/env python
# -*- coding: utf-8 -*-

import re
import os
import threading

g_ip_check = re.compile(r'^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$')


def check_ip_valid4(ip):
    """检查ipv4地址的合法性"""
    ret = g_ip_check.match(ip)
    if ret is not None:
        "each item range: [0,255]"
        for item in ret.groups():
            if int(item) > 255:
                return 0
        return 1
    else:
        return 0


def check_ip_valid6(ip):
    """Copied from http://stackoverflow.com/a/319293/2755602"""
    """Validates IPv6 addresses.
    """
    pattern = re.compile(r"""
        ^
        \s*                         # Leading whitespace
        (?!.*::.*::)                # Only a single whildcard allowed
        (?:(?!:)|:(?=:))            # Colon iff it would be part of a wildcard
        (?:                         # Repeat 6 times:
            [0-9a-f]{0,4}           #   A group of at most four hexadecimal digits
            (?:(?<=::)|(?<!::):)    #   Colon unless preceeded by wildcard
        ){6}                        #
        (?:                         # Either
            [0-9a-f]{0,4}           #   Another group
            (?:(?<=::)|(?<!::):)    #   Colon unless preceeded by wildcard
            [0-9a-f]{0,4}           #   Last group
            (?: (?<=::)             #   Colon iff preceeded by exacly one colon
             |  (?<!:)              #
             |  (?<=:) (?<!::) :    #
             )                      # OR
         |                          #   A v4 address with NO leading zeros
            (?:25[0-4]|2[0-4]\d|1\d\d|[1-9]?\d)
            (?: \.
                (?:25[0-4]|2[0-4]\d|1\d\d|[1-9]?\d)
            ){3}
        )
        \s*                         # Trailing whitespace
        $
    """, re.VERBOSE | re.IGNORECASE | re.DOTALL)
    return pattern.match(ip) is not None


def check_ip_valid(ip):
    if ':' in ip:
        return check_ip_valid6(ip)
    else:
        return check_ip_valid4(ip)


domain_allowed = re.compile("(?!-)[A-Z\d-]{1,63}(?<!-)$")


def check_domain_valid(hostname):
    if len(hostname) > 255:
        return False
    if hostname.endswith("."):
        hostname = hostname[:-1]

    return all(domain_allowed.match(x) for x in hostname.split("."))


def str2hex(data):
    return ":".join("{:02x}".format(ord(c)) for c in data)


def get_ip_maskc(ip_str):
    head = ".".join(ip_str.split(".")[:-1])
    return head + ".0"


def split_ip(strline):
    """从每组地址中分离出起始IP以及结束IP"""
    begin = ""
    end = ""
    if "-" in strline:
        num_regions = strline.split(".")
        if len(num_regions) == 4:
            "xxx.xxx.xxx-xxx.xxx-xxx"
            begin = ''
            end = ''
            for region in num_regions:
                if '-' in region:
                    s, e = region.split('-')
                    begin += '.' + s
                    end += '.' + e
                else:
                    begin += '.' + region
                    end += '.' + region
            begin = begin[1:]
            end = end[1:]

        else:
            "xxx.xxx.xxx.xxx-xxx.xxx.xxx.xxx"
            begin, end = strline.split("-")
            if 1 <= len(end) <= 3:
                prefix = begin[0:begin.rfind(".")]
                end = prefix + "." + end

    elif strline.endswith("."):
        "xxx.xxx.xxx."
        begin = strline + "0"
        end = strline + "255"
    elif "/" in strline:
        "xxx.xxx.xxx.xxx/xx"
        (ip, bits) = strline.split("/")
        if check_ip_valid4(ip) and (0 <= int(bits) <= 32):
            orgip = ip_string_to_num(ip)
            end_bits = (1 << (32 - int(bits))) - 1
            begin_bits = 0xFFFFFFFF ^ end_bits
            begin = ip_num_to_string(orgip & begin_bits)
            end = ip_num_to_string(orgip | end_bits)
    else:
        "xxx.xxx.xxx.xxx"
        begin = strline
        end = strline

    return begin, end


def generate_random_lowercase(n):
    min_lc = ord(b'a')
    len_lc = 26
    ba = bytearray(os.urandom(n))
    for i, b in enumerate(ba):
        ba[i] = min_lc + b % len_lc # convert 0..255 to 97..122
    #sys.stdout.buffer.write(ba)
    return ba


class SimpleCondition(object):
    def __init__(self):
        self.lock = threading.Condition()

    def notify(self):
        self.lock.acquire()
        self.lock.notify()
        self.lock.release()

    def wait(self):
        self.lock.acquire()
        self.lock.wait()
        self.lock.release()


def split_domain(host):
    hl = host.split(".")
    return hl[0], ".".join(hl[1:])


def ip_string_to_num(s):
    """Convert dotted IPv4 address to integer."""
    return reduce(lambda a, b: a << 8 | b, map(int, s.split(".")))


def ip_num_to_string(ip):
    """Convert 32-bit integer to dotted IPv4 address."""
    return ".".join(map(lambda n: str(ip >> n & 0xFF), [24, 16, 8, 0]))


private_ipv4_range = [
    ("10.0.0.0", "10.255.255.255"),
    ("127.0.0.0", "127.255.255.255"),
    ("169.254.0.0", "169.254.255.255"),
    ("172.16.0.0", "172.31.255.255"),
    ("192.168.0.0", "192.168.255.255")
]

private_ipv6_range = [
    ("::1", "::1"),
    ("fc00::", "fdff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")
]


private_ipv4_range_bin = []
for b, e in private_ipv4_range:
    bb = ip_string_to_num(b)
    ee = ip_string_to_num(e)
    private_ipv4_range_bin.append((bb, ee))


def is_private_ip(ip):
    try:
        if "." in ip:
            ip_bin = ip_string_to_num(ip)
            for b, e in private_ipv4_range_bin:
                if b <= ip_bin <= e:
                    return True
            return False
        else:
            if ip == "::1":
                return True

            fi = ip.find(":")
            if fi != 4:
                return False

            be = ip[0:2]
            if be in ["fc", "fd"]:
                return True
            else:
                return False
    except Exception as e:
        print("is_private_ip(%s), except:%r", ip, e)
        return False


import string
printable = set(string.printable)


def get_printable(s):
    return filter(lambda x: x in printable, s)


if __name__ == '__main__':
    print(is_private_ip("fa00::1"))
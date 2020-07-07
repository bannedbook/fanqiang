#!/usr/bin/env python
# -*- coding: utf-8 -*-
import random
import time
import os
import threading
import struct

import utils
import random_get_slice

random.seed(time.time()* 1000000)


class IpSimpleSource(object):
    def __init__(self, ips=[]):
        self.ips = ips

    def set_ips(self, ips):
        self.ips = ips

    def get_ip(self):
        if not self.ips:
            return ""
        else:
            return random.choice(self.ips)


class Ipv4RangeSource(object):
    def __init__(self, logger, config, default_range_fn, user_range_fn):
        self.logger = logger
        self.config = config

        self.default_range_file = default_range_fn
        self.user_range_fn = user_range_fn
        self.load_ip_range()

    def load_range_content(self, default=False):
        if not default and os.path.isfile(self.user_range_fn):
            fd = open(self.user_range_fn, "r")
            if fd:
                content = fd.read()
                fd.close()
                if len(content) > 10:
                    self.logger.info("load ip range file:%s", self.user_range_fn)
                    return content

        self.logger.info("load ip range file:%s", self.default_range_file)
        fd = open(self.default_range_file, "r")
        if not fd:
            self.logger.error("load ip range %s fail", self.default_range_file)
            return

        content = fd.read()
        fd.close()
        return content

    def update_range_content(self, content):
        with open(self.user_range_fn, "w") as fd:
            fd.write(content)

    def remove_user_range(self):
        try:
            os.remove(self.user_range_fn)
        except:
            pass

    def load_ip_range(self):
        self.ip_range_map = {}
        self.ip_range_list = []
        self.ip_range_index = []
        self.candidate_amount_ip = 0

        content = self.load_range_content()
        lines = content.splitlines()
        for line in lines:
            if len(line) == 0 or line[0] == '#':
                continue

            try:
                begin, end = utils.split_ip(line)
                nbegin = utils.ip_string_to_num(begin)
                nend = utils.ip_string_to_num(end)
                if not nbegin or not nend or nend < nbegin:
                    self.logger.warn("load ip range:%s fail", line)
                    continue
            except Exception as e:
                self.logger.exception("load ip range:%s fail:%r", line, e)
                continue

            self.ip_range_map[self.candidate_amount_ip] = [nbegin, nend]
            self.ip_range_list.append( [nbegin, nend] )
            self.ip_range_index.append(self.candidate_amount_ip)
            num = nend - nbegin
            self.candidate_amount_ip += num
            # print utils.ip_num_to_string(nbegin), utils.ip_num_to_string(nend), num

        self.ip_range_index.sort()
        #print "amount ip num:", self.candidate_amount_ip

    def get_ip(self):
        while True:
            index = random.randint(0, len(self.ip_range_list) - 1)
            ip_range = self.ip_range_list[index]
            #self.logger.debug("random.randint %d - %d", ip_range[0], ip_range[1])
            if ip_range[1] == ip_range[0]:
                return utils.ip_num_to_string(ip_range[1])

            try:
                id_2 = random.randint(0, ip_range[1] - ip_range[0])
            except Exception as e:
                self.logger.exception("random.randint:%r %d - %d, %d", e, ip_range[0], ip_range[1], ip_range[1] - ip_range[0])
                return

            ip = ip_range[0] + id_2
            add_last_byte = ip % 256
            if add_last_byte == 0 or add_last_byte == 255:
                continue

            return utils.ip_num_to_string(ip)


class Ipv4PoolSource(object):
    def __init__(self, logger, source_txt_fn, dest_bin_fn):
        self.logger = logger
        self.source_txt_fn = source_txt_fn
        self.dest_bin_fn = dest_bin_fn

        self.bin_fd = None
        threading.Thread(target=self.init).start()

    def init(self):
        if not self.check_bin():
            self.generate_bin()
        self.bin_fd = open(self.dest_bin_fn, "rb")
        self.bin_size = os.path.getsize(self.dest_bin_fn)

    def check_bin(self):
        if not os.path.isfile(self.dest_bin_fn):
            return False

        if os.path.getmtime(self.dest_bin_fn) < os.path.getmtime(self.source_txt_fn):
            return False

        return True

    def generate_bin(self):
        self.logger.info("generating binary ip pool file.")
        rfd = open(self.source_txt_fn, "rt")
        wfd = open(self.dest_bin_fn, "wb")
        num = 0
        for line in rfd.readlines():
            ip = line
            try:
                ip_num = utils.ip_string_to_num(ip)
            except Exception as e:
                self.logger.warn("ip %s not valid in %s", ip, self.source_txt_fn)
                continue
            ip_bin = struct.pack("<I", ip_num)
            wfd.write(ip_bin)
            num += 1

        rfd.close()
        wfd.close()
        self.logger.info("finished generate binary ip pool file, num:%d", num)

    def get_ip(self):
        while self.bin_fd is None:
            time.sleep(1)

        for _ in range(5):
            position = random.randint(0, self.bin_size/4) * 4
            self.bin_fd.seek(position)
            ip_bin = self.bin_fd.read(4)
            if ip_bin is None:
                self.logger.warn("ip_pool.random_get_ip position:%d get None", position)
            elif len(ip_bin) != 4:
                self.logger.warn("ip_pool.random_get_ip position:%d len:%d", position, len(ip_bin))
            else:
                ip_num = struct.unpack("<I", ip_bin)[0]
                ip = utils.ip_num_to_string(ip_num)
                return ip
        time.sleep(3)
        raise Exception("get ip fail.")


class Ipv6PoolSource(object):
    def __init__(self, logger, config, list_fn):
        self.logger = logger
        self.config = config
        self.list_fn = list_fn

        self.source = random_get_slice.RandomGetSlice(list_fn, 200)

    def get_ip(self):
        line = self.source.get()
        lp = line.split()
        ip = lp[0]
        return ip


class IpCombineSource(object):
    def __init__(self, logger, config, ipv4_source, ipv6_source):
        self.logger = logger
        self.config = config
        self.ipv4_source = ipv4_source
        self.ipv6_source = ipv6_source

    def get_ip(self, use_ipv6=None):
        if use_ipv6 is None:
            use_ipv6 = self.config.use_ipv6

        if use_ipv6 == "force_ipv4":
            return self.ipv4_source.get_ip()
        elif use_ipv6 == "force_ipv6":
            return self.ipv6_source.get_ip()
        else:
            if use_ipv6 != "auto":
                self.logger.warn("IpRange get_ip but use_ip is %s", use_ipv6)

            ran = random.randint(0, 100)
            if ran < self.config.ipv6_scan_ratio:
                return self.ipv6_source.get_ip()
            else:
                return self.ipv4_source.get_ip()

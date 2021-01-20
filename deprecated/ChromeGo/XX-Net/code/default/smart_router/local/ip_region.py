#!/usr/bin/env python
# coding: utf-8

import os
import struct
import sys
import socket


current_path = os.path.dirname(os.path.abspath(__file__))
launcher_path = os.path.abspath( os.path.join(current_path, os.pardir, os.pardir, "launcher"))

root_path = os.path.abspath(os.path.join(current_path, os.pardir, os.pardir))
top_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir))
data_path = os.path.join(top_path, 'data', "smart_router")

if __name__ == '__main__':
    python_path = os.path.join(root_path, 'python27', '1.0')
    noarch_lib = os.path.abspath(os.path.join(python_path, 'lib', 'noarch'))
    sys.path.append(noarch_lib)

import utils
from xlog import getLogger
xlog = getLogger("smart_router")


class IpRegion(object):
    cn_ipv4_range = os.path.join(current_path, "cn_ipv4_range.txt")
    cn_ipdb = os.path.join(data_path, "cn_ipdb.dat")

    def __init__(self):
        self.cn = "CN"
        self.ipdb = self.load_db()

    def load_db(self):
        if not os.path.isfile(self.cn_ipdb):
            self.generate_db()

        with open(self.cn_ipdb, 'rb') as f:
            # 读取 IP 范围数据长度 BE Ulong -> int
            data_len, = struct.unpack('>L', f.read(4))
            # 读取索引数据
            index = f.read(224 * 4)
            # 读取 IP 范围数据
            data = f.read(data_len)
            # 简单验证结束
            if f.read(3) != b'end':
                raise ValueError('%s file format error' % self.cn_ipdb)
            # 读取更新信息
            self.update = f.read().decode('ascii')
        # 格式化并缓存索引数据
        # 使用 struct.unpack 一次性分割数据效率更高
        # 每 4 字节为一个索引范围 fip：BE short -> int，对应 IP 范围序数
        self.index = struct.unpack('>' + 'h' * (224 * 2), index)
        # 每 8 字节对应一段直连 IP 范围和一段非直连 IP 范围
        self.data = struct.unpack('4s' * (data_len // 4), data)

    def check_ip(self, ip):
        if ":" in ip:
            return False

        #转换 IP 为 BE Uint32，实际类型 bytes
        nip = socket.inet_aton(ip)
        #确定索引范围
        index = self.index
        fip = ord(nip[0])
        #从 224 开始都属于保留地址
        if fip >= 224:
            return True
        fip *= 2
        lo = index[fip]
        if lo < 0:
            return False
        hi = index[fip + 1]
        #与 IP 范围比较确定 IP 位置
        data = self.data
        while lo < hi:
            mid = (lo + hi) // 2
            if data[mid] > nip:
                hi = mid
            else:
                lo = mid + 1
        #根据位置序数奇偶确定是否属于直连 IP
        return lo & 1

    def check_ips(self, ips):
        for ipd in ips:
            if "|" in ipd:
                ipl = ipd.split("|")
                ip = ipl[0]
            else:
                ip = ipd

            try:
                if self.check_ip(ip):
                    return True
            except Exception as e:
                xlog.exception("check ip %s fail:%r", ip, e)

        return False

    def generate_db(self):
        keeprange = (
                     '0.0.0.0/8',  # 本地网络
                     '10.0.0.0/8',  # 私有网络
                     '100.64.0.0/10',  # 地址共享（运营商 NAT）
                     '127.0.0.0/8',  # 环回地址
                     '169.254.0.0/16',  # 链路本地
                     '172.16.0.0/12',  # 私有网络
                     '192.0.0.0/24',  # 保留地址（IANA）
                     '192.0.2.0/24',  # TEST-NET-1
                     '192.88.99.0/24',  # 6to4 中继
                     '192.168.0.0/16',  # 私有网络
                     '198.18.0.0/15',  # 网络基准测试
                     '198.51.100.0/24',  # TEST-NET-2
                     '203.0.113.0/24',  # TEST-NET-3
                     # 连续地址直到 IP 结束，特殊处理
                     # '224.0.0.0/4',  #组播地址（D类）
                     # '240.0.0.0/4',  #保留地址（E类）
                     )
        keeplist = []
        for iprange in keeprange:
            ip, mask = iprange.split('/')
            keeplist.append((utils.ip_string_to_num(ip), 32 - int(mask)))

        mask_dict = dict((str(2 ** i), i) for i in range(8, 25))

        def int2bytes2(n, pack=struct.pack):
            '''将整数转换为大端序字节'''
            return pack('>H', n)
            # return bytes(map(lambda b: (-1 >> b & 255), (8, 0)))

        def int2bytes4(n, pack=struct.pack):
            '''将整数转换为大端序字节'''
            return pack('>I', n)
            # return bytes(map(lambda b: (n >> b & 255), (24, 16, 8, 0)))

        def bytes2int(s):

            nchars = len(s)
            # string to int or long. Type depends on nchars
            x = sum(ord(s[byte]) << 8 * (nchars - byte - 1) for byte in range(nchars))
            return x


        #    +---------+
        #    | 4 bytes |                     <- data length
        #    +---------------+
        #    | 224 * 4 bytes |               <- first ip number index
        #    +---------------+
        #    |  2n * 4 bytes |               <- cn ip ranges data
        #    +------------------------+
        #    | b'end' and update info |      <- end verify
        #    +------------------------+
        lastip_s = 0
        lastip_e = 0
        index = {}
        index_n = 0
        index_fip = -1
        offset = 0

        padding = b'\xff\xff'
        update = ""

        iplist = []
        fdi = open(self.cn_ipv4_range,"r")
        for line in fdi.readlines():
            lp = line.split()
            iplist.append((utils.ip_string_to_num(lp[0]), mask_dict[lp[1]]))

        iplist.extend(keeplist)
        # 排序，不然无法处理
        iplist.sort(key=lambda x: x[0])
        # 随便算一下
        buffering = len(iplist) * 8 + 224 * 4 + 64 + 4
        buffer = bytearray(buffering)
        for ip, mask in iplist:
            ip_s = ip >> mask << mask
            ip_e = (ip >> mask) + 1 << mask
            # 判断连续
            if ip_s <= lastip_e:
                # 判断覆盖
                if ip_e > lastip_e:
                    lastip_e = ip_e
                continue
            # 排除初始值
            if lastip_e:
                # 一段范围分为包含和排除
                buffer[offset:] = lastip_s = int2bytes4(lastip_s)
                buffer[offset + 4:] = int2bytes4(lastip_e)
                # 一个索引分为开始和结束
                fip = ord(lastip_s[0]) * 2
                if fip != index_fip:
                    # 前一个索引结束，序数多 1
                    # 避免无法搜索从当前索引结尾地址到下个索引开始地址
                    index[index_fip + 1] = index_b = int2bytes2(index_n)
                    # 当前索引开始
                    index[fip] = index_b
                    index_fip = fip
                index_n += 2
                offset += 8
            lastip_s = ip_s
            lastip_e = ip_e
        # 添加最后一段范围
        buffer[offset:] = lastip_s = int2bytes4(lastip_s)
        buffer[offset + 4:] = int2bytes4(lastip_e)
        fip = ord(lastip_s[0]) * 2
        if fip != index_fip:
            index[index_fip + 1] = index_b = int2bytes2(index_n)
            index[fip] = index_b
        index_n += 2
        offset += 8
        # 添加最后一个结束索引
        index[fip + 1] = int2bytes2(index_n)
        # 写入文件
        fd = open(self.cn_ipdb, 'wb', buffering)
        fd.write(int2bytes4(offset))
        for i in xrange(224 * 2):
            fd.write(index.get(i, padding))
        fd.write(buffer[:offset])
        fd.write('endCN IP from ')
        fd.write(update.encode(u'ascii'))

        count = int(index_n // 2)
        fd.write(', range count: %d' % count)
        fd.close()

        xlog.debug('include IP range number：%s' % count)
        xlog.debug('save to file:%s' % self.cn_ipdb)


class UpdateIpRange(object):
    cn_ipv4_range = os.path.join(current_path, "cn_ipv4_range.txt")

    def __init__(self):
        fn = os.path.join(data_path, "apnic.txt")

        self.download_apnic(fn)
        self.save_apnic_cniplist(fn)

    def download_apnic(self, fn):
        import subprocess
        import sys
        import urllib2
        url = 'https://ftp.apnic.net/apnic/stats/apnic/delegated-apnic-latest'
        try:
            data = subprocess.check_output(['wget', url, '-O-'])
        except (OSError, AttributeError):
            print >> sys.stderr, "Fetching data from apnic.net, " \
                                 "it might take a few minutes, please wait..."
            data = urllib2.urlopen(url).read()

        with open(fn, "w") as f:
            f.write(data)
        return data

    def save_apnic_cniplist(self, fn):
        try:
            fd = open(fn, "r")
            fw = open(self.cn_ipv4_range, "w")
            for line in fd.readlines():
                if line.startswith(b'apnic|CN|ipv4'):
                    ip = line.decode().split('|')
                    if len(ip) > 5:
                        fw.write("%s %s\n" % (ip[3], ip[4]))

        except Exception as e:
            xlog.exception("parse_apnic_cniplist %s e:%r", fn, e)


if __name__ == '__main__':
    #up = UpdateIpRange()
    ipr = IpRegion()
    print(ipr.check_ip("8.8.8.8"))
    print(ipr.check_ip("114.111.114.114"))
#!/usr/bin/python
# -*- encoding: utf8 -*-

import sys

import IPy


def main():
    china_list_set = IPy.IPSet()
    for line in sys.stdin:
        china_list_set.add(IPy.IP(line))

    # 输出结果
    for ip in china_list_set:
        print '<item>' + str(ip) + '</item>'


if __name__ == "__main__":
    main()

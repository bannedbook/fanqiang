#!/usr/bin/python
# -*- coding: utf-8 -*-

import pkgutil
import urlparse
import socket
import logging
from argparse import ArgumentParser
from datetime import date

__all__ = ['main']


def parse_args():
    parser = ArgumentParser()
    parser.add_argument('-i', '--input', dest='input', required=True,
                      help='path to gfwlist', metavar='GFWLIST')
    parser.add_argument('-f', '--file', dest='output', required=True,
                      help='path to output acl', metavar='ACL')
    return parser.parse_args()


def decode_gfwlist(content):
    # decode base64 if have to
    try:
        return content.decode('base64')
    except:
        return content


def get_hostname(something):
    try:
        # quite enough for GFW
        if not something.startswith('http:'):
            something = 'http://' + something
        r = urlparse.urlparse(something)
        return r.hostname
    except Exception as e:
        logging.error(e) 
        return None


def add_domain_to_set(s, something):
    hostname = get_hostname(something)
    if hostname is not None:
        if hostname.startswith('.'):
            hostname = hostname.lstrip('.')
        if hostname.endswith('/'):
            hostname = hostname.rstrip('/')
        if hostname:
            s.add(hostname)


def parse_gfwlist(content):
    gfwlist = content.splitlines(False)
    domains = set()
    for line in gfwlist:
        if line.find('.*') >= 0:
            continue
        elif line.find('*') >= 0:
            line = line.replace('*', '/')
        if line.startswith('!'):
            continue
        elif line.startswith('['):
            continue
        elif line.startswith('@'):
            # ignore white list
            continue
        elif line.startswith('||'):
            add_domain_to_set(domains, line.lstrip('||'))
        elif line.startswith('|'):
            add_domain_to_set(domains, line.lstrip('|'))
        elif line.startswith('.'):
            add_domain_to_set(domains, line.lstrip('.'))
        else:
            add_domain_to_set(domains, line)
    # TODO: reduce ['www.google.com', 'google.com'] to ['google.com']
    return domains


def generate_acl(domains):
    header ="""#
# GFW list from https://github.com/gfwlist/gfwlist/blob/master/gfwlist.txt
# updated on DATE
#

[bypass_all]

[proxy_list]

"""
    header = header.replace('DATE', str(date.today()))
    proxy_content = ""
    ip_content = ""

    for domain in sorted(domains):
        try:
            socket.inet_aton(domain)
            ip_content += (domain + "\n")
        except socket.error:
            domain = domain.replace('.', '\.')
            proxy_content += ('(?:^|\.)' + domain + '$\n')

    proxy_content = header + ip_content + proxy_content

    return proxy_content


def main():
    args = parse_args()
    with open(args.input, 'rb') as f:
        content = f.read()
    content = decode_gfwlist(content)
    domains = parse_gfwlist(content)
    acl_content = generate_acl(domains)
    with open(args.output, 'wb') as f:
        f.write(acl_content)

if __name__ == '__main__':
    main()


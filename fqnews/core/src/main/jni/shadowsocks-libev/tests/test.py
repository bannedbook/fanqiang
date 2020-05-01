#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Copyright 2015 clowwindy
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.

from __future__ import absolute_import, division, print_function, \
    with_statement

import sys
import os
import signal
import select
import time
import argparse
from subprocess import Popen, PIPE

default_url = 'http://www.google.com/'

parser = argparse.ArgumentParser(description='test Shadowsocks')
parser.add_argument('-c', '--client-conf', type=str, default=None)
parser.add_argument('-s', '--server-conf', type=str, default=None)
parser.add_argument('-a', '--client-args', type=str, default=None)
parser.add_argument('-b', '--server-args', type=str, default=None)
parser.add_argument('--should-fail', action='store_true', default=None)
parser.add_argument('--url', type=str, default=default_url)
parser.add_argument('--dns', type=str, default='8.8.8.8')
parser.add_argument('--bin', type=str, default='')

config = parser.parse_args()

client_args = ['%s%s' % (config.bin, 'ss-local'), '-v']
server_args = ['%s%s' % (config.bin, 'ss-server'), '-v', '-u']
tunnel_args = ['%s%s' % (config.bin, 'ss-tunnel'), '-v', '-u', '-l1082', '-L%s:53' % config.dns]

if config.client_conf:
    client_args.extend(['-c', config.client_conf])
    tunnel_args.extend(['-c', config.client_conf])
    if config.server_conf:
        server_args.extend(['-c', config.server_conf])
    else:
        server_args.extend(['-c', config.client_conf])

if config.client_args:
    client_args.extend(config.client_args.split())
    tunnel_args.extend(config.client_args.split())
    if config.server_args:
        server_args.extend(config.server_args.split())
    else:
        server_args.extend(config.client_args.split())

p1 = Popen(server_args, stdin=PIPE, stdout=PIPE, stderr=PIPE, close_fds=True)
p2 = Popen(client_args, stdin=PIPE, stdout=PIPE, stderr=PIPE, close_fds=True)
p5 = Popen(tunnel_args, stdin=PIPE, stdout=PIPE, stderr=PIPE, close_fds=True)
p3 = None
p4 = None
p3_fin = False
p4_fin = False

# 1 shadowsocks started
# 2 curl started
# 3 curl finished
# 4 dig started
# 5 dig finished
stage = 1

try:
    fdset = []
    time.sleep(2)

    p3 = Popen(['curl', config.url, '-v', '-L',
        '--socks5-hostname', '127.0.0.1:1081',
        '-m', '15', '--connect-timeout', '10'],
        stdin=PIPE, stdout=PIPE, stderr=PIPE, close_fds=True)
    if p3 is not None:
        fdset.append(p3.stdout)
        fdset.append(p3.stderr)
        stage = 2
    else:
        sys.exit(1)

    while True:
        r, w, e = select.select(fdset, [], fdset)
        if e:
            break

        for fd in r:
            line = fd.readline()
            if not line:
                if stage == 2 and fd == p3.stdout:
                    stage = 3
                if stage == 4 and fd == p4.stdout:
                    stage = 5
            if bytes != str:
                line = str(line, 'utf8')
            sys.stderr.write(line)

        if stage == 3 and p3 is not None:
            fdset.remove(p3.stdout)
            fdset.remove(p3.stderr)
            r = p3.wait()
            if config.should_fail:
                if r == 0:
                    sys.exit(1)
            else:
                if r != 0:
                    sys.exit(1)
            p4 = Popen(['dig', '@127.0.0.1', '-p1082',
                        'www.google.com'],
                       stdin=PIPE, stdout=PIPE, stderr=PIPE, close_fds=True)
            if p4 is not None:
                fdset.append(p4.stdout)
                fdset.append(p4.stderr)
                stage = 4
            else:
                sys.exit(1)

        if stage == 5:
            r = p4.wait()
            if config.should_fail:
                if r == 0:
                    sys.exit(1)
                print('test passed (expecting failure)')
            else:
                if r != 0:
                    sys.exit(1)
                print('test passed')
            break
finally:
    for p in [p1, p2, p5]:
        try:
            os.kill(p.pid, signal.SIGINT)
            os.waitpid(p.pid, 0)
        except OSError:
            pass

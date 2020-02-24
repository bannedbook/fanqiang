#!/usr/bin/env python
# coding:utf-8
# Based on GAppProxy 2.0.0 by Du XiaoGang <dugang.2008@gmail.com>
# Based on WallProxy 0.4.0 by Hust Moon <www.ehust@gmail.com>
# Contributor:
#      Phus Lu           <phus.lu@gmail.com>
#      Hewig Xu          <hewigovens@gmail.com>
#      Ayanamist Yang    <ayanamist@gmail.com>
#      V.E.O             <V.E.O@tom.com>
#      Max Lv            <max.c.lv@gmail.com>
#      AlsoTang          <alsotang@gmail.com>
#      Christopher Meng  <i@cicku.me>
#      Yonsm Guo         <YonsmGuo@gmail.com>
#      Parkman           <cseparkman@gmail.com>
#      Ming Bai          <mbbill@gmail.com>
#      Bin Yu            <yubinlove1991@gmail.com>
#      lileixuan         <lileixuan@gmail.com>
#      Cong Ding         <cong@cding.org>
#      Zhang Youfu       <zhangyoufu@gmail.com>
#      Lu Wei            <luwei@barfoo>
#      Harmony Meow      <harmony.meow@gmail.com>
#      logostream        <logostream@gmail.com>
#      Rui Wang          <isnowfy@gmail.com>
#      Wang Wei Qiang    <wwqgtxx@gmail.com>
#      Felix Yan         <felixonmars@gmail.com>
#      Sui Feng          <suifeng.me@qq.com>
#      QXO               <qxodream@gmail.com>
#      Geek An           <geekan@foxmail.com>
#      Poly Rabbit       <mcx_221@foxmail.com>
#      oxnz              <yunxinyi@gmail.com>
#      Shusen Liu        <liushusen.smart@gmail.com>
#      Yad Smood         <y.s.inside@gmail.com>
#      Chen Shuang       <cs0x7f@gmail.com>
#      cnfuyu            <cnfuyu@gmail.com>
#      cuixin            <steven.cuixin@gmail.com>
#      s2marine0         <s2marine0@gmail.com>
#      Toshio Xiang      <snachx@gmail.com>
#      Bo Tian           <dxmtb@163.com>
#      Virgil            <variousvirgil@gmail.com>
#      hub01             <miaojiabumiao@yeah.net>
#      v3aqb             <sgzz.cj@gmail.com>
#      Oling Cat         <olingcat@gmail.com>
#      Meng Zhuo         <mengzhuo1203@gmail.com>
#      zwhfly            <zwhfly@163.com>
#      Hubertzhang       <hubert.zyk@gmail.com>
#      arrix             <arrixzhou@gmail.com>
#      gwjwin            <gwjwin@sina.com>
#      Zhuhao Wang       <zhuhaow@gmail.com>
#      YFdyh000          <yfdyh000@gmail.com>
#      zzq1015           <zzq1015@users.noreply.github.com>
#      Zhengfa Dang      <zfdang@users.noreply.github.com>
#      haosdent          <haosdent@gmail.com>
#      xk liu            <lxk1012@gmail.com>

__version__ = '3.2.3'

import os
import sys
import sysconfig

reload(sys).setdefaultencoding('UTF-8')
sys.dont_write_bytecode = True
sys.path = [(os.path.dirname(__file__) or '.') + '/packages.egg/noarch'] + sys.path + [(os.path.dirname(__file__) or '.') + '/packages.egg/' + sysconfig.get_platform().split('-')[0]]

try:
    __import__('gevent.monkey', fromlist=['.']).patch_all()
except (ImportError, SystemError):
    sys.exit(sys.stderr.write('please install python-gevent\n'))

import base64
import collections
import ConfigParser
import errno
import httplib
import io
import locale
import Queue
import random
import re
import socket
import ssl
import struct
import thread
import threading
import time
import traceback
import urllib2
import urlparse
import zlib
import select

import gevent
import gevent.server
import OpenSSL

NetWorkIOError = (socket.error, ssl.SSLError, OpenSSL.SSL.Error, OSError)

class Logging(type(sys)):
    CRITICAL = 50
    FATAL = CRITICAL
    ERROR = 40
    WARNING = 30
    WARN = WARNING
    INFO = 20
    DEBUG = 10
    NOTSET = 0

    def __init__(self, *args, **kwargs):
        self.level = self.__class__.INFO
        self.__set_error_color = lambda: None
        self.__set_warning_color = lambda: None
        self.__set_debug_color = lambda: None
        self.__reset_color = lambda: None
        if hasattr(sys.stderr, 'isatty') and sys.stderr.isatty():
            if os.name == 'nt':
                import ctypes
                SetConsoleTextAttribute = ctypes.windll.kernel32.SetConsoleTextAttribute
                GetStdHandle = ctypes.windll.kernel32.GetStdHandle
                self.__set_error_color = lambda: SetConsoleTextAttribute(GetStdHandle(-11), 0x04)
                self.__set_warning_color = lambda: SetConsoleTextAttribute(GetStdHandle(-11), 0x06)
                self.__set_debug_color = lambda: SetConsoleTextAttribute(GetStdHandle(-11), 0x002)
                self.__reset_color = lambda: SetConsoleTextAttribute(GetStdHandle(-11), 0x07)
            elif os.name == 'posix':
                self.__set_error_color = lambda: sys.stderr.write('\033[31m')
                self.__set_warning_color = lambda: sys.stderr.write('\033[33m')
                self.__set_debug_color = lambda: sys.stderr.write('\033[32m')
                self.__reset_color = lambda: sys.stderr.write('\033[0m')

    @classmethod
    def getLogger(cls, *args, **kwargs):
        return cls(*args, **kwargs)

    def basicConfig(self, *args, **kwargs):
        self.level = int(kwargs.get('level', self.__class__.INFO))
        if self.level > self.__class__.DEBUG:
            self.debug = self.dummy

    def log(self, level, fmt, *args, **kwargs):
        sys.stderr.write('%s - [%s] %s\n' % (level, time.ctime()[4:-5], fmt % args))

    def dummy(self, *args, **kwargs):
        pass

    def debug(self, fmt, *args, **kwargs):
        self.__set_debug_color()
        self.log('DEBUG', fmt, *args, **kwargs)
        self.__reset_color()

    def info(self, fmt, *args, **kwargs):
        self.log('INFO', fmt, *args)

    def warning(self, fmt, *args, **kwargs):
        self.__set_warning_color()
        self.log('WARNING', fmt, *args, **kwargs)
        self.__reset_color()

    def warn(self, fmt, *args, **kwargs):
        self.warning(fmt, *args, **kwargs)

    def error(self, fmt, *args, **kwargs):
        self.__set_error_color()
        self.log('ERROR', fmt, *args, **kwargs)
        self.__reset_color()

    def exception(self, fmt, *args, **kwargs):
        self.error(fmt, *args, **kwargs)
        sys.stderr.write(traceback.format_exc() + '\n')

    def critical(self, fmt, *args, **kwargs):
        self.__set_error_color()
        self.log('CRITICAL', fmt, *args, **kwargs)
        self.__reset_color()
logging = sys.modules['logging'] = Logging('logging')


from proxylib import AuthFilter
from proxylib import AutoRangeFilter
from proxylib import BaseFetchPlugin
from proxylib import BaseProxyHandlerFilter
from proxylib import BlackholeFilter
from proxylib import CertUtil
from proxylib import CipherFileObject
from proxylib import deflate
from proxylib import DirectFetchPlugin
from proxylib import DirectRegionFilter
from proxylib import dnslib_record2iplist
from proxylib import dnslib_resolve_over_tcp
from proxylib import dnslib_resolve_over_udp
from proxylib import FakeHttpsFilter
from proxylib import ForceHttpsFilter
from proxylib import CRLFSitesFilter
from proxylib import get_dnsserver_list
from proxylib import get_process_list
from proxylib import get_uptime
from proxylib import inflate
from proxylib import LocalProxyServer
from proxylib import message_html
from proxylib import MockFetchPlugin
from proxylib import AdvancedNet2
from proxylib import Net2
from proxylib import ProxyNet2
from proxylib import ProxyUtil
from proxylib import RC4Cipher
from proxylib import SimpleProxyHandler
from proxylib import spawn_later
from proxylib import StaticFileFilter
from proxylib import StripPlugin
from proxylib import StripPluginEx
from proxylib import URLRewriteFilter
from proxylib import UserAgentFilter
from proxylib import XORCipher
from proxylib import forward_socket


def is_google_ip(ipaddr):
    if ipaddr in ('74.125.127.102', '74.125.155.102', '74.125.39.102', '74.125.39.113', '209.85.229.138'):
        return False
    if ipaddr.startswith(('173.194.', '207.126.', '209.85.', '216.239.', '64.18.', '64.233.', '66.102.', '66.249.', '72.14.', '74.125.')):
        return True
    return False


class RangeFetch(object):
    """Range Fetch Class"""

    threads = 2
    maxsize = 1024*1024*4
    bufsize = 8192
    waitsize = 1024*512

    def __init__(self, handler, plugin, response, fetchservers, **kwargs):
        assert isinstance(plugin, BaseFetchPlugin) and hasattr(plugin, 'fetch')
        self.handler = handler
        self.url = handler.path
        self.plugin = plugin
        self.response = response
        self.fetchservers = fetchservers
        self.kwargs = kwargs
        self._stopped = None
        self._last_app_status = {}
        self.expect_begin = 0

    def fetch(self):
        response_status = self.response.status
        response_headers = dict((k.title(), v) for k, v in self.response.getheaders())
        content_range = response_headers['Content-Range']
        #content_length = response_headers['Content-Length']
        start, end, length = tuple(int(x) for x in re.search(r'bytes (\d+)-(\d+)/(\d+)', content_range).group(1, 2, 3))
        if start == 0:
            response_status = 200
            response_headers['Content-Length'] = str(length)
            del response_headers['Content-Range']
        else:
            response_headers['Content-Range'] = 'bytes %s-%s/%s' % (start, end, length)
            response_headers['Content-Length'] = str(length-start)

        logging.info('>>>>>>>>>>>>>>> RangeFetch started(%r) %d-%d', self.url, start, end)
        self.handler.send_response(response_status)
        for key, value in response_headers.items():
            self.handler.send_header(key, value)
        self.handler.end_headers()

        data_queue = Queue.PriorityQueue()
        range_queue = Queue.PriorityQueue()
        range_queue.put((start, end, self.response))
        self.expect_begin = start
        for begin in range(end+1, length, self.maxsize):
            range_queue.put((begin, min(begin+self.maxsize-1, length-1), None))
        for i in xrange(0, self.threads):
            range_delay_size = i * self.maxsize
            spawn_later(float(range_delay_size)/self.waitsize, self.__fetchlet, range_queue, data_queue, range_delay_size)
        has_peek = hasattr(data_queue, 'peek')
        peek_timeout = 120
        while self.expect_begin < length - 1:
            try:
                if has_peek:
                    begin, data = data_queue.peek(timeout=peek_timeout)
                    if self.expect_begin == begin:
                        data_queue.get()
                    elif self.expect_begin < begin:
                        time.sleep(0.1)
                        continue
                    else:
                        logging.error('RangeFetch Error: begin(%r) < expect_begin(%r), quit.', begin, self.expect_begin)
                        break
                else:
                    begin, data = data_queue.get(timeout=peek_timeout)
                    if self.expect_begin == begin:
                        pass
                    elif self.expect_begin < begin:
                        data_queue.put((begin, data))
                        time.sleep(0.1)
                        continue
                    else:
                        logging.error('RangeFetch Error: begin(%r) < expect_begin(%r), quit.', begin, self.expect_begin)
                        break
            except Queue.Empty:
                logging.error('data_queue peek timeout, break')
                break
            try:
                self.handler.wfile.write(data)
                self.expect_begin += len(data)
                del data
            except Exception as e:
                logging.info('RangeFetch client connection aborted(%s).', e)
                break
        self._stopped = True

    def __fetchlet(self, range_queue, data_queue, range_delay_size):
        headers = dict((k.title(), v) for k, v in self.handler.headers.items())
        headers['Connection'] = 'close'
        while 1:
            try:
                if self._stopped:
                    return
                try:
                    start, end, response = range_queue.get(timeout=1)
                    if self.expect_begin < start and data_queue.qsize() * self.bufsize + range_delay_size > 30*1024*1024:
                        range_queue.put((start, end, response))
                        time.sleep(10)
                        continue
                    headers['Range'] = 'bytes=%d-%d' % (start, end)
                    fetchserver = ''
                    if not response:
                        fetchserver = random.choice(self.fetchservers)
                        if self._last_app_status.get(fetchserver, 200) >= 500:
                            time.sleep(5)
                        response = self.plugin.fetch(self.handler, self.handler.command, self.url, headers, self.handler.body, timeout=self.handler.net2.connect_timeout, fetchserver=fetchserver, **self.kwargs)
                except Queue.Empty:
                    continue
                except Exception as e:
                    logging.warning("RangeFetch fetch response %r in __fetchlet", e)
                    range_queue.put((start, end, None))
                    continue
                if not response:
                    logging.warning('RangeFetch %s return %r', headers['Range'], response)
                    range_queue.put((start, end, None))
                    continue
                if fetchserver:
                    self._last_app_status[fetchserver] = response.app_status
                if response.app_status != 200:
                    logging.warning('Range Fetch "%s %s" %s return %s', self.handler.command, self.url, headers['Range'], response.app_status)
                    response.close()
                    range_queue.put((start, end, None))
                    continue
                if response.getheader('Location'):
                    self.url = urlparse.urljoin(self.url, response.getheader('Location'))
                    logging.info('RangeFetch Redirect(%r)', self.url)
                    response.close()
                    range_queue.put((start, end, None))
                    continue
                if 200 <= response.status < 300:
                    content_range = response.getheader('Content-Range')
                    if not content_range:
                        logging.warning('RangeFetch "%s %s" return Content-Range=%r: response headers=%r, retry %s-%s', self.handler.command, self.url, content_range, response.getheaders(), start, end)
                        response.close()
                        range_queue.put((start, end, None))
                        continue
                    content_length = int(response.getheader('Content-Length', 0))
                    logging.info('>>>>>>>>>>>>>>> [thread %s] %s %s', threading.currentThread().ident, content_length, content_range)
                    while 1:
                        try:
                            if self._stopped:
                                response.close()
                                return
                            data = None
                            with gevent.Timeout(max(1, self.bufsize//8192), False):
                                data = response.read(self.bufsize)
                            if not data:
                                break
                            data_queue.put((start, data))
                            start += len(data)
                        except Exception as e:
                            logging.warning('RangeFetch "%s %s" %s failed: %s', self.handler.command, self.url, headers['Range'], e)
                            break
                    if start < end + 1:
                        logging.warning('RangeFetch "%s %s" retry %s-%s', self.handler.command, self.url, start, end)
                        response.close()
                        range_queue.put((start, end, None))
                        continue
                    logging.info('>>>>>>>>>>>>>>> Successfully reached %d bytes.', start - 1)
                else:
                    logging.error('RangeFetch %r return %s', self.url, response.status)
                    response.close()
                    range_queue.put((start, end, None))
                    continue
            except StandardError as e:
                logging.exception('RangeFetch._fetchlet error:%s', e)
                raise


class GAEFetchPlugin(BaseFetchPlugin):
    """gae fetch plugin"""
    max_retry = 2

    def __init__(self, appids, password, path, mode, cachesock, keepalive, obfuscate, pagespeed, validate, options):
        BaseFetchPlugin.__init__(self)
        self.appids = appids
        self.password = password
        self.path = path
        self.mode = mode
        self.cachesock = cachesock
        self.keepalive = keepalive
        self.obfuscate = obfuscate
        self.pagespeed = pagespeed
        self.validate = validate
        self.options = options

    def handle(self, handler, **kwargs):
        assert handler.command != 'CONNECT'
        rescue_bytes = int(kwargs.pop('rescue_bytes', 0))
        method = handler.command
        headers = dict((k.title(), v) for k, v in handler.headers.items())
        body = handler.body
        if handler.path[0] == '/':
            url = '%s://%s%s' % (handler.scheme, handler.headers['Host'], handler.path)
        elif handler.path.lower().startswith(('http://', 'https://', 'ftp://')):
            url = handler.path
        else:
            raise ValueError('URLFETCH %r is not a valid url' % handler.path)
        errors = []
        response = None
        for i in xrange(self.max_retry):
            try:
                if rescue_bytes:
                    headers['Range'] = 'bytes=%d-' % rescue_bytes
                response = self.fetch(handler, method, url, headers, body, handler.net2.connect_timeout)
                if response.app_status < 500:
                    break
                else:
                    if response.app_status == 503:
                        # appid over qouta, switch to next appid
                        if len(self.appids) > 1:
                            self.appids.append(self.appids.pop(0))
                            logging.info('gae over qouta, switch next appid=%r', self.appids[0])
                    elif i < self.max_retry - 1 and len(self.appids) > 1:
                        self.appids.append(self.appids.pop(0))
                        logging.info('URLFETCH return %d, trying next appid=%r', response.app_status, self.appids[0])
                    response.close()
            except Exception as e:
                errors.append(e)
                logging.info('GAE "%s %s" appid=%r %r, retry...', handler.command, handler.path, self.appids[0], e)
        if len(errors) == self.max_retry:
            if response and response.app_status >= 500:
                status = response.app_status
                headers = dict(response.getheaders())
                content = response.read()
                response.close()
            else:
                status = 502
                headers = {'Content-Type': 'text/html'}
                content = message_html('502 URLFetch failed', 'Local URLFetch %r failed' % handler.path, '<br>'.join(str(x).decode(locale.getpreferredencoding()) for x in errors))
            return handler.handler_plugins['mock'].handle(handler, status, headers, content)
        logging.info('%s "GAE %s %s %s" %s %s', handler.address_string(), handler.command, handler.path, handler.protocol_version, response.status, response.getheader('Content-Length', '-'))
        try:
            if response.status == 206 and not rescue_bytes:
                fetchservers = ['%s://%s.appspot.com%s' % (self.mode, x, self.path) for x in self.appids]
                return RangeFetch(handler, self, response, fetchservers).fetch()
            handler.close_connection = not response.getheader('Content-Length')
            if not rescue_bytes:
                handler.send_response(response.status)
                for key, value in response.getheaders():
                    if key.title() == 'Transfer-Encoding':
                        continue
                    handler.send_header(key, value)
                handler.end_headers()
            bufsize = 8192
            written = rescue_bytes
            while True:
                data = None
                with gevent.Timeout(handler.net2.connect_timeout, False):
                    data = response.read(bufsize)
                if data is None:
                    logging.warning('GAE response.read(%r) %r timeout', bufsize, url)
                    if response.getheader('Accept-Ranges', '') == 'bytes' and not urlparse.urlparse(url).query:
                        return self.handle(handler, rescue_bytes=written)
                    handler.close_connection = True
                    break
                if data:
                    handler.wfile.write(data)
                    written += len(data)
                if not data:
                    cache_sock = getattr(response, 'cache_sock', None)
                    if cache_sock:
                        cache_sock.close()
                        del response.cache_sock
                    response.close()
                    break
                del data
        except NetWorkIOError as e:
            if e[0] in (errno.ECONNABORTED, errno.EPIPE) or 'bad write retry' in repr(e):
                return

    def fetch(self, handler, method, url, headers, body, timeout, **kwargs):
        if isinstance(body, basestring) and body:
            if len(body) < 10 * 1024 * 1024 and 'Content-Encoding' not in headers:
                zbody = deflate(body)
                if len(zbody) < len(body):
                    body = zbody
                    headers['Content-Encoding'] = 'deflate'
            headers['Content-Length'] = str(len(body))
        # GAE donot allow set `Host` header
        if 'Host' in headers:
            del headers['Host']
        kwargs = {}
        if self.password:
            kwargs['password'] = self.password
        if self.options:
            kwargs['options'] = self.options
        if self.validate:
            kwargs['validate'] = self.validate
        payload = '%s %s %s\r\n' % (method, url, handler.request_version)
        payload += ''.join('%s: %s\r\n' % (k, v) for k, v in headers.items() if k not in handler.net2.skip_headers)
        payload += ''.join('X-URLFETCH-%s: %s\r\n' % (k, v) for k, v in kwargs.items() if v)
        # prepare GAE request
        request_method = 'POST'
        fetchserver_index = random.randint(0, len(self.appids)-1) if 'Range' in headers else 0
        fetchserver = kwargs.get('fetchserver') or '%s://%s.appspot.com%s' % (self.mode, self.appids[fetchserver_index], self.path)
        request_headers = {}
        if common.GAE_OBFUSCATE:
            request_method = 'GET'
            fetchserver += 'ps/%d%s.gif' % (int(time.time()*1000), random.random())
            request_headers['X-URLFETCH-PS1'] = base64.b64encode(deflate(payload)).strip()
            if body:
                request_headers['X-URLFETCH-PS2'] = base64.b64encode(deflate(body)).strip()
                body = ''
            if common.GAE_PAGESPEED:
                fetchserver = re.sub(r'^(\w+://)', r'\g<1>1-ps.googleusercontent.com/h/', fetchserver)
        else:
            payload = deflate(payload)
            body = '%s%s%s' % (struct.pack('!h', len(payload)), payload, body)
            if 'rc4' in common.GAE_OPTIONS:
                request_headers['X-URLFETCH-Options'] = 'rc4'
                body = RC4Cipher(kwargs.get('password')).encrypt(body)
            request_headers['Content-Length'] = str(len(body))
        # post data
        need_crlf = 0 if common.GAE_MODE == 'https' else 1
        need_validate = common.GAE_VALIDATE
        cache_key = '%s:%d' % (handler.net2.host_postfix_map['.appspot.com'], 443 if common.GAE_MODE == 'https' else 80)
        headfirst = bool(common.GAE_HEADFIRST)
        response = handler.net2.create_http_request(request_method, fetchserver, request_headers, body, timeout, crlf=need_crlf, validate=need_validate, cache_key=cache_key, headfirst=headfirst)
        response.app_status = response.status
        if response.app_status != 200:
            return response
        if 'rc4' in request_headers.get('X-URLFETCH-Options', ''):
            response.fp = CipherFileObject(response.fp, RC4Cipher(kwargs['password']))
        data = response.read(2)
        if len(data) < 2:
            response.status = 502
            response.fp = io.BytesIO(b'connection aborted. too short leadbyte data=' + data)
            response.read = response.fp.read
            return response
        headers_length, = struct.unpack('!h', data)
        data = response.read(headers_length)
        if len(data) < headers_length:
            response.status = 502
            response.fp = io.BytesIO(b'connection aborted. too short headers data=' + data)
            response.read = response.fp.read
            return response
        raw_response_line, headers_data = inflate(data).split('\r\n', 1)
        _, response.status, response.reason = raw_response_line.split(None, 2)
        response.status = int(response.status)
        response.reason = response.reason.strip()
        response.msg = httplib.HTTPMessage(io.BytesIO(headers_data))
        return response


class PHPFetchPlugin(BaseFetchPlugin):
    """php fetch plugin"""

    def __init__(self, fetchserver, password, validate):
        BaseFetchPlugin.__init__(self)
        self.fetchservers = [fetchserver]
        self.password = password
        self.validate = validate

    def handle(self, handler, **kwargs):
        method = handler.command
        url = handler.path
        headers = dict((k.title(), v) for k, v in handler.headers.items())
        body = handler.body
        if body:
            if len(body) < 10 * 1024 * 1024 and 'Content-Encoding' not in headers:
                zbody = deflate(body)
                if len(zbody) < len(body):
                    body = zbody
                    headers['Content-Encoding'] = 'deflate'
            headers['Content-Length'] = str(len(body))
        skip_headers = handler.net2.skip_headers
        if self.password:
            kwargs['password'] = self.password
        if self.validate:
            kwargs['validate'] = self.validate
        payload = '%s %s %s\r\n' % (method, url, handler.request_version)
        payload += ''.join('%s: %s\r\n' % (k, v) for k, v in headers.items() if k not in handler.net2.skip_headers)
        payload += ''.join('X-URLFETCH-%s: %s\r\n' % (k, v) for k, v in kwargs.items() if v)
        payload = deflate(payload)
        body = '%s%s%s' % ((struct.pack('!h', len(payload)), payload, body))
        request_headers = {'Content-Length': len(body), 'Content-Type': 'application/octet-stream'}
        fetchserver_index = 0 if 'Range' not in headers else random.randint(0, len(self.fetchservers)-1)
        fetchserver = '%s?%s' % (self.fetchservers[fetchserver_index], random.random())
        crlf = 0
        cache_key = '%s//:%s' % urlparse.urlsplit(fetchserver)[:2]
        try:
            response = handler.net2.create_http_request('POST', fetchserver, request_headers, body, handler.net2.connect_timeout, crlf=crlf, cache_key=cache_key)
        except Exception as e:
            logging.warning('%s "%s" failed %r', method, url, e)
            return
        response.app_status = response.status
        need_decrypt = self.password and response.app_status == 200 and response.getheader('Content-Type', '') == 'image/gif' and response.fp
        if need_decrypt:
            response.fp = CipherFileObject(response.fp, XORCipher(self.password[0]))
        logging.info('%s "PHP %s %s %s" %s %s', handler.address_string(), handler.command, url, handler.protocol_version, response.status, response.getheader('Content-Length', '-'))
        handler.close_connection = bool(response.getheader('Transfer-Encoding'))
        while True:
            data = response.read(8192)
            if not data:
                break
            handler.wfile.write(data)
            del data


class VPSServer(gevent.server.StreamServer):
    """vps server"""
    net2 = Net2()

    def __init__(self, *args, **kwargs):
        self.fetchservers = kwargs.pop('fetchservers')
        gevent.server.StreamServer.__init__(self, *args, **kwargs)
        self.remote_cache = {}

    def forward_socket(self, local, remote, timeout, bufsize):
        """forward socket"""
        tick = 1
        count = timeout
        while 1:
            count -= tick
            if count <= 0:
                break
            ins, _, errors = select.select([local, remote], [], [local, remote], tick)
            if remote in errors:
                local.close()
                remote.close()
                return
            if local in errors:
                local.close()
                remote.close()
                return
            if remote in ins:
                data = remote.recv(bufsize)
                if not data:
                    remote.close()
                    local.close()
                    return
                local.sendall(data)
            if local in ins:
                data = local.recv(bufsize)
                if not data:
                    remote.close()
                    local.close()
                    return
                remote.sendall(data)
            if ins:
                count = timeout

    def handle(self, sock, addr):
        request_data = data = ''
        while True:
            data = sock.recv(8192)
            request_data += data
            if '\r\n' in data:
                break
            if data == '':
                return
        request_line, _, header_data = request_data.partition('\r\n')
        logging.info('%s:%d "VPS %s" - -', addr[0], addr[1], request_line)
        fetchserver = self.fetchservers[0]
        scheme, username, password, netloc = ProxyUtil.parse_proxy(fetchserver)
        if scheme != 'https':
            raise ValueError('VPSServer current only support https protocol')
        if netloc.rfind(':') <= netloc.rfind(']'):
            # no port number
            host = netloc
            port = 443 if scheme == 'https' else 80
        else:
            host, _, port = netloc.rpartition(':')
            port = int(port)
        remote = self.net2.create_ssl_connection(host, port, 8, cache_key=netloc)
        request_data = '%s\r\nProxy-Authorization: Baisic %s\r\n%s' % (request_line, base64.b64encode('%s:%s' % (username, password)).strip(), header_data)
        remote.sendall(request_data)
        try:
            self.forward_socket(sock, remote, 60, bufsize=256*1024)
        except (socket.error, ssl.SSLError, OpenSSL.SSL.Error) as e:
            if e.args[0] not in (errno.ECONNABORTED, errno.ECONNRESET, errno.ENOTCONN, errno.EPIPE):
                raise
            if e.args[0] in (errno.EBADF,):
                return


class GAEFetchFilter(BaseProxyHandlerFilter):
    """gae fetch filter"""
    #https://github.com/AppScale/gae_sdk/blob/master/google/appengine/api/taskqueue/taskqueue.py#L241
    MAX_URL_LENGTH = 2083
    def filter(self, handler):
        """https://developers.google.com/appengine/docs/python/urlfetch/"""
        if handler.command == 'CONNECT':
            do_ssl_handshake = 440 <= handler.port <= 450 or 1024 <= handler.port <= 65535
            alias = handler.net2.getaliasbyname(handler.path)
            if alias:
                return 'direct', {'cache_key': '%s:%d' % (alias, handler.port), 'headfirst': '.google' in handler.host}
            else:
                return 'strip', {'do_ssl_handshake': do_ssl_handshake}
        elif handler.command in ('GET', 'POST', 'HEAD', 'PUT', 'DELETE', 'PATCH'):
            alias = handler.net2.getaliasbyname(handler.path)
            if alias:
                return 'direct', {'cache_key': '%s:%d' % (alias, handler.port), 'headfirst': '.google' in handler.host}
            else:
                return 'gae', {}
        else:
            if 'php' in handler.handler_plugins:
                return 'php', {}
            else:
                logging.warning('"%s %s" not supported by GAE, please enable PHP mode!', handler.command, handler.path)
                return 'direct', {}


class WithGAEFilter(BaseProxyHandlerFilter):
    """withgae/withphp filter"""
    def __init__(self, withgae_sites, withphp_sites):
        self.withgae_sites = set(x for x in withgae_sites if not x.startswith('.'))
        self.withgae_sites_postfix = tuple(x for x in withgae_sites if x.startswith('.'))
        self.withphp_sites = set(x for x in withphp_sites if not x.startswith('.'))
        self.withphp_sites_postfix = tuple(x for x in withphp_sites if x.startswith('.'))

    def filter(self, handler):
        plugin = ''
        if handler.host in self.withgae_sites or handler.host.endswith(self.withgae_sites_postfix):
            plugin = 'gae'
        elif handler.host in self.withphp_sites or handler.host.endswith(self.withphp_sites_postfix):
            if 'php' not in handler.handler_plugins:
                logging.warning('handler=%s does not contains php plugin, fallback to gae plugin!', handler)
                plugin = 'gae'
            else:
                plugin = 'php'
        if plugin:
            if handler.command == 'CONNECT':
                do_ssl_handshake = 440 <= handler.port <= 450 or 1024 <= handler.port <= 65535
                return 'strip', {'do_ssl_handshake': do_ssl_handshake}
            else:
                return plugin, {}


class GAEProxyHandler(SimpleProxyHandler):
    """GAE Proxy Handler"""
    handler_filters = [GAEFetchFilter()]
    handler_plugins = {'direct': DirectFetchPlugin(),
                       'mock': MockFetchPlugin(),
                       'strip': StripPlugin(),}

    def __init__(self, *args, **kwargs):
        SimpleProxyHandler.__init__(self, *args, **kwargs)

    def first_run(self):
        """GAEProxyHandler setup, init domain/iplist map"""
        if not common.PROXY_ENABLE:
            logging.info('resolve common.IPLIST_ALIAS names=%s to iplist', list(common.IPLIST_ALIAS))
            common.resolve_iplist()
        random.shuffle(common.GAE_APPIDS)
        self.__class__.handler_plugins['gae'] = GAEFetchPlugin(common.GAE_APPIDS, common.GAE_PASSWORD, common.GAE_PATH, common.GAE_MODE, common.GAE_CACHESOCK, common.GAE_KEEPALIVE, common.GAE_OBFUSCATE, common.GAE_PAGESPEED, common.GAE_VALIDATE, common.GAE_OPTIONS)
        if not common.PROXY_ENABLE:
            net2 = AdvancedNet2(window=common.GAE_WINDOW, ssl_version=common.GAE_SSLVERSION, dns_servers=common.DNS_SERVERS, dns_blacklist=common.DNS_BLACKLIST)
            for name, iplist in common.IPLIST_ALIAS.items():
                net2.add_iplist_alias(name, iplist)
                if name == 'google_hk':
                    for delay in (30, 60, 150, 240, 300, 450, 600, 900):
                        spawn_later(delay, self.extend_iplist, name)
            net2.add_fixed_iplist(common.IPLIST_PREDEFINED)
            for pattern, hosts in common.RULE_MAP.items():
                net2.add_rule(pattern, hosts)
            if common.GAE_CACHESOCK:
                net2.enable_connection_cache()
            if common.GAE_KEEPALIVE:
                net2.enable_connection_keepalive()
            net2.enable_openssl_session_cache()
            self.__class__.net2 = net2

    def extend_iplist(self, iplist_name):
        hosts = [x for x in common.CONFIG.get('iplist', iplist_name).split('|') if not re.match(r'^\d+\.\d+\.\d+\.\d+$', x) and ':' not in x]
        logging.info('extend_iplist start for hosts=%s', hosts)
        new_iplist = []
        def do_remote_resolve(host, dnsserver, queue):
            assert isinstance(dnsserver, basestring)
            for dnslib_resolve in (dnslib_resolve_over_udp, dnslib_resolve_over_tcp):
                try:
                    time.sleep(random.random())
                    iplist = dnslib_record2iplist(dnslib_resolve(host, [dnsserver], timeout=4, blacklist=common.DNS_BLACKLIST))
                    queue.put((host, dnsserver, iplist))
                except (socket.error, OSError) as e:
                    logging.info('%s remote host=%r failed: %s', str(dnslib_resolve).split()[1], host, e)
                    time.sleep(1)
        result_queue = Queue.Queue()
        pool = __import__('gevent.pool', fromlist=['.']).Pool(8) if sys.modules.get('gevent') else None
        for host in hosts:
            for dnsserver in common.DNS_SERVERS:
                logging.debug('remote resolve host=%r from dnsserver=%r', host, dnsserver)
                if pool:
                    pool.spawn(do_remote_resolve, host, dnsserver, result_queue)
                else:
                    thread.start_new_thread(do_remote_resolve, (host, dnsserver, result_queue))
        for _ in xrange(len(common.DNS_SERVERS) * len(hosts) * 2):
            try:
                host, dnsserver, iplist = result_queue.get(timeout=16)
                logging.debug('%r remote host=%r return %s', dnsserver, host, iplist)
                if '.google' in host:
                    if common.GAE_IPV6:
                        iplist = [x for x in iplist if ':' in x]
                    else:
                        iplist = [x for x in iplist if is_google_ip(x)]
                new_iplist += iplist
            except Queue.Empty:
                break
        logging.info('extend_iplist finished, added %s', len(set(self.net2.iplist_alias[iplist_name])-set(new_iplist)))
        self.net2.add_iplist_alias(iplist_name, new_iplist)


class PHPFetchFilter(BaseProxyHandlerFilter):
    """php fetch filter"""
    def filter(self, handler):
        if handler.net2.getaliasbyname(handler.path):
            return 'direct', {}
        if handler.command == 'CONNECT':
            return 'strip', {}
        else:
            return 'php', {}


class PHPProxyHandler(SimpleProxyHandler):
    """PHP Proxy Handler"""
    handler_filters = [PHPFetchFilter()]
    handler_plugins = {'direct': DirectFetchPlugin(),
                       'mock': MockFetchPlugin(),
                       'strip': StripPlugin(),}

    def __init__(self, *args, **kwargs):
        SimpleProxyHandler.__init__(self, *args, **kwargs)

    def first_run(self):
        """PHPProxyHandler setup, init domain/iplist map"""
        if not common.PROXY_ENABLE:
            hostname = urlparse.urlsplit(common.PHP_FETCHSERVER).hostname
            net2 = AdvancedNet2(window=4, ssl_version='TLSv1', dns_servers=common.DNS_SERVERS, dns_blacklist=common.DNS_BLACKLIST)
            if not common.PHP_HOSTS:
                common.PHP_HOSTS = net2.gethostsbyname(hostname)
            net2.add_iplist_alias('php_fetchserver', common.PHP_HOSTS)
            net2.add_fixed_iplist(common.PHP_HOSTS)
            net2.add_rule(hostname, 'php_fetchserver')
            net2.enable_connection_cache()
            if common.PHP_KEEPALIVE:
                net2.enable_connection_keepalive()
            net2.enable_openssl_session_cache()
            self.__class__.net2 = net2


class PacUtil(object):
    """GoAgent Pac Util"""

    @staticmethod
    def urlread(url, proxy_address):
        try:
            conn = httplib.HTTPConnection(proxy_address)
            conn.request('GET', url)
            response = conn.getresponse()
            return response.read()
        finally:
            conn.close()

    @staticmethod
    def update_pacfile(filename):
        listen_ip = '127.0.0.1'
        autoproxy = '%s:%s' % (listen_ip, common.LISTEN_PORT)
        blackhole = '%s:%s' % (listen_ip, common.PAC_PORT)
        default = 'PROXY %s:%s' % (common.PROXY_HOST, common.PROXY_PORT) if common.PROXY_ENABLE else 'DIRECT'
        content = ''
        need_update = True
        with open(filename, 'rb') as fp:
            content = fp.read()
        try:
            placeholder = '// AUTO-GENERATED RULES, DO NOT MODIFY!'
            content = content[:content.index(placeholder)+len(placeholder)]
            content = re.sub(r'''blackhole\s*=\s*['"]PROXY [\.\w:]+['"]''', 'blackhole = \'PROXY %s\'' % blackhole, content)
            content = re.sub(r'''autoproxy\s*=\s*['"]PROXY [\.\w:]+['"]''', 'autoproxy = \'PROXY %s\'' % autoproxy, content)
            content = re.sub(r'''defaultproxy\s*=\s*['"](DIRECT|PROXY [\.\w:]+)['"]''', 'defaultproxy = \'%s\'' % default, content)
            content = re.sub(r'''host\s*==\s*['"][\.\w:]+['"]\s*\|\|\s*isPlainHostName''', 'host == \'%s\' || isPlainHostName' % listen_ip, content)
            if content.startswith('//'):
                line = '// Proxy Auto-Config file generated by autoproxy2pac, %s\r\n' % time.strftime('%Y-%m-%d %H:%M:%S')
                content = line + '\r\n'.join(content.splitlines()[1:])
        except ValueError:
            need_update = False
        try:
            if common.PAC_ADBLOCK:
                admode = common.PAC_ADMODE
                logging.info('try download %r to update_pacfile(%r)', common.PAC_ADBLOCK, filename)
                adblock_content = PacUtil.urlread(common.PAC_ADBLOCK, autoproxy)
                logging.info('%r downloaded, try convert it with adblock2pac', common.PAC_ADBLOCK)
                if 'gevent' in sys.modules and time.sleep is getattr(sys.modules['gevent'], 'sleep', None) and hasattr(gevent.get_hub(), 'threadpool'):
                    jsrule = gevent.get_hub().threadpool.apply_e(Exception, PacUtil.adblock2pac, (adblock_content, 'FindProxyForURLByAdblock', blackhole, default, admode))
                else:
                    jsrule = PacUtil.adblock2pac(adblock_content, 'FindProxyForURLByAdblock', blackhole, default, admode)
                content += '\r\n' + jsrule + '\r\n'
                logging.info('%r downloaded and parsed', common.PAC_ADBLOCK)
            else:
                content += '\r\nfunction FindProxyForURLByAdblock(url, host) {return "DIRECT";}\r\n'
        except StandardError as e:
            need_update = False
            logging.exception('update_pacfile failed: %r', e)
        try:
            autoproxy_content_list = []
            for url in common.PAC_GFWLIST.split('|'):
                logging.info('try download %r to update_pacfile(%r)', url, filename)
                if url.startswith('file://'):
                    try:
                        with open(url[len('file://'):], 'rb') as fp:
                            autoproxy_content_list.append(fp.read())
                    except IOError as e:
                        logging.warning('PacUtil load %r failed: %r', url, e)
                else:
                    url_content = PacUtil.urlread(url, autoproxy)
                    if not any(x in url_content for x in '!-@|'):
                        url_content = base64.b64decode(url_content)
                    autoproxy_content_list.append(url_content)
            autoproxy_content = '\n'.join(autoproxy_content_list)
            logging.info('%r downloaded, try convert it with autoproxy2pac_lite', common.PAC_GFWLIST)
            if 'gevent' in sys.modules and time.sleep is getattr(sys.modules['gevent'], 'sleep', None) and hasattr(gevent.get_hub(), 'threadpool'):
                jsrule = gevent.get_hub().threadpool.apply_e(Exception, PacUtil.autoproxy2pac_lite, (autoproxy_content, 'FindProxyForURLByAutoProxy', autoproxy, default))
            else:
                jsrule = PacUtil.autoproxy2pac_lite(autoproxy_content, 'FindProxyForURLByAutoProxy', autoproxy, default)
            content += '\r\n' + jsrule + '\r\n'
            logging.info('%r downloaded and parsed', common.PAC_GFWLIST)
        except StandardError as e:
            need_update = False
            logging.exception('update_pacfile failed: %r', e)
        if need_update:
            with open(filename, 'wb') as fp:
                fp.write(content)
            logging.info('%r successfully updated', filename)

    @staticmethod
    def autoproxy2pac(content, func_name='FindProxyForURLByAutoProxy', proxy='127.0.0.1:8087', default='DIRECT', indent=4):
        """Autoproxy to Pac, based on https://github.com/iamamac/autoproxy2pac"""
        jsLines = []
        for line in content.splitlines()[1:]:
            if line and not line.startswith("!"):
                use_proxy = True
                if line.startswith("@@"):
                    line = line[2:]
                    use_proxy = False
                return_proxy = 'PROXY %s' % proxy if use_proxy else default
                if line.startswith('/') and line.endswith('/'):
                    jsLine = 'if (/%s/i.test(url)) return "%s";' % (line[1:-1], return_proxy)
                elif line.startswith('||'):
                    domain = line[2:].lstrip('.')
                    if len(jsLines) > 0 and ('host.indexOf(".%s") >= 0' % domain in jsLines[-1] or 'host.indexOf("%s") >= 0' % domain in jsLines[-1]):
                        jsLines.pop()
                    jsLine = 'if (dnsDomainIs(host, ".%s") || host == "%s") return "%s";' % (domain, domain, return_proxy)
                elif line.startswith('|'):
                    jsLine = 'if (url.indexOf("%s") == 0) return "%s";' % (line[1:], return_proxy)
                elif '*' in line:
                    jsLine = 'if (shExpMatch(url, "*%s*")) return "%s";' % (line.strip('*'), return_proxy)
                elif '/' not in line:
                    jsLine = 'if (host.indexOf("%s") >= 0) return "%s";' % (line, return_proxy)
                else:
                    jsLine = 'if (url.indexOf("%s") >= 0) return "%s";' % (line, return_proxy)
                jsLine = ' ' * indent + jsLine
                if use_proxy:
                    jsLines.append(jsLine)
                else:
                    jsLines.insert(0, jsLine)
        function = 'function %s(url, host) {\r\n%s\r\n%sreturn "%s";\r\n}' % (func_name, '\n'.join(jsLines), ' '*indent, default)
        return function

    @staticmethod
    def autoproxy2pac_lite(content, func_name='FindProxyForURLByAutoProxy', proxy='127.0.0.1:8087', default='DIRECT', indent=4):
        """Autoproxy to Pac, based on https://github.com/iamamac/autoproxy2pac"""
        direct_domain_set = set([])
        proxy_domain_set = set([])
        for line in content.splitlines()[1:]:
            if line and not line.startswith(('!', '|!', '||!')):
                use_proxy = True
                if line.startswith("@@"):
                    line = line[2:]
                    use_proxy = False
                domain = ''
                if line.startswith('/') and line.endswith('/'):
                    line = line[1:-1]
                    if line.startswith('^https?:\\/\\/[^\\/]+') and re.match(r'^(\w|\\\-|\\\.)+$', line[18:]):
                        domain = line[18:].replace(r'\.', '.')
                    else:
                        logging.warning('unsupport gfwlist regex: %r', line)
                elif line.startswith('||'):
                    domain = line[2:].lstrip('*').rstrip('/')
                elif line.startswith('|'):
                    domain = urlparse.urlsplit(line[1:]).hostname.lstrip('*')
                elif line.startswith(('http://', 'https://')):
                    domain = urlparse.urlsplit(line).hostname.lstrip('*')
                elif re.search(r'^([\w\-\_\.]+)([\*\/]|$)', line):
                    domain = re.split(r'[\*\/]', line)[0]
                else:
                    pass
                if '*' in domain:
                    domain = domain.split('*')[-1]
                if not domain or re.match(r'^\w+$', domain):
                    logging.debug('unsupport gfwlist rule: %r', line)
                    continue
                if use_proxy:
                    proxy_domain_set.add(domain)
                else:
                    direct_domain_set.add(domain)
        proxy_domain_list = sorted(set(x.lstrip('.') for x in proxy_domain_set))
        autoproxy_host = ',\r\n'.join('%s"%s": 1' % (' '*indent, x) for x in proxy_domain_list)
        template = '''\
                    var autoproxy_host = {
                    %(autoproxy_host)s
                    };
                    function %(func_name)s(url, host) {
                        var lastPos;
                        do {
                            if (autoproxy_host.hasOwnProperty(host)) {
                                return 'PROXY %(proxy)s';
                            }
                            lastPos = host.indexOf('.') + 1;
                            host = host.slice(lastPos);
                        } while (lastPos >= 1);
                        return '%(default)s';
                    }'''
        template = re.sub(r'(?m)^\s{%d}' % min(len(re.search(r' +', x).group()) for x in template.splitlines()), '', template)
        template_args = {'autoproxy_host': autoproxy_host,
                         'func_name': func_name,
                         'proxy': proxy,
                         'default': default}
        return template % template_args

    @staticmethod
    def urlfilter2pac(content, func_name='FindProxyForURLByUrlfilter', proxy='127.0.0.1:8086', default='DIRECT', indent=4):
        """urlfilter.ini to Pac, based on https://github.com/iamamac/autoproxy2pac"""
        jsLines = []
        for line in content[content.index('[exclude]'):].splitlines()[1:]:
            if line and not line.startswith(';'):
                use_proxy = True
                if line.startswith("@@"):
                    line = line[2:]
                    use_proxy = False
                return_proxy = 'PROXY %s' % proxy if use_proxy else default
                if '*' in line:
                    jsLine = 'if (shExpMatch(url, "%s")) return "%s";' % (line, return_proxy)
                else:
                    jsLine = 'if (url == "%s") return "%s";' % (line, return_proxy)
                jsLine = ' ' * indent + jsLine
                if use_proxy:
                    jsLines.append(jsLine)
                else:
                    jsLines.insert(0, jsLine)
        function = 'function %s(url, host) {\r\n%s\r\n%sreturn "%s";\r\n}' % (func_name, '\n'.join(jsLines), ' '*indent, default)
        return function

    @staticmethod
    def adblock2pac(content, func_name='FindProxyForURLByAdblock', proxy='127.0.0.1:8086', default='DIRECT', admode=1, indent=4):
        """adblock list to Pac, based on https://github.com/iamamac/autoproxy2pac"""
        white_conditions = {'host': [], 'url.indexOf': [], 'shExpMatch': []}
        black_conditions = {'host': [], 'url.indexOf': [], 'shExpMatch': []}
        for line in content.splitlines()[1:]:
            if not line or line.startswith('!') or '##' in line or '#@#' in line:
                continue
            use_proxy = True
            use_start = False
            use_end = False
            use_domain = False
            use_postfix = []
            if '$' in line:
                posfixs = line.split('$')[-1].split(',')
                if any('domain' in x for x in posfixs):
                    continue
                if 'image' in posfixs:
                    use_postfix += ['.jpg', '.gif']
                elif 'script' in posfixs:
                    use_postfix += ['.js']
                else:
                    continue
            line = line.split('$')[0]
            if line.startswith("@@"):
                line = line[2:]
                use_proxy = False
            if '||' == line[:2]:
                line = line[2:]
                if '/' not in line:
                    use_domain = True
                else:
                    use_start = True
            elif '|' == line[0]:
                line = line[1:]
                use_start = True
            if line[-1] in ('^', '|'):
                line = line[:-1]
                if not use_postfix:
                    use_end = True
            line = line.replace('^', '*').strip('*')
            conditions = black_conditions if use_proxy else white_conditions
            if use_start and use_end:
                conditions['shExpMatch'] += ['*%s*' % line]
            elif use_start:
                if '*' in line:
                    if use_postfix:
                        conditions['shExpMatch'] += ['*%s*%s' % (line, x) for x in use_postfix]
                    else:
                        conditions['shExpMatch'] += ['*%s*' % line]
                else:
                    conditions['url.indexOf'] += [line]
            elif use_domain and use_end:
                if '*' in line:
                    conditions['shExpMatch'] += ['%s*' % line]
                else:
                    conditions['host'] += [line]
            elif use_domain:
                if line.split('/')[0].count('.') <= 1:
                    if use_postfix:
                        conditions['shExpMatch'] += ['*.%s*%s' % (line, x) for x in use_postfix]
                    else:
                        conditions['shExpMatch'] += ['*.%s*' % line]
                else:
                    if '*' in line:
                        if use_postfix:
                            conditions['shExpMatch'] += ['*%s*%s' % (line, x) for x in use_postfix]
                        else:
                            conditions['shExpMatch'] += ['*%s*' % line]
                    else:
                        if use_postfix:
                            conditions['shExpMatch'] += ['*%s*%s' % (line, x) for x in use_postfix]
                        else:
                            conditions['url.indexOf'] += ['http://%s' % line]
            else:
                if use_postfix:
                    conditions['shExpMatch'] += ['*%s*%s' % (line, x) for x in use_postfix]
                else:
                    conditions['shExpMatch'] += ['*%s*' % line]
        templates = ['''\
                    function %(func_name)s(url, host) {
                        return '%(default)s';
                    }''',
                    '''\
                    var blackhole_host = {
                    %(blackhole_host)s
                    };
                    function %(func_name)s(url, host) {
                        // untrusted ablock plus list, disable whitelist until chinalist come back.
                        if (blackhole_host.hasOwnProperty(host)) {
                            return 'PROXY %(proxy)s';
                        }
                        return '%(default)s';
                    }''',
                    '''\
                    var blackhole_host = {
                    %(blackhole_host)s
                    };
                    var blackhole_url_indexOf = [
                    %(blackhole_url_indexOf)s
                    ];
                    function %s(url, host) {
                        // untrusted ablock plus list, disable whitelist until chinalist come back.
                        if (blackhole_host.hasOwnProperty(host)) {
                            return 'PROXY %(proxy)s';
                        }
                        for (i = 0; i < blackhole_url_indexOf.length; i++) {
                            if (url.indexOf(blackhole_url_indexOf[i]) >= 0) {
                                return 'PROXY %(proxy)s';
                            }
                        }
                        return '%(default)s';
                    }''',
                    '''\
                    var blackhole_host = {
                    %(blackhole_host)s
                    };
                    var blackhole_url_indexOf = [
                    %(blackhole_url_indexOf)s
                    ];
                    var blackhole_shExpMatch = [
                    %(blackhole_shExpMatch)s
                    ];
                    function %(func_name)s(url, host) {
                        // untrusted ablock plus list, disable whitelist until chinalist come back.
                        if (blackhole_host.hasOwnProperty(host)) {
                            return 'PROXY %(proxy)s';
                        }
                        for (i = 0; i < blackhole_url_indexOf.length; i++) {
                            if (url.indexOf(blackhole_url_indexOf[i]) >= 0) {
                                return 'PROXY %(proxy)s';
                            }
                        }
                        for (i = 0; i < blackhole_shExpMatch.length; i++) {
                            if (shExpMatch(url, blackhole_shExpMatch[i])) {
                                return 'PROXY %(proxy)s';
                            }
                        }
                        return '%(default)s';
                    }''']
        template = re.sub(r'(?m)^\s{%d}' % min(len(re.search(r' +', x).group()) for x in templates[admode].splitlines()), '', templates[admode])
        template_kwargs = {'blackhole_host': ',\r\n'.join("%s'%s': 1" % (' '*indent, x) for x in sorted(black_conditions['host'])),
                           'blackhole_url_indexOf': ',\r\n'.join("%s'%s'" % (' '*indent, x) for x in sorted(black_conditions['url.indexOf'])),
                           'blackhole_shExpMatch': ',\r\n'.join("%s'%s'" % (' '*indent, x) for x in sorted(black_conditions['shExpMatch'])),
                           'func_name': func_name,
                           'proxy': proxy,
                           'default': default}
        return template % template_kwargs


class PacFileFilter(BaseProxyHandlerFilter):
    """pac file filter"""

    def filter(self, handler):
        is_local_client = handler.client_address[0] in ('127.0.0.1', '::1')
        pacfile = os.path.join(os.path.dirname(os.path.abspath(__file__)), common.PAC_FILE)
        urlparts = urlparse.urlsplit(handler.path)
        if handler.command == 'GET' and urlparts.path.lstrip('/') == common.PAC_FILE:
            if urlparts.query == 'flush':
                if is_local_client:
                    thread.start_new_thread(PacUtil.update_pacfile, (pacfile,))
                else:
                    return 'mock', {'status': 403, 'headers': {'Content-Type': 'text/plain'}, 'body': 'client address %r not allowed' % handler.client_address[0]}
            if time.time() - os.path.getmtime(pacfile) > common.PAC_EXPIRED:
                # check system uptime > 30 minutes
                uptime = get_uptime()
                if uptime and uptime > 1800:
                    thread.start_new_thread(lambda: os.utime(pacfile, (time.time(), time.time())) or PacUtil.update_pacfile(pacfile), tuple())
            with open(pacfile, 'rb') as fp:
                content = fp.read()
                if not is_local_client:
                    serving_addr = urlparts.hostname or ProxyUtil.get_listen_ip()
                    content = content.replace('127.0.0.1', serving_addr)
                headers = {'Content-Type': 'text/plain'}
                if 'gzip' in handler.headers.get('Accept-Encoding', ''):
                    headers['Content-Encoding'] = 'gzip'
                    compressobj = zlib.compressobj(zlib.Z_DEFAULT_COMPRESSION, zlib.DEFLATED, -zlib.MAX_WBITS, zlib.DEF_MEM_LEVEL, 0)
                    dataio = io.BytesIO()
                    dataio.write('\x1f\x8b\x08\x00\x00\x00\x00\x00\x02\xff')
                    dataio.write(compressobj.compress(content))
                    dataio.write(compressobj.flush())
                    dataio.write(struct.pack('<LL', zlib.crc32(content) & 0xFFFFFFFFL, len(content) & 0xFFFFFFFFL))
                    content = dataio.getvalue()
                return 'mock', {'status': 200, 'headers': headers, 'body': content}


class PACProxyHandler(SimpleProxyHandler):
    """pac proxy handler"""
    handler_filters = [PacFileFilter(), StaticFileFilter(), BlackholeFilter()]


class Common(object):
    """Global Config Object"""

    ENV_CONFIG_PREFIX = 'GOAGENT_'

    def __init__(self):
        """load config from proxy.ini"""
        ConfigParser.RawConfigParser.OPTCRE = re.compile(r'(?P<option>\S+)\s+(?P<vi>[=])\s+(?P<value>.*)$')
        self.CONFIG = ConfigParser.ConfigParser()
        self.CONFIG_FILENAME = os.path.splitext(os.path.abspath(__file__))[0]+'.ini'
        self.CONFIG_USER_FILENAME = re.sub(r'\.ini$', '.user.ini', self.CONFIG_FILENAME)
        self.CONFIG.read([self.CONFIG_FILENAME, self.CONFIG_USER_FILENAME])

        for key, value in os.environ.items():
            m = re.match(r'^%s([A-Z]+)_([A-Z\_\-]+)$' % self.ENV_CONFIG_PREFIX, key)
            if m:
                self.CONFIG.set(m.group(1).lower(), m.group(2).lower(), value)

        self.LISTEN_IP = self.CONFIG.get('listen', 'ip')
        self.LISTEN_PORT = self.CONFIG.getint('listen', 'port')
        self.LISTEN_USERNAME = self.CONFIG.get('listen', 'username') if self.CONFIG.has_option('listen', 'username') else ''
        self.LISTEN_PASSWORD = self.CONFIG.get('listen', 'password') if self.CONFIG.has_option('listen', 'password') else ''
        self.LISTEN_VISIBLE = self.CONFIG.getint('listen', 'visible')
        self.LISTEN_DEBUGINFO = self.CONFIG.getint('listen', 'debuginfo')

        self.GAE_ENABLE = self.CONFIG.getint('gae', 'enable')
        self.GAE_APPIDS = re.findall(r'[\w\-\.]+', self.CONFIG.get('gae', 'appid').replace('.appspot.com', ''))
        self.GAE_PASSWORD = self.CONFIG.get('gae', 'password').strip()
        self.GAE_PATH = self.CONFIG.get('gae', 'path')
        self.GAE_MODE = self.CONFIG.get('gae', 'mode')
        self.GAE_IPV6 = self.CONFIG.getint('gae', 'ipv6')
        self.GAE_WINDOW = self.CONFIG.getint('gae', 'window')
        self.GAE_KEEPALIVE = self.CONFIG.getint('gae', 'keepalive')
        self.GAE_CACHESOCK = self.CONFIG.getint('gae', 'cachesock')
        self.GAE_HEADFIRST = self.CONFIG.getint('gae', 'headfirst')
        self.GAE_OBFUSCATE = self.CONFIG.getint('gae', 'obfuscate')
        self.GAE_VALIDATE = self.CONFIG.getint('gae', 'validate')
        self.GAE_TRANSPORT = self.CONFIG.getint('gae', 'transport') if self.CONFIG.has_option('gae', 'transport') else 0
        self.GAE_OPTIONS = self.CONFIG.get('gae', 'options')
        self.GAE_REGIONS = set(x.upper() for x in self.CONFIG.get('gae', 'regions').split('|') if x.strip())
        self.GAE_SSLVERSION = self.CONFIG.get('gae', 'sslversion')
        self.GAE_PAGESPEED = self.CONFIG.getint('gae', 'pagespeed') if self.CONFIG.has_option('gae', 'pagespeed') else 0

        if self.GAE_IPV6:
            sock = None
            try:
                sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
                sock.connect(('2001:4860:4860::8888', 53))
                logging.info('use ipv6 interface %s for gae', sock.getsockname()[0])
            except Exception as e:
                logging.info('Fail try use ipv6 %r, fallback ipv4', e)
                self.GAE_IPV6 = 0
            finally:
                if sock:
                    sock.close()

        if 'USERDNSDOMAIN' in os.environ and re.match(r'^\w+\.\w+$', os.environ['USERDNSDOMAIN']):
            self.CONFIG.set('profile', '.' + os.environ['USERDNSDOMAIN'], 'direct')

        withgae_sites = []
        withphp_sites = []
        crlf_sites = []
        nocrlf_sites = []
        forcehttps_sites = []
        noforcehttps_sites = []
        fakehttps_sites = []
        nofakehttps_sites = []
        dns_servers = []
        urlrewrite_map = collections.OrderedDict()
        rule_map = collections.OrderedDict()

        for pattern, rule in self.CONFIG.items('profile'):
            rules = [x.strip() for x in re.split(r'[,\|]', rule) if x.strip()]
            if rule.startswith(('file://', 'http://', 'https://')) or '$1' in rule:
                urlrewrite_map[pattern] = rule
                continue
            for rule, sites in [('withgae', withgae_sites),
                                ('withphp', withphp_sites),
                                ('crlf', crlf_sites),
                                ('nocrlf', nocrlf_sites),
                                ('forcehttps', forcehttps_sites),
                                ('noforcehttps', noforcehttps_sites),
                                ('fakehttps', fakehttps_sites),
                                ('nofakehttps', nofakehttps_sites)]:
                if rule in rules:
                    sites.append(pattern)
                    rules.remove(rule)
            if rules:
                rule_map[pattern] = rules[0]

        self.HTTP_DNS = dns_servers
        self.WITHGAE_SITES = tuple(withgae_sites)
        self.WITHPHP_SITES = tuple(withphp_sites)
        self.CRLF_SITES = tuple(crlf_sites)
        self.NOCRLF_SITES = set(nocrlf_sites)
        self.FORCEHTTPS_SITES = tuple(forcehttps_sites)
        self.NOFORCEHTTPS_SITES = set(noforcehttps_sites)
        self.FAKEHTTPS_SITES = tuple(fakehttps_sites)
        self.NOFAKEHTTPS_SITES = set(nofakehttps_sites)
        self.URLREWRITE_MAP = urlrewrite_map
        self.RULE_MAP = rule_map

        self.IPLIST_ALIAS = collections.OrderedDict((k, v.split('|') if v else []) for k, v in self.CONFIG.items('iplist'))
        self.IPLIST_PREDEFINED = [x for x in sum(self.IPLIST_ALIAS.values(), []) if re.match(r'^\d+\.\d+\.\d+\.\d+$', x) or ':' in x]

        if self.GAE_IPV6 and 'google_ipv6' in self.IPLIST_ALIAS:
            for name in self.IPLIST_ALIAS.keys():
                if name.startswith('google') and name not in ('google_ipv6', 'google_talk'):
                    self.IPLIST_ALIAS[name] = self.IPLIST_ALIAS['google_ipv6']

        self.PAC_ENABLE = self.CONFIG.getint('pac', 'enable')
        self.PAC_IP = self.CONFIG.get('pac', 'ip')
        self.PAC_PORT = self.CONFIG.getint('pac', 'port')
        self.PAC_FILE = self.CONFIG.get('pac', 'file').lstrip('/')
        self.PAC_GFWLIST = self.CONFIG.get('pac', 'gfwlist')
        self.PAC_ADBLOCK = self.CONFIG.get('pac', 'adblock')
        self.PAC_ADMODE = self.CONFIG.getint('pac', 'admode')
        self.PAC_EXPIRED = self.CONFIG.getint('pac', 'expired')

        self.PHP_ENABLE = self.CONFIG.getint('php', 'enable')
        self.PHP_LISTEN = self.CONFIG.get('php', 'listen')
        self.PHP_PASSWORD = self.CONFIG.get('php', 'password') if self.CONFIG.has_option('php', 'password') else ''
        self.PHP_CRLF = self.CONFIG.getint('php', 'crlf') if self.CONFIG.has_option('php', 'crlf') else 1
        self.PHP_VALIDATE = self.CONFIG.getint('php', 'validate') if self.CONFIG.has_option('php', 'validate') else 0
        self.PHP_KEEPALIVE = self.CONFIG.getint('php', 'keepalive')
        self.PHP_FETCHSERVER = self.CONFIG.get('php', 'fetchserver')
        self.PHP_HOSTS = self.CONFIG.get('php', 'hosts').split('|') if self.CONFIG.get('php', 'hosts') else []

        self.VPS_ENABLE = self.CONFIG.getint('vps', 'enable')
        self.VPS_LISTEN = self.CONFIG.get('vps', 'listen')
        self.VPS_FETCHSERVER = self.CONFIG.get('vps', 'fetchserver')

        self.PROXY_ENABLE = self.CONFIG.getint('proxy', 'enable')
        self.PROXY_AUTODETECT = self.CONFIG.getint('proxy', 'autodetect') if self.CONFIG.has_option('proxy', 'autodetect') else 0
        self.PROXY_HOST = self.CONFIG.get('proxy', 'host')
        self.PROXY_PORT = self.CONFIG.getint('proxy', 'port')
        self.PROXY_USERNAME = self.CONFIG.get('proxy', 'username')
        self.PROXY_PASSWROD = self.CONFIG.get('proxy', 'password')

        if not self.PROXY_ENABLE and self.PROXY_AUTODETECT:
            system_proxy = ProxyUtil.get_system_proxy()
            if system_proxy and self.LISTEN_IP not in system_proxy:
                _, username, password, address = ProxyUtil.parse_proxy(system_proxy)
                proxyhost, _, proxyport = address.rpartition(':')
                self.PROXY_ENABLE = 1
                self.PROXY_USERNAME = username
                self.PROXY_PASSWROD = password
                self.PROXY_HOST = proxyhost
                self.PROXY_PORT = int(proxyport)
        if self.PROXY_ENABLE:
            self.GAE_MODE = 'https'

        self.AUTORANGE_HOSTS = self.CONFIG.get('autorange', 'hosts').split('|')
        self.AUTORANGE_ENDSWITH = tuple(self.CONFIG.get('autorange', 'endswith').split('|'))
        self.AUTORANGE_NOENDSWITH = tuple(self.CONFIG.get('autorange', 'noendswith').split('|'))
        self.AUTORANGE_MAXSIZE = self.CONFIG.getint('autorange', 'maxsize')
        self.AUTORANGE_WAITSIZE = self.CONFIG.getint('autorange', 'waitsize')
        self.AUTORANGE_BUFSIZE = self.CONFIG.getint('autorange', 'bufsize')
        self.AUTORANGE_THREADS = self.CONFIG.getint('autorange', 'threads')

        self.FETCHMAX_LOCAL = self.CONFIG.getint('fetchmax', 'local') if self.CONFIG.get('fetchmax', 'local') else 3
        self.FETCHMAX_SERVER = self.CONFIG.get('fetchmax', 'server')

        self.DNS_ENABLE = self.CONFIG.getint('dns', 'enable')
        self.DNS_LISTEN = self.CONFIG.get('dns', 'listen')
        self.DNS_SERVERS = self.HTTP_DNS or self.CONFIG.get('dns', 'servers').split('|')
        self.DNS_BLACKLIST = set(self.CONFIG.get('dns', 'blacklist').split('|'))
        self.DNS_TCPOVER = tuple(self.CONFIG.get('dns', 'tcpover').split('|')) if self.CONFIG.get('dns', 'tcpover').strip() else tuple()
        if self.GAE_IPV6:
            self.DNS_SERVERS = [x for x in self.DNS_SERVERS if ':' in x]
        else:
            self.DNS_SERVERS = [x for x in self.DNS_SERVERS if ':' not in x]

        self.USERAGENT_ENABLE = self.CONFIG.getint('useragent', 'enable')
        self.USERAGENT_STRING = self.CONFIG.get('useragent', 'string')

        self.LOVE_ENABLE = self.CONFIG.getint('love', 'enable')
        self.LOVE_TIP = self.CONFIG.get('love', 'tip').encode('utf8').decode('unicode-escape').split('|')

    def resolve_iplist(self):
        # https://support.google.com/websearch/answer/186669?hl=zh-Hans
        def do_local_resolve(host, queue):
            assert isinstance(host, basestring)
            for _ in xrange(3):
                try:
                    family = socket.AF_INET6 if self.GAE_IPV6 else socket.AF_INET
                    iplist = [x[-1][0] for x in socket.getaddrinfo(host, 80, family)]
                    queue.put((host, iplist))
                except (socket.error, OSError) as e:
                    logging.warning('socket.getaddrinfo host=%r failed: %s', host, e)
                    time.sleep(0.1)
        google_blacklist = ['216.239.32.20'] + list(self.DNS_BLACKLIST)
        google_blacklist_prefix = tuple(x for x in self.DNS_BLACKLIST if x.endswith('.'))
        for name, need_resolve_hosts in list(self.IPLIST_ALIAS.items()):
            if all(re.match(r'\d+\.\d+\.\d+\.\d+', x) or ':' in x for x in need_resolve_hosts):
                continue
            need_resolve_remote = [x for x in need_resolve_hosts if ':' not in x and not re.match(r'\d+\.\d+\.\d+\.\d+', x)]
            resolved_iplist = [x for x in need_resolve_hosts if x not in need_resolve_remote]
            result_queue = Queue.Queue()
            for host in need_resolve_remote:
                logging.debug('local resolve host=%r', host)
                thread.start_new_thread(do_local_resolve, (host, result_queue))
            for _ in xrange(len(need_resolve_remote)):
                try:
                    host, iplist = result_queue.get(timeout=8)
                    resolved_iplist += iplist
                except Queue.Empty:
                    break
            if name.startswith('google_') and name not in ('google_cn', 'google_hk') and resolved_iplist:
                iplist_prefix = re.split(r'[\.:]', resolved_iplist[0])[0]
                resolved_iplist = list(set(x for x in resolved_iplist if x.startswith(iplist_prefix)))
            else:
                resolved_iplist = list(set(resolved_iplist))
            if name.startswith('google_'):
                resolved_iplist = list(set(resolved_iplist) - set(google_blacklist))
                resolved_iplist = [x for x in resolved_iplist if not x.startswith(google_blacklist_prefix)]
            if len(resolved_iplist) == 0 and name in ('google_hk', 'google_cn') and not self.GAE_IPV6:
                logging.error('resolve %s host return empty! please retry!', name)
                sys.exit(-1)
            logging.info('resolve name=%s host to iplist=%r', name, resolved_iplist)
            common.IPLIST_ALIAS[name] = resolved_iplist
        if self.IPLIST_ALIAS.get('google_cn', []):
            try:
                for _ in xrange(4):
                    socket.create_connection((random.choice(self.IPLIST_ALIAS['google_cn']), 80), timeout=2).close()
            except socket.error:
                self.IPLIST_ALIAS['google_cn'] = []
        if len(self.IPLIST_ALIAS.get('google_cn', [])) < 4 and self.IPLIST_ALIAS.get('google_hk', []):
            logging.warning('google_cn resolved too short iplist=%s, switch to google_hk', self.IPLIST_ALIAS.get('google_cn', []))
            self.IPLIST_ALIAS['google_cn'] = self.IPLIST_ALIAS['google_hk']

    def summary(self):
        info = ''
        info += '------------------------------------------------------\n'
        info += 'GoAgent Version    : %s (python/%s gevent/%s pyopenssl/%s)\n' % (__version__, sys.version[:5], gevent.__version__, OpenSSL.__version__)
        info += 'Uvent Version      : %s (pyuv/%s libuv/%s)\n' % (__import__('uvent').__version__, __import__('pyuv').__version__, __import__('pyuv').LIBUV_VERSION) if all(x in sys.modules for x in ('pyuv', 'uvent')) else ''
        info += 'Listen Address     : %s:%d\n' % (self.LISTEN_IP, self.LISTEN_PORT)
        info += 'Local Proxy        : %s:%s\n' % (self.PROXY_HOST, self.PROXY_PORT) if self.PROXY_ENABLE else ''
        info += 'Debug INFO         : %s\n' % self.LISTEN_DEBUGINFO if self.LISTEN_DEBUGINFO else ''
        info += 'GAE Mode           : %s\n' % ('%s (%s)' % (self.GAE_MODE, self.GAE_SSLVERSION) if common.GAE_MODE == 'https' else self.GAE_MODE)
        info += 'GAE IPv6           : %s\n' % self.GAE_IPV6 if self.GAE_IPV6 else ''
        info += 'GAE APPID          : %s\n' % '|'.join(self.GAE_APPIDS)
        info += 'GAE Validate       : %s\n' % self.GAE_VALIDATE if self.GAE_VALIDATE else ''
        info += 'GAE Obfuscate      : %s\n' % self.GAE_OBFUSCATE if self.GAE_OBFUSCATE else ''
        if common.PAC_ENABLE:
            info += 'Pac Server         : http://%s:%d/%s\n' % (self.PAC_IP if self.PAC_IP and self.PAC_IP != '0.0.0.0' else ProxyUtil.get_listen_ip(), self.PAC_PORT, self.PAC_FILE)
            info += 'Pac File           : file://%s\n' % os.path.abspath(self.PAC_FILE)
        if common.PHP_ENABLE:
            info += 'PHP Listen         : %s\n' % common.PHP_LISTEN
            info += 'PHP FetchServer    : %s\n' % common.PHP_FETCHSERVER
        if common.VPS_ENABLE:
            info += 'VPS Listen         : %s\n' % common.VPS_LISTEN
            info += 'VPS FetchServer    : %s\n' % common.VPS_FETCHSERVER
        if common.DNS_ENABLE:
            info += 'DNS Listen         : %s\n' % common.DNS_LISTEN
            info += 'DNS Servers        : %s\n' % '|'.join(common.DNS_SERVERS)
        info += '------------------------------------------------------\n'
        return info

common = Common()


def pre_start():
    if sys.platform == 'cygwin':
        logging.info('cygwin is not officially supported, please continue at your own risk :)')
        #sys.exit(-1)
    elif os.name == 'posix':
        try:
            import resource
            resource.setrlimit(resource.RLIMIT_NOFILE, (8192, -1))
        except ValueError:
            pass
    elif os.name == 'nt':
        import ctypes
        ctypes.windll.kernel32.SetConsoleTitleW(u'GoAgent v%s' % __version__)
        if not common.LISTEN_VISIBLE:
            ctypes.windll.user32.ShowWindow(ctypes.windll.kernel32.GetConsoleWindow(), 0)
        else:
            ctypes.windll.user32.ShowWindow(ctypes.windll.kernel32.GetConsoleWindow(), 1)
        if common.LOVE_ENABLE and random.randint(1, 100) <= 5:
            title = ctypes.create_unicode_buffer(1024)
            ctypes.windll.kernel32.GetConsoleTitleW(ctypes.byref(title), len(title)-1)
            ctypes.windll.kernel32.SetConsoleTitleW('%s %s' % (title.value, random.choice(common.LOVE_TIP)))
        blacklist = {'360safe': False,
                     'QQProtect': False, }
        softwares = [k for k, v in blacklist.items() if v]
        if softwares:
            tasklist = '\n'.join(x.name for x in get_process_list()).lower()
            softwares = [x for x in softwares if x.lower() in tasklist]
            if softwares:
                title = u'GoAgent 建议'
                error = u'某些安全软件(如 %s)可能和本软件存在冲突，造成 CPU 占用过高。\n如有此现象建议暂时退出此安全软件来继续运行GoAgent' % ','.join(softwares)
                ctypes.windll.user32.MessageBoxW(None, error, title, 0)
                #sys.exit(0)
    if os.path.isfile('/proc/cpuinfo'):
        with open('/proc/cpuinfo', 'rb') as fp:
            m = re.search(r'(?im)(BogoMIPS|cpu MHz)\s+:\s+([\d\.]+)', fp.read())
            if m and float(m.group(2)) < 1000:
                logging.warning("*NOTE*, if you want to fix high cpu usage, please decrease [gae]window")
    if gevent.__version__ < '1.0':
        logging.warning("*NOTE*, please upgrade to gevent 1.1 as possible")
    if common.GAE_PAGESPEED and not common.GAE_OBFUSCATE:
        logging.critical("*NOTE*, [gae]pagespeed=1 requires [gae]obfuscate=1")
        sys.exit(-1)
    if common.GAE_ENABLE and common.GAE_APPIDS[0] == 'goagent':
        logging.warning('please edit %s to add your appid to [gae] !', common.CONFIG_FILENAME)
    if common.GAE_ENABLE and common.GAE_MODE == 'http' and common.GAE_PASSWORD == '':
        logging.warning('to enable http mode, you should set %r [gae]password = <your_pass> and [gae]options = rc4', common.CONFIG_FILENAME)
    if common.GAE_TRANSPORT:
        GAEProxyHandler.disable_transport_ssl = False
    if common.PAC_ENABLE:
        pac_ip = ProxyUtil.get_listen_ip() if common.PAC_IP in ('', '::', '0.0.0.0') else common.PAC_IP
        url = 'http://%s:%d/%s' % (pac_ip, common.PAC_PORT, common.PAC_FILE)
        spawn_later(600, urllib2.build_opener(urllib2.ProxyHandler({})).open, url)
    if not common.DNS_ENABLE:
        if not common.HTTP_DNS:
            common.HTTP_DNS = common.DNS_SERVERS[:]
        for dnsservers_ref in (common.HTTP_DNS, common.DNS_SERVERS):
            any(dnsservers_ref.insert(0, x) for x in [y for y in get_dnsserver_list() if y not in dnsservers_ref])
        GAEProxyHandler.dns_servers = common.HTTP_DNS
        GAEProxyHandler.dns_blacklist = common.DNS_BLACKLIST
    else:
        GAEProxyHandler.dns_servers = common.HTTP_DNS or common.DNS_SERVERS
        GAEProxyHandler.dns_blacklist = common.DNS_BLACKLIST
    RangeFetch.threads = common.AUTORANGE_THREADS
    RangeFetch.maxsize = common.AUTORANGE_MAXSIZE
    RangeFetch.bufsize = common.AUTORANGE_BUFSIZE
    RangeFetch.waitsize = common.AUTORANGE_WAITSIZE
    if True:
        GAEProxyHandler.handler_filters.insert(0, AutoRangeFilter(common.AUTORANGE_HOSTS, common.AUTORANGE_ENDSWITH, common.AUTORANGE_NOENDSWITH, common.AUTORANGE_MAXSIZE))
    if common.GAE_REGIONS:
        GAEProxyHandler.handler_filters.insert(0, DirectRegionFilter(common.GAE_REGIONS))
    if common.CRLF_SITES:
        GAEProxyHandler.handler_filters.insert(0, CRLFSitesFilter(common.CRLF_SITES, common.NOCRLF_SITES))
    if common.URLREWRITE_MAP:
        GAEProxyHandler.handler_filters.insert(0, URLRewriteFilter(common.URLREWRITE_MAP, common.FORCEHTTPS_SITES, common.NOFORCEHTTPS_SITES))
    if common.FAKEHTTPS_SITES:
        GAEProxyHandler.handler_filters.insert(0, FakeHttpsFilter(common.FAKEHTTPS_SITES, common.NOFAKEHTTPS_SITES))
    if common.FORCEHTTPS_SITES:
        GAEProxyHandler.handler_filters.insert(0, ForceHttpsFilter(common.FORCEHTTPS_SITES, common.NOFORCEHTTPS_SITES))
    if common.WITHGAE_SITES or common.WITHPHP_SITES:
        GAEProxyHandler.handler_filters.insert(0, WithGAEFilter(common.WITHGAE_SITES, common.WITHPHP_SITES))
    if common.USERAGENT_ENABLE:
        GAEProxyHandler.handler_filters.insert(0, UserAgentFilter(common.USERAGENT_STRING))
    if common.LISTEN_USERNAME:
        GAEProxyHandler.handler_filters.insert(0, AuthFilter(common.LISTEN_USERNAME, common.LISTEN_PASSWORD))


def main():
    global __file__
    __file__ = os.path.abspath(__file__)
    if os.path.islink(__file__):
        __file__ = getattr(os, 'readlink', lambda x: x)(__file__)
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    logging.basicConfig(level=logging.DEBUG if common.LISTEN_DEBUGINFO else logging.INFO, format='%(levelname)s - %(asctime)s %(message)s', datefmt='[%b %d %H:%M:%S]')
    pre_start()
    sys.stderr.write(common.summary())

    if common.PAC_ENABLE:
        thread.start_new_thread(LocalProxyServer((common.PAC_IP, common.PAC_PORT), PACProxyHandler).serve_forever, tuple())

    if common.DNS_ENABLE:
        try:
            from dnsproxy import DNSServer
            host, port = common.DNS_LISTEN.split(':')
            dns_server = DNSServer((host, int(port)), dns_servers=common.DNS_SERVERS, dns_blacklist=common.DNS_BLACKLIST, dns_tcpover=common.DNS_TCPOVER)
            thread.start_new_thread(dns_server.serve_forever, tuple())
        except ImportError:
            logging.exception('GoAgent DNSServer requires dnslib and gevent 1.0')
            sys.exit(-1)

    if common.GAE_ENABLE or common.PHP_ENABLE:
        CertUtil.check_ca()

    if common.PROXY_ENABLE:
        if common.GAE_ENABLE:
            GAEProxyHandler.net2 = ProxyNet2(common.PROXY_HOST, common.PROXY_PORT, common.PROXY_USERNAME, common.PROXY_PASSWROD)
        if common.PHP_ENABLE:
            PHPProxyHandler.net2 = ProxyNet2(common.PROXY_HOST, common.PROXY_PORT, common.PROXY_USERNAME, common.PROXY_PASSWROD)
        if common.VPS_ENABLE:
            VPSServer.net2 = ProxyNet2(common.PROXY_HOST, common.PROXY_PORT, common.PROXY_USERNAME, common.PROXY_PASSWROD)

    php_server = None
    if common.PHP_ENABLE:
        host, port = common.PHP_LISTEN.split(':')
        PHPProxyHandler.handler_plugins['php'] = PHPFetchPlugin(common.PHP_FETCHSERVER, common.PHP_PASSWORD, common.PHP_VALIDATE)
        php_server = LocalProxyServer((host, int(port)), PHPProxyHandler)
        thread.start_new_thread(php_server.serve_forever, tuple())

    vps_server = None
    if common.VPS_ENABLE:
        host, port = common.VPS_LISTEN.split(':')
        VPSServer.net2 = AdvancedNet2(window=2, ssl_version='SSLv23', dns_servers=common.DNS_SERVERS, dns_blacklist=common.DNS_BLACKLIST)
        VPSServer.net2.enable_connection_cache()
        VPSServer.net2.enable_connection_keepalive()
        VPSServer.net2.enable_openssl_session_cache()
        VPSServer.net2.openssl_context.set_cipher_list('RC4-MD5:RC4-SHA:!aNULL:!eNULL')
        vps_server = VPSServer((host, int(port)), backlog=1024, fetchservers=common.VPS_FETCHSERVER.split('|'))
        thread.start_new_thread(vps_server.serve_forever, tuple())

    if common.GAE_ENABLE:
        if common.PHP_ENABLE:
            GAEProxyHandler.handler_plugins['php'] = php_server.RequestHandlerClass.handler_plugins['php']
        if os.name == 'nt':
            GAEProxyHandler.handler_plugins['strip'] = StripPluginEx()
        gae_server = LocalProxyServer((common.LISTEN_IP, common.LISTEN_PORT), GAEProxyHandler)
        gae_server.serve_forever()
    else:
        gevent.sleep(sys.maxint)

if __name__ == '__main__':
    main()

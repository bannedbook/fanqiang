#!/usr/bin/env python
# coding:utf-8


import platform
import urlparse
import json
import os
import re
import subprocess
import cgi
import sys
import datetime
import locale
import time
import hashlib
import OpenSSL
import httplib

import yaml
import simple_http_server
import env_info
import utils

import front_base.openssl_wrap as openssl_wrap

from xlog import getLogger
xlog = getLogger("gae_proxy")
from config import config, direct_config
import check_local_network
import cert_util
import ipv6_tunnel
from front import front, direct_front
import download_gae_lib

current_path = os.path.dirname(os.path.abspath(__file__))

root_path = os.path.abspath(os.path.join(current_path, os.pardir, os.pardir))
top_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir))
data_path = os.path.abspath( os.path.join(top_path, 'data', 'gae_proxy'))
web_ui_path = os.path.join(current_path, os.path.pardir, "web_ui")


def get_fake_host():
    return "deja.com"


def test_appid_exist(ssl_sock, appid):
    request_data = 'GET /_gh/ HTTP/1.1\r\nHost: %s.appspot.com\r\n\r\n' % appid
    ssl_sock.send(request_data.encode())
    response = httplib.HTTPResponse(ssl_sock, buffering=True)

    response.begin()
    if response.status == 404:
        # xlog.warn("app check %s status:%d", appid, response.status)
        return False

    if response.status == 503:
        # out of quota
        return True

    if response.status != 200:
        xlog.warn("test appid %s status:%d", appid, response.status)

    content = response.read()
    if "GoAgent" not in content:
        # xlog.warn("app check %s content:%s", appid, content)
        return False

    return True


def test_appid(appid):
    for i in range(0, 3):
        ssl_sock = direct_front.connect_manager.get_ssl_connection()
        if not ssl_sock:
            continue

        try:
            return test_appid_exist(ssl_sock, appid)
        except Exception as e:
            xlog.warn("check_appid %s %r", appid, e)
            continue

    return False


def test_appids(appids):
    appid_list = appids.split("|")
    fail_appid_list = []
    for appid in appid_list:
        if not test_appid(appid):
            fail_appid_list.append(appid)
        else:
            # return success if one appid is work
            # just reduce wait time
            # here can be more ui friendly.
            return []
    return fail_appid_list


def get_openssl_version():
    return "%s %s h2:%s" % (OpenSSL.version.__version__,
                            front.openssl_context.ssl_version,
                            front.openssl_context.support_alpn_npn)


deploy_proc = None


class ControlHandler(simple_http_server.HttpServerHandler):
    def __init__(self, client_address, headers, command, path, rfile, wfile):
        self.client_address = client_address
        self.headers = headers
        self.command = command
        self.path = path
        self.rfile = rfile
        self.wfile = wfile

    def do_CONNECT(self):
        self.wfile.write(b'HTTP/1.1 403\r\nConnection: close\r\n\r\n')

    def do_GET(self):
        path = urlparse.urlparse(self.path).path
        if path == "/log":
            return self.req_log_handler()
        elif path == "/status":
            return self.req_status_handler()
        else:
            xlog.debug('GAEProxy Web_control %s %s %s ', self.address_string(), self.command, self.path)


        if path == '/deploy':
            return self.req_deploy_handler()
        elif path == "/config":
            return self.req_config_handler()
        elif path == "/ip_list":
            return self.req_ip_list_handler()
        elif path == "/scan_ip":
            return self.req_scan_ip_handler()
        elif path == "/ssl_pool":
            return self.req_ssl_pool_handler()
        elif path == "/workers":
            return self.req_workers_handler()
        elif path == "/download_cert":
            return self.req_download_cert_handler()
        elif path == "/is_ready":
            return self.req_is_ready_handler()
        elif path == "/test_ip":
            return self.req_test_ip_handler()
        elif path == "/check_ip":
            return self.req_check_ip_handler()
        elif path == "/debug":
            return self.req_debug_handler()
        elif path.startswith("/ipv6_tunnel"):
            return self.req_ipv6_tunnel_handler()
        elif path == "/quit":
            front.stop()
            direct_front.stop()
            data = "Quit"
            self.wfile.write(('HTTP/1.1 200\r\nContent-Type: %s\r\nContent-Length: %s\r\n\r\n' % ('text/plain', len(data))).encode())
            self.wfile.write(data)
            #sys.exit(0)
            #quit()
            #os._exit(0)
            return
        elif path.startswith("/wizard/"):
            file_path = os.path.abspath(os.path.join(web_ui_path, '/'.join(path.split('/')[1:])))
            if not os.path.isfile(file_path):
                self.wfile.write(b'HTTP/1.1 404 Not Found\r\n\r\n')
                xlog.warn('%s %s %s wizard file %s not found', self.address_string(), self.command, self.path, file_path)
                return

            if file_path.endswith('.html'):
                mimetype = 'text/html'
            elif file_path.endswith('.png'):
                mimetype = 'image/png'
            elif file_path.endswith('.jpg') or file_path.endswith('.jpeg'):
                mimetype = 'image/jpeg'
            else:
                mimetype = 'application/octet-stream'

            self.send_file(file_path, mimetype)
            return
        else:
            xlog.warn('Control Req %s %s %s ', self.address_string(), self.command, self.path)

        # check for '..', which will leak file
        if re.search(r'(\.{2})', self.path) is not None:
            self.wfile.write(b'HTTP/1.1 404\r\n\r\n')
            xlog.warn('%s %s %s haking', self.address_string(), self.command, self.path )
            return


        filename = os.path.normpath('./' + path)
        if self.path.startswith(('http://', 'https://')):
            data = b'HTTP/1.1 200\r\nCache-Control: max-age=86400\r\nExpires:Oct, 01 Aug 2100 00:00:00 GMT\r\nConnection: close\r\n'

            data += b'\r\n'
            self.wfile.write(data)
            xlog.info('%s "%s %s HTTP/1.1" 200 -', self.address_string(), self.command, self.path)
        elif os.path.isfile(filename):
            if filename.endswith('.pac'):
                mimetype = 'text/plain'
            else:
                mimetype = 'application/octet-stream'
            #self.send_file(filename, mimetype)
        else:
            self.wfile.write(b'HTTP/1.1 404\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n404 Not Found')
            xlog.info('%s "%s %s HTTP/1.1" 404 -', self.address_string(), self.command, self.path)

    def do_POST(self):
        try:
            refer = self.headers.getheader('Referer')
            netloc = urlparse.urlparse(refer).netloc
            if not netloc.startswith("127.0.0.1") and not netloc.startswitch("localhost"):
                xlog.warn("web control ref:%s refuse", netloc)
                return
        except:
            pass

        xlog.debug ('GAEProxy web_control %s %s %s ', self.address_string(), self.command, self.path)
        try:
            ctype, pdict = cgi.parse_header(self.headers.getheader('content-type'))
            if ctype == 'multipart/form-data':
                self.postvars = cgi.parse_multipart(self.rfile, pdict)
            elif ctype == 'application/x-www-form-urlencoded':
                length = int(self.headers.getheader('content-length'))
                self.postvars = urlparse.parse_qs(self.rfile.read(length), keep_blank_values=1)
            else:
                self.postvars = {}
        except:
            self.postvars = {}

        path = urlparse.urlparse(self.path).path
        if path == '/deploy':
            return self.req_deploy_handler()
        elif path == "/config":
            return self.req_config_handler()
        elif path == "/scan_ip":
            return self.req_scan_ip_handler()
        elif path.startswith("/importip"):
            return self.req_importip_handler()
        else:
            self.wfile.write(b'HTTP/1.1 404\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n404 Not Found')
            xlog.info('%s "%s %s HTTP/1.1" 404 -', self.address_string(), self.command, self.path)

    def req_log_handler(self):
        req = urlparse.urlparse(self.path).query
        reqs = urlparse.parse_qs(req, keep_blank_values=True)
        data = ''

        cmd = "get_last"
        if reqs["cmd"]:
            cmd = reqs["cmd"][0]

        if cmd == "get_last":
            max_line = int(reqs["max_line"][0])
            data = xlog.get_last_lines(max_line)
        elif cmd == "get_new":
            last_no = int(reqs["last_no"][0])
            data = xlog.get_new_lines(last_no)
        else:
            xlog.error('WebUI log from:%s unknown cmd:%s path:%s ', self.address_string(), self.command, self.path)

        mimetype = 'text/plain'
        self.send_response_nc(mimetype, data)

    def get_launcher_version(self):
        data_path = os.path.abspath( os.path.join(top_path, 'data', 'launcher', 'config.yaml'))
        try:
            config = yaml.load(file(data_path, 'r'))
            return config["modules"]["launcher"]["current_version"]
            #print yaml.dump(config)
        except yaml.YAMLError, exc:
            print "Error in configuration file:", exc
            return "unknown"

    @staticmethod
    def xxnet_version():
        version_file = os.path.join(root_path, "version.txt")
        try:
            with open(version_file, "r") as fd:
                version = fd.read()
            return version
        except Exception as e:
            xlog.exception("xxnet_version fail")
        return "get_version_fail"

    def get_os_language(self):
        if hasattr(self, "lang_code"):
            return self.lang_code

        try:
            lang_code, code_page = locale.getdefaultlocale()
            #('en_GB', 'cp1252'), en_US,
            self.lang_code = lang_code
            return lang_code
        except:
            #Mac fail to run this
            pass

        if sys.platform == "darwin":
            try:
                oot = os.pipe()
                p = subprocess.Popen(["/usr/bin/defaults", 'read', 'NSGlobalDomain', 'AppleLanguages'],stdout=oot[1])
                p.communicate()
                lang_code = os.read(oot[0],10000)
                self.lang_code = lang_code
                return lang_code
            except:
                pass

        lang_code = 'Unknown'
        return lang_code

    def req_status_handler(self):
        if "user-agent" in self.headers.dict:
            user_agent = self.headers.dict["user-agent"]
        else:
            user_agent = ""

        if config.PROXY_ENABLE:
            lan_proxy = "%s://%s:%s" % (config.PROXY_TYPE, config.PROXY_HOST, config.PROXY_PORT)
        else:
            lan_proxy = "Disable"

        res_arr = {
            "sys_platform": "%s, %s" % (platform.machine(), platform.platform()),
            "os_system": platform.system(),
            "os_version": platform.version(),
            "os_release": platform.release(),
            "architecture": platform.architecture(),
            "os_detail": env_info.os_detail(),
            "language": self.get_os_language(),
            "browser": user_agent,
            "xxnet_version": self.xxnet_version(),
            "python_version": platform.python_version(),
            "openssl_version": get_openssl_version(),

            "proxy_listen": str(config.listen_ip) + ":" + str(config.listen_port),
            "use_ipv6": config.use_ipv6,
            "lan_proxy": lan_proxy,

            "gae_appid": "|".join(config.GAE_APPIDS),
            "working_appid": "|".join(front.appid_manager.working_appid_list),
            "out_of_quota_appids": "|".join(front.appid_manager.out_of_quota_appids),
            "not_exist_appids": "|".join(front.appid_manager.not_exist_appids),

            "ipv4_state": check_local_network.IPv4.get_stat(),
            "ipv6_state": check_local_network.IPv6.get_stat(),
            #"ipv6_tunnel": ipv6_tunnel.state(),
            "all_ip_num": len(front.ip_manager.ip_list),
            "good_ipv4_num": front.ip_manager.good_ipv4_num,
            "good_ipv6_num": front.ip_manager.good_ipv6_num,
            "connected_link_new": len(front.connect_manager.new_conn_pool.pool),
            "connection_pool_min": config.https_connection_pool_min,
            "worker_h1": front.http_dispatcher.h1_num,
            "worker_h2": front.http_dispatcher.h2_num,
            "is_idle": int(front.http_dispatcher.is_idle()),
            "scan_ip_thread_num": front.ip_manager.scan_thread_count,
            "ip_quality": front.ip_manager.ip_quality(),

            "fake_host": get_fake_host()
        }
        data = json.dumps(res_arr, indent=0, sort_keys=True)
        self.send_response_nc('text/html', data)

    def req_config_handler(self):
        req = urlparse.urlparse(self.path).query
        reqs = urlparse.parse_qs(req, keep_blank_values=True)
        data = ''

        appid_updated = False

        try:
            if reqs['cmd'] == ['get_config']:
                ret_config = {
                    "appid": "|".join(config.GAE_APPIDS),
                    "auto_adjust_scan_ip_thread_num": config.auto_adjust_scan_ip_thread_num,
                    "scan_ip_thread_num": config.max_scan_ip_thread_num,
                    "use_ipv6": config.use_ipv6,
                    "setting_level": config.setting_level,
                    "connect_receive_buffer": config.connect_receive_buffer,
                }
                data = json.dumps(ret_config, default=lambda o: o.__dict__)
            elif reqs['cmd'] == ['set_config']:
                appids = self.postvars['appid'][0]
                if appids != "|".join(config.GAE_APPIDS):
                    if appids and (front.ip_manager.good_ipv4_num + front.ip_manager.good_ipv6_num):
                        fail_appid_list = test_appids(appids)
                        if len(fail_appid_list):
                            fail_appid = "|".join(fail_appid_list)
                            data = json.dumps({"res": "fail",
                                               "reason": "appid fail:" + fail_appid
                                               })
                            return

                    appid_updated = True
                    if appids:
                        xlog.info("set appids:%s", appids)
                        config.GAE_APPIDS = appids.split("|")
                    else:
                        config.GAE_APPIDS = []

                config.save()

                config.load()
                front.appid_manager.reset_appid()
                if appid_updated:
                    front.http_dispatcher.close_all_worker()

                front.ip_manager.reset()

                data = '{"res":"success"}'
                #http_request("http://127.0.0.1:8085/init_module?module=gae_proxy&cmd=restart")
            elif reqs['cmd'] == ['set_config_level']:
                setting_level = self.postvars['setting_level'][0]
                if setting_level:
                    xlog.info("set globa config level to %s", setting_level)
                    config.set_level(setting_level)
                    direct_config.set_level(setting_level)
                    front.ip_manager.load_config()
                    front.ip_manager.adjust_scan_thread_num()
                    front.ip_manager.remove_slowest_ip()
                    front.ip_manager.search_more_ip()

                connect_receive_buffer = int(self.postvars['connect_receive_buffer'][0])
                if 8192 <= connect_receive_buffer <= 2097152 and connect_receive_buffer != config.connect_receive_buffer:
                    xlog.info("set connect receive buffer to %dKB", connect_receive_buffer // 1024)
                    config.connect_receive_buffer = connect_receive_buffer
                    config.save()
                    config.load()

                    front.connect_manager.new_conn_pool.clear()
                    direct_front.connect_manager.new_conn_pool.clear()
                    front.http_dispatcher.close_all_worker()
                    for _, http_dispatcher in direct_front.dispatchs.items():
                        http_dispatcher.close_all_worker()

                data = '{"res":"success"}'
        except Exception as e:
            xlog.exception("req_config_handler except:%s", e)
            data = '{"res":"fail", "except":"%s"}' % e
        finally:
            self.send_response_nc('text/html', data)

    def req_deploy_handler(self):
        global deploy_proc
        req = urlparse.urlparse(self.path).query
        reqs = urlparse.parse_qs(req, keep_blank_values=True)
        data = ''

        log_path = os.path.abspath(os.path.join(current_path, os.pardir, "server", 'upload.log'))
        time_now = datetime.datetime.today().strftime('%H:%M:%S-%a/%d/%b/%Y')

        if reqs['cmd'] == ['deploy']:
            appid = self.postvars['appid'][0]
            debug = int(self.postvars['debug'][0])

            if deploy_proc and deploy_proc.poll() == None:
                xlog.warn("deploy is running, request denied.")
                data = '{"res":"deploy is running", "time":"%s"}' % time_now

            else:
                try:
                    download_gae_lib.check_lib_or_download()

                    if os.path.isfile(log_path):
                        os.remove(log_path)
                    script_path = os.path.abspath(os.path.join(current_path, os.pardir, "server", 'uploader.py'))

                    args = [sys.executable, script_path, appid]
                    if debug:
                        args.append("-debug")

                    deploy_proc = subprocess.Popen(args)
                    xlog.info("deploy begin.")
                    data = '{"res":"success", "time":"%s"}' % time_now
                except Exception as e:
                    data = '{"res":"%s", "time":"%s"}' % (e, time_now)

        elif reqs['cmd'] == ['cancel']:
            if deploy_proc and deploy_proc.poll() == None:
                deploy_proc.kill()
                data = '{"res":"deploy is killed", "time":"%s"}' % time_now
            else:
                data = '{"res":"deploy is not running", "time":"%s"}' % time_now

        elif reqs['cmd'] == ['get_log']:
            if deploy_proc and os.path.isfile(log_path):
                with open(log_path, "r") as f:
                    content = f.read()
            else:
                content = ""

            status = 'init'
            if deploy_proc:
                if deploy_proc.poll() == None:
                    status = 'running'
                else:
                    status = 'finished'

            data = json.dumps({'status': status, 'log': content, 'time': time_now})

        self.send_response_nc('text/html', data)

    def req_importip_handler(self):
        req = urlparse.urlparse(self.path).query
        reqs = urlparse.parse_qs(req, keep_blank_values=True)
        data = ''

        if reqs['cmd'] == ['importip']:
            count = 0
            ip_list = self.postvars['ipList'][0]
            lines = ip_list.split("\n")
            for line in lines:
                addresses = line.split('|')
                for ip in addresses:
                    ip = ip.strip()
                    if not utils.check_ip_valid(ip):
                        continue
                    if front.ip_manager.add_ip(ip, 100, "google.com", "gws"):
                        count += 1
            data = '{"res":"%s"}' % count
            front.ip_manager.save(force=True)

        elif reqs['cmd'] == ['exportip']:
            data = '{"res":"'
            for ip in front.ip_manager.ip_list:
                if front.ip_manager.ip_dict[ip]['fail_times'] > 0:
                    continue
                data += "%s|" % ip
            data = data[0: len(data) - 1]
            data += '"}'

        self.send_response_nc('text/html', data)

    def req_test_ip_handler(self):
        req = urlparse.urlparse(self.path).query
        reqs = urlparse.parse_qs(req, keep_blank_values=True)

        ip = reqs['ip'][0]
        result = front.check_ip.check_ip(ip)
        if not result or not result.ok:
            data = "{'res':'fail'}"
        else:
            data = json.dumps("{'ip':'%s', 'handshake':'%s', 'server':'%s', 'domain':'%s'}" %
                  (ip, result.handshake_time, result.server_type, result.domain))

        self.send_response_nc('text/html', data)

    def req_ip_list_handler(self):
        time_now = time.time()
        data = "<html><body><div  style='float: left; white-space:nowrap;font-family: monospace;'>"
        data += "time:%d  pointer:%d<br>\r\n" % (time_now, front.ip_manager.ip_pointer)
        data += "<table><tr><th>N</th><th>IP</th><th>HS</th><th>Fails</th>"
        data += "<th>down_fail</th><th>links</th>"
        data += "<th>get_time</th><th>success_time</th><th>fail_time</th><th>down_fail_time</th>"
        data += "<th>data_active</th><th>transfered_data</th><th>Trans</th>"
        data += "<th>history</th></tr>\n"
        i = 1
        for ip in front.ip_manager.gws_ip_list:
            handshake_time = front.ip_manager.ip_dict[ip]["handshake_time"]

            fail_times = front.ip_manager.ip_dict[ip]["fail_times"]
            down_fail = front.ip_manager.ip_dict[ip]["down_fail"]
            links = front.ip_manager.ip_dict[ip]["links"]

            get_time = front.ip_manager.ip_dict[ip]["get_time"]
            if get_time:
                get_time = time_now - get_time

            success_time = front.ip_manager.ip_dict[ip]["success_time"]
            if success_time:
                success_time = time_now - success_time

            fail_time = front.ip_manager.ip_dict[ip]["fail_time"]
            if fail_time:
                fail_time = time_now - fail_time

            down_fail_time = front.ip_manager.ip_dict[ip]["down_fail_time"]
            if down_fail_time:
                down_fail_time = time_now - down_fail_time

            data_active = front.ip_manager.ip_dict[ip]["data_active"]
            if data_active:
                active_time = time_now - data_active
            else:
                active_time = 0

            history = front.ip_manager.ip_dict[ip]["history"]
            t0 = 0
            str_out = ''
            for item in history:
                t = item[0]
                v = item[1]
                if t0 == 0:
                    t0 = t
                time_per = int((t - t0) * 1000)
                t0 = t
                str_out += "%d(%s) " % (time_per, v)
            data += "<tr><td>%d</td><td>%s</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td>" \
                    "<td>%d</td><td>%d</td><td>%s</td></tr>\n" % \
                    (i, ip, handshake_time, fail_times, down_fail, links, get_time, success_time, fail_time, down_fail_time, \
                    active_time, str_out)
            i += 1

        data += "</table></div></body></html>"
        mimetype = 'text/html'
        self.send_response_nc(mimetype, data)

    def req_scan_ip_handler(self):
        req = urlparse.urlparse(self.path).query
        reqs = urlparse.parse_qs(req, keep_blank_values=True)
        data = ""
        if reqs['cmd'] == ['get_range']:
            data = front.ipv4_source.load_range_content()
        elif reqs['cmd'] == ['update']:
            #update ip_range if needed
            content = self.postvars['ip_range'][0]

            #check ip_range checksums, update if needed
            default_digest = hashlib.md5(front.ipv4_source.load_range_content(default=True)).hexdigest()
            old_digest = hashlib.md5(front.ipv4_source.load_range_content()).hexdigest()
            new_digest = hashlib.md5(content).hexdigest()

            if new_digest == default_digest:
                front.ipv4_source.remove_user_range()

            else:
                if old_digest != new_digest:
                    front.ipv4_source.update_range_content(content)

            if old_digest != new_digest:
                front.ipv4_source.load_ip_range()

            #update auto_adjust_scan_ip and scan_ip_thread_num
            should_auto_adjust_scan_ip = int(self.postvars['auto_adjust_scan_ip_thread_num'][0])
            thread_num_for_scan_ip = int(self.postvars['scan_ip_thread_num'][0])

            use_ipv6 = self.postvars['use_ipv6'][0]
            if config.use_ipv6 != use_ipv6:
                xlog.debug("use_ipv6 change to %s", use_ipv6)
                config.use_ipv6 = use_ipv6

            #update user config settings
            config.auto_adjust_scan_ip_thread_num = should_auto_adjust_scan_ip
            config.max_scan_ip_thread_num = thread_num_for_scan_ip
            config.save()
            config.load()

            front.ip_manager.adjust_scan_thread_num()

            #reponse 
            data='{"res":"success"}'

        mimetype = 'text/plain'
        self.send_response_nc(mimetype, data)

    def req_ssl_pool_handler(self):
        data = "New conn:\n"
        data += front.connect_manager.new_conn_pool.to_string()

        for host in front.connect_manager.host_conn_pool:
            data += "\nHost:%s\n" % host
            data += front.connect_manager.host_conn_pool[host].to_string()

        mimetype = 'text/plain'
        self.send_response_nc(mimetype, data)

    def req_workers_handler(self):
        data = front.http_dispatcher.to_string()

        mimetype = 'text/plain'
        self.send_response_nc(mimetype, data)

    def req_download_cert_handler(self):
        filename = cert_util.CertUtil.ca_keyfile
        with open(filename, 'rb') as fp:
            data = fp.read()
        mimetype = 'application/x-x509-ca-cert'

        self.wfile.write(('HTTP/1.1 200\r\nContent-Disposition: inline; filename=CA.crt\r\nContent-Type: %s\r\nContent-Length: %s\r\n\r\n' % (mimetype, len(data))).encode())
        self.wfile.write(data)

    def req_is_ready_handler(self):
        data = "%s" % config.cert_import_ready

        mimetype = 'text/plain'
        self.send_response_nc(mimetype, data)

    def req_check_ip_handler(self):
        req = urlparse.urlparse(self.path).query
        reqs = urlparse.parse_qs(req, keep_blank_values=True)
        data = ""
        if reqs['cmd'] == ['get_process']:
            all_ip_num = len(front.ip_manager.ip_dict)
            left_num = front.ip_manager.scan_exist_ip_queue.qsize()
            good_num = (front.ip_manager.good_ipv4_num + front.ip_manager.good_ipv6_num)
            data = json.dumps(dict(all_ip_num=all_ip_num, left_num=left_num, good_num=good_num))
            self.send_response_nc('text/plain', data)
        elif reqs['cmd'] == ['start']:
            left_num = front.ip_manager.scan_exist_ip_queue.qsize()
            if left_num:
                self.send_response_nc('text/plain', '{"res":"fail", "reason":"running"}')
            else:
                front.ip_manager.start_scan_all_exist_ip()
                self.send_response_nc('text/plain', '{"res":"success"}')
        elif reqs['cmd'] == ['stop']:
            left_num = front.ip_manager.scan_exist_ip_queue.qsize()
            if not left_num:
                self.send_response_nc('text/plain', '{"res":"fail", "reason":"not running"}')
            else:
                front.ip_manager.stop_scan_all_exist_ip()
                self.send_response_nc('text/plain', '{"res":"success"}')
        else:
            return self.send_not_exist()

    def req_debug_handler(self):
        data = "ssl_socket num:%d \n" % openssl_wrap.socks_num
        for obj in [front.connect_manager, front.http_dispatcher]:
            data += "%s\r\n" % obj.__class__
            for attr in dir(obj):
                if attr.startswith("__"):
                    continue
                data += "    %s = %s\r\n" % (attr, getattr(obj, attr))

        mimetype = 'text/plain'
        self.send_response_nc(mimetype, data)

    def req_ipv6_tunnel_handler(self):
        req = urlparse.urlparse(self.path).query
        reqs = urlparse.parse_qs(req, keep_blank_values=True)
        data = ''

        log_path = os.path.join(data_path, "ipv6_tunnel.log")
        time_now = datetime.datetime.today().strftime('%H:%M:%S-%a/%d/%b/%Y')

        client_ip = self.client_address[0]
        is_local = client_ip.endswith("127.0.0.1") or client_ip == "::1"

        if reqs['cmd'] in [['enable'],
                           ['disable'],
                           ['test_teredo'],
                           ['test_teredo_usability'],
                           ['test_teredo_server'],
                           ['set_best_server']]:
            cmd = reqs['cmd'][0]
            xlog.info("ipv6_tunnel switch %s", cmd)

            # Don't remove log file at here.

            if cmd == "enable":
                result = ipv6_tunnel.enable(is_local)
            elif cmd == "disable":
                result = ipv6_tunnel.disable(is_local)
            elif cmd == "test_teredo":
                result = ipv6_tunnel.test_teredo()
            elif cmd == "test_teredo_usability":
                result = ipv6_tunnel.test_teredo(probe_server=False)
            elif cmd == "test_teredo_server":
                result = ipv6_tunnel.test_teredo(probe_nat=False)
            elif cmd == "set_best_server":
                result = ipv6_tunnel.set_best_server(is_local)
            else:
                xlog.warn("unknown cmd:%s", cmd)

            xlog.info("ipv6_tunnel switch %s, result: %s", cmd, result)

            data = json.dumps({'res': result, 'time': time_now})

        elif reqs['cmd'] == ['get_log']:
            if os.path.isfile(log_path):
                with open(log_path, "r") as f:
                    content = f.read()
            else:
                content = ""

            status = ipv6_tunnel.state()

            data = json.dumps({'status': status, 'log': content.decode("GBK"), 'time': time_now})

        elif reqs['cmd'] == ['get_priority']:
            data = json.dumps({'res': ipv6_tunnel.state_pp(), 'time': time_now})

        elif reqs['cmd'] == ['switch_pp']:
            data = json.dumps({'res': ipv6_tunnel.switch_pp(), 'time': time_now})

        self.send_response_nc('text/html', data)


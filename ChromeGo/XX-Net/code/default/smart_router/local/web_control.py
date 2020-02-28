#!/usr/bin/env python
# coding:utf-8

import urlparse
import os
import cgi

from xlog import getLogger
xlog = getLogger("smart_router")

import simple_http_server
import pac_server
import global_var as g

current_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.abspath(os.path.join(current_path, os.pardir, os.pardir))
web_ui_path = os.path.join(current_path, os.path.pardir, "web_ui")


class ControlHandler(simple_http_server.HttpServerHandler):
    def __init__(self, client_address, headers, command, path, rfile, wfile):
        self.client_address = client_address
        self.headers = headers
        self.command = command
        self.path = path
        self.rfile = rfile
        self.wfile = wfile

    def do_GET(self):
        path = urlparse.urlparse(self.path).path
        if path == "/log":
            return self.req_log_handler()
        elif path == "/status":
            return self.req_status()
        else:
            xlog.warn('Control Req %s %s %s ', self.address_string(), self.command, self.path)

    def do_POST(self):
        xlog.debug('Web_control %s %s %s ', self.address_string(), self.command, self.path)
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
        if path == '/rules':
            return self.req_rules_handler()
        elif path == "/cache":
            return self.req_cache_handler()
        elif path == "/config":
            return self.req_config_handler()
        else:
            xlog.info('%s "%s %s HTTP/1.1" 404 -', self.address_string(), self.command, self.path)
            return self.send_not_found()

    def req_log_handler(self):
        req = urlparse.urlparse(self.path).query
        reqs = urlparse.parse_qs(req, keep_blank_values=True)
        data = ''

        if reqs["cmd"]:
            cmd = reqs["cmd"][0]
        else:
            cmd = "get_last"

        if cmd == "get_last":
            max_line = int(reqs["max_line"][0])
            data = xlog.get_last_lines(max_line)
        elif cmd == "get_new":
            last_no = int(reqs["last_no"][0])
            data = xlog.get_new_lines(last_no)
        else:
            xlog.error('xtunnel log cmd:%s', cmd)

        mimetype = 'text/plain'
        self.send_response(mimetype, data)

    def req_rules_handler(self):
        reqs = {}
        for key in self.postvars:
            value = self.postvars[key][0]
            reqs[key] = value

        if "cmd" in reqs and reqs["cmd"]:
            cmd = reqs["cmd"]
        else:
            cmd = "get"

        if cmd == "get":
            rules = g.user_rules.get_rules()
            rules["res"] = "success"
            return self.response_json(rules)
        elif cmd == "set":
            g.user_rules.save(reqs)
            g.user_rules.load()
            return self.response_json({"res": "OK"})

    def req_config_handler(self):
        reqs = self.postvars
        if "cmd" in reqs and reqs["cmd"]:
            cmd = reqs["cmd"][0]
        else:
            cmd = "get"

        if cmd == "get":
            data = {
                "pac_policy": g.config.pac_policy,
                "country": g.config.country_code,
                "auto_direct":g.config.auto_direct,
                "auto_direct6":g.config.auto_direct6,
                "auto_gae": g.config.auto_gae,
                "enable_fake_ca": g.config.enable_fake_ca,
                "block_advertisement": g.config.block_advertisement
            }
            return self.response_json(data)
        elif cmd == "set":
            if "pac_policy" in reqs:
                pac_policy = reqs["pac_policy"][0]
                if pac_policy not in pac_server.allow_policy:
                    return self.response_json({"res": "fail", "reason": "policy not allow"})

                g.config.pac_policy = pac_policy
            if "country" in reqs:
                g.config.country_code = reqs["country"][0]
            if "auto_direct" in reqs:
                g.config.auto_direct = bool(int(reqs["auto_direct"][0]))
            if "auto_direct6" in reqs:
                g.config.auto_direct6 = bool(int(reqs["auto_direct6"][0]))
            if "auto_gae" in reqs:
                g.config.auto_gae = bool(int(reqs["auto_gae"][0]))
            if "enable_fake_ca" in reqs:
                g.config.enable_fake_ca = bool(int(reqs["enable_fake_ca"][0]))
            if "block_advertisement" in reqs:
                g.config.block_advertisement = bool(int(reqs["block_advertisement"][0]))
            g.config.save()
            return self.response_json({"res": "success"})

    def req_cache_handler(self):
        reqs = self.postvars
        if "cmd" in reqs and reqs["cmd"]:
            cmd = reqs["cmd"][0]
        else:
            cmd = "get"

        if cmd == "get":
            g.domain_cache.save(True)
            g.ip_cache.save(True)
            data = {
                "domain_cache_list": g.domain_cache.get_content(),
                "ip_cache_list": g.ip_cache.get_content(),
                "res": "success"
            }
            return self.response_json(data)
        elif cmd == "clean":
            g.domain_cache.clean()
            g.ip_cache.clean()
            return self.response_json({"res": "success"})

    def req_status(self):
        out_str = "pipe status:\n" + str(g.pipe_socks)
        self.send_response("text/plain", out_str)


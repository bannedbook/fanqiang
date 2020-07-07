#!/usr/bin/env python
# coding:utf-8

import urlparse
import os
import cgi
import time
import hashlib
import threading

import utils
from xlog import getLogger
xlog = getLogger("x_tunnel")

import simple_http_server
import global_var as g
import proxy_session
from cloudflare_front import web_control as cloudflare_web
from cloudfront_front import web_control as cloudfront_web
from tls_relay_front import web_control as tls_relay_web
from heroku_front import web_control as heroku_web
from front_dispatcher import all_fronts

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
        elif path == "/debug":
            data = g.session.status()
            return self.send_response('text/html', data)
        elif path == "/info":
            return self.req_info_handler()
        elif path == "/config":
            return self.req_config_handler()
        elif path == "/get_history":
            return self.req_get_history_handler()
        elif path == "/status":
            return self.req_status()
        elif path.startswith("/cloudflare_front/"):
            path = self.path[17:]
            controler = cloudflare_web.ControlHandler(self.client_address,
                             self.headers,
                             self.command, path,
                             self.rfile, self.wfile)
            controler.do_GET()
        elif path.startswith("/cloudfront_front/"):
            path = self.path[17:]
            controler = cloudfront_web.ControlHandler(self.client_address,
                             self.headers,
                             self.command, path,
                             self.rfile, self.wfile)
            controler.do_GET()
        elif path.startswith("/heroku_front/"):
            path = self.path[13:]
            controler = heroku_web.ControlHandler(self.client_address,
                             self.headers,
                             self.command, path,
                             self.rfile, self.wfile)
            controler.do_GET()
        elif path.startswith("/tls_relay_front/"):
            path = self.path[16:]
            controler = tls_relay_web.ControlHandler(self.client_address,
                             self.headers,
                             self.command, path,
                             self.rfile, self.wfile)
            controler.do_GET()
        else:
            xlog.warn('Control Req %s %s %s ', self.address_string(), self.command, self.path)

    def do_POST(self):
        xlog.debug('x-tunnel web_control %s %s %s ', self.address_string(), self.command, self.path)
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
        if path == '/login':
            return self.req_login_handler()
        elif path == "/logout":
            return self.req_logout_handler()
        elif path == "/register":
            return self.req_login_handler()
        elif path == "/config":
            return self.req_config_handler()
        elif path == "/order":
            return self.req_order_handler()
        elif path == "/transfer":
            return self.req_transfer_handler()
        elif path.startswith("/cloudflare_front/"):
            path = path[17:]
            controler = cloudflare_web.ControlHandler(self.client_address,
                                                      self.headers,
                                                      self.command, path,
                                                      self.rfile, self.wfile)
            controler.do_POST()
        elif path.startswith("/cloudfront_front/"):
            path = path[17:]
            controler = cloudfront_web.ControlHandler(self.client_address,
                                                      self.headers,
                                                      self.command, path,
                                                      self.rfile, self.wfile)
            controler.do_POST()
        elif path.startswith("/heroku_front/"):
            path = path[13:]
            controler = heroku_web.ControlHandler(self.client_address,
                                                      self.headers,
                                                      self.command, path,
                                                      self.rfile, self.wfile)
            controler.do_POST()
        elif path.startswith("/tls_relay_front/"):
            path = path[16:]
            controler = tls_relay_web.ControlHandler(self.client_address,
                                                      self.headers,
                                                      self.command, path,
                                                      self.rfile, self.wfile)
            controler.do_POST()
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

    def req_info_handler(self):
        if len(g.config.login_account) == 0 or len(g.config.login_password) == 0:
            return self.response_json({
                "res": "logout"
            })

        if proxy_session.center_login_process:
            return self.response_json({
                "res": "login_process"
            })

        req = urlparse.urlparse(self.path).query
        reqs = urlparse.parse_qs(req, keep_blank_values=True)

        force = False
        if 'force' in reqs:
            xlog.debug("req_info in force")
            force = 1

        time_now = time.time()
        if force or time_now - g.last_refresh_time > 3600 or \
                (g.last_api_error.startswith("status:") and (time_now - g.last_refresh_time > 30)):
            xlog.debug("x_tunnel force update info")
            g.last_refresh_time = time_now

            threading.Thread(target=proxy_session.request_balance, args=(None,None,False,False)).start()

            return self.response_json({
                "res": "login_process"
            })

        if len(g.last_api_error) and g.last_api_error != 'balance not enough':
            res_arr = {
                "res": "fail",
                "login_account": "%s" % (g.config.login_account),
                "reason": g.last_api_error
            }
        else:
            res_arr = {
                "res": "success",
                "login_account": "%s" % (g.config.login_account),
                "promote_code": g.promote_code,
                "promoter": g.promoter,
                "balance": "%f" % (g.balance),
                "quota": "%d" % (g.quota),
                "quota_list": g.quota_list,
                "traffic": g.session.traffic,
                "last_fail": g.last_api_error
            }
        self.response_json(res_arr)

    def req_login_handler(self):
        def check_email(email):
            import re
            if not re.match(r"[^@]+@[^@]+\.[^@]+", email):
                return False
            else:
                return True

        username    = str(self.postvars['username'][0])
        #username = utils.get_printable(username)
        password    = str(self.postvars['password'][0])
        promoter = self.postvars.get("promoter", [""])[0]
        is_register = int(self.postvars['is_register'][0])

        pa = check_email(username)
        if not pa:
            return self.response_json({
                "res": "fail",
                "reason": "Invalid email."
            })
        elif len(password) < 6:
            return self.response_json({
                "res": "fail",
                "reason": "Password needs at least 6 charactors."
            })

        if password == "_HiddenPassword":
            if username == g.config.login_account and len(g.config.login_password):
                password_hash = g.config.login_password
            else:

                res_arr = {
                    "res": "fail",
                    "reason": "account not exist"
                }
                return self.response_json(res_arr)
        else:
            password_hash = str(hashlib.sha256(password).hexdigest())

        res, reason = proxy_session.request_balance(username, password_hash, is_register,
                                                    update_server=True, promoter=promoter)
        if res:
            g.config.login_account  = username
            g.config.login_password = password_hash
            g.config.save()
            res_arr = {
                "res": "success",
                "balance": float(g.balance)
            }
            g.last_refresh_time = time.time()
            g.session.start()
        else:
            res_arr = {
                "res": "fail",
                "reason": reason
            }

        return self.response_json(res_arr)

    def req_logout_handler(self):
        g.config.login_account = ""
        g.config.login_password = ""
        g.config.save()

        g.session.stop()

        return self.response_json({"res": "success"})

    def req_config_handler(self):
        req = urlparse.urlparse(self.path).query
        reqs = urlparse.parse_qs(req, keep_blank_values=True)

        def is_server_available(server):
            if g.selectable and server == '':
                return True # "auto"
            else:
                for choice in g.selectable:
                    if choice[0] == server:
                        return True # "selectable"
                return False # "unselectable"

        if reqs['cmd'] == ['get']:
            g.config.load()
            server = {
                'selectable': g.selectable,
                'selected': 'auto' if g.config.server_host == '' else g.config.server_host,  # "auto" as default
                'available': is_server_available(g.config.server_host)
            }
            res = {
                'server': server,
                'promoter': g.promoter
            }
        elif reqs['cmd'] == ['set']:
            if 'server' in self.postvars:
                server = str(self.postvars['server'][0])
                server = '' if server == 'auto' else server

                promoter = self.postvars.get("promoter", [""])[0]
                if promoter != g.promoter:
                    res, info = proxy_session.call_api("/set_config", {
                        "account": g.config.login_account,
                        "password": g.config.login_password,
                        "promoter": promoter
                    })
                    if not res:
                        xlog.warn("set_config fail:%s", info)
                        return self.response_json({"res": "fail", "reason": info})
                    else:
                        g.promoter = promoter

                if is_server_available(server):
                    if server != g.config.server_host:
                        g.server_host = g.config.server_host = server
                        g.server_port = g.config.server_port = 443
                        g.config.save()

                        threading.Thread(target=g.session.reset).start()

                    res = {"res": "success"}
                else:
                    res = {
                        "res": "fail",
                        "reason": "server not available"
                    }
            else:
                res = {"res": "fail"}

        return self.response_json(res)

    def req_order_handler(self):
        product = self.postvars['product'][0]
        if product != 'x_tunnel':
            xlog.warn("x_tunnel order product %s not support", product)
            return self.response_json({
                "res": "fail",
                "reason": "product %s not support" % product
            })

        plan = self.postvars['plan'][0]
        if plan not in ["quarterly", "yearly"]:
            xlog.warn("x_tunnel order plan %s not support", plan)
            return self.response_json({
                "res": "fail",
                "reason": "plan %s not support" % plan
            })

        res, info = proxy_session.call_api("/order", {
            "account": g.config.login_account,
            "password": g.config.login_password,
            "product": "x_tunnel",
            "plan": plan
        })
        if not res:
            xlog.warn("order fail:%s", info)
            threading.Thread(target=proxy_session.update_quota_loop).start()
            return self.response_json({"res": "fail", "reason": info})

        self.response_json({"res": "success"})

    def req_transfer_handler(self):
        to_account = self.postvars['to_account'][0]
        amount = float(self.postvars['amount'][0])
        transfer_type = self.postvars['transfer_type'][0]
        if transfer_type == 'balance':
            if amount > g.balance:
                reason = "balance not enough"
                xlog.warn("transfer fail:%s", reason)
                return self.response_json({"res": "fail", "reason": reason})
            end_time = 0
        elif transfer_type == "quota":
            end_time = int(self.postvars['end_time'][0])
        else:
            reason = "transfer type not support:%s" % transfer_type
            xlog.warn("transfer fail:%s", reason)
            return self.response_json({"res": "fail", "reason": reason})

        req_info = {
            "account": g.config.login_account,
            "password": g.config.login_password,
            "transfer_type": transfer_type,
            "end_time": end_time,
            "to_account": to_account,
            "amount": amount
        }

        res, info = proxy_session.call_api("/transfer", req_info)
        if not res:
            xlog.warn("transfer fail:%s", info)
            return self.response_json({
                "res": "fail",
                "reason": info
            })

        self.response_json({"res": "success"})

    def req_get_history_handler(self):
        req = urlparse.urlparse(self.path).query
        reqs = urlparse.parse_qs(req, keep_blank_values=True)

        req_info = {
            "account": g.config.login_account,
            "password": g.config.login_password,
            "start": int(reqs['start'][0]),
            "end": int(reqs['end'][0]),
            "limit": int(reqs['limit'][0])
        }

        res, info = proxy_session.call_api("/get_history", req_info)
        if not res:
            xlog.warn("get history fail:%s", info)
            return self.response_json({
                "res": "fail",
                "reason": info
            })
        self.response_json({
            "res": "success",
            "history": info["history"]
        })

    def req_status(self):
        res = g.session.get_stat()

        self.response_json({
            "res": "success",
            "status": res

        })

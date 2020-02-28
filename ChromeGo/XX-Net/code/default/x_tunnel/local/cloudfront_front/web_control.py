#!/usr/bin/env python
# coding:utf-8


import os
import time
import urlparse

import simple_http_server
from front import front


current_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.abspath(os.path.join(current_path, os.pardir, os.pardir))
top_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir, os.pardir))
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
        elif path == "/ip_list":
            return self.req_ip_list_handler()
        elif path == "/debug":
            return self.req_debug_handler()
        else:
            front.logger.warn('Control Req %s %s %s ', self.address_string(), self.command, self.path)

        self.wfile.write(b'HTTP/1.1 404\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n404 Not Found')
        front.logger.info('%s "%s %s HTTP/1.1" 404 -', self.address_string(), self.command, self.path)

    def req_log_handler(self):
        req = urlparse.urlparse(self.path).query
        reqs = urlparse.parse_qs(req, keep_blank_values=True)
        data = ''

        cmd = "get_last"
        if reqs["cmd"]:
            cmd = reqs["cmd"][0]

        if cmd == "get_last":
            max_line = int(reqs["max_line"][0])
            data = front.logger.get_last_lines(max_line)
        elif cmd == "get_new":
            last_no = int(reqs["last_no"][0])
            data = front.logger.get_new_lines(last_no)
        else:
            front.logger.error('PAC %s %s %s ', self.address_string(), self.command, self.path)

        mimetype = 'text/plain'
        self.send_response_nc(mimetype, data)

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
        for ip in front.ip_manager.ip_list:
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

    def req_debug_handler(self):
        data = ""
        objs = [front.connect_manager] + front.dispatchs.values()
        for obj in objs:

            data += "%s\r\n" % obj.__class__
            for attr in dir(obj):
                if attr.startswith("__"):
                    continue
                sub_obj = getattr(obj, attr)
                if callable(sub_obj):
                    continue
                data += "    %s = %s\r\n" % (attr, sub_obj)
            if hasattr(obj, "to_string"):
                data += obj.to_string()

        mimetype = 'text/plain'
        self.send_response_nc(mimetype, data)
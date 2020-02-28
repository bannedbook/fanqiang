import os

from front_base.config import ConfigBase
import simple_http_client

current_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.abspath(os.path.join(current_path, os.pardir, os.pardir))
data_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir, 'data'))
module_data_path = os.path.join(data_path, 'gae_proxy')


headers = {"connection": "close"}
fqrouter = simple_http_client.request("GET", "http://127.0.0.1:2515/ping", headers=headers, timeout=0.5)
mobile = fqrouter and "PONG" in fqrouter.text
del headers, fqrouter

class Config(ConfigBase):
    def __init__(self, fn):
        super(Config, self).__init__(fn)

        # globa setting level
        # passive < conservative < normal < radical < extreme
        self.set_var("setting_level", "normal")

        # proxy
        self.set_var("listen_ip", "127.0.0.1")
        self.set_var("listen_port", 8087)

        # auto range
        self.set_var("AUTORANGE_THREADS", 10)
        self.set_var("AUTORANGE_MAXSIZE", 512 * 1024)
        if mobile:
            self.set_var("AUTORANGE_MAXBUFFERSIZE", 10 * 1024 * 1024 / 8)
        else:
            self.set_var("AUTORANGE_MAXBUFFERSIZE", 20 * 1024 * 1024)
        self.set_var("JS_MAXSIZE", 0)

        # gae
        self.set_var("GAE_PASSWORD", "")
        self.set_var("GAE_VALIDATE", 0)

        # host rules
        self.set_var("hosts_direct", [
            "docs.google.com",
            #"play.google.com",
            #"scholar.google.com",
            #"scholar.google.com.hk",
            "appengine.google.com"
        ])
        self.set_var("hosts_direct_endswith", [
            ".gvt1.com",
            ".appspot.com"
        ])

        self.set_var("hosts_gae", [
            "accounts.google.com",
            "mail.google.com"
        ])

        self.set_var("hosts_gae_endswith", [
            ".googleapis.com"
        ])

        # sites using br
        self.set_var("BR_SITES", [
            "webcache.googleusercontent.com",
            "www.google.com",
            "www.google.com.hk",
            "www.google.com.cn",
            "fonts.googleapis.com"
        ])

        self.set_var("BR_SITES_ENDSWITH", [
            ".youtube.com",
            ".facebook.com",
            ".googlevideo.com"
        ])

        # some unsupport request like url length > 2048, will go Direct
        self.set_var("google_endswith", [
            ".youtube.com",
            ".googleapis.com",
            ".google.com",
            ".googleusercontent.com",
            ".ytimg.com",
            ".doubleclick.net",
            ".google-analytics.com",
            ".googlegroups.com",
            ".googlesource.com",
            ".gstatic.com",
            ".appspot.com",
            ".gvt1.com",
            ".android.com",
            ".ggpht.com",
            ".googleadservices.com",
            ".googlesyndication.com",
            ".2mdn.net"
        ])

        # front
        self.set_var("front_continue_fail_num", 10)
        self.set_var("front_continue_fail_block", 0)

        # http_dispatcher
        self.set_var("dispather_min_idle_workers", 3)
        self.set_var("dispather_work_min_idle_time", 0)
        self.set_var("dispather_work_max_score", 1000)
        self.set_var("dispather_min_workers", 20)
        self.set_var("dispather_max_workers", 50)
        self.set_var("dispather_max_idle_workers", 15)

        self.set_var("max_task_num", 80)

        # http 1 worker
        self.set_var("http1_first_ping_wait", 5)
        self.set_var("http1_idle_time", 200)
        self.set_var("http1_ping_interval", 0)

        # http 2 worker
        self.set_var("http2_max_concurrent", 20)
        self.set_var("http2_target_concurrent", 1)
        self.set_var("http2_max_timeout_tasks", 1)
        self.set_var("http2_timeout_active", 0)
        self.set_var("http2_ping_min_interval", 30)

        # connect_manager
        self.set_var("https_max_connect_thread", 10)
        self.set_var("ssl_first_use_timeout", 5)
        self.set_var("connection_pool_min", 1)
        self.set_var("https_connection_pool_min", 0)
        self.set_var("https_connection_pool_max", 10)
        self.set_var("https_new_connect_num", 3)
        self.set_var("https_keep_alive", 15)

        # check_ip
        self.set_var("check_ip_host", "xxnet-1.appspot.com")
        self.set_var("check_ip_accept_status", [200, 503])
        self.set_var("check_ip_content", "GoAgent")

        # host_manager
        self.set_var("GAE_APPIDS", [])

        # connect_creator
        self.set_var("check_pkp", [
# https://pki.google.com/GIAG2.crt has expired

# https://pki.goog/gsr2/GIAG3.crt
# https://pki.goog/gsr2/GTSGIAG3.crt
b'''\
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAylJL6h7/ziRrqNpyGGjV
Vl0OSFotNQl2Ws+kyByxqf5TifutNP+IW5+75+gAAdw1c3UDrbOxuaR9KyZ5zhVA
Cu9RuJ8yjHxwhlJLFv5qJ2vmNnpiUNjfmonMCSnrTykUiIALjzgegGoYfB29lzt4
fUVJNk9BzaLgdlc8aDF5ZMlu11EeZsOiZCx5wOdlw1aEU1pDbcuaAiDS7xpp0bCd
c6LgKmBlUDHP+7MvvxGIQC61SRAPCm7cl/q/LJ8FOQtYVK8GlujFjgEWvKgaTUHF
k5GiHqGL8v7BiCRJo0dLxRMB3adXEmliK+v+IO9p+zql8H4p7u2WFvexH6DkkCXg
MwIDAQAB
-----END PUBLIC KEY-----
''',
# https://pki.goog/gsr4/GIAG3ECC.crt
b'''\
-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEG4ANKJrwlpAPXThRcA3Z4XbkwQvW
hj5J/kicXpbBQclS4uyuQ5iSOGKcuCRt8ralqREJXuRsnLZo0sIT680+VQ==
-----END PUBLIC KEY-----
''',
# https://pki.goog/gsr2/giag4.crt
b'''\
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvSw7AnhsoyYa5z/crKtt
B52X+R0ld3UdQBU4Yc/4wmF66cpHeEOMSmhdaY5RzYrowZ6kG1xXLrSoVUuudUPR
fg/zjRqv/AAVDJFqc8OnhghzaWZU9zlhtRgY4lx4Z6pDosTuR5imCcKvwqiDztOJ
r4YKHuk23p3cxu1zDnUsuN+cm4TkVtI1SsuSc9t1uErBvFIcW6v3dLcjrPkmwE61
udZQlBDHJzCFwrhXLtXLlmuSA5/9pOuWJ+U3rSgS7ICSfa83vkBe00ymjIZT6ogD
XWuFsu4edue27nG8g9gO1YozIUCV7+zExG0G5kxTovis+FJpy9hIIxSFrRIKM4DX
aQIDAQAB
-----END PUBLIC KEY-----
''',
# https://pki.goog/gsr4/giag4ecc.crt
b'''\
-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEWgDxDsTP7Od9rB8TPUltMacYCHYI
NthcDjlPu3wP0Csmy6Drit3ghqaTqFecqcgks5RwcKQkT9rbY3e8lHuuAw==
-----END PUBLIC KEY-----
''',
# https://pki.goog/gsr2/GTS1O1.crt
b'''\
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA0BjPRdSLzdOc5EDvfrTd
aSEbyc88jkx1uQ8xGYQ9njwp71ANEJNvBYCAnyqgvRJLAuE9n1gWJP4wnwt0d1WT
HUv3TeGSghD2UawMw7IilA80a5gQSecLnYM53SDGHC3v0RhhZecjgyCoIxL/0iR/
1C/nRGpbTddQZrCvnkJjBfvgHMRjYa+fajP/Ype9SNnTfBRn3HXcLmno+G14adC3
EAW48THCOyT9GjN0+CPg7GsZihbG482kzQvbs6RZYDiIO60ducaMp1Mb/LzZpKu8
3Txh15MVmO6BvY/iZEcgQAZO16yX6LnAWRKhSSUj5O1wNCyltGN8+aM9g9HNbSSs
BwIDAQAB
-----END PUBLIC KEY-----
''',
# https://pki.goog/gsr2/GTS1D2.crt
b'''\
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAstl74eHXPxyRcv/5EM2H
FXl0tz5Hi7JhVf0MNsZ+d0I6svpSWwtxgdZN1ekrJE0jXosrcl8hVbUp70TL64JS
qz4npJJJQUreqN0x4DzfbXpNLdZtCbAO42Hysv6QbFp7EGRJtAs8CPLqeQxsphqJ
alYyoCmiMIKPgVEM86K52XW5Ip4nFLpKLyxjWIfxXRDmX5G7uVvMR+IedbaMj8x1
XVcF54LGhA50cirLO1X1bnDrZmnDJLs4kzWbaGEvm9aupndyfHFIWDMQr+mAgh21
B0Ab9j3soq1HnbSUKTSzjC/NJQNYNcAlpFVf4bMHVj3I0GO4IPuMHUMs+Pmp1exv
lwIDAQAB
-----END PUBLIC KEY-----
'''
        ])
        #self.set_var("check_commonname", "Google")
        self.set_var("min_intermediate_CA", 2)
        self.set_var("support_http2", 1)

        # ip_manager
        self.set_var("max_scan_ip_thread_num", 10)
        self.set_var("max_good_ip_num", 100)
        self.set_var("target_handshake_time", 600)

        # ip source
        self.set_var("use_ipv6", "auto") #force_ipv4/force_ipv6/auto
        self.set_var("ipv6_scan_ratio", 90) # 0 - 100

        self.load()

    def load(self):
        super(Config, self).load()

        need_save = 0
        if not os.path.isfile(self.config_path):
            for fn in [
                os.path.join(module_data_path, "config.ini"),
                os.path.join(module_data_path, "manual.ini")
            ]:
                need_save += self.load_old_config(fn)

        self.HOSTS_GAE = tuple(self.hosts_gae)
        self.HOSTS_DIRECT = tuple(self.hosts_direct)
        self.HOSTS_GAE_ENDSWITH = tuple(self.hosts_gae_endswith)
        self.HOSTS_DIRECT_ENDSWITH = tuple(self.hosts_direct_endswith)
        self.GOOGLE_ENDSWITH = tuple(self.google_endswith)

        self.br_sites = tuple(self.BR_SITES)
        self.br_endswith = tuple(self.BR_SITES_ENDSWITH)

        # there are only hundreds of GAE IPs, we don't need a large threads num
        self.max_scan_ip_thread_num = min(self.max_scan_ip_thread_num, 30)

        if need_save:
            self.save()

    def load_old_config(self, fn):
        if not os.path.isfile(fn):
            return 0

        need_save = 0
        with open(fn, "r") as fd:
            for line in fd.readlines():
                if line.startswith("appid"):
                    try:
                        appid_str = line.split("=")[1]
                        appids = []
                        for appid in appid_str.split("|"):
                            appid = appid.strip()
                            appids.append(appid)
                        self.GAE_APPIDS = appids
                        need_save += 1
                    except Exception as e:
                        pass
                elif line.startswith("password"):
                    password = line.split("=")[1].strip()
                    self.GAE_PASSWORD = password
                    need_save += 1

        return need_save

    def set_level(self, level=None):
        if level is None:
            level = self.setting_level
        elif level in ["passive", "conservative", "normal", "radical", "extreme"]:
            self.setting_level = level

            if level == "passive":
                self.dispather_min_idle_workers = 0
                self.dispather_work_min_idle_time = 0
                self.dispather_work_max_score = 1000
                self.dispather_min_workers = 5
                self.dispather_max_workers = 30
                self.dispather_max_idle_workers = 5
                self.max_task_num = 50
                self.https_max_connect_thread = 10
                self.https_keep_alive = 5
                self.https_connection_pool_min = 0
                self.https_connection_pool_max = 10
                self.max_scan_ip_thread_num = 10
                self.max_good_ip_num = 60
                self.target_handshake_time = 600
            elif level == "conservative":
                self.dispather_min_idle_workers = 1
                self.dispather_work_min_idle_time = 0
                self.dispather_work_max_score = 1000
                self.dispather_min_workers = 10
                self.dispather_max_workers = 30
                self.dispather_max_idle_workers = 10
                self.max_task_num = 50
                self.https_max_connect_thread = 10
                self.https_keep_alive = 15
                self.https_connection_pool_min = 0
                self.https_connection_pool_max = 10
                self.max_scan_ip_thread_num = 10
                self.max_good_ip_num = 100
                self.target_handshake_time = 600
            elif level == "normal":
                self.dispather_min_idle_workers = 3
                self.dispather_work_min_idle_time = 0
                self.dispather_work_max_score = 1000
                self.dispather_min_workers = 20
                self.dispather_max_workers = 50
                self.dispather_max_idle_workers = 15
                self.max_task_num = 80
                self.https_max_connect_thread = 10
                self.https_keep_alive = 15
                self.https_connection_pool_min = 0
                self.https_connection_pool_max = 10
                self.max_scan_ip_thread_num = 10
                self.max_good_ip_num = 100
                self.target_handshake_time = 600
            elif level == "radical":
                self.dispather_min_idle_workers = 3
                self.dispather_work_min_idle_time = 1
                self.dispather_work_max_score = 1000
                self.dispather_min_workers = 30
                self.dispather_max_workers = 70
                self.dispather_max_idle_workers = 25
                self.max_task_num = 100
                self.https_max_connect_thread = 15
                self.https_keep_alive = 15
                self.https_connection_pool_min = 1
                self.https_connection_pool_max = 15
                self.max_scan_ip_thread_num = 20
                self.max_good_ip_num = 100
                self.target_handshake_time = 1200
            elif level == "extreme":
                self.dispather_min_idle_workers = 5
                self.dispather_work_min_idle_time = 5
                self.dispather_work_max_score = 1000
                self.dispather_min_workers = 45
                self.dispather_max_workers = 100
                self.dispather_max_idle_workers = 40
                self.max_task_num = 130
                self.https_max_connect_thread = 20
                self.https_keep_alive = 15
                self.https_connection_pool_min = 2
                self.https_connection_pool_max = 20
                self.max_scan_ip_thread_num = 30
                self.max_good_ip_num = 200
                self.target_handshake_time = 1500

            self.save()
            self.load()

class DirectConfig(object):
    def __init__(self, config):
        self._config = config
        self.set_default()

    def __getattr__(self, attr):
        return getattr(self._config, attr)

    def dummy(*args, **kwargs):
        pass

    set_var = save = load = dummy

    def set_level(self, level=None):
        if level is None:
            level = self.setting_level

        if level == "passive":
            self.dispather_min_idle_workers = 0
            self.dispather_work_min_idle_time = 0
            self.dispather_work_max_score = 1000
            self.dispather_min_workers = 0
            self.dispather_max_workers = 8
            self.dispather_max_idle_workers = 0
            self.max_task_num = 16
            self.https_max_connect_thread = 4
            self.https_connection_pool_min = 0
            self.https_connection_pool_max = 6
        elif level == "conservative":
            self.dispather_min_idle_workers = 1
            self.dispather_work_min_idle_time = 0
            self.dispather_work_max_score = 1000
            self.dispather_min_workers = 1
            self.dispather_max_workers = 8
            self.dispather_max_idle_workers = 2
            self.max_task_num = 16
            self.https_max_connect_thread = 5
            self.https_connection_pool_min = 0
            self.https_connection_pool_max = 8
        elif level == "normal":
            self.dispather_min_idle_workers = 2
            self.dispather_work_min_idle_time = 0
            self.dispather_work_max_score = 1000
            self.dispather_min_workers = 3
            self.dispather_max_workers = 8
            self.dispather_max_idle_workers = 3
            self.max_task_num = 16
            self.https_max_connect_thread = 6
            self.https_connection_pool_min = 0
            self.https_connection_pool_max = 10
        elif level == "radical":
            self.dispather_min_idle_workers = 3
            self.dispather_work_min_idle_time = 1
            self.dispather_work_max_score = 1000
            self.dispather_min_workers = 5
            self.dispather_max_workers = 10
            self.dispather_max_idle_workers = 5
            self.max_task_num = 20
            self.https_max_connect_thread = 6
            self.https_connection_pool_min = 1
            self.https_connection_pool_max = 10
        elif level == "extreme":
            self.dispather_min_idle_workers = 5
            self.dispather_work_min_idle_time = 5
            self.dispather_work_max_score = 1000
            self.dispather_min_workers = 5
            self.dispather_max_workers = 15
            self.dispather_max_idle_workers = 5
            self.max_task_num = 30
            self.https_max_connect_thread = 10
            self.https_connection_pool_min = 1
            self.https_connection_pool_max = 10

    set_default = set_level

config_path = os.path.join(module_data_path, "config.json")
config = Config(config_path)
direct_config = DirectConfig(config)


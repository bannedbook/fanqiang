
from front_base.config import ConfigBase


class Config(ConfigBase):
    def __init__(self, fn):
        super(Config, self).__init__(fn)

        # front
        self.set_var("front_continue_fail_num", 10)
        self.set_var("front_continue_fail_block", 10)

        # https_dispather
        self.set_var("dispather_min_idle_workers", 0)
        self.set_var("dispather_work_min_idle_time", 0)
        self.set_var("dispather_work_max_score", 20000)
        self.set_var("dispather_max_workers", 60)

        # ip_manager
        self.set_var("ip_source_ips", [])
        self.set_var("max_scan_ip_thread_num", 0)
        self.set_var("down_fail_connect_interval", 1)

        # connect_manager
        self.set_var("https_connection_pool_min", 0)

        # check_ip
        self.set_var("check_ip_host", "scan1.xx-net.net")
        self.set_var("check_ip_content", "X_Tunnel OK.")

        # connect_creator
        self.set_var("connect_force_http2", 1)

        self.load()
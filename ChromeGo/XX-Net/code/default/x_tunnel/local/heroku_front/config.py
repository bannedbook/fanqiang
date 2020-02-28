
from front_base.config import ConfigBase


class Config(ConfigBase):
    def __init__(self, fn):
        super(Config, self).__init__(fn)

        # front
        self.set_var("front_continue_fail_num", 10)
        self.set_var("front_continue_fail_block", 180)

        # http_dispatcher
        self.set_var("dispather_min_idle_workers", 0)
        self.set_var("dispather_work_min_idle_time", 0)
        self.set_var("dispather_work_max_score", 20000)
        self.set_var("dispather_max_workers", 60)
        self.set_var("dispather_score_factor", 0.1)

        # http1
        self.set_var("http1_first_ping_wait", 10)
        self.set_var("http1_ping_interval", 0)
        self.set_var("http1_idle_time", 50)
        self.set_var("http1_max_process_tasks", 35)

        # connect_manager
        self.set_var("connection_pool_min", 0)
        self.set_var("https_new_connect_num", 0)

        # check_ip
        self.set_var("check_ip_host", "xxnet4.herokuapp.com")
        self.set_var("check_ip_content", "We are building new site.")

        # connect_creator
        self.set_var("check_sni", "herokuapp.com")

        # host_manager
        self.set_var("appids", []) # "xxnet4.herokuapp.com"

        # ip_manager
        self.set_var("max_scan_ip_thread_num", 0)
        self.set_var("down_fail_connect_interval", 30)

        self.load()
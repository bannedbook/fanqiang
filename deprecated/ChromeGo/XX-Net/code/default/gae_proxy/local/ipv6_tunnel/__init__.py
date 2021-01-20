
import sys
import platform

from .common import test_teredo

if "arm" in platform.machine():
    from .unknown import state, state_pp, switch_pp, enable, disable, set_best_server
elif sys.platform == "win32":
    from .win10 import state, state_pp, switch_pp, enable, disable, set_best_server
elif sys.platform.startswith("linux"):
    from .linux import state, state_pp, switch_pp, enable, disable, set_best_server
elif sys.platform == "darwin":
    from .darwin import state, state_pp, switch_pp, enable, disable, set_best_server
else:
    from .unknown import state, state_pp, switch_pp, enable, disable, set_best_server

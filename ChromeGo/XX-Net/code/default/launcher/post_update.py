import os
import sys
import re
import stat
import shutil

current_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.abspath(os.path.join(current_path, os.pardir))
top_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir))

from xlog import getLogger
xlog = getLogger("launcher")
import config


def check():
    import update_from_github
    current_version = update_from_github.current_version()
    last_run_version = config.get(["modules", "launcher", "last_run_version"], "0.0.0")
    if last_run_version == "0.0.0":
        postUpdateStat = "isNew"
    elif last_run_version != current_version:
        postUpdateStat = "isPostUpdate"
        run(last_run_version)
    else:
        return
    config.set(["update", "postUpdateStat"], postUpdateStat)
    config.set(["modules", "launcher", "last_run_version"], current_version)
    config.save()


def older_or_equal(version, reference_version):
    try:
        p = re.compile(r'([0-9]+)\.([0-9]+)\.([0-9]+)')
        m1 = p.match(version)
        m2 = p.match(reference_version)
        v1 = map(int, map(m1.group, [1, 2, 3]))
        v2 = map(int, map(m2.group, [1, 2, 3]))
        return v1 <= v2
    except:
        xlog.warn("older_or_equal fail: %s, %s" % (version, reference_version))  # e.g. "get_version_fail" when post_update.run(last_run_version), "last_run_version" in \data\launcher\config.yaml
        return False  # is not older


def run(last_run_version):
    if config.get(["modules", "launcher", "auto_start"], 0):
        import autorun
        autorun.enable()

    if os.path.isdir(os.path.join(top_path, 'launcher')):
        shutil.rmtree(os.path.join(top_path, 'launcher'))  # launcher is for auto-update from 2.X

    if older_or_equal(last_run_version, '3.0.4'):
        xlog.info("migrating to 3.x.x")
        for filename in os.listdir(top_path):
            filepath = os.path.join(top_path, filename)
            if os.path.isfile(filepath):
                if sys.platform != 'win32' and filename == 'start':
                    st = os.stat(filepath)
                    os.chmod(filepath, st.st_mode | stat.S_IEXEC)
                if filename in ['start.sh', 'start.command', 'start.lnk', 'LICENSE.txt', 'download.md', 'version.txt', 'xxnet', 'xxnet.bat', 'xxnet.vbs']:
                    os.remove(filepath)
            else:
                if filename in ['goagent', 'python27', 'gae_proxy', 'php_proxy', 'x_tunnel', 'python3', 'Python3', 'lib', 'SwitchySharp']:
                    shutil.rmtree(filepath)

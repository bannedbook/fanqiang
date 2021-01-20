import os
import sys
import time
import subprocess
import threading
import re
import zipfile
import shutil
import stat
import glob

current_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.abspath(os.path.join(current_path, os.pardir))
top_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir))
code_path = os.path.abspath(os.path.join(root_path, os.pardir))
data_root = os.path.join(top_path, 'data')
python_path = os.path.join(root_path, 'python27', '1.0')
noarch_lib = os.path.join(python_path, 'lib', 'noarch')
sys.path.append(noarch_lib)

import simple_http_client
from xlog import getLogger
xlog = getLogger("launcher")
import config
import update


if not os.path.isdir(data_root):
    os.mkdir(data_root)

download_path = os.path.join(data_root, 'downloads')
if not os.path.isdir(download_path):
    os.mkdir(download_path)

progress = {}  # link => {"size", 'downloaded', status:downloading|canceled|finished:failed}
progress["update_status"] = "Idle"
update_info = "init"


def init_update_info(check_update):
    global update_info
    if check_update == "dont-check":
        update_info = "dont-check"
    elif config.get(["update", "check_update"]) == update_info == "dont-check":
        update_info = "init"
    elif check_update != "init":
        update_info = ""


init_update_info(config.get(["update", "check_update"]))


def request(url, retry=0, timeout=30):
    if retry == 0:
        if int(config.get(["proxy", "enable"], 0)):
            client = simple_http_client.Client(proxy={
                "type": config.get(["proxy", "type"], ""),
                "host": config.get(["proxy", "host"], ""),
                "port": int(config.get(["proxy", "port"], 0)),
                "user": config.get(["proxy", "user"], ""),
                "pass": config.get(["proxy", "passwd"], ""),
            }, timeout=timeout)
        else:
            client = simple_http_client.Client(timeout=timeout)
    else:
        cert = os.path.join(data_root, "gae_proxy", "CA.crt")
        client = simple_http_client.Client(proxy={
            "type": "http",
            "host": "127.0.0.1",
            "port": 8087,
            "user": None,
            "pass": None
        }, timeout=timeout, cert=cert)

    res = client.request("GET", url, read_payload=False)
    return res


def download_file(url, filename):
    if url not in progress:
        progress[url] = {}
        progress[url]["status"] = "downloading"
        progress[url]["size"] = 1
        progress[url]["downloaded"] = 0
    else:
        if progress[url]["status"] == "downloading":
            xlog.warn("url in downloading, %s", url)
            return False

    for i in range(0, 2):
        try:
            xlog.info("download %s to %s, retry:%d", url, filename, i)
            req = request(url, i, timeout=120)
            if not req:
                continue

            start_time = time.time()
            timeout = 300

            if req.chunked:
                # don't known the file size, set to large for show the progress
                progress[url]["size"] = 20 * 1024 * 1024

                downloaded = 0
                with open(filename, 'wb') as fp:
                    while True:
                        time_left = timeout - (time.time() - start_time)
                        if time_left < 0:
                            raise Exception("time out")

                        dat = req.read(timeout=time_left)
                        if not dat:
                            break

                        fp.write(dat)
                        downloaded += len(dat)
                        progress[url]["downloaded"] = downloaded

                progress[url]["status"] = "finished"
                return True
            else:
                file_size = progress[url]["size"] = int(req.getheader('Content-Length', 0))

                left = file_size
                downloaded = 0
                with open(filename, 'wb') as fp:
                    while True:
                        chunk_len = min(65536, left)
                        if not chunk_len:
                            break

                        chunk = req.read(chunk_len)
                        if not chunk:
                            break
                        fp.write(chunk)
                        downloaded += len(chunk)
                        progress[url]["downloaded"] = downloaded
                        left -= len(chunk)

            if downloaded != progress[url]["size"]:
                xlog.warn("download size:%d, need size:%d, download fail.", downloaded, progress[url]["size"])
                continue
            else:
                progress[url]["status"] = "finished"
                return True
        except Exception as e:
            xlog.warn("download %s to %s fail:%r", url, filename, e)
            continue

    progress[url]["status"] = "failed"
    return False


def parse_readme_versions(readme_file):
    versions = []
    try:
        fd = open(readme_file, "r")
        lines = fd.readlines()
        p = re.compile(r'https://codeload.github.com/XX-net/XX-Net/zip/([0-9]+)\.([0-9]+)\.([0-9]+) ([0-9a-fA-F]*)')
        for line in lines:
            m = p.match(line)
            if m:
                version = m.group(1) + "." + m.group(2) + "." + m.group(3)
                hashsum = m.group(4).lower()
                versions.append([m.group(0), version, hashsum])
                if len(versions) == 2:
                    return versions
    except Exception as e:
        xlog.exception("xxnet_version fail:%r", e)

    raise Exception("get_version_fail:%s" % readme_file)


def current_version():
    readme_file = os.path.join(root_path, "version.txt")
    try:
        with open(readme_file) as fd:
            content = fd.read()
            p = re.compile(r'([0-9]+)\.([0-9]+)\.([0-9]+)')
            m = p.match(content)
            if m:
                version = m.group(1) + "." + m.group(2) + "." + m.group(3)
                return version
    except:
        xlog.warn("get_version_fail in update_from_github")

    return "get_version_fail"


def get_github_versions():
    readme_url = "https://raw.githubusercontent.com/XX-net/XX-Net/master/code/default/update_version.txt"
    readme_target = os.path.join(download_path, "version.txt")

    if not download_file(readme_url, readme_target):
        raise IOError("get README %s fail:" % readme_url)

    versions = parse_readme_versions(readme_target)
    return versions


def get_hash_sum(version):
    versions = get_github_versions()
    for v in versions:
        if v[1] == version:
            return v[2]


def hash_file_sum(filename):
    import hashlib

    BLOCKSIZE = 65536
    hasher = hashlib.sha256()
    try:
        with open(filename, 'rb') as afile:
            buf = afile.read(BLOCKSIZE)
            while len(buf) > 0:
                hasher.update(buf)
                buf = afile.read(BLOCKSIZE)
        return hasher.hexdigest()
    except:
        return False


def overwrite(xxnet_version, xxnet_unzip_path):
    progress["update_status"] = "Overwriting"
    try:
        for root, subdirs, files in os.walk(xxnet_unzip_path):
            relate_path = root[len(xxnet_unzip_path) + 1:]
            target_relate_path = relate_path
            if sys.platform == 'win32':
                if target_relate_path.startswith("code\\default"):
                    target_relate_path = "code\\" + xxnet_version + relate_path[12:]
            else:
                if target_relate_path.startswith("code/default"):
                    target_relate_path = "code/" + xxnet_version + relate_path[12:]

            for subdir in subdirs:
                if relate_path == "code" and subdir == "default":
                    subdir = xxnet_version

                target_path = os.path.join(top_path, target_relate_path, subdir)
                if not os.path.isdir(target_path):
                    xlog.info("mkdir %s", target_path)
                    os.mkdir(target_path)

            for filename in files:
                src_file = os.path.join(root, filename)
                dst_file = os.path.join(top_path, target_relate_path, filename)
                if not os.path.isfile(dst_file) or hash_file_sum(src_file) != hash_file_sum(dst_file):
                    xlog.info("copy %s => %s", src_file, dst_file)
                    # modify by outofmemo, files in '/sdcard' are not allowed to chmod for Android
                    # and shutil.copy() will call shutil.copymode()
                    if sys.platform != 'win32' and os.path.isfile("/system/bin/dalvikvm") == False and os.path.isfile("/system/bin/dalvikvm64") == False and os.path.isfile(dst_file):
                        st = os.stat(dst_file)
                        shutil.copy(src_file, dst_file)
                        if st.st_mode & stat.S_IEXEC:
                            os.chmod(dst_file, st.st_mode)
                    else:
                        shutil.copyfile(src_file, dst_file)

    except Exception as e:
        xlog.warn("update overwrite fail:%r", e)
        progress["update_status"] = "Overwrite Fail:%r" % e
        raise e
    xlog.info("update file finished.")


def download_overwrite_new_version(xxnet_version, checkhash=1):
    global update_progress

    xxnet_url = 'https://codeload.github.com/XX-net/XX-Net/zip/%s' % xxnet_version
    xxnet_zip_file = os.path.join(download_path, "XX-Net-%s.zip" % xxnet_version)
    xxnet_unzip_path = os.path.join(download_path, "XX-Net-%s" % xxnet_version)

    progress["update_status"] = "Downloading %s" % xxnet_url
    if not download_file(xxnet_url, xxnet_zip_file):
        progress["update_status"] = "Download Fail."
        raise Exception("download xxnet zip fail:%s" % xxnet_zip_file)

    if checkhash:
        hash_sum = get_hash_sum(xxnet_version)
        if len(hash_sum) and hash_file_sum(xxnet_zip_file) != hash_sum:
            progress["update_status"] = "Download Checksum Fail."
            xlog.warn("downloaded xxnet zip checksum fail:%s" % xxnet_zip_file)
            raise Exception("downloaded xxnet zip checksum fail:%s" % xxnet_zip_file)
    else:
        xlog.debug("skip checking downloaded file hash")

    xlog.info("update download %s finished.", download_path)

    xlog.info("update start unzip")
    progress["update_status"] = "Unziping"
    try:
        with zipfile.ZipFile(xxnet_zip_file, "r") as dz:
            dz.extractall(download_path)
            dz.close()
    except Exception as e:
        xlog.warn("unzip %s fail:%r", xxnet_zip_file, e)
        progress["update_status"] = "Unzip Fail:%s" % e
        raise e
    xlog.info("update finished unzip")

    overwrite(xxnet_version, xxnet_unzip_path)

    os.remove(xxnet_zip_file)
    shutil.rmtree(xxnet_unzip_path, ignore_errors=True)


def get_local_versions():
    def get_folder_version(folder):
        f = os.path.join(code_path, folder, "version.txt")
        try:
            with open(f) as fd:
                content = fd.read()
                p = re.compile(r'([0-9]+)\.([0-9]+)\.([0-9]+)')
                m = p.match(content)
                if m:
                    version = m.group(1) + "." + m.group(2) + "." + m.group(3)
                    return version
        except:
            return False

    files_in_code_path = os.listdir(code_path)
    local_versions = []
    for name in files_in_code_path:
        if os.path.isdir(os.path.join(code_path, name)):
            v = get_folder_version(name)
            if v:
                local_versions.append([v, name])
    local_versions.sort(key=lambda s: map(int, s[0].split('.')), reverse=True)
    return local_versions


def get_current_version_dir():
    current_dir = os.path.split(root_path)[-1]
    return current_dir


def del_version(version):
    if version == get_current_version_dir():
        xlog.warn("try to delect current version.")
        return False

    try:
        shutil.rmtree(os.path.join(top_path, "code", version))
        return True
    except Exception as e:
        xlog.warn("deleting fail: %s", e)
        return False


def update_current_version(version):
    start_script = os.path.join(top_path, "code", version, "launcher", "start.py")
    if not os.path.isfile(start_script):
        xlog.warn("set version %s not exist", version)
        return False

    current_version_file = os.path.join(top_path, "code", "version.txt")
    with open(current_version_file, "w") as fd:
        fd.write(version)
    return True


def restart_xxnet(version=None):
    import module_init
    module_init.stop_all()

    import web_control
    web_control.stop()
    # New process will hold the listen port
    # We should close all listen port before create new process
    xlog.info("Close web control port.")

    if version is None:
        current_version_file = os.path.join(top_path, "code", "version.txt")
        with open(current_version_file, "r") as fd:
            version = fd.read()

    xlog.info("restart to xx-net version:%s", version)

    start_script = os.path.join(top_path, "code", version, "launcher", "start.py")
    subprocess.Popen([sys.executable, start_script])
    time.sleep(20)

    xlog.info("Exit old process...")
    os._exit(0)


def update_version(version, checkhash=1):
    global update_progress, update_info
    _update_info = update_info
    update_info = ""
    try:
        download_overwrite_new_version(version, checkhash)

        update_current_version(version)

        progress["update_status"] = "Restarting"
        xlog.info("update try restart xxnet")
        restart_xxnet(version)
    except Exception as e:
        xlog.warn("update version %s fail:%r", version, e)
        update_info = _update_info


def start_update_version(version, checkhash=1):
    if progress["update_status"] != "Idle" and "Fail" not in progress["update_status"]:
        return progress["update_status"]

    progress["update_status"] = "Start update"
    th = threading.Thread(target=update_version, args=(version, checkhash))
    th.start()
    return True


def cleanup():
    def rm_paths(path_list):
        del_fullpaths = []
        for ps in path_list:
            pt = os.path.join(top_path, ps)
            pt = glob.glob(pt)
            del_fullpaths += pt
        if del_fullpaths:
            xlog.info("DELETE: %s", ' , '.join(del_fullpaths))

            for pt in del_fullpaths:
                try:
                    if os.path.isfile(pt):
                        os.remove(pt)
                    elif os.path.isdir(pt):
                        shutil.rmtree(pt)
                except:
                    pass

    keep_old_num = config.get(["modules", "launcher", "keep_old_ver_num"], 6)  # default keep several old versions
    if keep_old_num < 99 and keep_old_num >= 0:  # 99 means don't delete any old version
        del_paths = []
        local_vs = get_local_versions()
        for i in range(len(local_vs)):
            if local_vs[i][0] == current_version():
                for u in range(i + keep_old_num + 1, len(local_vs)):
                    del_paths.append("code/" + local_vs[u][1] + "/")
                break
        if del_paths:
            rm_paths(del_paths)

    del_paths = []
    if config.get(["savedisk", "clear_cache"], 0):
        del_paths += [
            "data/*/*.*.log",
            "data/*/*.log.*",
            "data/downloads/XX-Net-*.zip"
        ]

    if config.get(["savedisk", "del_win"], 0):
        del_paths += [
            "code/*/python27/1.0/WinSxS/",
            "code/*/python27/1.0/*.dll",
            "code/*/python27/1.0/*.exe",
            "code/*/python27/1.0/Microsoft.VC90.CRT.manifest",
            "code/*/python27/1.0/lib/win32/"
        ]
    if config.get(["savedisk", "del_mac"], 0):
        del_paths += [
            "code/*/python27/1.0/lib/darwin/"
        ]
    if config.get(["savedisk", "del_linux"], 0):
        del_paths += [
            "code/*/python27/1.0/lib/linux/"
        ]
    if config.get(["savedisk", "del_gae"], 0):
        del_paths += [
            "code/*/gae_proxy/"
        ]
    if config.get(["savedisk", "del_gae_server"], 0):
        del_paths += [
            "code/*/gae_proxy/server/"
        ]
    if config.get(["savedisk", "del_xtunnel"], 0):
        del_paths += [
            "code/*/x_tunnel/"
        ]
    if config.get(["savedisk", "del_smartroute"], 0):
        del_paths += [
            "code/*/smart_router/"
        ]

    if del_paths:
        rm_paths(del_paths)

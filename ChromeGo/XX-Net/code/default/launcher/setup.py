#!/usr/bin/env python

import config
import os
import re
import shutil
import subprocess
import sys
import time
import urllib2
import zipfile

from xlog import getLogger
xlog = getLogger("launcher")

current_path = os.path.dirname(os.path.abspath(__file__))
python_path = os.path.abspath(os.path.join(current_path, os.pardir, 'python27', '1.0'))
noarch_lib = os.path.abspath(os.path.join(python_path, 'lib', 'noarch'))
sys.path.append(noarch_lib)

opener = urllib2.build_opener()

root_path = os.path.abspath(os.path.join(current_path, os.pardir))
download_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir, 'data', 'downloads'))

xxnet_unzip_path = ""


def get_XXNet():
    global xxnet_unzip_path

    def download_file(url, file):
        try:
            xlog.info("download %s to %s", url, file)
            req = opener.open(url)
            CHUNK = 16 * 1024
            with open(file, 'wb') as fp:
                while True:
                    chunk = req.read(CHUNK)
                    if not chunk:
                        break
                    fp.write(chunk)
            return True
        except:
            xlog.info("download %s to %s fail", url, file)
            return False

    def get_xxnet_url_version(readme_file):

        try:
            fd = open(readme_file, "r")
            lines = fd.readlines()
            p = re.compile(r'https://codeload.github.com/XX-net/XX-Net/zip/([0-9]+)\.([0-9]+)\.([0-9]+)')
            for line in lines:
                m = p.match(line)
                if m:
                    version = m.group(1) + "." + m.group(2) + "." + m.group(3)
                    return m.group(0), version
        except Exception as e:
            xlog.exception("xxnet_version fail:%s", e)
        raise "get_version_fail:" % readme_file

    readme_url = "https://raw.githubusercontent.com/XX-net/XX-Net/master/README.md"
    readme_targe = os.path.join(download_path, "README.md")
    if not os.path.isdir(download_path):
        os.mkdir(download_path)
    if not download_file(readme_url, readme_targe):
        raise "get README fail:" % readme_url
    xxnet_url, xxnet_version = get_xxnet_url_version(readme_targe)
    xxnet_unzip_path = os.path.join(download_path, "XX-Net-%s" % xxnet_version)
    xxnet_zip_file = os.path.join(download_path, "XX-Net-%s.zip" % xxnet_version)

    if not download_file(xxnet_url, xxnet_zip_file):
        raise "download xxnet zip fail:" % download_path

    with zipfile.ZipFile(xxnet_zip_file, "r") as dz:
        dz.extractall(download_path)
        dz.close()


def get_new_new_config():
    global xxnet_unzip_path
    import yaml
    data_path = os.path.abspath(os.path.join(xxnet_unzip_path, 'data',
                                             'launcher', 'config.yaml'))
    try:
        new_config = yaml.load(open(data_path, 'r'))
        return new_config
    except yaml.YAMLError as exc:
        print("Error in configuration file:", exc)


def process_data_files():
    # TODO: fix bug
    # new_config = get_new_new_config()
    # config.load()
    # config.config["modules"]["gae_proxy"]["current_version"] = new_config["modules"]["gae_proxy"]["current_version"]
    # config.config["modules"]["launcher"]["current_version"] = new_config["modules"]["launcher"]["current_version"]
    config.save()


def install_xxnet_files():
    def sha1_file(filename):
        import hashlib

        BLOCKSIZE = 65536
        hasher = hashlib.sha1()
        try:
            with open(filename, 'rb') as afile:
                buf = afile.read(BLOCKSIZE)
                while len(buf) > 0:
                    hasher.update(buf)
                    buf = afile.read(BLOCKSIZE)
            return hasher.hexdigest()
        except:
            return False
        pass

    for root, subdirs, files in os.walk(xxnet_unzip_path):
        # print("root:", root)
        relate_path = root[len(xxnet_unzip_path) + 1:]
        for subdir in subdirs:

            target_path = os.path.join(root_path, relate_path, subdir)
            if not os.path.isdir(target_path):
                xlog.info("mkdir %s", target_path)
                os.mkdir(target_path)

        for filename in files:
            if relate_path == os.path.join("data", "gae_proxy") and filename == "config.ini":
                continue
            if relate_path == os.path.join("data", "launcher") and filename == "config.yaml":
                continue
            src_file = os.path.join(root, filename)
            dst_file = os.path.join(root_path, relate_path, filename)
            if not os.path.isfile(dst_file) or sha1_file(src_file) != sha1_file(dst_file):
                shutil.copy(src_file, dst_file)
                xlog.info("copy %s => %s", src_file, dst_file)


def update_environment():
    get_XXNet()
    process_data_files()
    install_xxnet_files()


def wait_xxnet_exit():
    def http_request(url, method="GET"):
        proxy_handler = urllib2.ProxyHandler({})
        opener = urllib2.build_opener(proxy_handler)
        try:
            req = opener.open(url)
            return req
        except Exception as e:
            # logging.exception("web_control http_request:%s fail:%s", url, e)
            return False

    for i in range(20):
        host_port = config.get(["modules", "launcher", "control_port"], 8085)
        req_url = "http://127.0.0.1:{port}/quit".format(port=host_port)
        if http_request(req_url) is False:
            return True
        time.sleep(1)
    return False


def run_new_start_script():
    current_path = os.path.dirname(os.path.abspath(__file__))
    start_sript = os.path.abspath(os.path.join(current_path, "start.py"))
    subprocess.Popen([sys.executable, start_sript], shell=False)


def main():
    wait_xxnet_exit()
    update_environment()

    time.sleep(2)
    xlog.info("setup start run new launcher")
    run_new_start_script()


if __name__ == "__main__":
    main()

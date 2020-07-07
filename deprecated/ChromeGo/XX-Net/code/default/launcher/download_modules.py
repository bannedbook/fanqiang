import os
import threading
import time
import zipfile
import shutil
import platform

from update_from_github import request, xlog

current_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.abspath(os.path.join(current_path, os.pardir))
top_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir))


def download_file(url, filename):
    org_url = url
    if os.path.isfile(filename):
        return True

    for i in range(0, 4):
        try:
            xlog.info("download %s to %s, retry:%d", url, filename, i)
            req = request(url, i, timeout=120)
            if not req:
                time.sleep(60)
                continue

            if req.status == 302:
                url = req.headers["Location"]
                continue

            start_time = time.time()
            timeout = 300

            if req.chunked:

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

                return True
            else:
                file_size = int(req.getheader('Content-Length', 0))

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
                        left -= len(chunk)

            if downloaded != file_size:
                xlog.warn("download size:%d, need size:%d, download fail.", downloaded, file_size)
                os.remove(filename)
                continue
            else:
                xlog.info("download %s to %s success.", org_url, filename)
                return True
        except Exception as e:
            xlog.warn("download %s to %s fail:%r", org_url, filename, e)
            continue
    xlog.warn("download %s fail", org_url)


def download_unzip(url, extract_path):
    if os.path.isdir(extract_path):
        return True

    data_root = os.path.join(top_path, 'data')
    download_path = os.path.join(data_root, 'downloads')
    if not os.path.isdir(download_path):
        os.mkdir(download_path)

    fn = url.split("/")[-1]
    dfn = os.path.join(download_path, fn)

    if not download_file(url, dfn):
        xlog.warn("download file %s fail.", url)
        return

    try:
        os.mkdir(extract_path)
        with zipfile.ZipFile(dfn, "r") as dz:
            dz.extractall(extract_path)
            dz.close()
        xlog.info("Extract %s to %s success.", fn, extract_path)
    except Exception as e:
        xlog.warn("unzip %s fail:%r", dfn, e)
        shutil.rmtree(extract_path)
        raise e
    os.remove(dfn)


def download_worker():
    if not ("arm" in platform.machine() or "mips" in platform.machine() or "aarch64" in platform.machine()):
        # If not Andorid/IOS/Router, download browser plugin
        switchyomega_path = os.path.join(top_path, "SwitchyOmega")
        download_file("https://github.com/XX-net/XX-Net/releases/download/3.15.0/SwitchyOmega.crx", os.path.join(switchyomega_path, "SwitchyOmega.crx"))
        download_file("https://github.com/XX-net/XX-Net/releases/download/3.15.0/AutoProxy.xpi", os.path.join(switchyomega_path, "AutoProxy.xpi"))


def start_download():
    time.sleep(10)
    th = threading.Thread(target=download_worker)
    th.start()
    return True


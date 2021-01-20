import os
import sys
import urllib2
import zipfile
import shutil


from xlog import getLogger
xlog = getLogger("launcher")


current_path = os.path.dirname(os.path.abspath(__file__))
root_path = os.path.abspath(os.path.join(current_path, os.pardir, os.pardir, os.pardir))
data_path = os.path.join(root_path, 'data')
python27_path = os.path.join(root_path, "python27")
python27_2_path = os.path.join(root_path, "python27", "2.0")
py27lib = os.path.join(python27_2_path, "lib")

# check python2 env.
# if not exit, download using urllib
# unzip python2
# add path.

def exec_check():
    if not sys.platform == "win32":
        return

    if not os.path.isdir(python27_2_path):
        download_unzip_python_env()

    if not py27lib in sys.path:
        sys.path.append(py27lib)


def download_unzip_python_env():
    if not os.path.isdir(data_path):
        os.mkdir(data_path)

    download_path = os.path.join(data_path, "download")
    if not os.path.isdir(download_path):
        os.mkdir(download_path)

    py_fp = os.path.join(download_path, "py27.zip")
    if os.path.isfile(py_fp):
        os.remove(py_fp)

    url = "https://raw.githubusercontent.com/XX-net/XX-Net-dev/master/download/py27.zip"
    xlog.info("downloading %s to %s", url, py_fp)

    # Ignore system proxy setting
    proxy_handler = urllib2.ProxyHandler({})
    opener = urllib2.build_opener(proxy_handler)
    filedata = opener.open(url)
    datatowrite = filedata.read()

    with open(py_fp, 'wb') as f:
        f.write(datatowrite)
    xlog.info("download %s to %s finished.", url, py_fp)

    unzip(py_fp, download_path)


    if not os.path.isdir(python27_path):
        os.mkdir(python27_path)
    src = os.path.join(download_path, "py27")
    dst = python27_2_path
    shutil.move(src, dst)


def unzip(zip_fp, dst_path):
    xlog.info("unzipping %s to %s", zip_fp, dst_path)
    try:
        with zipfile.ZipFile(zip_fp, "r") as dz:
            dz.extractall(dst_path)
            dz.close()
    except Exception as e:
        xlog.warn("unzip %s fail:%r", zip_fp, e)
        raise e

    xlog.info("unzip %s to %s success", zip_fp, dst_path)
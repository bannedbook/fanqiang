#!/usr/bin/env python
# coding:utf-8

import os
import sys
import glob
import binascii
import time
import random
import base64
import hashlib
import threading
import subprocess
import datetime

current_path = os.path.dirname(os.path.abspath(__file__))
python_path = os.path.abspath( os.path.join(current_path, os.pardir, os.pardir, 'python27', '1.0'))
root_path = os.path.abspath(os.path.join(current_path, os.pardir, os.pardir))
data_path = os.path.abspath(os.path.join(root_path, os.pardir, os.pardir, 'data', "gae_proxy"))
if not os.path.isdir(data_path):
    data_path = current_path

if __name__ == "__main__":
    noarch_lib = os.path.abspath( os.path.join(python_path, 'lib', 'noarch'))
    sys.path.append(noarch_lib)

    if sys.platform == "win32":
        win32_lib = os.path.abspath( os.path.join(python_path, 'lib', 'win32'))
        sys.path.append(win32_lib)
    elif sys.platform == "linux" or sys.platform == "linux2":
        linux_lib = os.path.abspath( os.path.join(python_path, 'lib', 'linux'))
        sys.path.append(linux_lib)
    elif sys.platform == "darwin":
        darwin_lib = os.path.abspath( os.path.join(python_path, 'lib', 'darwin'))
        sys.path.append(darwin_lib)

from xlog import getLogger
xlog = getLogger("gae_proxy")

import OpenSSL
from utils import check_ip_valid


def get_cmd_out(cmd):
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out = proc.stdout
    lines = out.readlines()
    return lines


class CertUtil(object):
    """CertUtil module, based on mitmproxy"""

    ca_vendor = 'GoAgent' #TODO: here should be XX-Net
    ca_certfile = os.path.join(data_path, 'CA.crt')
    ca_keyfile = os.path.join(data_path, 'CAkey.pem')
    ca_thumbprint = ''
    ca_privatekey = None
    ca_subject = None
    ca_certdir = os.path.join(data_path, 'certs')
    ca_digest = 'sha256'
    ca_lock = threading.Lock()
    ca_validity_years = 10
    ca_validity = 24 * 60 * 60 * 365 * ca_validity_years
    cert_validity_years = 2
    cert_validity = 24 * 60 * 60 * 365 * cert_validity_years
    cert_publickey = None
    cert_keyfile = os.path.join(data_path, 'Certkey.pem')
    serial_reduce =  3600 * 24 * 365 * 46

    @staticmethod
    def create_ca():
        key = OpenSSL.crypto.PKey()
        key.generate_key(OpenSSL.crypto.TYPE_RSA, 2048)
        ca = OpenSSL.crypto.X509()
        ca.set_version(2)
        ca.set_serial_number(0)
        subj = ca.get_subject()
        subj.countryName = 'CN'
        subj.stateOrProvinceName = 'Internet'
        subj.localityName = 'Cernet'
        subj.organizationName = CertUtil.ca_vendor
        # Log generated time.
        subj.organizationalUnitName = '%s Root - %d' % (CertUtil.ca_vendor, int(time.time()))
        subj.commonName = '%s XX-Net' % CertUtil.ca_vendor
        ca.gmtime_adj_notBefore(- 3600 * 24)
        ca.gmtime_adj_notAfter(CertUtil.ca_validity - 3600 * 24)
        ca.set_issuer(subj)
        ca.set_subject(subj)
        ca.set_pubkey(key)
        ca.add_extensions([
            OpenSSL.crypto.X509Extension(
                'basicConstraints', False, 'CA:TRUE', subject=ca, issuer=ca)
            ])
        ca.sign(key, CertUtil.ca_digest)
        #xlog.debug("CA key:%s", key)
        xlog.info("create CA")
        return key, ca

    @staticmethod
    def generate_ca_file():
        xlog.info("generate CA file:%s", CertUtil.ca_keyfile)
        key, ca = CertUtil.create_ca()
        with open(CertUtil.ca_certfile, 'wb') as fp:
            fp.write(OpenSSL.crypto.dump_certificate(OpenSSL.crypto.FILETYPE_PEM, ca))
        with open(CertUtil.ca_keyfile, 'wb') as fp:
            fp.write(OpenSSL.crypto.dump_certificate(OpenSSL.crypto.FILETYPE_PEM, ca))
            fp.write(OpenSSL.crypto.dump_privatekey(OpenSSL.crypto.FILETYPE_PEM, key))

    @staticmethod
    def generate_cert_keyfile():
        xlog.info("generate certs's key file:%s", CertUtil.cert_keyfile)
        pkey = OpenSSL.crypto.PKey()
        pkey.generate_key(OpenSSL.crypto.TYPE_RSA, 2048)
        with open(CertUtil.cert_keyfile, 'wb') as fp:
            fp.write(OpenSSL.crypto.dump_privatekey(OpenSSL.crypto.FILETYPE_PEM, pkey))
            fp.write(OpenSSL.crypto.dump_publickey(OpenSSL.crypto.FILETYPE_PEM, pkey))
        CertUtil.cert_publickey = pkey

    @staticmethod
    def _get_cert(commonname, isip=False, sans=None):
        cert = OpenSSL.crypto.X509()
        cert.set_version(2)
        # setting the only serial number, the browser will refused fixed serial number when cert updated.
        serial_number = int((int(time.time() - CertUtil.serial_reduce) + random.random()) * 100)
        while 1:
            try:
                cert.set_serial_number(serial_number)
            except OpenSSL.SSL.Error:
                serial_number += 1
            else:
                break
        subj = cert.get_subject()
        subj.countryName = 'CN'
        subj.stateOrProvinceName = 'Internet'
        subj.localityName = 'Cernet'
        subj.organizationalUnitName = '%s Branch' % CertUtil.ca_vendor
        subj.commonName = commonname
        subj.organizationName = commonname
        cert.gmtime_adj_notBefore(-600) #avoid crt time error warning
        cert.gmtime_adj_notAfter(CertUtil.cert_validity)
        cert.set_issuer(CertUtil.ca_subject)
        if CertUtil.cert_publickey:
            pkey = CertUtil.cert_publickey
        else:
            pkey = OpenSSL.crypto.PKey()
            pkey.generate_key(OpenSSL.crypto.TYPE_RSA, 2048)
        cert.set_pubkey(pkey)

        sans = set(sans) if sans else set()
        sans.add(commonname)
        if isip:
            sans = 'IP: ' + commonname
        else:
            sans = 'DNS: %s, DNS: *.%s' % (commonname,  commonname)
        cert.add_extensions([OpenSSL.crypto.X509Extension(b'subjectAltName', True, sans)])

        cert.sign(CertUtil.ca_privatekey, CertUtil.ca_digest)

        certfile = os.path.join(CertUtil.ca_certdir, commonname + '.crt')
        with open(certfile, 'wb') as fp:
            fp.write(OpenSSL.crypto.dump_certificate(OpenSSL.crypto.FILETYPE_PEM, cert))
            if CertUtil.cert_publickey is None:
                fp.write(OpenSSL.crypto.dump_privatekey(OpenSSL.crypto.FILETYPE_PEM, pkey))
        return certfile

    @staticmethod
    def _get_old_cert(commonname):
        certfile = os.path.join(CertUtil.ca_certdir, commonname + '.crt')
        if os.path.exists(certfile):
            with open(certfile, 'rb') as fp:
                cert = OpenSSL.crypto.load_certificate(OpenSSL.crypto.FILETYPE_PEM, fp.read())
            if datetime.datetime.strptime(cert.get_notAfter(), '%Y%m%d%H%M%SZ') < datetime.datetime.utcnow() + datetime.timedelta(days=30):
                try:
                    os.remove(certfile)
                except OSError as e:
                    xlog.warning('CertUtil._get_old_cert failed: unable to remove outdated cert, %r', e)
                else:
                    return
                # well, have to use the old one
            return certfile

    @staticmethod
    def get_cert(commonname, sans=None, full_name=False):
        isip =  check_ip_valid(commonname)
        with CertUtil.ca_lock:
            certfile = CertUtil._get_old_cert(commonname)
            if certfile:
                return certfile

            # some site need full name cert
            # like https://about.twitter.com in Google Chrome
            if not isip and not full_name and commonname.count('.') >= 2 and [len(x) for x in reversed(commonname.split('.'))] > [2, 4]:
                commonname = commonname.partition('.')[-1]
                certfile = CertUtil._get_old_cert(commonname)
                if certfile:
                    return certfile

            return CertUtil._get_cert(commonname, isip, sans)

    @staticmethod
    def win32_notify( msg="msg", title="Title"):
        import ctypes
        res = ctypes.windll.user32.MessageBoxW(None, msg, title, 1)
        # Yes:1 No:2
        return res

    @staticmethod
    def import_windows_ca(certfile):
        xlog.debug("Begin to import Windows CA")
        with open(certfile, 'rb') as fp:
            certdata = fp.read()
            if certdata.startswith(b'-----'):
                begin = b'-----BEGIN CERTIFICATE-----'
                end = b'-----END CERTIFICATE-----'
                certdata = base64.b64decode(b''.join(certdata[certdata.find(begin)+len(begin):certdata.find(end)].strip().splitlines()))
        try:
            common_name = OpenSSL.crypto.load_certificate(OpenSSL.crypto.FILETYPE_ASN1, certdata).get_subject().CN
        except Exception as e:
            logging.error('load_certificate(certfile=%r) 失败：%s', certfile, e)
            return -1

        assert certdata, 'cert file %r is broken' % certfile
        import ctypes
        import ctypes.wintypes
        class CERT_CONTEXT(ctypes.Structure):
            _fields_ = [
                ('dwCertEncodingType', ctypes.wintypes.DWORD),
                ('pbCertEncoded', ctypes.POINTER(ctypes.wintypes.BYTE)),
                ('cbCertEncoded', ctypes.wintypes.DWORD),
                ('pCertInfo', ctypes.c_void_p),
                ('hCertStore', ctypes.c_void_p),]
        X509_ASN_ENCODING = 0x1
        CERT_STORE_ADD_ALWAYS = 4
        CERT_STORE_PROV_SYSTEM = 10
        CERT_STORE_OPEN_EXISTING_FLAG = 0x4000
        CERT_SYSTEM_STORE_CURRENT_USER = 1 << 16
        CERT_SYSTEM_STORE_LOCAL_MACHINE = 2 << 16
        CERT_FIND_SUBJECT_STR = 8 << 16 | 7
        crypt32 = ctypes.windll.crypt32
        ca_exists = False
        store_handle = None
        pCertCtx = None
        ret = 0
        for store in (CERT_SYSTEM_STORE_LOCAL_MACHINE, CERT_SYSTEM_STORE_CURRENT_USER):
            try:
                store_handle = crypt32.CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, None, CERT_STORE_OPEN_EXISTING_FLAG | store, u'root')
                if not store_handle:
                    if store == CERT_SYSTEM_STORE_CURRENT_USER and not ca_exists:
                        xlog.warning('CertUtil.import_windows_ca failed: could not open system cert store')
                        return False
                    else:
                        continue

                pCertCtx = crypt32.CertFindCertificateInStore(store_handle, X509_ASN_ENCODING, 0, CERT_FIND_SUBJECT_STR, common_name, None)
                while pCertCtx:
                    certCtx = CERT_CONTEXT.from_address(pCertCtx)
                    _certdata = ctypes.string_at(certCtx.pbCertEncoded, certCtx.cbCertEncoded)
                    if _certdata == certdata:
                        ca_exists = True
                        xlog.debug("XX-Net CA already exists")
                    else:
                        cert =  OpenSSL.crypto.load_certificate(OpenSSL.crypto.FILETYPE_ASN1, _certdata)
                        if hasattr(cert, 'get_subject'):
                             cert = cert.get_subject()
                        cert_name = next((v for k, v in cert.get_components() if k == 'CN'), '')
                        if cert_name == common_name:
                            ret = crypt32.CertDeleteCertificateFromStore(crypt32.CertDuplicateCertificateContext(pCertCtx))
                            if ret == 1:
                                xlog.debug("Invalid Windows CA %r has been removed", common_name)
                            elif ret == 0 and store == CERT_SYSTEM_STORE_LOCAL_MACHINE:
                                # to elevate
                                break
                    pCertCtx = crypt32.CertFindCertificateInStore(store_handle, X509_ASN_ENCODING, 0, CERT_FIND_SUBJECT_STR, common_name, pCertCtx)

                # Only add to current user
                if store == CERT_SYSTEM_STORE_CURRENT_USER and not ca_exists:
                    ret = crypt32.CertAddEncodedCertificateToStore(store_handle, X509_ASN_ENCODING, certdata, len(certdata), CERT_STORE_ADD_ALWAYS, None)
            except Exception as e:
                xlog.warning('CertUtil.import_windows_ca failed: %r', e)
                if isinstance(e, OSError):
                    store_handle = None
                    continue
                return False
            finally:
                if pCertCtx:
                    crypt32.CertFreeCertificateContext(pCertCtx)
                    pCertCtx = None
                if store_handle:
                    crypt32.CertCloseStore(store_handle, 0)
                    store_handle = None

        if ca_exists:
            return True

        if ret == 0 and __name__ != "__main__":
            #res = CertUtil.win32_notify(msg=u'Import GoAgent Ca?', title=u'Authority need')
            #if res == 2:
            #    return -1

            import win32elevate
            try:
                win32elevate.elevateAdminRun(os.path.abspath(__file__))
            except Exception as e:
                xlog.warning('CertUtil.import_windows_ca failed: %r', e)
            return True
        elif ret == 1:
            CertUtil.win32_notify(msg=u'已经导入GoAgent证书，请重启浏览器.', title=u'Restart browser need.')

        return ret == 1

    @staticmethod
    def get_linux_firefox_path():
        home_path = os.path.expanduser("~")
        firefox_path = os.path.join(home_path, ".mozilla/firefox")
        if not os.path.isdir(firefox_path):
            return

        for filename in os.listdir(firefox_path):
            if filename.endswith(".default") and os.path.isdir(os.path.join(firefox_path, filename)):
                config_path = os.path.join(firefox_path, filename)
                #xlog.debug("Got Firefox path: %s", config_path)
                return config_path

    @staticmethod
    def import_linux_firefox_ca(common_name, ca_file):
        xlog.debug("Begin importing CA to Firefox")
        firefox_config_path = CertUtil.get_linux_firefox_path()
        if not firefox_config_path:
            #xlog.debug("Not found Firefox path")
            return False

        if not any(os.path.isfile('%s/certutil' % x) for x in os.environ['PATH'].split(os.pathsep)):
            xlog.warn('please install *libnss3-tools* package to import GoAgent root ca')
            return False

        xlog.info("Removing old cert to Firefox in %s", firefox_config_path)
        cmd_line = 'certutil -L -d %s |grep "GoAgent" &&certutil -d %s -D -n "%s" ' % (firefox_config_path, firefox_config_path, common_name)
        os.system(cmd_line) # remove old cert first

        xlog.info("Add new cert to Firefox in %s", firefox_config_path)
        cmd_line = 'certutil -d %s -A -t "C,," -n "%s" -i "%s"' % (firefox_config_path, common_name, ca_file)
        os.system(cmd_line) # install new cert
        return True

    @staticmethod
    def import_linux_ca(common_name, ca_file):

        def get_linux_ca_sha1(nss_path):
            commonname = "GoAgent XX-Net - GoAgent" #TODO: here should be GoAgent - XX-Net

            cmd = ['certutil', '-L','-d', 'sql:%s' % nss_path, '-n', commonname]
            lines = get_cmd_out(cmd)

            get_sha1_title = False
            sha1 = ""
            for line in lines:
                if line.endswith("Fingerprint (SHA1):\n"):
                    get_sha1_title = True
                    continue
                if get_sha1_title:
                    sha1 = line
                    break

            sha1 = sha1.replace(' ', '').replace(':', '').replace('\n', '')
            if len(sha1) != 40:
                return False
            else:
                return sha1

        home_path = os.path.expanduser("~")
        nss_path = os.path.join(home_path, ".pki/nssdb")
        if not os.path.isdir(nss_path):
            return False

        if not any(os.path.isfile('%s/certutil' % x) for x in os.environ['PATH'].split(os.pathsep)):
            xlog.info('please install *libnss3-tools* package to import GoAgent root ca')
            return False

        sha1 = get_linux_ca_sha1(nss_path)
        ca_hash = CertUtil.ca_thumbprint.replace(':', '')
        if sha1 == ca_hash:
            xlog.info("Database $HOME/.pki/nssdb cert exist")
            return

        # shell command to list all cert
        # certutil -L -d sql:$HOME/.pki/nssdb

        # remove old cert first
        xlog.info("Removing old cert in database $HOME/.pki/nssdb")
        cmd_line = 'certutil -L -d sql:$HOME/.pki/nssdb |grep "GoAgent" && certutil -d sql:$HOME/.pki/nssdb -D -n "%s" ' % ( common_name)
        os.system(cmd_line)

        # install new cert
        xlog.info("Add cert to database $HOME/.pki/nssdb")
        cmd_line = 'certutil -d sql:$HOME/.pki/nssdb -A -t "C,," -n "%s" -i "%s"' % (common_name, ca_file)
        os.system(cmd_line)
        return True

    @staticmethod
    def import_ubuntu_system_ca(common_name, certfile):
        import platform
        platform_distname = platform.dist()[0]
        if platform_distname != 'Ubuntu':
            return

        pemfile = "/etc/ssl/certs/CA.pem"
        new_certfile = "/usr/local/share/ca-certificates/CA.crt"
        if not os.path.exists(pemfile) or not CertUtil.file_is_same(certfile, new_certfile):
            if os.system('cp "%s" "%s" && update-ca-certificates' % (certfile, new_certfile)) != 0:
                xlog.warning('install root certificate failed, Please run as administrator/root/sudo')

    @staticmethod
    def file_is_same(file1, file2):
        BLOCKSIZE = 65536

        try:
            with open(file1, 'rb') as f1:
                buf1 = f1.read(BLOCKSIZE)
        except:
            return False

        try:
            with open(file2, 'rb') as f2:
                buf2 = f2.read(BLOCKSIZE)
        except:
            return False

        if buf1 != buf2:
            return False
        else:
            return True

    @staticmethod
    def import_mac_ca(common_name, certfile):
        commonname = "GoAgent XX-Net" #TODO: need check again
        ca_hash = CertUtil.ca_thumbprint.replace(':', '')

        def get_exist_ca_sha1():
            args = ['security', 'find-certificate', '-Z', '-a', '-c', commonname]
            output = subprocess.check_output(args)
            for line in output.splitlines(True):
                if len(line) == 53 and line.startswith("SHA-1 hash:"):
                    sha1_hash = line[12:52]
                    return sha1_hash

        exist_ca_sha1 = get_exist_ca_sha1()
        if exist_ca_sha1 == ca_hash:
            xlog.info("GoAgent CA exist")
            return

        import_command = 'security add-trusted-cert -d -r trustRoot -k /Library/Keychains/System.keychain ../../../../data/gae_proxy/CA.crt'# % certfile.decode('utf-8')
        if exist_ca_sha1:
            delete_ca_command = 'security delete-certificate -Z %s' % exist_ca_sha1
            exec_command = "%s;%s" % (delete_ca_command, import_command)
        else:
            exec_command = import_command

        admin_command = """osascript -e 'do shell script "%s" with administrator privileges' """ % exec_command
        cmd = admin_command.encode('utf-8')
        xlog.info("try auto import CA command:%s", cmd)
        os.system(cmd)

    @staticmethod
    def import_ca(certfile):
        xlog.debug("Importing CA")
        commonname = "GoAgent XX-Net - GoAgent" #TODO: here should be GoAgent - XX-Net
        if sys.platform.startswith('win'):
            CertUtil.import_windows_ca(certfile)
        elif sys.platform == 'darwin':
            CertUtil.import_mac_ca(commonname, certfile)
        elif sys.platform.startswith('linux'):
            CertUtil.import_linux_ca(commonname, certfile)
            CertUtil.import_linux_firefox_ca(commonname, certfile)
            #CertUtil.import_ubuntu_system_ca(commonname, certfile) # we don't need install CA to system root, special user is enough

    @staticmethod
    def verify_certificate(ca, cert):
        if hasattr(OpenSSL.crypto, "X509StoreContext"):
            store = OpenSSL.crypto.X509Store()
            store.add_cert(ca)
            try:
                OpenSSL.crypto.X509StoreContext(store, cert).verify_certificate()
            except:
                return False
            else:
                return True
        else:
            # A fake verify, just check generated time.
            return ca.get_subject().OU == cert.get_issuer().OU


    @staticmethod
    def init_ca():
        #xlog.debug("Initializing CA")

        #Check Certs Dir
        if not os.path.exists(CertUtil.ca_certdir):
            os.makedirs(CertUtil.ca_certdir)

        # Confirmed GoAgent CA exist
        if not os.path.exists(CertUtil.ca_keyfile):
            if os.path.exists(CertUtil.ca_certfile):
                # update old unsafe CA file
                xlog.info("update CA file storage format")
                if hasattr(OpenSSL.crypto, "X509StoreContext"):
                    os.rename(CertUtil.ca_certfile, CertUtil.ca_keyfile)
                else:
                    xlog.warning("users may need to re-import CA file")
                    CertUtil.generate_ca_file()
            else:
                xlog.info("no GAE CA file exist in XX-Net data dir")

                xlog.info("clean old site certs in XX-Net cert dir")
                any(os.remove(x) for x in glob.glob(os.path.join(CertUtil.ca_certdir, '*.crt')) + glob.glob(os.path.join(CertUtil.ca_certdir, '.*.crt')))

                CertUtil.generate_ca_file()

        # Load GoAgent CA
        with open(CertUtil.ca_keyfile, 'rb') as fp:
            content = fp.read()
        ca = OpenSSL.crypto.load_certificate(OpenSSL.crypto.FILETYPE_PEM, content)
        CertUtil.ca_privatekey = OpenSSL.crypto.load_privatekey(OpenSSL.crypto.FILETYPE_PEM, content)
        CertUtil.ca_thumbprint = ca.digest('sha1')
        CertUtil.ca_subject = ca.get_subject()
        ca_cert_error = True
        if os.path.exists(CertUtil.ca_certfile):
            with open(CertUtil.ca_certfile, 'rb') as fp:
                ca_cert_error = fp.read() not in content
        if ca_cert_error:
            with open(CertUtil.ca_certfile, 'wb') as fp:
                fp.write(OpenSSL.crypto.dump_certificate(OpenSSL.crypto.FILETYPE_PEM, ca))

        # Check cert keyfile exists
        if hasattr(OpenSSL.crypto, "load_publickey"):
            if os.path.exists(CertUtil.cert_keyfile):
                with open(CertUtil.cert_keyfile, 'rb') as fp:
                    CertUtil.cert_publickey = OpenSSL.crypto.load_publickey(OpenSSL.crypto.FILETYPE_PEM, fp.read())
            else:
                CertUtil.generate_cert_keyfile()
        else:
            CertUtil.cert_keyfile = None

        # Check exist site cert buffer with CA
        certfiles = glob.glob(os.path.join(CertUtil.ca_certdir, '*.crt')) + glob.glob(os.path.join(CertUtil.ca_certdir, '.*.crt'))
        if certfiles:
            filename = random.choice(certfiles)
            with open(filename, 'rb') as fp:
                cert = OpenSSL.crypto.load_certificate(OpenSSL.crypto.FILETYPE_PEM, fp.read())
            remove_certs = False
            if not CertUtil.verify_certificate(ca, cert):
                remove_certs = True
            if not remove_certs and CertUtil.cert_publickey:
                context = OpenSSL.SSL.Context(OpenSSL.SSL.TLSv1_METHOD)
                try:
                    context.use_certificate(cert)
                    context.use_privatekey_file(CertUtil.cert_keyfile)
                except OpenSSL.SSL.Error:
                    remove_certs = True
            if remove_certs:
                xlog.info("clean old site certs in XX-Net cert dir")
                any(os.remove(x) for x in certfiles)

        if os.getenv("XXNET_NO_MESS_SYSTEM", "0") == "0" :
            CertUtil.import_ca(CertUtil.ca_keyfile)

        # change the status,
        # web_control /cert_import_status will return True, else return False
        # launcher will wait ready to open browser and check update
        # config.cert_import_ready = True



if __name__ == '__main__':
    CertUtil.init_ca()


#TODO:
# CA commaon should be GoAgent, vander should be XX-Net
# need change and test on all support platform: Windows/Mac/Ubuntu/Debian

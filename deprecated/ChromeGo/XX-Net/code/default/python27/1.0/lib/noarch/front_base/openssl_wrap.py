
# OpenSSL is more stable then ssl
# but OpenSSL is different then ssl, so need a wrapper

# this wrap has a close callback.
# Which is used by google ip manager(google_ip.py)
# google ip manager keep a connection number counter for every ip.

# the wrap is used to keep some attribute like ip/appid for ssl

# __iowait and makefile is used for gevent but not use now.
import sys
import os
import select
import time
import socket
import errno


import OpenSSL
SSLError = OpenSSL.SSL.WantReadError

import datetime
import ssl
from pyasn1.type import univ, constraint, char, namedtype, tag
from pyasn1.codec.der.decoder import decode
from pyasn1.error import PyAsn1Error
socks_num = 0

# This is a throwaway variable to deal with a python bug
throwaway = datetime.datetime.strptime('20110101','%Y%m%d')

class _GeneralName(univ.Choice):
    # We are only interested in dNSNames. We use a default handler to ignore
    # other types.
    componentType = namedtype.NamedTypes(
        namedtype.NamedType('dNSName', char.IA5String().subtype(
                implicitTag=tag.Tag(tag.tagClassContext, tag.tagFormatSimple, 2)
            )
        ),
    )


class _GeneralNames(univ.SequenceOf):
    componentType = _GeneralName()
    sizeSpec = univ.SequenceOf.sizeSpec + constraint.ValueSizeConstraint(1, 1024)


class SSLCert:
    def __init__(self, cert):
        """
            Returns a (common name, [subject alternative names]) tuple.
        """
        self.x509 = cert

    @classmethod
    def from_pem(klass, txt):
        x509 = OpenSSL.crypto.load_certificate(OpenSSL.crypto.FILETYPE_PEM, txt)
        return klass(x509)

    @classmethod
    def from_der(klass, der):
        pem = ssl.DER_cert_to_PEM_cert(der)
        return klass.from_pem(pem)

    def to_pem(self):
        return OpenSSL.crypto.dump_certificate(OpenSSL.crypto.FILETYPE_PEM, self.x509)

    def digest(self, name):
        return self.x509.digest(name)

    @property
    def issuer(self):
        return self.x509.get_issuer().get_components()

    @property
    def notbefore(self):
        t = self.x509.get_notBefore()
        return datetime.datetime.strptime(t, "%Y%m%d%H%M%SZ")

    @property
    def notafter(self):
        t = self.x509.get_notAfter()
        return datetime.datetime.strptime(t, "%Y%m%d%H%M%SZ")

    @property
    def has_expired(self):
        return self.x509.has_expired()

    @property
    def subject(self):
        return self.x509.get_subject().get_components()

    @property
    def serial(self):
        return self.x509.get_serial_number()

    @property
    def keyinfo(self):
        pk = self.x509.get_pubkey()
        types = {
            OpenSSL.crypto.TYPE_RSA: "RSA",
            OpenSSL.crypto.TYPE_DSA: "DSA",
        }
        return (
            types.get(pk.type(), "UNKNOWN"),
            pk.bits()
        )

    @property
    def cn(self):
        c = None
        for i in self.subject:
            if i[0] == "CN":
                c = i[1]
        return c

    @property
    def altnames(self):
        altnames = []
        for i in range(self.x509.get_extension_count()):
            ext = self.x509.get_extension(i)
            if ext.get_short_name() == "subjectAltName":
                try:
                    dec = decode(ext.get_data(), asn1Spec=_GeneralNames())
                except PyAsn1Error:
                    continue
                for i in dec[0]:
                    altnames.append(i[0].asOctets())
        return altnames


class SSLConnection(object):

    def __init__(self, context, sock, ip=None, on_close=None):
        self._context = context
        self._sock = sock
        self.ip = ip
        self._connection = OpenSSL.SSL.Connection(context, sock)
        self._makefile_refs = 0
        self.on_close = on_close
        self.timeout = self._sock.gettimeout() or 0.1
        self.running = True
        self.socket_closed = False

        global socks_num
        socks_num += 1

    def __del__(self):
        if not self.socket_closed:
            socket.socket.close(self._sock)
            self.socket_closed = True
            if self.on_close:
                self.on_close(self.ip)
                
        global socks_num
        socks_num -= 1

    def __getattr__(self, attr):
        if attr not in ('_context', '_sock', '_connection', '_makefile_refs'):
            return getattr(self._connection, attr)

    def __iowait(self, io_func, *args, **kwargs):
        fd = self._sock.fileno()
        time_start = time.time()
        while self.running:
            time_now = time.time()
            wait_timeout = max(0.1, self.timeout - (time_now - time_start))
            wait_timeout = min(wait_timeout, 10)
            # in case socket was blocked by FW
            # recv is called before send request, which timeout is 240
            # then send request is called and timeout change to 100

            try:
                return io_func(*args, **kwargs)
            except (OpenSSL.SSL.WantReadError, OpenSSL.SSL.WantX509LookupError):
                sys.exc_clear()
                _, _, errors = select.select([fd], [], [fd], wait_timeout)
                if errors:
                    raise
                if time_now - time_start > self.timeout:
                    break
            except OpenSSL.SSL.WantWriteError:
                sys.exc_clear()
                _, _, errors = select.select([], [fd], [fd], wait_timeout)
                if errors:
                    raise
                time_now = time.time()
                if time_now - time_start > self.timeout:
                    break
            except OpenSSL.SSL.SysCallError as e:
                if e[0] == 10035 and 'WSAEWOULDBLOCK' in e[1]:
                    sys.exc_clear()
                    if io_func == self._connection.send:
                        _, _, errors = select.select([], [fd], [fd], wait_timeout)
                    else:
                        _, _, errors = select.select([fd], [], [fd], wait_timeout)

                    if errors:
                        raise
                    time_now = time.time()
                    if time_now - time_start > self.timeout:
                        break
                else:
                    raise e
            except Exception as e:
                #self.logger.exception("e:%r", e)
                raise e

        return 0

    def accept(self):
        sock, addr = self._sock.accept()
        client = OpenSSL.SSL.Connection(sock._context, sock)
        return client, addr

    def do_handshake(self):
        self.__iowait(self._connection.do_handshake)

    def connect(self, *args, **kwargs):
        return self.__iowait(self._connection.connect, *args, **kwargs)

    def __send(self, data, flags=0):
        try:
            return self.__iowait(self._connection.send, data, flags)
        except OpenSSL.SSL.SysCallError as e:
            if e[0] == -1 and not data:
                # errors when writing empty strings are expected and can be ignored
                return 0
            raise
        except Exception as e:
            #self.logger.exception("ssl send:%r", e)
            raise

    def __send_memoryview(self, data, flags=0):
        if hasattr(data, 'tobytes'):
            data = data.tobytes()
        return self.__send(data, flags)

    send = __send if sys.version_info >= (2, 7, 5) else __send_memoryview

    def recv(self, bufsiz, flags=0):
        pending = self._connection.pending()
        if pending:
            return self._connection.recv(min(pending, bufsiz))

        try:
            return self.__iowait(self._connection.recv, bufsiz, flags)
        except OpenSSL.SSL.ZeroReturnError:
            return ''
        except OpenSSL.SSL.SysCallError as e:
            if e[0] == -1 and 'Unexpected EOF' in e[1]:
                # remote closed
                #raise e
                return ""
            elif e[0] == 10053 or e[0] == 10054 or e[0] == 10038:
                return ""
            raise

    def recv_into(self, buf, nbytes=None):
        pending = self._connection.pending()
        if pending:
            ret = self._connection.recv_into(buf, nbytes)
            if not ret:
                # self.logger.debug("recv_into 0")
                pass
            return ret

        while self.running:
            try:
                ret = self.__iowait(self._connection.recv_into, buf, nbytes)
                if not ret:
                    # self.logger.debug("recv_into 0")
                    pass
                return ret
            except OpenSSL.SSL.ZeroReturnError as e:
                raise e
            except OpenSSL.SSL.SysCallError as e:
                if e[0] == -1 and 'Unexpected EOF' in e[1]:
                    # errors when reading empty strings are expected and can be ignored
                    return 0
                elif e[0] == 11 and e[1] == 'EAGAIN':
                    continue
                raise
            except errno.EAGAIN:
                continue
            except Exception as e:
                #self.logger.exception("recv_into:%r", e)
                raise e

    def read(self, bufsiz, flags=0):
        return self.recv(bufsiz, flags)

    def write(self, buf, flags=0):
        return self.sendall(buf, flags)

    def close(self):
        if self._makefile_refs < 1:
            self.running = False
            if not self.socket_closed:
                socket.socket.close(self._sock)
                self.socket_closed = True
                if self.on_close:
                    self.on_close(self.ip)
        else:
            self._makefile_refs -= 1

    def settimeout(self, t):
        if not self.running:
            return

        if self.timeout != t:
            self._sock.settimeout(t)
            self.timeout = t

    def makefile(self, mode='r', bufsize=-1):
        self._makefile_refs += 1
        return socket._fileobject(self, mode, bufsize, close=True)


class SSLContext(object):
    def __init__(self, logger, ca_certs=None, cipher_suites=None, support_http2=True):
        self.logger = logger

        if hasattr(OpenSSL.SSL, "TLSv1_2_METHOD"):
            ssl_version = "TLSv1_2"
        elif hasattr(OpenSSL.SSL, "TLSv1_1_METHOD"):
            ssl_version = "TLSv1_1"
        elif hasattr(OpenSSL.SSL, "TLSv1_METHOD"):
            ssl_version = "TLSv1"
        else:
            ssl_version = "SSLv23"

        if sys.platform == "darwin":
            # MacOS pyOpenSSL has TLSv1_2_METHOD attr but can use.
            # There for we hard code here.
            # may be try/cache is a better solution.
            ssl_version = "TLSv1"

        # freenas openssl support fix from twitter user "himanzero"
        # https://twitter.com/himanzero/status/645231724318748672
        if sys.platform == "freebsd9":
            ssl_version = "TLSv1"

        self.ssl_version = ssl_version
        self.logger.info("SSL use version:%s", ssl_version)

        protocol_version = getattr(OpenSSL.SSL, '%s_METHOD' % ssl_version)
        self.context = OpenSSL.SSL.Context(protocol_version)

        if ca_certs:
            self.context.load_verify_locations(os.path.abspath(ca_certs))
            self.context.set_verify(OpenSSL.SSL.VERIFY_PEER, lambda c, x, e, d, ok: ok)
        else:
            self.context.set_verify(OpenSSL.SSL.VERIFY_NONE, lambda c, x, e, d, ok: ok)

        if cipher_suites:
            self.context.set_cipher_list(':'.join(cipher_suites))

        self.support_alpn_npn = None
        if support_http2:
            try:
                self.context.set_alpn_protos([b'h2', b'http/1.1'])
                self.logger.info("OpenSSL support alpn")
                self.support_alpn_npn = "alpn"
                return
            except Exception as e:
                # self.logger.exception("set_alpn_protos:%r", e)
                pass

            try:
                self.context.set_npn_select_callback(SSLContext.npn_select_callback)
                self.logger.info("OpenSSL support npn")
                self.support_alpn_npn = "npn"
            except Exception as e:
                #xlog.exception("set_npn_select_callback:%r", e)
                self.logger.info("OpenSSL dont't support npn/alpn, no HTTP/2 supported.")
                pass

    @staticmethod
    def npn_select_callback(conn, protocols):
        # self.logger.debug("npn protocl:%s", ";".join(protocols))
        if b"h2" in protocols:
            conn.protos = "h2"
            return b"h2"
        else:
            return b"http/1.1"

    def set_ca(self, fn):
        try:
            self.context.load_verify_locations(fn)
            self.context.set_verify(OpenSSL.SSL.VERIFY_PEER, lambda c, x, e, d, ok: ok)
        except Exception as e:
            self.logger.debug("set_ca fail:%r", e)
            return

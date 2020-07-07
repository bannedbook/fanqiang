
import socket
import struct
import time

import OpenSSL
SSLError = OpenSSL.SSL.WantReadError

from pyasn1.codec.der import decoder as der_decoder
import socks
from subj_alt_name import SubjectAltName
import openssl_wrap


class ConnectCreator(object):
    def __init__(self, logger, config, openssl_context, host_manager,
                 timeout=5, debug=False,
                 check_cert=None):
        self.logger = logger
        self.config = config
        self.openssl_context = openssl_context
        self.host_manager = host_manager
        self.timeout = timeout
        self.debug = debug
        if check_cert:
            self.check_cert = check_cert
        self.update_config()

        self.connect_force_http1 = self.config.connect_force_http1
        self.connect_force_http2 = self.config.connect_force_http2

    def update_config(self):
        if int(self.config.PROXY_ENABLE):

            if self.config.PROXY_TYPE == "HTTP":
                proxy_type = socks.HTTP
            elif self.config.PROXY_TYPE == "SOCKS4":
                proxy_type = socks.SOCKS4
            elif self.config.PROXY_TYPE == "SOCKS5":
                proxy_type = socks.SOCKS5
            else:
                self.logger.error("proxy type %s unknown, disable proxy", self.config.PROXY_TYPE)
                raise Exception()

            socks.set_default_proxy(proxy_type, self.config.PROXY_HOST, self.config.PROXY_PORT,
                                    self.config.PROXY_USER,
                                    self.config.PROXY_PASSWD)

    @staticmethod
    def get_subj_alt_name(peer_cert):
        '''
        Copied from ndg.httpsclient.ssl_peer_verification.ServerSSLCertVerification
        Extract subjectAltName DNS name settings from certificate extensions
        @param peer_cert: peer certificate in SSL connection.  subjectAltName
        settings if any will be extracted from this
        @type peer_cert: OpenSSL.crypto.X509
        '''
        # Search through extensions
        dns_name = []
        general_names = SubjectAltName()
        for i in range(peer_cert.get_extension_count()):
            ext = peer_cert.get_extension(i)
            ext_name = ext.get_short_name()
            if ext_name == "subjectAltName":
                # PyOpenSSL returns extension data in ASN.1 encoded form
                ext_dat = ext.get_data()
                decoded_dat = der_decoder.decode(ext_dat, asn1Spec=general_names)

                for name in decoded_dat:
                    if isinstance(name, SubjectAltName):
                        for entry in range(len(name)):
                            component = name.getComponentByPosition(entry)
                            n = str(component.getComponent())
                            if n.startswith("*"):
                                continue
                            dns_name.append(n)
        return dns_name

    def get_ssl_cert_domain(self, ssl_sock):
        cert = ssl_sock.get_peer_certificate()
        if not cert:
            raise SSLError("no cert")

        ssl_cert = openssl_wrap.SSLCert(cert)
        ssl_sock.domain = ssl_cert.cn

    def connect_ssl(self, ip, port=443, sni="", close_cb=None):
        if sni:
            host = sni
        else:
            sni, host = self.host_manager.get_sni_host(ip)

        host = str(host)
        sni = str(sni)

        if int(self.config.PROXY_ENABLE):
            sock = socks.socksocket(socket.AF_INET if ':' not in ip else socket.AF_INET6)
        else:
            sock = socket.socket(socket.AF_INET if ':' not in ip else socket.AF_INET6)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        # set struct linger{l_onoff=1,l_linger=0} to avoid 10048 socket error
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack('ii', 1, 0))
        # resize socket recv buffer ->64 above to improve browser releated application performance
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, self.config.connect_receive_buffer)
        sock.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, True)
        sock.settimeout(self.timeout)

        ssl_sock = openssl_wrap.SSLConnection(self.openssl_context.context, sock, ip, on_close=close_cb)
        ssl_sock.set_connect_state()

        if sni:
            if self.debug:
                self.logger.debug("sni:%s", sni)

            try:
                ssl_sock.set_tlsext_host_name(sni)
            except:
                pass

        time_begin = time.time()
        ip_port = (ip, port)

        try:
            ssl_sock.connect(ip_port)
            time_connected = time.time()
            ssl_sock.do_handshake()
        except Exception as e:
            raise socket.error('conn fail, sni:%s, top:%s e:%r' % (sni, host, e))

        if self.connect_force_http1:
            ssl_sock.h2 = False
        elif self.connect_force_http2:
            ssl_sock.h2 = True
        else:
            try:
                h2 = ssl_sock.get_alpn_proto_negotiated()
                if h2 == "h2":
                    ssl_sock.h2 = True
                else:
                    ssl_sock.h2 = False
            except Exception as e:
                # xlog.exception("alpn:%r", e)
                if hasattr(ssl_sock._connection, "protos") and ssl_sock._connection.protos == "h2":
                    ssl_sock.h2 = True
                else:
                    ssl_sock.h2 = False

        time_handshaked = time.time()

        ssl_sock.sni = sni
        self.check_cert(ssl_sock)

        connect_time = int((time_connected - time_begin) * 1000)
        handshake_time = int((time_handshaked - time_begin) * 1000)
        # sometimes, we want to use raw tcp socket directly(select/epoll), so setattr it to ssl socket.
        ssl_sock.ip = ip
        ssl_sock._sock = sock
        ssl_sock.fd = sock.fileno()
        ssl_sock.create_time = time_begin
        ssl_sock.connect_time = connect_time
        ssl_sock.handshake_time = handshake_time
        ssl_sock.last_use_time = time_handshaked
        ssl_sock.host = host
        ssl_sock.received_size = 0

        return ssl_sock

    def check_cert(self, ssl_sock):
        cert_chain = ssl_sock.get_peer_cert_chain()
        if not cert_chain:
            raise socket.error('certificate is none, sni:%s' % ssl_sock.sni)

        if len(cert_chain) < self.config.min_intermediate_CA:
            raise socket.error('No intermediate CA was found.')

        if self.config.check_pkp and hasattr(OpenSSL.crypto, "dump_publickey"):
            # old OpenSSL not support this function.
            pub_key = OpenSSL.crypto.dump_publickey(OpenSSL.crypto.FILETYPE_PEM,
                                             cert_chain[1].get_pubkey())
            if pub_key not in self.config.CHECK_PKP:
                # google_ip.report_connect_fail(ip, force_remove=True)
                raise socket.error('The intermediate CA is mismatching.')
        self.get_ssl_cert_domain(ssl_sock)
        issuer_commonname = next((v for k, v in cert_chain[0].get_issuer().get_components() if k == 'CN'), '')
        if self.debug:
            for cert in cert_chain:
                for k, v in cert.get_issuer().get_components():
                    if k != "CN":
                        continue
                    cn = v
                    self.logger.debug("cn:%s", cn)

            self.logger.debug("issued by:%s", issuer_commonname)
            self.logger.debug("Common Name:%s", ssl_sock.domain)

        if self.config.check_commonname and not issuer_commonname.startswith(self.config.check_commonname):
            raise socket.error(' certificate is issued by %r' % (issuer_commonname))

        cert = ssl_sock.get_peer_certificate()
        if not cert:
            raise socket.error('certificate is none')

        if self.config.check_sni:
            # get_subj_alt_name cost near 100ms. be careful.
            try:
                alt_names = ConnectCreator.get_subj_alt_name(cert)
            except Exception as e:
                # self.logger.warn("get_subj_alt_name fail:%r", e)
                alt_names = [""]

            if self.debug:
                self.logger.debug('alt names: "%s"', '", "'.join(alt_names))

            if isinstance(self.config.check_sni, str):
                if self.config.check_sni not in alt_names:
                    raise socket.error('check sni fail, alt_names:%s' % (alt_names))
            else:
                alt_names = tuple(alt_names)
                if not ssl_sock.sni.endswith(alt_names):
                    raise socket.error('check sni:%s fail, alt_names:%s' % (ssl_sock.sni, alt_names))

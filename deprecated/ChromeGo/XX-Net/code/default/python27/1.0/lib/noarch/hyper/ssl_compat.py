# -*- coding: utf-8 -*-
"""
hyper/ssl_compat
~~~~~~~~~

Shoves pyOpenSSL into an API that looks like the standard Python 3.x ssl module.

Currently exposes exactly those attributes, classes, and methods that we
actually use in hyper (all method signatures are complete, however). May be
expanded to something more general-purpose in the future.
"""
try:
    import StringIO as BytesIO
except ImportError:
    from io import BytesIO
import errno
import socket
import time
import re

from OpenSSL import SSL as ossl
from service_identity.pyopenssl import verify_hostname as _verify

CERT_NONE = ossl.VERIFY_NONE
CERT_REQUIRED = ossl.VERIFY_PEER | ossl.VERIFY_FAIL_IF_NO_PEER_CERT

_OPENSSL_ATTRS = dict(
    OP_NO_COMPRESSION='OP_NO_COMPRESSION',
    PROTOCOL_TLSv1_2='TLSv1_2_METHOD',
    PROTOCOL_SSLv23='SSLv23_METHOD',
)

for external, internal in _OPENSSL_ATTRS.items():
    value = getattr(ossl, internal, None)
    if value:
        locals()[external] = value

OP_ALL = 0
for bit in [31] + list(range(10)): # TODO figure out the names of these other flags
    OP_ALL |= 1 << bit

HAS_NPN = True

def _proxy(method):
    return lambda self, *args, **kwargs: getattr(self._conn, method)(*args, **kwargs)

# TODO missing some attributes
class SSLError(OSError):
    pass

class CertificateError(SSLError):
    pass


def verify_hostname(ssl_sock, server_hostname):
    """
    A method nearly compatible with the stdlib's match_hostname.
    """
    if isinstance(server_hostname, bytes):
        server_hostname = server_hostname.decode('ascii')
    return _verify(ssl_sock._conn, server_hostname)


class SSLSocket(object):
    SSL_TIMEOUT = 3
    SSL_RETRY = .01

    def __init__(self, conn, server_side, do_handshake_on_connect,
                 suppress_ragged_eofs, server_hostname, check_hostname):
        self._conn = conn
        self._do_handshake_on_connect = do_handshake_on_connect
        self._suppress_ragged_eofs = suppress_ragged_eofs
        self._check_hostname = check_hostname

        if server_side:
            self._conn.set_accept_state()
        else:
            if server_hostname:
                self._conn.set_tlsext_host_name(server_hostname.encode('utf-8'))
                self._server_hostname = server_hostname
            self._conn.set_connect_state() # FIXME does this override do_handshake_on_connect=False?

        if self.connected and self._do_handshake_on_connect:
            self.do_handshake()

    @property
    def connected(self):
        try:
            self._conn.getpeername()
        except socket.error as e:
            if e.errno != errno.ENOTCONN:
                # It's an exception other than the one we expected if we're not
                # connected.
                raise
            return False
        return True

    # Lovingly stolen from CherryPy (http://svn.cherrypy.org/tags/cherrypy-3.2.1/cherrypy/wsgiserver/ssl_pyopenssl.py).
    def _safe_ssl_call(self, suppress_ragged_eofs, call, *args, **kwargs):
        """Wrap the given call with SSL error-trapping."""
        start = time.time()
        while True:
            try:
                return call(*args, **kwargs)
            except (ossl.WantReadError, ossl.WantWriteError):
                # Sleep and try again. This is dangerous, because it means
                # the rest of the stack has no way of differentiating
                # between a "new handshake" error and "client dropped".
                # Note this isn't an endless loop: there's a timeout below.
                time.sleep(self.SSL_RETRY)
            except ossl.Error as e:
                if suppress_ragged_eofs and e.args == (-1, 'Unexpected EOF'):
                    return b''
                raise socket.error(e.args[0])

            if time.time() - start > self.SSL_TIMEOUT:
                raise socket.timeout('timed out')

    def connect(self, address):
        self._conn.connect(address)
        if self._do_handshake_on_connect:
            self.do_handshake()

    def do_handshake(self):
        self._safe_ssl_call(False, self._conn.do_handshake)
        if self._check_hostname:
            verify_hostname(self, self._server_hostname)

    def recv(self, bufsize, flags=None):
        return self._safe_ssl_call(self._suppress_ragged_eofs, self._conn.recv,
                               bufsize, flags)

    def recv_into(self, buffer, bufsize=None, flags=None):
        # A temporary recv_into implementation. Should be replaced when
        # PyOpenSSL has merged pyca/pyopenssl#121.
        if bufsize is None:
            bufsize = len(buffer)

        data = self.recv(bufsize, flags)
        data_len = len(data)
        buffer[0:data_len] = data
        return data_len

    def send(self, data, flags=None):
        return self._safe_ssl_call(False, self._conn.send, data, flags)

    def selected_npn_protocol(self):
        proto = self._conn.get_next_proto_negotiated()
        if isinstance(proto, bytes):
            proto = proto.decode('ascii')

        return proto if proto else None

    def selected_alpn_protocol(self):
        proto = self._conn.get_alpn_proto_negotiated()
        if isinstance(proto, bytes):
            proto = proto.decode('ascii')

        return proto if proto else None

    def getpeercert(self):
        def resolve_alias(alias):
            return dict(
                C='countryName',
                ST='stateOrProvinceName',
                L='localityName',
                O='organizationName',
                OU='organizationalUnitName',
                CN='commonName',
            ).get(alias, alias)

        def to_components(name):
            # TODO Verify that these are actually *supposed* to all be single-element
            # tuples, and that's not just a quirk of the examples I've seen.
            return tuple([((resolve_alias(name.decode('utf-8')), value.decode('utf-8')),) for name, value in name.get_components()])

        # The standard getpeercert() takes the nice X509 object tree returned
        # by OpenSSL and turns it into a dict according to some format it seems
        # to have made up on the spot. Here, we do our best to emulate that.
        cert = self._conn.get_peer_certificate()
        result = dict(
            issuer=to_components(cert.get_issuer()),
            subject=to_components(cert.get_subject()),
            version=cert.get_subject(),
            serialNumber=cert.get_serial_number(),
            notBefore=cert.get_notBefore(),
            notAfter=cert.get_notAfter(),
        )
        # TODO extensions, including subjectAltName (see _decode_certificate in _ssl.c)
        return result

    # a dash of magic to reduce boilerplate
    for method in ['accept', 'bind', 'close', 'getsockname', 'listen', 'fileno']:
        locals()[method] = _proxy(method)


class SSLContext(object):
    def __init__(self, protocol):
        self.protocol = protocol
        self._ctx = ossl.Context(protocol)
        self.options = OP_ALL
        self.check_hostname = False
        self.npn_protos = []

    @property
    def options(self):
        return self._options

    @options.setter
    def options(self, value):
        self._options = value
        self._ctx.set_options(value)

    @property
    def verify_mode(self):
        return self._ctx.get_verify_mode()

    @verify_mode.setter
    def verify_mode(self, value):
        # TODO verify exception is raised on failure
        self._ctx.set_verify(value, lambda conn, cert, errnum, errdepth, ok: ok)

    def set_default_verify_paths(self):
        self._ctx.set_default_verify_paths()

    def load_verify_locations(self, cafile=None, capath=None, cadata=None):
        # TODO factor out common code
        if cafile is not None:
            cafile = cafile.encode('utf-8')
        if capath is not None:
            capath = capath.encode('utf-8')
        self._ctx.load_verify_locations(cafile, capath)
        if cadata is not None:
            self._ctx.load_verify_locations(BytesIO(cadata))

    def load_cert_chain(self, certfile, keyfile=None, password=None):
        self._ctx.use_certificate_file(certfile)
        if password is not None:
            self._ctx.set_passwd_cb(lambda max_length, prompt_twice, userdata: password)
        self._ctx.use_privatekey_file(keyfile or certfile)

    def set_npn_protocols(self, protocols):
        self.protocols = list(map(lambda x:x.encode('ascii'), protocols))

        def cb(conn, protos):
            # Detect the overlapping set of protocols.
            overlap = set(protos) & set(self.protocols)

            # Select the option that comes last in the list in the overlap.
            for p in self.protocols:
                if p in overlap:
                    return p
            else:
                return b''

        self._ctx.set_npn_select_callback(cb)

    def set_alpn_protocols(self, protocols):
        protocols = list(map(lambda x: x.encode('ascii'), protocols))
        self._ctx.set_alpn_protos(protocols)

    def wrap_socket(self, sock, server_side=False, do_handshake_on_connect=True,
                    suppress_ragged_eofs=True, server_hostname=None):
        conn = ossl.Connection(self._ctx, sock)
        return SSLSocket(conn, server_side, do_handshake_on_connect,
                         suppress_ragged_eofs, server_hostname,
                         # TODO what if this is changed after the fact?
                         self.check_hostname)

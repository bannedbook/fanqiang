# -*- coding: utf-8 -*-
"""
hyper/http11/connection
~~~~~~~~~~~~~~~~~~~~~~~

Objects that build hyper's connection-level HTTP/1.1 abstraction.
"""
import logging
import os
import socket
import base64

from collections import Iterable, Mapping

from .response import HTTP11Response
from ..tls import wrap_socket, H2C_PROTOCOL
from ..common.bufsocket import BufferedSocket
from ..common.exceptions import TLSUpgrade, HTTPUpgrade
from ..common.headers import HTTPHeaderMap
from ..common.util import to_bytestring, to_host_port_tuple
from ..compat import bytes

from ..packages.hyperframe.frame import SettingsFrame

# We prefer pycohttpparser to the pure-Python interpretation
try:  # pragma: no cover
    from pycohttpparser.api import Parser
except ImportError:  # pragma: no cover
    from .parser import Parser


log = logging.getLogger(__name__)

BODY_CHUNKED = 1
BODY_FLAT = 2


class HTTP11Connection(object):
    """
    An object representing a single HTTP/1.1 connection to a server.

    :param host: The host to connect to. This may be an IP address or a
        hostname, and optionally may include a port: for example,
        ``'twitter.com'``, ``'twitter.com:443'`` or ``'127.0.0.1'``.
    :param port: (optional) The port to connect to. If not provided and one also
        isn't provided in the ``host`` parameter, defaults to 80.
    :param secure: (optional) Whether the request should use TLS. Defaults to
        ``False`` for most requests, but to ``True`` for any request issued to
        port 443.
    :param ssl_context: (optional) A class with custom certificate settings.
        If not provided then hyper's default ``SSLContext`` is used instead.
    :param proxy_host: (optional) The proxy to connect to.  This can be an IP 
        address or a host name and may include a port.
    :param proxy_port: (optional) The proxy port to connect to. If not provided 
        and one also isn't provided in the ``proxy`` parameter, 
        defaults to 8080.
    """
    def __init__(self, host, port=None, secure=None, ssl_context=None, 
                 proxy_host=None, proxy_port=None, **kwargs):
        if port is None:
            self.host, self.port = to_host_port_tuple(host, default_port=80)
        else:
            self.host, self.port = host, port

        # Record whether we plan to secure the request. In future this should
        # be extended to a security profile, but a bool will do for now.
        # TODO: Actually do something with this!
        if secure is not None:
            self.secure = secure
        elif self.port == 443:
            self.secure = True
        else:
            self.secure = False

        # only send http upgrade headers for non-secure connection
        self._send_http_upgrade = not self.secure

        self.ssl_context = ssl_context
        self._sock = None

        # Setup proxy details if applicable.
        if proxy_host:
            if proxy_port is None:
                self.proxy_host, self.proxy_port = to_host_port_tuple(proxy_host, default_port=8080)
            else:
                self.proxy_host, self.proxy_port = proxy_host, proxy_port
        else:
            self.proxy_host = None
            self.proxy_port = None

        #: The size of the in-memory buffer used to store data from the
        #: network. This is used as a performance optimisation. Increase buffer
        #: size to improve performance: decrease it to conserve memory.
        #: Defaults to 64kB.
        self.network_buffer_size = 65536

        #: The object used to perform HTTP/1.1 parsing. Needs to conform to
        #: the standard hyper parsing interface.
        self.parser = Parser()

    def connect(self):
        """
        Connect to the server specified when the object was created. This is a
        no-op if we're already connected.

        :returns: Nothing.
        """
        if self._sock is None:
            if not self.proxy_host:
                host = self.host
                port = self.port
            else:
                host = self.proxy_host
                port = self.proxy_port
                
            sock = socket.create_connection((host, port), 5)
            proto = None

            if self.secure:
                assert not self.proxy_host, "Using a proxy with HTTPS not yet supported."
                sock, proto = wrap_socket(sock, host, self.ssl_context)

            log.debug("Selected protocol: %s", proto)
            sock = BufferedSocket(sock, self.network_buffer_size)

            if proto not in ('http/1.1', None):
                raise TLSUpgrade(proto, sock)

            self._sock = sock

        return

    def request(self, method, url, body=None, headers={}):
        """
        This will send a request to the server using the HTTP request method
        ``method`` and the selector ``url``. If the ``body`` argument is
        present, it should be string or bytes object of data to send after the
        headers are finished. Strings are encoded as UTF-8. To use other
        encodings, pass a bytes object. The Content-Length header is set to the
        length of the body field.

        :param method: The request method, e.g. ``'GET'``.
        :param url: The URL to contact, e.g. ``'/path/segment'``.
        :param body: (optional) The request body to send. Must be a bytestring
            or a file-like object.
        :param headers: (optional) The headers to send on the request.
        :returns: Nothing.
        """
        method = to_bytestring(method)
        url = to_bytestring(url)

        if not isinstance(headers, HTTPHeaderMap):
            if isinstance(headers, Mapping):
                headers = HTTPHeaderMap(headers.items())
            elif isinstance(headers, Iterable):
                headers = HTTPHeaderMap(headers)
            else:
                raise ValueError('Header argument must be a dictionary or an iterable')

        if self._sock is None:
            self.connect()

        if self._send_http_upgrade:
            self._add_upgrade_headers(headers)
            self._send_http_upgrade = False

        # We may need extra headers.
        if body:
            body_type = self._add_body_headers(headers, body)

        if b'host' not in headers:
            headers[b'host'] = self.host

        # Begin by emitting the header block.
        self._send_headers(method, url, headers)

        # Next, send the request body.
        if body:
            self._send_body(body, body_type)

        return

    def get_response(self):
        """
        Returns a response object.

        This is an early beta, so the response object is pretty stupid. That's
        ok, we'll fix it later.
        """
        headers = HTTPHeaderMap()

        response = None
        while response is None:
            # 'encourage' the socket to receive data.
            self._sock.fill()
            response = self.parser.parse_response(self._sock.buffer)

        for n, v in response.headers:
            headers[n.tobytes()] = v.tobytes()

        self._sock.advance_buffer(response.consumed)

        if (response.status == 101 and 
           b'upgrade' in headers['connection'] and 
           H2C_PROTOCOL.encode('utf-8') in headers['upgrade']):
            raise HTTPUpgrade(H2C_PROTOCOL, self._sock)

        return HTTP11Response(
            response.status,
            response.msg.tobytes(),
            headers,
            self._sock,
            self
        )

    def _send_headers(self, method, url, headers):
        """
        Handles the logic of sending the header block.
        """
        self._sock.send(b' '.join([method, url, b'HTTP/1.1\r\n']))

        for name, value in headers.iter_raw():
            name, value = to_bytestring(name), to_bytestring(value)
            header = b''.join([name, b': ', value, b'\r\n'])
            self._sock.send(header)

        self._sock.send(b'\r\n')

    def _add_body_headers(self, headers, body):
        """
        Adds any headers needed for sending the request body. This will always
        defer to the user-supplied header content.

        :returns: One of (BODY_CHUNKED, BODY_FLAT), indicating what type of
            request body should be used.
        """
        if b'content-length' in headers:
            return BODY_FLAT

        if b'chunked' in headers.get(b'transfer-encoding', []):
            return BODY_CHUNKED

        # For bytestring bodies we upload the content with a fixed length.
        # For file objects, we use the length of the file object.
        if isinstance(body, bytes):
            length = str(len(body)).encode('utf-8')
        elif hasattr(body, 'fileno'):
            length = str(os.fstat(body.fileno()).st_size).encode('utf-8')
        else:
            length = None

        if length:
            headers[b'content-length'] = length
            return BODY_FLAT

        headers[b'transfer-encoding'] = b'chunked'
        return BODY_CHUNKED

    def _add_upgrade_headers(self, headers):
        # Add HTTP Upgrade headers.
        headers[b'connection'] = b'Upgrade, HTTP2-Settings'
        headers[b'upgrade'] = H2C_PROTOCOL
        
        # Encode SETTINGS frame payload in Base64 and put into the HTTP-2 Settings header.
        http2_settings = SettingsFrame(0)
        http2_settings.settings[SettingsFrame.INITIAL_WINDOW_SIZE] = 65535
        headers[b'HTTP2-Settings'] = base64.b64encode(http2_settings.serialize_body())

    def _send_body(self, body, body_type):
        """
        Handles the HTTP/1.1 logic for sending HTTP bodies. This does magical
        different things in different cases.
        """
        if body_type == BODY_FLAT:
            # Special case for files and other 'readable' objects.
            if hasattr(body, 'read'):
                while True:
                    block = body.read(16*1024)
                    if not block:
                        break

                    try:
                        self._sock.send(block)
                    except TypeError:
                        raise ValueError(
                            "File objects must return bytestrings"
                        )

                return

            # Case for bytestrings.
            elif isinstance(body, bytes):
                self._sock.send(body)

                return

            # Iterables that set a specific content length.
            else:
                for item in body:
                    try:
                        self._sock.send(item)
                    except TypeError:
                        raise ValueError("Body must be a bytestring")

                return

        # Chunked! For chunked bodies we don't special-case, we just iterate
        # over what we have and send stuff out.
        for chunk in body:
            length = '{0:x}'.format(len(chunk)).encode('ascii')

            # For now write this as four 'send' calls. That's probably
            # inefficient, let's come back to it.
            try:
                self._sock.send(length)
                self._sock.send(b'\r\n')
                self._sock.send(chunk)
                self._sock.send(b'\r\n')
            except TypeError:
                raise ValueError(
                    "Iterable bodies must always iterate in bytestrings"
                )

        self._sock.send(b'0\r\n\r\n')
        return

    def close(self):
        """
        Closes the connection. This closes the socket and then abandons the
        reference to it. After calling this method, any outstanding
        :class:`Response <hyper.http11.response.Response>` objects will throw
        exceptions if attempts are made to read their bodies.

        In some cases this method will automatically be called.

        .. warning:: This method should absolutely only be called when you are
                     certain the connection object is no longer needed.
        """
        self._sock.close()
        self._sock = None

    # The following two methods are the implementation of the context manager
    # protocol.
    def __enter__(self):
        return self

    def __exit__(self, type, value, tb):
        self.close()
        return False  # Never swallow exceptions.

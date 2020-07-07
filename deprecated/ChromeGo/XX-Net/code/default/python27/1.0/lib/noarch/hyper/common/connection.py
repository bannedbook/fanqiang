# -*- coding: utf-8 -*-
"""
hyper/common/connection
~~~~~~~~~~~~~~~~~~~~~~~

Hyper's HTTP/1.1 and HTTP/2 abstraction layer.
"""
from .exceptions import TLSUpgrade, HTTPUpgrade
from ..http11.connection import HTTP11Connection
from ..http20.connection import HTTP20Connection
from ..tls import H2_NPN_PROTOCOLS, H2C_PROTOCOL

class HTTPConnection(object):
    """
    An object representing a single HTTP connection to a server.

    This object behaves similarly to the Python standard library's
    ``HTTPConnection`` object, with a few critical differences.

    Most of the standard library's arguments to the constructor are not
    supported by hyper. Most optional parameters apply to *either* HTTP/1.1 or
    HTTP/2.

    :param host: The host to connect to. This may be an IP address or a
        hostname, and optionally may include a port: for example,
        ``'http2bin.org'``, ``'http2bin.org:443'`` or ``'127.0.0.1'``.
    :param port: (optional) The port to connect to. If not provided and one also
        isn't provided in the ``host`` parameter, defaults to 443.
    :param secure: (optional) Whether the request should use TLS.
        Defaults to ``False`` for most requests, but to ``True`` for any
        request issued to port 443.
    :param window_manager: (optional) The class to use to manage flow control
        windows. This needs to be a subclass of the
        :class:`BaseFlowControlManager <hyper.http20.window.BaseFlowControlManager>`.
        If not provided,
        :class:`FlowControlManager <hyper.http20.window.FlowControlManager>`
        will be used.
    :param enable_push: (optional) Whether the server is allowed to push
        resources to the client (see
        :meth:`get_pushes() <hyper.HTTP20Connection.get_pushes>`).
    :param ssl_context: (optional) A class with custom certificate settings.
        If not provided then hyper's default ``SSLContext`` is used instead.
    :param proxy_host: (optional) The proxy to connect to.  This can be an IP address
        or a host name and may include a port.
    :param proxy_port: (optional) The proxy port to connect to. If not provided 
        and one also isn't provided in the ``proxy`` parameter, defaults to 8080.
    """
    def __init__(self,
                 host,
                 port=None,
                 secure=None,
                 window_manager=None,
                 enable_push=False,
                 ssl_context=None,
                 proxy_host=None,
                 proxy_port=None,
                 **kwargs):

        self._host = host
        self._port = port
        self._h1_kwargs = {
            'secure': secure, 'ssl_context': ssl_context, 
            'proxy_host': proxy_host, 'proxy_port': proxy_port 
        }
        self._h2_kwargs = {
            'window_manager': window_manager, 'enable_push': enable_push,
            'secure': secure, 'ssl_context': ssl_context, 
            'proxy_host': proxy_host, 'proxy_port': proxy_port
        }

        # Add any unexpected kwargs to both dictionaries.
        self._h1_kwargs.update(kwargs)
        self._h2_kwargs.update(kwargs)

        self._conn = HTTP11Connection(
            self._host, self._port, **self._h1_kwargs
        )

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
        :returns: A stream ID for the request, or ``None`` if the request is
            made over HTTP/1.1.
        """
        try:
            return self._conn.request(
                method=method, url=url, body=body, headers=headers
            )
        except TLSUpgrade as e:
            # We upgraded in the NPN/ALPN handshake. We can just go straight to
            # the world of HTTP/2. Replace the backing object and insert the
            # socket into it.
            assert e.negotiated in H2_NPN_PROTOCOLS

            self._conn = HTTP20Connection(
                self._host, self._port, **self._h2_kwargs
            )
            self._conn._sock = e.sock

            # Because we skipped the connecting logic, we need to send the
            # HTTP/2 preamble.
            self._conn._send_preamble()

            return self._conn.request(
                method=method, url=url, body=body, headers=headers
            )

    def get_response(self, *args, **kwargs):
        """
        Returns a response object.
        """
        try:
            return self._conn.get_response(*args, **kwargs)
        except HTTPUpgrade as e:
            # We upgraded via the HTTP Upgrade mechanism. We can just
            # go straight to the world of HTTP/2. Replace the backing object
            # and insert the socket into it.
            assert e.negotiated == H2C_PROTOCOL

            self._conn = HTTP20Connection(
                self._host, self._port, **self._h2_kwargs
            )

            self._conn._sock = e.sock
            # stream id 1 is used by the upgrade request and response
            # and is half-closed by the client
            self._conn._new_stream(stream_id=1, local_closed=True)

            # HTTP/2 preamble must be sent after receipt of a HTTP/1.1 101
            self._conn._send_preamble()

            return self._conn.get_response(1)

    # The following two methods are the implementation of the context manager
    # protocol.
    def __enter__(self):  # pragma: no cover
        return self

    def __exit__(self, type, value, tb):  # pragma: no cover
        self._conn.close()
        return False  # Never swallow exceptions.

    # Can anyone say 'proxy object pattern'?
    def __getattr__(self, name):
        return getattr(self._conn, name)

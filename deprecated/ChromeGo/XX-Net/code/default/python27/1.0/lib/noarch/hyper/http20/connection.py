# -*- coding: utf-8 -*-
"""
hyper/http20/connection
~~~~~~~~~~~~~~~~~~~~~~~

Objects that build hyper's connection-level HTTP/2 abstraction.
"""
from ..tls import wrap_socket, H2_NPN_PROTOCOLS, H2C_PROTOCOL
from ..common.exceptions import ConnectionResetError
from ..common.bufsocket import BufferedSocket
from ..common.headers import HTTPHeaderMap
from ..common.util import to_host_port_tuple, to_native_string, to_bytestring
from ..packages.hyperframe.frame import (
    FRAMES, DataFrame, HeadersFrame, PushPromiseFrame, RstStreamFrame,
    SettingsFrame, Frame, WindowUpdateFrame, GoAwayFrame, PingFrame,
    BlockedFrame, FRAME_MAX_LEN, FRAME_MAX_ALLOWED_LEN
)
from ..packages.hpack.hpack_compat import Encoder, Decoder
from .stream import Stream
from .response import HTTP20Response, HTTP20Push
from .window import FlowControlManager
from .exceptions import ConnectionError, ProtocolError
from . import errors

import errno
import logging
import socket

log = logging.getLogger(__name__)

DEFAULT_WINDOW_SIZE = 65535


class HTTP20Connection(object):
    """
    An object representing a single HTTP/2 connection to a server.

    This object behaves similarly to the Python standard library's
    ``HTTPConnection`` object, with a few critical differences.

    Most of the standard library's arguments to the constructor are irrelevant
    for HTTP/2 or not supported by hyper.

    :param host: The host to connect to. This may be an IP address or a
        hostname, and optionally may include a port: for example,
        ``'http2bin.org'``, ``'http2bin.org:443'`` or ``'127.0.0.1'``.
    :param port: (optional) The port to connect to. If not provided and one also
        isn't provided in the ``host`` parameter, defaults to 443.
    :param secure: (optional) Whether the request should use TLS. Defaults to
        ``False`` for most requests, but to ``True`` for any request issued to
        port 443.
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
    def __init__(self, ssl_sock, host=None, ip=None, port=None, secure=None, window_manager=None, enable_push=False,
                 ssl_context=None, proxy_host=None, proxy_port=None, **kwargs):
        """
        Creates an HTTP/2 connection to a specific server.
        """

        self.ip = ip

        if port is None:
            self.host, self.port = to_host_port_tuple(host, default_port=443)
        else:
            self.host, self.port = host, port

        if secure is not None:
            self.secure = secure
        elif self.port == 443:
            self.secure = True
        else:
            self.secure = False

        self._enable_push = enable_push
        self.ssl_context = ssl_context

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

        # Create the mutable state.
        self.__wm_class = window_manager or FlowControlManager
        self.__init_state()

        if ssl_sock:
            self._sock = BufferedSocket(ssl_sock, self.network_buffer_size)

            self._send_preamble()

        return

    def __init_state(self):
        """
        Initializes the 'mutable state' portions of the HTTP/2 connection
        object.

        This method exists to enable HTTP20Connection objects to be reused if
        they're closed, by resetting the connection object to its basic state
        whenever it ends up closed. Any situation that needs to recreate the
        connection can call this method and it will be done.

        This is one of the only methods in hyper that is truly private, as
        users should be strongly discouraged from messing about with connection
        objects themselves.
        """
        # Streams are stored in a dictionary keyed off their stream IDs. We
        # also save the most recent one for easy access without having to walk
        # the dictionary.
        # Finally, we add a set of all streams that we or the remote party
        # forcefully closed with RST_STREAM, to avoid encountering issues where
        # frames were already in flight before the RST was processed.
        self.streams = {}
        self.recent_stream = None
        self.next_stream_id = 1
        self.reset_streams = set()

        # Header encoding/decoding is at the connection scope, so we embed a
        # header encoder and a decoder. These get passed to child stream
        # objects.
        self.encoder = Encoder()
        self.decoder = Decoder()

        # Values for the settings used on an HTTP/2 connection.
        self._settings = {
            SettingsFrame.INITIAL_WINDOW_SIZE: DEFAULT_WINDOW_SIZE,
            SettingsFrame.SETTINGS_MAX_FRAME_SIZE: FRAME_MAX_LEN,
        }

        # The socket used to send data.
        self._sock = None

        # The inbound and outbound flow control windows.
        self._out_flow_control_window = 65535

        # Instantiate a window manager.
        self.window_manager = self.__wm_class(65535)

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
        :returns: A stream ID for the request.
        """
        stream_id = self.putrequest(method, url)

        default_headers = (':method', ':scheme', ':authority', ':path')
        for name, value in headers.items():
            is_default = to_native_string(name) in default_headers
            self.putheader(name, value, stream_id, replace=is_default)

        # Convert the body to bytes if needed.
        if body:
            body = to_bytestring(body)

        self.endheaders(message_body=body, final=True, stream_id=stream_id)

        return stream_id

    def _get_stream(self, stream_id):
        return (self.streams[stream_id] if stream_id is not None
                else self.recent_stream)

    def get_response(self, stream_id=None):
        """
        Should be called after a request is sent to get a response from the
        server. If sending multiple parallel requests, pass the stream ID of
        the request whose response you want. Returns a
        :class:`HTTP20Response <hyper.HTTP20Response>` instance.
        If you pass no ``stream_id``, you will receive the oldest
        :class:`HTTPResponse <hyper.HTTP20Response>` still outstanding.

        :param stream_id: (optional) The stream ID of the request for which to
            get a response.
        :returns: A :class:`HTTP20Response <hyper.HTTP20Response>` object.
        """
        stream = self._get_stream(stream_id)
        return HTTP20Response(stream.getheaders(), stream)

    def get_pushes(self, stream_id=None, capture_all=False):
        """
        Returns a generator that yields push promises from the server. **Note
        that this method is not idempotent**: promises returned in one call
        will not be returned in subsequent calls. Iterating through generators
        returned by multiple calls to this method simultaneously results in
        undefined behavior.

        :param stream_id: (optional) The stream ID of the request for which to
            get push promises.
        :param capture_all: (optional) If ``False``, the generator will yield
            all buffered push promises without blocking. If ``True``, the
            generator will first yield all buffered push promises, then yield
            additional ones as they arrive, and terminate when the original
            stream closes.
        :returns: A generator of :class:`HTTP20Push <hyper.HTTP20Push>` objects
            corresponding to the streams pushed by the server.
        """
        stream = self._get_stream(stream_id)
        for promised_stream_id, headers in stream.get_pushes(capture_all):
            yield HTTP20Push(
                HTTPHeaderMap(headers), self.streams[promised_stream_id]
            )

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

            if self.ip:
                sock = socket.create_connection((self.ip, port), 5)
            else:
                sock = socket.create_connection((host, port), 5)

            if self.secure:
                assert not self.proxy_host, "Using a proxy with HTTPS not yet supported."
                sock, proto = wrap_socket(sock, host, self.ssl_context)
            else:
                proto = H2C_PROTOCOL

            log.debug("Selected NPN protocol: %s", proto)
            assert proto in H2_NPN_PROTOCOLS or proto == H2C_PROTOCOL

            self._sock = BufferedSocket(sock, self.network_buffer_size)

            self._send_preamble()

        return

    def _send_preamble(self):
        """
        Sends the necessary HTTP/2 preamble.
        """
        # We need to send the connection header immediately on this
        # connection, followed by an initial settings frame.
        self._sock.send(b'PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n')
        f = SettingsFrame(0)
        f.settings[SettingsFrame.ENABLE_PUSH] = int(self._enable_push)
        self._send_cb(f)

        # The server will also send an initial settings frame, so get it.
        self._recv_cb()

    def close(self, error_code=None):
        """
        Close the connection to the server.

        :param error_code: (optional) The error code to reset all streams with.
        :returns: Nothing.
        """
        # Close all streams
        for stream in list(self.streams.values()):
            log.debug("Close stream %d" % stream.stream_id)
            stream.close(error_code)

        # Send GoAway frame to the server
        try:
            self._send_cb(GoAwayFrame(0), True)
        except Exception as e:  # pragma: no cover
            log.warn("GoAway frame could not be sent: %s" % e)

        if self._sock is not None:
            self._sock.close()
            self.__init_state()

    def putrequest(self, method, selector, **kwargs):
        """
        This should be the first call for sending a given HTTP request to a
        server. It returns a stream ID for the given connection that should be
        passed to all subsequent request building calls.

        :param method: The request method, e.g. ``'GET'``.
        :param selector: The path selector.
        :returns: A stream ID for the request.
        """
        # Create a new stream.
        s = self._new_stream()

        # To this stream we need to immediately add a few headers that are
        # HTTP/2 specific. These are: ":method", ":scheme", ":authority" and
        # ":path". We can set all of these now.
        s.add_header(":method", method)
        s.add_header(":scheme", "https" if self.secure else "http")
        s.add_header(":authority", self.host)
        s.add_header(":path", selector)

        # Save the stream.
        self.recent_stream = s

        return s.stream_id

    def putheader(self, header, argument, stream_id=None, replace=False):
        """
        Sends an HTTP header to the server, with name ``header`` and value
        ``argument``.

        Unlike the ``httplib`` version of this function, this version does not
        actually send anything when called. Instead, it queues the headers up
        to be sent when you call
        :meth:`endheaders() <hyper.HTTP20Connection.endheaders>`.

        This method ensures that headers conform to the HTTP/2 specification.
        In particular, it strips out the ``Connection`` header, as that header
        is no longer valid in HTTP/2. This is to make it easy to write code
        that runs correctly in both HTTP/1.1 and HTTP/2.

        :param header: The name of the header.
        :param argument: The value of the header.
        :param stream_id: (optional) The stream ID of the request to add the
            header to.
        :returns: Nothing.
        """
        stream = self._get_stream(stream_id)
        stream.add_header(header, argument, replace)

        return

    def endheaders(self, message_body=None, final=False, stream_id=None):
        """
        Sends the prepared headers to the server. If the ``message_body``
        argument is provided it will also be sent to the server as the body of
        the request, and the stream will immediately be closed. If the
        ``final`` argument is set to True, the stream will also immediately
        be closed: otherwise, the stream will be left open and subsequent calls
        to ``send()`` will be required.

        :param message_body: (optional) The body to send. May not be provided
            assuming that ``send()`` will be called.
        :param final: (optional) If the ``message_body`` parameter is provided,
            should be set to ``True`` if no further data will be provided via
            calls to :meth:`send() <hyper.HTTP20Connection.send>`.
        :param stream_id: (optional) The stream ID of the request to finish
            sending the headers on.
        :returns: Nothing.
        """
        self.connect()

        stream = self._get_stream(stream_id)

        # Close this if we've been told no more data is coming and we don't
        # have any to send.
        stream.open(final and message_body is None)

        # Send whatever data we have.
        if message_body is not None:
            stream.send_data(message_body, final)

        return

    def send(self, data, final=False, stream_id=None):
        """
        Sends some data to the server. This data will be sent immediately
        (excluding the normal HTTP/2 flow control rules). If this is the last
        data that will be sent as part of this request, the ``final`` argument
        should be set to ``True``. This will cause the stream to be closed.

        :param data: The data to send.
        :param final: (optional) Whether this is the last bit of data to be
            sent on this request.
        :param stream_id: (optional) The stream ID of the request to send the
            data on.
        :returns: Nothing.
        """
        stream = self._get_stream(stream_id)
        stream.send_data(data, final)

        return

    def receive_frame(self, frame):
        """
        Handles receiving frames intended for the stream.
        """
        if frame.type == WindowUpdateFrame.type:
            self._out_flow_control_window += frame.window_increment
        elif frame.type == PingFrame.type:
            if 'ACK' not in frame.flags:
                # The spec requires us to reply with PING+ACK and identical data.
                p = PingFrame(0)
                p.flags.add('ACK')
                p.opaque_data = frame.opaque_data
                self._send_cb(p, True)
        elif frame.type == SettingsFrame.type:
            if 'ACK' not in frame.flags:
                self._update_settings(frame)

                # When the setting containing the max frame size value is out
                # of range, the spec dictates to tear down the connection.
                # Therefore we make sure the socket is still alive before
                # returning the ack.
                if self._sock is not None:
                    f = SettingsFrame(0)
                    f.flags.add('ACK')
                    self._send_cb(f)
        elif frame.type == GoAwayFrame.type:
            # If we get GoAway with error code zero, we are doing a graceful
            # shutdown and all is well. Otherwise, throw an exception.
            self.close()

            # If an error occured, try to read the error description from
            # code registry otherwise use the frame's additional data.
            if frame.error_code != 0:
                try:
                    name, number, description = errors.get_data(
                        frame.error_code
                    )
                except ValueError:
                    error_string = (
                        "Encountered error code %d, extra data %s" %
                        (frame.error_code, frame.additional_data)
                    )
                else:
                    error_string = (
                        "Encountered error %s %s: %s" %
                        (name, number, description)
                    )

                raise ConnectionError(error_string)

        elif frame.type == BlockedFrame.type:
            increment = self.window_manager._blocked()
            if increment:
                f = WindowUpdateFrame(0)
                f.window_increment = increment
                self._send_cb(f, True)
        elif frame.type in FRAMES:
            # This frame isn't valid at this point.
            raise ValueError("Unexpected frame %s." % frame)
        else:  # pragma: no cover
            # Unexpected frames belong to extensions. Just drop it on the
            # floor, but log so that users know that something happened.
            log.warning("Received unknown frame, type %d", frame.type)
            pass

    def _update_settings(self, frame):
        """
        Handles the data sent by a settings frame.
        """
        if SettingsFrame.HEADER_TABLE_SIZE in frame.settings:
            new_size = frame.settings[SettingsFrame.HEADER_TABLE_SIZE]

            self._settings[SettingsFrame.HEADER_TABLE_SIZE] = new_size
            self.encoder.header_table_size = new_size

        if SettingsFrame.INITIAL_WINDOW_SIZE in frame.settings:
            newsize = frame.settings[SettingsFrame.INITIAL_WINDOW_SIZE]
            oldsize = self._settings[SettingsFrame.INITIAL_WINDOW_SIZE]
            delta = newsize - oldsize

            for stream in self.streams.values():
                stream._out_flow_control_window += delta

            # Update the connection window size.
            self._out_flow_control_window += delta

            self._settings[SettingsFrame.INITIAL_WINDOW_SIZE] = newsize

        if SettingsFrame.SETTINGS_MAX_FRAME_SIZE in frame.settings:
            new_size = frame.settings[SettingsFrame.SETTINGS_MAX_FRAME_SIZE]
            if FRAME_MAX_LEN <= new_size <= FRAME_MAX_ALLOWED_LEN:
                self._settings[SettingsFrame.SETTINGS_MAX_FRAME_SIZE] = (
                    new_size
                )
            else:
                log.warning(
                    "Frame size %d is outside of allowed range",
                    new_size
                )

                # Tear the connection down with error code PROTOCOL_ERROR
                self.close(1)
                error_string = (
                    "Advertised frame size %d is outside of range" % (new_size)
                )
                raise ConnectionError(error_string)

    def _new_stream(self, stream_id=None, local_closed=False):
        """
        Returns a new stream object for this connection.
        """
        window_size = self._settings[SettingsFrame.INITIAL_WINDOW_SIZE]
        s = Stream(
            stream_id or self.next_stream_id, self._send_cb, self._recv_cb,
            self._close_stream, self.encoder, self.decoder,
            self.__wm_class(DEFAULT_WINDOW_SIZE), local_closed
        )
        s._out_flow_control_window = window_size
        self.streams[s.stream_id] = s
        self.next_stream_id += 2

        return s

    def _close_stream(self, stream_id, error_code=None):
        """
        Called by a stream when it would like to be 'closed'.
        """
        # Graceful shutdown of streams involves not emitting an error code
        # at all.
        if error_code:
            self._send_rst_frame(stream_id, error_code)
        else:
            # Just delete the stream.
            try:
                del self.streams[stream_id]
            except KeyError as e:  # pragma: no cover
                log.warn(
                    "Stream with id %d does not exist: %s",
                    stream_id, e)

    def _send_cb(self, frame, tolerate_peer_gone=False):
        """
        This is the callback used by streams to send data on the connection.

        It expects to receive a single frame, and then to serialize that frame
        and send it on the connection. It does so obeying the connection-level
        flow-control principles of HTTP/2.
        """
        # Maintain our outgoing flow-control window.
        if frame.type == DataFrame.type:
            # If we don't have room in the flow control window, we need to look
            # for a Window Update frame.
            while self._out_flow_control_window < len(frame.data):
                self._recv_cb()

            self._out_flow_control_window -= len(frame.data)

        data = frame.serialize()

        max_frame_size = self._settings[SettingsFrame.SETTINGS_MAX_FRAME_SIZE]
        if frame.body_len > max_frame_size:
            raise ValueError(
                     "Frame size %d exceeds maximum frame size setting %d" %
                     (frame.body_len,
                      self._settings[SettingsFrame.SETTINGS_MAX_FRAME_SIZE])
            )

        log.info(
            "Sending frame %s on stream %d",
            frame.__class__.__name__,
            frame.stream_id
        )

        try:
            self._sock.send(data)
        except socket.error as e:
            if (not tolerate_peer_gone or
                e.errno not in (errno.EPIPE, errno.ECONNRESET)):
                raise

    def _adjust_receive_window(self, frame_len):
        """
        Adjusts the window size in response to receiving a DATA frame of length
        ``frame_len``. May send a WINDOWUPDATE frame if necessary.
        """
        increment = self.window_manager._handle_frame(frame_len)

        if increment:
            f = WindowUpdateFrame(0)
            f.window_increment = increment
            self._send_cb(f, True)

        return

    def _consume_single_frame(self):
        """
        Consumes a single frame from the TCP stream.

        Right now this method really does a bit too much: it shouldn't be
        responsible for determining if a frame is valid or to increase the
        flow control window.
        """
        # Begin by reading 9 bytes from the socket.
        header = self._sock.recv(9)

        # Parse the header. We can use the returned memoryview directly here.
        frame, length = Frame.parse_frame_header(header)

        if (length > FRAME_MAX_ALLOWED_LEN):
            log.warning(
                "Frame size exceeded on stream %d (received: %d, max: %d)",
                frame.stream_id,
                length,
                FRAME_MAX_ALLOWED_LEN
            )
            self._send_rst_frame(frame.stream_id, 6) # 6 = FRAME_SIZE_ERROR

        # Read the remaining data from the socket.
        data = self._recv_payload(length)
        self._consume_frame_payload(frame, data)

    def _recv_payload(self, length):
        """
        This receive function handles the situation where the underlying socket
        has not received the full set of data. It spins on calling `recv`
        until the full quantity of data is available before returning.

        Note that this function makes us vulnerable to a DoS attack, where a
        server can send part of a frame and then swallow the rest. We should
        add support for socket timeouts here at some stage.
        """
        # TODO: Fix DoS risk.
        if not length:
            return memoryview(b'')

        buffer = bytearray(length)
        buffer_view = memoryview(buffer)
        index = 0
        data_length = -1
        # _sock.recv(length) might not read out all data if the given length
        # is very large. So it should be to retrieve from socket repeatedly.
        while length and data_length:
            data = self._sock.recv(length)
            data_length = len(data)
            end = index + data_length
            buffer_view[index:end] = data[:]
            length -= data_length
            index = end

        return buffer_view[:end]

    def _consume_frame_payload(self, frame, data):
        """
        This builds and handles a frame.
        """
        frame.parse_body(data)

        log.info(
            "Received frame %s on stream %d",
            frame.__class__.__name__,
            frame.stream_id
        )

        # Maintain our flow control window. We do this by delegating to the
        # chosen WindowManager.
        if frame.type == DataFrame.type:
            # Inform the WindowManager of how much data was received. If the
            # manager tells us to increment the window, do so.
            self._adjust_receive_window(frame.flow_controlled_length)
        elif frame.type == PushPromiseFrame.type:
            if self._enable_push:
                self._new_stream(frame.promised_stream_id, local_closed=True)
            else:
                # Servers are forbidden from sending push promises when
                # the ENABLE_PUSH setting is 0, but the spec leaves the client
                # action undefined when they do it anyway. So we just refuse
                # the stream and go about our business.
                self._send_rst_frame(frame.promised_stream_id, 7)

        # If this frame was received on a stream that has been reset, drop it.
        if frame.stream_id in self.reset_streams:
            log.info(
                "Stream %s has been reset, dropping frame.", frame.stream_id
            )
            return

        # Work out to whom this frame should go.
        if frame.stream_id != 0:
            try:
                self.streams[frame.stream_id].receive_frame(frame)
            except KeyError:
                # If we receive an unexpected stream identifier then we
                # cancel the stream with an error of type PROTOCOL_ERROR
                self._send_rst_frame(frame.stream_id, 1)
                log.warning(
                    "Unexpected stream identifier %d" % (frame.stream_id)
                )

            # If this is a RST_STREAM frame, we may get more than one (because
            # of frames in flight). Keep track.
            if frame.type == RstStreamFrame.type:
                self.reset_streams.add(frame.stream_id)
        else:
            self.receive_frame(frame)

    def _recv_cb(self):
        """
        This is the callback used by streams to read data from the connection.

        It expects to read a single frame, and then to deserialize that frame
        and pass it to the relevant stream. It then attempts to optimistically
        read further frames (in an attempt to ensure that we see control frames
        as early as possible).

        This is generally called by a stream, not by the connection itself, and
        it's likely that streams will read a frame that doesn't belong to them.
        """
        self._consume_single_frame()
        count = 9

        while count and self._sock is not None and self._sock.can_read:
            # If the connection has been closed, bail out.
            try:
                self._consume_single_frame()
            except ConnectionResetError:
                break

            count -= 1

    def _send_rst_frame(self, stream_id, error_code):
        """
        Send reset stream frame with error code and remove stream from map.
        """
        f = RstStreamFrame(stream_id)
        f.error_code = error_code
        self._send_cb(f)

        try:
            del self.streams[stream_id]
        except KeyError as e:  # pragma: no cover
            log.warn(
                "Stream with id %d does not exist: %s",
                stream_id, e)

        # Keep track of the fact that we reset this stream in case there are
        # other frames in flight.
        self.reset_streams.add(stream_id)

    # The following two methods are the implementation of the context manager
    # protocol.
    def __enter__(self):
        return self

    def __exit__(self, type, value, tb):
        self.close()
        return False  # Never swallow exceptions.

# -*- coding: utf-8 -*-
"""
hyper/http20/stream
~~~~~~~~~~~~~~~~~~~

Objects that make up the stream-level abstraction of hyper's HTTP/2 support.

These objects are not expected to be part of the public HTTP/2 API: they're
intended purely for use inside hyper's HTTP/2 abstraction.

Conceptually, a single HTTP/2 connection is made up of many streams: each
stream is an independent, bi-directional sequence of HTTP headers and data.
Each stream is identified by a monotonically increasing integer, assigned to
the stream by the endpoint that initiated the stream.
"""
from ..common.headers import HTTPHeaderMap
from ..packages.hyperframe.frame import (
    FRAME_MAX_LEN, FRAMES, HeadersFrame, DataFrame, PushPromiseFrame,
    WindowUpdateFrame, ContinuationFrame, BlockedFrame, RstStreamFrame
)
from .exceptions import ProtocolError, StreamResetError
from .util import h2_safe_headers
import collections
import logging
import zlib

log = logging.getLogger(__name__)


# Define a set of states for a HTTP/2 stream.
STATE_IDLE               = 0
STATE_OPEN               = 1
STATE_HALF_CLOSED_LOCAL  = 2
STATE_HALF_CLOSED_REMOTE = 3
STATE_CLOSED             = 4


# Define the largest chunk of data we'll send in one go. Realistically, we
# should take the MSS into account but that's pretty dull, so let's just say
# 1kB and call it a day.
MAX_CHUNK = 1024


class Stream(object):
    """
    A single HTTP/2 stream.

    A stream is an independent, bi-directional sequence of HTTP headers and
    data. Each stream is identified by a single integer. From a HTTP
    perspective, a stream _approximately_ matches a single request-response
    pair.
    """
    def __init__(self,
                 stream_id,
                 data_cb,
                 recv_cb,
                 close_cb,
                 header_encoder,
                 header_decoder,
                 window_manager,
                 local_closed=False):
        self.stream_id = stream_id
        self.state = STATE_HALF_CLOSED_LOCAL if local_closed else STATE_IDLE
        self.headers = HTTPHeaderMap()

        # Set to a key-value set of the response headers once their
        # HEADERS..CONTINUATION frame sequence finishes.
        self.response_headers = None

        # Set to a key-value set of the response trailers once their
        # HEADERS..CONTINUATION frame sequence finishes.
        self.response_trailers = None

        # A dict mapping the promised stream ID of a pushed resource to a
        # key-value set of its request headers. Entries are added once their
        # PUSH_PROMISE..CONTINUATION frame sequence finishes.
        self.promised_headers = {}

        # Chunks of encoded header data from the current
        # (HEADERS|PUSH_PROMISE)..CONTINUATION frame sequence. Since sending any
        # frame other than a CONTINUATION is disallowed while a header block is
        # being transmitted, this and ``promised_stream_id`` are the only pieces
        # of state we have to track.
        self.header_data = []
        self.promised_stream_id = None

        # Unconsumed response data chunks. Empties after every call to _read().
        self.data = []

        # There are two flow control windows: one for data we're sending,
        # one for data being sent to us.
        self._in_window_manager = window_manager
        self._out_flow_control_window = 65535

        # This is the callback handed to the stream by its parent connection.
        # It is called when the stream wants to send data. It expects to
        # receive a list of frames that will be automatically serialized.
        self._data_cb = data_cb

        # Similarly, this is a callback that reads one frame off the
        # connection.
        self._recv_cb = recv_cb

        # This is the callback to be called when the stream is closed.
        self._close_cb = close_cb

        # A reference to the header encoder and decoder objects belonging to
        # the parent connection.
        self._encoder = header_encoder
        self._decoder = header_decoder

    def add_header(self, name, value, replace=False):
        """
        Adds a single HTTP header to the headers to be sent on the request.
        """
        if not replace:
            self.headers[name] = value
        else:
            self.headers.replace(name, value)


    def send_data(self, data, final):
        """
        Send some data on the stream. If this is the end of the data to be
        sent, the ``final`` flag _must_ be set to True. If no data is to be
        sent, set ``data`` to ``None``.
        """
        # Define a utility iterator for file objects.
        def file_iterator(fobj):
            while True:
                data = fobj.read(MAX_CHUNK)
                yield data
                if len(data) < MAX_CHUNK:
                    break

        # Build the appropriate iterator for the data, in chunks of CHUNK_SIZE.
        if hasattr(data, 'read'):
            chunks = file_iterator(data)
        else:
            chunks = (data[i:i+MAX_CHUNK]
                      for i in range(0, len(data), MAX_CHUNK))

        for chunk in chunks:
            self._send_chunk(chunk, final)

    @property
    def _local_closed(self):
        return self.state in (STATE_CLOSED, STATE_HALF_CLOSED_LOCAL)

    @property
    def _remote_closed(self):
        return self.state in (STATE_CLOSED, STATE_HALF_CLOSED_REMOTE)

    @property
    def _local_open(self):
        return self.state in (STATE_OPEN, STATE_HALF_CLOSED_REMOTE)

    def _close_local(self):
        self.state = (
            STATE_HALF_CLOSED_LOCAL if self.state == STATE_OPEN
            else STATE_CLOSED
        )

    def _close_remote(self):
        self.state = (
            STATE_HALF_CLOSED_REMOTE if self.state == STATE_OPEN
            else STATE_CLOSED
        )

    def _read(self, amt=None):
        """
        Read data from the stream. Unlike a normal read behaviour, this
        function returns _at least_ ``amt`` data, but may return more.
        """
        def listlen(list):
            return sum(map(len, list))

        # Keep reading until the stream is closed or we get enough data.
        while not self._remote_closed and (amt is None or listlen(self.data) < amt):
            self._recv_cb()

        result = b''.join(self.data)
        self.data = []
        return result

    def _read_one_frame(self):
        """
        Reads a single data frame from the stream and returns it.
        """
        # Keep reading until the stream is closed or we have a data frame.
        while not self._remote_closed and not self.data:
            self._recv_cb()

        try:
            return self.data.pop(0)
        except IndexError:
            return None

    def receive_frame(self, frame):
        """
        Handle a frame received on this stream.
        """
        if 'END_STREAM' in frame.flags:
            log.debug("Closing remote side of stream")
            self._close_remote()

        if frame.type == WindowUpdateFrame.type:
            self._out_flow_control_window += frame.window_increment
        elif frame.type == HeadersFrame.type:
            # Begin the header block for the response headers.
            self.promised_stream_id = None
            self.header_data = [frame.data]
        elif frame.type == PushPromiseFrame.type:
            # Begin a header block for the request headers of a pushed resource.
            self.promised_stream_id = frame.promised_stream_id
            self.header_data = [frame.data]
        elif frame.type == ContinuationFrame.type:
            # Continue a header block begun with either HEADERS or PUSH_PROMISE.
            self.header_data.append(frame.data)
        elif frame.type == DataFrame.type:
            # Increase the window size. Only do this if the data frame contains
            # actual data.
            size = frame.flow_controlled_length
            increment = self._in_window_manager._handle_frame(size)

            # Append the data to the buffer.
            self.data.append(frame.data.tobytes())

            if increment and not self._remote_closed:
                w = WindowUpdateFrame(self.stream_id)
                w.window_increment = increment
                self._data_cb(w, True)
        elif frame.type == BlockedFrame.type:
            # If we've been blocked we may want to fixup the window.
            increment = self._in_window_manager._blocked()
            if increment:
                w = WindowUpdateFrame(self.stream_id)
                w.window_increment = increment
                self._data_cb(w, True)
        elif frame.type == RstStreamFrame.type:
            self.close(0)
            raise StreamResetError("Stream forcefully closed.")
        elif frame.type in FRAMES:
            # This frame isn't valid at this point.
            raise ValueError("Unexpected frame %s." % frame)
        else:  # pragma: no cover
            # Unknown frames belong to extensions. Just drop it on the
            # floor, but log so that users know that something happened.
            log.warning("Received unknown frame, type %d", frame.type)
            pass

        if 'END_HEADERS' in frame.flags:
            # Begin by decoding the header block. If this fails, we need to
            # tear down the entire connection. TODO: actually do that.
            if len(self.header_data) == 1:
                headers = self._decoder.decode(self.header_data[0])
            else:
                headers = self._decoder.decode(b''.join(self.header_data.tobytes()))

            # If we're involved in a PUSH_PROMISE sequence, this header block
            # is the proposed request headers. Save it off. Otherwise, handle
            # it as an in-stream HEADERS block.
            if (self.promised_stream_id is None):
                self._handle_header_block(headers)
            else:
                self.promised_headers[self.promised_stream_id] = headers

            # We've handled the headers, zero them out.
            self.header_data = None

    def open(self, end):
        """
        Open the stream. Does this by encoding and sending the headers: no more
        calls to ``add_header`` are allowed after this method is called.
        The `end` flag controls whether this will be the end of the stream, or
        whether data will follow.
        """
        # Strip any headers invalid in H2.
        headers = h2_safe_headers(self.headers)

        # Encode the headers.
        encoded_headers = self._encoder.encode(headers)

        # It's possible that there is a substantial amount of data here. The
        # data needs to go into one HEADERS frame, followed by a number of
        # CONTINUATION frames. For now, for ease of implementation, let's just
        # assume that's never going to happen (16kB of headers is lots!).
        # Additionally, since this is so unlikely, there's no point writing a
        # test for this: it's just so simple.
        if len(encoded_headers) > FRAME_MAX_LEN:  # pragma: no cover
            raise ValueError("Header block too large.")

        header_frame = HeadersFrame(self.stream_id)
        header_frame.data = encoded_headers

        # If no data has been provided, this is the end of the stream. Either
        # way, due to the restriction above it's definitely the end of the
        # headers.
        header_frame.flags.add('END_HEADERS')

        if end:
            header_frame.flags.add('END_STREAM')

        # Send the header frame.
        self._data_cb(header_frame)

        # Transition the stream state appropriately.
        self.state = STATE_HALF_CLOSED_LOCAL if end else STATE_OPEN

        return

    def getheaders(self):
        """
        Once all data has been sent on this connection, returns a key-value set
        of the headers of the response to the original request.
        """
        assert self._local_closed

        # Keep reading until all headers are received.
        while self.response_headers is None:
            self._recv_cb()

        # Find the Content-Length header if present.
        self._in_window_manager.document_size = (
            int(self.response_headers.get(b'content-length', [0])[0])
        )

        return self.response_headers

    def gettrailers(self):
        """
        Once all data has been sent on this connection, returns a key-value set
        of the trailers of the response to the original request.

        .. warning:: Note that this method requires that the stream is
                     totally exhausted. This means that, if you have not
                     completely read from the stream, all stream data will be
                     read into memory.

        :returns: The key-value set of the trailers, or ``None`` if no trailers
                  were sent.
        """
        # Keep reading until the stream is done.
        # TODO: Right now this doesn't handle CONTINUATION blocks in trailers.
        # The idea of receiving such a thing is mind-boggling it's so unlikely,
        # but we should fix this up at some stage.
        while not self._remote_closed:
            self._recv_cb()

        return self.response_trailers

    def get_pushes(self, capture_all=False):
        """
        Returns a generator that yields push promises from the server. Note that
        this method is not idempotent; promises returned in one call will not be
        returned in subsequent calls. Iterating through generators returned by
        multiple calls to this method simultaneously results in undefined
        behavior.

        :param capture_all: If ``False``, the generator will yield all buffered
            push promises without blocking. If ``True``, the generator will
            first yield all buffered push promises, then yield additional ones
            as they arrive, and terminate when the original stream closes.
        """
        while True:
            for pair in self.promised_headers.items():
                yield pair
            self.promised_headers = {}
            if not capture_all or self._remote_closed:
                break
            self._recv_cb()

    def close(self, error_code=None):
        """
        Closes the stream. If the stream is currently open, attempts to close
        it as gracefully as possible.

        :param error_code: (optional) The error code to reset the stream with.
        :returns: Nothing.
        """
        # Right now let's not bother with grace, let's just call close on the
        # connection. If not error code is provided then assume it is a
        # gracefull shutdown.
        self._close_cb(self.stream_id, error_code or 0)

    def _handle_header_block(self, headers):
        """
        Handles the logic for receiving a completed headers block.

        A headers block is an uninterrupted sequence of one HEADERS frame
        followed by zero or more CONTINUATION frames, and is terminated by a
        frame bearing the END_HEADERS flag.

        HTTP/2 allows receipt of up to three such blocks on a stream. The first
        is optional, and contains a 1XX response. The second is mandatory, and
        must contain a final response (200 or higher). The third is optional,
        and may contain 'trailers', headers that are sent after a chunk-encoded
        body is sent. This method enforces the logic associated with this,
        storing the headers in the appropriate places.
        """
        # At this stage we should check for a provisional response (1XX). As
        # hyper doesn't currently support such responses, we'll leave that as a
        # TODO.
        # TODO: Handle 1XX responses here.

        # The header block may be for trailers or headers. If we've already
        # received headers these _must_ be for trailers.
        if self.response_headers is None:
            self.response_headers = HTTPHeaderMap(headers)
        elif self.response_trailers is None:
            self.response_trailers = HTTPHeaderMap(headers)
        else:
            # Received too many headers blocks.
            raise ProtocolError("Too many header blocks.")

        return

    def _send_chunk(self, data, final):
        """
        Implements most of the sending logic.

        Takes a single chunk of size at most MAX_CHUNK, wraps it in a frame and
        sends it. Optionally sets the END_STREAM flag if this is the last chunk
        (determined by being of size less than MAX_CHUNK) and no more data is
        to be sent.
        """
        assert self._local_open

        f = DataFrame(self.stream_id)
        f.data = data

        # If the length of the data is less than MAX_CHUNK, we're probably
        # at the end of the file. If this is the end of the data, mark it
        # as END_STREAM.
        if len(data) < MAX_CHUNK and final:
            f.flags.add('END_STREAM')

        # If we don't fit in the connection window, try popping frames off the
        # connection in hope that one might be a Window Update frame.
        while len(data) > self._out_flow_control_window:
            self._recv_cb()

        # Send the frame and decrement the flow control window.
        self._data_cb(f)
        self._out_flow_control_window -= len(data)

        # If no more data is to be sent on this stream, transition our state.
        if len(data) < MAX_CHUNK and final:
            self._close_local()

# -*- coding: utf-8 -*-
"""
port from hyper/http20/stream for async
remove push support
increase init window size to improve performance
~~~~~~~~~~~~~~~~~~~

Objects that make up the stream-level abstraction of hyper's HTTP/2 support.


Conceptually, a single HTTP/2 connection is made up of many streams: each
stream is an independent, bi-directional sequence of HTTP headers and data.
Each stream is identified by a monotonically increasing integer, assigned to
the stream by the endpoint that initiated the stream.
"""


import threading
from hyper.common.headers import HTTPHeaderMap
from hyper.packages.hyperframe.frame import (
    FRAME_MAX_LEN, FRAMES, HeadersFrame, DataFrame, PushPromiseFrame,
    WindowUpdateFrame, ContinuationFrame, BlockedFrame, RstStreamFrame
)
from hyper.http20.exceptions import ProtocolError, StreamResetError
from hyper.http20.util import h2_safe_headers
from hyper.http20.response import strip_headers
from hyper.common.util import to_host_port_tuple, to_native_string, to_bytestring
import simple_http_client

from http_common import *


# Define a set of states for a HTTP/2 stream.
STATE_IDLE               = 0
STATE_OPEN               = 1
STATE_HALF_CLOSED_LOCAL  = 2
STATE_HALF_CLOSED_REMOTE = 3
STATE_CLOSED             = 4


class Stream(object):
    """
    A single HTTP/2 stream.

    A stream is an independent, bi-directional sequence of HTTP headers and
    data. Each stream is identified by a single integer. From a HTTP
    perspective, a stream _approximately_ matches a single request-response
    pair.
    """
    def __init__(self,
                 logger,
                 config,
                 connection,
                 ip,
                 stream_id,
                 task,
                 send_cb,
                 close_cb,
                 encoder,
                 decoder,
                 receive_window_manager,
                 remote_window_size,
                 max_frame_size):

        self.logger = logger
        self.config = config
        self.connection = connection
        self.ip = ip
        self.stream_id = stream_id
        self.task = task
        self.state = STATE_IDLE
        self.get_head_time = None

        # There are two flow control windows: one for data we're sending,
        # one for data being sent to us.
        self.receive_window_manager = receive_window_manager
        self.remote_window_size = remote_window_size
        self.max_frame_size = max_frame_size

        # This is the callback handed to the stream by its parent connection.
        # It is called when the stream wants to send data. It expects to
        # receive a list of frames that will be automatically serialized.
        self._send_cb = send_cb

        # This is the callback to be called when the stream is closed.
        self._close_cb = close_cb

        # A reference to the header encoder and decoder objects belonging to
        # the parent connection.
        self._encoder = encoder
        self._decoder = decoder

        self.request_headers = HTTPHeaderMap()

        # Convert the body to bytes if needed.
        self.request_body = to_bytestring(self.task.body)

        # request body not send blocked by send window
        # the left body will send when send window opened.
        self.request_body_left = len(self.request_body)
        self.request_body_sended = False

        # data list before decode
        self.response_header_datas = []

        # Set to a key-value set of the response headers once their
        # HEADERS..CONTINUATION frame sequence finishes.
        self.response_headers = None

        # Unconsumed response data chunks
        self.response_body = []
        self.response_body_len = 0
        self.start_time = time.time()

    def start_request(self):
        """
        Open the stream. Does this by encoding and sending the headers: no more
        calls to ``add_header`` are allowed after this method is called.
        The `end` flag controls whether this will be the end of the stream, or
        whether data will follow.
        """
        # Strip any headers invalid in H2.
        #headers = h2_safe_headers(self.request_headers)

        host = self.connection.get_host(self.task.host)

        self.add_header(":method", self.task.method)
        self.add_header(":scheme", "https")
        self.add_header(":authority", host)
        self.add_header(":path", self.task.path)

        default_headers = (':method', ':scheme', ':authority', ':path')
        #headers = h2_safe_headers(self.task.headers)
        for name, value in self.task.headers.items():
            is_default = to_native_string(name) in default_headers
            self.add_header(name, value, replace=is_default)

        # Encode the headers.
        encoded_headers = self._encoder(self.request_headers)

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
        if self.request_body_left == 0:
            header_frame.flags.add('END_STREAM')

        # Send the header frame.
        self.task.set_state("start send header")
        self._send_cb(header_frame)

        # Transition the stream state appropriately.
        self.state = STATE_OPEN

        self.task.set_state("start send left body")
        if self.request_body_left > 0:
            self.send_left_body()

    def add_header(self, name, value, replace=False):
        """
        Adds a single HTTP header to the headers to be sent on the request.
        """
        if not replace:
            self.request_headers[name] = value
        else:
            self.request_headers.replace(name, value)

    def send_left_body(self):
        while self.remote_window_size and not self.request_body_sended:
            send_size = min(self.remote_window_size, self.request_body_left, self.max_frame_size)

            f = DataFrame(self.stream_id)
            data_start = len(self.request_body) - self.request_body_left
            f.data = self.request_body[data_start:data_start+send_size]

            self.remote_window_size -= send_size
            self.request_body_left -= send_size

            # If the length of the data is less than MAX_CHUNK, we're probably
            # at the end of the file. If this is the end of the data, mark it
            # as END_STREAM.
            if self.request_body_left == 0:
                f.flags.add('END_STREAM')

            # Send the frame and decrement the flow control window.
            self._send_cb(f)

            # If no more data is to be sent on this stream, transition our state.
            if self.request_body_left == 0:
                self.request_body_sended = True
                self._close_local()
                self.task.set_state("end send left body")

    def receive_frame(self, frame):
        """
        Handle a frame received on this stream.
        called by connection.
        """
        # self.logger.debug("stream %d recved frame %r", self.stream_id, frame)
        if frame.type == WindowUpdateFrame.type:
            self.remote_window_size += frame.window_increment
            self.send_left_body()
        elif frame.type == HeadersFrame.type:
            # Begin the header block for the response headers.
            #self.response_header_datas = [frame.data]
            self.response_header_datas.append(frame.data)
        elif frame.type == PushPromiseFrame.type:
            self.logger.error("%s receive PushPromiseFrame:%d", self.ip, frame.stream_id)
        elif frame.type == ContinuationFrame.type:
            # Continue a header block begun with either HEADERS or PUSH_PROMISE.
            self.response_header_datas.append(frame.data)
        elif frame.type == DataFrame.type:
            # Append the data to the buffer.
            if not self.task.finished:
                self.task.put_data(frame.data)

            if 'END_STREAM' not in frame.flags:
                # Increase the window size. Only do this if the data frame contains
                # actual data.
                # don't do it if stream is closed.
                size = frame.flow_controlled_length
                increment = self.receive_window_manager._handle_frame(size)
                #if increment:
                #    self.logger.debug("stream:%d frame size:%d increase win:%d", self.stream_id, size, increment)

                #content_len = int(self.request_headers.get("Content-Length")[0])
                #self.logger.debug("%s get:%d s:%d", self.ip, self.response_body_len, size)

                if increment and not self._remote_closed:
                    w = WindowUpdateFrame(self.stream_id)
                    w.window_increment = increment
                    self._send_cb(w)
        elif frame.type == BlockedFrame.type:
            # If we've been blocked we may want to fixup the window.
            increment = self.receive_window_manager._blocked()
            if increment:
                w = WindowUpdateFrame(self.stream_id)
                w.window_increment = increment
                self._send_cb(w)
        elif frame.type == RstStreamFrame.type:
            # Rest Frame send from server is not define in RFC
            inactive_time = time.time() - self.connection.last_recv_time
            self.logger.debug("%s Stream %d Rest by server, inactive:%d. error code:%d",
                       self.ip, self.stream_id, inactive_time, frame.error_code)
            self.connection.close("RESET")
        elif frame.type in FRAMES:
            # This frame isn't valid at this point.
            #raise ValueError("Unexpected frame %s." % frame)
            self.logger.error("%s Unexpected frame %s.", self.ip, frame)
        else:  # pragma: no cover
            # Unknown frames belong to extensions. Just drop it on the
            # floor, but log so that users know that something happened.
            self.logger.error("%s Received unknown frame, type %d", self.ip, frame.type)
            pass

        if 'END_HEADERS' in frame.flags:
            if self.response_headers is not None:
                raise ProtocolError("Too many header blocks.")

            # Begin by decoding the header block. If this fails, we need to
            # tear down the entire connection.
            if len(self.response_header_datas) == 1:
                header_data = self.response_header_datas[0]
            else:
                header_data = b''.join(self.response_header_datas)

            try:
                headers = self._decoder.decode(header_data)
            except Exception as e:
                self.logger.exception("decode h2 header %s fail:%r", header_data, e)
                raise e

            self.response_headers = HTTPHeaderMap(headers)

            # We've handled the headers, zero them out.
            self.response_header_datas = None

            self.get_head_time = time.time()

            length = self.response_headers.get("Content-Length", None)
            if isinstance(length, list):
                length = int(length[0])
            if not self.task.finished:
                self.task.content_length = length
                self.task.set_state("h2_get_head")
                self.send_response()

        if 'END_STREAM' in frame.flags:
            #self.logger.debug("%s Closing remote side of stream:%d", self.ip, self.stream_id)
            time_now = time.time()
            time_cost = time_now - self.get_head_time
            if time_cost > 0 and \
                    isinstance(self.task.content_length, int) and \
                    not self.task.finished:
                speed = self.task.content_length / time_cost
                self.task.set_state("h2_finish[SP:%d]" % speed)

            self._close_remote()

            self.close("end stream")
            if not self.task.finished:
                self.connection.continue_timeout = 0

    def send_response(self):
        if self.task.responsed:
            self.logger.error("http2_stream send_response but responsed.%s", self.task.url)
            self.close("h2 stream send_response but sended.")
            return

        self.task.responsed = True
        status = int(self.response_headers[b':status'][0])
        strip_headers(self.response_headers)
        response = simple_http_client.BaseResponse(status=status, headers=self.response_headers)
        response.ssl_sock = self.connection.ssl_sock
        response.worker = self.connection
        response.task = self.task
        self.task.queue.put(response)
        if status in self.config.http2_status_to_close:
            self.connection.close("status %d" % status)

    def close(self, reason="close"):
        if not self.task.responsed:
            self.connection.retry_task_cb(self.task, reason)
        else:
            self.task.finish()
            # empty block means fail or closed.
        self._close_remote()
        self._close_cb(self.stream_id, reason)

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

    def check_timeout(self, now):
        if time.time() - self.start_time < self.task.timeout:
            return

        if self._remote_closed:
            return

        self.logger.warn("h2 timeout %s task_trace:%s worker_trace:%s",
                  self.connection.ssl_sock.ip,
                  self.task.get_trace(),
                  self.connection.get_trace())
        self.task.set_state("timeout")

        if self.task.responsed:
            self.task.finish()
        else:
            self.task.response_fail("timeout")

        self.connection.continue_timeout += 1
        #if self.connection.continue_timeout >= self.connection.config.http2_max_timeout_tasks and \
        #        time.time() - self.connection.last_redv_time > self.connection.config.http2_timeout_active:
        #    self.connection.close("down fail")
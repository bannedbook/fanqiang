
import Queue
import threading
import socket
import errno
import struct

from http_common import *


from hyper.common.bufsocket import BufferedSocket

from hyper.packages.hyperframe.frame import (
    FRAMES, DataFrame, HeadersFrame, PushPromiseFrame, RstStreamFrame,
    SettingsFrame, Frame, WindowUpdateFrame, GoAwayFrame, PingFrame,
    BlockedFrame, FRAME_MAX_ALLOWED_LEN, FRAME_MAX_LEN
)
from http2_stream import Stream
from hyper.http20.window import BaseFlowControlManager

from hyper.packages.hpack import Encoder, Decoder

# this is defined in rfc7540
# default window size 64k
DEFAULT_WINDOW_SIZE = 65535

# default max frame is 16k, defined in rfc7540
DEFAULT_MAX_FRAME = FRAME_MAX_LEN


class FlowControlManager(BaseFlowControlManager):
    """
    ``hyper``'s default flow control manager.

    This implements hyper's flow control algorithms. This algorithm attempts to
    reduce the number of WINDOWUPDATE frames we send without blocking the remote
    endpoint behind the flow control window.

    This algorithm will become more complicated over time. In the current form,
    the algorithm is very simple:
        - When the flow control window gets less than 3/4 of the maximum size,
          increment back to the maximum.
        - Otherwise, if the flow control window gets to less than 1kB, increment
          back to the maximum.
    """
    def increase_window_size(self, frame_size):
        future_window_size = self.window_size - frame_size

        if ((future_window_size < (self.initial_window_size * 3 / 4)) or
            (future_window_size < 1000)):
            return self.initial_window_size - future_window_size

        return 0

    def blocked(self):
        return self.initial_window_size - self.window_size


class RawFrame(object):
    def __init__(self, dat):
        self.dat = dat

    def serialize(self):
        return self.dat

    def __repr__(self):
        out_str = "{type}".format(type=type(self).__name__)
        return out_str


class Http2Worker(HttpWorker):
    version = "2"

    def __init__(self, logger, ip_manager, config, ssl_sock, close_cb, retry_task_cb, idle_cb, log_debug_data):
        super(Http2Worker, self).__init__(
            logger, ip_manager, config, ssl_sock, close_cb, retry_task_cb, idle_cb, log_debug_data)

        self.network_buffer_size = 65535

        # Google http/2 time out is 4 mins.
        self.ssl_sock.settimeout(240)
        self._sock = BufferedSocket(ssl_sock, self.network_buffer_size)

        self.next_stream_id = 1
        self.streams = {}
        self.last_ping_time = time.time()
        self.continue_timeout = 0

        # count ping not ACK
        # increase when send ping
        # decrease when recv ping ack
        # if this in not 0, don't accept request.
        self.ping_on_way = 0
        self.accept_task = False

        # request_lock
        self.request_lock = threading.Lock()

        # all send frame must put to this queue
        # then send by send_loop
        # every frame put to this queue must allowed by stream window and connection window
        # any data frame blocked by connection window should put to self.blocked_send_frames
        self.send_queue = Queue.Queue()
        self.encoder = Encoder()
        self.decoder = Decoder()

        # keep blocked data frame in this buffer
        # which is allowed by stream window but blocked by connection window.
        # They will be sent when connection window open
        self.blocked_send_frames = []

        # Values for the settings used on an HTTP/2 connection.
        # will send to remote using Setting Frame
        self.local_settings = {
            SettingsFrame.INITIAL_WINDOW_SIZE: 16 * 1024 * 1024,
            SettingsFrame.SETTINGS_MAX_FRAME_SIZE: 256 * 1024
        }
        self.local_connection_initial_windows = 32 * 1024 * 1024
        self.local_window_manager = FlowControlManager(self.local_connection_initial_windows)

        # changed by server, with SettingFrame
        self.remote_settings = {
            SettingsFrame.INITIAL_WINDOW_SIZE: DEFAULT_WINDOW_SIZE,
            SettingsFrame.SETTINGS_MAX_FRAME_SIZE: DEFAULT_MAX_FRAME,
            SettingsFrame.MAX_CONCURRENT_STREAMS: 100
        }

        #self.remote_window_size = DEFAULT_WINDOW_SIZE
        self.remote_window_size = 32 * 1024 * 1024

        # send Setting frame before accept task.
        self._send_preamble()

        threading.Thread(target=self.send_loop).start()
        threading.Thread(target=self.recv_loop).start()

    # export api
    def request(self, task):
        if not self.keep_running:
            # race condition
            self.retry_task_cb(task)
            return

        if len(self.streams) > self.config.http2_max_concurrent:
            self.accept_task = False

        task.set_state("h2_req")
        self.request_task(task)

    def encode_header(self, headers):
        return self.encoder.encode(headers)

    def request_task(self, task):
        with self.request_lock:
            # create stream to process task
            stream_id = self.next_stream_id

            # http/2 client use odd stream_id
            self.next_stream_id += 2

            stream = Stream(self.logger, self.config, self, self.ip, stream_id, task,
                        self._send_cb, self._close_stream_cb, self.encode_header, self.decoder,
                        FlowControlManager(self.local_settings[SettingsFrame.INITIAL_WINDOW_SIZE]),
                        self.remote_settings[SettingsFrame.INITIAL_WINDOW_SIZE],
                        self.remote_settings[SettingsFrame.SETTINGS_MAX_FRAME_SIZE])
            self.streams[stream_id] = stream
            stream.start_request()

    def send_loop(self):
        while self.keep_running:
            frame = self.send_queue.get(True)
            if not frame:
                # None frame means exist
                break

            if self.config.http2_show_debug:
                self.logger.debug("%s Send:%s", self.ip, str(frame))
            data = frame.serialize()
            try:
                self._sock.send(data, flush=False)
                # don't flush for small package
                # reduce send api call

                if self.send_queue._qsize():
                    continue

                # wait for payload frame
                time.sleep(0.01)
                # combine header and payload in one tcp package.
                if not self.send_queue._qsize():
                    self._sock.flush()

                self.last_send_time = time.time()
            except socket.error as e:
                if e.errno not in (errno.EPIPE, errno.ECONNRESET):
                    self.logger.warn("%s http2 send fail:%r", self.ip, e)
                else:
                    self.logger.exception("send error:%r", e)

                self.close("send fail:%r" % e)
            except Exception as e:
                self.logger.debug("http2 %s send error:%r", self.ip, e)
                self.close("send fail:%r" % e)

    def recv_loop(self):
        while self.keep_running:
            try:
                self._consume_single_frame()
            except Exception as e:
                self.logger.exception("recv fail:%r", e)
                self.close("recv fail:%r" % e)

    def get_rtt_rate(self):
        return self.rtt + len(self.streams) * 3000

    def close(self, reason="conn close"):
        self.keep_running = False
        self.accept_task = False
        # Notify loop to exit
        # This function may be call by out side http2
        # When gae_proxy found the appid or ip is wrong
        self.send_queue.put(None)

        for stream in self.streams.values():
            if stream.task.responsed:
                # response have send to client
                # can't retry
                stream.close(reason=reason)
            else:
                self.retry_task_cb(stream.task)
        self.streams = {}
        super(Http2Worker, self).close(reason)

    def send_ping(self):
        p = PingFrame(0)
        p.opaque_data = struct.pack("!d", time.time())
        self.send_queue.put(p)
        self.last_ping_time = time.time()
        self.ping_on_way += 1

    def _send_preamble(self):
        self.send_queue.put(RawFrame(b'PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n'))

        f = SettingsFrame(0)
        f.settings[SettingsFrame.ENABLE_PUSH] = 0
        f.settings[SettingsFrame.INITIAL_WINDOW_SIZE] = self.local_settings[SettingsFrame.INITIAL_WINDOW_SIZE]
        f.settings[SettingsFrame.SETTINGS_MAX_FRAME_SIZE] = self.local_settings[SettingsFrame.SETTINGS_MAX_FRAME_SIZE]
        self._send_cb(f)

        # update local connection windows size
        f = WindowUpdateFrame(0)
        f.window_increment = self.local_connection_initial_windows - DEFAULT_WINDOW_SIZE
        self._send_cb(f)

    def increase_remote_window_size(self, inc_size):
        # check and send blocked frames if window allow
        self.remote_window_size += inc_size
        #self.logger.debug("%s increase send win:%d result:%d", self.ip, inc_size, self.remote_window_size)
        while len(self.blocked_send_frames):
            frame = self.blocked_send_frames[0]
            if len(frame.data) > self.remote_window_size:
                return

            self.remote_window_size -= len(frame.data)
            self.send_queue.put(frame)
            self.blocked_send_frames.pop(0)

        if self.keep_running and \
                self.accept_task == False and \
                len(self.streams) < self.config.http2_max_concurrent and \
                self.remote_window_size > 10000:
            self.accept_task = True
            self.idle_cb()

    def _send_cb(self, frame):
        # can called by stream
        # put to send_blocked if connection window not allow,
        if frame.type == DataFrame.type:
            if len(frame.data) > self.remote_window_size:
                self.blocked_send_frames.append(frame)
                self.accept_task = False
                return
            else:
                self.remote_window_size -= len(frame.data)
                self.send_queue.put(frame)
        else:
            self.send_queue.put(frame)

    def _close_stream_cb(self, stream_id, reason):
        # call by stream to remove from streams list
        # self.logger.debug("%s close stream:%d %s", self.ssl_sock.ip, stream_id, reason)
        try:
            del self.streams[stream_id]
        except KeyError:
            pass

        if self.keep_running and \
                len(self.streams) < self.config.http2_max_concurrent and \
                self.remote_window_size > 10000:
            self.accept_task = True
            self.idle_cb()

        self.processed_tasks += 1

    def _consume_single_frame(self):
        try:
            header = self._sock.recv(9)
        except Exception as e:
            self.logger.debug("%s _consume_single_frame:%r, inactive time:%d", self.ip, e, time.time() - self.last_recv_time)
            self.close("ConnectionReset:%r" % e)
            return
        self.last_recv_time = time.time()

        # Parse the header. We can use the returned memoryview directly here.
        frame, length = Frame.parse_frame_header(header)

        if length > FRAME_MAX_ALLOWED_LEN:
            self.logger.error("%s Frame size exceeded on stream %d (received: %d, max: %d)",
                self.ip, frame.stream_id, length, FRAME_MAX_LEN)
            # self._send_rst_frame(frame.stream_id, 6) # 6 = FRAME_SIZE_ERROR

        try:
            data = self._recv_payload(length)
        except Exception as e:
            self.close("ConnectionReset:%r" % e)
            return

        self._consume_frame_payload(frame, data)

    def _recv_payload(self, length):
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
            self.last_recv_time = time.time()
            data_length = len(data)
            end = index + data_length
            buffer_view[index:end] = data[:]
            length -= data_length
            index = end

        return buffer_view[:end]

    def _consume_frame_payload(self, frame, data):
        frame.parse_body(data)

        if self.config.http2_show_debug:
            self.logger.debug("%s Recv:%s", self.ip, str(frame))

        # Maintain our flow control window. We do this by delegating to the
        # chosen WindowManager.
        if frame.type == DataFrame.type:

            size = frame.flow_controlled_length
            increment = self.local_window_manager._handle_frame(size)

            if increment < 0:
                self.logger.warn("increment:%d", increment)
            elif increment:
                #self.logger.debug("%s frame size:%d increase win:%d", self.ip, size, increment)
                w = WindowUpdateFrame(0)
                w.window_increment = increment
                self._send_cb(w)

        elif frame.type == PushPromiseFrame.type:
            self.logger.error("%s receive push frame", self.ip,)

        # Work out to whom this frame should go.
        if frame.stream_id != 0:
            try:
                stream = self.streams[frame.stream_id]
                stream.receive_frame(frame)
            except KeyError as e:
                if frame.type not in [WindowUpdateFrame.type]:
                    self.logger.exception("%s Unexpected stream identifier %d, frame.type:%s e:%r",
                                   self.ip, frame.stream_id, frame, e)
        else:
            self.receive_frame(frame)

    def receive_frame(self, frame):
        if frame.type == WindowUpdateFrame.type:
            # self.logger.debug("WindowUpdateFrame %d", frame.window_increment)
            self.increase_remote_window_size(frame.window_increment)

        elif frame.type == PingFrame.type:
            if 'ACK' in frame.flags:
                ping_time = struct.unpack("!d", frame.opaque_data)[0]
                time_now = time.time()
                rtt = (time_now - ping_time) * 1000
                if rtt < 0:
                    self.logger.error("rtt:%f ping_time:%f now:%f", rtt, ping_time, time_now)
                self.rtt = rtt
                self.ping_on_way -= 1
                #self.logger.debug("RTT:%d, on_way:%d", self.rtt, self.ping_on_way)
                if self.keep_running and self.ping_on_way == 0:
                    self.accept_task = True
            else:
                # The spec requires us to reply with PING+ACK and identical data.
                p = PingFrame(0)
                p.flags.add('ACK')
                p.opaque_data = frame.opaque_data
                self._send_cb(p)

        elif frame.type == SettingsFrame.type:
            if 'ACK' not in frame.flags:
                # send ACK as soon as possible
                f = SettingsFrame(0)
                f.flags.add('ACK')
                self._send_cb(f)

                # this may trigger send DataFrame blocked by remote window
                self._update_settings(frame)
            else:
                self.accept_task = True
                self.idle_cb()

        elif frame.type == GoAwayFrame.type:
            # If we get GoAway with error code zero, we are doing a graceful
            # shutdown and all is well. Otherwise, throw an exception.

            # If an error occured, try to read the error description from
            # code registry otherwise use the frame's additional data.
            error_string = frame._extra_info()
            time_cost = time.time() - self.last_recv_time
            if frame.additional_data != "session_timed_out":
                self.logger.warn("goaway:%s, t:%d", error_string, time_cost)

            self.close("GoAway:%s inactive time:%d" % (error_string, time_cost))

        elif frame.type == BlockedFrame.type:
            self.logger.warn("%s get BlockedFrame", self.ip)
        elif frame.type in FRAMES:
            # This frame isn't valid at this point.
            #raise ValueError("Unexpected frame %s." % frame)
            self.logger.error("%s Unexpected frame %s.", self.ip, frame)
        else:  # pragma: no cover
            # Unexpected frames belong to extensions. Just drop it on the
            # floor, but log so that users know that something happened.
            self.logger.error("%s Received unknown frame, type %d", self.ip, frame.type)

    def _update_settings(self, frame):
        if SettingsFrame.HEADER_TABLE_SIZE in frame.settings:
            new_size = frame.settings[SettingsFrame.HEADER_TABLE_SIZE]

            self.remote_settings[SettingsFrame.HEADER_TABLE_SIZE] = new_size
            #self.encoder.header_table_size = new_size

        if SettingsFrame.INITIAL_WINDOW_SIZE in frame.settings:
            newsize = frame.settings[SettingsFrame.INITIAL_WINDOW_SIZE]
            oldsize = self.remote_settings[SettingsFrame.INITIAL_WINDOW_SIZE]
            delta = newsize - oldsize

            for stream in self.streams.values():
                stream.remote_window_size += delta

            self.remote_settings[SettingsFrame.INITIAL_WINDOW_SIZE] = newsize

        if SettingsFrame.SETTINGS_MAX_FRAME_SIZE in frame.settings:
            new_size = frame.settings[SettingsFrame.SETTINGS_MAX_FRAME_SIZE]
            if not (FRAME_MAX_LEN <= new_size <= FRAME_MAX_ALLOWED_LEN):
                self.logger.error("%s Frame size %d is outside of allowed range", self.ip, new_size)

                # Tear the connection down with error code PROTOCOL_ERROR
                self.close("bad max frame size")
                #error_string = ("Advertised frame size %d is outside of range" % (new_size))
                #raise ConnectionError(error_string)
                return

            self.remote_settings[SettingsFrame.SETTINGS_MAX_FRAME_SIZE] = new_size

            for stream in self.streams.values():
                stream.max_frame_size += new_size

    def get_trace(self):
        out_list = []
        out_list.append(" continue_timeout:%d" % self.continue_timeout)
        out_list.append(" processed:%d" % self.processed_tasks)
        out_list.append(" h2.stream_num:%d" % len(self.streams))
        out_list.append(" sni:%s, host:%s" % (self.ssl_sock.sni, self.ssl_sock.host))
        return ",".join(out_list)

    def check_active(self, now):
        if not self.keep_running or len(self.streams) == 0:
            return

        for sid in self.streams.keys():
            try:
                stream = self.streams[sid]
                stream.check_timeout(now)
            except:
                pass

        if len(self.streams) > 0 and\
                now - self.last_send_time > 3 and \
                now - self.last_ping_time > self.config.http2_ping_min_interval:

            if self.ping_on_way > 0:
                self.close("active timeout")
                return

            self.send_ping()

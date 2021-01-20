# -*- coding: utf-8 -*-
"""
hyperframe/frame
~~~~~~~~~~~~~~~~

Defines framing logic for HTTP/2. Provides both classes to represent framed
data and logic for aiding the connection when it comes to reading from the
socket.
"""
import collections
import xstruct as struct

from .flags import Flag, Flags
from ...http20 import errors

# The maximum initial length of a frame. Some frames have shorter maximum lengths.
FRAME_MAX_LEN = (2 ** 14)

# The maximum allowed length of a frame.
FRAME_MAX_ALLOWED_LEN = (2 ** 24) - 1


class Frame(object):
    """
    The base class for all HTTP/2 frames.
    """
    # The flags defined on this type of frame.
    defined_flags = []

    # The type of the frame.
    type = None

    # If 'has-stream', the frame's stream_id must be non-zero. If 'no-stream',
    # it must be zero. If 'either', it's not checked.
    stream_association = None

    def __init__(self, stream_id, flags=()):
        self.stream_id = stream_id
        self.flags = Flags(self.defined_flags)
        self.body_len = 0

        for flag in flags:
            self.flags.add(flag)

        if self.stream_association == 'has-stream' and not self.stream_id:
            raise ValueError('Stream ID must be non-zero')
        if self.stream_association == 'no-stream' and self.stream_id:
            raise ValueError('Stream ID must be zero')

    def _extra_info(self):
        return ""

    def __repr__(self):
        out_str = "{type}".format(type=type(self).__name__)
        if self.stream_id:
            out_str += " %d" % self.stream_id

        if len(self.flags):
            out_str += " F:" + ", ".join(self.flags)

        extra_str = self._extra_info()
        if extra_str:
            out_str += " " + extra_str

        return out_str

    @staticmethod
    def parse_frame_header(header):
        """
        Takes a 9-byte frame header and returns a tuple of the appropriate
        Frame object and the length that needs to be read from the socket.
        """
        fields = struct.unpack("!HBBBL", header)
        # First 24 bits are frame length.
        length = (fields[0] << 8) + fields[1]
        type = fields[2]
        flags = fields[3]
        stream_id = fields[4]

        if type not in FRAMES:
            raise ValueError("Unknown frame type %d" % type)

        frame = FRAMES[type](stream_id)
        frame.parse_flags(flags)
        return frame, length

    def parse_flags(self, flag_byte):
        for flag, flag_bit in self.defined_flags:
            if flag_byte & flag_bit:
                self.flags.add(flag)

        return self.flags

    def serialize(self):
        body = self.serialize_body()
        self.body_len = len(body)

        # Build the common frame header.
        # First, get the flags.
        flags = 0

        for flag, flag_bit in self.defined_flags:
            if flag in self.flags:
                flags |= flag_bit

        header = struct.pack(
            "!HBBBL",
            (self.body_len & 0xFFFF00) >> 8,  # Length is spread over top 24 bits
            self.body_len & 0x0000FF,
            self.type,
            flags,
            self.stream_id & 0x7FFFFFFF  # Stream ID is 32 bits.
        )

        return header + body

    def serialize_body(self):
        raise NotImplementedError()

    def parse_body(self, data):
        raise NotImplementedError()


class Padding(object):
    """
    Mixin for frames that contain padding.
    """
    def __init__(self, stream_id, pad_length=0, **kwargs):
        super(Padding, self).__init__(stream_id, **kwargs)

        self.pad_length = pad_length

    def serialize_padding_data(self):
        if 'PADDED' in self.flags:
            return struct.pack('!B', self.pad_length)
        return b''

    def parse_padding_data(self, data):
        if 'PADDED' in self.flags:
            self.pad_length = struct.unpack('!B', data[:1])[0]
            return 1
        return 0

    @property
    def total_padding(self):
        """Return the total length of the padding, if any."""
        return self.pad_length

    def _extra_info(self):
        if self.pad_length:
            return "pad_len:%d" % self.pad_length
        else:
            return ""


class Priority(object):
    """
    Mixin for frames that contain priority data.
    """
    def __init__(self, stream_id, depends_on=None, stream_weight=None, exclusive=None, **kwargs):
        super(Priority, self).__init__(stream_id, **kwargs)

        # The stream ID of the stream on which this stream depends.
        self.depends_on = depends_on

        # The weight of the stream. This is an integer between 0 and 256.
        self.stream_weight = stream_weight

        # Whether the exclusive bit was set.
        self.exclusive = exclusive

    def serialize_priority_data(self):
        return struct.pack(
            "!LB",
            self.depends_on | (int(self.exclusive) << 31),
            self.stream_weight
        )

    def parse_priority_data(self, data):
        MASK = 0x80000000
        self.depends_on, self.stream_weight = struct.unpack(
            "!LB", data[:5]
        )
        self.exclusive = bool(self.depends_on & MASK)
        self.depends_on &= ~MASK
        return 5


class DataFrame(Padding, Frame):
    """
    DATA frames convey arbitrary, variable-length sequences of octets
    associated with a stream. One or more DATA frames are used, for instance,
    to carry HTTP request or response payloads.
    """
    defined_flags = [
        Flag('END_STREAM', 0x01),
        Flag('PADDED', 0x08),
    ]

    type = 0x0

    stream_association = 'has-stream'

    def __init__(self, stream_id, data=b'', **kwargs):
        super(DataFrame, self).__init__(stream_id, **kwargs)

        self.data = data

    def serialize_body(self):
        padding_data = self.serialize_padding_data()
        padding = b'\0' * self.total_padding
        return b''.join([padding_data, self.data, padding])

    def parse_body(self, data):
        padding_data_length = self.parse_padding_data(data)
        self.data = data[padding_data_length:len(data)-self.total_padding]
        self.body_len = len(data)

    @property
    def flow_controlled_length(self):
        """
        If the frame is padded we need to include the padding length byte in
        the flow control used.
        """
        padding_len = self.total_padding + 1 if self.total_padding else 0
        return len(self.data) + padding_len

    def _extra_info(self):
        return "len:%d" % len(self.data)


class PriorityFrame(Priority, Frame):
    """
    The PRIORITY frame specifies the sender-advised priority of a stream. It
    can be sent at any time for an existing stream. This enables
    reprioritisation of existing streams.
    """
    defined_flags = []

    type = 0x02

    stream_association = 'has-stream'

    def serialize_body(self):
        return self.serialize_priority_data()

    def parse_body(self, data):
        self.parse_priority_data(data)
        self.body_len = len(data)


class RstStreamFrame(Frame):
    """
    The RST_STREAM frame allows for abnormal termination of a stream. When sent
    by the initiator of a stream, it indicates that they wish to cancel the
    stream or that an error condition has occurred. When sent by the receiver
    of a stream, it indicates that either the receiver is rejecting the stream,
    requesting that the stream be cancelled or that an error condition has
    occurred.
    """
    defined_flags = []

    type = 0x03

    stream_association = 'has-stream'

    def __init__(self, stream_id, error_code=0, **kwargs):
        super(RstStreamFrame, self).__init__(stream_id, **kwargs)

        self.error_code = error_code

    def serialize_body(self):
        return struct.pack("!L", self.error_code)

    def parse_body(self, data):
        if len(data) != 4:
            raise ValueError()

        self.error_code = struct.unpack("!L", data)[0]
        self.body_len = len(data)

    def _extra_info(self):
        return "error_code:%d" % self.error_code


class SettingsFrame(Frame):
    """
    The SETTINGS frame conveys configuration parameters that affect how
    endpoints communicate. The parameters are either constraints on peer
    behavior or preferences.

    Settings are not negotiated. Settings describe characteristics of the
    sending peer, which are used by the receiving peer. Different values for
    the same setting can be advertised by each peer. For example, a client
    might set a high initial flow control window, whereas a server might set a
    lower value to conserve resources.
    """
    defined_flags = [Flag('ACK', 0x01)]

    type = 0x04

    stream_association = 'no-stream'

    # We need to define the known settings, they may as well be class
    # attributes.
    HEADER_TABLE_SIZE             = 0x01
    ENABLE_PUSH                   = 0x02
    MAX_CONCURRENT_STREAMS        = 0x03
    INITIAL_WINDOW_SIZE           = 0x04
    SETTINGS_MAX_FRAME_SIZE       = 0x05
    SETTINGS_MAX_HEADER_LIST_SIZE = 0x06

    def __init__(self, stream_id=0, settings=None, **kwargs):
        super(SettingsFrame, self).__init__(stream_id, **kwargs)

        if settings and "ACK" in kwargs.get("flags", ()):
            raise ValueError("Settings must be empty if ACK flag is set.")

        # A dictionary of the setting type byte to the value.
        self.settings = settings or {}

    def serialize_body(self):
        settings = [struct.pack("!HL", setting & 0xFF, value)
                    for setting, value in self.settings.items()]
        return b''.join(settings)

    def parse_body(self, data):
        for i in range(0, len(data), 6):
            name, value = struct.unpack("!HL", data[i:i+6])
            self.settings[name] = value

        self.body_len = len(data)

    def _extra_info(self):
        if not len(self.settings):
            return ""

        kv = []
        for k in self.settings:
            kv.append(str(k) + ":" + str(self.settings[k]))

        return ";".join(kv)


class PushPromiseFrame(Padding, Frame):
    """
    The PUSH_PROMISE frame is used to notify the peer endpoint in advance of
    streams the sender intends to initiate.
    """
    defined_flags = [
        Flag('END_HEADERS', 0x04),
        Flag('PADDED', 0x08)
    ]

    type = 0x05

    stream_association = 'has-stream'

    def __init__(self, stream_id, promised_stream_id=0, data=b'', **kwargs):
        super(PushPromiseFrame, self).__init__(stream_id, **kwargs)

        self.promised_stream_id = promised_stream_id
        self.data = data

    def serialize_body(self):
        padding_data = self.serialize_padding_data()
        padding = b'\0' * self.total_padding
        data = struct.pack("!L", self.promised_stream_id)
        return b''.join([padding_data, data, self.data, padding])

    def parse_body(self, data):
        padding_data_length = self.parse_padding_data(data)
        self.promised_stream_id = struct.unpack("!L", data[padding_data_length:padding_data_length + 4])[0]
        self.data = data[padding_data_length + 4:].tobytes()
        self.body_len = len(data)


class PingFrame(Frame):
    """
    The PING frame is a mechanism for measuring a minimal round-trip time from
    the sender, as well as determining whether an idle connection is still
    functional. PING frames can be sent from any endpoint.
    """
    defined_flags = [Flag('ACK', 0x01)]

    type = 0x06

    stream_association = 'no-stream'

    def __init__(self, stream_id=0, opaque_data=b'', **kwargs):
        super(PingFrame, self).__init__(stream_id, **kwargs)

        self.opaque_data = opaque_data

    def serialize_body(self):
        if len(self.opaque_data) > 8:
            raise ValueError()

        data = self.opaque_data
        data += b'\x00' * (8 - len(self.opaque_data))
        return data

    def parse_body(self, data):
        if len(data) > 8:
            raise ValueError()

        self.opaque_data = data.tobytes()
        self.body_len = len(data)


class GoAwayFrame(Frame):
    """
    The GOAWAY frame informs the remote peer to stop creating streams on this
    connection. It can be sent from the client or the server. Once sent, the
    sender will ignore frames sent on new streams for the remainder of the
    connection.
    """
    type = 0x07

    stream_association = 'no-stream'

    def __init__(self, stream_id=0, last_stream_id=0, error_code=0, additional_data=b'', **kwargs):
        super(GoAwayFrame, self).__init__(stream_id, **kwargs)

        self.last_stream_id = last_stream_id
        self.error_code = error_code
        self.additional_data = additional_data

    def serialize_body(self):
        data = struct.pack(
            "!LL",
            self.last_stream_id & 0x7FFFFFFF,
            self.error_code
        )
        data += self.additional_data

        return data

    def parse_body(self, data):
        self.last_stream_id, self.error_code = struct.unpack("!LL", data[:8])
        self.body_len = len(data)

        if len(data) > 8:
            self.additional_data = data[8:].tobytes()

    def _extra_info(self):
        if self.error_code != 0:
            try:
                name, number, description = errors.get_data(self.error_code)
            except ValueError:
                error_string = ("Encountered error code %d, extra data %s" % (self.error_code, self.additional_data))
            else:
                error_string = ("Encountered error %s %s: %s" % (name, number, description))
        else:
            error_string = ""

        out_str = ""
        if error_string:
            out_str += "error_string:%s" % error_string
        if self.additional_data:
            out_str += " additional_data:%s" % self.additional_data

        return out_str


class WindowUpdateFrame(Frame):
    """
    The WINDOW_UPDATE frame is used to implement flow control.

    Flow control operates at two levels: on each individual stream and on the
    entire connection.

    Both types of flow control are hop by hop; that is, only between the two
    endpoints. Intermediaries do not forward WINDOW_UPDATE frames between
    dependent connections. However, throttling of data transfer by any receiver
    can indirectly cause the propagation of flow control information toward the
    original sender.
    """
    type = 0x08

    stream_association = 'either'

    def __init__(self, stream_id, window_increment=0, **kwargs):
        super(WindowUpdateFrame, self).__init__(stream_id, **kwargs)

        self.window_increment = window_increment

    def serialize_body(self):
        return struct.pack("!L", self.window_increment & 0x7FFFFFFF)

    def parse_body(self, data):
        self.window_increment = struct.unpack("!L", data)[0]
        self.body_len = len(data)

    def _extra_info(self):
        return "win_inc:%d" % self.window_increment


class HeadersFrame(Padding, Priority, Frame):
    """
    The HEADERS frame carries name-value pairs. It is used to open a stream.
    HEADERS frames can be sent on a stream in the "open" or "half closed
    (remote)" states.

    The HeadersFrame class is actually basically a data frame in this
    implementation, because of the requirement to control the sizes of frames.
    A header block fragment that doesn't fit in an entire HEADERS frame needs
    to be followed with CONTINUATION frames. From the perspective of the frame
    building code the header block is an opaque data segment.
    """
    type = 0x01

    stream_association = 'has-stream'

    defined_flags = [
        Flag('END_STREAM', 0x01),
        Flag('END_HEADERS', 0x04),
        Flag('PADDED', 0x08),
        Flag('PRIORITY', 0x20),
    ]

    def __init__(self, stream_id, data=b'', **kwargs):
        super(HeadersFrame, self).__init__(stream_id, **kwargs)

        self.data = data

    def serialize_body(self):
        padding_data = self.serialize_padding_data()
        padding = b'\0' * self.total_padding

        if 'PRIORITY' in self.flags:
            priority_data = self.serialize_priority_data()
        else:
            priority_data = b''

        return b''.join([padding_data, priority_data, self.data, padding])

    def parse_body(self, data):
        padding_data_length = self.parse_padding_data(data)
        data = data[padding_data_length:]

        if 'PRIORITY' in self.flags:
            priority_data_length = self.parse_priority_data(data)
        else:
            priority_data_length = 0

        self.body_len = len(data)
        self.data = data[priority_data_length:len(data)-self.total_padding]


class ContinuationFrame(Frame):
    """
    The CONTINUATION frame is used to continue a sequence of header block
    fragments. Any number of CONTINUATION frames can be sent on an existing
    stream, as long as the preceding frame on the same stream is one of
    HEADERS, PUSH_PROMISE or CONTINUATION without the END_HEADERS flag set.

    Much like the HEADERS frame, hyper treats this as an opaque data frame with
    different flags and a different type.
    """
    type = 0x09

    stream_association = 'has-stream'

    defined_flags = [Flag('END_HEADERS', 0x04), ]

    def __init__(self, stream_id, data=b'', **kwargs):
        super(ContinuationFrame, self).__init__(stream_id, **kwargs)

        self.data = data

    def serialize_body(self):
        return self.data

    def parse_body(self, data):
        self.data = data
        self.body_len = len(data)


Origin = collections.namedtuple('Origin', ['scheme', 'host', 'port'])


class AltSvcFrame(Frame):
    """
    The ALTSVC frame is used to advertise alternate services that the current
    host, or a different one, can understand.
    """
    type = 0xA

    stream_association = 'no-stream'

    def __init__(self, stream_id=0, host=b'', port=0, protocol_id=b'', max_age=0, origin=None, **kwargs):
        super(AltSvcFrame, self).__init__(stream_id, **kwargs)

        self.host = host
        self.port = port
        self.protocol_id = protocol_id
        self.max_age = max_age
        self.origin = origin

    def serialize_origin(self):
        if self.origin is not None:
            if self.origin.port is None:
                hostport = self.origin.host
            else:
                hostport = self.origin.host + b':' + str(self.origin.port).encode('ascii')
            return self.origin.scheme + b'://' + hostport
        return b''

    def parse_origin(self, data):
        if len(data) > 0:
            data = data.tobytes()
            scheme, hostport = data.split(b'://')
            host, _, port = hostport.partition(b':')
            self.origin = Origin(scheme=scheme, host=host,
                                 port=int(port) if len(port) > 0 else None)

    def serialize_body(self):
        first = struct.pack("!LHxB", self.max_age, self.port, len(self.protocol_id))
        host_length = struct.pack("!B", len(self.host))
        return b''.join([first, self.protocol_id, host_length, self.host,
                         self.serialize_origin()])

    def parse_body(self, data):
        self.body_len = len(data)
        self.max_age, self.port, protocol_id_length = struct.unpack("!LHxB", data[:8])
        pos = 8
        self.protocol_id = data[pos:pos+protocol_id_length].tobytes()
        pos += protocol_id_length
        host_length = struct.unpack("!B", data[pos:pos+1])[0]
        pos += 1
        self.host = data[pos:pos+host_length].tobytes()
        pos += host_length
        self.parse_origin(data[pos:])


class BlockedFrame(Frame):
    """
    The BLOCKED frame indicates that the sender is unable to send data due to a
    closed flow control window.

    The BLOCKED frame is used to provide feedback about the performance of flow
    control for the purposes of performance tuning and debugging. The BLOCKED
    frame can be sent by a peer when flow controlled data cannot be sent due to
    the connection- or stream-level flow control. This frame MUST NOT be sent
    if there are other reasons preventing data from being sent, either a lack
    of available data, or the underlying transport being blocked.
    """
    type = 0x0B

    stream_association = 'both'

    defined_flags = []

    def serialize_body(self):
        return b''

    def parse_body(self, data):
        pass


# A map of type byte to frame class.
_FRAME_CLASSES = [
    DataFrame,
    HeadersFrame,
    PriorityFrame,
    RstStreamFrame,
    SettingsFrame,
    PushPromiseFrame,
    PingFrame,
    GoAwayFrame,
    WindowUpdateFrame,
    ContinuationFrame,
    AltSvcFrame,
    BlockedFrame
]
FRAMES = {cls.type: cls for cls in _FRAME_CLASSES}

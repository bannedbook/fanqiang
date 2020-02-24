# -*- coding: utf-8 -*-
"""
hyper/http20/response
~~~~~~~~~~~~~~~~~~~~~

Contains the HTTP/2 equivalent of the HTTPResponse object defined in
httplib/http.client.
"""
import logging
import zlib

from ..common.decoder import DeflateDecoder
from ..common.headers import HTTPHeaderMap

log = logging.getLogger(__name__)


def strip_headers(headers):
    """
    Strips the headers attached to the instance of any header beginning
    with a colon that ``hyper`` doesn't understand. This method logs at
    warning level about the deleted headers, for discoverability.
    """
    # Convert to list to ensure that we don't mutate the headers while
    # we iterate over them.
    for name in list(headers.keys()):
        if name.startswith(b':'):
            del headers[name]


class HTTP20Response(object):
    """
    An ``HTTP20Response`` wraps the HTTP/2 response from the server. It
    provides access to the response headers and the entity body. The response
    is an iterable object and can be used in a with statement (though due to
    the persistent connections used in HTTP/2 this has no effect, and is done
    soley for compatibility).
    """
    def __init__(self, headers, stream):
        #: The reason phrase returned by the server. This is not used in
        #: HTTP/2, and so is always the empty string.
        self.reason = ''

        status = headers[b':status'][0]
        strip_headers(headers)

        #: The status code returned by the server.
        self.status = int(status)

        #: The response headers. These are determined upon creation, assigned
        #: once, and never assigned again.
        self.headers = headers

        # The response trailers. These are always intially ``None``.
        self._trailers = None

        # The stream this response is being sent over.
        self._stream = stream

        # We always read in one-data-frame increments from the stream, so we
        # may need to buffer some for incomplete reads.
        self._data_buffer = b''

        # This object is used for decompressing gzipped request bodies. Right
        # now we only support gzip because that's all the RFC mandates of us.
        # Later we'll add support for more encodings.
        # This 16 + MAX_WBITS nonsense is to force gzip. See this
        # Stack Overflow answer for more:
        # http://stackoverflow.com/a/2695466/1401686
        if b'gzip' in self.headers.get(b'content-encoding', []):
            self._decompressobj = zlib.decompressobj(16 + zlib.MAX_WBITS)
        elif b'deflate' in self.headers.get(b'content-encoding', []):
            self._decompressobj = DeflateDecoder()
        else:
            self._decompressobj = None

    @property
    def trailers(self):
        """
        Trailers on the HTTP message, if any.

        .. warning:: Note that this property requires that the stream is
                     totally exhausted. This means that, if you have not
                     completely read from the stream, all stream data will be
                     read into memory.
        """
        if self._trailers is None:
            self._trailers = self._stream.gettrailers() or HTTPHeaderMap()
            strip_headers(self._trailers)

        return self._trailers

    def read(self, amt=None, decode_content=True):
        """
        Reads the response body, or up to the next ``amt`` bytes.

        :param amt: (optional) The amount of data to read. If not provided, all
            the data will be read from the response.
        :param decode_content: (optional) If ``True``, will transparently
            decode the response data.
        :returns: The read data. Note that if ``decode_content`` is set to
            ``True``, the actual amount of data returned may be different to
            the amount requested.
        """
        if amt is not None and amt <= len(self._data_buffer):
            data = self._data_buffer[:amt]
            self._data_buffer = self._data_buffer[amt:]
            response_complete = False
        elif amt is not None:
            read_amt = amt - len(self._data_buffer)
            self._data_buffer += self._stream._read(read_amt)
            data = self._data_buffer[:amt]
            self._data_buffer = self._data_buffer[amt:]
            response_complete = len(data) < amt
        else:
            data = b''.join([self._data_buffer, self._stream._read()])
            response_complete = True

        # We may need to decode the body.
        if decode_content and self._decompressobj and data:
            data = self._decompressobj.decompress(data)

        # If we're at the end of the request, we have some cleaning up to do.
        # Close the stream, and if necessary flush the buffer.
        if response_complete:
            if decode_content and self._decompressobj:
                data += self._decompressobj.flush()

            if self._stream.response_headers:
                self.headers.merge(self._stream.response_headers)

        # We're at the end. Close the connection.
        if not data:
            self.close()

        return data

    def read_chunked(self, decode_content=True):
        """
        Reads chunked transfer encoded bodies. This method returns a generator:
        each iteration of which yields one data frame *unless* the frames
        contain compressed data and ``decode_content`` is ``True``, in which
        case it yields whatever the decompressor provides for each chunk.

        .. warning:: This may yield the empty string, without that being the
                     end of the body!
        """
        while True:
            data = self._stream._read_one_frame()

            if data is None:
                break

            if decode_content and self._decompressobj:
                data = self._decompressobj.decompress(data)

            yield data

        if decode_content and self._decompressobj:
            yield self._decompressobj.flush()

        self.close()

        return

    def fileno(self):
        """
        Return the ``fileno`` of the underlying socket. This function is
        currently not implemented.
        """
        raise NotImplementedError("Not currently implemented.")

    def close(self):
        """
        Close the response. In effect this closes the backing HTTP/2 stream.

        :returns: Nothing.
        """
        self._stream.close()

    # The following methods implement the context manager protocol.
    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()
        return False  # Never swallow exceptions.


class HTTP20Push(object):
    """
    Represents a request-response pair sent by the server through the server
    push mechanism.
    """
    def __init__(self, request_headers, stream):
        #: The scheme of the simulated request
        self.scheme = request_headers[b':scheme'][0]
        #: The method of the simulated request (must be safe and cacheable, e.g. GET)
        self.method = request_headers[b':method'][0]
        #: The authority of the simulated request (usually host:port)
        self.authority = request_headers[b':authority'][0]
        #: The path of the simulated request
        self.path = request_headers[b':path'][0]

        strip_headers(request_headers)

        #: The headers the server attached to the simulated request.
        self.request_headers = request_headers

        self._stream = stream

    def get_response(self):
        """
        Get the pushed response provided by the server.

        :returns: A :class:`HTTP20Response <hyper.HTTP20Response>` object
            representing the pushed response.
        """
        return HTTP20Response(self._stream.getheaders(), self._stream)

    def cancel(self):
        """
        Cancel the pushed response and close the stream.

        :returns: Nothing.
        """
        self._stream.close(8) # CANCEL

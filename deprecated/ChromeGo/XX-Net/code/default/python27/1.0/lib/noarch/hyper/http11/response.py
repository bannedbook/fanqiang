# -*- coding: utf-8 -*-
"""
hyper/http11/response
~~~~~~~~~~~~~~~~~~~~~

Contains the HTTP/1.1 equivalent of the HTTPResponse object defined in
httplib/http.client.
"""
import logging
import weakref
import zlib

from ..common.decoder import DeflateDecoder
from ..common.exceptions import ChunkedDecodeError, InvalidResponseError
from ..common.exceptions import ConnectionResetError

log = logging.getLogger(__name__)


class HTTP11Response(object):
    """
    An ``HTTP11Response`` wraps the HTTP/1.1 response from the server. It
    provides access to the response headers and the entity body. The response
    is an iterable object and can be used in a with statement.
    """
    def __init__(self, code, reason, headers, sock, connection=None):
        #: The reason phrase returned by the server.
        self.reason = reason

        #: The status code returned by the server.
        self.status = code

        #: The response headers. These are determined upon creation, assigned
        #: once, and never assigned again.
        self.headers = headers

        #: The response trailers. These are always intially ``None``.
        self.trailers = None

        # The socket this response is being sent over.
        self._sock = sock

        # Whether we expect the connection to be closed. If we do, we don't
        # bother checking for content-length, we just keep reading until
        # we no longer can.
        self._expect_close = False
        if b'close' in self.headers.get(b'connection', []):
            self._expect_close = True

        # The expected length of the body.
        try:
            self._length = int(self.headers[b'content-length'][0])
        except KeyError:
            self._length = None

        # Whether we expect a chunked response.
        self._chunked = b'chunked' in self.headers.get(b'transfer-encoding', [])

        # One of the following must be true: we must expect that the connection
        # will be closed following the body, or that a content-length was sent,
        # or that we're getting a chunked response.
        # FIXME: Remove naked assert, replace with something better.
        assert self._expect_close or self._length is not None or self._chunked

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

        # This is a reference that allows for the Response class to tell the
        # parent connection object to throw away its socket object. This is to
        # be used when the connection is genuinely closed, so that the user
        # can keep using the Connection object.
        # Strictly, we take a weakreference to this so that we don't set up a
        # reference cycle.
        if connection is not None:
            self._parent = weakref.ref(connection)
        else:
            self._parent = None

        self._buffered_data = b''
        self._chunker = None

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
        # Return early if we've lost our connection.
        if self._sock is None:
            return b''

        if self._chunked:
            return self._normal_read_chunked(amt, decode_content)

        # If we're asked to do a read without a length, we need to read
        # everything. That means either the entire content length, or until the
        # socket is closed, depending.
        if amt is None:
            if self._length is not None:
                amt = self._length
            elif self._expect_close:
                return self._read_expect_closed(decode_content)
            else:  # pragma: no cover
                raise InvalidResponseError(
                    "Response must either have length or Connection: close"
                )

        # Otherwise, we've been asked to do a bounded read. We should read no
        # more than the remaining length, obviously.
        # FIXME: Handle cases without _length
        if self._length is not None:
            amt = min(amt, self._length)

        # If we are now going to read nothing, exit early. We still need to
        # close the socket.
        if not amt:
            self.close(socket_close=self._expect_close)
            return b''

        # Now, issue reads until we read that length. This is to account for
        # the fact that it's possible that we'll be asked to read more than
        # 65kB in one shot.
        to_read = amt
        chunks = []

        # Ideally I'd like this to read 'while to_read', but I want to be
        # defensive against the admittedly unlikely case that the socket
        # returns *more* data than I want.
        while to_read > 0:
            chunk = self._sock.recv(amt).tobytes()

            # If we got an empty read, but were expecting more, the remote end
            # has hung up. Raise an exception if we were expecting more data,
            # but if we were expecting the remote end to close then it's ok.
            if not chunk:
                if self._length is not None or not self._expect_close:
                    self.close(socket_close=True)
                    raise ConnectionResetError("Remote end hung up!")

                break

            to_read -= len(chunk)
            chunks.append(chunk)

        data = b''.join(chunks)
        if self._length is not None:
            self._length -= len(data)

        # If we're at the end of the request, we have some cleaning up to do.
        # Close the stream, and if necessary flush the buffer. Checking that
        # we're at the end is actually obscenely complex: either we've read the
        # full content-length or, if we were expecting a closed connection,
        # we've had a read shorter than the requested amount. We also have to
        # do this before we try to decompress the body.
        end_of_request = (self._length == 0 or
                          (self._expect_close and len(data) < amt))

        # We may need to decode the body.
        if decode_content and self._decompressobj and data:
            data = self._decompressobj.decompress(data)

        if decode_content and self._decompressobj and end_of_request:
            data += self._decompressobj.flush()

        # We're at the end. Close the connection. Explicit check for zero here
        # because self._length might be None.
        if end_of_request:
            self.close(socket_close=self._expect_close)

        return data

    def read_chunked(self, decode_content=True):
        """
        Reads chunked transfer encoded bodies. This method returns a generator:
        each iteration of which yields one chunk *unless* the chunks are
        compressed, in which case it yields whatever the decompressor provides
        for each chunk.

        .. warning:: This may yield the empty string, without that being the
                     end of the body!
        """
        if not self._chunked:
            raise ChunkedDecodeError(
                "Attempted chunked read of non-chunked body."
            )

        # Return early if possible.
        if self._sock is None:
            return

        while True:
            # Read to the newline to get the chunk length. This is a
            # hexadecimal integer.
            chunk_length = int(self._sock.readline().tobytes().strip(), 16)
            data = b''

            # If the chunk length is zero, consume the newline and then we're
            # done. If we were decompressing data, return the remaining data.
            if not chunk_length:
                self._sock.readline()

                if decode_content and self._decompressobj:
                    yield self._decompressobj.flush()

                self.close(socket_close=self._expect_close)
                break

            # Then read that many bytes.
            while chunk_length > 0:
                chunk = self._sock.recv(chunk_length).tobytes()
                data += chunk
                chunk_length -= len(chunk)

            assert chunk_length == 0

            # Now, consume the newline.
            self._sock.readline()

            # We may need to decode the body.
            if decode_content and self._decompressobj and data:
                data = self._decompressobj.decompress(data)

            yield data

        return

    def close(self, socket_close=False):
        """
        Close the response. This causes the Response to lose access to the
        backing socket. In some cases, it can also cause the backing connection
        to be torn down.

        :param socket_close: Whether to close the backing socket.
        :returns: Nothing.
        """
        if socket_close and self._parent is not None:
            # The double call is necessary because we need to dereference the
            # weakref. If the weakref is no longer valid, that's fine, there's
            # no connection object to tell.
            parent = self._parent()
            if parent is not None:
                parent.close()

        self._sock = None

    def _read_expect_closed(self, decode_content):
        """
        Implements the logic for an unbounded read on a socket that we expect
        to be closed by the remote end.
        """
        # In this case, just read until we cannot read anymore. Then, close the
        # socket, becuase we know we have to.
        chunks = []
        while True:
            try:
                chunk = self._sock.recv(65535).tobytes()
                if not chunk:
                    break
            except ConnectionResetError:
                break
            else:
                chunks.append(chunk)

        self.close(socket_close=True)

        # We may need to decompress the data.
        data = b''.join(chunks)
        if decode_content and self._decompressobj:
            data = self._decompressobj.decompress(data)
            data += self._decompressobj.flush()

        return data

    def _normal_read_chunked(self, amt, decode_content):
        """
        Implements the logic for calling ``read()`` on a chunked response.
        """
        # If we're doing a full read, read it as chunked and then just join
        # the chunks together!
        if amt is None:
            return self._buffered_data + b''.join(self.read_chunked())

        if self._chunker is None:
            self._chunker = self.read_chunked()

        # Otherwise, we have a certain amount of data we want to read.
        current_amount = len(self._buffered_data)

        extra_data = [self._buffered_data]
        while current_amount < amt:
            try:
                chunk = next(self._chunker)
            except StopIteration:
                self.close(socket_close=self._expect_close)
                break

            current_amount += len(chunk)
            extra_data.append(chunk)

        data = b''.join(extra_data)
        self._buffered_data = data[amt:]
        return data[:amt]

    # The following methods implement the context manager protocol.
    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()
        return False  # Never swallow exceptions.

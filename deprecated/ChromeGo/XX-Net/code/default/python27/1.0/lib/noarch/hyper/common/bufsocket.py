# -*- coding: utf-8 -*-
"""
hyper/http20/bufsocket.py
~~~~~~~~~~~~~~~~~~~~~~~~~

This file implements a buffered socket wrapper.

The purpose of this is to avoid the overhead of unnecessary syscalls while
allowing small reads from the network. This represents a potentially massive
performance optimisation at the cost of burning some memory in the userspace
process.
"""
import select
from .exceptions import ConnectionResetError, LineTooLongError
# import logging
# logger = logging.getLogger()
# logger.setLevel(logging.DEBUG)


class WriteBuffer(object):
    def __init__(self, s=None):
        if isinstance(s, str):
            self.string_len = len(s)
            self.buffer_list = [s]
        else:
            self.reset()

    def reset(self):
        self.buffer_list = []
        self.string_len = 0

    def __len__(self):
        return self.string_len

    def __add__(self, other):
        self.append(other)
        return self

    def insert(self, s):
        if isinstance(s, WriteBuffer):
            self.buffer_list = s.buffer_list + self.buffer_list
            self.string_len += s.string_len
        elif isinstance(s, str):
            self.buffer_list.insert(0, s)
            self.string_len += len(s)
        else:
            raise Exception("WriteBuffer append not string or StringBuffer")

    def append(self, s):
        if isinstance(s, WriteBuffer):
            self.buffer_list.extend(s.buffer_list)
            self.string_len += s.string_len
        elif isinstance(s, str):
            self.buffer_list.append(s)
            self.string_len += len(s)
        else:
            raise Exception("WriteBuffer append not string or StringBuffer")

    def __str__(self):
        return self.get_string()

    def get_string(self):
        return "".join(self.buffer_list)


class BufferedSocket(object):
    """
    A buffered socket wrapper.

    The purpose of this is to avoid the overhead of unnecessary syscalls while
    allowing small reads from the network. This represents a potentially
    massive performance optimisation at the cost of burning some memory in the
    userspace process.
    """
    def __init__(self, sck, buffer_size=1000):
        """
        Create the buffered socket.

        :param sck: The socket to wrap.
        :param buffer_size: The size of the backing buffer in bytes. This
            parameter should be set to an appropriate value for your use case.
            Small values of ``buffer_size`` increase the overhead of buffer
            management: large values cause more memory to be used.
        """
        # The wrapped socket.
        self._sck = sck

        # The buffer we're using.
        self._backing_buffer = bytearray(buffer_size)
        self._buffer_view = memoryview(self._backing_buffer)

        # The size of the buffer.
        self._buffer_size = buffer_size

        # The start index in the memory view.
        self._index = 0

        # The number of bytes in the buffer.
        self._bytes_in_buffer = 0

        # following is define for send buffer
        # all send will be cache and send when flush called,
        # combine data to reduce the api call
        self.send_buffer = WriteBuffer()

    def send(self, buf, flush=True):
        self.send_buffer.append(buf)

        if len(self.send_buffer) > 1300 or flush:
            self.flush()

    def flush(self):
        if len(self.send_buffer):
            data = self.send_buffer.get_string()
            # logger.debug("buffer socket flush:%d", len(data))
            self.send_buffer.reset()

            data_len = len(data)
            start = 0
            while start < data_len:
                send_size = min(data_len - start, 65535)
                sended = self._sck.send(data[start:start+send_size])
                start += sended

    @property
    def _remaining_capacity(self):
        """
        The maximum number of bytes the buffer could still contain.
        """
        return self._buffer_size - self._index

    @property
    def _buffer_end(self):
        """
        The index of the first free byte in the buffer.
        """
        return self._index + self._bytes_in_buffer

    @property
    def can_read(self):
        """
        Whether or not there is more data to read from the socket.
        """
        if self._bytes_in_buffer:
            return True

        read = select.select([self._sck], [], [], 0)[0]
        if read:
            return True

        return False

    @property
    def buffer(self):
        """
        Get access to the buffer itself.
        """
        return self._buffer_view[self._index:self._buffer_end]

    def advance_buffer(self, count):
        """
        Advances the buffer by the amount of data consumed outside the socket.
        """
        self._index += count
        self._bytes_in_buffer -= count

    def new_buffer(self):
        """
        This method moves all the data in the backing buffer to the start of
        a new, fresh buffer. This gives the ability to read much more data.
        """
        def read_all_from_buffer():
            end = self._index + self._bytes_in_buffer
            return self._buffer_view[self._index:end]

        new_buffer = bytearray(self._buffer_size)
        new_buffer_view = memoryview(new_buffer)
        new_buffer_view[0:self._bytes_in_buffer] = read_all_from_buffer()

        self._index = 0
        self._backing_buffer = new_buffer
        self._buffer_view = new_buffer_view

        return

    def recv(self, amt):
        """
        Read some data from the socket.

        :param amt: The amount of data to read.
        :returns: A ``memoryview`` object containing the appropriate number of
            bytes. The data *must* be copied out by the caller before the next
            call to this function.
        """
        # In this implementation you can never read more than the number of
        # bytes in the buffer.
        if amt > self._buffer_size:
            amt = self._buffer_size

        # If the amount of data we've been asked to read is less than the
        # remaining space in the buffer, we need to clear out the buffer and
        # start over.
        if amt > self._remaining_capacity:
            self.new_buffer()

        # If there's still some room in the buffer, opportunistically attempt
        # to read into it.
        # If we don't actually _need_ the data (i.e. there's enough in the
        # buffer to satisfy the request), use select to work out if the read
        # attempt will block. If it will, don't bother reading. If we need the
        # data, always do the read.
        if self._bytes_in_buffer >= amt:
            should_read = select.select([self._sck], [], [], 0)[0]
        else:
            should_read = True

        if ((self._remaining_capacity > self._bytes_in_buffer) and (should_read)):
            count = self._sck.recv_into(self._buffer_view[self._buffer_end:])

            # The socket just got closed. We should throw an exception if we
            # were asked for more data than we can return.
            if not count and amt > self._bytes_in_buffer:
                raise ConnectionResetError()
            self._bytes_in_buffer += count

        # Read out the bytes and update the index.
        amt = min(amt, self._bytes_in_buffer)
        data = self._buffer_view[self._index:self._index+amt]

        self._index += amt
        self._bytes_in_buffer -= amt

        return data

    def recv_into(self, buf, nbytes):
        if self._bytes_in_buffer >= nbytes:
            buf[:] = self._buffer_view[self._index:self._index+nbytes]
            self._index += nbytes
            self._bytes_in_buffer -= nbytes
            return nbytes

        if self._bytes_in_buffer:
            buf[:self._bytes_in_buffer] = self._buffer_view[self._index:self._index+self._bytes_in_buffer]
            p = self._bytes_in_buffer
            nbytes -= self._bytes_in_buffer
            self._index = 0
            self._bytes_in_buffer = 0
        else:
            p = 0

        return self._sck.recv_into(buf[p:], nbytes)

    def fill(self):
        """
        Attempts to fill the buffer as much as possible. It will block for at
        most the time required to have *one* ``recv_into`` call return.
        """
        if not self._remaining_capacity:
            self.new_buffer()

        count = self._sck.recv_into(self._buffer_view[self._buffer_end:])
        if not count:
            raise ConnectionResetError()

        self._bytes_in_buffer += count

        return

    def readline(self):
        """
        Read up to a newline from the network and returns it. The implicit
        maximum line length is the buffer size of the buffered socket.

        Note that, unlike recv, this method absolutely *does* block until it
        can read the line.

        :returns: A ``memoryview`` object containing the appropriate number of
            bytes. The data *must* be copied out by the caller before the next
            call to this function.
        """
        # First, check if there's anything in the buffer. This is one of those
        # rare circumstances where this will work correctly on all platforms.
        index = self._backing_buffer.find(
            b'\n',
            self._index,
            self._index + self._bytes_in_buffer
        )

        if index != -1:
            length = index + 1 - self._index
            data = self._buffer_view[self._index:self._index+length]
            self._index += length
            self._bytes_in_buffer -= length
            return data

        # In this case, we didn't find a newline in the buffer. To fix that,
        # read some data into the buffer. To do our best to satisfy the read,
        # we should shunt the data down in the buffer so that it's right at
        # the start. We don't bother if we're already at the start of the
        # buffer.
        if self._index != 0:
            self.new_buffer()

        while self._bytes_in_buffer < self._buffer_size:
            count = self._sck.recv_into(self._buffer_view[self._buffer_end:])
            if not count:
                raise ConnectionResetError()

            # We have some more data. Again, look for a newline in that gap.
            first_new_byte = self._buffer_end
            self._bytes_in_buffer += count
            index = self._backing_buffer.find(
                b'\n',
                first_new_byte,
                first_new_byte + count,
            )

            if index != -1:
                # The length of the buffer is the index into the
                # buffer at which we found the newline plus 1, minus the start
                # index of the buffer, which really should be zero.
                assert not self._index
                length = index + 1
                data = self._buffer_view[:length]
                self._index += length
                self._bytes_in_buffer -= length
                return data

        # If we got here, it means we filled the buffer without ever getting
        # a newline. Time to throw an exception.
        raise LineTooLongError()

    def __getattr__(self, name):
        return getattr(self._sck, name)

# -*- coding: utf-8 -*-
"""
hyper/http11/parser
~~~~~~~~~~~~~~~~~~~

This module contains hyper's pure-Python HTTP/1.1 parser. This module defines
an abstraction layer for HTTP/1.1 parsing that allows for dropping in other
modules if needed, in order to obtain speedups on your chosen platform.
"""
from collections import namedtuple


Response = namedtuple(
    'Response', ['status', 'msg', 'minor_version', 'headers', 'consumed']
)


class ParseError(Exception):
    """
    An invalid HTTP message was passed to the parser.
    """
    pass


class Parser(object):
    """
    A single HTTP parser object.
    This object is not thread-safe, and it does maintain state that is shared
    across parsing requests. For this reason, make sure that access to this
    object is synchronized if you use it across multiple threads.
    """
    def __init__(self):
        pass

    def parse_response(self, buffer):
        """
        Parses a single HTTP response from a buffer.
        :param buffer: A ``memoryview`` object wrapping a buffer containing a
            HTTP response.
        :returns: A :class:`Response <hyper.http11.parser.Response>` object, or
            ``None`` if there is not enough data in the buffer.
        """
        # Begin by copying the data out of the buffer. This is necessary
        # because as much as possible we want to use the built-in bytestring
        # methods, rather than looping over the data in Python.
        temp_buffer = buffer.tobytes()

        index = temp_buffer.find(b'\n')
        if index == -1:
            return None

        version, status, reason = temp_buffer[0:index].split(None, 2)
        if not version.startswith(b'HTTP/1.'):
            raise ParseError("Not HTTP/1.X!")

        minor_version = int(version[7:])
        status = int(status)
        reason = memoryview(reason.strip())

        # Chomp the newline.
        index += 1

        # Now, parse the headers out.
        end_index = index
        headers = []

        while True:
            end_index = temp_buffer.find(b'\n', index)
            if end_index == -1:
                return None
            elif (end_index - index) <= 1:
                # Chomp the newline
                end_index += 1
                break

            name, value = temp_buffer[index:end_index].split(b':', 1)
            value = value.strip()
            headers.append((memoryview(name), memoryview(value)))
            index = end_index + 1

        resp = Response(status, reason, minor_version, headers, end_index)
        return resp

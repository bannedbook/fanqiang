# -*- coding: utf-8 -*-
"""
hpack/hpack_compat
~~~~~~~~~~~~~~~~~~

Provides an abstraction layer over two HPACK implementations.

This module has a pure-Python greenfield HPACK implementation that can be used
on all Python platforms. However, this implementation is both slower and more
memory-hungry than could be achieved with a C-language version. Additionally,
nghttp2's HPACK implementation currently achieves better compression ratios
than hyper's in almost all benchmarks.

For those who care about efficiency and speed in HPACK, this module allows you
to use nghttp2's HPACK implementation instead of ours. This module detects
whether the nghttp2 bindings are installed, and if they are it wraps them in
a hpack-compatible API and uses them instead of its own. If not, it falls back
to the built-in Python bindings.
"""
import logging
from .hpack import _to_bytes

log = logging.getLogger(__name__)

# Attempt to import nghttp2.
try:
    import nghttp2
    USE_NGHTTP2 = True
    log.debug("Using nghttp2's HPACK implementation.")
except ImportError:
    USE_NGHTTP2 = False
    log.debug("Using our pure-Python HPACK implementation.")

if USE_NGHTTP2:  # noqa
    class Encoder(object):
        """
        An HPACK encoder object. This object takes HTTP headers and emits
        encoded HTTP/2 header blocks.
        """
        def __init__(self):
            self._e = nghttp2.HDDeflater()

        @property
        def header_table_size(self):
            """
            Returns the header table size. For the moment this isn't
            useful, so we don't use it.
            """
            raise NotImplementedError()

        @header_table_size.setter
        def header_table_size(self, value):
            log.debug("Setting header table size to %d", value)
            self._e.change_table_size(value)

        def encode(self, headers, huffman=True):
            """
            Encode the headers. The huffman parameter has no effect, it is
            simply present for compatibility.
            """
            log.debug("HPACK encoding %s", headers)

            # Turn the headers into a list of tuples if possible. This is the
            # natural way to interact with them in HPACK.
            if isinstance(headers, dict):
                headers = headers.items()

            # Next, walk across the headers and turn them all into bytestrings.
            headers = [(_to_bytes(n), _to_bytes(v)) for n, v in headers]

            # Now, let nghttp2 do its thing.
            header_block = self._e.deflate(headers)

            return header_block

    class Decoder(object):
        """
        An HPACK decoder object.
        """
        def __init__(self):
            self._d = nghttp2.HDInflater()

        @property
        def header_table_size(self):
            """
            Returns the header table size. For the moment this isn't
            useful, so we don't use it.
            """
            raise NotImplementedError()

        @header_table_size.setter
        def header_table_size(self, value):
            log.debug("Setting header table size to %d", value)
            self._d.change_table_size(value)

        def decode(self, data):
            """
            Takes an HPACK-encoded header block and decodes it into a header
            set.
            """
            log.debug("Decoding %s", data)

            headers = self._d.inflate(data)
            return [(n.decode('utf-8'), v.decode('utf-8')) for n, v in headers]
else:
    # Grab the built-in encoder and decoder.
    from .hpack import Encoder, Decoder  # noqa

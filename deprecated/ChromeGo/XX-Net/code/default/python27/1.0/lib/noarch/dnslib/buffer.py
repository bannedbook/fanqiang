# -*- coding: utf-8 -*-

"""
    Buffer - simple data buffer
"""

import binascii,struct

class BufferError(Exception):
    pass

class Buffer(object):

    """
    A simple data buffer - supports packing/unpacking in struct format

    # Needed for Python 2/3 doctest compatibility
    >>> def p(s):
    ...     if not isinstance(s,str):
    ...         return s.decode()
    ...     return s

    >>> b = Buffer()
    >>> b.pack("!BHI",1,2,3)
    >>> b.offset
    7
    >>> b.append(b"0123456789")
    >>> b.offset
    17
    >>> p(b.hex())
    '0100020000000330313233343536373839'
    >>> b.offset = 0
    >>> b.unpack("!BHI")
    (1, 2, 3)
    >>> bytearray(b.get(5))
    bytearray(b'01234')
    >>> bytearray(b.get(5))
    bytearray(b'56789')
    >>> b.update(7,"2s",b"xx")
    >>> b.offset = 7
    >>> bytearray(b.get(5))
    bytearray(b'xx234')
    """

    def __init__(self,data=b''):
        """
            Initialise Buffer from data
        """
        self.data = bytearray(data)
        self.offset = 0

    def remaining(self):
        """
            Return bytes remaining
        """
        return len(self.data) - self.offset

    def get(self,length):
        """
            Gen len bytes at current offset (& increment offset)
        """
        if length > self.remaining():
            raise BufferError("Not enough bytes [offset=%d,remaining=%d,requested=%d]" %
                    (self.offset,self.remaining(),length))
        start = self.offset
        end = self.offset + length
        self.offset += length
        return bytes(self.data[start:end])

    def hex(self):
        """
            Return data as hex string
        """
        return binascii.hexlify(self.data)

    def pack(self,fmt,*args):
        """
            Pack data at end of data according to fmt (from struct) & increment
            offset
        """
        self.offset += struct.calcsize(fmt)
        self.data += struct.pack(fmt,*args)

    def append(self,s):
        """
            Append s to end of data & increment offset
        """
        self.offset += len(s)
        self.data += s

    def update(self,ptr,fmt,*args):
        """
            Modify data at offset `ptr`
        """
        s = struct.pack(fmt,*args)
        self.data[ptr:ptr+len(s)] = s

    def unpack(self,fmt):
        """
            Unpack data at current offset according to fmt (from struct)
        """
        try:
            data = self.get(struct.calcsize(fmt))
            return struct.unpack(fmt,data)
        except struct.error as e:
            raise BufferError("Error unpacking struct '%s' <%s>" %
                    (fmt,binascii.hexlify(data).decode()))

    def __len__(self):
        return len(self.data)

if __name__ == '__main__':
    import doctest
    doctest.testmod()

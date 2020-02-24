# -*- coding: utf-8 -*-

"""
    Some basic bit mainpulation utilities
"""
from __future__ import print_function

FILTER = bytearray([ (i < 32 or i > 127) and 46 or i for i in range(256) ])

def hexdump(src, length=16, prefix=''):
    """
    Print hexdump of string

    >>> print(hexdump(b"abcd" * 4))
    0000  61 62 63 64 61 62 63 64  61 62 63 64 61 62 63 64  abcdabcd abcdabcd

    >>> print(hexdump(bytearray(range(48))))
    0000  00 01 02 03 04 05 06 07  08 09 0a 0b 0c 0d 0e 0f  ........ ........
    0010  10 11 12 13 14 15 16 17  18 19 1a 1b 1c 1d 1e 1f  ........ ........
    0020  20 21 22 23 24 25 26 27  28 29 2a 2b 2c 2d 2e 2f   !"#$%&' ()*+,-./

    """
    n = 0
    left = length // 2
    right = length - left
    result= []
    src = bytearray(src)
    while src:
        s,src = src[:length],src[length:]
        l,r = s[:left],s[left:]
        hexa = "%-*s" % (left*3,' '.join(["%02x"%x for x in l]))
        hexb = "%-*s" % (right*3,' '.join(["%02x"%x for x in r]))
        lf = l.translate(FILTER)
        rf = r.translate(FILTER)
        result.append("%s%04x  %s %s %s %s" % (prefix, n, hexa, hexb,
                                               lf.decode(), rf.decode()))
        n += length
    return "\n".join(result)

def get_bits(data,offset,bits=1):
    """
        Get specified bits from integer

        >>> bin(get_bits(0b0011100,2))
        '0b1'
        >>> bin(get_bits(0b0011100,0,4))
        '0b1100'

    """
    mask = ((1 << bits) - 1) << offset
    return (data & mask) >> offset

def set_bits(data,value,offset,bits=1):
    """
        Set specified bits in integer

        >>> bin(set_bits(0,0b1010,0,4))
        '0b1010'
        >>> bin(set_bits(0,0b1010,3,4))
        '0b1010000'
    """
    mask = ((1 << bits) - 1) << offset
    clear = 0xffff ^ mask
    data = (data & clear) | ((value << offset) & mask)
    return data

def binary(n,count=16,reverse=False):
    """
        Display n in binary (only difference from built-in `bin` is
        that this function returns a fixed width string and can
        optionally be reversed

        >>> binary(6789)
        '0001101010000101'
        >>> binary(6789,8)
        '10000101'
        >>> binary(6789,reverse=True)
        '1010000101011000'

    """
    bits = [str((n >> y) & 1) for y in range(count-1, -1, -1)]
    if reverse:
        bits.reverse()
    return "".join(bits)

if __name__ == '__main__':
    import doctest
    doctest.testmod()


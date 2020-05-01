# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------
# Copyright © 2011, RedJack, LLC.
# All rights reserved.
#
# Please see the COPYING file in this distribution for license
# details.
# ----------------------------------------------------------------------

# Calculates the 32-bit MurmurHash3 hash value [1] for a string provided on the
# command line.
#
# [1] http://code.google.com/p/smhasher/wiki/MurmurHash3

def rotl32(a, b):
    return (((a << (b & 0x1f)) & 0xffffffff) |
            ((a >> (32 - (b & 0x1f))) & 0xffffffff))

def fmix(h):
    h = h ^ (h >> 16)
    h = (h * 0x85ebca6b) & 0xffffffff
    h = h ^ (h >> 13)
    h = (h * 0xc2b2ae35) & 0xffffffff
    h = h ^ (h >> 16)
    return h

def hash(value, seed):
    import struct
    length = len(value)
    num_blocks = length / 4
    tail_length = length % 4
    fmt = "<" + ("i" * num_blocks) + ("b" * tail_length)
    vals = struct.unpack(fmt, value)

    h1 = seed
    c1 = 0xcc9e2d51
    c2 = 0x1b873593
    for block in vals[:num_blocks]:
        k1 = block
        k1 = (k1 * c1) & 0xffffffff
        k1 = rotl32(k1, 15)
        k1 = (k1 * c2) & 0xffffffff

        h1 = h1 ^ k1
        h1 = rotl32(h1, 13)
        h1 = (h1 * 5 + 0xe6546b64) & 0xffffffff

    k1 = 0
    if tail_length >= 3:
        k1 = k1 ^ ((vals[num_blocks + 2] << 16) & 0xffffffff)
    if tail_length >= 2:
        k1 = k1 ^ ((vals[num_blocks + 1] <<  8) & 0xffffffff)
    if tail_length >= 1:
        k1 = k1 ^ ( vals[num_blocks]            & 0xffffffff)
        k1 = (k1 * c1) & 0xffffffff
        k1 = rotl32(k1, 15)
        k1 = (k1 * c2) & 0xffffffff
        h1 = h1 ^ k1

    h1 = h1 ^ (length & 0xffffffff)
    return fmix(h1)


if __name__ == "__main__":
    import sys
    if len(sys.argv) != 2 and len(sys.argv) != 3:
        print("Usage: hashstring.py <string> [<seed>]")
        sys.exit(1)

    def myhex(v):
        return hex(v).rstrip("L")

    if len(sys.argv) == 3:
        seed = long(sys.argv[2]) & 0xffffffff
    else:
        seed = 0

    print(myhex(hash(sys.argv[1], seed)))

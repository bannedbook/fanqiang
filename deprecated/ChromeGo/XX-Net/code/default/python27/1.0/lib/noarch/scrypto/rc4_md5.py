#!/usr/bin/env python

# Copyright (c) 2014 clowwindy
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

from __future__ import absolute_import, division, print_function, \
    with_statement

import hashlib


__all__ = ['ciphers']


def create_cipher(alg, key, iv, op, key_as_bytes=0, d=None, salt=None,
                  i=1, padding=1):
    md5 = hashlib.md5()
    md5.update(key)
    md5.update(iv)
    rc4_key = md5.digest()

    try:
        from crypto import ctypes_openssl
        return ctypes_openssl.CtypesCrypto(b'rc4', rc4_key, b'', op)
    except:
        import M2Crypto.EVP
        return M2Crypto.EVP.Cipher(b'rc4', rc4_key, b'', op,
                                   key_as_bytes=0, d='md5', salt=None, i=1,
                                   padding=1)


ciphers = {
    b'rc4-md5': (16, 16, create_cipher),
}


def test():
    from shadowsocks.crypto import util

    cipher = create_cipher(b'rc4-md5', b'k' * 32, b'i' * 16, 1)
    decipher = create_cipher(b'rc4-md5', b'k' * 32, b'i' * 16, 0)

    util.run_cipher(cipher, decipher)


if __name__ == '__main__':
    test()

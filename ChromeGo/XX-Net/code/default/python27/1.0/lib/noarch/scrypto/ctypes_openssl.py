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

import xlog
from ctypes import CDLL, c_char_p, c_int, c_long, byref,\
    create_string_buffer, c_void_p

__all__ = ['ciphers']

libcrypto = None
loaded = False

buf_size = 2048


def load_openssl():
    global loaded, libcrypto, buf

    from ctypes.util import find_library
    for p in ('crypto', 'eay32', 'libeay32'):
        libcrypto_path = find_library(p)
        if libcrypto_path:
            break
    else:
        raise Exception('libcrypto(OpenSSL) not found')
    xlog.info('loading libcrypto from %s', libcrypto_path)
    libcrypto = CDLL(libcrypto_path)
    libcrypto.EVP_get_cipherbyname.restype = c_void_p
    libcrypto.EVP_CIPHER_CTX_new.restype = c_void_p

    libcrypto.EVP_CipherInit_ex.argtypes = (c_void_p, c_void_p, c_char_p,
                                            c_char_p, c_char_p, c_int)

    libcrypto.EVP_CipherUpdate.argtypes = (c_void_p, c_void_p, c_void_p,
                                           c_char_p, c_int)

    libcrypto.EVP_CIPHER_CTX_cleanup.argtypes = (c_void_p,)
    libcrypto.EVP_CIPHER_CTX_free.argtypes = (c_void_p,)
    if hasattr(libcrypto, 'OpenSSL_add_all_ciphers'):
        libcrypto.OpenSSL_add_all_ciphers()

    buf = create_string_buffer(buf_size)
    loaded = True


def load_cipher(cipher_name):
    func_name = b'EVP_' + cipher_name.replace(b'-', b'_')
    if bytes != str:
        func_name = str(func_name, 'utf-8')
    cipher = getattr(libcrypto, func_name, None)
    if cipher:
        cipher.restype = c_void_p
        return cipher()
    return None


class CtypesCrypto(object):
    def __init__(self, cipher_name, key, iv, op):
        if not loaded:
            load_openssl()
        self._ctx = None
        cipher = libcrypto.EVP_get_cipherbyname(cipher_name)
        if not cipher:
            cipher = load_cipher(cipher_name)
        if not cipher:
            raise Exception('cipher %s not found in libcrypto' % cipher_name)
        key_ptr = c_char_p(key)
        iv_ptr = c_char_p(iv)
        self._ctx = libcrypto.EVP_CIPHER_CTX_new()
        if not self._ctx:
            raise Exception('can not create cipher context')
        r = libcrypto.EVP_CipherInit_ex(self._ctx, cipher, None,
                                        key_ptr, iv_ptr, c_int(op))
        if not r:
            self.clean()
            raise Exception('can not initialize cipher context')

    def update(self, data):
        global buf_size, buf
        cipher_out_len = c_long(0)
        l = len(data)
        if buf_size < l:
            buf_size = l * 2
            buf = create_string_buffer(buf_size)
        libcrypto.EVP_CipherUpdate(self._ctx, byref(buf),
                                   byref(cipher_out_len), c_char_p(data), l)
        # buf is copied to a str object when we access buf.raw
        return buf.raw[:cipher_out_len.value]

    def __del__(self):
        self.clean()

    def clean(self):
        if self._ctx:
            libcrypto.EVP_CIPHER_CTX_cleanup(self._ctx)
            libcrypto.EVP_CIPHER_CTX_free(self._ctx)


ciphers = {
    b'aes-128-cfb': (16, 16, CtypesCrypto),
    b'aes-192-cfb': (24, 16, CtypesCrypto),
    b'aes-256-cfb': (32, 16, CtypesCrypto),
    b'aes-128-ofb': (16, 16, CtypesCrypto),
    b'aes-192-ofb': (24, 16, CtypesCrypto),
    b'aes-256-ofb': (32, 16, CtypesCrypto),
    b'aes-128-ctr': (16, 16, CtypesCrypto),
    b'aes-192-ctr': (24, 16, CtypesCrypto),
    b'aes-256-ctr': (32, 16, CtypesCrypto),
    b'aes-128-cfb8': (16, 16, CtypesCrypto),
    b'aes-192-cfb8': (24, 16, CtypesCrypto),
    b'aes-256-cfb8': (32, 16, CtypesCrypto),
    b'aes-128-cfb1': (16, 16, CtypesCrypto),
    b'aes-192-cfb1': (24, 16, CtypesCrypto),
    b'aes-256-cfb1': (32, 16, CtypesCrypto),
    b'bf-cfb': (16, 8, CtypesCrypto),
    b'camellia-128-cfb': (16, 16, CtypesCrypto),
    b'camellia-192-cfb': (24, 16, CtypesCrypto),
    b'camellia-256-cfb': (32, 16, CtypesCrypto),
    b'cast5-cfb': (16, 8, CtypesCrypto),
    b'des-cfb': (8, 8, CtypesCrypto),
    b'idea-cfb': (16, 8, CtypesCrypto),
    b'rc2-cfb': (16, 8, CtypesCrypto),
    b'rc4': (16, 0, CtypesCrypto),
    b'seed-cfb': (16, 16, CtypesCrypto),
}


def run_method(method):
    from shadowsocks.crypto import util

    cipher = CtypesCrypto(method, b'k' * 32, b'i' * 16, 1)
    decipher = CtypesCrypto(method, b'k' * 32, b'i' * 16, 0)

    util.run_cipher(cipher, decipher)


def test_aes_128_cfb():
    run_method(b'aes-128-cfb')


def test_aes_256_cfb():
    run_method(b'aes-256-cfb')


def test_aes_128_cfb8():
    run_method(b'aes-128-cfb8')


def test_aes_256_ofb():
    run_method(b'aes-256-ofb')


def test_aes_256_ctr():
    run_method(b'aes-256-ctr')


def test_bf_cfb():
    run_method(b'bf-cfb')


def test_rc4():
    run_method(b'rc4')


if __name__ == '__main__':
    test_aes_128_cfb()

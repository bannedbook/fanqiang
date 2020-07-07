# !/usr/bin/env python

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

import string
import struct
import hashlib


__all__ = ['ciphers']

cached_tables = {}

if hasattr(string, 'maketrans'):
    maketrans = string.maketrans
    translate = string.translate
else:
    maketrans = bytes.maketrans
    translate = bytes.translate


def get_table(key):
    m = hashlib.md5()
    m.update(key)
    s = m.digest()
    a, b = struct.unpack('<QQ', s)
    table = maketrans(b'', b'')
    table = [table[i: i + 1] for i in range(len(table))]
    for i in range(1, 1024):
        table.sort(key=lambda x: int(a % (ord(x) + i)))
    return table


def init_table(key):
    if key not in cached_tables:
        encrypt_table = b''.join(get_table(key))
        decrypt_table = maketrans(encrypt_table, maketrans(b'', b''))
        cached_tables[key] = [encrypt_table, decrypt_table]
    return cached_tables[key]


class TableCipher(object):
    def __init__(self, cipher_name, key, iv, op):
        self._encrypt_table, self._decrypt_table = init_table(key)
        self._op = op

    def update(self, data):
        if self._op:
            return translate(data, self._encrypt_table)
        else:
            return translate(data, self._decrypt_table)


ciphers = {
    b'table': (0, 0, TableCipher)
}


def test_table_result():
    from shadowsocks.common import ord
    target1 = [
        [60, 53, 84, 138, 217, 94, 88, 23, 39, 242, 219, 35, 12, 157, 165, 181,
         255, 143, 83, 247, 162, 16, 31, 209, 190, 171, 115, 65, 38, 41, 21,
         245, 236, 46, 121, 62, 166, 233, 44, 154, 153, 145, 230, 49, 128, 216,
         173, 29, 241, 119, 64, 229, 194, 103, 131, 110, 26, 197, 218, 59, 204,
         56, 27, 34, 141, 221, 149, 239, 192, 195, 24, 155, 170, 183, 11, 254,
         213, 37, 137, 226, 75, 203, 55, 19, 72, 248, 22, 129, 33, 175, 178,
         10, 198, 71, 77, 36, 113, 167, 48, 2, 117, 140, 142, 66, 199, 232,
         243, 32, 123, 54, 51, 82, 57, 177, 87, 251, 150, 196, 133, 5, 253,
         130, 8, 184, 14, 152, 231, 3, 186, 159, 76, 89, 228, 205, 156, 96,
         163, 146, 18, 91, 132, 85, 80, 109, 172, 176, 105, 13, 50, 235, 127,
         0, 189, 95, 98, 136, 250, 200, 108, 179, 211, 214, 106, 168, 78, 79,
         74, 210, 30, 73, 201, 151, 208, 114, 101, 174, 92, 52, 120, 240, 15,
         169, 220, 182, 81, 224, 43, 185, 40, 99, 180, 17, 212, 158, 42, 90, 9,
         191, 45, 6, 25, 4, 222, 67, 126, 1, 116, 124, 206, 69, 61, 7, 68, 97,
         202, 63, 244, 20, 28, 58, 93, 134, 104, 144, 227, 147, 102, 118, 135,
         148, 47, 238, 86, 112, 122, 70, 107, 215, 100, 139, 223, 225, 164,
         237, 111, 125, 207, 160, 187, 246, 234, 161, 188, 193, 249, 252],
        [151, 205, 99, 127, 201, 119, 199, 211, 122, 196, 91, 74, 12, 147, 124,
         180, 21, 191, 138, 83, 217, 30, 86, 7, 70, 200, 56, 62, 218, 47, 168,
         22, 107, 88, 63, 11, 95, 77, 28, 8, 188, 29, 194, 186, 38, 198, 33,
         230, 98, 43, 148, 110, 177, 1, 109, 82, 61, 112, 219, 59, 0, 210, 35,
         215, 50, 27, 103, 203, 212, 209, 235, 93, 84, 169, 166, 80, 130, 94,
         164, 165, 142, 184, 111, 18, 2, 141, 232, 114, 6, 131, 195, 139, 176,
         220, 5, 153, 135, 213, 154, 189, 238, 174, 226, 53, 222, 146, 162,
         236, 158, 143, 55, 244, 233, 96, 173, 26, 206, 100, 227, 49, 178, 34,
         234, 108, 207, 245, 204, 150, 44, 87, 121, 54, 140, 118, 221, 228,
         155, 78, 3, 239, 101, 64, 102, 17, 223, 41, 137, 225, 229, 66, 116,
         171, 125, 40, 39, 71, 134, 13, 193, 129, 247, 251, 20, 136, 242, 14,
         36, 97, 163, 181, 72, 25, 144, 46, 175, 89, 145, 113, 90, 159, 190,
         15, 183, 73, 123, 187, 128, 248, 252, 152, 24, 197, 68, 253, 52, 69,
         117, 57, 92, 104, 157, 170, 214, 81, 60, 133, 208, 246, 172, 23, 167,
         160, 192, 76, 161, 237, 45, 4, 58, 10, 182, 65, 202, 240, 185, 241,
         79, 224, 132, 51, 42, 126, 105, 37, 250, 149, 32, 243, 231, 67, 179,
         48, 9, 106, 216, 31, 249, 19, 85, 254, 156, 115, 255, 120, 75, 16]]

    target2 = [
        [124, 30, 170, 247, 27, 127, 224, 59, 13, 22, 196, 76, 72, 154, 32,
         209, 4, 2, 131, 62, 101, 51, 230, 9, 166, 11, 99, 80, 208, 112, 36,
         248, 81, 102, 130, 88, 218, 38, 168, 15, 241, 228, 167, 117, 158, 41,
         10, 180, 194, 50, 204, 243, 246, 251, 29, 198, 219, 210, 195, 21, 54,
         91, 203, 221, 70, 57, 183, 17, 147, 49, 133, 65, 77, 55, 202, 122,
         162, 169, 188, 200, 190, 125, 63, 244, 96, 31, 107, 106, 74, 143, 116,
         148, 78, 46, 1, 137, 150, 110, 181, 56, 95, 139, 58, 3, 231, 66, 165,
         142, 242, 43, 192, 157, 89, 175, 109, 220, 128, 0, 178, 42, 255, 20,
         214, 185, 83, 160, 253, 7, 23, 92, 111, 153, 26, 226, 33, 176, 144,
         18, 216, 212, 28, 151, 71, 206, 222, 182, 8, 174, 205, 201, 152, 240,
         155, 108, 223, 104, 239, 98, 164, 211, 184, 34, 193, 14, 114, 187, 40,
         254, 12, 67, 93, 217, 6, 94, 16, 19, 82, 86, 245, 24, 197, 134, 132,
         138, 229, 121, 5, 235, 238, 85, 47, 103, 113, 179, 69, 250, 45, 135,
         156, 25, 61, 75, 44, 146, 189, 84, 207, 172, 119, 53, 123, 186, 120,
         171, 68, 227, 145, 136, 100, 90, 48, 79, 159, 149, 39, 213, 236, 126,
         52, 60, 225, 199, 105, 73, 233, 252, 118, 215, 35, 115, 64, 37, 97,
         129, 161, 177, 87, 237, 141, 173, 191, 163, 140, 234, 232, 249],
        [117, 94, 17, 103, 16, 186, 172, 127, 146, 23, 46, 25, 168, 8, 163, 39,
         174, 67, 137, 175, 121, 59, 9, 128, 179, 199, 132, 4, 140, 54, 1, 85,
         14, 134, 161, 238, 30, 241, 37, 224, 166, 45, 119, 109, 202, 196, 93,
         190, 220, 69, 49, 21, 228, 209, 60, 73, 99, 65, 102, 7, 229, 200, 19,
         82, 240, 71, 105, 169, 214, 194, 64, 142, 12, 233, 88, 201, 11, 72,
         92, 221, 27, 32, 176, 124, 205, 189, 177, 246, 35, 112, 219, 61, 129,
         170, 173, 100, 84, 242, 157, 26, 218, 20, 33, 191, 155, 232, 87, 86,
         153, 114, 97, 130, 29, 192, 164, 239, 90, 43, 236, 208, 212, 185, 75,
         210, 0, 81, 227, 5, 116, 243, 34, 18, 182, 70, 181, 197, 217, 95, 183,
         101, 252, 248, 107, 89, 136, 216, 203, 68, 91, 223, 96, 141, 150, 131,
         13, 152, 198, 111, 44, 222, 125, 244, 76, 251, 158, 106, 24, 42, 38,
         77, 2, 213, 207, 249, 147, 113, 135, 245, 118, 193, 47, 98, 145, 66,
         160, 123, 211, 165, 78, 204, 80, 250, 110, 162, 48, 58, 10, 180, 55,
         231, 79, 149, 74, 62, 50, 148, 143, 206, 28, 15, 57, 159, 139, 225,
         122, 237, 138, 171, 36, 56, 115, 63, 144, 154, 6, 230, 133, 215, 41,
         184, 22, 104, 254, 234, 253, 187, 226, 247, 188, 156, 151, 40, 108,
         51, 83, 178, 52, 3, 31, 255, 195, 53, 235, 126, 167, 120]]

    encrypt_table = b''.join(get_table(b'foobar!'))
    decrypt_table = maketrans(encrypt_table, maketrans(b'', b''))

    for i in range(0, 256):
        assert (target1[0][i] == ord(encrypt_table[i]))
        assert (target1[1][i] == ord(decrypt_table[i]))

    encrypt_table = b''.join(get_table(b'barfoo!'))
    decrypt_table = maketrans(encrypt_table, maketrans(b'', b''))

    for i in range(0, 256):
        assert (target2[0][i] == ord(encrypt_table[i]))
        assert (target2[1][i] == ord(decrypt_table[i]))


def test_encryption():
    from shadowsocks.crypto import util

    cipher = TableCipher(b'table', b'test', b'', 1)
    decipher = TableCipher(b'table', b'test', b'', 0)

    util.run_cipher(cipher, decipher)


if __name__ == '__main__':
    test_table_result()
    test_encryption()

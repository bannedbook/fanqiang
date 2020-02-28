# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import base64
import struct

import six

from cryptography import utils
from cryptography.exceptions import UnsupportedAlgorithm
from cryptography.hazmat.primitives.asymmetric import dsa, ec, ed25519, rsa


def load_ssh_public_key(data, backend):
    key_parts = data.split(b' ', 2)

    if len(key_parts) < 2:
        raise ValueError(
            'Key is not in the proper format or contains extra data.')

    key_type = key_parts[0]

    if key_type == b'ssh-rsa':
        loader = _load_ssh_rsa_public_key
    elif key_type == b'ssh-dss':
        loader = _load_ssh_dss_public_key
    elif key_type in [
        b'ecdsa-sha2-nistp256', b'ecdsa-sha2-nistp384', b'ecdsa-sha2-nistp521',
    ]:
        loader = _load_ssh_ecdsa_public_key
    elif key_type == b'ssh-ed25519':
        loader = _load_ssh_ed25519_public_key
    else:
        raise UnsupportedAlgorithm('Key type is not supported.')

    key_body = key_parts[1]

    try:
        decoded_data = base64.b64decode(key_body)
    except TypeError:
        raise ValueError('Key is not in the proper format.')

    inner_key_type, rest = _ssh_read_next_string(decoded_data)

    if inner_key_type != key_type:
        raise ValueError(
            'Key header and key body contain different key type values.'
        )

    return loader(key_type, rest, backend)


def _load_ssh_rsa_public_key(key_type, decoded_data, backend):
    e, rest = _ssh_read_next_mpint(decoded_data)
    n, rest = _ssh_read_next_mpint(rest)

    if rest:
        raise ValueError('Key body contains extra bytes.')

    return rsa.RSAPublicNumbers(e, n).public_key(backend)


def _load_ssh_dss_public_key(key_type, decoded_data, backend):
    p, rest = _ssh_read_next_mpint(decoded_data)
    q, rest = _ssh_read_next_mpint(rest)
    g, rest = _ssh_read_next_mpint(rest)
    y, rest = _ssh_read_next_mpint(rest)

    if rest:
        raise ValueError('Key body contains extra bytes.')

    parameter_numbers = dsa.DSAParameterNumbers(p, q, g)
    public_numbers = dsa.DSAPublicNumbers(y, parameter_numbers)

    return public_numbers.public_key(backend)


def _load_ssh_ecdsa_public_key(expected_key_type, decoded_data, backend):
    curve_name, rest = _ssh_read_next_string(decoded_data)
    data, rest = _ssh_read_next_string(rest)

    if expected_key_type != b"ecdsa-sha2-" + curve_name:
        raise ValueError(
            'Key header and key body contain different key type values.'
        )

    if rest:
        raise ValueError('Key body contains extra bytes.')

    curve = {
        b"nistp256": ec.SECP256R1,
        b"nistp384": ec.SECP384R1,
        b"nistp521": ec.SECP521R1,
    }[curve_name]()

    if six.indexbytes(data, 0) != 4:
        raise NotImplementedError(
            "Compressed elliptic curve points are not supported"
        )

    return ec.EllipticCurvePublicKey.from_encoded_point(curve, data)


def _load_ssh_ed25519_public_key(expected_key_type, decoded_data, backend):
    data, rest = _ssh_read_next_string(decoded_data)

    if rest:
        raise ValueError('Key body contains extra bytes.')

    return ed25519.Ed25519PublicKey.from_public_bytes(data)


def _ssh_read_next_string(data):
    """
    Retrieves the next RFC 4251 string value from the data.

    While the RFC calls these strings, in Python they are bytes objects.
    """
    if len(data) < 4:
        raise ValueError("Key is not in the proper format")

    str_len, = struct.unpack('>I', data[:4])
    if len(data) < str_len + 4:
        raise ValueError("Key is not in the proper format")

    return data[4:4 + str_len], data[4 + str_len:]


def _ssh_read_next_mpint(data):
    """
    Reads the next mpint from the data.

    Currently, all mpints are interpreted as unsigned.
    """
    mpint_data, rest = _ssh_read_next_string(data)

    return (
        utils.int_from_bytes(mpint_data, byteorder='big', signed=False), rest
    )


def _ssh_write_string(data):
    return struct.pack(">I", len(data)) + data


def _ssh_write_mpint(value):
    data = utils.int_to_bytes(value)
    if six.indexbytes(data, 0) & 0x80:
        data = b"\x00" + data
    return _ssh_write_string(data)

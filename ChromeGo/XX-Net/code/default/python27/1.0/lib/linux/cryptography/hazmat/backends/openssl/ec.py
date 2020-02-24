# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

from cryptography import utils
from cryptography.exceptions import (
    InvalidSignature, UnsupportedAlgorithm, _Reasons
)
from cryptography.hazmat.backends.openssl.utils import _truncate_digest
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import (
    AsymmetricSignatureContext, AsymmetricVerificationContext, ec
)


def _truncate_digest_for_ecdsa(ec_key_cdata, digest, backend):
    """
    This function truncates digests that are longer than a given elliptic
    curve key's length so they can be signed. Since elliptic curve keys are
    much shorter than RSA keys many digests (e.g. SHA-512) may require
    truncation.
    """

    _lib = backend._lib
    _ffi = backend._ffi

    group = _lib.EC_KEY_get0_group(ec_key_cdata)

    with backend._tmp_bn_ctx() as bn_ctx:
        order = _lib.BN_CTX_get(bn_ctx)
        assert order != _ffi.NULL

        res = _lib.EC_GROUP_get_order(group, order, bn_ctx)
        assert res == 1

        order_bits = _lib.BN_num_bits(order)

    return _truncate_digest(digest, order_bits)


def _ec_key_curve_sn(backend, ec_key):
    group = backend._lib.EC_KEY_get0_group(ec_key)
    assert group != backend._ffi.NULL

    nid = backend._lib.EC_GROUP_get_curve_name(group)
    # The following check is to find EC keys with unnamed curves and raise
    # an error for now.
    if nid == backend._lib.NID_undef:
        raise NotImplementedError(
            "ECDSA certificates with unnamed curves are unsupported "
            "at this time"
        )

    curve_name = backend._lib.OBJ_nid2sn(nid)
    assert curve_name != backend._ffi.NULL

    sn = backend._ffi.string(curve_name).decode('ascii')
    return sn


def _mark_asn1_named_ec_curve(backend, ec_cdata):
    """
    Set the named curve flag on the EC_KEY. This causes OpenSSL to
    serialize EC keys along with their curve OID which makes
    deserialization easier.
    """

    backend._lib.EC_KEY_set_asn1_flag(
        ec_cdata, backend._lib.OPENSSL_EC_NAMED_CURVE
    )


def _sn_to_elliptic_curve(backend, sn):
    try:
        return ec._CURVE_TYPES[sn]()
    except KeyError:
        raise UnsupportedAlgorithm(
            "{0} is not a supported elliptic curve".format(sn),
            _Reasons.UNSUPPORTED_ELLIPTIC_CURVE
        )


@utils.register_interface(AsymmetricSignatureContext)
class _ECDSASignatureContext(object):
    def __init__(self, backend, private_key, algorithm):
        self._backend = backend
        self._private_key = private_key
        self._digest = hashes.Hash(algorithm, backend)

    def update(self, data):
        self._digest.update(data)

    def finalize(self):
        ec_key = self._private_key._ec_key

        digest = self._digest.finalize()

        digest = _truncate_digest_for_ecdsa(ec_key, digest, self._backend)

        max_size = self._backend._lib.ECDSA_size(ec_key)
        assert max_size > 0

        sigbuf = self._backend._ffi.new("char[]", max_size)
        siglen_ptr = self._backend._ffi.new("unsigned int[]", 1)
        res = self._backend._lib.ECDSA_sign(
            0,
            digest,
            len(digest),
            sigbuf,
            siglen_ptr,
            ec_key
        )
        assert res == 1
        return self._backend._ffi.buffer(sigbuf)[:siglen_ptr[0]]


@utils.register_interface(AsymmetricVerificationContext)
class _ECDSAVerificationContext(object):
    def __init__(self, backend, public_key, signature, algorithm):
        self._backend = backend
        self._public_key = public_key
        self._signature = signature
        self._digest = hashes.Hash(algorithm, backend)

    def update(self, data):
        self._digest.update(data)

    def verify(self):
        ec_key = self._public_key._ec_key

        digest = self._digest.finalize()

        digest = _truncate_digest_for_ecdsa(ec_key, digest, self._backend)

        res = self._backend._lib.ECDSA_verify(
            0,
            digest,
            len(digest),
            self._signature,
            len(self._signature),
            ec_key
        )
        if res != 1:
            self._backend._consume_errors()
            raise InvalidSignature
        return True


@utils.register_interface(ec.EllipticCurvePrivateKeyWithSerialization)
class _EllipticCurvePrivateKey(object):
    def __init__(self, backend, ec_key_cdata):
        self._backend = backend
        _mark_asn1_named_ec_curve(backend, ec_key_cdata)
        self._ec_key = ec_key_cdata

        sn = _ec_key_curve_sn(backend, ec_key_cdata)
        self._curve = _sn_to_elliptic_curve(backend, sn)

    curve = utils.read_only_property("_curve")

    def signer(self, signature_algorithm):
        if isinstance(signature_algorithm, ec.ECDSA):
            return _ECDSASignatureContext(
                self._backend, self, signature_algorithm.algorithm
            )
        else:
            raise UnsupportedAlgorithm(
                "Unsupported elliptic curve signature algorithm.",
                _Reasons.UNSUPPORTED_PUBLIC_KEY_ALGORITHM)

    def public_key(self):
        group = self._backend._lib.EC_KEY_get0_group(self._ec_key)
        assert group != self._backend._ffi.NULL

        curve_nid = self._backend._lib.EC_GROUP_get_curve_name(group)

        public_ec_key = self._backend._lib.EC_KEY_new_by_curve_name(curve_nid)
        assert public_ec_key != self._backend._ffi.NULL
        public_ec_key = self._backend._ffi.gc(
            public_ec_key, self._backend._lib.EC_KEY_free
        )

        point = self._backend._lib.EC_KEY_get0_public_key(self._ec_key)
        assert point != self._backend._ffi.NULL

        res = self._backend._lib.EC_KEY_set_public_key(public_ec_key, point)
        assert res == 1

        return _EllipticCurvePublicKey(
            self._backend, public_ec_key
        )

    def private_numbers(self):
        bn = self._backend._lib.EC_KEY_get0_private_key(self._ec_key)
        private_value = self._backend._bn_to_int(bn)
        return ec.EllipticCurvePrivateNumbers(
            private_value=private_value,
            public_numbers=self.public_key().public_numbers()
        )

    def private_bytes(self, encoding, format, encryption_algorithm):
        evp_pkey = self._backend._lib.EVP_PKEY_new()
        assert evp_pkey != self._backend._ffi.NULL
        evp_pkey = self._backend._ffi.gc(
            evp_pkey, self._backend._lib.EVP_PKEY_free
        )
        res = self._backend._lib.EVP_PKEY_set1_EC_KEY(evp_pkey, self._ec_key)
        assert res == 1
        return self._backend._private_key_bytes(
            encoding,
            format,
            encryption_algorithm,
            self._backend._lib.PEM_write_bio_ECPrivateKey,
            evp_pkey,
            self._ec_key
        )


@utils.register_interface(ec.EllipticCurvePublicKeyWithNumbers)
class _EllipticCurvePublicKey(object):
    def __init__(self, backend, ec_key_cdata):
        self._backend = backend
        _mark_asn1_named_ec_curve(backend, ec_key_cdata)
        self._ec_key = ec_key_cdata

        sn = _ec_key_curve_sn(backend, ec_key_cdata)
        self._curve = _sn_to_elliptic_curve(backend, sn)

    curve = utils.read_only_property("_curve")

    def verifier(self, signature, signature_algorithm):
        if isinstance(signature_algorithm, ec.ECDSA):
            return _ECDSAVerificationContext(
                self._backend, self, signature, signature_algorithm.algorithm
            )
        else:
            raise UnsupportedAlgorithm(
                "Unsupported elliptic curve signature algorithm.",
                _Reasons.UNSUPPORTED_PUBLIC_KEY_ALGORITHM)

    def public_numbers(self):
        set_func, get_func, group = (
            self._backend._ec_key_determine_group_get_set_funcs(self._ec_key)
        )
        point = self._backend._lib.EC_KEY_get0_public_key(self._ec_key)
        assert point != self._backend._ffi.NULL

        with self._backend._tmp_bn_ctx() as bn_ctx:
            bn_x = self._backend._lib.BN_CTX_get(bn_ctx)
            bn_y = self._backend._lib.BN_CTX_get(bn_ctx)

            res = get_func(group, point, bn_x, bn_y, bn_ctx)
            assert res == 1

            x = self._backend._bn_to_int(bn_x)
            y = self._backend._bn_to_int(bn_y)

        return ec.EllipticCurvePublicNumbers(
            x=x,
            y=y,
            curve=self._curve
        )

    def public_bytes(self, encoding, format):
        if format is serialization.PublicFormat.PKCS1:
            raise ValueError(
                "EC public keys do not support PKCS1 serialization"
            )

        evp_pkey = self._backend._lib.EVP_PKEY_new()
        assert evp_pkey != self._backend._ffi.NULL
        evp_pkey = self._backend._ffi.gc(
            evp_pkey, self._backend._lib.EVP_PKEY_free
        )
        res = self._backend._lib.EVP_PKEY_set1_EC_KEY(evp_pkey, self._ec_key)
        assert res == 1
        return self._backend._public_key_bytes(
            encoding,
            format,
            None,
            evp_pkey,
            None
        )

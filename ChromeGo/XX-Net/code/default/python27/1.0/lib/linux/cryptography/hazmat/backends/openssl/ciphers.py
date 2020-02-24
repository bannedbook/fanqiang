# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

from cryptography import utils
from cryptography.exceptions import InvalidTag, UnsupportedAlgorithm, _Reasons
from cryptography.hazmat.primitives import ciphers
from cryptography.hazmat.primitives.ciphers import modes


@utils.register_interface(ciphers.CipherContext)
@utils.register_interface(ciphers.AEADCipherContext)
@utils.register_interface(ciphers.AEADEncryptionContext)
class _CipherContext(object):
    _ENCRYPT = 1
    _DECRYPT = 0

    def __init__(self, backend, cipher, mode, operation):
        self._backend = backend
        self._cipher = cipher
        self._mode = mode
        self._operation = operation
        self._tag = None

        if isinstance(self._cipher, ciphers.BlockCipherAlgorithm):
            self._block_size = self._cipher.block_size
        else:
            self._block_size = 1

        ctx = self._backend._lib.EVP_CIPHER_CTX_new()
        ctx = self._backend._ffi.gc(
            ctx, self._backend._lib.EVP_CIPHER_CTX_free
        )

        registry = self._backend._cipher_registry
        try:
            adapter = registry[type(cipher), type(mode)]
        except KeyError:
            raise UnsupportedAlgorithm(
                "cipher {0} in {1} mode is not supported "
                "by this backend.".format(
                    cipher.name, mode.name if mode else mode),
                _Reasons.UNSUPPORTED_CIPHER
            )

        evp_cipher = adapter(self._backend, cipher, mode)
        if evp_cipher == self._backend._ffi.NULL:
            raise UnsupportedAlgorithm(
                "cipher {0} in {1} mode is not supported "
                "by this backend.".format(
                    cipher.name, mode.name if mode else mode),
                _Reasons.UNSUPPORTED_CIPHER
            )

        if isinstance(mode, modes.ModeWithInitializationVector):
            iv_nonce = mode.initialization_vector
        elif isinstance(mode, modes.ModeWithNonce):
            iv_nonce = mode.nonce
        else:
            iv_nonce = self._backend._ffi.NULL
        # begin init with cipher and operation type
        res = self._backend._lib.EVP_CipherInit_ex(ctx, evp_cipher,
                                                   self._backend._ffi.NULL,
                                                   self._backend._ffi.NULL,
                                                   self._backend._ffi.NULL,
                                                   operation)
        assert res != 0
        # set the key length to handle variable key ciphers
        res = self._backend._lib.EVP_CIPHER_CTX_set_key_length(
            ctx, len(cipher.key)
        )
        assert res != 0
        if isinstance(mode, modes.GCM):
            res = self._backend._lib.EVP_CIPHER_CTX_ctrl(
                ctx, self._backend._lib.EVP_CTRL_GCM_SET_IVLEN,
                len(iv_nonce), self._backend._ffi.NULL
            )
            assert res != 0
            if operation == self._DECRYPT:
                res = self._backend._lib.EVP_CIPHER_CTX_ctrl(
                    ctx, self._backend._lib.EVP_CTRL_GCM_SET_TAG,
                    len(mode.tag), mode.tag
                )
                assert res != 0

        # pass key/iv
        res = self._backend._lib.EVP_CipherInit_ex(
            ctx,
            self._backend._ffi.NULL,
            self._backend._ffi.NULL,
            cipher.key,
            iv_nonce,
            operation
        )
        assert res != 0
        # We purposely disable padding here as it's handled higher up in the
        # API.
        self._backend._lib.EVP_CIPHER_CTX_set_padding(ctx, 0)
        self._ctx = ctx

    def update(self, data):
        # OpenSSL 0.9.8e has an assertion in its EVP code that causes it
        # to SIGABRT if you call update with an empty byte string. This can be
        # removed when we drop support for 0.9.8e (CentOS/RHEL 5). This branch
        # should be taken only when length is zero and mode is not GCM because
        # AES GCM can return improper tag values if you don't call update
        # with empty plaintext when authenticating AAD for ...reasons.
        if len(data) == 0 and not isinstance(self._mode, modes.GCM):
            return b""

        buf = self._backend._ffi.new("unsigned char[]",
                                     len(data) + self._block_size - 1)
        outlen = self._backend._ffi.new("int *")
        res = self._backend._lib.EVP_CipherUpdate(self._ctx, buf, outlen, data,
                                                  len(data))
        assert res != 0
        return self._backend._ffi.buffer(buf)[:outlen[0]]

    def finalize(self):
        # OpenSSL 1.0.1 on Ubuntu 12.04 (and possibly other distributions)
        # appears to have a bug where you must make at least one call to update
        # even if you are only using authenticate_additional_data or the
        # GCM tag will be wrong. An (empty) call to update resolves this
        # and is harmless for all other versions of OpenSSL.
        if isinstance(self._mode, modes.GCM):
            self.update(b"")

        buf = self._backend._ffi.new("unsigned char[]", self._block_size)
        outlen = self._backend._ffi.new("int *")
        res = self._backend._lib.EVP_CipherFinal_ex(self._ctx, buf, outlen)
        if res == 0:
            errors = self._backend._consume_errors()

            if not errors and isinstance(self._mode, modes.GCM):
                raise InvalidTag

            assert errors

            if errors[0][1:] == (
                self._backend._lib.ERR_LIB_EVP,
                self._backend._lib.EVP_F_EVP_ENCRYPTFINAL_EX,
                self._backend._lib.EVP_R_DATA_NOT_MULTIPLE_OF_BLOCK_LENGTH
            ) or errors[0][1:] == (
                self._backend._lib.ERR_LIB_EVP,
                self._backend._lib.EVP_F_EVP_DECRYPTFINAL_EX,
                self._backend._lib.EVP_R_DATA_NOT_MULTIPLE_OF_BLOCK_LENGTH
            ):
                raise ValueError(
                    "The length of the provided data is not a multiple of "
                    "the block length."
                )
            else:
                raise self._backend._unknown_error(errors[0])

        if (isinstance(self._mode, modes.GCM) and
           self._operation == self._ENCRYPT):
            block_byte_size = self._block_size // 8
            tag_buf = self._backend._ffi.new(
                "unsigned char[]", block_byte_size
            )
            res = self._backend._lib.EVP_CIPHER_CTX_ctrl(
                self._ctx, self._backend._lib.EVP_CTRL_GCM_GET_TAG,
                block_byte_size, tag_buf
            )
            assert res != 0
            self._tag = self._backend._ffi.buffer(tag_buf)[:]

        res = self._backend._lib.EVP_CIPHER_CTX_cleanup(self._ctx)
        assert res == 1
        return self._backend._ffi.buffer(buf)[:outlen[0]]

    def authenticate_additional_data(self, data):
        outlen = self._backend._ffi.new("int *")
        res = self._backend._lib.EVP_CipherUpdate(
            self._ctx, self._backend._ffi.NULL, outlen, data, len(data)
        )
        assert res != 0

    tag = utils.read_only_property("_tag")


@utils.register_interface(ciphers.CipherContext)
class _AESCTRCipherContext(object):
    """
    This is needed to provide support for AES CTR mode in OpenSSL 0.9.8. It can
    be removed when we drop 0.9.8 support (RHEL5 extended life ends 2020).
    """
    def __init__(self, backend, cipher, mode):
        self._backend = backend

        self._key = self._backend._ffi.new("AES_KEY *")
        assert self._key != self._backend._ffi.NULL
        res = self._backend._lib.AES_set_encrypt_key(
            cipher.key, len(cipher.key) * 8, self._key
        )
        assert res == 0
        self._ecount = self._backend._ffi.new("char[]", 16)
        self._nonce = self._backend._ffi.new("char[16]", mode.nonce)
        self._num = self._backend._ffi.new("unsigned int *", 0)

    def update(self, data):
        buf = self._backend._ffi.new("unsigned char[]", len(data))
        self._backend._lib.AES_ctr128_encrypt(
            data, buf, len(data), self._key, self._nonce,
            self._ecount, self._num
        )
        return self._backend._ffi.buffer(buf)[:]

    def finalize(self):
        self._key = None
        self._ecount = None
        self._nonce = None
        self._num = None
        return b""

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
@utils.register_interface(ciphers.AEADDecryptionContext)
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
            self._block_size_bytes = self._cipher.block_size // 8
        else:
            self._block_size_bytes = 1

        ctx = self._backend._lib.EVP_CIPHER_CTX_new()
        ctx = self._backend._ffi.gc(
            ctx, self._backend._lib.EVP_CIPHER_CTX_free
        )

        registry = self._backend._cipher_registry
        try:
            adapter = registry[type(cipher), type(mode)]
        except KeyError:
            raise UnsupportedAlgorithm(
                "cipher {} in {} mode is not supported "
                "by this backend.".format(
                    cipher.name, mode.name if mode else mode),
                _Reasons.UNSUPPORTED_CIPHER
            )

        evp_cipher = adapter(self._backend, cipher, mode)
        if evp_cipher == self._backend._ffi.NULL:
            msg = "cipher {0.name} ".format(cipher)
            if mode is not None:
                msg += "in {0.name} mode ".format(mode)
            msg += (
                "is not supported by this backend (Your version of OpenSSL "
                "may be too old. Current version: {}.)"
            ).format(self._backend.openssl_version_text())
            raise UnsupportedAlgorithm(msg, _Reasons.UNSUPPORTED_CIPHER)

        if isinstance(mode, modes.ModeWithInitializationVector):
            iv_nonce = self._backend._ffi.from_buffer(
                mode.initialization_vector
            )
        elif isinstance(mode, modes.ModeWithTweak):
            iv_nonce = self._backend._ffi.from_buffer(mode.tweak)
        elif isinstance(mode, modes.ModeWithNonce):
            iv_nonce = self._backend._ffi.from_buffer(mode.nonce)
        elif isinstance(cipher, modes.ModeWithNonce):
            iv_nonce = self._backend._ffi.from_buffer(cipher.nonce)
        else:
            iv_nonce = self._backend._ffi.NULL
        # begin init with cipher and operation type
        res = self._backend._lib.EVP_CipherInit_ex(ctx, evp_cipher,
                                                   self._backend._ffi.NULL,
                                                   self._backend._ffi.NULL,
                                                   self._backend._ffi.NULL,
                                                   operation)
        self._backend.openssl_assert(res != 0)
        # set the key length to handle variable key ciphers
        res = self._backend._lib.EVP_CIPHER_CTX_set_key_length(
            ctx, len(cipher.key)
        )
        self._backend.openssl_assert(res != 0)
        if isinstance(mode, modes.GCM):
            res = self._backend._lib.EVP_CIPHER_CTX_ctrl(
                ctx, self._backend._lib.EVP_CTRL_AEAD_SET_IVLEN,
                len(iv_nonce), self._backend._ffi.NULL
            )
            self._backend.openssl_assert(res != 0)
            if mode.tag is not None:
                res = self._backend._lib.EVP_CIPHER_CTX_ctrl(
                    ctx, self._backend._lib.EVP_CTRL_AEAD_SET_TAG,
                    len(mode.tag), mode.tag
                )
                self._backend.openssl_assert(res != 0)
                self._tag = mode.tag
            elif (
                self._operation == self._DECRYPT and
                self._backend._lib.CRYPTOGRAPHY_OPENSSL_LESS_THAN_102 and
                not self._backend._lib.CRYPTOGRAPHY_IS_LIBRESSL
            ):
                raise NotImplementedError(
                    "delayed passing of GCM tag requires OpenSSL >= 1.0.2."
                    " To use this feature please update OpenSSL"
                )

        # pass key/iv
        res = self._backend._lib.EVP_CipherInit_ex(
            ctx,
            self._backend._ffi.NULL,
            self._backend._ffi.NULL,
            self._backend._ffi.from_buffer(cipher.key),
            iv_nonce,
            operation
        )
        self._backend.openssl_assert(res != 0)
        # We purposely disable padding here as it's handled higher up in the
        # API.
        self._backend._lib.EVP_CIPHER_CTX_set_padding(ctx, 0)
        self._ctx = ctx

    def update(self, data):
        buf = bytearray(len(data) + self._block_size_bytes - 1)
        n = self.update_into(data, buf)
        return bytes(buf[:n])

    def update_into(self, data, buf):
        if len(buf) < (len(data) + self._block_size_bytes - 1):
            raise ValueError(
                "buffer must be at least {} bytes for this "
                "payload".format(len(data) + self._block_size_bytes - 1)
            )

        buf = self._backend._ffi.cast(
            "unsigned char *", self._backend._ffi.from_buffer(buf)
        )
        outlen = self._backend._ffi.new("int *")
        res = self._backend._lib.EVP_CipherUpdate(
            self._ctx, buf, outlen,
            self._backend._ffi.from_buffer(data), len(data)
        )
        self._backend.openssl_assert(res != 0)
        return outlen[0]

    def finalize(self):
        # OpenSSL 1.0.1 on Ubuntu 12.04 (and possibly other distributions)
        # appears to have a bug where you must make at least one call to update
        # even if you are only using authenticate_additional_data or the
        # GCM tag will be wrong. An (empty) call to update resolves this
        # and is harmless for all other versions of OpenSSL.
        if isinstance(self._mode, modes.GCM):
            self.update(b"")

        if (
            self._operation == self._DECRYPT and
            isinstance(self._mode, modes.ModeWithAuthenticationTag) and
            self.tag is None
        ):
            raise ValueError(
                "Authentication tag must be provided when decrypting."
            )

        buf = self._backend._ffi.new("unsigned char[]", self._block_size_bytes)
        outlen = self._backend._ffi.new("int *")
        res = self._backend._lib.EVP_CipherFinal_ex(self._ctx, buf, outlen)
        if res == 0:
            errors = self._backend._consume_errors()

            if not errors and isinstance(self._mode, modes.GCM):
                raise InvalidTag

            self._backend.openssl_assert(
                errors[0]._lib_reason_match(
                    self._backend._lib.ERR_LIB_EVP,
                    self._backend._lib.EVP_R_DATA_NOT_MULTIPLE_OF_BLOCK_LENGTH
                )
            )
            raise ValueError(
                "The length of the provided data is not a multiple of "
                "the block length."
            )

        if (isinstance(self._mode, modes.GCM) and
           self._operation == self._ENCRYPT):
            tag_buf = self._backend._ffi.new(
                "unsigned char[]", self._block_size_bytes
            )
            res = self._backend._lib.EVP_CIPHER_CTX_ctrl(
                self._ctx, self._backend._lib.EVP_CTRL_AEAD_GET_TAG,
                self._block_size_bytes, tag_buf
            )
            self._backend.openssl_assert(res != 0)
            self._tag = self._backend._ffi.buffer(tag_buf)[:]

        res = self._backend._lib.EVP_CIPHER_CTX_cleanup(self._ctx)
        self._backend.openssl_assert(res == 1)
        return self._backend._ffi.buffer(buf)[:outlen[0]]

    def finalize_with_tag(self, tag):
        if (
            self._backend._lib.CRYPTOGRAPHY_OPENSSL_LESS_THAN_102 and
            not self._backend._lib.CRYPTOGRAPHY_IS_LIBRESSL
        ):
            raise NotImplementedError(
                "finalize_with_tag requires OpenSSL >= 1.0.2. To use this "
                "method please update OpenSSL"
            )
        if len(tag) < self._mode._min_tag_length:
            raise ValueError(
                "Authentication tag must be {} bytes or longer.".format(
                    self._mode._min_tag_length)
            )
        res = self._backend._lib.EVP_CIPHER_CTX_ctrl(
            self._ctx, self._backend._lib.EVP_CTRL_AEAD_SET_TAG,
            len(tag), tag
        )
        self._backend.openssl_assert(res != 0)
        self._tag = tag
        return self.finalize()

    def authenticate_additional_data(self, data):
        outlen = self._backend._ffi.new("int *")
        res = self._backend._lib.EVP_CipherUpdate(
            self._ctx, self._backend._ffi.NULL, outlen,
            self._backend._ffi.from_buffer(data), len(data)
        )
        self._backend.openssl_assert(res != 0)

    tag = utils.read_only_property("_tag")

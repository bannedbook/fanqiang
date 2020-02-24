# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import abc

import six

from cryptography import utils
from cryptography.hazmat.primitives import ciphers, hashes
from cryptography.hazmat.primitives.asymmetric import (
    AsymmetricSignatureContext, AsymmetricVerificationContext, dsa, ec,
    padding, rsa
)
from cryptography.hazmat.primitives.ciphers import modes
from cryptography.hazmat.primitives.kdf import KeyDerivationFunction
from cryptography.hazmat.primitives.padding import PaddingContext


BlockCipherAlgorithm = utils.deprecated(
    ciphers.BlockCipherAlgorithm,
    __name__,
    (
        "The BlockCipherAlgorithm interface has moved to the "
        "cryptography.hazmat.primitives.ciphers module"
    ),
    utils.DeprecatedIn08
)


CipherAlgorithm = utils.deprecated(
    ciphers.CipherAlgorithm,
    __name__,
    (
        "The CipherAlgorithm interface has moved to the "
        "cryptography.hazmat.primitives.ciphers module"
    ),
    utils.DeprecatedIn08
)


Mode = utils.deprecated(
    modes.Mode,
    __name__,
    (
        "The Mode interface has moved to the "
        "cryptography.hazmat.primitives.ciphers.modes module"
    ),
    utils.DeprecatedIn08
)


ModeWithAuthenticationTag = utils.deprecated(
    modes.ModeWithAuthenticationTag,
    __name__,
    (
        "The ModeWithAuthenticationTag interface has moved to the "
        "cryptography.hazmat.primitives.ciphers.modes module"
    ),
    utils.DeprecatedIn08
)


ModeWithInitializationVector = utils.deprecated(
    modes.ModeWithInitializationVector,
    __name__,
    (
        "The ModeWithInitializationVector interface has moved to the "
        "cryptography.hazmat.primitives.ciphers.modes module"
    ),
    utils.DeprecatedIn08
)


ModeWithNonce = utils.deprecated(
    modes.ModeWithNonce,
    __name__,
    (
        "The ModeWithNonce interface has moved to the "
        "cryptography.hazmat.primitives.ciphers.modes module"
    ),
    utils.DeprecatedIn08
)


CipherContext = utils.deprecated(
    ciphers.CipherContext,
    __name__,
    (
        "The CipherContext interface has moved to the "
        "cryptography.hazmat.primitives.ciphers module"
    ),
    utils.DeprecatedIn08
)


AEADCipherContext = utils.deprecated(
    ciphers.AEADCipherContext,
    __name__,
    (
        "The AEADCipherContext interface has moved to the "
        "cryptography.hazmat.primitives.ciphers module"
    ),
    utils.DeprecatedIn08
)


AEADEncryptionContext = utils.deprecated(
    ciphers.AEADEncryptionContext,
    __name__,
    (
        "The AEADEncryptionContext interface has moved to the "
        "cryptography.hazmat.primitives.ciphers module"
    ),
    utils.DeprecatedIn08
)


EllipticCurve = utils.deprecated(
    ec.EllipticCurve,
    __name__,
    (
        "The EllipticCurve interface has moved to the "
        "cryptography.hazmat.primitives.asymmetric.ec module"
    ),
    utils.DeprecatedIn08
)


EllipticCurvePrivateKey = utils.deprecated(
    ec.EllipticCurvePrivateKey,
    __name__,
    (
        "The EllipticCurvePrivateKey interface has moved to the "
        "cryptography.hazmat.primitives.asymmetric.ec module"
    ),
    utils.DeprecatedIn08
)


EllipticCurvePrivateKeyWithNumbers = utils.deprecated(
    ec.EllipticCurvePrivateKeyWithNumbers,
    __name__,
    (
        "The EllipticCurvePrivateKeyWithNumbers interface has moved to the "
        "cryptography.hazmat.primitives.asymmetric.ec module"
    ),
    utils.DeprecatedIn08
)


EllipticCurvePublicKey = utils.deprecated(
    ec.EllipticCurvePublicKey,
    __name__,
    (
        "The EllipticCurvePublicKey interface has moved to the "
        "cryptography.hazmat.primitives.asymmetric.ec module"
    ),
    utils.DeprecatedIn08
)


EllipticCurvePublicKeyWithNumbers = utils.deprecated(
    ec.EllipticCurvePublicKeyWithNumbers,
    __name__,
    (
        "The EllipticCurvePublicKeyWithNumbers interface has moved to the "
        "cryptography.hazmat.primitives.asymmetric.ec module"
    ),
    utils.DeprecatedIn08
)


EllipticCurveSignatureAlgorithm = utils.deprecated(
    ec.EllipticCurveSignatureAlgorithm,
    __name__,
    (
        "The EllipticCurveSignatureAlgorithm interface has moved to the "
        "cryptography.hazmat.primitives.asymmetric.ec module"
    ),
    utils.DeprecatedIn08
)


DSAParameters = utils.deprecated(
    dsa.DSAParameters,
    __name__,
    (
        "The DSAParameters interface has moved to the "
        "cryptography.hazmat.primitives.asymmetric.dsa.module"
    ),
    utils.DeprecatedIn08
)

DSAParametersWithNumbers = utils.deprecated(
    dsa.DSAParametersWithNumbers,
    __name__,
    (
        "The DSAParametersWithNumbers interface has moved to the "
        "cryptography.hazmat.primitives.asymmetric.dsa.module"
    ),
    utils.DeprecatedIn08
)

DSAPrivateKey = utils.deprecated(
    dsa.DSAPrivateKey,
    __name__,
    (
        "The DSAPrivateKey interface has moved to the "
        "cryptography.hazmat.primitives.asymmetric.dsa.module"
    ),
    utils.DeprecatedIn08
)

DSAPrivateKeyWithNumbers = utils.deprecated(
    dsa.DSAPrivateKeyWithNumbers,
    __name__,
    (
        "The DSAPrivateKeyWithNumbers interface has moved to the "
        "cryptography.hazmat.primitives.asymmetric.dsa.module"
    ),
    utils.DeprecatedIn08
)

DSAPublicKey = utils.deprecated(
    dsa.DSAPublicKey,
    __name__,
    (
        "The DSAPublicKeyWithNumbers interface has moved to the "
        "cryptography.hazmat.primitives.asymmetric.dsa.module"
    ),
    utils.DeprecatedIn08
)

DSAPublicKeyWithNumbers = utils.deprecated(
    dsa.DSAPublicKeyWithNumbers,
    __name__,
    (
        "The DSAPublicKeyWithNumbers interface has moved to the "
        "cryptography.hazmat.primitives.asymmetric.dsa.module"
    ),
    utils.DeprecatedIn08
)


PaddingContext = utils.deprecated(
    PaddingContext,
    __name__,
    (
        "The PaddingContext interface has moved to the "
        "cryptography.hazmat.primitives.padding module"
    ),
    utils.DeprecatedIn08
)


HashContext = utils.deprecated(
    hashes.HashContext,
    __name__,
    (
        "The HashContext interface has moved to the "
        "cryptography.hazmat.primitives.hashes module"
    ),
    utils.DeprecatedIn08
)


HashAlgorithm = utils.deprecated(
    hashes.HashAlgorithm,
    __name__,
    (
        "The HashAlgorithm interface has moved to the "
        "cryptography.hazmat.primitives.hashes module"
    ),
    utils.DeprecatedIn08
)


RSAPrivateKey = utils.deprecated(
    rsa.RSAPrivateKey,
    __name__,
    (
        "The RSAPrivateKey interface has moved to the "
        "cryptography.hazmat.primitives.asymmetric.rsa module"
    ),
    utils.DeprecatedIn08
)

RSAPrivateKeyWithNumbers = utils.deprecated(
    rsa.RSAPrivateKeyWithSerialization,
    __name__,
    (
        "The RSAPrivateKeyWithNumbers interface has moved to the "
        "cryptography.hazmat.primitives.asymmetric.rsa module and has been "
        "renamed RSAPrivateKeyWithSerialization"
    ),
    utils.DeprecatedIn08
)

RSAPublicKey = utils.deprecated(
    rsa.RSAPublicKey,
    __name__,
    (
        "The RSAPublicKeyWithNumbers interface has moved to the "
        "cryptography.hazmat.primitives.asymmetric.rsa module"
    ),
    utils.DeprecatedIn08
)

RSAPublicKeyWithNumbers = utils.deprecated(
    rsa.RSAPublicKeyWithNumbers,
    __name__,
    (
        "The RSAPublicKeyWithNumbers interface has moved to the "
        "cryptography.hazmat.primitives.asymmetric.rsa module"
    ),
    utils.DeprecatedIn08
)

AsymmetricPadding = utils.deprecated(
    padding.AsymmetricPadding,
    __name__,
    (
        "The AsymmetricPadding interface has moved to the "
        "cryptography.hazmat.primitives.asymmetric.padding module"
    ),
    utils.DeprecatedIn08
)

AsymmetricSignatureContext = utils.deprecated(
    AsymmetricSignatureContext,
    __name__,
    (
        "The AsymmetricPadding interface has moved to the "
        "cryptography.hazmat.primitives.asymmetric module"
    ),
    utils.DeprecatedIn08
)

AsymmetricVerificationContext = utils.deprecated(
    AsymmetricVerificationContext,
    __name__,
    (
        "The AsymmetricVerificationContext interface has moved to the "
        "cryptography.hazmat.primitives.asymmetric module"
    ),
    utils.DeprecatedIn08
)

KeyDerivationFunction = utils.deprecated(
    KeyDerivationFunction,
    __name__,
    (
        "The KeyDerivationFunction interface has moved to the "
        "cryptography.hazmat.primitives.kdf module"
    ),
    utils.DeprecatedIn08
)


@six.add_metaclass(abc.ABCMeta)
class MACContext(object):
    @abc.abstractmethod
    def update(self, data):
        """
        Processes the provided bytes.
        """

    @abc.abstractmethod
    def finalize(self):
        """
        Returns the message authentication code as bytes.
        """

    @abc.abstractmethod
    def copy(self):
        """
        Return a MACContext that is a copy of the current context.
        """

    @abc.abstractmethod
    def verify(self, signature):
        """
        Checks if the generated message authentication code matches the
        signature.
        """

CMACContext = utils.deprecated(
    MACContext,
    __name__,
    "The CMACContext interface has been renamed to MACContext",
    utils.DeprecatedIn07
)

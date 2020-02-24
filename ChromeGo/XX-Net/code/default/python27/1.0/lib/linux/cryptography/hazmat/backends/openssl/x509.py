# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from __future__ import absolute_import, division, print_function

import datetime

from cryptography import utils, x509
from cryptography.exceptions import UnsupportedAlgorithm
from cryptography.hazmat.primitives import hashes


@utils.register_interface(x509.Certificate)
class _Certificate(object):
    def __init__(self, backend, x509):
        self._backend = backend
        self._x509 = x509

    def fingerprint(self, algorithm):
        h = hashes.Hash(algorithm, self._backend)
        bio = self._backend._create_mem_bio()
        res = self._backend._lib.i2d_X509_bio(
            bio, self._x509
        )
        assert res == 1
        der = self._backend._read_mem_bio(bio)
        h.update(der)
        return h.finalize()

    @property
    def version(self):
        version = self._backend._lib.X509_get_version(self._x509)
        if version == 0:
            return x509.Version.v1
        elif version == 2:
            return x509.Version.v3
        else:
            raise x509.InvalidVersion(
                "{0} is not a valid X509 version".format(version), version
            )

    @property
    def serial(self):
        asn1_int = self._backend._lib.X509_get_serialNumber(self._x509)
        assert asn1_int != self._backend._ffi.NULL
        bn = self._backend._lib.ASN1_INTEGER_to_BN(
            asn1_int, self._backend._ffi.NULL
        )
        assert bn != self._backend._ffi.NULL
        bn = self._backend._ffi.gc(bn, self._backend._lib.BN_free)
        return self._backend._bn_to_int(bn)

    def public_key(self):
        pkey = self._backend._lib.X509_get_pubkey(self._x509)
        assert pkey != self._backend._ffi.NULL
        pkey = self._backend._ffi.gc(pkey, self._backend._lib.EVP_PKEY_free)

        return self._backend._evp_pkey_to_public_key(pkey)

    @property
    def not_valid_before(self):
        asn1_time = self._backend._lib.X509_get_notBefore(self._x509)
        return self._parse_asn1_time(asn1_time)

    @property
    def not_valid_after(self):
        asn1_time = self._backend._lib.X509_get_notAfter(self._x509)
        return self._parse_asn1_time(asn1_time)

    def _parse_asn1_time(self, asn1_time):
        assert asn1_time != self._backend._ffi.NULL
        generalized_time = self._backend._lib.ASN1_TIME_to_generalizedtime(
            asn1_time, self._backend._ffi.NULL
        )
        assert generalized_time != self._backend._ffi.NULL
        generalized_time = self._backend._ffi.gc(
            generalized_time, self._backend._lib.ASN1_GENERALIZEDTIME_free
        )
        time = self._backend._ffi.string(
            self._backend._lib.ASN1_STRING_data(
                self._backend._ffi.cast("ASN1_STRING *", generalized_time)
            )
        ).decode("ascii")
        return datetime.datetime.strptime(time, "%Y%m%d%H%M%SZ")

    @property
    def issuer(self):
        issuer = self._backend._lib.X509_get_issuer_name(self._x509)
        assert issuer != self._backend._ffi.NULL
        return self._build_x509_name(issuer)

    @property
    def subject(self):
        subject = self._backend._lib.X509_get_subject_name(self._x509)
        assert subject != self._backend._ffi.NULL
        return self._build_x509_name(subject)

    def _build_x509_name(self, x509_name):
        count = self._backend._lib.X509_NAME_entry_count(x509_name)
        attributes = []
        for x in range(count):
            entry = self._backend._lib.X509_NAME_get_entry(x509_name, x)
            obj = self._backend._lib.X509_NAME_ENTRY_get_object(entry)
            assert obj != self._backend._ffi.NULL
            data = self._backend._lib.X509_NAME_ENTRY_get_data(entry)
            assert data != self._backend._ffi.NULL
            buf = self._backend._ffi.new("unsigned char **")
            res = self._backend._lib.ASN1_STRING_to_UTF8(buf, data)
            assert res >= 0
            assert buf[0] != self._backend._ffi.NULL
            buf = self._backend._ffi.gc(
                buf, lambda buf: self._backend._lib.OPENSSL_free(buf[0])
            )
            value = self._backend._ffi.buffer(buf[0], res)[:].decode('utf8')
            oid = self._obj2txt(obj)
            attributes.append(
                x509.NameAttribute(
                    x509.ObjectIdentifier(oid), value
                )
            )

        return x509.Name(attributes)

    def _obj2txt(self, obj):
        # Set to 80 on the recommendation of
        # https://www.openssl.org/docs/crypto/OBJ_nid2ln.html#return_values
        buf_len = 80
        buf = self._backend._ffi.new("char[]", buf_len)
        res = self._backend._lib.OBJ_obj2txt(buf, buf_len, obj, 1)
        assert res > 0
        return self._backend._ffi.buffer(buf, res)[:].decode()

    @property
    def signature_hash_algorithm(self):
        oid = self._obj2txt(self._x509.sig_alg.algorithm)
        try:
            return x509._SIG_OIDS_TO_HASH[oid]
        except KeyError:
            raise UnsupportedAlgorithm(
                "Signature algorithm OID:{0} not recognized".format(oid)
            )

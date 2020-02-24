# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

INCLUDES = """
#include <openssl/obj_mac.h>
"""

TYPES = """
static const int Cryptography_HAS_ECDSA_SHA2_NIDS;

static const int NID_undef;
static const int NID_dsa;
static const int NID_dsaWithSHA;
static const int NID_dsaWithSHA1;
static const int NID_md2;
static const int NID_md4;
static const int NID_md5;
static const int NID_mdc2;
static const int NID_ripemd160;
static const int NID_sha;
static const int NID_sha1;
static const int NID_sha256;
static const int NID_sha384;
static const int NID_sha512;
static const int NID_sha224;
static const int NID_sha;
static const int NID_ecdsa_with_SHA1;
static const int NID_ecdsa_with_SHA224;
static const int NID_ecdsa_with_SHA256;
static const int NID_ecdsa_with_SHA384;
static const int NID_ecdsa_with_SHA512;
static const int NID_pbe_WithSHA1And3_Key_TripleDES_CBC;
static const int NID_X9_62_c2pnb163v1;
static const int NID_X9_62_c2pnb163v2;
static const int NID_X9_62_c2pnb163v3;
static const int NID_X9_62_c2pnb176v1;
static const int NID_X9_62_c2tnb191v1;
static const int NID_X9_62_c2tnb191v2;
static const int NID_X9_62_c2tnb191v3;
static const int NID_X9_62_c2onb191v4;
static const int NID_X9_62_c2onb191v5;
static const int NID_X9_62_c2pnb208w1;
static const int NID_X9_62_c2tnb239v1;
static const int NID_X9_62_c2tnb239v2;
static const int NID_X9_62_c2tnb239v3;
static const int NID_X9_62_c2onb239v4;
static const int NID_X9_62_c2onb239v5;
static const int NID_X9_62_c2pnb272w1;
static const int NID_X9_62_c2pnb304w1;
static const int NID_X9_62_c2tnb359v1;
static const int NID_X9_62_c2pnb368w1;
static const int NID_X9_62_c2tnb431r1;
static const int NID_X9_62_prime192v1;
static const int NID_X9_62_prime192v2;
static const int NID_X9_62_prime192v3;
static const int NID_X9_62_prime239v1;
static const int NID_X9_62_prime239v2;
static const int NID_X9_62_prime239v3;
static const int NID_X9_62_prime256v1;
static const int NID_secp112r1;
static const int NID_secp112r2;
static const int NID_secp128r1;
static const int NID_secp128r2;
static const int NID_secp160k1;
static const int NID_secp160r1;
static const int NID_secp160r2;
static const int NID_sect163k1;
static const int NID_sect163r1;
static const int NID_sect163r2;
static const int NID_secp192k1;
static const int NID_secp224k1;
static const int NID_secp224r1;
static const int NID_secp256k1;
static const int NID_secp384r1;
static const int NID_secp521r1;
static const int NID_sect113r1;
static const int NID_sect113r2;
static const int NID_sect131r1;
static const int NID_sect131r2;
static const int NID_sect193r1;
static const int NID_sect193r2;
static const int NID_sect233k1;
static const int NID_sect233r1;
static const int NID_sect239k1;
static const int NID_sect283k1;
static const int NID_sect283r1;
static const int NID_sect409k1;
static const int NID_sect409r1;
static const int NID_sect571k1;
static const int NID_sect571r1;
static const int NID_wap_wsg_idm_ecid_wtls1;
static const int NID_wap_wsg_idm_ecid_wtls3;
static const int NID_wap_wsg_idm_ecid_wtls4;
static const int NID_wap_wsg_idm_ecid_wtls5;
static const int NID_wap_wsg_idm_ecid_wtls6;
static const int NID_wap_wsg_idm_ecid_wtls7;
static const int NID_wap_wsg_idm_ecid_wtls8;
static const int NID_wap_wsg_idm_ecid_wtls9;
static const int NID_wap_wsg_idm_ecid_wtls10;
static const int NID_wap_wsg_idm_ecid_wtls11;
static const int NID_wap_wsg_idm_ecid_wtls12;
static const int NID_ipsec3;
static const int NID_ipsec4;
static const char *const SN_X9_62_c2pnb163v1;
static const char *const SN_X9_62_c2pnb163v2;
static const char *const SN_X9_62_c2pnb163v3;
static const char *const SN_X9_62_c2pnb176v1;
static const char *const SN_X9_62_c2tnb191v1;
static const char *const SN_X9_62_c2tnb191v2;
static const char *const SN_X9_62_c2tnb191v3;
static const char *const SN_X9_62_c2onb191v4;
static const char *const SN_X9_62_c2onb191v5;
static const char *const SN_X9_62_c2pnb208w1;
static const char *const SN_X9_62_c2tnb239v1;
static const char *const SN_X9_62_c2tnb239v2;
static const char *const SN_X9_62_c2tnb239v3;
static const char *const SN_X9_62_c2onb239v4;
static const char *const SN_X9_62_c2onb239v5;
static const char *const SN_X9_62_c2pnb272w1;
static const char *const SN_X9_62_c2pnb304w1;
static const char *const SN_X9_62_c2tnb359v1;
static const char *const SN_X9_62_c2pnb368w1;
static const char *const SN_X9_62_c2tnb431r1;
static const char *const SN_X9_62_prime192v1;
static const char *const SN_X9_62_prime192v2;
static const char *const SN_X9_62_prime192v3;
static const char *const SN_X9_62_prime239v1;
static const char *const SN_X9_62_prime239v2;
static const char *const SN_X9_62_prime239v3;
static const char *const SN_X9_62_prime256v1;
static const char *const SN_secp112r1;
static const char *const SN_secp112r2;
static const char *const SN_secp128r1;
static const char *const SN_secp128r2;
static const char *const SN_secp160k1;
static const char *const SN_secp160r1;
static const char *const SN_secp160r2;
static const char *const SN_sect163k1;
static const char *const SN_sect163r1;
static const char *const SN_sect163r2;
static const char *const SN_secp192k1;
static const char *const SN_secp224k1;
static const char *const SN_secp224r1;
static const char *const SN_secp256k1;
static const char *const SN_secp384r1;
static const char *const SN_secp521r1;
static const char *const SN_sect113r1;
static const char *const SN_sect113r2;
static const char *const SN_sect131r1;
static const char *const SN_sect131r2;
static const char *const SN_sect193r1;
static const char *const SN_sect193r2;
static const char *const SN_sect233k1;
static const char *const SN_sect233r1;
static const char *const SN_sect239k1;
static const char *const SN_sect283k1;
static const char *const SN_sect283r1;
static const char *const SN_sect409k1;
static const char *const SN_sect409r1;
static const char *const SN_sect571k1;
static const char *const SN_sect571r1;
static const char *const SN_wap_wsg_idm_ecid_wtls1;
static const char *const SN_wap_wsg_idm_ecid_wtls3;
static const char *const SN_wap_wsg_idm_ecid_wtls4;
static const char *const SN_wap_wsg_idm_ecid_wtls5;
static const char *const SN_wap_wsg_idm_ecid_wtls6;
static const char *const SN_wap_wsg_idm_ecid_wtls7;
static const char *const SN_wap_wsg_idm_ecid_wtls8;
static const char *const SN_wap_wsg_idm_ecid_wtls9;
static const char *const SN_wap_wsg_idm_ecid_wtls10;
static const char *const SN_wap_wsg_idm_ecid_wtls11;
static const char *const SN_wap_wsg_idm_ecid_wtls12;
static const char *const SN_ipsec3;
static const char *const SN_ipsec4;

static const int NID_subject_key_identifier;
static const int NID_authority_key_identifier;
static const int NID_policy_constraints;
static const int NID_ext_key_usage;
static const int NID_info_access;
static const int NID_key_usage;
static const int NID_subject_alt_name;
static const int NID_issuer_alt_name;
static const int NID_basic_constraints;
static const int NID_issuing_distribution_point;
static const int NID_certificate_issuer;
static const int NID_name_constraints;
static const int NID_crl_distribution_points;
static const int NID_certificate_policies;
static const int NID_inhibit_any_policy;

static const int NID_private_key_usage_period;
static const int NID_crl_number;
static const int NID_crl_reason;
static const int NID_invalidity_date;
static const int NID_delta_crl;
static const int NID_any_policy;
static const int NID_policy_mappings;
static const int NID_target_information;
static const int NID_no_rev_avail;

static const int NID_commonName;
static const int NID_countryName;
static const int NID_localityName;
static const int NID_stateOrProvinceName;
static const int NID_organizationName;
static const int NID_organizationalUnitName;
static const int NID_serialNumber;
static const int NID_surname;
static const int NID_givenName;
static const int NID_title;
static const int NID_generationQualifier;
static const int NID_dnQualifier;
static const int NID_pseudonym;
static const int NID_domainComponent;
static const int NID_pkcs9_emailAddress;
"""

FUNCTIONS = """
"""

MACROS = """
"""

CUSTOMIZATIONS = """
/* OpenSSL 0.9.8g+ */
#if OPENSSL_VERSION_NUMBER >= 0x0090807fL
static const long Cryptography_HAS_ECDSA_SHA2_NIDS = 1;
#else
static const long Cryptography_HAS_ECDSA_SHA2_NIDS = 0;
static const int NID_ecdsa_with_SHA224 = 0;
static const int NID_ecdsa_with_SHA256 = 0;
static const int NID_ecdsa_with_SHA384 = 0;
static const int NID_ecdsa_with_SHA512 = 0;
#endif
"""

CONDITIONAL_NAMES = {
    "Cryptography_HAS_ECDSA_SHA2_NIDS": [
        "NID_ecdsa_with_SHA224",
        "NID_ecdsa_with_SHA256",
        "NID_ecdsa_with_SHA384",
        "NID_ecdsa_with_SHA512",
    ],
}

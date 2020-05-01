/*
 *  Query Mbed TLS compile time configurations from config.h
 *
 *  Copyright (C) 2018, Arm Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of Mbed TLS (https://tls.mbed.org)
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#define mbedtls_printf printf
#endif /* MBEDTLS_PLATFORM_C */

/*
 * Include all the headers with public APIs in case they define a macro to its
 * default value when that configuration is not set in the config.h.
 */
#include "mbedtls/aes.h"
#include "mbedtls/aesni.h"
#include "mbedtls/arc4.h"
#include "mbedtls/aria.h"
#include "mbedtls/asn1.h"
#include "mbedtls/asn1write.h"
#include "mbedtls/base64.h"
#include "mbedtls/bignum.h"
#include "mbedtls/blowfish.h"
#include "mbedtls/camellia.h"
#include "mbedtls/ccm.h"
#include "mbedtls/certs.h"
#include "mbedtls/chacha20.h"
#include "mbedtls/chachapoly.h"
#include "mbedtls/cipher.h"
#include "mbedtls/cmac.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#include "mbedtls/des.h"
#include "mbedtls/dhm.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/ecjpake.h"
#include "mbedtls/ecp.h"
#include "mbedtls/entropy.h"
#include "mbedtls/entropy_poll.h"
#include "mbedtls/error.h"
#include "mbedtls/gcm.h"
#include "mbedtls/havege.h"
#include "mbedtls/hkdf.h"
#include "mbedtls/hmac_drbg.h"
#include "mbedtls/md.h"
#include "mbedtls/md2.h"
#include "mbedtls/md4.h"
#include "mbedtls/md5.h"
#include "mbedtls/memory_buffer_alloc.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/nist_kw.h"
#include "mbedtls/oid.h"
#include "mbedtls/padlock.h"
#include "mbedtls/pem.h"
#include "mbedtls/pk.h"
#include "mbedtls/pkcs11.h"
#include "mbedtls/pkcs12.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/platform_time.h"
#include "mbedtls/platform_util.h"
#include "mbedtls/poly1305.h"
#include "mbedtls/ripemd160.h"
#include "mbedtls/rsa.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#include "mbedtls/ssl.h"
#include "mbedtls/ssl_cache.h"
#include "mbedtls/ssl_ciphersuites.h"
#include "mbedtls/ssl_cookie.h"
#include "mbedtls/ssl_internal.h"
#include "mbedtls/ssl_ticket.h"
#include "mbedtls/threading.h"
#include "mbedtls/timing.h"
#include "mbedtls/version.h"
#include "mbedtls/x509.h"
#include "mbedtls/x509_crl.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/x509_csr.h"
#include "mbedtls/xtea.h"

#include <string.h>

/*
 * Helper macros to convert a macro or its expansion into a string
 * WARNING: This does not work for expanding function-like macros. However,
 * Mbed TLS does not currently have configuration options used in this fashion.
 */
#define MACRO_EXPANSION_TO_STR(macro)   MACRO_NAME_TO_STR(macro)
#define MACRO_NAME_TO_STR(macro)                                        \
    mbedtls_printf( "%s", strlen( #macro "" ) > 0 ? #macro "\n" : "" )

#if defined(_MSC_VER)
/*
 * Visual Studio throws the warning 4003 because many Mbed TLS feature macros
 * are defined empty. This means that from the preprocessor's point of view
 * the macro MBEDTLS_EXPANSION_TO_STR is being invoked without arguments as
 * some macros expand to nothing. We suppress that specific warning to get a
 * clean build and to ensure that tests treating warnings as errors do not
 * fail.
 */
#pragma warning(push)
#pragma warning(disable:4003)
#endif /* _MSC_VER */

int query_config( const char *config )
{
#if defined(MBEDTLS_HAVE_ASM)
    if( strcmp( "MBEDTLS_HAVE_ASM", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_HAVE_ASM );
        return( 0 );
    }
#endif /* MBEDTLS_HAVE_ASM */

#if defined(MBEDTLS_NO_UDBL_DIVISION)
    if( strcmp( "MBEDTLS_NO_UDBL_DIVISION", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_NO_UDBL_DIVISION );
        return( 0 );
    }
#endif /* MBEDTLS_NO_UDBL_DIVISION */

#if defined(MBEDTLS_NO_64BIT_MULTIPLICATION)
    if( strcmp( "MBEDTLS_NO_64BIT_MULTIPLICATION", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_NO_64BIT_MULTIPLICATION );
        return( 0 );
    }
#endif /* MBEDTLS_NO_64BIT_MULTIPLICATION */

#if defined(MBEDTLS_HAVE_SSE2)
    if( strcmp( "MBEDTLS_HAVE_SSE2", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_HAVE_SSE2 );
        return( 0 );
    }
#endif /* MBEDTLS_HAVE_SSE2 */

#if defined(MBEDTLS_HAVE_TIME)
    if( strcmp( "MBEDTLS_HAVE_TIME", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_HAVE_TIME );
        return( 0 );
    }
#endif /* MBEDTLS_HAVE_TIME */

#if defined(MBEDTLS_HAVE_TIME_DATE)
    if( strcmp( "MBEDTLS_HAVE_TIME_DATE", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_HAVE_TIME_DATE );
        return( 0 );
    }
#endif /* MBEDTLS_HAVE_TIME_DATE */

#if defined(MBEDTLS_PLATFORM_MEMORY)
    if( strcmp( "MBEDTLS_PLATFORM_MEMORY", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_MEMORY );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_MEMORY */

#if defined(MBEDTLS_PLATFORM_NO_STD_FUNCTIONS)
    if( strcmp( "MBEDTLS_PLATFORM_NO_STD_FUNCTIONS", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_NO_STD_FUNCTIONS );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_NO_STD_FUNCTIONS */

#if defined(MBEDTLS_PLATFORM_EXIT_ALT)
    if( strcmp( "MBEDTLS_PLATFORM_EXIT_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_EXIT_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_EXIT_ALT */

#if defined(MBEDTLS_PLATFORM_TIME_ALT)
    if( strcmp( "MBEDTLS_PLATFORM_TIME_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_TIME_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_TIME_ALT */

#if defined(MBEDTLS_PLATFORM_FPRINTF_ALT)
    if( strcmp( "MBEDTLS_PLATFORM_FPRINTF_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_FPRINTF_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_FPRINTF_ALT */

#if defined(MBEDTLS_PLATFORM_PRINTF_ALT)
    if( strcmp( "MBEDTLS_PLATFORM_PRINTF_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_PRINTF_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_PRINTF_ALT */

#if defined(MBEDTLS_PLATFORM_SNPRINTF_ALT)
    if( strcmp( "MBEDTLS_PLATFORM_SNPRINTF_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_SNPRINTF_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_SNPRINTF_ALT */

#if defined(MBEDTLS_PLATFORM_NV_SEED_ALT)
    if( strcmp( "MBEDTLS_PLATFORM_NV_SEED_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_NV_SEED_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_NV_SEED_ALT */

#if defined(MBEDTLS_PLATFORM_SETUP_TEARDOWN_ALT)
    if( strcmp( "MBEDTLS_PLATFORM_SETUP_TEARDOWN_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_SETUP_TEARDOWN_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_SETUP_TEARDOWN_ALT */

#if defined(MBEDTLS_DEPRECATED_WARNING)
    if( strcmp( "MBEDTLS_DEPRECATED_WARNING", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_DEPRECATED_WARNING );
        return( 0 );
    }
#endif /* MBEDTLS_DEPRECATED_WARNING */

#if defined(MBEDTLS_DEPRECATED_REMOVED)
    if( strcmp( "MBEDTLS_DEPRECATED_REMOVED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_DEPRECATED_REMOVED );
        return( 0 );
    }
#endif /* MBEDTLS_DEPRECATED_REMOVED */

#if defined(MBEDTLS_CHECK_PARAMS)
    if( strcmp( "MBEDTLS_CHECK_PARAMS", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CHECK_PARAMS );
        return( 0 );
    }
#endif /* MBEDTLS_CHECK_PARAMS */

#if defined(MBEDTLS_TIMING_ALT)
    if( strcmp( "MBEDTLS_TIMING_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_TIMING_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_TIMING_ALT */

#if defined(MBEDTLS_AES_ALT)
    if( strcmp( "MBEDTLS_AES_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_AES_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_AES_ALT */

#if defined(MBEDTLS_ARC4_ALT)
    if( strcmp( "MBEDTLS_ARC4_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ARC4_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_ARC4_ALT */

#if defined(MBEDTLS_ARIA_ALT)
    if( strcmp( "MBEDTLS_ARIA_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ARIA_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_ARIA_ALT */

#if defined(MBEDTLS_BLOWFISH_ALT)
    if( strcmp( "MBEDTLS_BLOWFISH_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_BLOWFISH_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_BLOWFISH_ALT */

#if defined(MBEDTLS_CAMELLIA_ALT)
    if( strcmp( "MBEDTLS_CAMELLIA_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CAMELLIA_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_CAMELLIA_ALT */

#if defined(MBEDTLS_CCM_ALT)
    if( strcmp( "MBEDTLS_CCM_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CCM_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_CCM_ALT */

#if defined(MBEDTLS_CHACHA20_ALT)
    if( strcmp( "MBEDTLS_CHACHA20_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CHACHA20_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_CHACHA20_ALT */

#if defined(MBEDTLS_CHACHAPOLY_ALT)
    if( strcmp( "MBEDTLS_CHACHAPOLY_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CHACHAPOLY_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_CHACHAPOLY_ALT */

#if defined(MBEDTLS_CMAC_ALT)
    if( strcmp( "MBEDTLS_CMAC_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CMAC_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_CMAC_ALT */

#if defined(MBEDTLS_DES_ALT)
    if( strcmp( "MBEDTLS_DES_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_DES_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_DES_ALT */

#if defined(MBEDTLS_DHM_ALT)
    if( strcmp( "MBEDTLS_DHM_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_DHM_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_DHM_ALT */

#if defined(MBEDTLS_ECJPAKE_ALT)
    if( strcmp( "MBEDTLS_ECJPAKE_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECJPAKE_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_ECJPAKE_ALT */

#if defined(MBEDTLS_GCM_ALT)
    if( strcmp( "MBEDTLS_GCM_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_GCM_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_GCM_ALT */

#if defined(MBEDTLS_NIST_KW_ALT)
    if( strcmp( "MBEDTLS_NIST_KW_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_NIST_KW_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_NIST_KW_ALT */

#if defined(MBEDTLS_MD2_ALT)
    if( strcmp( "MBEDTLS_MD2_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_MD2_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_MD2_ALT */

#if defined(MBEDTLS_MD4_ALT)
    if( strcmp( "MBEDTLS_MD4_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_MD4_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_MD4_ALT */

#if defined(MBEDTLS_MD5_ALT)
    if( strcmp( "MBEDTLS_MD5_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_MD5_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_MD5_ALT */

#if defined(MBEDTLS_POLY1305_ALT)
    if( strcmp( "MBEDTLS_POLY1305_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_POLY1305_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_POLY1305_ALT */

#if defined(MBEDTLS_RIPEMD160_ALT)
    if( strcmp( "MBEDTLS_RIPEMD160_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_RIPEMD160_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_RIPEMD160_ALT */

#if defined(MBEDTLS_RSA_ALT)
    if( strcmp( "MBEDTLS_RSA_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_RSA_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_RSA_ALT */

#if defined(MBEDTLS_SHA1_ALT)
    if( strcmp( "MBEDTLS_SHA1_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SHA1_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_SHA1_ALT */

#if defined(MBEDTLS_SHA256_ALT)
    if( strcmp( "MBEDTLS_SHA256_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SHA256_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_SHA256_ALT */

#if defined(MBEDTLS_SHA512_ALT)
    if( strcmp( "MBEDTLS_SHA512_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SHA512_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_SHA512_ALT */

#if defined(MBEDTLS_XTEA_ALT)
    if( strcmp( "MBEDTLS_XTEA_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_XTEA_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_XTEA_ALT */

#if defined(MBEDTLS_ECP_ALT)
    if( strcmp( "MBEDTLS_ECP_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_ALT */

#if defined(MBEDTLS_MD2_PROCESS_ALT)
    if( strcmp( "MBEDTLS_MD2_PROCESS_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_MD2_PROCESS_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_MD2_PROCESS_ALT */

#if defined(MBEDTLS_MD4_PROCESS_ALT)
    if( strcmp( "MBEDTLS_MD4_PROCESS_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_MD4_PROCESS_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_MD4_PROCESS_ALT */

#if defined(MBEDTLS_MD5_PROCESS_ALT)
    if( strcmp( "MBEDTLS_MD5_PROCESS_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_MD5_PROCESS_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_MD5_PROCESS_ALT */

#if defined(MBEDTLS_RIPEMD160_PROCESS_ALT)
    if( strcmp( "MBEDTLS_RIPEMD160_PROCESS_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_RIPEMD160_PROCESS_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_RIPEMD160_PROCESS_ALT */

#if defined(MBEDTLS_SHA1_PROCESS_ALT)
    if( strcmp( "MBEDTLS_SHA1_PROCESS_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SHA1_PROCESS_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_SHA1_PROCESS_ALT */

#if defined(MBEDTLS_SHA256_PROCESS_ALT)
    if( strcmp( "MBEDTLS_SHA256_PROCESS_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SHA256_PROCESS_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_SHA256_PROCESS_ALT */

#if defined(MBEDTLS_SHA512_PROCESS_ALT)
    if( strcmp( "MBEDTLS_SHA512_PROCESS_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SHA512_PROCESS_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_SHA512_PROCESS_ALT */

#if defined(MBEDTLS_DES_SETKEY_ALT)
    if( strcmp( "MBEDTLS_DES_SETKEY_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_DES_SETKEY_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_DES_SETKEY_ALT */

#if defined(MBEDTLS_DES_CRYPT_ECB_ALT)
    if( strcmp( "MBEDTLS_DES_CRYPT_ECB_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_DES_CRYPT_ECB_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_DES_CRYPT_ECB_ALT */

#if defined(MBEDTLS_DES3_CRYPT_ECB_ALT)
    if( strcmp( "MBEDTLS_DES3_CRYPT_ECB_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_DES3_CRYPT_ECB_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_DES3_CRYPT_ECB_ALT */

#if defined(MBEDTLS_AES_SETKEY_ENC_ALT)
    if( strcmp( "MBEDTLS_AES_SETKEY_ENC_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_AES_SETKEY_ENC_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_AES_SETKEY_ENC_ALT */

#if defined(MBEDTLS_AES_SETKEY_DEC_ALT)
    if( strcmp( "MBEDTLS_AES_SETKEY_DEC_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_AES_SETKEY_DEC_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_AES_SETKEY_DEC_ALT */

#if defined(MBEDTLS_AES_ENCRYPT_ALT)
    if( strcmp( "MBEDTLS_AES_ENCRYPT_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_AES_ENCRYPT_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_AES_ENCRYPT_ALT */

#if defined(MBEDTLS_AES_DECRYPT_ALT)
    if( strcmp( "MBEDTLS_AES_DECRYPT_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_AES_DECRYPT_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_AES_DECRYPT_ALT */

#if defined(MBEDTLS_ECDH_GEN_PUBLIC_ALT)
    if( strcmp( "MBEDTLS_ECDH_GEN_PUBLIC_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECDH_GEN_PUBLIC_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_ECDH_GEN_PUBLIC_ALT */

#if defined(MBEDTLS_ECDH_COMPUTE_SHARED_ALT)
    if( strcmp( "MBEDTLS_ECDH_COMPUTE_SHARED_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECDH_COMPUTE_SHARED_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_ECDH_COMPUTE_SHARED_ALT */

#if defined(MBEDTLS_ECDSA_VERIFY_ALT)
    if( strcmp( "MBEDTLS_ECDSA_VERIFY_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECDSA_VERIFY_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_ECDSA_VERIFY_ALT */

#if defined(MBEDTLS_ECDSA_SIGN_ALT)
    if( strcmp( "MBEDTLS_ECDSA_SIGN_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECDSA_SIGN_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_ECDSA_SIGN_ALT */

#if defined(MBEDTLS_ECDSA_GENKEY_ALT)
    if( strcmp( "MBEDTLS_ECDSA_GENKEY_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECDSA_GENKEY_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_ECDSA_GENKEY_ALT */

#if defined(MBEDTLS_ECP_INTERNAL_ALT)
    if( strcmp( "MBEDTLS_ECP_INTERNAL_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_INTERNAL_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_INTERNAL_ALT */

#if defined(MBEDTLS_ECP_RANDOMIZE_JAC_ALT)
    if( strcmp( "MBEDTLS_ECP_RANDOMIZE_JAC_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_RANDOMIZE_JAC_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_RANDOMIZE_JAC_ALT */

#if defined(MBEDTLS_ECP_ADD_MIXED_ALT)
    if( strcmp( "MBEDTLS_ECP_ADD_MIXED_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_ADD_MIXED_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_ADD_MIXED_ALT */

#if defined(MBEDTLS_ECP_DOUBLE_JAC_ALT)
    if( strcmp( "MBEDTLS_ECP_DOUBLE_JAC_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_DOUBLE_JAC_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_DOUBLE_JAC_ALT */

#if defined(MBEDTLS_ECP_NORMALIZE_JAC_MANY_ALT)
    if( strcmp( "MBEDTLS_ECP_NORMALIZE_JAC_MANY_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_NORMALIZE_JAC_MANY_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_NORMALIZE_JAC_MANY_ALT */

#if defined(MBEDTLS_ECP_NORMALIZE_JAC_ALT)
    if( strcmp( "MBEDTLS_ECP_NORMALIZE_JAC_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_NORMALIZE_JAC_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_NORMALIZE_JAC_ALT */

#if defined(MBEDTLS_ECP_DOUBLE_ADD_MXZ_ALT)
    if( strcmp( "MBEDTLS_ECP_DOUBLE_ADD_MXZ_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_DOUBLE_ADD_MXZ_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_DOUBLE_ADD_MXZ_ALT */

#if defined(MBEDTLS_ECP_RANDOMIZE_MXZ_ALT)
    if( strcmp( "MBEDTLS_ECP_RANDOMIZE_MXZ_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_RANDOMIZE_MXZ_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_RANDOMIZE_MXZ_ALT */

#if defined(MBEDTLS_ECP_NORMALIZE_MXZ_ALT)
    if( strcmp( "MBEDTLS_ECP_NORMALIZE_MXZ_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_NORMALIZE_MXZ_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_NORMALIZE_MXZ_ALT */

#if defined(MBEDTLS_TEST_NULL_ENTROPY)
    if( strcmp( "MBEDTLS_TEST_NULL_ENTROPY", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_TEST_NULL_ENTROPY );
        return( 0 );
    }
#endif /* MBEDTLS_TEST_NULL_ENTROPY */

#if defined(MBEDTLS_ENTROPY_HARDWARE_ALT)
    if( strcmp( "MBEDTLS_ENTROPY_HARDWARE_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ENTROPY_HARDWARE_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_ENTROPY_HARDWARE_ALT */

#if defined(MBEDTLS_AES_ROM_TABLES)
    if( strcmp( "MBEDTLS_AES_ROM_TABLES", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_AES_ROM_TABLES );
        return( 0 );
    }
#endif /* MBEDTLS_AES_ROM_TABLES */

#if defined(MBEDTLS_AES_FEWER_TABLES)
    if( strcmp( "MBEDTLS_AES_FEWER_TABLES", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_AES_FEWER_TABLES );
        return( 0 );
    }
#endif /* MBEDTLS_AES_FEWER_TABLES */

#if defined(MBEDTLS_CAMELLIA_SMALL_MEMORY)
    if( strcmp( "MBEDTLS_CAMELLIA_SMALL_MEMORY", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CAMELLIA_SMALL_MEMORY );
        return( 0 );
    }
#endif /* MBEDTLS_CAMELLIA_SMALL_MEMORY */

#if defined(MBEDTLS_CIPHER_MODE_CBC)
    if( strcmp( "MBEDTLS_CIPHER_MODE_CBC", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CIPHER_MODE_CBC );
        return( 0 );
    }
#endif /* MBEDTLS_CIPHER_MODE_CBC */

#if defined(MBEDTLS_CIPHER_MODE_CFB)
    if( strcmp( "MBEDTLS_CIPHER_MODE_CFB", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CIPHER_MODE_CFB );
        return( 0 );
    }
#endif /* MBEDTLS_CIPHER_MODE_CFB */

#if defined(MBEDTLS_CIPHER_MODE_CTR)
    if( strcmp( "MBEDTLS_CIPHER_MODE_CTR", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CIPHER_MODE_CTR );
        return( 0 );
    }
#endif /* MBEDTLS_CIPHER_MODE_CTR */

#if defined(MBEDTLS_CIPHER_MODE_OFB)
    if( strcmp( "MBEDTLS_CIPHER_MODE_OFB", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CIPHER_MODE_OFB );
        return( 0 );
    }
#endif /* MBEDTLS_CIPHER_MODE_OFB */

#if defined(MBEDTLS_CIPHER_MODE_XTS)
    if( strcmp( "MBEDTLS_CIPHER_MODE_XTS", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CIPHER_MODE_XTS );
        return( 0 );
    }
#endif /* MBEDTLS_CIPHER_MODE_XTS */

#if defined(MBEDTLS_CIPHER_NULL_CIPHER)
    if( strcmp( "MBEDTLS_CIPHER_NULL_CIPHER", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CIPHER_NULL_CIPHER );
        return( 0 );
    }
#endif /* MBEDTLS_CIPHER_NULL_CIPHER */

#if defined(MBEDTLS_CIPHER_PADDING_PKCS7)
    if( strcmp( "MBEDTLS_CIPHER_PADDING_PKCS7", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CIPHER_PADDING_PKCS7 );
        return( 0 );
    }
#endif /* MBEDTLS_CIPHER_PADDING_PKCS7 */

#if defined(MBEDTLS_CIPHER_PADDING_ONE_AND_ZEROS)
    if( strcmp( "MBEDTLS_CIPHER_PADDING_ONE_AND_ZEROS", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CIPHER_PADDING_ONE_AND_ZEROS );
        return( 0 );
    }
#endif /* MBEDTLS_CIPHER_PADDING_ONE_AND_ZEROS */

#if defined(MBEDTLS_CIPHER_PADDING_ZEROS_AND_LEN)
    if( strcmp( "MBEDTLS_CIPHER_PADDING_ZEROS_AND_LEN", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CIPHER_PADDING_ZEROS_AND_LEN );
        return( 0 );
    }
#endif /* MBEDTLS_CIPHER_PADDING_ZEROS_AND_LEN */

#if defined(MBEDTLS_CIPHER_PADDING_ZEROS)
    if( strcmp( "MBEDTLS_CIPHER_PADDING_ZEROS", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CIPHER_PADDING_ZEROS );
        return( 0 );
    }
#endif /* MBEDTLS_CIPHER_PADDING_ZEROS */

#if defined(MBEDTLS_ENABLE_WEAK_CIPHERSUITES)
    if( strcmp( "MBEDTLS_ENABLE_WEAK_CIPHERSUITES", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ENABLE_WEAK_CIPHERSUITES );
        return( 0 );
    }
#endif /* MBEDTLS_ENABLE_WEAK_CIPHERSUITES */

#if defined(MBEDTLS_REMOVE_ARC4_CIPHERSUITES)
    if( strcmp( "MBEDTLS_REMOVE_ARC4_CIPHERSUITES", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_REMOVE_ARC4_CIPHERSUITES );
        return( 0 );
    }
#endif /* MBEDTLS_REMOVE_ARC4_CIPHERSUITES */

#if defined(MBEDTLS_REMOVE_3DES_CIPHERSUITES)
    if( strcmp( "MBEDTLS_REMOVE_3DES_CIPHERSUITES", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_REMOVE_3DES_CIPHERSUITES );
        return( 0 );
    }
#endif /* MBEDTLS_REMOVE_3DES_CIPHERSUITES */

#if defined(MBEDTLS_ECP_DP_SECP192R1_ENABLED)
    if( strcmp( "MBEDTLS_ECP_DP_SECP192R1_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_DP_SECP192R1_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_DP_SECP192R1_ENABLED */

#if defined(MBEDTLS_ECP_DP_SECP224R1_ENABLED)
    if( strcmp( "MBEDTLS_ECP_DP_SECP224R1_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_DP_SECP224R1_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_DP_SECP224R1_ENABLED */

#if defined(MBEDTLS_ECP_DP_SECP256R1_ENABLED)
    if( strcmp( "MBEDTLS_ECP_DP_SECP256R1_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_DP_SECP256R1_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_DP_SECP256R1_ENABLED */

#if defined(MBEDTLS_ECP_DP_SECP384R1_ENABLED)
    if( strcmp( "MBEDTLS_ECP_DP_SECP384R1_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_DP_SECP384R1_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_DP_SECP384R1_ENABLED */

#if defined(MBEDTLS_ECP_DP_SECP521R1_ENABLED)
    if( strcmp( "MBEDTLS_ECP_DP_SECP521R1_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_DP_SECP521R1_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_DP_SECP521R1_ENABLED */

#if defined(MBEDTLS_ECP_DP_SECP192K1_ENABLED)
    if( strcmp( "MBEDTLS_ECP_DP_SECP192K1_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_DP_SECP192K1_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_DP_SECP192K1_ENABLED */

#if defined(MBEDTLS_ECP_DP_SECP224K1_ENABLED)
    if( strcmp( "MBEDTLS_ECP_DP_SECP224K1_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_DP_SECP224K1_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_DP_SECP224K1_ENABLED */

#if defined(MBEDTLS_ECP_DP_SECP256K1_ENABLED)
    if( strcmp( "MBEDTLS_ECP_DP_SECP256K1_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_DP_SECP256K1_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_DP_SECP256K1_ENABLED */

#if defined(MBEDTLS_ECP_DP_BP256R1_ENABLED)
    if( strcmp( "MBEDTLS_ECP_DP_BP256R1_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_DP_BP256R1_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_DP_BP256R1_ENABLED */

#if defined(MBEDTLS_ECP_DP_BP384R1_ENABLED)
    if( strcmp( "MBEDTLS_ECP_DP_BP384R1_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_DP_BP384R1_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_DP_BP384R1_ENABLED */

#if defined(MBEDTLS_ECP_DP_BP512R1_ENABLED)
    if( strcmp( "MBEDTLS_ECP_DP_BP512R1_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_DP_BP512R1_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_DP_BP512R1_ENABLED */

#if defined(MBEDTLS_ECP_DP_CURVE25519_ENABLED)
    if( strcmp( "MBEDTLS_ECP_DP_CURVE25519_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_DP_CURVE25519_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_DP_CURVE25519_ENABLED */

#if defined(MBEDTLS_ECP_DP_CURVE448_ENABLED)
    if( strcmp( "MBEDTLS_ECP_DP_CURVE448_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_DP_CURVE448_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_DP_CURVE448_ENABLED */

#if defined(MBEDTLS_ECP_NIST_OPTIM)
    if( strcmp( "MBEDTLS_ECP_NIST_OPTIM", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_NIST_OPTIM );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_NIST_OPTIM */

#if defined(MBEDTLS_ECP_RESTARTABLE)
    if( strcmp( "MBEDTLS_ECP_RESTARTABLE", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_RESTARTABLE );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_RESTARTABLE */

#if defined(MBEDTLS_ECDSA_DETERMINISTIC)
    if( strcmp( "MBEDTLS_ECDSA_DETERMINISTIC", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECDSA_DETERMINISTIC );
        return( 0 );
    }
#endif /* MBEDTLS_ECDSA_DETERMINISTIC */

#if defined(MBEDTLS_KEY_EXCHANGE_PSK_ENABLED)
    if( strcmp( "MBEDTLS_KEY_EXCHANGE_PSK_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_KEY_EXCHANGE_PSK_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_KEY_EXCHANGE_PSK_ENABLED */

#if defined(MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED)
    if( strcmp( "MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED */

#if defined(MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED)
    if( strcmp( "MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED */

#if defined(MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED)
    if( strcmp( "MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED */

#if defined(MBEDTLS_KEY_EXCHANGE_RSA_ENABLED)
    if( strcmp( "MBEDTLS_KEY_EXCHANGE_RSA_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_KEY_EXCHANGE_RSA_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_KEY_EXCHANGE_RSA_ENABLED */

#if defined(MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED)
    if( strcmp( "MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED */

#if defined(MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED)
    if( strcmp( "MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED */

#if defined(MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED)
    if( strcmp( "MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED */

#if defined(MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED)
    if( strcmp( "MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED */

#if defined(MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED)
    if( strcmp( "MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED */

#if defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED)
    if( strcmp( "MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED );
        return( 0 );
    }
#endif /* MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED */

#if defined(MBEDTLS_PK_PARSE_EC_EXTENDED)
    if( strcmp( "MBEDTLS_PK_PARSE_EC_EXTENDED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PK_PARSE_EC_EXTENDED );
        return( 0 );
    }
#endif /* MBEDTLS_PK_PARSE_EC_EXTENDED */

#if defined(MBEDTLS_ERROR_STRERROR_DUMMY)
    if( strcmp( "MBEDTLS_ERROR_STRERROR_DUMMY", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ERROR_STRERROR_DUMMY );
        return( 0 );
    }
#endif /* MBEDTLS_ERROR_STRERROR_DUMMY */

#if defined(MBEDTLS_GENPRIME)
    if( strcmp( "MBEDTLS_GENPRIME", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_GENPRIME );
        return( 0 );
    }
#endif /* MBEDTLS_GENPRIME */

#if defined(MBEDTLS_FS_IO)
    if( strcmp( "MBEDTLS_FS_IO", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_FS_IO );
        return( 0 );
    }
#endif /* MBEDTLS_FS_IO */

#if defined(MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES)
    if( strcmp( "MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES );
        return( 0 );
    }
#endif /* MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES */

#if defined(MBEDTLS_NO_PLATFORM_ENTROPY)
    if( strcmp( "MBEDTLS_NO_PLATFORM_ENTROPY", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_NO_PLATFORM_ENTROPY );
        return( 0 );
    }
#endif /* MBEDTLS_NO_PLATFORM_ENTROPY */

#if defined(MBEDTLS_ENTROPY_FORCE_SHA256)
    if( strcmp( "MBEDTLS_ENTROPY_FORCE_SHA256", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ENTROPY_FORCE_SHA256 );
        return( 0 );
    }
#endif /* MBEDTLS_ENTROPY_FORCE_SHA256 */

#if defined(MBEDTLS_ENTROPY_NV_SEED)
    if( strcmp( "MBEDTLS_ENTROPY_NV_SEED", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ENTROPY_NV_SEED );
        return( 0 );
    }
#endif /* MBEDTLS_ENTROPY_NV_SEED */

#if defined(MBEDTLS_MEMORY_DEBUG)
    if( strcmp( "MBEDTLS_MEMORY_DEBUG", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_MEMORY_DEBUG );
        return( 0 );
    }
#endif /* MBEDTLS_MEMORY_DEBUG */

#if defined(MBEDTLS_MEMORY_BACKTRACE)
    if( strcmp( "MBEDTLS_MEMORY_BACKTRACE", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_MEMORY_BACKTRACE );
        return( 0 );
    }
#endif /* MBEDTLS_MEMORY_BACKTRACE */

#if defined(MBEDTLS_PK_RSA_ALT_SUPPORT)
    if( strcmp( "MBEDTLS_PK_RSA_ALT_SUPPORT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PK_RSA_ALT_SUPPORT );
        return( 0 );
    }
#endif /* MBEDTLS_PK_RSA_ALT_SUPPORT */

#if defined(MBEDTLS_PKCS1_V15)
    if( strcmp( "MBEDTLS_PKCS1_V15", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PKCS1_V15 );
        return( 0 );
    }
#endif /* MBEDTLS_PKCS1_V15 */

#if defined(MBEDTLS_PKCS1_V21)
    if( strcmp( "MBEDTLS_PKCS1_V21", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PKCS1_V21 );
        return( 0 );
    }
#endif /* MBEDTLS_PKCS1_V21 */

#if defined(MBEDTLS_RSA_NO_CRT)
    if( strcmp( "MBEDTLS_RSA_NO_CRT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_RSA_NO_CRT );
        return( 0 );
    }
#endif /* MBEDTLS_RSA_NO_CRT */

#if defined(MBEDTLS_SELF_TEST)
    if( strcmp( "MBEDTLS_SELF_TEST", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SELF_TEST );
        return( 0 );
    }
#endif /* MBEDTLS_SELF_TEST */

#if defined(MBEDTLS_SHA256_SMALLER)
    if( strcmp( "MBEDTLS_SHA256_SMALLER", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SHA256_SMALLER );
        return( 0 );
    }
#endif /* MBEDTLS_SHA256_SMALLER */

#if defined(MBEDTLS_SSL_ALL_ALERT_MESSAGES)
    if( strcmp( "MBEDTLS_SSL_ALL_ALERT_MESSAGES", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_ALL_ALERT_MESSAGES );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_ALL_ALERT_MESSAGES */

#if defined(MBEDTLS_SSL_ASYNC_PRIVATE)
    if( strcmp( "MBEDTLS_SSL_ASYNC_PRIVATE", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_ASYNC_PRIVATE );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_ASYNC_PRIVATE */

#if defined(MBEDTLS_SSL_DEBUG_ALL)
    if( strcmp( "MBEDTLS_SSL_DEBUG_ALL", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_DEBUG_ALL );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_DEBUG_ALL */

#if defined(MBEDTLS_SSL_ENCRYPT_THEN_MAC)
    if( strcmp( "MBEDTLS_SSL_ENCRYPT_THEN_MAC", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_ENCRYPT_THEN_MAC );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_ENCRYPT_THEN_MAC */

#if defined(MBEDTLS_SSL_EXTENDED_MASTER_SECRET)
    if( strcmp( "MBEDTLS_SSL_EXTENDED_MASTER_SECRET", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_EXTENDED_MASTER_SECRET );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_EXTENDED_MASTER_SECRET */

#if defined(MBEDTLS_SSL_FALLBACK_SCSV)
    if( strcmp( "MBEDTLS_SSL_FALLBACK_SCSV", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_FALLBACK_SCSV );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_FALLBACK_SCSV */

#if defined(MBEDTLS_SSL_HW_RECORD_ACCEL)
    if( strcmp( "MBEDTLS_SSL_HW_RECORD_ACCEL", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_HW_RECORD_ACCEL );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_HW_RECORD_ACCEL */

#if defined(MBEDTLS_SSL_CBC_RECORD_SPLITTING)
    if( strcmp( "MBEDTLS_SSL_CBC_RECORD_SPLITTING", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_CBC_RECORD_SPLITTING );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_CBC_RECORD_SPLITTING */

#if defined(MBEDTLS_SSL_RENEGOTIATION)
    if( strcmp( "MBEDTLS_SSL_RENEGOTIATION", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_RENEGOTIATION );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_RENEGOTIATION */

#if defined(MBEDTLS_SSL_SRV_SUPPORT_SSLV2_CLIENT_HELLO)
    if( strcmp( "MBEDTLS_SSL_SRV_SUPPORT_SSLV2_CLIENT_HELLO", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_SRV_SUPPORT_SSLV2_CLIENT_HELLO );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_SRV_SUPPORT_SSLV2_CLIENT_HELLO */

#if defined(MBEDTLS_SSL_SRV_RESPECT_CLIENT_PREFERENCE)
    if( strcmp( "MBEDTLS_SSL_SRV_RESPECT_CLIENT_PREFERENCE", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_SRV_RESPECT_CLIENT_PREFERENCE );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_SRV_RESPECT_CLIENT_PREFERENCE */

#if defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
    if( strcmp( "MBEDTLS_SSL_MAX_FRAGMENT_LENGTH", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_MAX_FRAGMENT_LENGTH );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_MAX_FRAGMENT_LENGTH */

#if defined(MBEDTLS_SSL_PROTO_SSL3)
    if( strcmp( "MBEDTLS_SSL_PROTO_SSL3", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_PROTO_SSL3 );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_PROTO_SSL3 */

#if defined(MBEDTLS_SSL_PROTO_TLS1)
    if( strcmp( "MBEDTLS_SSL_PROTO_TLS1", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_PROTO_TLS1 );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_PROTO_TLS1 */

#if defined(MBEDTLS_SSL_PROTO_TLS1_1)
    if( strcmp( "MBEDTLS_SSL_PROTO_TLS1_1", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_PROTO_TLS1_1 );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_PROTO_TLS1_1 */

#if defined(MBEDTLS_SSL_PROTO_TLS1_2)
    if( strcmp( "MBEDTLS_SSL_PROTO_TLS1_2", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_PROTO_TLS1_2 );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_PROTO_TLS1_2 */

#if defined(MBEDTLS_SSL_PROTO_DTLS)
    if( strcmp( "MBEDTLS_SSL_PROTO_DTLS", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_PROTO_DTLS );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_PROTO_DTLS */

#if defined(MBEDTLS_SSL_ALPN)
    if( strcmp( "MBEDTLS_SSL_ALPN", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_ALPN );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_ALPN */

#if defined(MBEDTLS_SSL_DTLS_ANTI_REPLAY)
    if( strcmp( "MBEDTLS_SSL_DTLS_ANTI_REPLAY", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_DTLS_ANTI_REPLAY );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_DTLS_ANTI_REPLAY */

#if defined(MBEDTLS_SSL_DTLS_HELLO_VERIFY)
    if( strcmp( "MBEDTLS_SSL_DTLS_HELLO_VERIFY", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_DTLS_HELLO_VERIFY );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_DTLS_HELLO_VERIFY */

#if defined(MBEDTLS_SSL_DTLS_CLIENT_PORT_REUSE)
    if( strcmp( "MBEDTLS_SSL_DTLS_CLIENT_PORT_REUSE", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_DTLS_CLIENT_PORT_REUSE );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_DTLS_CLIENT_PORT_REUSE */

#if defined(MBEDTLS_SSL_DTLS_BADMAC_LIMIT)
    if( strcmp( "MBEDTLS_SSL_DTLS_BADMAC_LIMIT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_DTLS_BADMAC_LIMIT );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_DTLS_BADMAC_LIMIT */

#if defined(MBEDTLS_SSL_SESSION_TICKETS)
    if( strcmp( "MBEDTLS_SSL_SESSION_TICKETS", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_SESSION_TICKETS );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_SESSION_TICKETS */

#if defined(MBEDTLS_SSL_EXPORT_KEYS)
    if( strcmp( "MBEDTLS_SSL_EXPORT_KEYS", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_EXPORT_KEYS );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_EXPORT_KEYS */

#if defined(MBEDTLS_SSL_SERVER_NAME_INDICATION)
    if( strcmp( "MBEDTLS_SSL_SERVER_NAME_INDICATION", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_SERVER_NAME_INDICATION );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_SERVER_NAME_INDICATION */

#if defined(MBEDTLS_SSL_TRUNCATED_HMAC)
    if( strcmp( "MBEDTLS_SSL_TRUNCATED_HMAC", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_TRUNCATED_HMAC );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_TRUNCATED_HMAC */

#if defined(MBEDTLS_SSL_TRUNCATED_HMAC_COMPAT)
    if( strcmp( "MBEDTLS_SSL_TRUNCATED_HMAC_COMPAT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_TRUNCATED_HMAC_COMPAT );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_TRUNCATED_HMAC_COMPAT */

#if defined(MBEDTLS_THREADING_ALT)
    if( strcmp( "MBEDTLS_THREADING_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_THREADING_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_THREADING_ALT */

#if defined(MBEDTLS_THREADING_PTHREAD)
    if( strcmp( "MBEDTLS_THREADING_PTHREAD", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_THREADING_PTHREAD );
        return( 0 );
    }
#endif /* MBEDTLS_THREADING_PTHREAD */

#if defined(MBEDTLS_VERSION_FEATURES)
    if( strcmp( "MBEDTLS_VERSION_FEATURES", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_VERSION_FEATURES );
        return( 0 );
    }
#endif /* MBEDTLS_VERSION_FEATURES */

#if defined(MBEDTLS_X509_ALLOW_EXTENSIONS_NON_V3)
    if( strcmp( "MBEDTLS_X509_ALLOW_EXTENSIONS_NON_V3", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_X509_ALLOW_EXTENSIONS_NON_V3 );
        return( 0 );
    }
#endif /* MBEDTLS_X509_ALLOW_EXTENSIONS_NON_V3 */

#if defined(MBEDTLS_X509_ALLOW_UNSUPPORTED_CRITICAL_EXTENSION)
    if( strcmp( "MBEDTLS_X509_ALLOW_UNSUPPORTED_CRITICAL_EXTENSION", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_X509_ALLOW_UNSUPPORTED_CRITICAL_EXTENSION );
        return( 0 );
    }
#endif /* MBEDTLS_X509_ALLOW_UNSUPPORTED_CRITICAL_EXTENSION */

#if defined(MBEDTLS_X509_CHECK_KEY_USAGE)
    if( strcmp( "MBEDTLS_X509_CHECK_KEY_USAGE", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_X509_CHECK_KEY_USAGE );
        return( 0 );
    }
#endif /* MBEDTLS_X509_CHECK_KEY_USAGE */

#if defined(MBEDTLS_X509_CHECK_EXTENDED_KEY_USAGE)
    if( strcmp( "MBEDTLS_X509_CHECK_EXTENDED_KEY_USAGE", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_X509_CHECK_EXTENDED_KEY_USAGE );
        return( 0 );
    }
#endif /* MBEDTLS_X509_CHECK_EXTENDED_KEY_USAGE */

#if defined(MBEDTLS_X509_RSASSA_PSS_SUPPORT)
    if( strcmp( "MBEDTLS_X509_RSASSA_PSS_SUPPORT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_X509_RSASSA_PSS_SUPPORT );
        return( 0 );
    }
#endif /* MBEDTLS_X509_RSASSA_PSS_SUPPORT */

#if defined(MBEDTLS_ZLIB_SUPPORT)
    if( strcmp( "MBEDTLS_ZLIB_SUPPORT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ZLIB_SUPPORT );
        return( 0 );
    }
#endif /* MBEDTLS_ZLIB_SUPPORT */

#if defined(MBEDTLS_AESNI_C)
    if( strcmp( "MBEDTLS_AESNI_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_AESNI_C );
        return( 0 );
    }
#endif /* MBEDTLS_AESNI_C */

#if defined(MBEDTLS_AES_C)
    if( strcmp( "MBEDTLS_AES_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_AES_C );
        return( 0 );
    }
#endif /* MBEDTLS_AES_C */

#if defined(MBEDTLS_ARC4_C)
    if( strcmp( "MBEDTLS_ARC4_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ARC4_C );
        return( 0 );
    }
#endif /* MBEDTLS_ARC4_C */

#if defined(MBEDTLS_ASN1_PARSE_C)
    if( strcmp( "MBEDTLS_ASN1_PARSE_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ASN1_PARSE_C );
        return( 0 );
    }
#endif /* MBEDTLS_ASN1_PARSE_C */

#if defined(MBEDTLS_ASN1_WRITE_C)
    if( strcmp( "MBEDTLS_ASN1_WRITE_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ASN1_WRITE_C );
        return( 0 );
    }
#endif /* MBEDTLS_ASN1_WRITE_C */

#if defined(MBEDTLS_BASE64_C)
    if( strcmp( "MBEDTLS_BASE64_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_BASE64_C );
        return( 0 );
    }
#endif /* MBEDTLS_BASE64_C */

#if defined(MBEDTLS_BIGNUM_C)
    if( strcmp( "MBEDTLS_BIGNUM_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_BIGNUM_C );
        return( 0 );
    }
#endif /* MBEDTLS_BIGNUM_C */

#if defined(MBEDTLS_BLOWFISH_C)
    if( strcmp( "MBEDTLS_BLOWFISH_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_BLOWFISH_C );
        return( 0 );
    }
#endif /* MBEDTLS_BLOWFISH_C */

#if defined(MBEDTLS_CAMELLIA_C)
    if( strcmp( "MBEDTLS_CAMELLIA_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CAMELLIA_C );
        return( 0 );
    }
#endif /* MBEDTLS_CAMELLIA_C */

#if defined(MBEDTLS_ARIA_C)
    if( strcmp( "MBEDTLS_ARIA_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ARIA_C );
        return( 0 );
    }
#endif /* MBEDTLS_ARIA_C */

#if defined(MBEDTLS_CCM_C)
    if( strcmp( "MBEDTLS_CCM_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CCM_C );
        return( 0 );
    }
#endif /* MBEDTLS_CCM_C */

#if defined(MBEDTLS_CERTS_C)
    if( strcmp( "MBEDTLS_CERTS_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CERTS_C );
        return( 0 );
    }
#endif /* MBEDTLS_CERTS_C */

#if defined(MBEDTLS_CHACHA20_C)
    if( strcmp( "MBEDTLS_CHACHA20_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CHACHA20_C );
        return( 0 );
    }
#endif /* MBEDTLS_CHACHA20_C */

#if defined(MBEDTLS_CHACHAPOLY_C)
    if( strcmp( "MBEDTLS_CHACHAPOLY_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CHACHAPOLY_C );
        return( 0 );
    }
#endif /* MBEDTLS_CHACHAPOLY_C */

#if defined(MBEDTLS_CIPHER_C)
    if( strcmp( "MBEDTLS_CIPHER_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CIPHER_C );
        return( 0 );
    }
#endif /* MBEDTLS_CIPHER_C */

#if defined(MBEDTLS_CMAC_C)
    if( strcmp( "MBEDTLS_CMAC_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CMAC_C );
        return( 0 );
    }
#endif /* MBEDTLS_CMAC_C */

#if defined(MBEDTLS_CTR_DRBG_C)
    if( strcmp( "MBEDTLS_CTR_DRBG_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CTR_DRBG_C );
        return( 0 );
    }
#endif /* MBEDTLS_CTR_DRBG_C */

#if defined(MBEDTLS_DEBUG_C)
    if( strcmp( "MBEDTLS_DEBUG_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_DEBUG_C );
        return( 0 );
    }
#endif /* MBEDTLS_DEBUG_C */

#if defined(MBEDTLS_DES_C)
    if( strcmp( "MBEDTLS_DES_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_DES_C );
        return( 0 );
    }
#endif /* MBEDTLS_DES_C */

#if defined(MBEDTLS_DHM_C)
    if( strcmp( "MBEDTLS_DHM_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_DHM_C );
        return( 0 );
    }
#endif /* MBEDTLS_DHM_C */

#if defined(MBEDTLS_ECDH_C)
    if( strcmp( "MBEDTLS_ECDH_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECDH_C );
        return( 0 );
    }
#endif /* MBEDTLS_ECDH_C */

#if defined(MBEDTLS_ECDSA_C)
    if( strcmp( "MBEDTLS_ECDSA_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECDSA_C );
        return( 0 );
    }
#endif /* MBEDTLS_ECDSA_C */

#if defined(MBEDTLS_ECJPAKE_C)
    if( strcmp( "MBEDTLS_ECJPAKE_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECJPAKE_C );
        return( 0 );
    }
#endif /* MBEDTLS_ECJPAKE_C */

#if defined(MBEDTLS_ECP_C)
    if( strcmp( "MBEDTLS_ECP_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_C );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_C */

#if defined(MBEDTLS_ENTROPY_C)
    if( strcmp( "MBEDTLS_ENTROPY_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ENTROPY_C );
        return( 0 );
    }
#endif /* MBEDTLS_ENTROPY_C */

#if defined(MBEDTLS_ERROR_C)
    if( strcmp( "MBEDTLS_ERROR_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ERROR_C );
        return( 0 );
    }
#endif /* MBEDTLS_ERROR_C */

#if defined(MBEDTLS_GCM_C)
    if( strcmp( "MBEDTLS_GCM_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_GCM_C );
        return( 0 );
    }
#endif /* MBEDTLS_GCM_C */

#if defined(MBEDTLS_HAVEGE_C)
    if( strcmp( "MBEDTLS_HAVEGE_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_HAVEGE_C );
        return( 0 );
    }
#endif /* MBEDTLS_HAVEGE_C */

#if defined(MBEDTLS_HKDF_C)
    if( strcmp( "MBEDTLS_HKDF_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_HKDF_C );
        return( 0 );
    }
#endif /* MBEDTLS_HKDF_C */

#if defined(MBEDTLS_HMAC_DRBG_C)
    if( strcmp( "MBEDTLS_HMAC_DRBG_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_HMAC_DRBG_C );
        return( 0 );
    }
#endif /* MBEDTLS_HMAC_DRBG_C */

#if defined(MBEDTLS_NIST_KW_C)
    if( strcmp( "MBEDTLS_NIST_KW_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_NIST_KW_C );
        return( 0 );
    }
#endif /* MBEDTLS_NIST_KW_C */

#if defined(MBEDTLS_MD_C)
    if( strcmp( "MBEDTLS_MD_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_MD_C );
        return( 0 );
    }
#endif /* MBEDTLS_MD_C */

#if defined(MBEDTLS_MD2_C)
    if( strcmp( "MBEDTLS_MD2_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_MD2_C );
        return( 0 );
    }
#endif /* MBEDTLS_MD2_C */

#if defined(MBEDTLS_MD4_C)
    if( strcmp( "MBEDTLS_MD4_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_MD4_C );
        return( 0 );
    }
#endif /* MBEDTLS_MD4_C */

#if defined(MBEDTLS_MD5_C)
    if( strcmp( "MBEDTLS_MD5_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_MD5_C );
        return( 0 );
    }
#endif /* MBEDTLS_MD5_C */

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
    if( strcmp( "MBEDTLS_MEMORY_BUFFER_ALLOC_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_MEMORY_BUFFER_ALLOC_C );
        return( 0 );
    }
#endif /* MBEDTLS_MEMORY_BUFFER_ALLOC_C */

#if defined(MBEDTLS_NET_C)
    if( strcmp( "MBEDTLS_NET_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_NET_C );
        return( 0 );
    }
#endif /* MBEDTLS_NET_C */

#if defined(MBEDTLS_OID_C)
    if( strcmp( "MBEDTLS_OID_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_OID_C );
        return( 0 );
    }
#endif /* MBEDTLS_OID_C */

#if defined(MBEDTLS_PADLOCK_C)
    if( strcmp( "MBEDTLS_PADLOCK_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PADLOCK_C );
        return( 0 );
    }
#endif /* MBEDTLS_PADLOCK_C */

#if defined(MBEDTLS_PEM_PARSE_C)
    if( strcmp( "MBEDTLS_PEM_PARSE_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PEM_PARSE_C );
        return( 0 );
    }
#endif /* MBEDTLS_PEM_PARSE_C */

#if defined(MBEDTLS_PEM_WRITE_C)
    if( strcmp( "MBEDTLS_PEM_WRITE_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PEM_WRITE_C );
        return( 0 );
    }
#endif /* MBEDTLS_PEM_WRITE_C */

#if defined(MBEDTLS_PK_C)
    if( strcmp( "MBEDTLS_PK_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PK_C );
        return( 0 );
    }
#endif /* MBEDTLS_PK_C */

#if defined(MBEDTLS_PK_PARSE_C)
    if( strcmp( "MBEDTLS_PK_PARSE_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PK_PARSE_C );
        return( 0 );
    }
#endif /* MBEDTLS_PK_PARSE_C */

#if defined(MBEDTLS_PK_WRITE_C)
    if( strcmp( "MBEDTLS_PK_WRITE_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PK_WRITE_C );
        return( 0 );
    }
#endif /* MBEDTLS_PK_WRITE_C */

#if defined(MBEDTLS_PKCS5_C)
    if( strcmp( "MBEDTLS_PKCS5_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PKCS5_C );
        return( 0 );
    }
#endif /* MBEDTLS_PKCS5_C */

#if defined(MBEDTLS_PKCS11_C)
    if( strcmp( "MBEDTLS_PKCS11_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PKCS11_C );
        return( 0 );
    }
#endif /* MBEDTLS_PKCS11_C */

#if defined(MBEDTLS_PKCS12_C)
    if( strcmp( "MBEDTLS_PKCS12_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PKCS12_C );
        return( 0 );
    }
#endif /* MBEDTLS_PKCS12_C */

#if defined(MBEDTLS_PLATFORM_C)
    if( strcmp( "MBEDTLS_PLATFORM_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_C );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_C */

#if defined(MBEDTLS_POLY1305_C)
    if( strcmp( "MBEDTLS_POLY1305_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_POLY1305_C );
        return( 0 );
    }
#endif /* MBEDTLS_POLY1305_C */

#if defined(MBEDTLS_RIPEMD160_C)
    if( strcmp( "MBEDTLS_RIPEMD160_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_RIPEMD160_C );
        return( 0 );
    }
#endif /* MBEDTLS_RIPEMD160_C */

#if defined(MBEDTLS_RSA_C)
    if( strcmp( "MBEDTLS_RSA_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_RSA_C );
        return( 0 );
    }
#endif /* MBEDTLS_RSA_C */

#if defined(MBEDTLS_SHA1_C)
    if( strcmp( "MBEDTLS_SHA1_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SHA1_C );
        return( 0 );
    }
#endif /* MBEDTLS_SHA1_C */

#if defined(MBEDTLS_SHA256_C)
    if( strcmp( "MBEDTLS_SHA256_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SHA256_C );
        return( 0 );
    }
#endif /* MBEDTLS_SHA256_C */

#if defined(MBEDTLS_SHA512_C)
    if( strcmp( "MBEDTLS_SHA512_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SHA512_C );
        return( 0 );
    }
#endif /* MBEDTLS_SHA512_C */

#if defined(MBEDTLS_SSL_CACHE_C)
    if( strcmp( "MBEDTLS_SSL_CACHE_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_CACHE_C );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_CACHE_C */

#if defined(MBEDTLS_SSL_COOKIE_C)
    if( strcmp( "MBEDTLS_SSL_COOKIE_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_COOKIE_C );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_COOKIE_C */

#if defined(MBEDTLS_SSL_TICKET_C)
    if( strcmp( "MBEDTLS_SSL_TICKET_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_TICKET_C );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_TICKET_C */

#if defined(MBEDTLS_SSL_CLI_C)
    if( strcmp( "MBEDTLS_SSL_CLI_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_CLI_C );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_CLI_C */

#if defined(MBEDTLS_SSL_SRV_C)
    if( strcmp( "MBEDTLS_SSL_SRV_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_SRV_C );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_SRV_C */

#if defined(MBEDTLS_SSL_TLS_C)
    if( strcmp( "MBEDTLS_SSL_TLS_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_TLS_C );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_TLS_C */

#if defined(MBEDTLS_THREADING_C)
    if( strcmp( "MBEDTLS_THREADING_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_THREADING_C );
        return( 0 );
    }
#endif /* MBEDTLS_THREADING_C */

#if defined(MBEDTLS_TIMING_C)
    if( strcmp( "MBEDTLS_TIMING_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_TIMING_C );
        return( 0 );
    }
#endif /* MBEDTLS_TIMING_C */

#if defined(MBEDTLS_VERSION_C)
    if( strcmp( "MBEDTLS_VERSION_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_VERSION_C );
        return( 0 );
    }
#endif /* MBEDTLS_VERSION_C */

#if defined(MBEDTLS_X509_USE_C)
    if( strcmp( "MBEDTLS_X509_USE_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_X509_USE_C );
        return( 0 );
    }
#endif /* MBEDTLS_X509_USE_C */

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    if( strcmp( "MBEDTLS_X509_CRT_PARSE_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_X509_CRT_PARSE_C );
        return( 0 );
    }
#endif /* MBEDTLS_X509_CRT_PARSE_C */

#if defined(MBEDTLS_X509_CRL_PARSE_C)
    if( strcmp( "MBEDTLS_X509_CRL_PARSE_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_X509_CRL_PARSE_C );
        return( 0 );
    }
#endif /* MBEDTLS_X509_CRL_PARSE_C */

#if defined(MBEDTLS_X509_CSR_PARSE_C)
    if( strcmp( "MBEDTLS_X509_CSR_PARSE_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_X509_CSR_PARSE_C );
        return( 0 );
    }
#endif /* MBEDTLS_X509_CSR_PARSE_C */

#if defined(MBEDTLS_X509_CREATE_C)
    if( strcmp( "MBEDTLS_X509_CREATE_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_X509_CREATE_C );
        return( 0 );
    }
#endif /* MBEDTLS_X509_CREATE_C */

#if defined(MBEDTLS_X509_CRT_WRITE_C)
    if( strcmp( "MBEDTLS_X509_CRT_WRITE_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_X509_CRT_WRITE_C );
        return( 0 );
    }
#endif /* MBEDTLS_X509_CRT_WRITE_C */

#if defined(MBEDTLS_X509_CSR_WRITE_C)
    if( strcmp( "MBEDTLS_X509_CSR_WRITE_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_X509_CSR_WRITE_C );
        return( 0 );
    }
#endif /* MBEDTLS_X509_CSR_WRITE_C */

#if defined(MBEDTLS_XTEA_C)
    if( strcmp( "MBEDTLS_XTEA_C", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_XTEA_C );
        return( 0 );
    }
#endif /* MBEDTLS_XTEA_C */

#if defined(MBEDTLS_MPI_WINDOW_SIZE)
    if( strcmp( "MBEDTLS_MPI_WINDOW_SIZE", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_MPI_WINDOW_SIZE );
        return( 0 );
    }
#endif /* MBEDTLS_MPI_WINDOW_SIZE */

#if defined(MBEDTLS_MPI_MAX_SIZE)
    if( strcmp( "MBEDTLS_MPI_MAX_SIZE", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_MPI_MAX_SIZE );
        return( 0 );
    }
#endif /* MBEDTLS_MPI_MAX_SIZE */

#if defined(MBEDTLS_CTR_DRBG_ENTROPY_LEN)
    if( strcmp( "MBEDTLS_CTR_DRBG_ENTROPY_LEN", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CTR_DRBG_ENTROPY_LEN );
        return( 0 );
    }
#endif /* MBEDTLS_CTR_DRBG_ENTROPY_LEN */

#if defined(MBEDTLS_CTR_DRBG_RESEED_INTERVAL)
    if( strcmp( "MBEDTLS_CTR_DRBG_RESEED_INTERVAL", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CTR_DRBG_RESEED_INTERVAL );
        return( 0 );
    }
#endif /* MBEDTLS_CTR_DRBG_RESEED_INTERVAL */

#if defined(MBEDTLS_CTR_DRBG_MAX_INPUT)
    if( strcmp( "MBEDTLS_CTR_DRBG_MAX_INPUT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CTR_DRBG_MAX_INPUT );
        return( 0 );
    }
#endif /* MBEDTLS_CTR_DRBG_MAX_INPUT */

#if defined(MBEDTLS_CTR_DRBG_MAX_REQUEST)
    if( strcmp( "MBEDTLS_CTR_DRBG_MAX_REQUEST", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CTR_DRBG_MAX_REQUEST );
        return( 0 );
    }
#endif /* MBEDTLS_CTR_DRBG_MAX_REQUEST */

#if defined(MBEDTLS_CTR_DRBG_MAX_SEED_INPUT)
    if( strcmp( "MBEDTLS_CTR_DRBG_MAX_SEED_INPUT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CTR_DRBG_MAX_SEED_INPUT );
        return( 0 );
    }
#endif /* MBEDTLS_CTR_DRBG_MAX_SEED_INPUT */

#if defined(MBEDTLS_CTR_DRBG_USE_128_BIT_KEY)
    if( strcmp( "MBEDTLS_CTR_DRBG_USE_128_BIT_KEY", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_CTR_DRBG_USE_128_BIT_KEY );
        return( 0 );
    }
#endif /* MBEDTLS_CTR_DRBG_USE_128_BIT_KEY */

#if defined(MBEDTLS_HMAC_DRBG_RESEED_INTERVAL)
    if( strcmp( "MBEDTLS_HMAC_DRBG_RESEED_INTERVAL", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_HMAC_DRBG_RESEED_INTERVAL );
        return( 0 );
    }
#endif /* MBEDTLS_HMAC_DRBG_RESEED_INTERVAL */

#if defined(MBEDTLS_HMAC_DRBG_MAX_INPUT)
    if( strcmp( "MBEDTLS_HMAC_DRBG_MAX_INPUT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_HMAC_DRBG_MAX_INPUT );
        return( 0 );
    }
#endif /* MBEDTLS_HMAC_DRBG_MAX_INPUT */

#if defined(MBEDTLS_HMAC_DRBG_MAX_REQUEST)
    if( strcmp( "MBEDTLS_HMAC_DRBG_MAX_REQUEST", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_HMAC_DRBG_MAX_REQUEST );
        return( 0 );
    }
#endif /* MBEDTLS_HMAC_DRBG_MAX_REQUEST */

#if defined(MBEDTLS_HMAC_DRBG_MAX_SEED_INPUT)
    if( strcmp( "MBEDTLS_HMAC_DRBG_MAX_SEED_INPUT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_HMAC_DRBG_MAX_SEED_INPUT );
        return( 0 );
    }
#endif /* MBEDTLS_HMAC_DRBG_MAX_SEED_INPUT */

#if defined(MBEDTLS_ECP_MAX_BITS)
    if( strcmp( "MBEDTLS_ECP_MAX_BITS", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_MAX_BITS );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_MAX_BITS */

#if defined(MBEDTLS_ECP_WINDOW_SIZE)
    if( strcmp( "MBEDTLS_ECP_WINDOW_SIZE", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_WINDOW_SIZE );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_WINDOW_SIZE */

#if defined(MBEDTLS_ECP_FIXED_POINT_OPTIM)
    if( strcmp( "MBEDTLS_ECP_FIXED_POINT_OPTIM", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ECP_FIXED_POINT_OPTIM );
        return( 0 );
    }
#endif /* MBEDTLS_ECP_FIXED_POINT_OPTIM */

#if defined(MBEDTLS_ENTROPY_MAX_SOURCES)
    if( strcmp( "MBEDTLS_ENTROPY_MAX_SOURCES", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ENTROPY_MAX_SOURCES );
        return( 0 );
    }
#endif /* MBEDTLS_ENTROPY_MAX_SOURCES */

#if defined(MBEDTLS_ENTROPY_MAX_GATHER)
    if( strcmp( "MBEDTLS_ENTROPY_MAX_GATHER", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ENTROPY_MAX_GATHER );
        return( 0 );
    }
#endif /* MBEDTLS_ENTROPY_MAX_GATHER */

#if defined(MBEDTLS_ENTROPY_MIN_HARDWARE)
    if( strcmp( "MBEDTLS_ENTROPY_MIN_HARDWARE", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_ENTROPY_MIN_HARDWARE );
        return( 0 );
    }
#endif /* MBEDTLS_ENTROPY_MIN_HARDWARE */

#if defined(MBEDTLS_MEMORY_ALIGN_MULTIPLE)
    if( strcmp( "MBEDTLS_MEMORY_ALIGN_MULTIPLE", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_MEMORY_ALIGN_MULTIPLE );
        return( 0 );
    }
#endif /* MBEDTLS_MEMORY_ALIGN_MULTIPLE */

#if defined(MBEDTLS_PLATFORM_STD_MEM_HDR)
    if( strcmp( "MBEDTLS_PLATFORM_STD_MEM_HDR", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_STD_MEM_HDR );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_STD_MEM_HDR */

#if defined(MBEDTLS_PLATFORM_STD_CALLOC)
    if( strcmp( "MBEDTLS_PLATFORM_STD_CALLOC", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_STD_CALLOC );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_STD_CALLOC */

#if defined(MBEDTLS_PLATFORM_STD_FREE)
    if( strcmp( "MBEDTLS_PLATFORM_STD_FREE", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_STD_FREE );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_STD_FREE */

#if defined(MBEDTLS_PLATFORM_STD_EXIT)
    if( strcmp( "MBEDTLS_PLATFORM_STD_EXIT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_STD_EXIT );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_STD_EXIT */

#if defined(MBEDTLS_PLATFORM_STD_TIME)
    if( strcmp( "MBEDTLS_PLATFORM_STD_TIME", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_STD_TIME );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_STD_TIME */

#if defined(MBEDTLS_PLATFORM_STD_FPRINTF)
    if( strcmp( "MBEDTLS_PLATFORM_STD_FPRINTF", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_STD_FPRINTF );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_STD_FPRINTF */

#if defined(MBEDTLS_PLATFORM_STD_PRINTF)
    if( strcmp( "MBEDTLS_PLATFORM_STD_PRINTF", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_STD_PRINTF );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_STD_PRINTF */

#if defined(MBEDTLS_PLATFORM_STD_SNPRINTF)
    if( strcmp( "MBEDTLS_PLATFORM_STD_SNPRINTF", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_STD_SNPRINTF );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_STD_SNPRINTF */

#if defined(MBEDTLS_PLATFORM_STD_EXIT_SUCCESS)
    if( strcmp( "MBEDTLS_PLATFORM_STD_EXIT_SUCCESS", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_STD_EXIT_SUCCESS );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_STD_EXIT_SUCCESS */

#if defined(MBEDTLS_PLATFORM_STD_EXIT_FAILURE)
    if( strcmp( "MBEDTLS_PLATFORM_STD_EXIT_FAILURE", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_STD_EXIT_FAILURE );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_STD_EXIT_FAILURE */

#if defined(MBEDTLS_PLATFORM_STD_NV_SEED_READ)
    if( strcmp( "MBEDTLS_PLATFORM_STD_NV_SEED_READ", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_STD_NV_SEED_READ );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_STD_NV_SEED_READ */

#if defined(MBEDTLS_PLATFORM_STD_NV_SEED_WRITE)
    if( strcmp( "MBEDTLS_PLATFORM_STD_NV_SEED_WRITE", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_STD_NV_SEED_WRITE );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_STD_NV_SEED_WRITE */

#if defined(MBEDTLS_PLATFORM_STD_NV_SEED_FILE)
    if( strcmp( "MBEDTLS_PLATFORM_STD_NV_SEED_FILE", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_STD_NV_SEED_FILE );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_STD_NV_SEED_FILE */

#if defined(MBEDTLS_PLATFORM_CALLOC_MACRO)
    if( strcmp( "MBEDTLS_PLATFORM_CALLOC_MACRO", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_CALLOC_MACRO );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_CALLOC_MACRO */

#if defined(MBEDTLS_PLATFORM_FREE_MACRO)
    if( strcmp( "MBEDTLS_PLATFORM_FREE_MACRO", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_FREE_MACRO );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_FREE_MACRO */

#if defined(MBEDTLS_PLATFORM_EXIT_MACRO)
    if( strcmp( "MBEDTLS_PLATFORM_EXIT_MACRO", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_EXIT_MACRO );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_EXIT_MACRO */

#if defined(MBEDTLS_PLATFORM_TIME_MACRO)
    if( strcmp( "MBEDTLS_PLATFORM_TIME_MACRO", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_TIME_MACRO );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_TIME_MACRO */

#if defined(MBEDTLS_PLATFORM_TIME_TYPE_MACRO)
    if( strcmp( "MBEDTLS_PLATFORM_TIME_TYPE_MACRO", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_TIME_TYPE_MACRO );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_TIME_TYPE_MACRO */

#if defined(MBEDTLS_PLATFORM_FPRINTF_MACRO)
    if( strcmp( "MBEDTLS_PLATFORM_FPRINTF_MACRO", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_FPRINTF_MACRO );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_FPRINTF_MACRO */

#if defined(MBEDTLS_PLATFORM_PRINTF_MACRO)
    if( strcmp( "MBEDTLS_PLATFORM_PRINTF_MACRO", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_PRINTF_MACRO );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_PRINTF_MACRO */

#if defined(MBEDTLS_PLATFORM_SNPRINTF_MACRO)
    if( strcmp( "MBEDTLS_PLATFORM_SNPRINTF_MACRO", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_SNPRINTF_MACRO );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_SNPRINTF_MACRO */

#if defined(MBEDTLS_PLATFORM_NV_SEED_READ_MACRO)
    if( strcmp( "MBEDTLS_PLATFORM_NV_SEED_READ_MACRO", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_NV_SEED_READ_MACRO );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_NV_SEED_READ_MACRO */

#if defined(MBEDTLS_PLATFORM_NV_SEED_WRITE_MACRO)
    if( strcmp( "MBEDTLS_PLATFORM_NV_SEED_WRITE_MACRO", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_NV_SEED_WRITE_MACRO );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_NV_SEED_WRITE_MACRO */

#if defined(MBEDTLS_SSL_CACHE_DEFAULT_TIMEOUT)
    if( strcmp( "MBEDTLS_SSL_CACHE_DEFAULT_TIMEOUT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_CACHE_DEFAULT_TIMEOUT );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_CACHE_DEFAULT_TIMEOUT */

#if defined(MBEDTLS_SSL_CACHE_DEFAULT_MAX_ENTRIES)
    if( strcmp( "MBEDTLS_SSL_CACHE_DEFAULT_MAX_ENTRIES", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_CACHE_DEFAULT_MAX_ENTRIES );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_CACHE_DEFAULT_MAX_ENTRIES */

#if defined(MBEDTLS_SSL_MAX_CONTENT_LEN)
    if( strcmp( "MBEDTLS_SSL_MAX_CONTENT_LEN", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_MAX_CONTENT_LEN );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_MAX_CONTENT_LEN */

#if defined(MBEDTLS_SSL_IN_CONTENT_LEN)
    if( strcmp( "MBEDTLS_SSL_IN_CONTENT_LEN", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_IN_CONTENT_LEN );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_IN_CONTENT_LEN */

#if defined(MBEDTLS_SSL_OUT_CONTENT_LEN)
    if( strcmp( "MBEDTLS_SSL_OUT_CONTENT_LEN", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_OUT_CONTENT_LEN );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_OUT_CONTENT_LEN */

#if defined(MBEDTLS_SSL_DTLS_MAX_BUFFERING)
    if( strcmp( "MBEDTLS_SSL_DTLS_MAX_BUFFERING", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_DTLS_MAX_BUFFERING );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_DTLS_MAX_BUFFERING */

#if defined(MBEDTLS_SSL_DEFAULT_TICKET_LIFETIME)
    if( strcmp( "MBEDTLS_SSL_DEFAULT_TICKET_LIFETIME", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_DEFAULT_TICKET_LIFETIME );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_DEFAULT_TICKET_LIFETIME */

#if defined(MBEDTLS_PSK_MAX_LEN)
    if( strcmp( "MBEDTLS_PSK_MAX_LEN", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PSK_MAX_LEN );
        return( 0 );
    }
#endif /* MBEDTLS_PSK_MAX_LEN */

#if defined(MBEDTLS_SSL_COOKIE_TIMEOUT)
    if( strcmp( "MBEDTLS_SSL_COOKIE_TIMEOUT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_SSL_COOKIE_TIMEOUT );
        return( 0 );
    }
#endif /* MBEDTLS_SSL_COOKIE_TIMEOUT */

#if defined(MBEDTLS_X509_MAX_INTERMEDIATE_CA)
    if( strcmp( "MBEDTLS_X509_MAX_INTERMEDIATE_CA", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_X509_MAX_INTERMEDIATE_CA );
        return( 0 );
    }
#endif /* MBEDTLS_X509_MAX_INTERMEDIATE_CA */

#if defined(MBEDTLS_X509_MAX_FILE_PATH_LEN)
    if( strcmp( "MBEDTLS_X509_MAX_FILE_PATH_LEN", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_X509_MAX_FILE_PATH_LEN );
        return( 0 );
    }
#endif /* MBEDTLS_X509_MAX_FILE_PATH_LEN */

#if defined(MBEDTLS_TLS_DEFAULT_ALLOW_SHA1_IN_CERTIFICATES)
    if( strcmp( "MBEDTLS_TLS_DEFAULT_ALLOW_SHA1_IN_CERTIFICATES", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_TLS_DEFAULT_ALLOW_SHA1_IN_CERTIFICATES );
        return( 0 );
    }
#endif /* MBEDTLS_TLS_DEFAULT_ALLOW_SHA1_IN_CERTIFICATES */

#if defined(MBEDTLS_TLS_DEFAULT_ALLOW_SHA1_IN_KEY_EXCHANGE)
    if( strcmp( "MBEDTLS_TLS_DEFAULT_ALLOW_SHA1_IN_KEY_EXCHANGE", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_TLS_DEFAULT_ALLOW_SHA1_IN_KEY_EXCHANGE );
        return( 0 );
    }
#endif /* MBEDTLS_TLS_DEFAULT_ALLOW_SHA1_IN_KEY_EXCHANGE */

#if defined(MBEDTLS_PLATFORM_ZEROIZE_ALT)
    if( strcmp( "MBEDTLS_PLATFORM_ZEROIZE_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_ZEROIZE_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_ZEROIZE_ALT */

#if defined(MBEDTLS_PLATFORM_GMTIME_R_ALT)
    if( strcmp( "MBEDTLS_PLATFORM_GMTIME_R_ALT", config ) == 0 )
    {
        MACRO_EXPANSION_TO_STR( MBEDTLS_PLATFORM_GMTIME_R_ALT );
        return( 0 );
    }
#endif /* MBEDTLS_PLATFORM_GMTIME_R_ALT */

    /* If the symbol is not found, return an error */
    return( 1 );
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif /* _MSC_VER */

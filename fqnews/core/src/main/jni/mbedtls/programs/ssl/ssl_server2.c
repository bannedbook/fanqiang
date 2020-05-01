/*
 *  SSL client with options
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
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
 *  This file is part of mbed TLS (https://tls.mbed.org)
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
#include <stdlib.h>
#define mbedtls_free       free
#define mbedtls_time       time
#define mbedtls_time_t     time_t
#define mbedtls_calloc    calloc
#define mbedtls_fprintf    fprintf
#define mbedtls_printf     printf
#define mbedtls_exit            exit
#define MBEDTLS_EXIT_SUCCESS    EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE    EXIT_FAILURE
#endif

#if !defined(MBEDTLS_ENTROPY_C) || \
    !defined(MBEDTLS_SSL_TLS_C) || !defined(MBEDTLS_SSL_SRV_C) || \
    !defined(MBEDTLS_NET_C) || !defined(MBEDTLS_CTR_DRBG_C)
int main( void )
{
    mbedtls_printf("MBEDTLS_ENTROPY_C and/or "
           "MBEDTLS_SSL_TLS_C and/or MBEDTLS_SSL_SRV_C and/or "
           "MBEDTLS_NET_C and/or MBEDTLS_CTR_DRBG_C and/or not defined.\n");
    return( 0 );
}
#else

#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "mbedtls/timing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#if !defined(_MSC_VER)
#include <inttypes.h>
#endif

#if !defined(_WIN32)
#include <signal.h>
#endif

#if defined(MBEDTLS_SSL_CACHE_C)
#include "mbedtls/ssl_cache.h"
#endif

#if defined(MBEDTLS_SSL_TICKET_C)
#include "mbedtls/ssl_ticket.h"
#endif

#if defined(MBEDTLS_SSL_COOKIE_C)
#include "mbedtls/ssl_cookie.h"
#endif

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include "mbedtls/memory_buffer_alloc.h"
#endif

#if defined(MBEDTLS_SSL_SERVER_NAME_INDICATION) && defined(MBEDTLS_FS_IO)
#define SNI_OPTION
#endif

#if defined(_WIN32)
#include <windows.h>
#endif

/* Size of memory to be allocated for the heap, when using the library's memory
 * management and MBEDTLS_MEMORY_BUFFER_ALLOC_C is enabled. */
#define MEMORY_HEAP_SIZE        120000

#define DFL_SERVER_ADDR         NULL
#define DFL_SERVER_PORT         "4433"
#define DFL_RESPONSE_SIZE       -1
#define DFL_DEBUG_LEVEL         0
#define DFL_NBIO                0
#define DFL_EVENT               0
#define DFL_READ_TIMEOUT        0
#define DFL_CA_FILE             ""
#define DFL_CA_PATH             ""
#define DFL_CRT_FILE            ""
#define DFL_KEY_FILE            ""
#define DFL_CRT_FILE2           ""
#define DFL_KEY_FILE2           ""
#define DFL_ASYNC_OPERATIONS    "-"
#define DFL_ASYNC_PRIVATE_DELAY1 ( -1 )
#define DFL_ASYNC_PRIVATE_DELAY2 ( -1 )
#define DFL_ASYNC_PRIVATE_ERROR  ( 0 )
#define DFL_PSK                 ""
#define DFL_PSK_IDENTITY        "Client_identity"
#define DFL_ECJPAKE_PW          NULL
#define DFL_PSK_LIST            NULL
#define DFL_FORCE_CIPHER        0
#define DFL_VERSION_SUITES      NULL
#define DFL_RENEGOTIATION       MBEDTLS_SSL_RENEGOTIATION_DISABLED
#define DFL_ALLOW_LEGACY        -2
#define DFL_RENEGOTIATE         0
#define DFL_RENEGO_DELAY        -2
#define DFL_RENEGO_PERIOD       ( (uint64_t)-1 )
#define DFL_EXCHANGES           1
#define DFL_MIN_VERSION         -1
#define DFL_MAX_VERSION         -1
#define DFL_ARC4                -1
#define DFL_SHA1                -1
#define DFL_AUTH_MODE           -1
#define DFL_CERT_REQ_CA_LIST    MBEDTLS_SSL_CERT_REQ_CA_LIST_ENABLED
#define DFL_MFL_CODE            MBEDTLS_SSL_MAX_FRAG_LEN_NONE
#define DFL_TRUNC_HMAC          -1
#define DFL_TICKETS             MBEDTLS_SSL_SESSION_TICKETS_ENABLED
#define DFL_TICKET_TIMEOUT      86400
#define DFL_CACHE_MAX           -1
#define DFL_CACHE_TIMEOUT       -1
#define DFL_SNI                 NULL
#define DFL_ALPN_STRING         NULL
#define DFL_CURVES              NULL
#define DFL_DHM_FILE            NULL
#define DFL_TRANSPORT           MBEDTLS_SSL_TRANSPORT_STREAM
#define DFL_COOKIES             1
#define DFL_ANTI_REPLAY         -1
#define DFL_HS_TO_MIN           0
#define DFL_HS_TO_MAX           0
#define DFL_DTLS_MTU            -1
#define DFL_BADMAC_LIMIT        -1
#define DFL_DGRAM_PACKING        1
#define DFL_EXTENDED_MS         -1
#define DFL_ETM                 -1

#define LONG_RESPONSE "<p>01-blah-blah-blah-blah-blah-blah-blah-blah-blah\r\n" \
    "02-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah\r\n"  \
    "03-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah\r\n"  \
    "04-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah\r\n"  \
    "05-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah\r\n"  \
    "06-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah\r\n"  \
    "07-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah-blah</p>\r\n"

/* Uncomment LONG_RESPONSE at the end of HTTP_RESPONSE to test sending longer
 * packets (for fragmentation purposes) */
#define HTTP_RESPONSE \
    "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n" \
    "<h2>mbed TLS Test Server</h2>\r\n" \
    "<p>Successful connection using: %s</p>\r\n" // LONG_RESPONSE

/*
 * Size of the basic I/O buffer. Able to hold our default response.
 *
 * You will need to adapt the mbedtls_ssl_get_bytes_avail() test in ssl-opt.sh
 * if you change this value to something outside the range <= 100 or > 500
 */
#define DFL_IO_BUF_LEN      200

#if defined(MBEDTLS_X509_CRT_PARSE_C)
#if defined(MBEDTLS_FS_IO)
#define USAGE_IO \
    "    ca_file=%%s          The single file containing the top-level CA(s) you fully trust\n" \
    "                        default: \"\" (pre-loaded)\n" \
    "    ca_path=%%s          The path containing the top-level CA(s) you fully trust\n" \
    "                        default: \"\" (pre-loaded) (overrides ca_file)\n" \
    "    crt_file=%%s         Your own cert and chain (in bottom to top order, top may be omitted)\n" \
    "                        default: see note after key_file2\n" \
    "    key_file=%%s         default: see note after key_file2\n" \
    "    crt_file2=%%s        Your second cert and chain (in bottom to top order, top may be omitted)\n" \
    "                        default: see note after key_file2\n" \
    "    key_file2=%%s        default: see note below\n" \
    "                        note: if neither crt_file/key_file nor crt_file2/key_file2 are used,\n" \
    "                              preloaded certificate(s) and key(s) are used if available\n" \
    "    dhm_file=%%s        File containing Diffie-Hellman parameters\n" \
    "                       default: preloaded parameters\n"
#else
#define USAGE_IO \
    "\n"                                                    \
    "    No file operations available (MBEDTLS_FS_IO not defined)\n" \
    "\n"
#endif /* MBEDTLS_FS_IO */
#else
#define USAGE_IO ""
#endif /* MBEDTLS_X509_CRT_PARSE_C */

#if defined(MBEDTLS_SSL_ASYNC_PRIVATE)
#define USAGE_SSL_ASYNC \
    "    async_operations=%%c...   d=decrypt, s=sign (default: -=off)\n" \
    "    async_private_delay1=%%d  Asynchronous delay for key_file or preloaded key\n" \
    "    async_private_delay2=%%d  Asynchronous delay for key_file2 and sni\n" \
    "                              default: -1 (not asynchronous)\n" \
    "    async_private_error=%%d   Async callback error injection (default=0=none,\n" \
    "                              1=start, 2=cancel, 3=resume, negative=first time only)"
#else
#define USAGE_SSL_ASYNC ""
#endif /* MBEDTLS_SSL_ASYNC_PRIVATE */

#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
#define USAGE_PSK                                                       \
    "    psk=%%s              default: \"\" (in hex, without 0x)\n"     \
    "    psk_list=%%s         default: \"\"\n"                          \
    "                          A list of (PSK identity, PSK value) pairs.\n" \
    "                          The PSK values are in hex, without 0x.\n" \
    "                          id1,psk1[,id2,psk2[,...]]\n"             \
    "    psk_identity=%%s     default: \"Client_identity\"\n"
#else
#define USAGE_PSK ""
#endif /* MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED */

#if defined(MBEDTLS_SSL_SESSION_TICKETS)
#define USAGE_TICKETS                                       \
    "    tickets=%%d          default: 1 (enabled)\n"       \
    "    ticket_timeout=%%d   default: 86400 (one day)\n"
#else
#define USAGE_TICKETS ""
#endif /* MBEDTLS_SSL_SESSION_TICKETS */

#if defined(MBEDTLS_SSL_CACHE_C)
#define USAGE_CACHE                                             \
    "    cache_max=%%d        default: cache default (50)\n"    \
    "    cache_timeout=%%d    default: cache default (1d)\n"
#else
#define USAGE_CACHE ""
#endif /* MBEDTLS_SSL_CACHE_C */

#if defined(SNI_OPTION)
#define USAGE_SNI                                                           \
    "    sni=%%s              name1,cert1,key1,ca1,crl1,auth1[,...]\n"  \
    "                        default: disabled\n"
#else
#define USAGE_SNI ""
#endif /* SNI_OPTION */

#if defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
#define USAGE_MAX_FRAG_LEN                                      \
    "    max_frag_len=%%d     default: 16384 (tls default)\n"   \
    "                        options: 512, 1024, 2048, 4096\n"
#else
#define USAGE_MAX_FRAG_LEN ""
#endif /* MBEDTLS_SSL_MAX_FRAGMENT_LENGTH */

#if defined(MBEDTLS_SSL_TRUNCATED_HMAC)
#define USAGE_TRUNC_HMAC \
    "    trunc_hmac=%%d       default: library default\n"
#else
#define USAGE_TRUNC_HMAC ""
#endif

#if defined(MBEDTLS_SSL_ALPN)
#define USAGE_ALPN \
    "    alpn=%%s             default: \"\" (disabled)\n"   \
    "                        example: spdy/1,http/1.1\n"
#else
#define USAGE_ALPN ""
#endif /* MBEDTLS_SSL_ALPN */

#if defined(MBEDTLS_SSL_DTLS_HELLO_VERIFY)
#define USAGE_COOKIES \
    "    cookies=0/1/-1      default: 1 (enabled)\n"        \
    "                        0: disabled, -1: library default (broken)\n"
#else
#define USAGE_COOKIES ""
#endif

#if defined(MBEDTLS_SSL_DTLS_ANTI_REPLAY)
#define USAGE_ANTI_REPLAY \
    "    anti_replay=0/1     default: (library default: enabled)\n"
#else
#define USAGE_ANTI_REPLAY ""
#endif

#if defined(MBEDTLS_SSL_DTLS_BADMAC_LIMIT)
#define USAGE_BADMAC_LIMIT \
    "    badmac_limit=%%d     default: (library default: disabled)\n"
#else
#define USAGE_BADMAC_LIMIT ""
#endif

#if defined(MBEDTLS_SSL_PROTO_DTLS)
#define USAGE_DTLS \
    "    dtls=%%d             default: 0 (TLS)\n"                           \
    "    hs_timeout=%%d-%%d    default: (library default: 1000-60000)\n"    \
    "                        range of DTLS handshake timeouts in millisecs\n" \
    "    mtu=%%d              default: (library default: unlimited)\n"  \
    "    dgram_packing=%%d    default: 1 (allowed)\n"                   \
    "                        allow or forbid packing of multiple\n" \
    "                        records within a single datgram.\n"
#else
#define USAGE_DTLS ""
#endif

#if defined(MBEDTLS_SSL_EXTENDED_MASTER_SECRET)
#define USAGE_EMS \
    "    extended_ms=0/1     default: (library default: on)\n"
#else
#define USAGE_EMS ""
#endif

#if defined(MBEDTLS_SSL_ENCRYPT_THEN_MAC)
#define USAGE_ETM \
    "    etm=0/1             default: (library default: on)\n"
#else
#define USAGE_ETM ""
#endif

#if defined(MBEDTLS_SSL_RENEGOTIATION)
#define USAGE_RENEGO \
    "    renegotiation=%%d    default: 0 (disabled)\n"      \
    "    renegotiate=%%d      default: 0 (disabled)\n"      \
    "    renego_delay=%%d     default: -2 (library default)\n" \
    "    renego_period=%%d    default: (2^64 - 1 for TLS, 2^48 - 1 for DTLS)\n"
#else
#define USAGE_RENEGO ""
#endif

#if defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED)
#define USAGE_ECJPAKE \
    "    ecjpake_pw=%%s       default: none (disabled)\n"
#else
#define USAGE_ECJPAKE ""
#endif

#if defined(MBEDTLS_ECP_C)
#define USAGE_CURVES \
    "    curves=a,b,c,d      default: \"default\" (library default)\n"  \
    "                        example: \"secp521r1,brainpoolP512r1\"\n"  \
    "                        - use \"none\" for empty list\n"           \
    "                        - see mbedtls_ecp_curve_list()\n"          \
    "                          for acceptable curve names\n"
#else
#define USAGE_CURVES ""
#endif

#define USAGE \
    "\n usage: ssl_server2 param=<>...\n"                   \
    "\n acceptable parameters:\n"                           \
    "    server_addr=%%s      default: (all interfaces)\n"  \
    "    server_port=%%d      default: 4433\n"              \
    "    debug_level=%%d      default: 0 (disabled)\n"      \
    "    buffer_size=%%d      default: 200 \n" \
    "                         (minimum: 1, max: 16385)\n" \
    "    response_size=%%d    default: about 152 (basic response)\n" \
    "                          (minimum: 0, max: 16384)\n" \
    "                          increases buffer_size if bigger\n"\
    "    nbio=%%d             default: 0 (blocking I/O)\n"  \
    "                        options: 1 (non-blocking), 2 (added delays)\n" \
    "    event=%%d            default: 0 (loop)\n"                            \
    "                        options: 1 (level-triggered, implies nbio=1),\n" \
    "    read_timeout=%%d     default: 0 ms (no timeout)\n"    \
    "\n"                                                    \
    USAGE_DTLS                                              \
    USAGE_COOKIES                                           \
    USAGE_ANTI_REPLAY                                       \
    USAGE_BADMAC_LIMIT                                      \
    "\n"                                                    \
    "    auth_mode=%%s        default: (library default: none)\n"      \
    "                        options: none, optional, required\n" \
    "    cert_req_ca_list=%%d default: 1 (send ca list)\n"  \
    "                        options: 1 (send ca list), 0 (don't send)\n" \
    USAGE_IO                                                \
    USAGE_SSL_ASYNC                                         \
    USAGE_SNI                                               \
    "\n"                                                    \
    USAGE_PSK                                               \
    USAGE_ECJPAKE                                           \
    "\n"                                                    \
    "    allow_legacy=%%d     default: (library default: no)\n"      \
    USAGE_RENEGO                                            \
    "    exchanges=%%d        default: 1\n"                 \
    "\n"                                                    \
    USAGE_TICKETS                                           \
    USAGE_CACHE                                             \
    USAGE_MAX_FRAG_LEN                                      \
    USAGE_TRUNC_HMAC                                        \
    USAGE_ALPN                                              \
    USAGE_EMS                                               \
    USAGE_ETM                                               \
    USAGE_CURVES                                            \
    "\n"                                                    \
    "    arc4=%%d             default: (library default: 0)\n" \
    "    allow_sha1=%%d       default: 0\n"                             \
    "    min_version=%%s      default: (library default: tls1)\n"       \
    "    max_version=%%s      default: (library default: tls1_2)\n"     \
    "    force_version=%%s    default: \"\" (none)\n"       \
    "                        options: ssl3, tls1, tls1_1, tls1_2, dtls1, dtls1_2\n" \
    "\n"                                                                \
    "    version_suites=a,b,c,d      per-version ciphersuites\n"        \
    "                                in order from ssl3 to tls1_2\n"    \
    "                                default: all enabled\n"            \
    "    force_ciphersuite=<name>    default: all enabled\n"            \
    "    query_config=<name>         return 0 if the specified\n"       \
    "                                configuration macro is defined and 1\n"  \
    "                                otherwise. The expansion of the macro\n" \
    "                                is printed if it is defined\n"     \
    " acceptable ciphersuite names:\n"


#define ALPN_LIST_SIZE  10
#define CURVE_LIST_SIZE 20

#define PUT_UINT64_BE(out_be,in_le,i)                                   \
{                                                                       \
    (out_be)[(i) + 0] = (unsigned char)( ( (in_le) >> 56 ) & 0xFF );    \
    (out_be)[(i) + 1] = (unsigned char)( ( (in_le) >> 48 ) & 0xFF );    \
    (out_be)[(i) + 2] = (unsigned char)( ( (in_le) >> 40 ) & 0xFF );    \
    (out_be)[(i) + 3] = (unsigned char)( ( (in_le) >> 32 ) & 0xFF );    \
    (out_be)[(i) + 4] = (unsigned char)( ( (in_le) >> 24 ) & 0xFF );    \
    (out_be)[(i) + 5] = (unsigned char)( ( (in_le) >> 16 ) & 0xFF );    \
    (out_be)[(i) + 6] = (unsigned char)( ( (in_le) >> 8  ) & 0xFF );    \
    (out_be)[(i) + 7] = (unsigned char)( ( (in_le) >> 0  ) & 0xFF );    \
}

#if defined(MBEDTLS_CHECK_PARAMS)
#include "mbedtls/platform_util.h"
void mbedtls_param_failed( const char *failure_condition,
                           const char *file,
                           int line )
{
    mbedtls_printf( "%s:%i: Input param failed - %s\n",
                    file, line, failure_condition );
    mbedtls_exit( MBEDTLS_EXIT_FAILURE );
}
#endif

/*
 * global options
 */
struct options
{
    const char *server_addr;    /* address on which the ssl service runs    */
    const char *server_port;    /* port on which the ssl service runs       */
    int debug_level;            /* level of debugging                       */
    int nbio;                   /* should I/O be blocking?                  */
    int event;                  /* loop or event-driven IO? level or edge triggered? */
    uint32_t read_timeout;      /* timeout on mbedtls_ssl_read() in milliseconds    */
    int response_size;          /* pad response with header to requested size */
    uint16_t buffer_size;       /* IO buffer size */
    const char *ca_file;        /* the file with the CA certificate(s)      */
    const char *ca_path;        /* the path with the CA certificate(s) reside */
    const char *crt_file;       /* the file with the server certificate     */
    const char *key_file;       /* the file with the server key             */
    const char *crt_file2;      /* the file with the 2nd server certificate */
    const char *key_file2;      /* the file with the 2nd server key         */
    const char *async_operations; /* supported SSL asynchronous operations  */
    int async_private_delay1;   /* number of times f_async_resume needs to be called for key 1, or -1 for no async */
    int async_private_delay2;   /* number of times f_async_resume needs to be called for key 2, or -1 for no async */
    int async_private_error;    /* inject error in async private callback */
    const char *psk;            /* the pre-shared key                       */
    const char *psk_identity;   /* the pre-shared key identity              */
    char *psk_list;             /* list of PSK id/key pairs for callback    */
    const char *ecjpake_pw;     /* the EC J-PAKE password                   */
    int force_ciphersuite[2];   /* protocol/ciphersuite to use, or all      */
    const char *version_suites; /* per-version ciphersuites                 */
    int renegotiation;          /* enable / disable renegotiation           */
    int allow_legacy;           /* allow legacy renegotiation               */
    int renegotiate;            /* attempt renegotiation?                   */
    int renego_delay;           /* delay before enforcing renegotiation     */
    uint64_t renego_period;     /* period for automatic renegotiation       */
    int exchanges;              /* number of data exchanges                 */
    int min_version;            /* minimum protocol version accepted        */
    int max_version;            /* maximum protocol version accepted        */
    int arc4;                   /* flag for arc4 suites support             */
    int allow_sha1;             /* flag for SHA-1 support                   */
    int auth_mode;              /* verify mode for connection               */
    int cert_req_ca_list;       /* should we send the CA list?              */
    unsigned char mfl_code;     /* code for maximum fragment length         */
    int trunc_hmac;             /* accept truncated hmac?                   */
    int tickets;                /* enable / disable session tickets         */
    int ticket_timeout;         /* session ticket lifetime                  */
    int cache_max;              /* max number of session cache entries      */
    int cache_timeout;          /* expiration delay of session cache entries */
    char *sni;                  /* string describing sni information        */
    const char *curves;         /* list of supported elliptic curves        */
    const char *alpn_string;    /* ALPN supported protocols                 */
    const char *dhm_file;       /* the file with the DH parameters          */
    int extended_ms;            /* allow negotiation of extended MS?        */
    int etm;                    /* allow negotiation of encrypt-then-MAC?   */
    int transport;              /* TLS or DTLS?                             */
    int cookies;                /* Use cookies for DTLS? -1 to break them   */
    int anti_replay;            /* Use anti-replay for DTLS? -1 for default */
    uint32_t hs_to_min;         /* Initial value of DTLS handshake timer    */
    uint32_t hs_to_max;         /* Max value of DTLS handshake timer        */
    int dtls_mtu;               /* UDP Maximum tranport unit for DTLS       */
    int dgram_packing;          /* allow/forbid datagram packing            */
    int badmac_limit;           /* Limit of records with bad MAC            */
} opt;

int query_config( const char *config );

static void my_debug( void *ctx, int level,
                      const char *file, int line,
                      const char *str )
{
    const char *p, *basename;

    /* Extract basename from file */
    for( p = basename = file; *p != '\0'; p++ )
        if( *p == '/' || *p == '\\' )
            basename = p + 1;

    mbedtls_fprintf( (FILE *) ctx, "%s:%04d: |%d| %s", basename, line, level, str );
    fflush(  (FILE *) ctx  );
}

/*
 * Test recv/send functions that make sure each try returns
 * WANT_READ/WANT_WRITE at least once before sucesseding
 */
static int my_recv( void *ctx, unsigned char *buf, size_t len )
{
    static int first_try = 1;
    int ret;

    if( first_try )
    {
        first_try = 0;
        return( MBEDTLS_ERR_SSL_WANT_READ );
    }

    ret = mbedtls_net_recv( ctx, buf, len );
    if( ret != MBEDTLS_ERR_SSL_WANT_READ )
        first_try = 1; /* Next call will be a new operation */
    return( ret );
}

static int my_send( void *ctx, const unsigned char *buf, size_t len )
{
    static int first_try = 1;
    int ret;

    if( first_try )
    {
        first_try = 0;
        return( MBEDTLS_ERR_SSL_WANT_WRITE );
    }

    ret = mbedtls_net_send( ctx, buf, len );
    if( ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        first_try = 1; /* Next call will be a new operation */
    return( ret );
}

/*
 * Return authmode from string, or -1 on error
 */
static int get_auth_mode( const char *s )
{
    if( strcmp( s, "none" ) == 0 )
        return( MBEDTLS_SSL_VERIFY_NONE );
    if( strcmp( s, "optional" ) == 0 )
        return( MBEDTLS_SSL_VERIFY_OPTIONAL );
    if( strcmp( s, "required" ) == 0 )
        return( MBEDTLS_SSL_VERIFY_REQUIRED );

    return( -1 );
}

/*
 * Used by sni_parse and psk_parse to handle coma-separated lists
 */
#define GET_ITEM( dst )         \
    dst = p;                    \
    while( *p != ',' )          \
        if( ++p > end )         \
            goto error;         \
    *p++ = '\0';

#if defined(SNI_OPTION)
typedef struct _sni_entry sni_entry;

struct _sni_entry {
    const char *name;
    mbedtls_x509_crt *cert;
    mbedtls_pk_context *key;
    mbedtls_x509_crt* ca;
    mbedtls_x509_crl* crl;
    int authmode;
    sni_entry *next;
};

void sni_free( sni_entry *head )
{
    sni_entry *cur = head, *next;

    while( cur != NULL )
    {
        mbedtls_x509_crt_free( cur->cert );
        mbedtls_free( cur->cert );

        mbedtls_pk_free( cur->key );
        mbedtls_free( cur->key );

        mbedtls_x509_crt_free( cur->ca );
        mbedtls_free( cur->ca );

        mbedtls_x509_crl_free( cur->crl );
        mbedtls_free( cur->crl );

        next = cur->next;
        mbedtls_free( cur );
        cur = next;
    }
}

/*
 * Parse a string of sextuples name1,crt1,key1,ca1,crl1,auth1[,...]
 * into a usable sni_entry list. For ca1, crl1, auth1, the special value
 * '-' means unset. If ca1 is unset, then crl1 is ignored too.
 *
 * Modifies the input string! This is not production quality!
 */
sni_entry *sni_parse( char *sni_string )
{
    sni_entry *cur = NULL, *new = NULL;
    char *p = sni_string;
    char *end = p;
    char *crt_file, *key_file, *ca_file, *crl_file, *auth_str;

    while( *end != '\0' )
        ++end;
    *end = ',';

    while( p <= end )
    {
        if( ( new = mbedtls_calloc( 1, sizeof( sni_entry ) ) ) == NULL )
        {
            sni_free( cur );
            return( NULL );
        }

        GET_ITEM( new->name );
        GET_ITEM( crt_file );
        GET_ITEM( key_file );
        GET_ITEM( ca_file );
        GET_ITEM( crl_file );
        GET_ITEM( auth_str );

        if( ( new->cert = mbedtls_calloc( 1, sizeof( mbedtls_x509_crt ) ) ) == NULL ||
            ( new->key = mbedtls_calloc( 1, sizeof( mbedtls_pk_context ) ) ) == NULL )
            goto error;

        mbedtls_x509_crt_init( new->cert );
        mbedtls_pk_init( new->key );

        if( mbedtls_x509_crt_parse_file( new->cert, crt_file ) != 0 ||
            mbedtls_pk_parse_keyfile( new->key, key_file, "" ) != 0 )
            goto error;

        if( strcmp( ca_file, "-" ) != 0 )
        {
            if( ( new->ca = mbedtls_calloc( 1, sizeof( mbedtls_x509_crt ) ) ) == NULL )
                goto error;

            mbedtls_x509_crt_init( new->ca );

            if( mbedtls_x509_crt_parse_file( new->ca, ca_file ) != 0 )
                goto error;
        }

        if( strcmp( crl_file, "-" ) != 0 )
        {
            if( ( new->crl = mbedtls_calloc( 1, sizeof( mbedtls_x509_crl ) ) ) == NULL )
                goto error;

            mbedtls_x509_crl_init( new->crl );

            if( mbedtls_x509_crl_parse_file( new->crl, crl_file ) != 0 )
                goto error;
        }

        if( strcmp( auth_str, "-" ) != 0 )
        {
            if( ( new->authmode = get_auth_mode( auth_str ) ) < 0 )
                goto error;
        }
        else
            new->authmode = DFL_AUTH_MODE;

        new->next = cur;
        cur = new;
    }

    return( cur );

error:
    sni_free( new );
    sni_free( cur );
    return( NULL );
}

/*
 * SNI callback.
 */
int sni_callback( void *p_info, mbedtls_ssl_context *ssl,
                  const unsigned char *name, size_t name_len )
{
    const sni_entry *cur = (const sni_entry *) p_info;

    while( cur != NULL )
    {
        if( name_len == strlen( cur->name ) &&
            memcmp( name, cur->name, name_len ) == 0 )
        {
            if( cur->ca != NULL )
                mbedtls_ssl_set_hs_ca_chain( ssl, cur->ca, cur->crl );

            if( cur->authmode != DFL_AUTH_MODE )
                mbedtls_ssl_set_hs_authmode( ssl, cur->authmode );

            return( mbedtls_ssl_set_hs_own_cert( ssl, cur->cert, cur->key ) );
        }

        cur = cur->next;
    }

    return( -1 );
}

#endif /* SNI_OPTION */

#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)

#define HEX2NUM( c )                    \
        if( c >= '0' && c <= '9' )      \
            c -= '0';                   \
        else if( c >= 'a' && c <= 'f' ) \
            c -= 'a' - 10;              \
        else if( c >= 'A' && c <= 'F' ) \
            c -= 'A' - 10;              \
        else                            \
            return( -1 );

/*
 * Convert a hex string to bytes.
 * Return 0 on success, -1 on error.
 */
int unhexify( unsigned char *output, const char *input, size_t *olen )
{
    unsigned char c;
    size_t j;

    *olen = strlen( input );
    if( *olen % 2 != 0 || *olen / 2 > MBEDTLS_PSK_MAX_LEN )
        return( -1 );
    *olen /= 2;

    for( j = 0; j < *olen * 2; j += 2 )
    {
        c = input[j];
        HEX2NUM( c );
        output[ j / 2 ] = c << 4;

        c = input[j + 1];
        HEX2NUM( c );
        output[ j / 2 ] |= c;
    }

    return( 0 );
}

typedef struct _psk_entry psk_entry;

struct _psk_entry
{
    const char *name;
    size_t key_len;
    unsigned char key[MBEDTLS_PSK_MAX_LEN];
    psk_entry *next;
};

/*
 * Free a list of psk_entry's
 */
void psk_free( psk_entry *head )
{
    psk_entry *next;

    while( head != NULL )
    {
        next = head->next;
        mbedtls_free( head );
        head = next;
    }
}

/*
 * Parse a string of pairs name1,key1[,name2,key2[,...]]
 * into a usable psk_entry list.
 *
 * Modifies the input string! This is not production quality!
 */
psk_entry *psk_parse( char *psk_string )
{
    psk_entry *cur = NULL, *new = NULL;
    char *p = psk_string;
    char *end = p;
    char *key_hex;

    while( *end != '\0' )
        ++end;
    *end = ',';

    while( p <= end )
    {
        if( ( new = mbedtls_calloc( 1, sizeof( psk_entry ) ) ) == NULL )
            goto error;

        memset( new, 0, sizeof( psk_entry ) );

        GET_ITEM( new->name );
        GET_ITEM( key_hex );

        if( unhexify( new->key, key_hex, &new->key_len ) != 0 )
            goto error;

        new->next = cur;
        cur = new;
    }

    return( cur );

error:
    psk_free( new );
    psk_free( cur );
    return( 0 );
}

/*
 * PSK callback
 */
int psk_callback( void *p_info, mbedtls_ssl_context *ssl,
                  const unsigned char *name, size_t name_len )
{
    psk_entry *cur = (psk_entry *) p_info;

    while( cur != NULL )
    {
        if( name_len == strlen( cur->name ) &&
            memcmp( name, cur->name, name_len ) == 0 )
        {
            return( mbedtls_ssl_set_hs_psk( ssl, cur->key, cur->key_len ) );
        }

        cur = cur->next;
    }

    return( -1 );
}
#endif /* MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED */

static mbedtls_net_context listen_fd, client_fd;

/* Interruption handler to ensure clean exit (for valgrind testing) */
#if !defined(_WIN32)
static int received_sigterm = 0;
void term_handler( int sig )
{
    ((void) sig);
    received_sigterm = 1;
    mbedtls_net_free( &listen_fd ); /* causes mbedtls_net_accept() to abort */
    mbedtls_net_free( &client_fd ); /* causes net_read() to abort */
}
#endif

#if defined(MBEDTLS_X509_CRT_PARSE_C)
static int ssl_sig_hashes_for_test[] = {
#if defined(MBEDTLS_SHA512_C)
    MBEDTLS_MD_SHA512,
    MBEDTLS_MD_SHA384,
#endif
#if defined(MBEDTLS_SHA256_C)
    MBEDTLS_MD_SHA256,
    MBEDTLS_MD_SHA224,
#endif
#if defined(MBEDTLS_SHA1_C)
    /* Allow SHA-1 as we use it extensively in tests. */
    MBEDTLS_MD_SHA1,
#endif
    MBEDTLS_MD_NONE
};
#endif /* MBEDTLS_X509_CRT_PARSE_C */

/** Return true if \p ret is a status code indicating that there is an
 * operation in progress on an SSL connection, and false if it indicates
 * success or a fatal error.
 *
 * The possible operations in progress are:
 *
 * - A read, when the SSL input buffer does not contain a full message.
 * - A write, when the SSL output buffer contains some data that has not
 *   been sent over the network yet.
 * - An asynchronous callback that has not completed yet. */
static int mbedtls_status_is_ssl_in_progress( int ret )
{
    return( ret == MBEDTLS_ERR_SSL_WANT_READ ||
            ret == MBEDTLS_ERR_SSL_WANT_WRITE ||
            ret == MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS );
}

#if defined(MBEDTLS_SSL_ASYNC_PRIVATE)
typedef struct
{
    mbedtls_x509_crt *cert; /*!< Certificate corresponding to the key */
    mbedtls_pk_context *pk; /*!< Private key */
    unsigned delay; /*!< Number of resume steps to go through */
    unsigned pk_owned : 1; /*!< Whether to free the pk object on exit */
} ssl_async_key_slot_t;

typedef enum {
    SSL_ASYNC_INJECT_ERROR_NONE = 0, /*!< Let the callbacks succeed */
    SSL_ASYNC_INJECT_ERROR_START, /*!< Inject error during start */
    SSL_ASYNC_INJECT_ERROR_CANCEL, /*!< Close the connection after async start */
    SSL_ASYNC_INJECT_ERROR_RESUME, /*!< Inject error during resume */
#define SSL_ASYNC_INJECT_ERROR_MAX SSL_ASYNC_INJECT_ERROR_RESUME
} ssl_async_inject_error_t;

typedef struct
{
    ssl_async_key_slot_t slots[4]; /* key, key2, sni1, sni2 */
    size_t slots_used;
    ssl_async_inject_error_t inject_error;
    int (*f_rng)(void *, unsigned char *, size_t);
    void *p_rng;
} ssl_async_key_context_t;

int ssl_async_set_key( ssl_async_key_context_t *ctx,
                       mbedtls_x509_crt *cert,
                       mbedtls_pk_context *pk,
                       int pk_take_ownership,
                       unsigned delay )
{
    if( ctx->slots_used >= sizeof( ctx->slots ) / sizeof( *ctx->slots ) )
        return( -1 );
    ctx->slots[ctx->slots_used].cert = cert;
    ctx->slots[ctx->slots_used].pk = pk;
    ctx->slots[ctx->slots_used].delay = delay;
    ctx->slots[ctx->slots_used].pk_owned = pk_take_ownership;
    ++ctx->slots_used;
    return( 0 );
}

#define SSL_ASYNC_INPUT_MAX_SIZE 512

typedef enum
{
    ASYNC_OP_SIGN,
    ASYNC_OP_DECRYPT,
} ssl_async_operation_type_t;
/* Note that the enum above and the array below need to be kept in sync!
 * `ssl_async_operation_names[op]` is the name of op for each value `op`
 * of type `ssl_async_operation_type_t`. */
static const char *const ssl_async_operation_names[] =
{
    "sign",
    "decrypt",
};

typedef struct
{
    unsigned slot;
    ssl_async_operation_type_t operation_type;
    mbedtls_md_type_t md_alg;
    unsigned char input[SSL_ASYNC_INPUT_MAX_SIZE];
    size_t input_len;
    unsigned remaining_delay;
} ssl_async_operation_context_t;

static int ssl_async_start( mbedtls_ssl_context *ssl,
                            mbedtls_x509_crt *cert,
                            ssl_async_operation_type_t op_type,
                            mbedtls_md_type_t md_alg,
                            const unsigned char *input,
                            size_t input_len )
{
    ssl_async_key_context_t *config_data =
        mbedtls_ssl_conf_get_async_config_data( ssl->conf );
    unsigned slot;
    ssl_async_operation_context_t *ctx = NULL;
    const char *op_name = ssl_async_operation_names[op_type];

    {
        char dn[100];
        if( mbedtls_x509_dn_gets( dn, sizeof( dn ), &cert->subject ) > 0 )
            mbedtls_printf( "Async %s callback: looking for DN=%s\n",
                            op_name, dn );
    }

    /* Look for a private key that matches the public key in cert.
     * Since this test code has the private key inside Mbed TLS,
     * we call mbedtls_pk_check_pair to match a private key with the
     * public key. */
    for( slot = 0; slot < config_data->slots_used; slot++ )
    {
        if( mbedtls_pk_check_pair( &cert->pk,
                                   config_data->slots[slot].pk ) == 0 )
            break;
    }
    if( slot == config_data->slots_used )
    {
        mbedtls_printf( "Async %s callback: no key matches this certificate.\n",
                        op_name );
        return( MBEDTLS_ERR_SSL_HW_ACCEL_FALLTHROUGH );
    }
    mbedtls_printf( "Async %s callback: using key slot %u, delay=%u.\n",
                    op_name, slot, config_data->slots[slot].delay );

    if( config_data->inject_error == SSL_ASYNC_INJECT_ERROR_START )
    {
        mbedtls_printf( "Async %s callback: injected error\n", op_name );
        return( MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE );
    }

    if( input_len > SSL_ASYNC_INPUT_MAX_SIZE )
        return( MBEDTLS_ERR_SSL_BAD_INPUT_DATA );

    ctx = mbedtls_calloc( 1, sizeof( *ctx ) );
    if( ctx == NULL )
        return( MBEDTLS_ERR_SSL_ALLOC_FAILED );
    ctx->slot = slot;
    ctx->operation_type = op_type;
    ctx->md_alg = md_alg;
    memcpy( ctx->input, input, input_len );
    ctx->input_len = input_len;
    ctx->remaining_delay = config_data->slots[slot].delay;
    mbedtls_ssl_set_async_operation_data( ssl, ctx );

    if( ctx->remaining_delay == 0 )
        return( 0 );
    else
        return( MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS );
}

static int ssl_async_sign( mbedtls_ssl_context *ssl,
                           mbedtls_x509_crt *cert,
                           mbedtls_md_type_t md_alg,
                           const unsigned char *hash,
                           size_t hash_len )
{
    return( ssl_async_start( ssl, cert,
                             ASYNC_OP_SIGN, md_alg,
                             hash, hash_len ) );
}

static int ssl_async_decrypt( mbedtls_ssl_context *ssl,
                              mbedtls_x509_crt *cert,
                              const unsigned char *input,
                              size_t input_len )
{
    return( ssl_async_start( ssl, cert,
                             ASYNC_OP_DECRYPT, MBEDTLS_MD_NONE,
                             input, input_len ) );
}

static int ssl_async_resume( mbedtls_ssl_context *ssl,
                             unsigned char *output,
                             size_t *output_len,
                             size_t output_size )
{
    ssl_async_operation_context_t *ctx = mbedtls_ssl_get_async_operation_data( ssl );
    ssl_async_key_context_t *config_data =
        mbedtls_ssl_conf_get_async_config_data( ssl->conf );
    ssl_async_key_slot_t *key_slot = &config_data->slots[ctx->slot];
    int ret;
    const char *op_name;

    if( ctx->remaining_delay > 0 )
    {
        --ctx->remaining_delay;
        mbedtls_printf( "Async resume (slot %u): call %u more times.\n",
                        ctx->slot, ctx->remaining_delay );
        return( MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS );
    }

    switch( ctx->operation_type )
    {
        case ASYNC_OP_DECRYPT:
            ret = mbedtls_pk_decrypt( key_slot->pk,
                                      ctx->input, ctx->input_len,
                                      output, output_len, output_size,
                                      config_data->f_rng, config_data->p_rng );
            break;
        case ASYNC_OP_SIGN:
            ret = mbedtls_pk_sign( key_slot->pk,
                                   ctx->md_alg,
                                   ctx->input, ctx->input_len,
                                   output, output_len,
                                   config_data->f_rng, config_data->p_rng );
            break;
        default:
            mbedtls_printf( "Async resume (slot %u): unknown operation type %ld. This shouldn't happen.\n",
                            ctx->slot, (long) ctx->operation_type );
            mbedtls_free( ctx );
            return( MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE );
            break;
    }

    op_name = ssl_async_operation_names[ctx->operation_type];

    if( config_data->inject_error == SSL_ASYNC_INJECT_ERROR_RESUME )
    {
        mbedtls_printf( "Async resume callback: %s done but injected error\n",
                        op_name );
        mbedtls_free( ctx );
        return( MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE );
    }

    mbedtls_printf( "Async resume (slot %u): %s done, status=%d.\n",
                    ctx->slot, op_name, ret );
    mbedtls_free( ctx );
    return( ret );
}

static void ssl_async_cancel( mbedtls_ssl_context *ssl )
{
    ssl_async_operation_context_t *ctx = mbedtls_ssl_get_async_operation_data( ssl );
    mbedtls_printf( "Async cancel callback.\n" );
    mbedtls_free( ctx );
}
#endif /* MBEDTLS_SSL_ASYNC_PRIVATE */

/*
 * Wait for an event from the underlying transport or the timer
 * (Used in event-driven IO mode).
 */
#if !defined(MBEDTLS_TIMING_C)
int idle( mbedtls_net_context *fd,
          int idle_reason )
#else
int idle( mbedtls_net_context *fd,
          mbedtls_timing_delay_context *timer,
          int idle_reason )
#endif
{
    int ret;
    int poll_type = 0;

    if( idle_reason == MBEDTLS_ERR_SSL_WANT_WRITE )
        poll_type = MBEDTLS_NET_POLL_WRITE;
    else if( idle_reason == MBEDTLS_ERR_SSL_WANT_READ )
        poll_type = MBEDTLS_NET_POLL_READ;
#if !defined(MBEDTLS_TIMING_C)
    else
        return( 0 );
#endif

    while( 1 )
    {
        /* Check if timer has expired */
#if defined(MBEDTLS_TIMING_C)
        if( timer != NULL &&
            mbedtls_timing_get_delay( timer ) == 2 )
        {
            break;
        }
#endif /* MBEDTLS_TIMING_C */

        /* Check if underlying transport became available */
        if( poll_type != 0 )
        {
            ret = mbedtls_net_poll( fd, poll_type, 0 );
            if( ret < 0 )
                return( ret );
            if( ret == poll_type )
                break;
        }
    }

    return( 0 );
}

int main( int argc, char *argv[] )
{
    int ret = 0, len, written, frags, exchanges_left;
    int version_suites[4][2];
    unsigned char* buf = 0;
#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
    unsigned char psk[MBEDTLS_PSK_MAX_LEN];
    size_t psk_len = 0;
    psk_entry *psk_info = NULL;
#endif
    const char *pers = "ssl_server2";
    unsigned char client_ip[16] = { 0 };
    size_t cliip_len;
#if defined(MBEDTLS_SSL_COOKIE_C)
    mbedtls_ssl_cookie_ctx cookie_ctx;
#endif

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    mbedtls_x509_crt_profile crt_profile_for_test = mbedtls_x509_crt_profile_default;
#endif
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
#if defined(MBEDTLS_TIMING_C)
    mbedtls_timing_delay_context timer;
#endif
#if defined(MBEDTLS_SSL_RENEGOTIATION)
    unsigned char renego_period[8] = { 0 };
#endif
#if defined(MBEDTLS_X509_CRT_PARSE_C)
    uint32_t flags;
    mbedtls_x509_crt cacert;
    mbedtls_x509_crt srvcert;
    mbedtls_pk_context pkey;
    mbedtls_x509_crt srvcert2;
    mbedtls_pk_context pkey2;
    int key_cert_init = 0, key_cert_init2 = 0;
#if defined(MBEDTLS_SSL_ASYNC_PRIVATE)
    ssl_async_key_context_t ssl_async_keys;
#endif /* MBEDTLS_SSL_ASYNC_PRIVATE */
#endif /* MBEDTLS_X509_CRT_PARSE_C */
#if defined(MBEDTLS_DHM_C) && defined(MBEDTLS_FS_IO)
    mbedtls_dhm_context dhm;
#endif
#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_cache_context cache;
#endif
#if defined(MBEDTLS_SSL_SESSION_TICKETS)
    mbedtls_ssl_ticket_context ticket_ctx;
#endif
#if defined(SNI_OPTION)
    sni_entry *sni_info = NULL;
#endif
#if defined(MBEDTLS_ECP_C)
    mbedtls_ecp_group_id curve_list[CURVE_LIST_SIZE];
    const mbedtls_ecp_curve_info * curve_cur;
#endif
#if defined(MBEDTLS_SSL_ALPN)
    const char *alpn_list[ALPN_LIST_SIZE];
#endif
#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
    unsigned char alloc_buf[MEMORY_HEAP_SIZE];
#endif

    int i;
    char *p, *q;
    const int *list;

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
    mbedtls_memory_buffer_alloc_init( alloc_buf, sizeof(alloc_buf) );
#endif

    /*
     * Make sure memory references are valid in case we exit early.
     */
    mbedtls_net_init( &client_fd );
    mbedtls_net_init( &listen_fd );
    mbedtls_ssl_init( &ssl );
    mbedtls_ssl_config_init( &conf );
    mbedtls_ctr_drbg_init( &ctr_drbg );
#if defined(MBEDTLS_X509_CRT_PARSE_C)
    mbedtls_x509_crt_init( &cacert );
    mbedtls_x509_crt_init( &srvcert );
    mbedtls_pk_init( &pkey );
    mbedtls_x509_crt_init( &srvcert2 );
    mbedtls_pk_init( &pkey2 );
#if defined(MBEDTLS_SSL_ASYNC_PRIVATE)
    memset( &ssl_async_keys, 0, sizeof( ssl_async_keys ) );
#endif
#endif
#if defined(MBEDTLS_DHM_C) && defined(MBEDTLS_FS_IO)
    mbedtls_dhm_init( &dhm );
#endif
#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_cache_init( &cache );
#endif
#if defined(MBEDTLS_SSL_SESSION_TICKETS)
    mbedtls_ssl_ticket_init( &ticket_ctx );
#endif
#if defined(MBEDTLS_SSL_ALPN)
    memset( (void *) alpn_list, 0, sizeof( alpn_list ) );
#endif
#if defined(MBEDTLS_SSL_COOKIE_C)
    mbedtls_ssl_cookie_init( &cookie_ctx );
#endif

#if !defined(_WIN32)
    /* Abort cleanly on SIGTERM and SIGINT */
    signal( SIGTERM, term_handler );
    signal( SIGINT, term_handler );
#endif

    if( argc == 0 )
    {
    usage:
        if( ret == 0 )
            ret = 1;

        mbedtls_printf( USAGE );

        list = mbedtls_ssl_list_ciphersuites();
        while( *list )
        {
            mbedtls_printf(" %-42s", mbedtls_ssl_get_ciphersuite_name( *list ) );
            list++;
            if( !*list )
                break;
            mbedtls_printf(" %s\n", mbedtls_ssl_get_ciphersuite_name( *list ) );
            list++;
        }
        mbedtls_printf("\n");
        goto exit;
    }

    opt.buffer_size         = DFL_IO_BUF_LEN;
    opt.server_addr         = DFL_SERVER_ADDR;
    opt.server_port         = DFL_SERVER_PORT;
    opt.debug_level         = DFL_DEBUG_LEVEL;
    opt.event               = DFL_EVENT;
    opt.response_size       = DFL_RESPONSE_SIZE;
    opt.nbio                = DFL_NBIO;
    opt.read_timeout        = DFL_READ_TIMEOUT;
    opt.ca_file             = DFL_CA_FILE;
    opt.ca_path             = DFL_CA_PATH;
    opt.crt_file            = DFL_CRT_FILE;
    opt.key_file            = DFL_KEY_FILE;
    opt.crt_file2           = DFL_CRT_FILE2;
    opt.key_file2           = DFL_KEY_FILE2;
    opt.async_operations    = DFL_ASYNC_OPERATIONS;
    opt.async_private_delay1 = DFL_ASYNC_PRIVATE_DELAY1;
    opt.async_private_delay2 = DFL_ASYNC_PRIVATE_DELAY2;
    opt.async_private_error = DFL_ASYNC_PRIVATE_ERROR;
    opt.psk                 = DFL_PSK;
    opt.psk_identity        = DFL_PSK_IDENTITY;
    opt.psk_list            = DFL_PSK_LIST;
    opt.ecjpake_pw          = DFL_ECJPAKE_PW;
    opt.force_ciphersuite[0]= DFL_FORCE_CIPHER;
    opt.version_suites      = DFL_VERSION_SUITES;
    opt.renegotiation       = DFL_RENEGOTIATION;
    opt.allow_legacy        = DFL_ALLOW_LEGACY;
    opt.renegotiate         = DFL_RENEGOTIATE;
    opt.renego_delay        = DFL_RENEGO_DELAY;
    opt.renego_period       = DFL_RENEGO_PERIOD;
    opt.exchanges           = DFL_EXCHANGES;
    opt.min_version         = DFL_MIN_VERSION;
    opt.max_version         = DFL_MAX_VERSION;
    opt.arc4                = DFL_ARC4;
    opt.allow_sha1          = DFL_SHA1;
    opt.auth_mode           = DFL_AUTH_MODE;
    opt.cert_req_ca_list    = DFL_CERT_REQ_CA_LIST;
    opt.mfl_code            = DFL_MFL_CODE;
    opt.trunc_hmac          = DFL_TRUNC_HMAC;
    opt.tickets             = DFL_TICKETS;
    opt.ticket_timeout      = DFL_TICKET_TIMEOUT;
    opt.cache_max           = DFL_CACHE_MAX;
    opt.cache_timeout       = DFL_CACHE_TIMEOUT;
    opt.sni                 = DFL_SNI;
    opt.alpn_string         = DFL_ALPN_STRING;
    opt.curves              = DFL_CURVES;
    opt.dhm_file            = DFL_DHM_FILE;
    opt.transport           = DFL_TRANSPORT;
    opt.cookies             = DFL_COOKIES;
    opt.anti_replay         = DFL_ANTI_REPLAY;
    opt.hs_to_min           = DFL_HS_TO_MIN;
    opt.hs_to_max           = DFL_HS_TO_MAX;
    opt.dtls_mtu            = DFL_DTLS_MTU;
    opt.dgram_packing       = DFL_DGRAM_PACKING;
    opt.badmac_limit        = DFL_BADMAC_LIMIT;
    opt.extended_ms         = DFL_EXTENDED_MS;
    opt.etm                 = DFL_ETM;

    for( i = 1; i < argc; i++ )
    {
        p = argv[i];
        if( ( q = strchr( p, '=' ) ) == NULL )
            goto usage;
        *q++ = '\0';

        if( strcmp( p, "server_port" ) == 0 )
            opt.server_port = q;
        else if( strcmp( p, "server_addr" ) == 0 )
            opt.server_addr = q;
        else if( strcmp( p, "dtls" ) == 0 )
        {
            int t = atoi( q );
            if( t == 0 )
                opt.transport = MBEDTLS_SSL_TRANSPORT_STREAM;
            else if( t == 1 )
                opt.transport = MBEDTLS_SSL_TRANSPORT_DATAGRAM;
            else
                goto usage;
        }
        else if( strcmp( p, "debug_level" ) == 0 )
        {
            opt.debug_level = atoi( q );
            if( opt.debug_level < 0 || opt.debug_level > 65535 )
                goto usage;
        }
        else if( strcmp( p, "nbio" ) == 0 )
        {
            opt.nbio = atoi( q );
            if( opt.nbio < 0 || opt.nbio > 2 )
                goto usage;
        }
        else if( strcmp( p, "event" ) == 0 )
        {
            opt.event = atoi( q );
            if( opt.event < 0 || opt.event > 2 )
                goto usage;
        }
        else if( strcmp( p, "read_timeout" ) == 0 )
            opt.read_timeout = atoi( q );
        else if( strcmp( p, "buffer_size" ) == 0 )
        {
            opt.buffer_size = atoi( q );
            if( opt.buffer_size < 1 || opt.buffer_size > MBEDTLS_SSL_MAX_CONTENT_LEN + 1 )
                goto usage;
        }
        else if( strcmp( p, "response_size" ) == 0 )
        {
            opt.response_size = atoi( q );
            if( opt.response_size < 0 || opt.response_size > MBEDTLS_SSL_MAX_CONTENT_LEN )
                goto usage;
            if( opt.buffer_size < opt.response_size )
                opt.buffer_size = opt.response_size;
        }
        else if( strcmp( p, "ca_file" ) == 0 )
            opt.ca_file = q;
        else if( strcmp( p, "ca_path" ) == 0 )
            opt.ca_path = q;
        else if( strcmp( p, "crt_file" ) == 0 )
            opt.crt_file = q;
        else if( strcmp( p, "key_file" ) == 0 )
            opt.key_file = q;
        else if( strcmp( p, "crt_file2" ) == 0 )
            opt.crt_file2 = q;
        else if( strcmp( p, "key_file2" ) == 0 )
            opt.key_file2 = q;
        else if( strcmp( p, "dhm_file" ) == 0 )
            opt.dhm_file = q;
#if defined(MBEDTLS_SSL_ASYNC_PRIVATE)
        else if( strcmp( p, "async_operations" ) == 0 )
            opt.async_operations = q;
        else if( strcmp( p, "async_private_delay1" ) == 0 )
            opt.async_private_delay1 = atoi( q );
        else if( strcmp( p, "async_private_delay2" ) == 0 )
            opt.async_private_delay2 = atoi( q );
        else if( strcmp( p, "async_private_error" ) == 0 )
        {
            int n = atoi( q );
            if( n < -SSL_ASYNC_INJECT_ERROR_MAX ||
                n > SSL_ASYNC_INJECT_ERROR_MAX )
            {
                ret = 2;
                goto usage;
            }
            opt.async_private_error = n;
        }
#endif /* MBEDTLS_SSL_ASYNC_PRIVATE */
        else if( strcmp( p, "psk" ) == 0 )
            opt.psk = q;
        else if( strcmp( p, "psk_identity" ) == 0 )
            opt.psk_identity = q;
        else if( strcmp( p, "psk_list" ) == 0 )
            opt.psk_list = q;
        else if( strcmp( p, "ecjpake_pw" ) == 0 )
            opt.ecjpake_pw = q;
        else if( strcmp( p, "force_ciphersuite" ) == 0 )
        {
            opt.force_ciphersuite[0] = mbedtls_ssl_get_ciphersuite_id( q );

            if( opt.force_ciphersuite[0] == 0 )
            {
                ret = 2;
                goto usage;
            }
            opt.force_ciphersuite[1] = 0;
        }
        else if( strcmp( p, "curves" ) == 0 )
            opt.curves = q;
        else if( strcmp( p, "version_suites" ) == 0 )
            opt.version_suites = q;
        else if( strcmp( p, "renegotiation" ) == 0 )
        {
            opt.renegotiation = (atoi( q )) ?
                MBEDTLS_SSL_RENEGOTIATION_ENABLED :
                MBEDTLS_SSL_RENEGOTIATION_DISABLED;
        }
        else if( strcmp( p, "allow_legacy" ) == 0 )
        {
            switch( atoi( q ) )
            {
                case -1:
                    opt.allow_legacy = MBEDTLS_SSL_LEGACY_BREAK_HANDSHAKE;
                    break;
                case 0:
                    opt.allow_legacy = MBEDTLS_SSL_LEGACY_NO_RENEGOTIATION;
                    break;
                case 1:
                    opt.allow_legacy = MBEDTLS_SSL_LEGACY_ALLOW_RENEGOTIATION;
                    break;
                default: goto usage;
            }
        }
        else if( strcmp( p, "renegotiate" ) == 0 )
        {
            opt.renegotiate = atoi( q );
            if( opt.renegotiate < 0 || opt.renegotiate > 1 )
                goto usage;
        }
        else if( strcmp( p, "renego_delay" ) == 0 )
        {
            opt.renego_delay = atoi( q );
        }
        else if( strcmp( p, "renego_period" ) == 0 )
        {
#if defined(_MSC_VER)
            opt.renego_period = _strtoui64( q, NULL, 10 );
#else
            if( sscanf( q, "%" SCNu64, &opt.renego_period ) != 1 )
                goto usage;
#endif /* _MSC_VER */
            if( opt.renego_period < 2 )
                goto usage;
        }
        else if( strcmp( p, "exchanges" ) == 0 )
        {
            opt.exchanges = atoi( q );
            if( opt.exchanges < 0 )
                goto usage;
        }
        else if( strcmp( p, "min_version" ) == 0 )
        {
            if( strcmp( q, "ssl3" ) == 0 )
                opt.min_version = MBEDTLS_SSL_MINOR_VERSION_0;
            else if( strcmp( q, "tls1" ) == 0 )
                opt.min_version = MBEDTLS_SSL_MINOR_VERSION_1;
            else if( strcmp( q, "tls1_1" ) == 0 ||
                     strcmp( q, "dtls1" ) == 0 )
                opt.min_version = MBEDTLS_SSL_MINOR_VERSION_2;
            else if( strcmp( q, "tls1_2" ) == 0 ||
                     strcmp( q, "dtls1_2" ) == 0 )
                opt.min_version = MBEDTLS_SSL_MINOR_VERSION_3;
            else
                goto usage;
        }
        else if( strcmp( p, "max_version" ) == 0 )
        {
            if( strcmp( q, "ssl3" ) == 0 )
                opt.max_version = MBEDTLS_SSL_MINOR_VERSION_0;
            else if( strcmp( q, "tls1" ) == 0 )
                opt.max_version = MBEDTLS_SSL_MINOR_VERSION_1;
            else if( strcmp( q, "tls1_1" ) == 0 ||
                     strcmp( q, "dtls1" ) == 0 )
                opt.max_version = MBEDTLS_SSL_MINOR_VERSION_2;
            else if( strcmp( q, "tls1_2" ) == 0 ||
                     strcmp( q, "dtls1_2" ) == 0 )
                opt.max_version = MBEDTLS_SSL_MINOR_VERSION_3;
            else
                goto usage;
        }
        else if( strcmp( p, "arc4" ) == 0 )
        {
            switch( atoi( q ) )
            {
                case 0:     opt.arc4 = MBEDTLS_SSL_ARC4_DISABLED;   break;
                case 1:     opt.arc4 = MBEDTLS_SSL_ARC4_ENABLED;    break;
                default:    goto usage;
            }
        }
        else if( strcmp( p, "allow_sha1" ) == 0 )
        {
            switch( atoi( q ) )
            {
                case 0:     opt.allow_sha1 = 0;   break;
                case 1:     opt.allow_sha1 = 1;    break;
                default:    goto usage;
            }
        }
        else if( strcmp( p, "force_version" ) == 0 )
        {
            if( strcmp( q, "ssl3" ) == 0 )
            {
                opt.min_version = MBEDTLS_SSL_MINOR_VERSION_0;
                opt.max_version = MBEDTLS_SSL_MINOR_VERSION_0;
            }
            else if( strcmp( q, "tls1" ) == 0 )
            {
                opt.min_version = MBEDTLS_SSL_MINOR_VERSION_1;
                opt.max_version = MBEDTLS_SSL_MINOR_VERSION_1;
            }
            else if( strcmp( q, "tls1_1" ) == 0 )
            {
                opt.min_version = MBEDTLS_SSL_MINOR_VERSION_2;
                opt.max_version = MBEDTLS_SSL_MINOR_VERSION_2;
            }
            else if( strcmp( q, "tls1_2" ) == 0 )
            {
                opt.min_version = MBEDTLS_SSL_MINOR_VERSION_3;
                opt.max_version = MBEDTLS_SSL_MINOR_VERSION_3;
            }
            else if( strcmp( q, "dtls1" ) == 0 )
            {
                opt.min_version = MBEDTLS_SSL_MINOR_VERSION_2;
                opt.max_version = MBEDTLS_SSL_MINOR_VERSION_2;
                opt.transport = MBEDTLS_SSL_TRANSPORT_DATAGRAM;
            }
            else if( strcmp( q, "dtls1_2" ) == 0 )
            {
                opt.min_version = MBEDTLS_SSL_MINOR_VERSION_3;
                opt.max_version = MBEDTLS_SSL_MINOR_VERSION_3;
                opt.transport = MBEDTLS_SSL_TRANSPORT_DATAGRAM;
            }
            else
                goto usage;
        }
        else if( strcmp( p, "auth_mode" ) == 0 )
        {
            if( ( opt.auth_mode = get_auth_mode( q ) ) < 0 )
                goto usage;
        }
        else if( strcmp( p, "cert_req_ca_list" ) == 0 )
        {
            opt.cert_req_ca_list = atoi( q );
            if( opt.cert_req_ca_list < 0 || opt.cert_req_ca_list > 1 )
                goto usage;
        }
        else if( strcmp( p, "max_frag_len" ) == 0 )
        {
            if( strcmp( q, "512" ) == 0 )
                opt.mfl_code = MBEDTLS_SSL_MAX_FRAG_LEN_512;
            else if( strcmp( q, "1024" ) == 0 )
                opt.mfl_code = MBEDTLS_SSL_MAX_FRAG_LEN_1024;
            else if( strcmp( q, "2048" ) == 0 )
                opt.mfl_code = MBEDTLS_SSL_MAX_FRAG_LEN_2048;
            else if( strcmp( q, "4096" ) == 0 )
                opt.mfl_code = MBEDTLS_SSL_MAX_FRAG_LEN_4096;
            else
                goto usage;
        }
        else if( strcmp( p, "alpn" ) == 0 )
        {
            opt.alpn_string = q;
        }
        else if( strcmp( p, "trunc_hmac" ) == 0 )
        {
            switch( atoi( q ) )
            {
                case 0: opt.trunc_hmac = MBEDTLS_SSL_TRUNC_HMAC_DISABLED; break;
                case 1: opt.trunc_hmac = MBEDTLS_SSL_TRUNC_HMAC_ENABLED; break;
                default: goto usage;
            }
        }
        else if( strcmp( p, "extended_ms" ) == 0 )
        {
            switch( atoi( q ) )
            {
                case 0:
                    opt.extended_ms = MBEDTLS_SSL_EXTENDED_MS_DISABLED;
                    break;
                case 1:
                    opt.extended_ms = MBEDTLS_SSL_EXTENDED_MS_ENABLED;
                    break;
                default: goto usage;
            }
        }
        else if( strcmp( p, "etm" ) == 0 )
        {
            switch( atoi( q ) )
            {
                case 0: opt.etm = MBEDTLS_SSL_ETM_DISABLED; break;
                case 1: opt.etm = MBEDTLS_SSL_ETM_ENABLED; break;
                default: goto usage;
            }
        }
        else if( strcmp( p, "tickets" ) == 0 )
        {
            opt.tickets = atoi( q );
            if( opt.tickets < 0 || opt.tickets > 1 )
                goto usage;
        }
        else if( strcmp( p, "ticket_timeout" ) == 0 )
        {
            opt.ticket_timeout = atoi( q );
            if( opt.ticket_timeout < 0 )
                goto usage;
        }
        else if( strcmp( p, "cache_max" ) == 0 )
        {
            opt.cache_max = atoi( q );
            if( opt.cache_max < 0 )
                goto usage;
        }
        else if( strcmp( p, "cache_timeout" ) == 0 )
        {
            opt.cache_timeout = atoi( q );
            if( opt.cache_timeout < 0 )
                goto usage;
        }
        else if( strcmp( p, "cookies" ) == 0 )
        {
            opt.cookies = atoi( q );
            if( opt.cookies < -1 || opt.cookies > 1)
                goto usage;
        }
        else if( strcmp( p, "anti_replay" ) == 0 )
        {
            opt.anti_replay = atoi( q );
            if( opt.anti_replay < 0 || opt.anti_replay > 1)
                goto usage;
        }
        else if( strcmp( p, "badmac_limit" ) == 0 )
        {
            opt.badmac_limit = atoi( q );
            if( opt.badmac_limit < 0 )
                goto usage;
        }
        else if( strcmp( p, "hs_timeout" ) == 0 )
        {
            if( ( p = strchr( q, '-' ) ) == NULL )
                goto usage;
            *p++ = '\0';
            opt.hs_to_min = atoi( q );
            opt.hs_to_max = atoi( p );
            if( opt.hs_to_min == 0 || opt.hs_to_max < opt.hs_to_min )
                goto usage;
        }
        else if( strcmp( p, "mtu" ) == 0 )
        {
            opt.dtls_mtu = atoi( q );
            if( opt.dtls_mtu < 0 )
                goto usage;
        }
        else if( strcmp( p, "dgram_packing" ) == 0 )
        {
            opt.dgram_packing = atoi( q );
            if( opt.dgram_packing != 0 &&
                opt.dgram_packing != 1 )
            {
                goto usage;
            }
        }
        else if( strcmp( p, "sni" ) == 0 )
        {
            opt.sni = q;
        }
        else if( strcmp( p, "query_config" ) == 0 )
        {
            return query_config( q );
        }
        else
            goto usage;
    }

    /* Event-driven IO is incompatible with the above custom
     * receive and send functions, as the polling builds on
     * refers to the underlying net_context. */
    if( opt.event == 1 && opt.nbio != 1 )
    {
        mbedtls_printf( "Warning: event-driven IO mandates nbio=1 - overwrite\n" );
        opt.nbio = 1;
    }

#if defined(MBEDTLS_DEBUG_C)
    mbedtls_debug_set_threshold( opt.debug_level );
#endif
    buf = mbedtls_calloc( 1, opt.buffer_size + 1 );
    if( buf == NULL )
    {
        mbedtls_printf( "Could not allocate %u bytes\n", opt.buffer_size );
        ret = 3;
        goto exit;
    }

    if( opt.force_ciphersuite[0] > 0 )
    {
        const mbedtls_ssl_ciphersuite_t *ciphersuite_info;
        ciphersuite_info =
            mbedtls_ssl_ciphersuite_from_id( opt.force_ciphersuite[0] );

        if( opt.max_version != -1 &&
            ciphersuite_info->min_minor_ver > opt.max_version )
        {
            mbedtls_printf( "forced ciphersuite not allowed with this protocol version\n" );
            ret = 2;
            goto usage;
        }
        if( opt.min_version != -1 &&
            ciphersuite_info->max_minor_ver < opt.min_version )
        {
            mbedtls_printf( "forced ciphersuite not allowed with this protocol version\n" );
            ret = 2;
            goto usage;
        }

        /* If we select a version that's not supported by
         * this suite, then there will be no common ciphersuite... */
        if( opt.max_version == -1 ||
            opt.max_version > ciphersuite_info->max_minor_ver )
        {
            opt.max_version = ciphersuite_info->max_minor_ver;
        }
        if( opt.min_version < ciphersuite_info->min_minor_ver )
        {
            opt.min_version = ciphersuite_info->min_minor_ver;
            /* DTLS starts with TLS 1.1 */
            if( opt.transport == MBEDTLS_SSL_TRANSPORT_DATAGRAM &&
                opt.min_version < MBEDTLS_SSL_MINOR_VERSION_2 )
                opt.min_version = MBEDTLS_SSL_MINOR_VERSION_2;
        }

        /* Enable RC4 if needed and not explicitly disabled */
        if( ciphersuite_info->cipher == MBEDTLS_CIPHER_ARC4_128 )
        {
            if( opt.arc4 == MBEDTLS_SSL_ARC4_DISABLED )
            {
                mbedtls_printf("forced RC4 ciphersuite with RC4 disabled\n");
                ret = 2;
                goto usage;
            }

            opt.arc4 = MBEDTLS_SSL_ARC4_ENABLED;
        }
    }

    if( opt.version_suites != NULL )
    {
        const char *name[4] = { 0 };

        /* Parse 4-element coma-separated list */
        for( i = 0, p = (char *) opt.version_suites;
             i < 4 && *p != '\0';
             i++ )
        {
            name[i] = p;

            /* Terminate the current string and move on to next one */
            while( *p != ',' && *p != '\0' )
                p++;
            if( *p == ',' )
                *p++ = '\0';
        }

        if( i != 4 )
        {
            mbedtls_printf( "too few values for version_suites\n" );
            ret = 1;
            goto exit;
        }

        memset( version_suites, 0, sizeof( version_suites ) );

        /* Get the suites identifiers from their name */
        for( i = 0; i < 4; i++ )
        {
            version_suites[i][0] = mbedtls_ssl_get_ciphersuite_id( name[i] );

            if( version_suites[i][0] == 0 )
            {
                mbedtls_printf( "unknown ciphersuite: '%s'\n", name[i] );
                ret = 2;
                goto usage;
            }
        }
    }

#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
    /*
     * Unhexify the pre-shared key and parse the list if any given
     */
    if( unhexify( psk, opt.psk, &psk_len ) != 0 )
    {
        mbedtls_printf( "pre-shared key not valid hex\n" );
        goto exit;
    }

    if( opt.psk_list != NULL )
    {
        if( ( psk_info = psk_parse( opt.psk_list ) ) == NULL )
        {
            mbedtls_printf( "psk_list invalid" );
            goto exit;
        }
    }
#endif /* MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED */

#if defined(MBEDTLS_ECP_C)
    if( opt.curves != NULL )
    {
        p = (char *) opt.curves;
        i = 0;

        if( strcmp( p, "none" ) == 0 )
        {
            curve_list[0] = MBEDTLS_ECP_DP_NONE;
        }
        else if( strcmp( p, "default" ) != 0 )
        {
            /* Leave room for a final NULL in curve list */
            while( i < CURVE_LIST_SIZE - 1 && *p != '\0' )
            {
                q = p;

                /* Terminate the current string */
                while( *p != ',' && *p != '\0' )
                    p++;
                if( *p == ',' )
                    *p++ = '\0';

                if( ( curve_cur = mbedtls_ecp_curve_info_from_name( q ) ) != NULL )
                {
                    curve_list[i++] = curve_cur->grp_id;
                }
                else
                {
                    mbedtls_printf( "unknown curve %s\n", q );
                    mbedtls_printf( "supported curves: " );
                    for( curve_cur = mbedtls_ecp_curve_list();
                         curve_cur->grp_id != MBEDTLS_ECP_DP_NONE;
                         curve_cur++ )
                    {
                        mbedtls_printf( "%s ", curve_cur->name );
                    }
                    mbedtls_printf( "\n" );
                    goto exit;
                }
            }

            mbedtls_printf("Number of curves: %d\n", i );

            if( i == CURVE_LIST_SIZE - 1 && *p != '\0' )
            {
                mbedtls_printf( "curves list too long, maximum %d",
                                CURVE_LIST_SIZE - 1  );
                goto exit;
            }

            curve_list[i] = MBEDTLS_ECP_DP_NONE;
        }
    }
#endif /* MBEDTLS_ECP_C */

#if defined(MBEDTLS_SSL_ALPN)
    if( opt.alpn_string != NULL )
    {
        p = (char *) opt.alpn_string;
        i = 0;

        /* Leave room for a final NULL in alpn_list */
        while( i < ALPN_LIST_SIZE - 1 && *p != '\0' )
        {
            alpn_list[i++] = p;

            /* Terminate the current string and move on to next one */
            while( *p != ',' && *p != '\0' )
                p++;
            if( *p == ',' )
                *p++ = '\0';
        }
    }
#endif /* MBEDTLS_SSL_ALPN */

    /*
     * 0. Initialize the RNG and the session data
     */
    mbedtls_printf( "\n  . Seeding the random number generator..." );
    fflush( stdout );

    mbedtls_entropy_init( &entropy );
    if( ( ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func,
                                       &entropy, (const unsigned char *) pers,
                                       strlen( pers ) ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ctr_drbg_seed returned -0x%x\n",
                        -ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    /*
     * 1.1. Load the trusted CA
     */
    mbedtls_printf( "  . Loading the CA root certificate ..." );
    fflush( stdout );

#if defined(MBEDTLS_FS_IO)
    if( strlen( opt.ca_path ) )
        if( strcmp( opt.ca_path, "none" ) == 0 )
            ret = 0;
        else
            ret = mbedtls_x509_crt_parse_path( &cacert, opt.ca_path );
    else if( strlen( opt.ca_file ) )
        if( strcmp( opt.ca_file, "none" ) == 0 )
            ret = 0;
        else
            ret = mbedtls_x509_crt_parse_file( &cacert, opt.ca_file );
    else
#endif
#if defined(MBEDTLS_CERTS_C)
        for( i = 0; mbedtls_test_cas[i] != NULL; i++ )
        {
            ret = mbedtls_x509_crt_parse( &cacert,
                                  (const unsigned char *) mbedtls_test_cas[i],
                                  mbedtls_test_cas_len[i] );
            if( ret != 0 )
                break;
        }
#else
    {
        ret = 1;
        mbedtls_printf("MBEDTLS_CERTS_C not defined.");
    }
#endif
    if( ret < 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret );
        goto exit;
    }

    mbedtls_printf( " ok (%d skipped)\n", ret );

    /*
     * 1.2. Load own certificate and private key
     */
    mbedtls_printf( "  . Loading the server cert. and key..." );
    fflush( stdout );

#if defined(MBEDTLS_FS_IO)
    if( strlen( opt.crt_file ) && strcmp( opt.crt_file, "none" ) != 0 )
    {
        key_cert_init++;
        if( ( ret = mbedtls_x509_crt_parse_file( &srvcert, opt.crt_file ) ) != 0 )
        {
            mbedtls_printf( " failed\n  !  mbedtls_x509_crt_parse_file returned -0x%x\n\n",
                    -ret );
            goto exit;
        }
    }
    if( strlen( opt.key_file ) && strcmp( opt.key_file, "none" ) != 0 )
    {
        key_cert_init++;
        if( ( ret = mbedtls_pk_parse_keyfile( &pkey, opt.key_file, "" ) ) != 0 )
        {
            mbedtls_printf( " failed\n  !  mbedtls_pk_parse_keyfile returned -0x%x\n\n", -ret );
            goto exit;
        }
    }
    if( key_cert_init == 1 )
    {
        mbedtls_printf( " failed\n  !  crt_file without key_file or vice-versa\n\n" );
        goto exit;
    }

    if( strlen( opt.crt_file2 ) && strcmp( opt.crt_file2, "none" ) != 0 )
    {
        key_cert_init2++;
        if( ( ret = mbedtls_x509_crt_parse_file( &srvcert2, opt.crt_file2 ) ) != 0 )
        {
            mbedtls_printf( " failed\n  !  mbedtls_x509_crt_parse_file(2) returned -0x%x\n\n",
                    -ret );
            goto exit;
        }
    }
    if( strlen( opt.key_file2 ) && strcmp( opt.key_file2, "none" ) != 0 )
    {
        key_cert_init2++;
        if( ( ret = mbedtls_pk_parse_keyfile( &pkey2, opt.key_file2, "" ) ) != 0 )
        {
            mbedtls_printf( " failed\n  !  mbedtls_pk_parse_keyfile(2) returned -0x%x\n\n",
                            -ret );
            goto exit;
        }
    }
    if( key_cert_init2 == 1 )
    {
        mbedtls_printf( " failed\n  !  crt_file2 without key_file2 or vice-versa\n\n" );
        goto exit;
    }
#endif
    if( key_cert_init == 0 &&
        strcmp( opt.crt_file, "none" ) != 0 &&
        strcmp( opt.key_file, "none" ) != 0 &&
        key_cert_init2 == 0 &&
        strcmp( opt.crt_file2, "none" ) != 0 &&
        strcmp( opt.key_file2, "none" ) != 0 )
    {
#if !defined(MBEDTLS_CERTS_C)
        mbedtls_printf( "Not certificated or key provided, and \nMBEDTLS_CERTS_C not defined!\n" );
        goto exit;
#else
#if defined(MBEDTLS_RSA_C)
        if( ( ret = mbedtls_x509_crt_parse( &srvcert,
                                    (const unsigned char *) mbedtls_test_srv_crt_rsa,
                                    mbedtls_test_srv_crt_rsa_len ) ) != 0 )
        {
            mbedtls_printf( " failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n",
                            -ret );
            goto exit;
        }
        if( ( ret = mbedtls_pk_parse_key( &pkey,
                                  (const unsigned char *) mbedtls_test_srv_key_rsa,
                                  mbedtls_test_srv_key_rsa_len, NULL, 0 ) ) != 0 )
        {
            mbedtls_printf( " failed\n  !  mbedtls_pk_parse_key returned -0x%x\n\n",
                            -ret );
            goto exit;
        }
        key_cert_init = 2;
#endif /* MBEDTLS_RSA_C */
#if defined(MBEDTLS_ECDSA_C)
        if( ( ret = mbedtls_x509_crt_parse( &srvcert2,
                                    (const unsigned char *) mbedtls_test_srv_crt_ec,
                                    mbedtls_test_srv_crt_ec_len ) ) != 0 )
        {
            mbedtls_printf( " failed\n  !  x509_crt_parse2 returned -0x%x\n\n",
                            -ret );
            goto exit;
        }
        if( ( ret = mbedtls_pk_parse_key( &pkey2,
                                  (const unsigned char *) mbedtls_test_srv_key_ec,
                                  mbedtls_test_srv_key_ec_len, NULL, 0 ) ) != 0 )
        {
            mbedtls_printf( " failed\n  !  pk_parse_key2 returned -0x%x\n\n",
                            -ret );
            goto exit;
        }
        key_cert_init2 = 2;
#endif /* MBEDTLS_ECDSA_C */
#endif /* MBEDTLS_CERTS_C */
    }

    mbedtls_printf( " ok\n" );
#endif /* MBEDTLS_X509_CRT_PARSE_C */

#if defined(MBEDTLS_DHM_C) && defined(MBEDTLS_FS_IO)
    if( opt.dhm_file != NULL )
    {
        mbedtls_printf( "  . Loading DHM parameters..." );
        fflush( stdout );

        if( ( ret = mbedtls_dhm_parse_dhmfile( &dhm, opt.dhm_file ) ) != 0 )
        {
            mbedtls_printf( " failed\n  ! mbedtls_dhm_parse_dhmfile returned -0x%04X\n\n",
                     -ret );
            goto exit;
        }

        mbedtls_printf( " ok\n" );
    }
#endif

#if defined(SNI_OPTION)
    if( opt.sni != NULL )
    {
        mbedtls_printf( "  . Setting up SNI information..." );
        fflush( stdout );

        if( ( sni_info = sni_parse( opt.sni ) ) == NULL )
        {
            mbedtls_printf( " failed\n" );
            goto exit;
        }

        mbedtls_printf( " ok\n" );
    }
#endif /* SNI_OPTION */

    /*
     * 2. Setup the listening TCP socket
     */
    mbedtls_printf( "  . Bind on %s://%s:%s/ ...",
            opt.transport == MBEDTLS_SSL_TRANSPORT_STREAM ? "tcp" : "udp",
            opt.server_addr ? opt.server_addr : "*",
            opt.server_port );
    fflush( stdout );

    if( ( ret = mbedtls_net_bind( &listen_fd, opt.server_addr, opt.server_port,
                          opt.transport == MBEDTLS_SSL_TRANSPORT_STREAM ?
                          MBEDTLS_NET_PROTO_TCP : MBEDTLS_NET_PROTO_UDP ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_net_bind returned -0x%x\n\n", -ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    /*
     * 3. Setup stuff
     */
    mbedtls_printf( "  . Setting up the SSL/TLS structure..." );
    fflush( stdout );

    if( ( ret = mbedtls_ssl_config_defaults( &conf,
                    MBEDTLS_SSL_IS_SERVER,
                    opt.transport,
                    MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_config_defaults returned -0x%x\n\n", -ret );
        goto exit;
    }

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    /* The default algorithms profile disables SHA-1, but our tests still
       rely on it heavily. Hence we allow it here. A real-world server
       should use the default profile unless there is a good reason not to. */
    if( opt.allow_sha1 > 0 )
    {
        crt_profile_for_test.allowed_mds |= MBEDTLS_X509_ID_FLAG( MBEDTLS_MD_SHA1 );
        mbedtls_ssl_conf_cert_profile( &conf, &crt_profile_for_test );
        mbedtls_ssl_conf_sig_hashes( &conf, ssl_sig_hashes_for_test );
    }
#endif /* MBEDTLS_X509_CRT_PARSE_C */

    if( opt.auth_mode != DFL_AUTH_MODE )
        mbedtls_ssl_conf_authmode( &conf, opt.auth_mode );

    if( opt.cert_req_ca_list != DFL_CERT_REQ_CA_LIST )
        mbedtls_ssl_conf_cert_req_ca_list( &conf, opt.cert_req_ca_list );

#if defined(MBEDTLS_SSL_PROTO_DTLS)
    if( opt.hs_to_min != DFL_HS_TO_MIN || opt.hs_to_max != DFL_HS_TO_MAX )
        mbedtls_ssl_conf_handshake_timeout( &conf, opt.hs_to_min, opt.hs_to_max );

    if( opt.dgram_packing != DFL_DGRAM_PACKING )
        mbedtls_ssl_set_datagram_packing( &ssl, opt.dgram_packing );
#endif /* MBEDTLS_SSL_PROTO_DTLS */

#if defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
    if( ( ret = mbedtls_ssl_conf_max_frag_len( &conf, opt.mfl_code ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_conf_max_frag_len returned %d\n\n", ret );
        goto exit;
    };
#endif

#if defined(MBEDTLS_SSL_TRUNCATED_HMAC)
    if( opt.trunc_hmac != DFL_TRUNC_HMAC )
        mbedtls_ssl_conf_truncated_hmac( &conf, opt.trunc_hmac );
#endif

#if defined(MBEDTLS_SSL_EXTENDED_MASTER_SECRET)
    if( opt.extended_ms != DFL_EXTENDED_MS )
        mbedtls_ssl_conf_extended_master_secret( &conf, opt.extended_ms );
#endif

#if defined(MBEDTLS_SSL_ENCRYPT_THEN_MAC)
    if( opt.etm != DFL_ETM )
        mbedtls_ssl_conf_encrypt_then_mac( &conf, opt.etm );
#endif

#if defined(MBEDTLS_SSL_ALPN)
    if( opt.alpn_string != NULL )
        if( ( ret = mbedtls_ssl_conf_alpn_protocols( &conf, alpn_list ) ) != 0 )
        {
            mbedtls_printf( " failed\n  ! mbedtls_ssl_conf_alpn_protocols returned %d\n\n", ret );
            goto exit;
        }
#endif

    mbedtls_ssl_conf_rng( &conf, mbedtls_ctr_drbg_random, &ctr_drbg );
    mbedtls_ssl_conf_dbg( &conf, my_debug, stdout );

#if defined(MBEDTLS_SSL_CACHE_C)
    if( opt.cache_max != -1 )
        mbedtls_ssl_cache_set_max_entries( &cache, opt.cache_max );

    if( opt.cache_timeout != -1 )
        mbedtls_ssl_cache_set_timeout( &cache, opt.cache_timeout );

    mbedtls_ssl_conf_session_cache( &conf, &cache,
                                   mbedtls_ssl_cache_get,
                                   mbedtls_ssl_cache_set );
#endif

#if defined(MBEDTLS_SSL_SESSION_TICKETS)
    if( opt.tickets == MBEDTLS_SSL_SESSION_TICKETS_ENABLED )
    {
        if( ( ret = mbedtls_ssl_ticket_setup( &ticket_ctx,
                        mbedtls_ctr_drbg_random, &ctr_drbg,
                        MBEDTLS_CIPHER_AES_256_GCM,
                        opt.ticket_timeout ) ) != 0 )
        {
            mbedtls_printf( " failed\n  ! mbedtls_ssl_ticket_setup returned %d\n\n", ret );
            goto exit;
        }

        mbedtls_ssl_conf_session_tickets_cb( &conf,
                mbedtls_ssl_ticket_write,
                mbedtls_ssl_ticket_parse,
                &ticket_ctx );
    }
#endif

#if defined(MBEDTLS_SSL_PROTO_DTLS)
    if( opt.transport == MBEDTLS_SSL_TRANSPORT_DATAGRAM )
    {
#if defined(MBEDTLS_SSL_COOKIE_C)
        if( opt.cookies > 0 )
        {
            if( ( ret = mbedtls_ssl_cookie_setup( &cookie_ctx,
                                          mbedtls_ctr_drbg_random, &ctr_drbg ) ) != 0 )
            {
                mbedtls_printf( " failed\n  ! mbedtls_ssl_cookie_setup returned %d\n\n", ret );
                goto exit;
            }

            mbedtls_ssl_conf_dtls_cookies( &conf, mbedtls_ssl_cookie_write, mbedtls_ssl_cookie_check,
                                       &cookie_ctx );
        }
        else
#endif /* MBEDTLS_SSL_COOKIE_C */
#if defined(MBEDTLS_SSL_DTLS_HELLO_VERIFY)
        if( opt.cookies == 0 )
        {
            mbedtls_ssl_conf_dtls_cookies( &conf, NULL, NULL, NULL );
        }
        else
#endif /* MBEDTLS_SSL_DTLS_HELLO_VERIFY */
        {
            ; /* Nothing to do */
        }

#if defined(MBEDTLS_SSL_DTLS_ANTI_REPLAY)
        if( opt.anti_replay != DFL_ANTI_REPLAY )
            mbedtls_ssl_conf_dtls_anti_replay( &conf, opt.anti_replay );
#endif

#if defined(MBEDTLS_SSL_DTLS_BADMAC_LIMIT)
        if( opt.badmac_limit != DFL_BADMAC_LIMIT )
            mbedtls_ssl_conf_dtls_badmac_limit( &conf, opt.badmac_limit );
#endif
    }
#endif /* MBEDTLS_SSL_PROTO_DTLS */

    if( opt.force_ciphersuite[0] != DFL_FORCE_CIPHER )
        mbedtls_ssl_conf_ciphersuites( &conf, opt.force_ciphersuite );

#if defined(MBEDTLS_ARC4_C)
    if( opt.arc4 != DFL_ARC4 )
        mbedtls_ssl_conf_arc4_support( &conf, opt.arc4 );
#endif

    if( opt.version_suites != NULL )
    {
        mbedtls_ssl_conf_ciphersuites_for_version( &conf, version_suites[0],
                                          MBEDTLS_SSL_MAJOR_VERSION_3,
                                          MBEDTLS_SSL_MINOR_VERSION_0 );
        mbedtls_ssl_conf_ciphersuites_for_version( &conf, version_suites[1],
                                          MBEDTLS_SSL_MAJOR_VERSION_3,
                                          MBEDTLS_SSL_MINOR_VERSION_1 );
        mbedtls_ssl_conf_ciphersuites_for_version( &conf, version_suites[2],
                                          MBEDTLS_SSL_MAJOR_VERSION_3,
                                          MBEDTLS_SSL_MINOR_VERSION_2 );
        mbedtls_ssl_conf_ciphersuites_for_version( &conf, version_suites[3],
                                          MBEDTLS_SSL_MAJOR_VERSION_3,
                                          MBEDTLS_SSL_MINOR_VERSION_3 );
    }

    if( opt.allow_legacy != DFL_ALLOW_LEGACY )
        mbedtls_ssl_conf_legacy_renegotiation( &conf, opt.allow_legacy );
#if defined(MBEDTLS_SSL_RENEGOTIATION)
    mbedtls_ssl_conf_renegotiation( &conf, opt.renegotiation );

    if( opt.renego_delay != DFL_RENEGO_DELAY )
        mbedtls_ssl_conf_renegotiation_enforced( &conf, opt.renego_delay );

    if( opt.renego_period != DFL_RENEGO_PERIOD )
    {
        PUT_UINT64_BE( renego_period, opt.renego_period, 0 );
        mbedtls_ssl_conf_renegotiation_period( &conf, renego_period );
    }
#endif

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    if( strcmp( opt.ca_path, "none" ) != 0 &&
        strcmp( opt.ca_file, "none" ) != 0 )
    {
        mbedtls_ssl_conf_ca_chain( &conf, &cacert, NULL );
    }
    if( key_cert_init )
    {
        mbedtls_pk_context *pk = &pkey;
#if defined(MBEDTLS_SSL_ASYNC_PRIVATE)
        if( opt.async_private_delay1 >= 0 )
        {
            ret = ssl_async_set_key( &ssl_async_keys, &srvcert, pk, 0,
                                     opt.async_private_delay1 );
            if( ret < 0 )
            {
                mbedtls_printf( "  Test error: ssl_async_set_key failed (%d)\n",
                                ret );
                goto exit;
            }
            pk = NULL;
        }
#endif /* MBEDTLS_SSL_ASYNC_PRIVATE */
        if( ( ret = mbedtls_ssl_conf_own_cert( &conf, &srvcert, pk ) ) != 0 )
        {
            mbedtls_printf( " failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret );
            goto exit;
        }
    }
    if( key_cert_init2 )
    {
        mbedtls_pk_context *pk = &pkey2;
#if defined(MBEDTLS_SSL_ASYNC_PRIVATE)
        if( opt.async_private_delay2 >= 0 )
        {
            ret = ssl_async_set_key( &ssl_async_keys, &srvcert2, pk, 0,
                                     opt.async_private_delay2 );
            if( ret < 0 )
            {
                mbedtls_printf( "  Test error: ssl_async_set_key failed (%d)\n",
                                ret );
                goto exit;
            }
            pk = NULL;
        }
#endif /* MBEDTLS_SSL_ASYNC_PRIVATE */
        if( ( ret = mbedtls_ssl_conf_own_cert( &conf, &srvcert2, pk ) ) != 0 )
        {
            mbedtls_printf( " failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret );
            goto exit;
        }
    }

#if defined(MBEDTLS_SSL_ASYNC_PRIVATE)
    if( opt.async_operations[0] != '-' )
    {
        mbedtls_ssl_async_sign_t *sign = NULL;
        mbedtls_ssl_async_decrypt_t *decrypt = NULL;
        const char *r;
        for( r = opt.async_operations; *r; r++ )
        {
            switch( *r )
            {
            case 'd':
                decrypt = ssl_async_decrypt;
                break;
            case 's':
                sign = ssl_async_sign;
                break;
            }
        }
        ssl_async_keys.inject_error = ( opt.async_private_error < 0 ?
                                        - opt.async_private_error :
                                        opt.async_private_error );
        ssl_async_keys.f_rng = mbedtls_ctr_drbg_random;
        ssl_async_keys.p_rng = &ctr_drbg;
        mbedtls_ssl_conf_async_private_cb( &conf,
                                           sign,
                                           decrypt,
                                           ssl_async_resume,
                                           ssl_async_cancel,
                                           &ssl_async_keys );
    }
#endif /* MBEDTLS_SSL_ASYNC_PRIVATE */
#endif /* MBEDTLS_X509_CRT_PARSE_C */

#if defined(SNI_OPTION)
    if( opt.sni != NULL )
    {
        mbedtls_ssl_conf_sni( &conf, sni_callback, sni_info );
#if defined(MBEDTLS_SSL_ASYNC_PRIVATE)
        if( opt.async_private_delay2 >= 0 )
        {
            sni_entry *cur;
            for( cur = sni_info; cur != NULL; cur = cur->next )
            {
                ret = ssl_async_set_key( &ssl_async_keys,
                                         cur->cert, cur->key, 1,
                                         opt.async_private_delay2 );
                if( ret < 0 )
                {
                    mbedtls_printf( "  Test error: ssl_async_set_key failed (%d)\n",
                                    ret );
                    goto exit;
                }
                cur->key = NULL;
            }
        }
#endif /* MBEDTLS_SSL_ASYNC_PRIVATE */
    }
#endif

#if defined(MBEDTLS_ECP_C)
    if( opt.curves != NULL &&
        strcmp( opt.curves, "default" ) != 0 )
    {
        mbedtls_ssl_conf_curves( &conf, curve_list );
    }
#endif

#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
    if( strlen( opt.psk ) != 0 && strlen( opt.psk_identity ) != 0 )
    {
        ret = mbedtls_ssl_conf_psk( &conf, psk, psk_len,
                           (const unsigned char *) opt.psk_identity,
                           strlen( opt.psk_identity ) );
        if( ret != 0 )
        {
            mbedtls_printf( "  failed\n  mbedtls_ssl_conf_psk returned -0x%04X\n\n", - ret );
            goto exit;
        }
    }

    if( opt.psk_list != NULL )
        mbedtls_ssl_conf_psk_cb( &conf, psk_callback, psk_info );
#endif

#if defined(MBEDTLS_DHM_C)
    /*
     * Use different group than default DHM group
     */
#if defined(MBEDTLS_FS_IO)
    if( opt.dhm_file != NULL )
        ret = mbedtls_ssl_conf_dh_param_ctx( &conf, &dhm );
#endif
    if( ret != 0 )
    {
        mbedtls_printf( "  failed\n  mbedtls_ssl_conf_dh_param returned -0x%04X\n\n", - ret );
        goto exit;
    }
#endif

    if( opt.min_version != DFL_MIN_VERSION )
        mbedtls_ssl_conf_min_version( &conf, MBEDTLS_SSL_MAJOR_VERSION_3, opt.min_version );

    if( opt.max_version != DFL_MIN_VERSION )
        mbedtls_ssl_conf_max_version( &conf, MBEDTLS_SSL_MAJOR_VERSION_3, opt.max_version );

    if( ( ret = mbedtls_ssl_setup( &ssl, &conf ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_setup returned -0x%x\n\n", -ret );
        goto exit;
    }

    if( opt.nbio == 2 )
        mbedtls_ssl_set_bio( &ssl, &client_fd, my_send, my_recv, NULL );
    else
        mbedtls_ssl_set_bio( &ssl, &client_fd, mbedtls_net_send, mbedtls_net_recv,
                             opt.nbio == 0 ? mbedtls_net_recv_timeout : NULL );

#if defined(MBEDTLS_SSL_PROTO_DTLS)
    if( opt.dtls_mtu != DFL_DTLS_MTU )
        mbedtls_ssl_set_mtu( &ssl, opt.dtls_mtu );
#endif

#if defined(MBEDTLS_TIMING_C)
    mbedtls_ssl_set_timer_cb( &ssl, &timer, mbedtls_timing_set_delay,
                                            mbedtls_timing_get_delay );
#endif

    mbedtls_printf( " ok\n" );

reset:
#if !defined(_WIN32)
    if( received_sigterm )
    {
        mbedtls_printf( " interrupted by SIGTERM (not in net_accept())\n" );
        if( ret == MBEDTLS_ERR_NET_INVALID_CONTEXT )
            ret = 0;

        goto exit;
    }
#endif

    if( ret == MBEDTLS_ERR_SSL_CLIENT_RECONNECT )
    {
        mbedtls_printf( "  ! Client initiated reconnection from same port\n" );
        goto handshake;
    }

#ifdef MBEDTLS_ERROR_C
    if( ret != 0 )
    {
        char error_buf[100];
        mbedtls_strerror( ret, error_buf, 100 );
        mbedtls_printf("Last error was: %d - %s\n\n", ret, error_buf );
    }
#endif

    mbedtls_net_free( &client_fd );

    mbedtls_ssl_session_reset( &ssl );

    /*
     * 3. Wait until a client connects
     */
    mbedtls_printf( "  . Waiting for a remote connection ..." );
    fflush( stdout );

    if( ( ret = mbedtls_net_accept( &listen_fd, &client_fd,
                    client_ip, sizeof( client_ip ), &cliip_len ) ) != 0 )
    {
#if !defined(_WIN32)
        if( received_sigterm )
        {
            mbedtls_printf( " interrupted by SIGTERM (in net_accept())\n" );
            if( ret == MBEDTLS_ERR_NET_ACCEPT_FAILED )
                ret = 0;

            goto exit;
        }
#endif

        mbedtls_printf( " failed\n  ! mbedtls_net_accept returned -0x%x\n\n", -ret );
        goto exit;
    }

    if( opt.nbio > 0 )
        ret = mbedtls_net_set_nonblock( &client_fd );
    else
        ret = mbedtls_net_set_block( &client_fd );
    if( ret != 0 )
    {
        mbedtls_printf( " failed\n  ! net_set_(non)block() returned -0x%x\n\n", -ret );
        goto exit;
    }

    mbedtls_ssl_conf_read_timeout( &conf, opt.read_timeout );

#if defined(MBEDTLS_SSL_DTLS_HELLO_VERIFY)
    if( opt.transport == MBEDTLS_SSL_TRANSPORT_DATAGRAM )
    {
        if( ( ret = mbedtls_ssl_set_client_transport_id( &ssl,
                        client_ip, cliip_len ) ) != 0 )
        {
            mbedtls_printf( " failed\n  ! mbedtls_ssl_set_client_transport_id() returned -0x%x\n\n",
                            -ret );
            goto exit;
        }
    }
#endif /* MBEDTLS_SSL_DTLS_HELLO_VERIFY */

#if defined(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED)
    if( opt.ecjpake_pw != DFL_ECJPAKE_PW )
    {
        if( ( ret = mbedtls_ssl_set_hs_ecjpake_password( &ssl,
                        (const unsigned char *) opt.ecjpake_pw,
                                        strlen( opt.ecjpake_pw ) ) ) != 0 )
        {
            mbedtls_printf( " failed\n  ! mbedtls_ssl_set_hs_ecjpake_password returned %d\n\n", ret );
            goto exit;
        }
    }
#endif

    mbedtls_printf( " ok\n" );

    /*
     * 4. Handshake
     */
handshake:
    mbedtls_printf( "  . Performing the SSL/TLS handshake..." );
    fflush( stdout );

    while( ( ret = mbedtls_ssl_handshake( &ssl ) ) != 0 )
    {
#if defined(MBEDTLS_SSL_ASYNC_PRIVATE)
        if( ret == MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS &&
            ssl_async_keys.inject_error == SSL_ASYNC_INJECT_ERROR_CANCEL )
        {
            mbedtls_printf( " cancelling on injected error\n" );
            break;
        }
#endif /* MBEDTLS_SSL_ASYNC_PRIVATE */

        if( ! mbedtls_status_is_ssl_in_progress( ret ) )
            break;

        /* For event-driven IO, wait for socket to become available */
        if( opt.event == 1 /* level triggered IO */ )
        {
#if defined(MBEDTLS_TIMING_C)
            ret = idle( &client_fd, &timer, ret );
#else
            ret = idle( &client_fd, ret );
#endif
            if( ret != 0 )
                goto reset;
        }
    }

    if( ret == MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED )
    {
        mbedtls_printf( " hello verification requested\n" );
        ret = 0;
        goto reset;
    }
    else if( ret != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret );

#if defined(MBEDTLS_X509_CRT_PARSE_C)
        if( ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED )
        {
            char vrfy_buf[512];
            flags = mbedtls_ssl_get_verify_result( &ssl );

            mbedtls_x509_crt_verify_info( vrfy_buf, sizeof( vrfy_buf ), "  ! ", flags );

            mbedtls_printf( "%s\n", vrfy_buf );
        }
#endif

#if defined(MBEDTLS_SSL_ASYNC_PRIVATE)
        if( opt.async_private_error < 0 )
            /* Injected error only the first time round, to test reset */
            ssl_async_keys.inject_error = SSL_ASYNC_INJECT_ERROR_NONE;
#endif
        goto reset;
    }
    else /* ret == 0 */
    {
        mbedtls_printf( " ok\n    [ Protocol is %s ]\n    [ Ciphersuite is %s ]\n",
                mbedtls_ssl_get_version( &ssl ), mbedtls_ssl_get_ciphersuite( &ssl ) );
    }

    if( ( ret = mbedtls_ssl_get_record_expansion( &ssl ) ) >= 0 )
        mbedtls_printf( "    [ Record expansion is %d ]\n", ret );
    else
        mbedtls_printf( "    [ Record expansion is unknown (compression) ]\n" );

#if defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
    mbedtls_printf( "    [ Maximum fragment length is %u ]\n",
                    (unsigned int) mbedtls_ssl_get_max_frag_len( &ssl ) );
#endif

#if defined(MBEDTLS_SSL_ALPN)
    if( opt.alpn_string != NULL )
    {
        const char *alp = mbedtls_ssl_get_alpn_protocol( &ssl );
        mbedtls_printf( "    [ Application Layer Protocol is %s ]\n",
                alp ? alp : "(none)" );
    }
#endif

#if defined(MBEDTLS_X509_CRT_PARSE_C)
    /*
     * 5. Verify the client certificate
     */
    mbedtls_printf( "  . Verifying peer X.509 certificate..." );

    if( ( flags = mbedtls_ssl_get_verify_result( &ssl ) ) != 0 )
    {
        char vrfy_buf[512];

        mbedtls_printf( " failed\n" );

        mbedtls_x509_crt_verify_info( vrfy_buf, sizeof( vrfy_buf ), "  ! ", flags );

        mbedtls_printf( "%s\n", vrfy_buf );
    }
    else
        mbedtls_printf( " ok\n" );

    if( mbedtls_ssl_get_peer_cert( &ssl ) != NULL )
    {
        char crt_buf[512];

        mbedtls_printf( "  . Peer certificate information    ...\n" );
        mbedtls_x509_crt_info( crt_buf, sizeof( crt_buf ), "      ",
                       mbedtls_ssl_get_peer_cert( &ssl ) );
        mbedtls_printf( "%s\n", crt_buf );
    }
#endif /* MBEDTLS_X509_CRT_PARSE_C */

    if( opt.exchanges == 0 )
        goto close_notify;

    exchanges_left = opt.exchanges;
data_exchange:
    /*
     * 6. Read the HTTP Request
     */
    mbedtls_printf( "  < Read from client:" );
    fflush( stdout );

    /*
     * TLS and DTLS need different reading styles (stream vs datagram)
     */
    if( opt.transport == MBEDTLS_SSL_TRANSPORT_STREAM )
    {
        do
        {
            int terminated = 0;
            len = opt.buffer_size - 1;
            memset( buf, 0, opt.buffer_size );
            ret = mbedtls_ssl_read( &ssl, buf, len );

            if( mbedtls_status_is_ssl_in_progress( ret ) )
            {
                if( opt.event == 1 /* level triggered IO */ )
                {
#if defined(MBEDTLS_TIMING_C)
                    idle( &client_fd, &timer, ret );
#else
                    idle( &client_fd, ret );
#endif
                }

                continue;
            }

            if( ret <= 0 )
            {
                switch( ret )
                {
                    case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                        mbedtls_printf( " connection was closed gracefully\n" );
                        goto close_notify;

                    case 0:
                    case MBEDTLS_ERR_NET_CONN_RESET:
                        mbedtls_printf( " connection was reset by peer\n" );
                        ret = MBEDTLS_ERR_NET_CONN_RESET;
                        goto reset;

                    default:
                        mbedtls_printf( " mbedtls_ssl_read returned -0x%x\n", -ret );
                        goto reset;
                }
            }

            if( mbedtls_ssl_get_bytes_avail( &ssl ) == 0 )
            {
                len = ret;
                buf[len] = '\0';
                mbedtls_printf( " %d bytes read\n\n%s\n", len, (char *) buf );

                /* End of message should be detected according to the syntax of the
                 * application protocol (eg HTTP), just use a dummy test here. */
                if( buf[len - 1] == '\n' )
                    terminated = 1;
            }
            else
            {
                int extra_len, ori_len;
                unsigned char *larger_buf;

                ori_len = ret;
                extra_len = (int) mbedtls_ssl_get_bytes_avail( &ssl );

                larger_buf = mbedtls_calloc( 1, ori_len + extra_len + 1 );
                if( larger_buf == NULL )
                {
                    mbedtls_printf( "  ! memory allocation failed\n" );
                    ret = 1;
                    goto reset;
                }

                memset( larger_buf, 0, ori_len + extra_len );
                memcpy( larger_buf, buf, ori_len );

                /* This read should never fail and get the whole cached data */
                ret = mbedtls_ssl_read( &ssl, larger_buf + ori_len, extra_len );
                if( ret != extra_len ||
                    mbedtls_ssl_get_bytes_avail( &ssl ) != 0 )
                {
                    mbedtls_printf( "  ! mbedtls_ssl_read failed on cached data\n" );
                    ret = 1;
                    goto reset;
                }

                larger_buf[ori_len + extra_len] = '\0';
                mbedtls_printf( " %u bytes read (%u + %u)\n\n%s\n",
                        ori_len + extra_len, ori_len, extra_len,
                        (char *) larger_buf );

                /* End of message should be detected according to the syntax of the
                 * application protocol (eg HTTP), just use a dummy test here. */
                if( larger_buf[ori_len + extra_len - 1] == '\n' )
                    terminated = 1;

                mbedtls_free( larger_buf );
            }

            if( terminated )
            {
                ret = 0;
                break;
            }
        }
        while( 1 );
    }
    else /* Not stream, so datagram */
    {
        len = opt.buffer_size - 1;
        memset( buf, 0, opt.buffer_size );

        do
        {
            /* Without the call to `mbedtls_ssl_check_pending`, it might
             * happen that the client sends application data in the same
             * datagram as the Finished message concluding the handshake.
             * In this case, the application data would be ready to be
             * processed while the underlying transport wouldn't signal
             * any further incoming data.
             *
             * See the test 'Event-driven I/O: session-id resume, UDP packing'
             * in tests/ssl-opt.sh.
             */

            /* For event-driven IO, wait for socket to become available */
            if( mbedtls_ssl_check_pending( &ssl ) == 0 &&
                opt.event == 1 /* level triggered IO */ )
            {
#if defined(MBEDTLS_TIMING_C)
                idle( &client_fd, &timer, MBEDTLS_ERR_SSL_WANT_READ );
#else
                idle( &client_fd, MBEDTLS_ERR_SSL_WANT_READ );
#endif
            }

            ret = mbedtls_ssl_read( &ssl, buf, len );

            /* Note that even if `mbedtls_ssl_check_pending` returns true,
             * it can happen that the subsequent call to `mbedtls_ssl_read`
             * returns `MBEDTLS_ERR_SSL_WANT_READ`, because the pending messages
             * might be discarded (e.g. because they are retransmissions). */
        }
        while( mbedtls_status_is_ssl_in_progress( ret ) );

        if( ret <= 0 )
        {
            switch( ret )
            {
                case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                    mbedtls_printf( " connection was closed gracefully\n" );
                    ret = 0;
                    goto close_notify;

                default:
                    mbedtls_printf( " mbedtls_ssl_read returned -0x%x\n", -ret );
                    goto reset;
            }
        }

        len = ret;
        buf[len] = '\0';
        mbedtls_printf( " %d bytes read\n\n%s", len, (char *) buf );
        ret = 0;
    }

    /*
     * 7a. Request renegotiation while client is waiting for input from us.
     * (only on the first exchange, to be able to test retransmission)
     */
#if defined(MBEDTLS_SSL_RENEGOTIATION)
    if( opt.renegotiate && exchanges_left == opt.exchanges )
    {
        mbedtls_printf( "  . Requestion renegotiation..." );
        fflush( stdout );

        while( ( ret = mbedtls_ssl_renegotiate( &ssl ) ) != 0 )
        {
            if( ! mbedtls_status_is_ssl_in_progress( ret ) )
            {
                mbedtls_printf( " failed\n  ! mbedtls_ssl_renegotiate returned %d\n\n", ret );
                goto reset;
            }

            /* For event-driven IO, wait for socket to become available */
            if( opt.event == 1 /* level triggered IO */ )
            {
#if defined(MBEDTLS_TIMING_C)
                idle( &client_fd, &timer, ret );
#else
                idle( &client_fd, ret );
#endif
            }
        }

        mbedtls_printf( " ok\n" );
    }
#endif /* MBEDTLS_SSL_RENEGOTIATION */

    /*
     * 7. Write the 200 Response
     */
    mbedtls_printf( "  > Write to client:" );
    fflush( stdout );

    len = sprintf( (char *) buf, HTTP_RESPONSE,
                   mbedtls_ssl_get_ciphersuite( &ssl ) );

    /* Add padding to the response to reach opt.response_size in length */
    if( opt.response_size != DFL_RESPONSE_SIZE &&
        len < opt.response_size )
    {
        memset( buf + len, 'B', opt.response_size - len );
        len += opt.response_size - len;
    }

    /* Truncate if response size is smaller than the "natural" size */
    if( opt.response_size != DFL_RESPONSE_SIZE &&
        len > opt.response_size )
    {
        len = opt.response_size;

        /* Still end with \r\n unless that's really not possible */
        if( len >= 2 ) buf[len - 2] = '\r';
        if( len >= 1 ) buf[len - 1] = '\n';
    }

    if( opt.transport == MBEDTLS_SSL_TRANSPORT_STREAM )
    {
        for( written = 0, frags = 0; written < len; written += ret, frags++ )
        {
            while( ( ret = mbedtls_ssl_write( &ssl, buf + written, len - written ) )
                           <= 0 )
            {
                if( ret == MBEDTLS_ERR_NET_CONN_RESET )
                {
                    mbedtls_printf( " failed\n  ! peer closed the connection\n\n" );
                    goto reset;
                }

                if( ! mbedtls_status_is_ssl_in_progress( ret ) )
                {
                    mbedtls_printf( " failed\n  ! mbedtls_ssl_write returned %d\n\n", ret );
                    goto reset;
                }

                /* For event-driven IO, wait for socket to become available */
                if( opt.event == 1 /* level triggered IO */ )
                {
#if defined(MBEDTLS_TIMING_C)
                    idle( &client_fd, &timer, ret );
#else
                    idle( &client_fd, ret );
#endif
                }
            }
        }
    }
    else /* Not stream, so datagram */
    {
        while( 1 )
        {
            ret = mbedtls_ssl_write( &ssl, buf, len );

            if( ! mbedtls_status_is_ssl_in_progress( ret ) )
                break;

            /* For event-driven IO, wait for socket to become available */
            if( opt.event == 1 /* level triggered IO */ )
            {
#if defined(MBEDTLS_TIMING_C)
                idle( &client_fd, &timer, ret );
#else
                idle( &client_fd, ret );
#endif
            }
        }

        if( ret < 0 )
        {
            mbedtls_printf( " failed\n  ! mbedtls_ssl_write returned %d\n\n", ret );
            goto reset;
        }

        frags = 1;
        written = ret;
    }

    buf[written] = '\0';
    mbedtls_printf( " %d bytes written in %d fragments\n\n%s\n", written, frags, (char *) buf );
    ret = 0;

    /*
     * 7b. Continue doing data exchanges?
     */
    if( --exchanges_left > 0 )
        goto data_exchange;

    /*
     * 8. Done, cleanly close the connection
     */
close_notify:
    mbedtls_printf( "  . Closing the connection..." );

    /* No error checking, the connection might be closed already */
    do ret = mbedtls_ssl_close_notify( &ssl );
    while( ret == MBEDTLS_ERR_SSL_WANT_WRITE );
    ret = 0;

    mbedtls_printf( " done\n" );

    goto reset;

    /*
     * Cleanup and exit
     */
exit:
#ifdef MBEDTLS_ERROR_C
    if( ret != 0 )
    {
        char error_buf[100];
        mbedtls_strerror( ret, error_buf, 100 );
        mbedtls_printf("Last error was: -0x%X - %s\n\n", -ret, error_buf );
    }
#endif

    mbedtls_printf( "  . Cleaning up..." );
    fflush( stdout );

    mbedtls_net_free( &client_fd );
    mbedtls_net_free( &listen_fd );

#if defined(MBEDTLS_DHM_C) && defined(MBEDTLS_FS_IO)
    mbedtls_dhm_free( &dhm );
#endif
#if defined(MBEDTLS_X509_CRT_PARSE_C)
    mbedtls_x509_crt_free( &cacert );
    mbedtls_x509_crt_free( &srvcert );
    mbedtls_pk_free( &pkey );
    mbedtls_x509_crt_free( &srvcert2 );
    mbedtls_pk_free( &pkey2 );
#endif
#if defined(MBEDTLS_SSL_ASYNC_PRIVATE)
    for( i = 0; (size_t) i < ssl_async_keys.slots_used; i++ )
    {
        if( ssl_async_keys.slots[i].pk_owned )
        {
            mbedtls_pk_free( ssl_async_keys.slots[i].pk );
            mbedtls_free( ssl_async_keys.slots[i].pk );
            ssl_async_keys.slots[i].pk = NULL;
        }
    }
#endif
#if defined(SNI_OPTION)
    sni_free( sni_info );
#endif
#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
    psk_free( psk_info );
#endif
#if defined(MBEDTLS_DHM_C) && defined(MBEDTLS_FS_IO)
    mbedtls_dhm_free( &dhm );
#endif

    mbedtls_ssl_free( &ssl );
    mbedtls_ssl_config_free( &conf );
    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );

#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_cache_free( &cache );
#endif
#if defined(MBEDTLS_SSL_SESSION_TICKETS)
    mbedtls_ssl_ticket_free( &ticket_ctx );
#endif
#if defined(MBEDTLS_SSL_COOKIE_C)
    mbedtls_ssl_cookie_free( &cookie_ctx );
#endif

    mbedtls_free( buf );

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#if defined(MBEDTLS_MEMORY_DEBUG)
    mbedtls_memory_buffer_alloc_status();
#endif
    mbedtls_memory_buffer_alloc_free();
#endif

    mbedtls_printf( " done.\n" );

#if defined(_WIN32)
    mbedtls_printf( "  + Press Enter to exit this program.\n" );
    fflush( stdout ); getchar();
#endif

    // Shell can not handle large exit numbers -> 1 for errors
    if( ret < 0 )
        ret = 1;

    return( ret );
}
#endif /* MBEDTLS_BIGNUM_C && MBEDTLS_ENTROPY_C && MBEDTLS_SSL_TLS_C &&
          MBEDTLS_SSL_SRV_C && MBEDTLS_NET_C && MBEDTLS_RSA_C &&
          MBEDTLS_CTR_DRBG_C */

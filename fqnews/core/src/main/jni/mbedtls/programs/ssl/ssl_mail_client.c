/*
 *  SSL client for SMTP servers
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

/* Enable definition of gethostname() even when compiling with -std=c99. Must
 * be set before config.h, which pulls in glibc's features.h indirectly.
 * Harmless on other platforms. */
#define _POSIX_C_SOURCE 200112L

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
#define mbedtls_time            time
#define mbedtls_time_t          time_t
#define mbedtls_fprintf         fprintf
#define mbedtls_printf          printf
#define mbedtls_exit            exit
#define MBEDTLS_EXIT_SUCCESS    EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE    EXIT_FAILURE
#endif /* MBEDTLS_PLATFORM_C */

#if !defined(MBEDTLS_BIGNUM_C) || !defined(MBEDTLS_ENTROPY_C) ||  \
    !defined(MBEDTLS_SSL_TLS_C) || !defined(MBEDTLS_SSL_CLI_C) || \
    !defined(MBEDTLS_NET_C) || !defined(MBEDTLS_RSA_C) ||         \
    !defined(MBEDTLS_CTR_DRBG_C) || !defined(MBEDTLS_X509_CRT_PARSE_C) || \
    !defined(MBEDTLS_FS_IO)
int main( void )
{
    mbedtls_printf("MBEDTLS_BIGNUM_C and/or MBEDTLS_ENTROPY_C and/or "
           "MBEDTLS_SSL_TLS_C and/or MBEDTLS_SSL_CLI_C and/or "
           "MBEDTLS_NET_C and/or MBEDTLS_RSA_C and/or "
           "MBEDTLS_CTR_DRBG_C and/or MBEDTLS_X509_CRT_PARSE_C "
           "not defined.\n");
    return( 0 );
}
#else

#include "mbedtls/base64.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"

#include <stdlib.h>
#include <string.h>

#if !defined(_MSC_VER) || defined(EFIX64) || defined(EFI32)
#include <unistd.h>
#else
#include <io.h>
#endif

#if defined(_WIN32) || defined(_WIN32_WCE)
#include <winsock2.h>
#include <windows.h>

#if defined(_MSC_VER)
#if defined(_WIN32_WCE)
#pragma comment( lib, "ws2.lib" )
#else
#pragma comment( lib, "ws2_32.lib" )
#endif
#endif /* _MSC_VER */
#endif

#define DFL_SERVER_NAME         "localhost"
#define DFL_SERVER_PORT         "465"
#define DFL_USER_NAME           "user"
#define DFL_USER_PWD            "password"
#define DFL_MAIL_FROM           ""
#define DFL_MAIL_TO             ""
#define DFL_DEBUG_LEVEL         0
#define DFL_CA_FILE             ""
#define DFL_CRT_FILE            ""
#define DFL_KEY_FILE            ""
#define DFL_FORCE_CIPHER        0
#define DFL_MODE                0
#define DFL_AUTHENTICATION      0

#define MODE_SSL_TLS            0
#define MODE_STARTTLS           0

#if defined(MBEDTLS_BASE64_C)
#define USAGE_AUTH \
    "    authentication=%%d   default: 0 (disabled)\n"      \
    "    user_name=%%s        default: \"user\"\n"          \
    "    user_pwd=%%s         default: \"password\"\n"
#else
#define USAGE_AUTH \
    "    authentication options disabled. (Require MBEDTLS_BASE64_C)\n"
#endif /* MBEDTLS_BASE64_C */

#if defined(MBEDTLS_FS_IO)
#define USAGE_IO \
    "    ca_file=%%s          default: \"\" (pre-loaded)\n" \
    "    crt_file=%%s         default: \"\" (pre-loaded)\n" \
    "    key_file=%%s         default: \"\" (pre-loaded)\n"
#else
#define USAGE_IO \
    "    No file operations available (MBEDTLS_FS_IO not defined)\n"
#endif /* MBEDTLS_FS_IO */

#define USAGE \
    "\n usage: ssl_mail_client param=<>...\n"               \
    "\n acceptable parameters:\n"                           \
    "    server_name=%%s      default: localhost\n"         \
    "    server_port=%%d      default: 4433\n"              \
    "    debug_level=%%d      default: 0 (disabled)\n"      \
    "    mode=%%d             default: 0 (SSL/TLS) (1 for STARTTLS)\n"  \
    USAGE_AUTH                                              \
    "    mail_from=%%s        default: \"\"\n"              \
    "    mail_to=%%s          default: \"\"\n"              \
    USAGE_IO                                                \
    "    force_ciphersuite=<name>    default: all enabled\n"\
    " acceptable ciphersuite names:\n"

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
    const char *server_name;    /* hostname of the server (client only)     */
    const char *server_port;    /* port on which the ssl service runs       */
    int debug_level;            /* level of debugging                       */
    int authentication;         /* if authentication is required            */
    int mode;                   /* SSL/TLS (0) or STARTTLS (1)              */
    const char *user_name;      /* username to use for authentication       */
    const char *user_pwd;       /* password to use for authentication       */
    const char *mail_from;      /* E-Mail address to use as sender          */
    const char *mail_to;        /* E-Mail address to use as recipient       */
    const char *ca_file;        /* the file with the CA certificate(s)      */
    const char *crt_file;       /* the file with the client certificate     */
    const char *key_file;       /* the file with the client key             */
    int force_ciphersuite[2];   /* protocol/ciphersuite to use, or all      */
} opt;

static void my_debug( void *ctx, int level,
                      const char *file, int line,
                      const char *str )
{
    ((void) level);

    mbedtls_fprintf( (FILE *) ctx, "%s:%04d: %s", file, line, str );
    fflush(  (FILE *) ctx  );
}

static int do_handshake( mbedtls_ssl_context *ssl )
{
    int ret;
    uint32_t flags;
    unsigned char buf[1024];
    memset(buf, 0, 1024);

    /*
     * 4. Handshake
     */
    mbedtls_printf( "  . Performing the SSL/TLS handshake..." );
    fflush( stdout );

    while( ( ret = mbedtls_ssl_handshake( ssl ) ) != 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
#if defined(MBEDTLS_ERROR_C)
            mbedtls_strerror( ret, (char *) buf, 1024 );
#endif
            mbedtls_printf( " failed\n  ! mbedtls_ssl_handshake returned %d: %s\n\n", ret, buf );
            return( -1 );
        }
    }

    mbedtls_printf( " ok\n    [ Ciphersuite is %s ]\n",
            mbedtls_ssl_get_ciphersuite( ssl ) );

    /*
     * 5. Verify the server certificate
     */
    mbedtls_printf( "  . Verifying peer X.509 certificate..." );

    /* In real life, we probably want to bail out when ret != 0 */
    if( ( flags = mbedtls_ssl_get_verify_result( ssl ) ) != 0 )
    {
        char vrfy_buf[512];

        mbedtls_printf( " failed\n" );

        mbedtls_x509_crt_verify_info( vrfy_buf, sizeof( vrfy_buf ), "  ! ", flags );

        mbedtls_printf( "%s\n", vrfy_buf );
    }
    else
        mbedtls_printf( " ok\n" );

    mbedtls_printf( "  . Peer certificate information    ...\n" );
    mbedtls_x509_crt_info( (char *) buf, sizeof( buf ) - 1, "      ",
                   mbedtls_ssl_get_peer_cert( ssl ) );
    mbedtls_printf( "%s\n", buf );

    return( 0 );
}

static int write_ssl_data( mbedtls_ssl_context *ssl, unsigned char *buf, size_t len )
{
    int ret;

    mbedtls_printf("\n%s", buf);
    while( len && ( ret = mbedtls_ssl_write( ssl, buf, len ) ) <= 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            mbedtls_printf( " failed\n  ! mbedtls_ssl_write returned %d\n\n", ret );
            return -1;
        }
    }

    return( 0 );
}

static int write_ssl_and_get_response( mbedtls_ssl_context *ssl, unsigned char *buf, size_t len )
{
    int ret;
    unsigned char data[128];
    char code[4];
    size_t i, idx = 0;

    mbedtls_printf("\n%s", buf);
    while( len && ( ret = mbedtls_ssl_write( ssl, buf, len ) ) <= 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            mbedtls_printf( " failed\n  ! mbedtls_ssl_write returned %d\n\n", ret );
            return -1;
        }
    }

    do
    {
        len = sizeof( data ) - 1;
        memset( data, 0, sizeof( data ) );
        ret = mbedtls_ssl_read( ssl, data, len );

        if( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE )
            continue;

        if( ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY )
            return -1;

        if( ret <= 0 )
        {
            mbedtls_printf( "failed\n  ! mbedtls_ssl_read returned %d\n\n", ret );
            return -1;
        }

        mbedtls_printf("\n%s", data);
        len = ret;
        for( i = 0; i < len; i++ )
        {
            if( data[i] != '\n' )
            {
                if( idx < 4 )
                    code[ idx++ ] = data[i];
                continue;
            }

            if( idx == 4 && code[0] >= '0' && code[0] <= '9' && code[3] == ' ' )
            {
                code[3] = '\0';
                return atoi( code );
            }

            idx = 0;
        }
    }
    while( 1 );
}

static int write_and_get_response( mbedtls_net_context *sock_fd, unsigned char *buf, size_t len )
{
    int ret;
    unsigned char data[128];
    char code[4];
    size_t i, idx = 0;

    mbedtls_printf("\n%s", buf);
    if( len && ( ret = mbedtls_net_send( sock_fd, buf, len ) ) <= 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_write returned %d\n\n", ret );
            return -1;
    }

    do
    {
        len = sizeof( data ) - 1;
        memset( data, 0, sizeof( data ) );
        ret = mbedtls_net_recv( sock_fd, data, len );

        if( ret <= 0 )
        {
            mbedtls_printf( "failed\n  ! read returned %d\n\n", ret );
            return -1;
        }

        data[len] = '\0';
        mbedtls_printf("\n%s", data);
        len = ret;
        for( i = 0; i < len; i++ )
        {
            if( data[i] != '\n' )
            {
                if( idx < 4 )
                    code[ idx++ ] = data[i];
                continue;
            }

            if( idx == 4 && code[0] >= '0' && code[0] <= '9' && code[3] == ' ' )
            {
                code[3] = '\0';
                return atoi( code );
            }

            idx = 0;
        }
    }
    while( 1 );
}

int main( int argc, char *argv[] )
{
    int ret = 1, len;
    int exit_code = MBEDTLS_EXIT_FAILURE;
    mbedtls_net_context server_fd;
#if defined(MBEDTLS_BASE64_C)
    unsigned char base[1024];
    /* buf is used as the destination buffer for printing base with the format:
     * "%s\r\n". Hence, the size of buf should be at least the size of base
     * plus 2 bytes for the \r and \n characters.
     */
    unsigned char buf[sizeof( base ) + 2];
#else
    unsigned char buf[1024];
#endif
    char hostname[32];
    const char *pers = "ssl_mail_client";

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt cacert;
    mbedtls_x509_crt clicert;
    mbedtls_pk_context pkey;
    int i;
    size_t n;
    char *p, *q;
    const int *list;

    /*
     * Make sure memory references are valid in case we exit early.
     */
    mbedtls_net_init( &server_fd );
    mbedtls_ssl_init( &ssl );
    mbedtls_ssl_config_init( &conf );
    memset( &buf, 0, sizeof( buf ) );
    mbedtls_x509_crt_init( &cacert );
    mbedtls_x509_crt_init( &clicert );
    mbedtls_pk_init( &pkey );
    mbedtls_ctr_drbg_init( &ctr_drbg );

    if( argc == 0 )
    {
    usage:
        mbedtls_printf( USAGE );

        list = mbedtls_ssl_list_ciphersuites();
        while( *list )
        {
            mbedtls_printf("    %s\n", mbedtls_ssl_get_ciphersuite_name( *list ) );
            list++;
        }
        mbedtls_printf("\n");
        goto exit;
    }

    opt.server_name         = DFL_SERVER_NAME;
    opt.server_port         = DFL_SERVER_PORT;
    opt.debug_level         = DFL_DEBUG_LEVEL;
    opt.authentication      = DFL_AUTHENTICATION;
    opt.mode                = DFL_MODE;
    opt.user_name           = DFL_USER_NAME;
    opt.user_pwd            = DFL_USER_PWD;
    opt.mail_from           = DFL_MAIL_FROM;
    opt.mail_to             = DFL_MAIL_TO;
    opt.ca_file             = DFL_CA_FILE;
    opt.crt_file            = DFL_CRT_FILE;
    opt.key_file            = DFL_KEY_FILE;
    opt.force_ciphersuite[0]= DFL_FORCE_CIPHER;

    for( i = 1; i < argc; i++ )
    {
        p = argv[i];
        if( ( q = strchr( p, '=' ) ) == NULL )
            goto usage;
        *q++ = '\0';

        if( strcmp( p, "server_name" ) == 0 )
            opt.server_name = q;
        else if( strcmp( p, "server_port" ) == 0 )
            opt.server_port = q;
        else if( strcmp( p, "debug_level" ) == 0 )
        {
            opt.debug_level = atoi( q );
            if( opt.debug_level < 0 || opt.debug_level > 65535 )
                goto usage;
        }
        else if( strcmp( p, "authentication" ) == 0 )
        {
            opt.authentication = atoi( q );
            if( opt.authentication < 0 || opt.authentication > 1 )
                goto usage;
        }
        else if( strcmp( p, "mode" ) == 0 )
        {
            opt.mode = atoi( q );
            if( opt.mode < 0 || opt.mode > 1 )
                goto usage;
        }
        else if( strcmp( p, "user_name" ) == 0 )
            opt.user_name = q;
        else if( strcmp( p, "user_pwd" ) == 0 )
            opt.user_pwd = q;
        else if( strcmp( p, "mail_from" ) == 0 )
            opt.mail_from = q;
        else if( strcmp( p, "mail_to" ) == 0 )
            opt.mail_to = q;
        else if( strcmp( p, "ca_file" ) == 0 )
            opt.ca_file = q;
        else if( strcmp( p, "crt_file" ) == 0 )
            opt.crt_file = q;
        else if( strcmp( p, "key_file" ) == 0 )
            opt.key_file = q;
        else if( strcmp( p, "force_ciphersuite" ) == 0 )
        {
            opt.force_ciphersuite[0] = -1;

            opt.force_ciphersuite[0] = mbedtls_ssl_get_ciphersuite_id( q );

            if( opt.force_ciphersuite[0] <= 0 )
                goto usage;

            opt.force_ciphersuite[1] = 0;
        }
        else
            goto usage;
    }

    /*
     * 0. Initialize the RNG and the session data
     */
    mbedtls_printf( "\n  . Seeding the random number generator..." );
    fflush( stdout );

    mbedtls_entropy_init( &entropy );
    if( ( ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    /*
     * 1.1. Load the trusted CA
     */
    mbedtls_printf( "  . Loading the CA root certificate ..." );
    fflush( stdout );

#if defined(MBEDTLS_FS_IO)
    if( strlen( opt.ca_file ) )
        ret = mbedtls_x509_crt_parse_file( &cacert, opt.ca_file );
    else
#endif
#if defined(MBEDTLS_CERTS_C) && defined(MBEDTLS_PEM_PARSE_C)
        ret = mbedtls_x509_crt_parse( &cacert, (const unsigned char *) mbedtls_test_cas_pem,
                              mbedtls_test_cas_pem_len );
#else
    {
        mbedtls_printf("MBEDTLS_CERTS_C and/or MBEDTLS_PEM_PARSE_C not defined.");
        goto exit;
    }
#endif
    if( ret < 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_x509_crt_parse returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok (%d skipped)\n", ret );

    /*
     * 1.2. Load own certificate and private key
     *
     * (can be skipped if client authentication is not required)
     */
    mbedtls_printf( "  . Loading the client cert. and key..." );
    fflush( stdout );

#if defined(MBEDTLS_FS_IO)
    if( strlen( opt.crt_file ) )
        ret = mbedtls_x509_crt_parse_file( &clicert, opt.crt_file );
    else
#endif
#if defined(MBEDTLS_CERTS_C)
        ret = mbedtls_x509_crt_parse( &clicert, (const unsigned char *) mbedtls_test_cli_crt,
                              mbedtls_test_cli_crt_len );
#else
    {
        mbedtls_printf("MBEDTLS_CERTS_C not defined.");
        goto exit;
    }
#endif
    if( ret != 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_x509_crt_parse returned %d\n\n", ret );
        goto exit;
    }

#if defined(MBEDTLS_FS_IO)
    if( strlen( opt.key_file ) )
        ret = mbedtls_pk_parse_keyfile( &pkey, opt.key_file, "" );
    else
#endif
#if defined(MBEDTLS_CERTS_C) && defined(MBEDTLS_PEM_PARSE_C)
        ret = mbedtls_pk_parse_key( &pkey, (const unsigned char *) mbedtls_test_cli_key,
                mbedtls_test_cli_key_len, NULL, 0 );
#else
    {
        mbedtls_printf("MBEDTLS_CERTS_C or MBEDTLS_PEM_PARSE_C not defined.");
        goto exit;
    }
#endif
    if( ret != 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_pk_parse_key returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    /*
     * 2. Start the connection
     */
    mbedtls_printf( "  . Connecting to tcp/%s/%s...", opt.server_name,
                                                opt.server_port );
    fflush( stdout );

    if( ( ret = mbedtls_net_connect( &server_fd, opt.server_name,
                             opt.server_port, MBEDTLS_NET_PROTO_TCP ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_net_connect returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    /*
     * 3. Setup stuff
     */
    mbedtls_printf( "  . Setting up the SSL/TLS structure..." );
    fflush( stdout );

    if( ( ret = mbedtls_ssl_config_defaults( &conf,
                    MBEDTLS_SSL_IS_CLIENT,
                    MBEDTLS_SSL_TRANSPORT_STREAM,
                    MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret );
        goto exit;
    }

    /* OPTIONAL is not optimal for security,
     * but makes interop easier in this simplified example */
    mbedtls_ssl_conf_authmode( &conf, MBEDTLS_SSL_VERIFY_OPTIONAL );

    mbedtls_ssl_conf_rng( &conf, mbedtls_ctr_drbg_random, &ctr_drbg );
    mbedtls_ssl_conf_dbg( &conf, my_debug, stdout );

    if( opt.force_ciphersuite[0] != DFL_FORCE_CIPHER )
        mbedtls_ssl_conf_ciphersuites( &conf, opt.force_ciphersuite );

    mbedtls_ssl_conf_ca_chain( &conf, &cacert, NULL );
    if( ( ret = mbedtls_ssl_conf_own_cert( &conf, &clicert, &pkey ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret );
        goto exit;
    }

    if( ( ret = mbedtls_ssl_setup( &ssl, &conf ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret );
        goto exit;
    }

    if( ( ret = mbedtls_ssl_set_hostname( &ssl, opt.server_name ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_ssl_set_bio( &ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL );

    mbedtls_printf( " ok\n" );

    if( opt.mode == MODE_SSL_TLS )
    {
        if( do_handshake( &ssl ) != 0 )
            goto exit;

        mbedtls_printf( "  > Get header from server:" );
        fflush( stdout );

        ret = write_ssl_and_get_response( &ssl, buf, 0 );
        if( ret < 200 || ret > 299 )
        {
            mbedtls_printf( " failed\n  ! server responded with %d\n\n", ret );
            goto exit;
        }

        mbedtls_printf(" ok\n" );

        mbedtls_printf( "  > Write EHLO to server:" );
        fflush( stdout );

        gethostname( hostname, 32 );
        len = sprintf( (char *) buf, "EHLO %s\r\n", hostname );
        ret = write_ssl_and_get_response( &ssl, buf, len );
        if( ret < 200 || ret > 299 )
        {
            mbedtls_printf( " failed\n  ! server responded with %d\n\n", ret );
            goto exit;
        }
    }
    else
    {
        mbedtls_printf( "  > Get header from server:" );
        fflush( stdout );

        ret = write_and_get_response( &server_fd, buf, 0 );
        if( ret < 200 || ret > 299 )
        {
            mbedtls_printf( " failed\n  ! server responded with %d\n\n", ret );
            goto exit;
        }

        mbedtls_printf(" ok\n" );

        mbedtls_printf( "  > Write EHLO to server:" );
        fflush( stdout );

        gethostname( hostname, 32 );
        len = sprintf( (char *) buf, "EHLO %s\r\n", hostname );
        ret = write_and_get_response( &server_fd, buf, len );
        if( ret < 200 || ret > 299 )
        {
            mbedtls_printf( " failed\n  ! server responded with %d\n\n", ret );
            goto exit;
        }

        mbedtls_printf(" ok\n" );

        mbedtls_printf( "  > Write STARTTLS to server:" );
        fflush( stdout );

        gethostname( hostname, 32 );
        len = sprintf( (char *) buf, "STARTTLS\r\n" );
        ret = write_and_get_response( &server_fd, buf, len );
        if( ret < 200 || ret > 299 )
        {
            mbedtls_printf( " failed\n  ! server responded with %d\n\n", ret );
            goto exit;
        }

        mbedtls_printf(" ok\n" );

        if( do_handshake( &ssl ) != 0 )
            goto exit;
    }

#if defined(MBEDTLS_BASE64_C)
    if( opt.authentication )
    {
        mbedtls_printf( "  > Write AUTH LOGIN to server:" );
        fflush( stdout );

        len = sprintf( (char *) buf, "AUTH LOGIN\r\n" );
        ret = write_ssl_and_get_response( &ssl, buf, len );
        if( ret < 200 || ret > 399 )
        {
            mbedtls_printf( " failed\n  ! server responded with %d\n\n", ret );
            goto exit;
        }

        mbedtls_printf(" ok\n" );

        mbedtls_printf( "  > Write username to server: %s", opt.user_name );
        fflush( stdout );

        ret = mbedtls_base64_encode( base, sizeof( base ), &n, (const unsigned char *) opt.user_name,
                             strlen( opt.user_name ) );

        if( ret != 0 ) {
            mbedtls_printf( " failed\n  ! mbedtls_base64_encode returned %d\n\n", ret );
            goto exit;
        }
        len = sprintf( (char *) buf, "%s\r\n", base );
        ret = write_ssl_and_get_response( &ssl, buf, len );
        if( ret < 300 || ret > 399 )
        {
            mbedtls_printf( " failed\n  ! server responded with %d\n\n", ret );
            goto exit;
        }

        mbedtls_printf(" ok\n" );

        mbedtls_printf( "  > Write password to server: %s", opt.user_pwd );
        fflush( stdout );

        ret = mbedtls_base64_encode( base, sizeof( base ), &n, (const unsigned char *) opt.user_pwd,
                             strlen( opt.user_pwd ) );

        if( ret != 0 ) {
            mbedtls_printf( " failed\n  ! mbedtls_base64_encode returned %d\n\n", ret );
            goto exit;
        }
        len = sprintf( (char *) buf, "%s\r\n", base );
        ret = write_ssl_and_get_response( &ssl, buf, len );
        if( ret < 200 || ret > 399 )
        {
            mbedtls_printf( " failed\n  ! server responded with %d\n\n", ret );
            goto exit;
        }

        mbedtls_printf(" ok\n" );
    }
#endif

    mbedtls_printf( "  > Write MAIL FROM to server:" );
    fflush( stdout );

    len = sprintf( (char *) buf, "MAIL FROM:<%s>\r\n", opt.mail_from );
    ret = write_ssl_and_get_response( &ssl, buf, len );
    if( ret < 200 || ret > 299 )
    {
        mbedtls_printf( " failed\n  ! server responded with %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf(" ok\n" );

    mbedtls_printf( "  > Write RCPT TO to server:" );
    fflush( stdout );

    len = sprintf( (char *) buf, "RCPT TO:<%s>\r\n", opt.mail_to );
    ret = write_ssl_and_get_response( &ssl, buf, len );
    if( ret < 200 || ret > 299 )
    {
        mbedtls_printf( " failed\n  ! server responded with %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf(" ok\n" );

    mbedtls_printf( "  > Write DATA to server:" );
    fflush( stdout );

    len = sprintf( (char *) buf, "DATA\r\n" );
    ret = write_ssl_and_get_response( &ssl, buf, len );
    if( ret < 300 || ret > 399 )
    {
        mbedtls_printf( " failed\n  ! server responded with %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf(" ok\n" );

    mbedtls_printf( "  > Write content to server:" );
    fflush( stdout );

    len = sprintf( (char *) buf, "From: %s\r\nSubject: mbed TLS Test mail\r\n\r\n"
            "This is a simple test mail from the "
            "mbed TLS mail client example.\r\n"
            "\r\n"
            "Enjoy!", opt.mail_from );
    ret = write_ssl_data( &ssl, buf, len );

    len = sprintf( (char *) buf, "\r\n.\r\n");
    ret = write_ssl_and_get_response( &ssl, buf, len );
    if( ret < 200 || ret > 299 )
    {
        mbedtls_printf( " failed\n  ! server responded with %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf(" ok\n" );

    mbedtls_ssl_close_notify( &ssl );

    exit_code = MBEDTLS_EXIT_SUCCESS;

exit:

    mbedtls_net_free( &server_fd );
    mbedtls_x509_crt_free( &clicert );
    mbedtls_x509_crt_free( &cacert );
    mbedtls_pk_free( &pkey );
    mbedtls_ssl_free( &ssl );
    mbedtls_ssl_config_free( &conf );
    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );

#if defined(_WIN32)
    mbedtls_printf( "  + Press Enter to exit this program.\n" );
    fflush( stdout ); getchar();
#endif

    return( exit_code );
}
#endif /* MBEDTLS_BIGNUM_C && MBEDTLS_ENTROPY_C && MBEDTLS_SSL_TLS_C &&
          MBEDTLS_SSL_CLI_C && MBEDTLS_NET_C && MBEDTLS_RSA_C **
          MBEDTLS_CTR_DRBG_C */

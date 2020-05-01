/*
 *  SSL server demonstration program using pthread for handling multiple
 *  clients.
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
#define mbedtls_fprintf    fprintf
#define mbedtls_printf     printf
#define mbedtls_snprintf   snprintf
#define mbedtls_exit            exit
#define MBEDTLS_EXIT_SUCCESS    EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE    EXIT_FAILURE
#endif

#if !defined(MBEDTLS_BIGNUM_C) || !defined(MBEDTLS_CERTS_C) ||            \
    !defined(MBEDTLS_ENTROPY_C) || !defined(MBEDTLS_SSL_TLS_C) ||         \
    !defined(MBEDTLS_SSL_SRV_C) || !defined(MBEDTLS_NET_C) ||             \
    !defined(MBEDTLS_RSA_C) || !defined(MBEDTLS_CTR_DRBG_C) ||            \
    !defined(MBEDTLS_X509_CRT_PARSE_C) || !defined(MBEDTLS_FS_IO) ||      \
    !defined(MBEDTLS_THREADING_C) || !defined(MBEDTLS_THREADING_PTHREAD) || \
    !defined(MBEDTLS_PEM_PARSE_C)
int main( void )
{
    mbedtls_printf("MBEDTLS_BIGNUM_C and/or MBEDTLS_CERTS_C and/or MBEDTLS_ENTROPY_C "
           "and/or MBEDTLS_SSL_TLS_C and/or MBEDTLS_SSL_SRV_C and/or "
           "MBEDTLS_NET_C and/or MBEDTLS_RSA_C and/or "
           "MBEDTLS_CTR_DRBG_C and/or MBEDTLS_X509_CRT_PARSE_C and/or "
           "MBEDTLS_THREADING_C and/or MBEDTLS_THREADING_PTHREAD "
           "and/or MBEDTLS_PEM_PARSE_C not defined.\n");
    return( 0 );
}
#else

#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/ssl.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/error.h"

#if defined(MBEDTLS_SSL_CACHE_C)
#include "mbedtls/ssl_cache.h"
#endif

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include "mbedtls/memory_buffer_alloc.h"
#endif

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

#define HTTP_RESPONSE \
    "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n" \
    "<h2>mbed TLS Test Server</h2>\r\n" \
    "<p>Successful connection using: %s</p>\r\n"

#define DEBUG_LEVEL 0

#define MAX_NUM_THREADS 5

mbedtls_threading_mutex_t debug_mutex;

static void my_mutexed_debug( void *ctx, int level,
                      const char *file, int line,
                      const char *str )
{
    long int thread_id = (long int) pthread_self();

    mbedtls_mutex_lock( &debug_mutex );

    ((void) level);
    mbedtls_fprintf( (FILE *) ctx, "%s:%04d: [ #%ld ] %s",
                                    file, line, thread_id, str );
    fflush(  (FILE *) ctx  );

    mbedtls_mutex_unlock( &debug_mutex );
}

typedef struct {
    mbedtls_net_context client_fd;
    int thread_complete;
    const mbedtls_ssl_config *config;
} thread_info_t;

typedef struct {
    int active;
    thread_info_t   data;
    pthread_t       thread;
} pthread_info_t;

static thread_info_t    base_info;
static pthread_info_t   threads[MAX_NUM_THREADS];

static void *handle_ssl_connection( void *data )
{
    int ret, len;
    thread_info_t *thread_info = (thread_info_t *) data;
    mbedtls_net_context *client_fd = &thread_info->client_fd;
    long int thread_id = (long int) pthread_self();
    unsigned char buf[1024];
    mbedtls_ssl_context ssl;

    /* Make sure memory references are valid */
    mbedtls_ssl_init( &ssl );

    mbedtls_printf( "  [ #%ld ]  Setting up SSL/TLS data\n", thread_id );

    /*
     * 4. Get the SSL context ready
     */
    if( ( ret = mbedtls_ssl_setup( &ssl, thread_info->config ) ) != 0 )
    {
        mbedtls_printf( "  [ #%ld ]  failed: mbedtls_ssl_setup returned -0x%04x\n",
                thread_id, -ret );
        goto thread_exit;
    }

    mbedtls_ssl_set_bio( &ssl, client_fd, mbedtls_net_send, mbedtls_net_recv, NULL );

    /*
     * 5. Handshake
     */
    mbedtls_printf( "  [ #%ld ]  Performing the SSL/TLS handshake\n", thread_id );

    while( ( ret = mbedtls_ssl_handshake( &ssl ) ) != 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            mbedtls_printf( "  [ #%ld ]  failed: mbedtls_ssl_handshake returned -0x%04x\n",
                    thread_id, -ret );
            goto thread_exit;
        }
    }

    mbedtls_printf( "  [ #%ld ]  ok\n", thread_id );

    /*
     * 6. Read the HTTP Request
     */
    mbedtls_printf( "  [ #%ld ]  < Read from client\n", thread_id );

    do
    {
        len = sizeof( buf ) - 1;
        memset( buf, 0, sizeof( buf ) );
        ret = mbedtls_ssl_read( &ssl, buf, len );

        if( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE )
            continue;

        if( ret <= 0 )
        {
            switch( ret )
            {
                case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                    mbedtls_printf( "  [ #%ld ]  connection was closed gracefully\n",
                            thread_id );
                    goto thread_exit;

                case MBEDTLS_ERR_NET_CONN_RESET:
                    mbedtls_printf( "  [ #%ld ]  connection was reset by peer\n",
                            thread_id );
                    goto thread_exit;

                default:
                    mbedtls_printf( "  [ #%ld ]  mbedtls_ssl_read returned -0x%04x\n",
                            thread_id, -ret );
                    goto thread_exit;
            }
        }

        len = ret;
        mbedtls_printf( "  [ #%ld ]  %d bytes read\n=====\n%s\n=====\n",
                thread_id, len, (char *) buf );

        if( ret > 0 )
            break;
    }
    while( 1 );

    /*
     * 7. Write the 200 Response
     */
    mbedtls_printf( "  [ #%ld ]  > Write to client:\n", thread_id );

    len = sprintf( (char *) buf, HTTP_RESPONSE,
                   mbedtls_ssl_get_ciphersuite( &ssl ) );

    while( ( ret = mbedtls_ssl_write( &ssl, buf, len ) ) <= 0 )
    {
        if( ret == MBEDTLS_ERR_NET_CONN_RESET )
        {
            mbedtls_printf( "  [ #%ld ]  failed: peer closed the connection\n",
                    thread_id );
            goto thread_exit;
        }

        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            mbedtls_printf( "  [ #%ld ]  failed: mbedtls_ssl_write returned -0x%04x\n",
                    thread_id, ret );
            goto thread_exit;
        }
    }

    len = ret;
    mbedtls_printf( "  [ #%ld ]  %d bytes written\n=====\n%s\n=====\n",
            thread_id, len, (char *) buf );

    mbedtls_printf( "  [ #%ld ]  . Closing the connection...", thread_id );

    while( ( ret = mbedtls_ssl_close_notify( &ssl ) ) < 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ &&
            ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
            mbedtls_printf( "  [ #%ld ]  failed: mbedtls_ssl_close_notify returned -0x%04x\n",
                    thread_id, ret );
            goto thread_exit;
        }
    }

    mbedtls_printf( " ok\n" );

    ret = 0;

thread_exit:

#ifdef MBEDTLS_ERROR_C
    if( ret != 0 )
    {
        char error_buf[100];
        mbedtls_strerror( ret, error_buf, 100 );
        mbedtls_printf("  [ #%ld ]  Last error was: -0x%04x - %s\n\n",
               thread_id, -ret, error_buf );
    }
#endif

    mbedtls_net_free( client_fd );
    mbedtls_ssl_free( &ssl );

    thread_info->thread_complete = 1;

    return( NULL );
}

static int thread_create( mbedtls_net_context *client_fd )
{
    int ret, i;

    /*
     * Find in-active or finished thread slot
     */
    for( i = 0; i < MAX_NUM_THREADS; i++ )
    {
        if( threads[i].active == 0 )
            break;

        if( threads[i].data.thread_complete == 1 )
        {
            mbedtls_printf( "  [ main ]  Cleaning up thread %d\n", i );
            pthread_join(threads[i].thread, NULL );
            memset( &threads[i], 0, sizeof(pthread_info_t) );
            break;
        }
    }

    if( i == MAX_NUM_THREADS )
        return( -1 );

    /*
     * Fill thread-info for thread
     */
    memcpy( &threads[i].data, &base_info, sizeof(base_info) );
    threads[i].active = 1;
    memcpy( &threads[i].data.client_fd, client_fd, sizeof( mbedtls_net_context ) );

    if( ( ret = pthread_create( &threads[i].thread, NULL, handle_ssl_connection,
                                &threads[i].data ) ) != 0 )
    {
        return( ret );
    }

    return( 0 );
}

int main( void )
{
    int ret;
    mbedtls_net_context listen_fd, client_fd;
    const char pers[] = "ssl_pthread_server";

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt srvcert;
    mbedtls_x509_crt cachain;
    mbedtls_pk_context pkey;
#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
    unsigned char alloc_buf[100000];
#endif
#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_cache_context cache;
#endif

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
    mbedtls_memory_buffer_alloc_init( alloc_buf, sizeof(alloc_buf) );
#endif

#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_cache_init( &cache );
#endif

    mbedtls_x509_crt_init( &srvcert );
    mbedtls_x509_crt_init( &cachain );

    mbedtls_ssl_config_init( &conf );
    mbedtls_ctr_drbg_init( &ctr_drbg );
    memset( threads, 0, sizeof(threads) );
    mbedtls_net_init( &listen_fd );
    mbedtls_net_init( &client_fd );

    mbedtls_mutex_init( &debug_mutex );

    base_info.config = &conf;

    /*
     * We use only a single entropy source that is used in all the threads.
     */
    mbedtls_entropy_init( &entropy );

    /*
     * 1. Load the certificates and private RSA key
     */
    mbedtls_printf( "\n  . Loading the server cert. and key..." );
    fflush( stdout );

    /*
     * This demonstration program uses embedded test certificates.
     * Instead, you may want to use mbedtls_x509_crt_parse_file() to read the
     * server and CA certificates, as well as mbedtls_pk_parse_keyfile().
     */
    ret = mbedtls_x509_crt_parse( &srvcert, (const unsigned char *) mbedtls_test_srv_crt,
                          mbedtls_test_srv_crt_len );
    if( ret != 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_x509_crt_parse returned %d\n\n", ret );
        goto exit;
    }

    ret = mbedtls_x509_crt_parse( &cachain, (const unsigned char *) mbedtls_test_cas_pem,
                          mbedtls_test_cas_pem_len );
    if( ret != 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_x509_crt_parse returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_pk_init( &pkey );
    ret =  mbedtls_pk_parse_key( &pkey, (const unsigned char *) mbedtls_test_srv_key,
                         mbedtls_test_srv_key_len, NULL, 0 );
    if( ret != 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_pk_parse_key returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    /*
     * 1b. Seed the random number generator
     */
    mbedtls_printf( "  . Seeding the random number generator..." );

    if( ( ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        mbedtls_printf( " failed: mbedtls_ctr_drbg_seed returned -0x%04x\n",
                -ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    /*
     * 1c. Prepare SSL configuration
     */
    mbedtls_printf( "  . Setting up the SSL data...." );

    if( ( ret = mbedtls_ssl_config_defaults( &conf,
                    MBEDTLS_SSL_IS_SERVER,
                    MBEDTLS_SSL_TRANSPORT_STREAM,
                    MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
    {
        mbedtls_printf( " failed: mbedtls_ssl_config_defaults returned -0x%04x\n",
                -ret );
        goto exit;
    }

    mbedtls_ssl_conf_rng( &conf, mbedtls_ctr_drbg_random, &ctr_drbg );
    mbedtls_ssl_conf_dbg( &conf, my_mutexed_debug, stdout );

    /* mbedtls_ssl_cache_get() and mbedtls_ssl_cache_set() are thread-safe if
     * MBEDTLS_THREADING_C is set.
     */
#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_conf_session_cache( &conf, &cache,
                                   mbedtls_ssl_cache_get,
                                   mbedtls_ssl_cache_set );
#endif

    mbedtls_ssl_conf_ca_chain( &conf, &cachain, NULL );
    if( ( ret = mbedtls_ssl_conf_own_cert( &conf, &srvcert, &pkey ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );


    /*
     * 2. Setup the listening TCP socket
     */
    mbedtls_printf( "  . Bind on https://localhost:4433/ ..." );
    fflush( stdout );

    if( ( ret = mbedtls_net_bind( &listen_fd, NULL, "4433", MBEDTLS_NET_PROTO_TCP ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_net_bind returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

reset:
#ifdef MBEDTLS_ERROR_C
    if( ret != 0 )
    {
        char error_buf[100];
        mbedtls_strerror( ret, error_buf, 100 );
        mbedtls_printf( "  [ main ]  Last error was: -0x%04x - %s\n", -ret, error_buf );
    }
#endif

    /*
     * 3. Wait until a client connects
     */
    mbedtls_printf( "  [ main ]  Waiting for a remote connection\n" );

    if( ( ret = mbedtls_net_accept( &listen_fd, &client_fd,
                                    NULL, 0, NULL ) ) != 0 )
    {
        mbedtls_printf( "  [ main ] failed: mbedtls_net_accept returned -0x%04x\n", ret );
        goto exit;
    }

    mbedtls_printf( "  [ main ]  ok\n" );
    mbedtls_printf( "  [ main ]  Creating a new thread\n" );

    if( ( ret = thread_create( &client_fd ) ) != 0 )
    {
        mbedtls_printf( "  [ main ]  failed: thread_create returned %d\n", ret );
        mbedtls_net_free( &client_fd );
        goto reset;
    }

    ret = 0;
    goto reset;

exit:
    mbedtls_x509_crt_free( &srvcert );
    mbedtls_pk_free( &pkey );
#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_cache_free( &cache );
#endif
    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );
    mbedtls_ssl_config_free( &conf );

    mbedtls_net_free( &listen_fd );

    mbedtls_mutex_free( &debug_mutex );

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
    mbedtls_memory_buffer_alloc_free();
#endif

#if defined(_WIN32)
    mbedtls_printf( "  Press Enter to exit this program.\n" );
    fflush( stdout ); getchar();
#endif

    return( ret );
}

#endif /* MBEDTLS_BIGNUM_C && MBEDTLS_CERTS_C && MBEDTLS_ENTROPY_C &&
          MBEDTLS_SSL_TLS_C && MBEDTLS_SSL_SRV_C && MBEDTLS_NET_C &&
          MBEDTLS_RSA_C && MBEDTLS_CTR_DRBG_C && MBEDTLS_THREADING_C &&
          MBEDTLS_THREADING_PTHREAD && MBEDTLS_PEM_PARSE_C */

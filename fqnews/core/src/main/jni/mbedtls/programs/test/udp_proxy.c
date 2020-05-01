/*
 *  UDP proxy: emulate an unreliable UDP connexion for DTLS testing
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

/*
 * Warning: this is an internal utility program we use for tests.
 * It does break some abstractions from the NET layer, and is thus NOT an
 * example of good general usage.
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
#include <time.h>
#define mbedtls_time            time
#define mbedtls_time_t          time_t
#define mbedtls_printf          printf
#define mbedtls_calloc          calloc
#define mbedtls_free            free
#define MBEDTLS_EXIT_SUCCESS    EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE    EXIT_FAILURE
#endif /* MBEDTLS_PLATFORM_C */

#if !defined(MBEDTLS_NET_C)
int main( void )
{
    mbedtls_printf( "MBEDTLS_NET_C not defined.\n" );
    return( 0 );
}
#else

#include "mbedtls/net_sockets.h"
#include "mbedtls/error.h"
#include "mbedtls/ssl.h"
#include "mbedtls/timing.h"

#include <string.h>

/* For select() */
#if (defined(_WIN32) || defined(_WIN32_WCE)) && !defined(EFIX64) && \
    !defined(EFI32)
#include <winsock2.h>
#include <windows.h>
#if defined(_MSC_VER)
#if defined(_WIN32_WCE)
#pragma comment( lib, "ws2.lib" )
#else
#pragma comment( lib, "ws2_32.lib" )
#endif
#endif /* _MSC_VER */
#else /* ( _WIN32 || _WIN32_WCE ) && !EFIX64 && !EFI32 */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif /* ( _WIN32 || _WIN32_WCE ) && !EFIX64 && !EFI32 */

#define MAX_MSG_SIZE            16384 + 2048 /* max record/datagram size */

#define DFL_SERVER_ADDR         "localhost"
#define DFL_SERVER_PORT         "4433"
#define DFL_LISTEN_ADDR         "localhost"
#define DFL_LISTEN_PORT         "5556"
#define DFL_PACK                0

#if defined(MBEDTLS_TIMING_C)
#define USAGE_PACK                                                          \
    "    pack=%%d             default: 0     (don't pack)\n"                \
    "                         options: t > 0 (pack for t milliseconds)\n"
#else
#define USAGE_PACK
#endif

#define USAGE                                                               \
    "\n usage: udp_proxy param=<>...\n"                                     \
    "\n acceptable parameters:\n"                                           \
    "    server_addr=%%s      default: localhost\n"                         \
    "    server_port=%%d      default: 4433\n"                              \
    "    listen_addr=%%s      default: localhost\n"                         \
    "    listen_port=%%d      default: 4433\n"                              \
    "\n"                                                                    \
    "    duplicate=%%d        default: 0 (no duplication)\n"                \
    "                        duplicate about 1:N packets randomly\n"        \
    "    delay=%%d            default: 0 (no delayed packets)\n"            \
    "                        delay about 1:N packets randomly\n"            \
    "    delay_ccs=0/1       default: 0 (don't delay ChangeCipherSpec)\n"   \
    "    delay_cli=%%s        Handshake message from client that should be\n"\
    "                        delayed. Possible values are 'ClientHello',\n" \
    "                        'Certificate', 'CertificateVerify', and\n"     \
    "                        'ClientKeyExchange'.\n"                        \
    "                        May be used multiple times, even for the same\n"\
    "                        message, in which case the respective message\n"\
    "                        gets delayed multiple times.\n"                 \
    "    delay_srv=%%s        Handshake message from server that should be\n"\
    "                        delayed. Possible values are 'HelloRequest',\n"\
    "                        'ServerHello', 'ServerHelloDone', 'Certificate'\n"\
    "                        'ServerKeyExchange', 'NewSessionTicket',\n"\
    "                        'HelloVerifyRequest' and ''CertificateRequest'.\n"\
    "                        May be used multiple times, even for the same\n"\
    "                        message, in which case the respective message\n"\
    "                        gets delayed multiple times.\n"                 \
    "    drop=%%d             default: 0 (no dropped packets)\n"            \
    "                        drop about 1:N packets randomly\n"             \
    "    mtu=%%d              default: 0 (unlimited)\n"                     \
    "                        drop packets larger than N bytes\n"            \
    "    bad_ad=0/1          default: 0 (don't add bad ApplicationData)\n"  \
    "    protect_hvr=0/1     default: 0 (don't protect HelloVerifyRequest)\n" \
    "    protect_len=%%d      default: (don't protect packets of this size)\n" \
    "\n"                                                                    \
    "    seed=%%d             default: (use current time)\n"                \
    USAGE_PACK                                                              \
    "\n"

/*
 * global options
 */

#define MAX_DELAYED_HS 10

static struct options
{
    const char *server_addr;    /* address to forward packets to            */
    const char *server_port;    /* port to forward packets to               */
    const char *listen_addr;    /* address for accepting client connections */
    const char *listen_port;    /* port for accepting client connections    */

    int duplicate;              /* duplicate 1 in N packets (none if 0)     */
    int delay;                  /* delay 1 packet in N (none if 0)          */
    int delay_ccs;              /* delay ChangeCipherSpec                   */
    char* delay_cli[MAX_DELAYED_HS];  /* handshake types of messages from
                                       * client that should be delayed.     */
    uint8_t delay_cli_cnt;      /* Number of entries in delay_cli.          */
    char* delay_srv[MAX_DELAYED_HS];  /* handshake types of messages from
                                       * server that should be delayed.     */
    uint8_t delay_srv_cnt;      /* Number of entries in delay_srv.          */
    int drop;                   /* drop 1 packet in N (none if 0)           */
    int mtu;                    /* drop packets larger than this            */
    int bad_ad;                 /* inject corrupted ApplicationData record  */
    int protect_hvr;            /* never drop or delay HelloVerifyRequest   */
    int protect_len;            /* never drop/delay packet of the given size*/
    unsigned pack;              /* merge packets into single datagram for
                                 * at most \c merge milliseconds if > 0     */
    unsigned int seed;          /* seed for "random" events                 */
} opt;

static void exit_usage( const char *name, const char *value )
{
    if( value == NULL )
        mbedtls_printf( " unknown option or missing value: %s\n", name );
    else
        mbedtls_printf( " option %s: illegal value: %s\n", name, value );

    mbedtls_printf( USAGE );
    exit( 1 );
}

static void get_options( int argc, char *argv[] )
{
    int i;
    char *p, *q;

    opt.server_addr    = DFL_SERVER_ADDR;
    opt.server_port    = DFL_SERVER_PORT;
    opt.listen_addr    = DFL_LISTEN_ADDR;
    opt.listen_port    = DFL_LISTEN_PORT;
    opt.pack           = DFL_PACK;
    /* Other members default to 0 */

    opt.delay_cli_cnt = 0;
    opt.delay_srv_cnt = 0;
    memset( opt.delay_cli, 0, sizeof( opt.delay_cli ) );
    memset( opt.delay_srv, 0, sizeof( opt.delay_srv ) );

    for( i = 1; i < argc; i++ )
    {
        p = argv[i];
        if( ( q = strchr( p, '=' ) ) == NULL )
            exit_usage( p, NULL );
        *q++ = '\0';

        if( strcmp( p, "server_addr" ) == 0 )
            opt.server_addr = q;
        else if( strcmp( p, "server_port" ) == 0 )
            opt.server_port = q;
        else if( strcmp( p, "listen_addr" ) == 0 )
            opt.listen_addr = q;
        else if( strcmp( p, "listen_port" ) == 0 )
            opt.listen_port = q;
        else if( strcmp( p, "duplicate" ) == 0 )
        {
            opt.duplicate = atoi( q );
            if( opt.duplicate < 0 || opt.duplicate > 20 )
                exit_usage( p, q );
        }
        else if( strcmp( p, "delay" ) == 0 )
        {
            opt.delay = atoi( q );
            if( opt.delay < 0 || opt.delay > 20 || opt.delay == 1 )
                exit_usage( p, q );
        }
        else if( strcmp( p, "delay_ccs" ) == 0 )
        {
            opt.delay_ccs = atoi( q );
            if( opt.delay_ccs < 0 || opt.delay_ccs > 1 )
                exit_usage( p, q );
        }
        else if( strcmp( p, "delay_cli" ) == 0 ||
                 strcmp( p, "delay_srv" ) == 0 )
        {
            uint8_t *delay_cnt;
            char **delay_list;
            size_t len;
            char *buf;

            if( strcmp( p, "delay_cli" ) == 0 )
            {
                delay_cnt  = &opt.delay_cli_cnt;
                delay_list = opt.delay_cli;
            }
            else
            {
                delay_cnt  = &opt.delay_srv_cnt;
                delay_list = opt.delay_srv;
            }

            if( *delay_cnt == MAX_DELAYED_HS )
            {
                mbedtls_printf( " too many uses of %s: only %d allowed\n",
                                p, MAX_DELAYED_HS );
                exit_usage( p, NULL );
            }

            len = strlen( q );
            buf = mbedtls_calloc( 1, len + 1 );
            if( buf == NULL )
            {
                mbedtls_printf( " Allocation failure\n" );
                exit( 1 );
            }
            memcpy( buf, q, len + 1 );

            delay_list[ (*delay_cnt)++ ] = buf;
        }
        else if( strcmp( p, "drop" ) == 0 )
        {
            opt.drop = atoi( q );
            if( opt.drop < 0 || opt.drop > 20 || opt.drop == 1 )
                exit_usage( p, q );
        }
        else if( strcmp( p, "pack" ) == 0 )
        {
#if defined(MBEDTLS_TIMING_C)
            opt.pack = (unsigned) atoi( q );
#else
            mbedtls_printf( " option pack only defined if MBEDTLS_TIMING_C is enabled\n" );
            exit( 1 );
#endif
        }
        else if( strcmp( p, "mtu" ) == 0 )
        {
            opt.mtu = atoi( q );
            if( opt.mtu < 0 || opt.mtu > MAX_MSG_SIZE )
                exit_usage( p, q );
        }
        else if( strcmp( p, "bad_ad" ) == 0 )
        {
            opt.bad_ad = atoi( q );
            if( opt.bad_ad < 0 || opt.bad_ad > 1 )
                exit_usage( p, q );
        }
        else if( strcmp( p, "protect_hvr" ) == 0 )
        {
            opt.protect_hvr = atoi( q );
            if( opt.protect_hvr < 0 || opt.protect_hvr > 1 )
                exit_usage( p, q );
        }
        else if( strcmp( p, "protect_len" ) == 0 )
        {
            opt.protect_len = atoi( q );
            if( opt.protect_len < 0 )
                exit_usage( p, q );
        }
        else if( strcmp( p, "seed" ) == 0 )
        {
            opt.seed = atoi( q );
            if( opt.seed == 0 )
                exit_usage( p, q );
        }
        else
            exit_usage( p, NULL );
    }
}

static const char *msg_type( unsigned char *msg, size_t len )
{
    if( len < 1 )                           return( "Invalid" );
    switch( msg[0] )
    {
        case MBEDTLS_SSL_MSG_CHANGE_CIPHER_SPEC:    return( "ChangeCipherSpec" );
        case MBEDTLS_SSL_MSG_ALERT:                 return( "Alert" );
        case MBEDTLS_SSL_MSG_APPLICATION_DATA:      return( "ApplicationData" );
        case MBEDTLS_SSL_MSG_HANDSHAKE:             break; /* See below */
        default:                            return( "Unknown" );
    }

    if( len < 13 + 12 )                     return( "Invalid handshake" );

    /*
     * Our handshake message are less than 2^16 bytes long, so they should
     * have 0 as the first byte of length, frag_offset and frag_length.
     * Otherwise, assume they are encrypted.
     */
    if( msg[14] || msg[19] || msg[22] )     return( "Encrypted handshake" );

    switch( msg[13] )
    {
        case MBEDTLS_SSL_HS_HELLO_REQUEST:          return( "HelloRequest" );
        case MBEDTLS_SSL_HS_CLIENT_HELLO:           return( "ClientHello" );
        case MBEDTLS_SSL_HS_SERVER_HELLO:           return( "ServerHello" );
        case MBEDTLS_SSL_HS_HELLO_VERIFY_REQUEST:   return( "HelloVerifyRequest" );
        case MBEDTLS_SSL_HS_NEW_SESSION_TICKET:     return( "NewSessionTicket" );
        case MBEDTLS_SSL_HS_CERTIFICATE:            return( "Certificate" );
        case MBEDTLS_SSL_HS_SERVER_KEY_EXCHANGE:    return( "ServerKeyExchange" );
        case MBEDTLS_SSL_HS_CERTIFICATE_REQUEST:    return( "CertificateRequest" );
        case MBEDTLS_SSL_HS_SERVER_HELLO_DONE:      return( "ServerHelloDone" );
        case MBEDTLS_SSL_HS_CERTIFICATE_VERIFY:     return( "CertificateVerify" );
        case MBEDTLS_SSL_HS_CLIENT_KEY_EXCHANGE:    return( "ClientKeyExchange" );
        case MBEDTLS_SSL_HS_FINISHED:               return( "Finished" );
        default:                            return( "Unknown handshake" );
    }
}

#if defined(MBEDTLS_TIMING_C)
/* Return elapsed time in milliseconds since the first call */
static unsigned ellapsed_time( void )
{
    static int initialized = 0;
    static struct mbedtls_timing_hr_time hires;

    if( initialized == 0 )
    {
        (void) mbedtls_timing_get_timer( &hires, 1 );
        initialized = 1;
        return( 0 );
    }

    return( mbedtls_timing_get_timer( &hires, 0 ) );
}

typedef struct
{
    mbedtls_net_context *ctx;

    const char *description;

    unsigned packet_lifetime;
    unsigned num_datagrams;

    unsigned char data[MAX_MSG_SIZE];
    size_t len;

} ctx_buffer;

static ctx_buffer outbuf[2];

static int ctx_buffer_flush( ctx_buffer *buf )
{
    int ret;

    mbedtls_printf( "  %05u flush    %s: %u bytes, %u datagrams, last %u ms\n",
                    ellapsed_time(), buf->description,
                    (unsigned) buf->len, buf->num_datagrams,
                    ellapsed_time() - buf->packet_lifetime );

    ret = mbedtls_net_send( buf->ctx, buf->data, buf->len );

    buf->len           = 0;
    buf->num_datagrams = 0;

    return( ret );
}

static unsigned ctx_buffer_time_remaining( ctx_buffer *buf )
{
    unsigned const cur_time = ellapsed_time();

    if( buf->num_datagrams == 0 )
        return( (unsigned) -1 );

    if( cur_time - buf->packet_lifetime >= opt.pack )
        return( 0 );

    return( opt.pack - ( cur_time - buf->packet_lifetime ) );
}

static int ctx_buffer_append( ctx_buffer *buf,
                              const unsigned char * data,
                              size_t len )
{
    int ret;

    if( len > (size_t) INT_MAX )
        return( -1 );

    if( len > sizeof( buf->data ) )
    {
        mbedtls_printf( "  ! buffer size %u too large (max %u)\n",
                        (unsigned) len, (unsigned) sizeof( buf->data ) );
        return( -1 );
    }

    if( sizeof( buf->data ) - buf->len < len )
    {
        if( ( ret = ctx_buffer_flush( buf ) ) <= 0 )
            return( ret );
    }

    memcpy( buf->data + buf->len, data, len );

    buf->len += len;
    if( ++buf->num_datagrams == 1 )
        buf->packet_lifetime = ellapsed_time();

    return( (int) len );
}
#endif /* MBEDTLS_TIMING_C */

static int dispatch_data( mbedtls_net_context *ctx,
                          const unsigned char * data,
                          size_t len )
{
#if defined(MBEDTLS_TIMING_C)
    ctx_buffer *buf = NULL;
    if( opt.pack > 0 )
    {
        if( outbuf[0].ctx == ctx )
            buf = &outbuf[0];
        else if( outbuf[1].ctx == ctx )
            buf = &outbuf[1];

        if( buf == NULL )
            return( -1 );

        return( ctx_buffer_append( buf, data, len ) );
    }
#endif /* MBEDTLS_TIMING_C */

    return( mbedtls_net_send( ctx, data, len ) );
}

typedef struct
{
    mbedtls_net_context *dst;
    const char *way;
    const char *type;
    unsigned len;
    unsigned char buf[MAX_MSG_SIZE];
} packet;

/* Print packet. Outgoing packets come with a reason (forward, dupl, etc.) */
void print_packet( const packet *p, const char *why )
{
#if defined(MBEDTLS_TIMING_C)
    if( why == NULL )
        mbedtls_printf( "  %05u dispatch %s %s (%u bytes)\n",
                ellapsed_time(), p->way, p->type, p->len );
    else
        mbedtls_printf( "  %05u dispatch %s %s (%u bytes): %s\n",
                ellapsed_time(), p->way, p->type, p->len, why );
#else
    if( why == NULL )
        mbedtls_printf( "        dispatch %s %s (%u bytes)\n",
                p->way, p->type, p->len );
    else
        mbedtls_printf( "        dispatch %s %s (%u bytes): %s\n",
                p->way, p->type, p->len, why );
#endif

    fflush( stdout );
}

int send_packet( const packet *p, const char *why )
{
    int ret;
    mbedtls_net_context *dst = p->dst;

    /* insert corrupted ApplicationData record? */
    if( opt.bad_ad &&
        strcmp( p->type, "ApplicationData" ) == 0 )
    {
        unsigned char buf[MAX_MSG_SIZE];
        memcpy( buf, p->buf, p->len );

        if( p->len <= 13 )
        {
            mbedtls_printf( "  ! can't corrupt empty AD record" );
        }
        else
        {
            ++buf[13];
            print_packet( p, "corrupted" );
        }

        if( ( ret = dispatch_data( dst, buf, p->len ) ) <= 0 )
        {
            mbedtls_printf( "  ! dispatch returned %d\n", ret );
            return( ret );
        }
    }

    print_packet( p, why );
    if( ( ret = dispatch_data( dst, p->buf, p->len ) ) <= 0 )
    {
        mbedtls_printf( "  ! dispatch returned %d\n", ret );
        return( ret );
    }

    /* Don't duplicate Application Data, only handshake covered */
    if( opt.duplicate != 0 &&
        strcmp( p->type, "ApplicationData" ) != 0 &&
        rand() % opt.duplicate == 0 )
    {
        print_packet( p, "duplicated" );

        if( ( ret = dispatch_data( dst, p->buf, p->len ) ) <= 0 )
        {
            mbedtls_printf( "  ! dispatch returned %d\n", ret );
            return( ret );
        }
    }

    return( 0 );
}

#define MAX_DELAYED_MSG 5
static size_t prev_len;
static packet prev[MAX_DELAYED_MSG];

void clear_pending( void )
{
    memset( &prev, 0, sizeof( prev ) );
    prev_len = 0;
}

void delay_packet( packet *delay )
{
    if( prev_len == MAX_DELAYED_MSG )
        return;

    memcpy( &prev[prev_len++], delay, sizeof( packet ) );
}

int send_delayed()
{
    uint8_t offset;
    int ret;
    for( offset = 0; offset < prev_len; offset++ )
    {
        ret = send_packet( &prev[offset], "delayed" );
        if( ret != 0 )
            return( ret );
    }

    clear_pending();
    return( 0 );
}

/*
 * Avoid dropping or delaying a packet that was already dropped twice: this
 * only results in uninteresting timeouts. We can't rely on type to identify
 * packets, since during renegotiation they're all encrypted.  So, rely on
 * size mod 2048 (which is usually just size).
 */
static unsigned char dropped[2048] = { 0 };
#define DROP_MAX 2

/*
 * OpenSSL groups packets in a datagram the first time it sends them, but not
 * when it resends them. Count every record as seen the first time.
 */
void update_dropped( const packet *p )
{
    size_t id = p->len % sizeof( dropped );
    const unsigned char *end = p->buf + p->len;
    const unsigned char *cur = p->buf;
    size_t len = ( ( cur[11] << 8 ) | cur[12] ) + 13;

    ++dropped[id];

    /* Avoid counting single record twice */
    if( len == p->len )
        return;

    while( cur < end )
    {
        len = ( ( cur[11] << 8 ) | cur[12] ) + 13;

        id = len % sizeof( dropped );
        ++dropped[id];

        cur += len;
    }
}

int handle_message( const char *way,
                    mbedtls_net_context *dst,
                    mbedtls_net_context *src )
{
    int ret;
    packet cur;
    size_t id;

    uint8_t delay_idx;
    char ** delay_list;
    uint8_t delay_list_len;

    /* receive packet */
    if( ( ret = mbedtls_net_recv( src, cur.buf, sizeof( cur.buf ) ) ) <= 0 )
    {
        mbedtls_printf( "  ! mbedtls_net_recv returned %d\n", ret );
        return( ret );
    }

    cur.len  = ret;
    cur.type = msg_type( cur.buf, cur.len );
    cur.way  = way;
    cur.dst  = dst;
    print_packet( &cur, NULL );

    id = cur.len % sizeof( dropped );

    if( strcmp( way, "S <- C" ) == 0 )
    {
        delay_list     = opt.delay_cli;
        delay_list_len = opt.delay_cli_cnt;
    }
    else
    {
        delay_list     = opt.delay_srv;
        delay_list_len = opt.delay_srv_cnt;
    }

    /* Check if message type is in the list of messages
     * that should be delayed */
    for( delay_idx = 0; delay_idx < delay_list_len; delay_idx++ )
    {
        if( delay_list[ delay_idx ] == NULL )
            continue;

        if( strcmp( delay_list[ delay_idx ], cur.type ) == 0 )
        {
            /* Delay message */
            delay_packet( &cur );

            /* Remove entry from list */
            mbedtls_free( delay_list[delay_idx] );
            delay_list[delay_idx] = NULL;

            return( 0 );
        }
    }

    /* do we want to drop, delay, or forward it? */
    if( ( opt.mtu != 0 &&
          cur.len > (unsigned) opt.mtu ) ||
        ( opt.drop != 0 &&
          strcmp( cur.type, "ApplicationData" ) != 0 &&
          ! ( opt.protect_hvr &&
              strcmp( cur.type, "HelloVerifyRequest" ) == 0 ) &&
          cur.len != (size_t) opt.protect_len &&
          dropped[id] < DROP_MAX &&
          rand() % opt.drop == 0 ) )
    {
        update_dropped( &cur );
    }
    else if( ( opt.delay_ccs == 1 &&
               strcmp( cur.type, "ChangeCipherSpec" ) == 0 ) ||
             ( opt.delay != 0 &&
               strcmp( cur.type, "ApplicationData" ) != 0 &&
               ! ( opt.protect_hvr &&
                   strcmp( cur.type, "HelloVerifyRequest" ) == 0 ) &&
               cur.len != (size_t) opt.protect_len &&
               dropped[id] < DROP_MAX &&
               rand() % opt.delay == 0 ) )
    {
        delay_packet( &cur );
    }
    else
    {
        /* forward and possibly duplicate */
        if( ( ret = send_packet( &cur, "forwarded" ) ) != 0 )
            return( ret );

        /* send previously delayed messages if any */
        ret = send_delayed();
        if( ret != 0 )
            return( ret );
    }

    return( 0 );
}

int main( int argc, char *argv[] )
{
    int ret = 1;
    int exit_code = MBEDTLS_EXIT_FAILURE;
    uint8_t delay_idx;

    mbedtls_net_context listen_fd, client_fd, server_fd;

#if defined( MBEDTLS_TIMING_C )
    struct timeval tm;
#endif

    struct timeval *tm_ptr = NULL;

    int nb_fds;
    fd_set read_fds;

    mbedtls_net_init( &listen_fd );
    mbedtls_net_init( &client_fd );
    mbedtls_net_init( &server_fd );

    get_options( argc, argv );

    /*
     * Decisions to drop/delay/duplicate packets are pseudo-random: dropping
     * exactly 1 in N packets would lead to problems when a flight has exactly
     * N packets: the same packet would be dropped on every resend.
     *
     * In order to be able to reproduce problems reliably, the seed may be
     * specified explicitly.
     */
    if( opt.seed == 0 )
    {
        opt.seed = (unsigned int) time( NULL );
        mbedtls_printf( "  . Pseudo-random seed: %u\n", opt.seed );
    }

    srand( opt.seed );

    /*
     * 0. "Connect" to the server
     */
    mbedtls_printf( "  . Connect to server on UDP/%s/%s ...",
            opt.server_addr, opt.server_port );
    fflush( stdout );

    if( ( ret = mbedtls_net_connect( &server_fd, opt.server_addr, opt.server_port,
                             MBEDTLS_NET_PROTO_UDP ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_net_connect returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    /*
     * 1. Setup the "listening" UDP socket
     */
    mbedtls_printf( "  . Bind on UDP/%s/%s ...",
            opt.listen_addr, opt.listen_port );
    fflush( stdout );

    if( ( ret = mbedtls_net_bind( &listen_fd, opt.listen_addr, opt.listen_port,
                          MBEDTLS_NET_PROTO_UDP ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_net_bind returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    /*
     * 2. Wait until a client connects
     */
accept:
    mbedtls_net_free( &client_fd );

    mbedtls_printf( "  . Waiting for a remote connection ..." );
    fflush( stdout );

    if( ( ret = mbedtls_net_accept( &listen_fd, &client_fd,
                                    NULL, 0, NULL ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_net_accept returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    /*
     * 3. Forward packets forever (kill the process to terminate it)
     */
    clear_pending();
    memset( dropped, 0, sizeof( dropped ) );

    nb_fds = client_fd.fd;
    if( nb_fds < server_fd.fd )
        nb_fds = server_fd.fd;
    if( nb_fds < listen_fd.fd )
        nb_fds = listen_fd.fd;
    ++nb_fds;

#if defined(MBEDTLS_TIMING_C)
    if( opt.pack > 0 )
    {
        outbuf[0].ctx = &server_fd;
        outbuf[0].description = "S <- C";
        outbuf[0].num_datagrams = 0;
        outbuf[0].len = 0;

        outbuf[1].ctx = &client_fd;
        outbuf[1].description = "S -> C";
        outbuf[1].num_datagrams = 0;
        outbuf[1].len = 0;
    }
#endif /* MBEDTLS_TIMING_C */

    while( 1 )
    {
#if defined(MBEDTLS_TIMING_C)
        if( opt.pack > 0 )
        {
            unsigned max_wait_server, max_wait_client, max_wait;
            max_wait_server = ctx_buffer_time_remaining( &outbuf[0] );
            max_wait_client = ctx_buffer_time_remaining( &outbuf[1] );

            max_wait = (unsigned) -1;

            if( max_wait_server == 0 )
                ctx_buffer_flush( &outbuf[0] );
            else
                max_wait = max_wait_server;

            if( max_wait_client == 0 )
                ctx_buffer_flush( &outbuf[1] );
            else
            {
                if( max_wait_client < max_wait )
                    max_wait = max_wait_client;
            }

            if( max_wait != (unsigned) -1 )
            {
                tm.tv_sec  = max_wait / 1000;
                tm.tv_usec = ( max_wait % 1000 ) * 1000;

                tm_ptr = &tm;
            }
            else
            {
                tm_ptr = NULL;
            }
        }
#endif /* MBEDTLS_TIMING_C */

        FD_ZERO( &read_fds );
        FD_SET( server_fd.fd, &read_fds );
        FD_SET( client_fd.fd, &read_fds );
        FD_SET( listen_fd.fd, &read_fds );

        if( ( ret = select( nb_fds, &read_fds, NULL, NULL, tm_ptr ) ) < 0 )
        {
            perror( "select" );
            goto exit;
        }

        if( FD_ISSET( listen_fd.fd, &read_fds ) )
            goto accept;

        if( FD_ISSET( client_fd.fd, &read_fds ) )
        {
            if( ( ret = handle_message( "S <- C",
                                        &server_fd, &client_fd ) ) != 0 )
                goto accept;
        }

        if( FD_ISSET( server_fd.fd, &read_fds ) )
        {
            if( ( ret = handle_message( "S -> C",
                                        &client_fd, &server_fd ) ) != 0 )
                goto accept;
        }

    }

    exit_code = MBEDTLS_EXIT_SUCCESS;

exit:

#ifdef MBEDTLS_ERROR_C
    if( exit_code != MBEDTLS_EXIT_SUCCESS )
    {
        char error_buf[100];
        mbedtls_strerror( ret, error_buf, 100 );
        mbedtls_printf( "Last error was: -0x%04X - %s\n\n", - ret, error_buf );
        fflush( stdout );
    }
#endif

    for( delay_idx = 0; delay_idx < MAX_DELAYED_HS; delay_idx++ )
    {
        mbedtls_free( opt.delay_cli + delay_idx );
        mbedtls_free( opt.delay_srv + delay_idx );
    }

    mbedtls_net_free( &client_fd );
    mbedtls_net_free( &server_fd );
    mbedtls_net_free( &listen_fd );

#if defined(_WIN32)
    mbedtls_printf( "  Press Enter to exit this program.\n" );
    fflush( stdout ); getchar();
#endif

    return( exit_code );
}

#endif /* MBEDTLS_NET_C */

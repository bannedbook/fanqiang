/*
 *  Diffie-Hellman-Merkle key exchange (server side)
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
#define mbedtls_printf          printf
#define mbedtls_time_t          time_t
#define mbedtls_exit            exit
#define MBEDTLS_EXIT_SUCCESS    EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE    EXIT_FAILURE
#endif /* MBEDTLS_PLATFORM_C */

#if defined(MBEDTLS_AES_C) && defined(MBEDTLS_DHM_C) && \
    defined(MBEDTLS_ENTROPY_C) && defined(MBEDTLS_NET_C) && \
    defined(MBEDTLS_RSA_C) && defined(MBEDTLS_SHA256_C) && \
    defined(MBEDTLS_FS_IO) && defined(MBEDTLS_CTR_DRBG_C) && \
    defined(MBEDTLS_SHA1_C)
#include "mbedtls/net_sockets.h"
#include "mbedtls/aes.h"
#include "mbedtls/dhm.h"
#include "mbedtls/rsa.h"
#include "mbedtls/sha1.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

#include <stdio.h>
#include <string.h>
#endif

#define SERVER_PORT "11999"
#define PLAINTEXT "==Hello there!=="

#if !defined(MBEDTLS_AES_C) || !defined(MBEDTLS_DHM_C) ||     \
    !defined(MBEDTLS_ENTROPY_C) || !defined(MBEDTLS_NET_C) ||  \
    !defined(MBEDTLS_RSA_C) || !defined(MBEDTLS_SHA256_C) ||    \
    !defined(MBEDTLS_FS_IO) || !defined(MBEDTLS_CTR_DRBG_C) || \
    !defined(MBEDTLS_SHA1_C)
int main( void )
{
    mbedtls_printf("MBEDTLS_AES_C and/or MBEDTLS_DHM_C and/or MBEDTLS_ENTROPY_C "
           "and/or MBEDTLS_NET_C and/or MBEDTLS_RSA_C and/or "
           "MBEDTLS_SHA256_C and/or MBEDTLS_FS_IO and/or "
           "MBEDTLS_CTR_DRBG_C not defined.\n");
    return( 0 );
}
#else

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

int main( void )
{
    FILE *f;

    int ret = 1;
    int exit_code = MBEDTLS_EXIT_FAILURE;
    size_t n, buflen;
    mbedtls_net_context listen_fd, client_fd;

    unsigned char buf[2048];
    unsigned char hash[32];
    unsigned char buf2[2];
    const char *pers = "dh_server";

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_rsa_context rsa;
    mbedtls_dhm_context dhm;
    mbedtls_aes_context aes;

    mbedtls_mpi N, P, Q, D, E;

    mbedtls_net_init( &listen_fd );
    mbedtls_net_init( &client_fd );
    mbedtls_rsa_init( &rsa, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_SHA256 );
    mbedtls_dhm_init( &dhm );
    mbedtls_aes_init( &aes );
    mbedtls_ctr_drbg_init( &ctr_drbg );

    mbedtls_mpi_init( &N ); mbedtls_mpi_init( &P ); mbedtls_mpi_init( &Q );
    mbedtls_mpi_init( &D ); mbedtls_mpi_init( &E );

    /*
     * 1. Setup the RNG
     */
    mbedtls_printf( "\n  . Seeding the random number generator" );
    fflush( stdout );

    mbedtls_entropy_init( &entropy );
    if( ( ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret );
        goto exit;
    }

    /*
     * 2a. Read the server's private RSA key
     */
    mbedtls_printf( "\n  . Reading private key from rsa_priv.txt" );
    fflush( stdout );

    if( ( f = fopen( "rsa_priv.txt", "rb" ) ) == NULL )
    {
        mbedtls_printf( " failed\n  ! Could not open rsa_priv.txt\n" \
                "  ! Please run rsa_genkey first\n\n" );
        goto exit;
    }

    mbedtls_rsa_init( &rsa, MBEDTLS_RSA_PKCS_V15, 0 );

    if( ( ret = mbedtls_mpi_read_file( &N , 16, f ) ) != 0 ||
        ( ret = mbedtls_mpi_read_file( &E , 16, f ) ) != 0 ||
        ( ret = mbedtls_mpi_read_file( &D , 16, f ) ) != 0 ||
        ( ret = mbedtls_mpi_read_file( &P , 16, f ) ) != 0 ||
        ( ret = mbedtls_mpi_read_file( &Q , 16, f ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_mpi_read_file returned %d\n\n",
                        ret );
        fclose( f );
        goto exit;
    }
    fclose( f );

    if( ( ret = mbedtls_rsa_import( &rsa, &N, &P, &Q, &D, &E ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_rsa_import returned %d\n\n",
                        ret );
        goto exit;
    }

    if( ( ret = mbedtls_rsa_complete( &rsa ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_rsa_complete returned %d\n\n",
                        ret );
        goto exit;
    }

    /*
     * 2b. Get the DHM modulus and generator
     */
    mbedtls_printf( "\n  . Reading DH parameters from dh_prime.txt" );
    fflush( stdout );

    if( ( f = fopen( "dh_prime.txt", "rb" ) ) == NULL )
    {
        mbedtls_printf( " failed\n  ! Could not open dh_prime.txt\n" \
                "  ! Please run dh_genprime first\n\n" );
        goto exit;
    }

    if( mbedtls_mpi_read_file( &dhm.P, 16, f ) != 0 ||
        mbedtls_mpi_read_file( &dhm.G, 16, f ) != 0 )
    {
        mbedtls_printf( " failed\n  ! Invalid DH parameter file\n\n" );
        fclose( f );
        goto exit;
    }

    fclose( f );

    /*
     * 3. Wait for a client to connect
     */
    mbedtls_printf( "\n  . Waiting for a remote connection" );
    fflush( stdout );

    if( ( ret = mbedtls_net_bind( &listen_fd, NULL, SERVER_PORT, MBEDTLS_NET_PROTO_TCP ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_net_bind returned %d\n\n", ret );
        goto exit;
    }

    if( ( ret = mbedtls_net_accept( &listen_fd, &client_fd,
                                    NULL, 0, NULL ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_net_accept returned %d\n\n", ret );
        goto exit;
    }

    /*
     * 4. Setup the DH parameters (P,G,Ys)
     */
    mbedtls_printf( "\n  . Sending the server's DH parameters" );
    fflush( stdout );

    memset( buf, 0, sizeof( buf ) );

    if( ( ret = mbedtls_dhm_make_params( &dhm, (int) mbedtls_mpi_size( &dhm.P ), buf, &n,
                                 mbedtls_ctr_drbg_random, &ctr_drbg ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_dhm_make_params returned %d\n\n", ret );
        goto exit;
    }

    /*
     * 5. Sign the parameters and send them
     */
    if( ( ret = mbedtls_sha1_ret( buf, n, hash ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_sha1_ret returned %d\n\n", ret );
        goto exit;
    }

    buf[n    ] = (unsigned char)( rsa.len >> 8 );
    buf[n + 1] = (unsigned char)( rsa.len      );

    if( ( ret = mbedtls_rsa_pkcs1_sign( &rsa, NULL, NULL, MBEDTLS_RSA_PRIVATE, MBEDTLS_MD_SHA256,
                                0, hash, buf + n + 2 ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_rsa_pkcs1_sign returned %d\n\n", ret );
        goto exit;
    }

    buflen = n + 2 + rsa.len;
    buf2[0] = (unsigned char)( buflen >> 8 );
    buf2[1] = (unsigned char)( buflen      );

    if( ( ret = mbedtls_net_send( &client_fd, buf2, 2 ) ) != 2 ||
        ( ret = mbedtls_net_send( &client_fd, buf, buflen ) ) != (int) buflen )
    {
        mbedtls_printf( " failed\n  ! mbedtls_net_send returned %d\n\n", ret );
        goto exit;
    }

    /*
     * 6. Get the client's public value: Yc = G ^ Xc mod P
     */
    mbedtls_printf( "\n  . Receiving the client's public value" );
    fflush( stdout );

    memset( buf, 0, sizeof( buf ) );

    n = dhm.len;
    if( ( ret = mbedtls_net_recv( &client_fd, buf, n ) ) != (int) n )
    {
        mbedtls_printf( " failed\n  ! mbedtls_net_recv returned %d\n\n", ret );
        goto exit;
    }

    if( ( ret = mbedtls_dhm_read_public( &dhm, buf, dhm.len ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_dhm_read_public returned %d\n\n", ret );
        goto exit;
    }

    /*
     * 7. Derive the shared secret: K = Ys ^ Xc mod P
     */
    mbedtls_printf( "\n  . Shared secret: " );
    fflush( stdout );

    if( ( ret = mbedtls_dhm_calc_secret( &dhm, buf, sizeof( buf ), &n,
                                 mbedtls_ctr_drbg_random, &ctr_drbg ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_dhm_calc_secret returned %d\n\n", ret );
        goto exit;
    }

    for( n = 0; n < 16; n++ )
        mbedtls_printf( "%02x", buf[n] );

    /*
     * 8. Setup the AES-256 encryption key
     *
     * This is an overly simplified example; best practice is
     * to hash the shared secret with a random value to derive
     * the keying material for the encryption/decryption keys
     * and MACs.
     */
    mbedtls_printf( "...\n  . Encrypting and sending the ciphertext" );
    fflush( stdout );

    mbedtls_aes_setkey_enc( &aes, buf, 256 );
    memcpy( buf, PLAINTEXT, 16 );
    mbedtls_aes_crypt_ecb( &aes, MBEDTLS_AES_ENCRYPT, buf, buf );

    if( ( ret = mbedtls_net_send( &client_fd, buf, 16 ) ) != 16 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_net_send returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf( "\n\n" );

    exit_code = MBEDTLS_EXIT_SUCCESS;

exit:

    mbedtls_mpi_free( &N ); mbedtls_mpi_free( &P ); mbedtls_mpi_free( &Q );
    mbedtls_mpi_free( &D ); mbedtls_mpi_free( &E );

    mbedtls_net_free( &client_fd );
    mbedtls_net_free( &listen_fd );

    mbedtls_aes_free( &aes );
    mbedtls_rsa_free( &rsa );
    mbedtls_dhm_free( &dhm );
    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );

#if defined(_WIN32)
    mbedtls_printf( "  + Press Enter to exit this program.\n" );
    fflush( stdout ); getchar();
#endif

    return( exit_code );
}
#endif /* MBEDTLS_AES_C && MBEDTLS_DHM_C && MBEDTLS_ENTROPY_C &&
          MBEDTLS_NET_C && MBEDTLS_RSA_C && MBEDTLS_SHA256_C &&
          MBEDTLS_FS_IO && MBEDTLS_CTR_DRBG_C */

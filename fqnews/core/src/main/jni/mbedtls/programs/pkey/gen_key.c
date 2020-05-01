/*
 *  Key generation application
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
#define mbedtls_exit            exit
#define MBEDTLS_EXIT_SUCCESS    EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE    EXIT_FAILURE
#endif /* MBEDTLS_PLATFORM_C */

#if defined(MBEDTLS_PK_WRITE_C) && defined(MBEDTLS_FS_IO) && \
    defined(MBEDTLS_ENTROPY_C) && defined(MBEDTLS_CTR_DRBG_C)
#include "mbedtls/error.h"
#include "mbedtls/pk.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/rsa.h"
#include "mbedtls/error.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32)
#include <unistd.h>

#define DEV_RANDOM_THRESHOLD        32

int dev_random_entropy_poll( void *data, unsigned char *output,
                             size_t len, size_t *olen )
{
    FILE *file;
    size_t ret, left = len;
    unsigned char *p = output;
    ((void) data);

    *olen = 0;

    file = fopen( "/dev/random", "rb" );
    if( file == NULL )
        return( MBEDTLS_ERR_ENTROPY_SOURCE_FAILED );

    while( left > 0 )
    {
        /* /dev/random can return much less than requested. If so, try again */
        ret = fread( p, 1, left, file );
        if( ret == 0 && ferror( file ) )
        {
            fclose( file );
            return( MBEDTLS_ERR_ENTROPY_SOURCE_FAILED );
        }

        p += ret;
        left -= ret;
        sleep( 1 );
    }
    fclose( file );
    *olen = len;

    return( 0 );
}
#endif /* !_WIN32 */
#endif

#if defined(MBEDTLS_ECP_C)
#define DFL_EC_CURVE            mbedtls_ecp_curve_list()->grp_id
#else
#define DFL_EC_CURVE            0
#endif

#if !defined(_WIN32) && defined(MBEDTLS_FS_IO)
#define USAGE_DEV_RANDOM \
    "    use_dev_random=0|1    default: 0\n"
#else
#define USAGE_DEV_RANDOM ""
#endif /* !_WIN32 && MBEDTLS_FS_IO */

#define FORMAT_PEM              0
#define FORMAT_DER              1

#define DFL_TYPE                MBEDTLS_PK_RSA
#define DFL_RSA_KEYSIZE         4096
#define DFL_FILENAME            "keyfile.key"
#define DFL_FORMAT              FORMAT_PEM
#define DFL_USE_DEV_RANDOM      0

#define USAGE \
    "\n usage: gen_key param=<>...\n"                   \
    "\n acceptable parameters:\n"                       \
    "    type=rsa|ec           default: rsa\n"          \
    "    rsa_keysize=%%d        default: 4096\n"        \
    "    ec_curve=%%s           see below\n"            \
    "    filename=%%s           default: keyfile.key\n" \
    "    format=pem|der        default: pem\n"          \
    USAGE_DEV_RANDOM                                    \
    "\n"

#if !defined(MBEDTLS_PK_WRITE_C) || !defined(MBEDTLS_PEM_WRITE_C) || \
    !defined(MBEDTLS_FS_IO) || !defined(MBEDTLS_ENTROPY_C) || \
    !defined(MBEDTLS_CTR_DRBG_C)
int main( void )
{
    mbedtls_printf( "MBEDTLS_PK_WRITE_C and/or MBEDTLS_FS_IO and/or "
            "MBEDTLS_ENTROPY_C and/or MBEDTLS_CTR_DRBG_C and/or "
            "MBEDTLS_PEM_WRITE_C"
            "not defined.\n" );
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

/*
 * global options
 */
struct options
{
    int type;                   /* the type of key to generate          */
    int rsa_keysize;            /* length of key in bits                */
    int ec_curve;               /* curve identifier for EC keys         */
    const char *filename;       /* filename of the key file             */
    int format;                 /* the output format to use             */
    int use_dev_random;         /* use /dev/random as entropy source    */
} opt;

static int write_private_key( mbedtls_pk_context *key, const char *output_file )
{
    int ret;
    FILE *f;
    unsigned char output_buf[16000];
    unsigned char *c = output_buf;
    size_t len = 0;

    memset(output_buf, 0, 16000);
    if( opt.format == FORMAT_PEM )
    {
        if( ( ret = mbedtls_pk_write_key_pem( key, output_buf, 16000 ) ) != 0 )
            return( ret );

        len = strlen( (char *) output_buf );
    }
    else
    {
        if( ( ret = mbedtls_pk_write_key_der( key, output_buf, 16000 ) ) < 0 )
            return( ret );

        len = ret;
        c = output_buf + sizeof(output_buf) - len;
    }

    if( ( f = fopen( output_file, "wb" ) ) == NULL )
        return( -1 );

    if( fwrite( c, 1, len, f ) != len )
    {
        fclose( f );
        return( -1 );
    }

    fclose( f );

    return( 0 );
}

int main( int argc, char *argv[] )
{
    int ret = 1;
    int exit_code = MBEDTLS_EXIT_FAILURE;
    mbedtls_pk_context key;
    char buf[1024];
    int i;
    char *p, *q;
    mbedtls_mpi N, P, Q, D, E, DP, DQ, QP;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = "gen_key";
#if defined(MBEDTLS_ECP_C)
    const mbedtls_ecp_curve_info *curve_info;
#endif

    /*
     * Set to sane values
     */

    mbedtls_mpi_init( &N ); mbedtls_mpi_init( &P ); mbedtls_mpi_init( &Q );
    mbedtls_mpi_init( &D ); mbedtls_mpi_init( &E ); mbedtls_mpi_init( &DP );
    mbedtls_mpi_init( &DQ ); mbedtls_mpi_init( &QP );

    mbedtls_pk_init( &key );
    mbedtls_ctr_drbg_init( &ctr_drbg );
    memset( buf, 0, sizeof( buf ) );

    if( argc == 0 )
    {
    usage:
        mbedtls_printf( USAGE );
#if defined(MBEDTLS_ECP_C)
        mbedtls_printf( " available ec_curve values:\n" );
        curve_info = mbedtls_ecp_curve_list();
        mbedtls_printf( "    %s (default)\n", curve_info->name );
        while( ( ++curve_info )->name != NULL )
            mbedtls_printf( "    %s\n", curve_info->name );
#endif /* MBEDTLS_ECP_C */
        goto exit;
    }

    opt.type                = DFL_TYPE;
    opt.rsa_keysize         = DFL_RSA_KEYSIZE;
    opt.ec_curve            = DFL_EC_CURVE;
    opt.filename            = DFL_FILENAME;
    opt.format              = DFL_FORMAT;
    opt.use_dev_random      = DFL_USE_DEV_RANDOM;

    for( i = 1; i < argc; i++ )
    {
        p = argv[i];
        if( ( q = strchr( p, '=' ) ) == NULL )
            goto usage;
        *q++ = '\0';

        if( strcmp( p, "type" ) == 0 )
        {
            if( strcmp( q, "rsa" ) == 0 )
                opt.type = MBEDTLS_PK_RSA;
            else if( strcmp( q, "ec" ) == 0 )
                opt.type = MBEDTLS_PK_ECKEY;
            else
                goto usage;
        }
        else if( strcmp( p, "format" ) == 0 )
        {
            if( strcmp( q, "pem" ) == 0 )
                opt.format = FORMAT_PEM;
            else if( strcmp( q, "der" ) == 0 )
                opt.format = FORMAT_DER;
            else
                goto usage;
        }
        else if( strcmp( p, "rsa_keysize" ) == 0 )
        {
            opt.rsa_keysize = atoi( q );
            if( opt.rsa_keysize < 1024 ||
                opt.rsa_keysize > MBEDTLS_MPI_MAX_BITS )
                goto usage;
        }
#if defined(MBEDTLS_ECP_C)
        else if( strcmp( p, "ec_curve" ) == 0 )
        {
            if( ( curve_info = mbedtls_ecp_curve_info_from_name( q ) ) == NULL )
                goto usage;
            opt.ec_curve = curve_info->grp_id;
        }
#endif
        else if( strcmp( p, "filename" ) == 0 )
            opt.filename = q;
        else if( strcmp( p, "use_dev_random" ) == 0 )
        {
            opt.use_dev_random = atoi( q );
            if( opt.use_dev_random < 0 || opt.use_dev_random > 1 )
                goto usage;
        }
        else
            goto usage;
    }

    mbedtls_printf( "\n  . Seeding the random number generator..." );
    fflush( stdout );

    mbedtls_entropy_init( &entropy );
#if !defined(_WIN32) && defined(MBEDTLS_FS_IO)
    if( opt.use_dev_random )
    {
        if( ( ret = mbedtls_entropy_add_source( &entropy, dev_random_entropy_poll,
                                        NULL, DEV_RANDOM_THRESHOLD,
                                        MBEDTLS_ENTROPY_SOURCE_STRONG ) ) != 0 )
        {
            mbedtls_printf( " failed\n  ! mbedtls_entropy_add_source returned -0x%04x\n", -ret );
            goto exit;
        }

        mbedtls_printf("\n    Using /dev/random, so can take a long time! " );
        fflush( stdout );
    }
#endif /* !_WIN32 && MBEDTLS_FS_IO */

    if( ( ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        mbedtls_printf( " failed\n  ! mbedtls_ctr_drbg_seed returned -0x%04x\n", -ret );
        goto exit;
    }

    /*
     * 1.1. Generate the key
     */
    mbedtls_printf( "\n  . Generating the private key ..." );
    fflush( stdout );

    if( ( ret = mbedtls_pk_setup( &key,
            mbedtls_pk_info_from_type( (mbedtls_pk_type_t) opt.type ) ) ) != 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_pk_setup returned -0x%04x", -ret );
        goto exit;
    }

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_GENPRIME)
    if( opt.type == MBEDTLS_PK_RSA )
    {
        ret = mbedtls_rsa_gen_key( mbedtls_pk_rsa( key ), mbedtls_ctr_drbg_random, &ctr_drbg,
                                   opt.rsa_keysize, 65537 );
        if( ret != 0 )
        {
            mbedtls_printf( " failed\n  !  mbedtls_rsa_gen_key returned -0x%04x", -ret );
            goto exit;
        }
    }
    else
#endif /* MBEDTLS_RSA_C */
#if defined(MBEDTLS_ECP_C)
    if( opt.type == MBEDTLS_PK_ECKEY )
    {
        ret = mbedtls_ecp_gen_key( (mbedtls_ecp_group_id) opt.ec_curve,
                                   mbedtls_pk_ec( key ),
                                   mbedtls_ctr_drbg_random, &ctr_drbg );
        if( ret != 0 )
        {
            mbedtls_printf( " failed\n  !  mbedtls_ecp_gen_key returned -0x%04x", -ret );
            goto exit;
        }
    }
    else
#endif /* MBEDTLS_ECP_C */
    {
        mbedtls_printf( " failed\n  !  key type not supported\n" );
        goto exit;
    }

    /*
     * 1.2 Print the key
     */
    mbedtls_printf( " ok\n  . Key information:\n" );

#if defined(MBEDTLS_RSA_C)
    if( mbedtls_pk_get_type( &key ) == MBEDTLS_PK_RSA )
    {
        mbedtls_rsa_context *rsa = mbedtls_pk_rsa( key );

        if( ( ret = mbedtls_rsa_export    ( rsa, &N, &P, &Q, &D, &E ) ) != 0 ||
            ( ret = mbedtls_rsa_export_crt( rsa, &DP, &DQ, &QP ) )      != 0 )
        {
            mbedtls_printf( " failed\n  ! could not export RSA parameters\n\n" );
            goto exit;
        }

        mbedtls_mpi_write_file( "N:  ",  &N,  16, NULL );
        mbedtls_mpi_write_file( "E:  ",  &E,  16, NULL );
        mbedtls_mpi_write_file( "D:  ",  &D,  16, NULL );
        mbedtls_mpi_write_file( "P:  ",  &P,  16, NULL );
        mbedtls_mpi_write_file( "Q:  ",  &Q,  16, NULL );
        mbedtls_mpi_write_file( "DP: ",  &DP, 16, NULL );
        mbedtls_mpi_write_file( "DQ:  ", &DQ, 16, NULL );
        mbedtls_mpi_write_file( "QP:  ", &QP, 16, NULL );
    }
    else
#endif
#if defined(MBEDTLS_ECP_C)
    if( mbedtls_pk_get_type( &key ) == MBEDTLS_PK_ECKEY )
    {
        mbedtls_ecp_keypair *ecp = mbedtls_pk_ec( key );
        mbedtls_printf( "curve: %s\n",
                mbedtls_ecp_curve_info_from_grp_id( ecp->grp.id )->name );
        mbedtls_mpi_write_file( "X_Q:   ", &ecp->Q.X, 16, NULL );
        mbedtls_mpi_write_file( "Y_Q:   ", &ecp->Q.Y, 16, NULL );
        mbedtls_mpi_write_file( "D:     ", &ecp->d  , 16, NULL );
    }
    else
#endif
        mbedtls_printf("  ! key type not supported\n");

    /*
     * 1.3 Export key
     */
    mbedtls_printf( "  . Writing key to file..." );

    if( ( ret = write_private_key( &key, opt.filename ) ) != 0 )
    {
        mbedtls_printf( " failed\n" );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    exit_code = MBEDTLS_EXIT_SUCCESS;

exit:

    if( exit_code != MBEDTLS_EXIT_SUCCESS )
    {
#ifdef MBEDTLS_ERROR_C
        mbedtls_strerror( ret, buf, sizeof( buf ) );
        mbedtls_printf( " - %s\n", buf );
#else
        mbedtls_printf("\n");
#endif
    }

    mbedtls_mpi_free( &N ); mbedtls_mpi_free( &P ); mbedtls_mpi_free( &Q );
    mbedtls_mpi_free( &D ); mbedtls_mpi_free( &E ); mbedtls_mpi_free( &DP );
    mbedtls_mpi_free( &DQ ); mbedtls_mpi_free( &QP );

    mbedtls_pk_free( &key );
    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_entropy_free( &entropy );

#if defined(_WIN32)
    mbedtls_printf( "  + Press Enter to exit this program.\n" );
    fflush( stdout ); getchar();
#endif

    return( exit_code );
}
#endif /* MBEDTLS_PK_WRITE_C && MBEDTLS_PEM_WRITE_C && MBEDTLS_FS_IO &&
        * MBEDTLS_ENTROPY_C && MBEDTLS_CTR_DRBG_C */

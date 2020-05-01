/*
 *  Key writing application
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

#if defined(MBEDTLS_PK_WRITE_C) && defined(MBEDTLS_FS_IO)
#include "mbedtls/error.h"
#include "mbedtls/pk.h"
#include "mbedtls/error.h"

#include <stdio.h>
#include <string.h>
#endif

#if defined(MBEDTLS_PEM_WRITE_C)
#define USAGE_OUT \
    "    output_file=%%s      default: keyfile.pem\n"   \
    "    output_format=pem|der default: pem\n"
#else
#define USAGE_OUT \
    "    output_file=%%s      default: keyfile.der\n"   \
    "    output_format=der     default: der\n"
#endif

#if defined(MBEDTLS_PEM_WRITE_C)
#define DFL_OUTPUT_FILENAME     "keyfile.pem"
#define DFL_OUTPUT_FORMAT       OUTPUT_FORMAT_PEM
#else
#define DFL_OUTPUT_FILENAME     "keyfile.der"
#define DFL_OUTPUT_FORMAT       OUTPUT_FORMAT_DER
#endif

#define DFL_MODE                MODE_NONE
#define DFL_FILENAME            "keyfile.key"
#define DFL_DEBUG_LEVEL         0
#define DFL_OUTPUT_MODE         OUTPUT_MODE_NONE

#define MODE_NONE               0
#define MODE_PRIVATE            1
#define MODE_PUBLIC             2

#define OUTPUT_MODE_NONE               0
#define OUTPUT_MODE_PRIVATE            1
#define OUTPUT_MODE_PUBLIC             2

#define OUTPUT_FORMAT_PEM              0
#define OUTPUT_FORMAT_DER              1

#define USAGE \
    "\n usage: key_app_writer param=<>...\n"            \
    "\n acceptable parameters:\n"                       \
    "    mode=private|public default: none\n"           \
    "    filename=%%s         default: keyfile.key\n"   \
    "    output_mode=private|public default: none\n"    \
    USAGE_OUT                                           \
    "\n"

#if !defined(MBEDTLS_PK_PARSE_C) || \
    !defined(MBEDTLS_PK_WRITE_C) || \
    !defined(MBEDTLS_FS_IO)
int main( void )
{
    mbedtls_printf( "MBEDTLS_PK_PARSE_C and/or MBEDTLS_PK_WRITE_C and/or MBEDTLS_FS_IO not defined.\n" );
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
    int mode;                   /* the mode to run the application in   */
    const char *filename;       /* filename of the key file             */
    int output_mode;            /* the output mode to use               */
    const char *output_file;    /* where to store the constructed key file  */
    int output_format;          /* the output format to use             */
} opt;

static int write_public_key( mbedtls_pk_context *key, const char *output_file )
{
    int ret;
    FILE *f;
    unsigned char output_buf[16000];
    unsigned char *c = output_buf;
    size_t len = 0;

    memset(output_buf, 0, 16000);

#if defined(MBEDTLS_PEM_WRITE_C)
    if( opt.output_format == OUTPUT_FORMAT_PEM )
    {
        if( ( ret = mbedtls_pk_write_pubkey_pem( key, output_buf, 16000 ) ) != 0 )
            return( ret );

        len = strlen( (char *) output_buf );
    }
    else
#endif
    {
        if( ( ret = mbedtls_pk_write_pubkey_der( key, output_buf, 16000 ) ) < 0 )
            return( ret );

        len = ret;
        c = output_buf + sizeof(output_buf) - len;
    }

    if( ( f = fopen( output_file, "w" ) ) == NULL )
        return( -1 );

    if( fwrite( c, 1, len, f ) != len )
    {
        fclose( f );
        return( -1 );
    }

    fclose( f );

    return( 0 );
}

static int write_private_key( mbedtls_pk_context *key, const char *output_file )
{
    int ret;
    FILE *f;
    unsigned char output_buf[16000];
    unsigned char *c = output_buf;
    size_t len = 0;

    memset(output_buf, 0, 16000);

#if defined(MBEDTLS_PEM_WRITE_C)
    if( opt.output_format == OUTPUT_FORMAT_PEM )
    {
        if( ( ret = mbedtls_pk_write_key_pem( key, output_buf, 16000 ) ) != 0 )
            return( ret );

        len = strlen( (char *) output_buf );
    }
    else
#endif
    {
        if( ( ret = mbedtls_pk_write_key_der( key, output_buf, 16000 ) ) < 0 )
            return( ret );

        len = ret;
        c = output_buf + sizeof(output_buf) - len - 1;
    }

    if( ( f = fopen( output_file, "w" ) ) == NULL )
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
    char buf[1024];
    int i;
    char *p, *q;

    mbedtls_pk_context key;
    mbedtls_mpi N, P, Q, D, E, DP, DQ, QP;

    /*
     * Set to sane values
     */
    mbedtls_pk_init( &key );
    memset( buf, 0, sizeof( buf ) );

    mbedtls_mpi_init( &N ); mbedtls_mpi_init( &P ); mbedtls_mpi_init( &Q );
    mbedtls_mpi_init( &D ); mbedtls_mpi_init( &E ); mbedtls_mpi_init( &DP );
    mbedtls_mpi_init( &DQ ); mbedtls_mpi_init( &QP );

    if( argc == 0 )
    {
    usage:
        mbedtls_printf( USAGE );
        goto exit;
    }

    opt.mode                = DFL_MODE;
    opt.filename            = DFL_FILENAME;
    opt.output_mode         = DFL_OUTPUT_MODE;
    opt.output_file         = DFL_OUTPUT_FILENAME;
    opt.output_format       = DFL_OUTPUT_FORMAT;

    for( i = 1; i < argc; i++ )
    {
        p = argv[i];
        if( ( q = strchr( p, '=' ) ) == NULL )
            goto usage;
        *q++ = '\0';

        if( strcmp( p, "mode" ) == 0 )
        {
            if( strcmp( q, "private" ) == 0 )
                opt.mode = MODE_PRIVATE;
            else if( strcmp( q, "public" ) == 0 )
                opt.mode = MODE_PUBLIC;
            else
                goto usage;
        }
        else if( strcmp( p, "output_mode" ) == 0 )
        {
            if( strcmp( q, "private" ) == 0 )
                opt.output_mode = OUTPUT_MODE_PRIVATE;
            else if( strcmp( q, "public" ) == 0 )
                opt.output_mode = OUTPUT_MODE_PUBLIC;
            else
                goto usage;
        }
        else if( strcmp( p, "output_format" ) == 0 )
        {
#if defined(MBEDTLS_PEM_WRITE_C)
            if( strcmp( q, "pem" ) == 0 )
                opt.output_format = OUTPUT_FORMAT_PEM;
            else
#endif
            if( strcmp( q, "der" ) == 0 )
                opt.output_format = OUTPUT_FORMAT_DER;
            else
                goto usage;
        }
        else if( strcmp( p, "filename" ) == 0 )
            opt.filename = q;
        else if( strcmp( p, "output_file" ) == 0 )
            opt.output_file = q;
        else
            goto usage;
    }

    if( opt.mode == MODE_NONE && opt.output_mode != OUTPUT_MODE_NONE )
    {
        mbedtls_printf( "\nCannot output a key without reading one.\n");
        goto exit;
    }

    if( opt.mode == MODE_PUBLIC && opt.output_mode == OUTPUT_MODE_PRIVATE )
    {
        mbedtls_printf( "\nCannot output a private key from a public key.\n");
        goto exit;
    }

    if( opt.mode == MODE_PRIVATE )
    {
        /*
         * 1.1. Load the key
         */
        mbedtls_printf( "\n  . Loading the private key ..." );
        fflush( stdout );

        ret = mbedtls_pk_parse_keyfile( &key, opt.filename, NULL );

        if( ret != 0 )
        {
            mbedtls_strerror( ret, (char *) buf, sizeof(buf) );
            mbedtls_printf( " failed\n  !  mbedtls_pk_parse_keyfile returned -0x%04x - %s\n\n", -ret, buf );
            goto exit;
        }

        mbedtls_printf( " ok\n" );

        /*
         * 1.2 Print the key
         */
        mbedtls_printf( "  . Key information    ...\n" );

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
            mbedtls_mpi_write_file( "Q(X): ", &ecp->Q.X, 16, NULL );
            mbedtls_mpi_write_file( "Q(Y): ", &ecp->Q.Y, 16, NULL );
            mbedtls_mpi_write_file( "Q(Z): ", &ecp->Q.Z, 16, NULL );
            mbedtls_mpi_write_file( "D   : ", &ecp->d  , 16, NULL );
        }
        else
#endif
            mbedtls_printf("key type not supported yet\n");

    }
    else if( opt.mode == MODE_PUBLIC )
    {
        /*
         * 1.1. Load the key
         */
        mbedtls_printf( "\n  . Loading the public key ..." );
        fflush( stdout );

        ret = mbedtls_pk_parse_public_keyfile( &key, opt.filename );

        if( ret != 0 )
        {
            mbedtls_strerror( ret, (char *) buf, sizeof(buf) );
            mbedtls_printf( " failed\n  !  mbedtls_pk_parse_public_key returned -0x%04x - %s\n\n", -ret, buf );
            goto exit;
        }

        mbedtls_printf( " ok\n" );

        /*
         * 1.2 Print the key
         */
        mbedtls_printf( "  . Key information    ...\n" );

#if defined(MBEDTLS_RSA_C)
        if( mbedtls_pk_get_type( &key ) == MBEDTLS_PK_RSA )
        {
            mbedtls_rsa_context *rsa = mbedtls_pk_rsa( key );

            if( ( ret = mbedtls_rsa_export( rsa, &N, NULL, NULL,
                                            NULL, &E ) ) != 0 )
            {
                mbedtls_printf( " failed\n  ! could not export RSA parameters\n\n" );
                goto exit;
            }
            mbedtls_mpi_write_file( "N: ", &N, 16, NULL );
            mbedtls_mpi_write_file( "E: ", &E, 16, NULL );
        }
        else
#endif
#if defined(MBEDTLS_ECP_C)
        if( mbedtls_pk_get_type( &key ) == MBEDTLS_PK_ECKEY )
        {
            mbedtls_ecp_keypair *ecp = mbedtls_pk_ec( key );
            mbedtls_mpi_write_file( "Q(X): ", &ecp->Q.X, 16, NULL );
            mbedtls_mpi_write_file( "Q(Y): ", &ecp->Q.Y, 16, NULL );
            mbedtls_mpi_write_file( "Q(Z): ", &ecp->Q.Z, 16, NULL );
        }
        else
#endif
            mbedtls_printf("key type not supported yet\n");
    }
    else
        goto usage;

    if( opt.output_mode == OUTPUT_MODE_PUBLIC )
    {
        write_public_key( &key, opt.output_file );
    }
    if( opt.output_mode == OUTPUT_MODE_PRIVATE )
    {
        write_private_key( &key, opt.output_file );
    }

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

#if defined(_WIN32)
    mbedtls_printf( "  + Press Enter to exit this program.\n" );
    fflush( stdout ); getchar();
#endif

    return( exit_code );
}
#endif /* MBEDTLS_PK_PARSE_C && MBEDTLS_PK_WRITE_C && MBEDTLS_FS_IO */

/*
 *  Key reading application
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

#if defined(MBEDTLS_BIGNUM_C) && \
    defined(MBEDTLS_PK_PARSE_C) && defined(MBEDTLS_FS_IO)
#include "mbedtls/error.h"
#include "mbedtls/rsa.h"
#include "mbedtls/x509.h"

#include <string.h>
#endif

#define MODE_NONE               0
#define MODE_PRIVATE            1
#define MODE_PUBLIC             2

#define DFL_MODE                MODE_NONE
#define DFL_FILENAME            "keyfile.key"
#define DFL_PASSWORD            ""
#define DFL_PASSWORD_FILE       ""
#define DFL_DEBUG_LEVEL         0

#define USAGE \
    "\n usage: key_app param=<>...\n"                   \
    "\n acceptable parameters:\n"                       \
    "    mode=private|public default: none\n"           \
    "    filename=%%s         default: keyfile.key\n"   \
    "    password=%%s         default: \"\"\n"          \
    "    password_file=%%s    default: \"\"\n"          \
    "\n"


#if !defined(MBEDTLS_BIGNUM_C) ||                                  \
    !defined(MBEDTLS_PK_PARSE_C) || !defined(MBEDTLS_FS_IO)
int main( void )
{
    mbedtls_printf("MBEDTLS_BIGNUM_C and/or "
           "MBEDTLS_PK_PARSE_C and/or MBEDTLS_FS_IO not defined.\n");
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
    const char *password;       /* password for the private key         */
    const char *password_file;  /* password_file for the private key    */
} opt;

int main( int argc, char *argv[] )
{
    int ret = 1;
    int exit_code = MBEDTLS_EXIT_FAILURE;
    char buf[1024];
    int i;
    char *p, *q;

    mbedtls_pk_context pk;
    mbedtls_mpi N, P, Q, D, E, DP, DQ, QP;

    /*
     * Set to sane values
     */
    mbedtls_pk_init( &pk );
    memset( buf, 0, sizeof(buf) );

    mbedtls_mpi_init( &N ); mbedtls_mpi_init( &P ); mbedtls_mpi_init( &Q );
    mbedtls_mpi_init( &D ); mbedtls_mpi_init( &E ); mbedtls_mpi_init( &DP );
    mbedtls_mpi_init( &DQ ); mbedtls_mpi_init( &QP );

    if( argc == 0 )
    {
    usage:
        mbedtls_printf( USAGE );
        goto cleanup;
    }

    opt.mode                = DFL_MODE;
    opt.filename            = DFL_FILENAME;
    opt.password            = DFL_PASSWORD;
    opt.password_file       = DFL_PASSWORD_FILE;

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
        else if( strcmp( p, "filename" ) == 0 )
            opt.filename = q;
        else if( strcmp( p, "password" ) == 0 )
            opt.password = q;
        else if( strcmp( p, "password_file" ) == 0 )
            opt.password_file = q;
        else
            goto usage;
    }

    if( opt.mode == MODE_PRIVATE )
    {
        if( strlen( opt.password ) && strlen( opt.password_file ) )
        {
            mbedtls_printf( "Error: cannot have both password and password_file\n" );
            goto usage;
        }

        if( strlen( opt.password_file ) )
        {
            FILE *f;

            mbedtls_printf( "\n  . Loading the password file ..." );
            if( ( f = fopen( opt.password_file, "rb" ) ) == NULL )
            {
                mbedtls_printf( " failed\n  !  fopen returned NULL\n" );
                goto cleanup;
            }
            if( fgets( buf, sizeof(buf), f ) == NULL )
            {
                fclose( f );
                mbedtls_printf( "Error: fgets() failed to retrieve password\n" );
                goto cleanup;
            }
            fclose( f );

            i = (int) strlen( buf );
            if( buf[i - 1] == '\n' ) buf[i - 1] = '\0';
            if( buf[i - 2] == '\r' ) buf[i - 2] = '\0';
            opt.password = buf;
        }

        /*
         * 1.1. Load the key
         */
        mbedtls_printf( "\n  . Loading the private key ..." );
        fflush( stdout );

        ret = mbedtls_pk_parse_keyfile( &pk, opt.filename, opt.password );

        if( ret != 0 )
        {
            mbedtls_printf( " failed\n  !  mbedtls_pk_parse_keyfile returned -0x%04x\n", -ret );
            goto cleanup;
        }

        mbedtls_printf( " ok\n" );

        /*
         * 1.2 Print the key
         */
        mbedtls_printf( "  . Key information    ...\n" );
#if defined(MBEDTLS_RSA_C)
        if( mbedtls_pk_get_type( &pk ) == MBEDTLS_PK_RSA )
        {
            mbedtls_rsa_context *rsa = mbedtls_pk_rsa( pk );

            if( ( ret = mbedtls_rsa_export    ( rsa, &N, &P, &Q, &D, &E ) ) != 0 ||
                ( ret = mbedtls_rsa_export_crt( rsa, &DP, &DQ, &QP ) )      != 0 )
            {
                mbedtls_printf( " failed\n  ! could not export RSA parameters\n\n" );
                goto cleanup;
            }

            MBEDTLS_MPI_CHK( mbedtls_mpi_write_file( "N:  ", &N, 16, NULL ) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_write_file( "E:  ", &E, 16, NULL ) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_write_file( "D:  ", &D, 16, NULL ) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_write_file( "P:  ", &P, 16, NULL ) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_write_file( "Q:  ", &Q, 16, NULL ) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_write_file( "DP: ", &DP, 16, NULL ) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_write_file( "DQ:  ", &DQ, 16, NULL ) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_write_file( "QP:  ", &QP, 16, NULL ) );
        }
        else
#endif
#if defined(MBEDTLS_ECP_C)
        if( mbedtls_pk_get_type( &pk ) == MBEDTLS_PK_ECKEY )
        {
            mbedtls_ecp_keypair *ecp = mbedtls_pk_ec( pk );
            MBEDTLS_MPI_CHK( mbedtls_mpi_write_file( "Q(X): ", &ecp->Q.X, 16, NULL ) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_write_file( "Q(Y): ", &ecp->Q.Y, 16, NULL ) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_write_file( "Q(Z): ", &ecp->Q.Z, 16, NULL ) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_write_file( "D   : ", &ecp->d  , 16, NULL ) );
        }
        else
#endif
        {
            mbedtls_printf("Do not know how to print key information for this type\n" );
            goto cleanup;
        }
    }
    else if( opt.mode == MODE_PUBLIC )
    {
        /*
         * 1.1. Load the key
         */
        mbedtls_printf( "\n  . Loading the public key ..." );
        fflush( stdout );

        ret = mbedtls_pk_parse_public_keyfile( &pk, opt.filename );

        if( ret != 0 )
        {
            mbedtls_printf( " failed\n  !  mbedtls_pk_parse_public_keyfile returned -0x%04x\n", -ret );
            goto cleanup;
        }

        mbedtls_printf( " ok\n" );

        mbedtls_printf( "  . Key information    ...\n" );
#if defined(MBEDTLS_RSA_C)
        if( mbedtls_pk_get_type( &pk ) == MBEDTLS_PK_RSA )
        {
            mbedtls_rsa_context *rsa = mbedtls_pk_rsa( pk );

            if( ( ret = mbedtls_rsa_export( rsa, &N, NULL, NULL,
                                            NULL, &E ) ) != 0 )
            {
                mbedtls_printf( " failed\n  ! could not export RSA parameters\n\n" );
                goto cleanup;
            }
            MBEDTLS_MPI_CHK( mbedtls_mpi_write_file( "N:  ", &N, 16, NULL ) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_write_file( "E:  ", &E, 16, NULL ) );
        }
        else
#endif
#if defined(MBEDTLS_ECP_C)
        if( mbedtls_pk_get_type( &pk ) == MBEDTLS_PK_ECKEY )
        {
            mbedtls_ecp_keypair *ecp = mbedtls_pk_ec( pk );
            MBEDTLS_MPI_CHK( mbedtls_mpi_write_file( "Q(X): ", &ecp->Q.X, 16, NULL ) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_write_file( "Q(Y): ", &ecp->Q.Y, 16, NULL ) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_write_file( "Q(Z): ", &ecp->Q.Z, 16, NULL ) );
        }
        else
#endif
        {
            mbedtls_printf("Do not know how to print key information for this type\n" );
            goto cleanup;
        }
    }
    else
        goto usage;

    exit_code = MBEDTLS_EXIT_SUCCESS;

cleanup:

#if defined(MBEDTLS_ERROR_C)
    if( exit_code != MBEDTLS_EXIT_SUCCESS )
    {
        mbedtls_strerror( ret, buf, sizeof( buf ) );
        mbedtls_printf( "  !  Last error was: %s\n", buf );
    }
#endif

    mbedtls_pk_free( &pk );
    mbedtls_mpi_free( &N ); mbedtls_mpi_free( &P ); mbedtls_mpi_free( &Q );
    mbedtls_mpi_free( &D ); mbedtls_mpi_free( &E ); mbedtls_mpi_free( &DP );
    mbedtls_mpi_free( &DQ ); mbedtls_mpi_free( &QP );

#if defined(_WIN32)
    mbedtls_printf( "  + Press Enter to exit this program.\n" );
    fflush( stdout ); getchar();
#endif

    return( exit_code );
}
#endif /* MBEDTLS_BIGNUM_C && MBEDTLS_PK_PARSE_C && MBEDTLS_FS_IO */

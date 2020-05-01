/*
 *  generic message digest layer demonstration program
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
#define mbedtls_fprintf         fprintf
#define mbedtls_printf          printf
#define mbedtls_exit            exit
#define MBEDTLS_EXIT_SUCCESS    EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE    EXIT_FAILURE
#endif /* MBEDTLS_PLATFORM_C */

#if defined(MBEDTLS_MD_C) && defined(MBEDTLS_FS_IO)
#include "mbedtls/md.h"

#include <stdio.h>
#include <string.h>
#endif

#if !defined(MBEDTLS_MD_C) || !defined(MBEDTLS_FS_IO)
int main( void )
{
    mbedtls_printf("MBEDTLS_MD_C and/or MBEDTLS_FS_IO not defined.\n");
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

static int generic_wrapper( const mbedtls_md_info_t *md_info, char *filename, unsigned char *sum )
{
    int ret = mbedtls_md_file( md_info, filename, sum );

    if( ret == 1 )
        mbedtls_fprintf( stderr, "failed to open: %s\n", filename );

    if( ret == 2 )
        mbedtls_fprintf( stderr, "failed to read: %s\n", filename );

    return( ret );
}

static int generic_print( const mbedtls_md_info_t *md_info, char *filename )
{
    int i;
    unsigned char sum[MBEDTLS_MD_MAX_SIZE];

    if( generic_wrapper( md_info, filename, sum ) != 0 )
        return( 1 );

    for( i = 0; i < mbedtls_md_get_size( md_info ); i++ )
        mbedtls_printf( "%02x", sum[i] );

    mbedtls_printf( "  %s\n", filename );
    return( 0 );
}

static int generic_check( const mbedtls_md_info_t *md_info, char *filename )
{
    int i;
    size_t n;
    FILE *f;
    int nb_err1, nb_err2;
    int nb_tot1, nb_tot2;
    unsigned char sum[MBEDTLS_MD_MAX_SIZE];
    char line[1024];
    char diff;
#if defined(__clang_analyzer__)
    char buf[MBEDTLS_MD_MAX_SIZE * 2 + 1] = { };
#else
    char buf[MBEDTLS_MD_MAX_SIZE * 2 + 1];
#endif

    if( ( f = fopen( filename, "rb" ) ) == NULL )
    {
        mbedtls_printf( "failed to open: %s\n", filename );
        return( 1 );
    }

    nb_err1 = nb_err2 = 0;
    nb_tot1 = nb_tot2 = 0;

    memset( line, 0, sizeof( line ) );

    n = sizeof( line );

    while( fgets( line, (int) n - 1, f ) != NULL )
    {
        n = strlen( line );

        if( n < (size_t) 2 * mbedtls_md_get_size( md_info ) + 4 )
        {
            mbedtls_printf("No '%s' hash found on line.\n", mbedtls_md_get_name( md_info ));
            continue;
        }

        if( line[2 * mbedtls_md_get_size( md_info )] != ' ' || line[2 * mbedtls_md_get_size( md_info ) + 1] != ' ' )
        {
            mbedtls_printf("No '%s' hash found on line.\n", mbedtls_md_get_name( md_info ));
            continue;
        }

        if( line[n - 1] == '\n' ) { n--; line[n] = '\0'; }
        if( line[n - 1] == '\r' ) { n--; line[n] = '\0'; }

        nb_tot1++;

        if( generic_wrapper( md_info, line + 2 + 2 * mbedtls_md_get_size( md_info ), sum ) != 0 )
        {
            nb_err1++;
            continue;
        }

        nb_tot2++;

        for( i = 0; i < mbedtls_md_get_size( md_info ); i++ )
            sprintf( buf + i * 2, "%02x", sum[i] );

        /* Use constant-time buffer comparison */
        diff = 0;
        for( i = 0; i < 2 * mbedtls_md_get_size( md_info ); i++ )
            diff |= line[i] ^ buf[i];

        if( diff != 0 )
        {
            nb_err2++;
            mbedtls_fprintf( stderr, "wrong checksum: %s\n", line + 66 );
        }

        n = sizeof( line );
    }

    if( nb_err1 != 0 )
    {
        mbedtls_printf( "WARNING: %d (out of %d) input files could "
                "not be read\n", nb_err1, nb_tot1 );
    }

    if( nb_err2 != 0 )
    {
        mbedtls_printf( "WARNING: %d (out of %d) computed checksums did "
                "not match\n", nb_err2, nb_tot2 );
    }

    fclose( f );

    return( nb_err1 != 0 || nb_err2 != 0 );
}

int main( int argc, char *argv[] )
{
    int ret = 1, i;
    int exit_code = MBEDTLS_EXIT_FAILURE;
    const mbedtls_md_info_t *md_info;
    mbedtls_md_context_t md_ctx;

    mbedtls_md_init( &md_ctx );

    if( argc == 1 )
    {
        const int *list;

        mbedtls_printf( "print mode:  generic_sum <mbedtls_md> <file> <file> ...\n" );
        mbedtls_printf( "check mode:  generic_sum <mbedtls_md> -c <checksum file>\n" );

        mbedtls_printf( "\nAvailable message digests:\n" );
        list = mbedtls_md_list();
        while( *list )
        {
            md_info = mbedtls_md_info_from_type( *list );
            mbedtls_printf( "  %s\n", mbedtls_md_get_name( md_info ) );
            list++;
        }

#if defined(_WIN32)
        mbedtls_printf( "\n  Press Enter to exit this program.\n" );
        fflush( stdout ); getchar();
#endif

        return( exit_code );
    }

    /*
     * Read the MD from the command line
     */
    md_info = mbedtls_md_info_from_string( argv[1] );
    if( md_info == NULL )
    {
        mbedtls_fprintf( stderr, "Message Digest '%s' not found\n", argv[1] );
        return( exit_code );
    }
    if( mbedtls_md_setup( &md_ctx, md_info, 0 ) )
    {
        mbedtls_fprintf( stderr, "Failed to initialize context.\n" );
        return( exit_code );
    }

    ret = 0;
    if( argc == 4 && strcmp( "-c", argv[2] ) == 0 )
    {
        ret |= generic_check( md_info, argv[3] );
        goto exit;
    }

    for( i = 2; i < argc; i++ )
        ret |= generic_print( md_info, argv[i] );

    if ( ret == 0 )
        exit_code = MBEDTLS_EXIT_SUCCESS;

exit:
    mbedtls_md_free( &md_ctx );

    return( exit_code );
}
#endif /* MBEDTLS_MD_C && MBEDTLS_FS_IO */

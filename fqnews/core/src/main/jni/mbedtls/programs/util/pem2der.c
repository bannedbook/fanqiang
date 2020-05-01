/*
 *  Convert PEM to DER
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
#define mbedtls_free            free
#define mbedtls_calloc          calloc
#define mbedtls_printf          printf
#define mbedtls_exit            exit
#define MBEDTLS_EXIT_SUCCESS    EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE    EXIT_FAILURE
#endif /* MBEDTLS_PLATFORM_C */

#if defined(MBEDTLS_BASE64_C) && defined(MBEDTLS_FS_IO)
#include "mbedtls/error.h"
#include "mbedtls/base64.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#define DFL_FILENAME            "file.pem"
#define DFL_OUTPUT_FILENAME     "file.der"

#define USAGE \
    "\n usage: pem2der param=<>...\n"                   \
    "\n acceptable parameters:\n"                       \
    "    filename=%%s         default: file.pem\n"      \
    "    output_file=%%s      default: file.der\n"      \
    "\n"

#if !defined(MBEDTLS_BASE64_C) || !defined(MBEDTLS_FS_IO)
int main( void )
{
    mbedtls_printf("MBEDTLS_BASE64_C and/or MBEDTLS_FS_IO not defined.\n");
    return( 0 );
}
#else

#if defined(MBEDTLS_CHECK_PARAMS)
#define mbedtls_exit            exit
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
    const char *filename;       /* filename of the input file             */
    const char *output_file;    /* where to store the output              */
} opt;

int convert_pem_to_der( const unsigned char *input, size_t ilen,
                        unsigned char *output, size_t *olen )
{
    int ret;
    const unsigned char *s1, *s2, *end = input + ilen;
    size_t len = 0;

    s1 = (unsigned char *) strstr( (const char *) input, "-----BEGIN" );
    if( s1 == NULL )
        return( -1 );

    s2 = (unsigned char *) strstr( (const char *) input, "-----END" );
    if( s2 == NULL )
        return( -1 );

    s1 += 10;
    while( s1 < end && *s1 != '-' )
        s1++;
    while( s1 < end && *s1 == '-' )
        s1++;
    if( *s1 == '\r' ) s1++;
    if( *s1 == '\n' ) s1++;

    if( s2 <= s1 || s2 > end )
        return( -1 );

    ret = mbedtls_base64_decode( NULL, 0, &len, (const unsigned char *) s1, s2 - s1 );
    if( ret == MBEDTLS_ERR_BASE64_INVALID_CHARACTER )
        return( ret );

    if( len > *olen )
        return( -1 );

    if( ( ret = mbedtls_base64_decode( output, len, &len, (const unsigned char *) s1,
                               s2 - s1 ) ) != 0 )
    {
        return( ret );
    }

    *olen = len;

    return( 0 );
}

/*
 * Load all data from a file into a given buffer.
 */
static int load_file( const char *path, unsigned char **buf, size_t *n )
{
    FILE *f;
    long size;

    if( ( f = fopen( path, "rb" ) ) == NULL )
        return( -1 );

    fseek( f, 0, SEEK_END );
    if( ( size = ftell( f ) ) == -1 )
    {
        fclose( f );
        return( -1 );
    }
    fseek( f, 0, SEEK_SET );

    *n = (size_t) size;

    if( *n + 1 == 0 ||
        ( *buf = mbedtls_calloc( 1, *n + 1 ) ) == NULL )
    {
        fclose( f );
        return( -1 );
    }

    if( fread( *buf, 1, *n, f ) != *n )
    {
        fclose( f );
        free( *buf );
        *buf = NULL;
        return( -1 );
    }

    fclose( f );

    (*buf)[*n] = '\0';

    return( 0 );
}

/*
 * Write buffer to a file
 */
static int write_file( const char *path, unsigned char *buf, size_t n )
{
    FILE *f;

    if( ( f = fopen( path, "wb" ) ) == NULL )
        return( -1 );

    if( fwrite( buf, 1, n, f ) != n )
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
    unsigned char *pem_buffer = NULL;
    unsigned char der_buffer[4096];
    char buf[1024];
    size_t pem_size, der_size = sizeof(der_buffer);
    int i;
    char *p, *q;

    /*
     * Set to sane values
     */
    memset( buf, 0, sizeof(buf) );
    memset( der_buffer, 0, sizeof(der_buffer) );

    if( argc == 0 )
    {
    usage:
        mbedtls_printf( USAGE );
        goto exit;
    }

    opt.filename            = DFL_FILENAME;
    opt.output_file         = DFL_OUTPUT_FILENAME;

    for( i = 1; i < argc; i++ )
    {

        p = argv[i];
        if( ( q = strchr( p, '=' ) ) == NULL )
            goto usage;
        *q++ = '\0';

        if( strcmp( p, "filename" ) == 0 )
            opt.filename = q;
        else if( strcmp( p, "output_file" ) == 0 )
            opt.output_file = q;
        else
            goto usage;
    }

    /*
     * 1.1. Load the PEM file
     */
    mbedtls_printf( "\n  . Loading the PEM file ..." );
    fflush( stdout );

    ret = load_file( opt.filename, &pem_buffer, &pem_size );

    if( ret != 0 )
    {
#ifdef MBEDTLS_ERROR_C
        mbedtls_strerror( ret, buf, 1024 );
#endif
        mbedtls_printf( " failed\n  !  load_file returned %d - %s\n\n", ret, buf );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    /*
     * 1.2. Convert from PEM to DER
     */
    mbedtls_printf( "  . Converting from PEM to DER ..." );
    fflush( stdout );

    if( ( ret = convert_pem_to_der( pem_buffer, pem_size, der_buffer, &der_size ) ) != 0 )
    {
#ifdef MBEDTLS_ERROR_C
        mbedtls_strerror( ret, buf, 1024 );
#endif
        mbedtls_printf( " failed\n  !  convert_pem_to_der %d - %s\n\n", ret, buf );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    /*
     * 1.3. Write the DER file
     */
    mbedtls_printf( "  . Writing the DER file ..." );
    fflush( stdout );

    ret = write_file( opt.output_file, der_buffer, der_size );

    if( ret != 0 )
    {
#ifdef MBEDTLS_ERROR_C
        mbedtls_strerror( ret, buf, 1024 );
#endif
        mbedtls_printf( " failed\n  !  write_file returned %d - %s\n\n", ret, buf );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    exit_code = MBEDTLS_EXIT_SUCCESS;

exit:
    free( pem_buffer );

#if defined(_WIN32)
    mbedtls_printf( "  + Press Enter to exit this program.\n" );
    fflush( stdout ); getchar();
#endif

    return( exit_code );
}
#endif /* MBEDTLS_BASE64_C && MBEDTLS_FS_IO */

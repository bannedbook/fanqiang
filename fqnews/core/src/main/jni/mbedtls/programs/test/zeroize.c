/*
 * Zeroize application for debugger-driven testing
 *
 * This is a simple test application used for debugger-driven testing to check
 * whether calls to mbedtls_platform_zeroize() are being eliminated by compiler
 * optimizations. This application is used by the GDB script at
 * tests/scripts/test_zeroize.gdb under the assumption that the code does not
 * change often (as opposed to the library code) because the script sets a
 * breakpoint at the last return statement in the main() function of this
 * program. The debugger facilities are then used to manually inspect the
 * memory and verify that the call to mbedtls_platform_zeroize() was not
 * eliminated.
 *
 *  Copyright (C) 2018, Arm Limited, All Rights Reserved
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

#include <stdio.h>

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdlib.h>
#define mbedtls_printf     printf
#define MBEDTLS_EXIT_SUCCESS EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE EXIT_FAILURE
#endif

#include "mbedtls/platform_util.h"

#define BUFFER_LEN 1024

void usage( void )
{
    mbedtls_printf( "Zeroize is a simple program to assist with testing\n" );
    mbedtls_printf( "the mbedtls_platform_zeroize() function by using the\n" );
    mbedtls_printf( "debugger. This program takes a file as input and\n" );
    mbedtls_printf( "prints the first %d characters. Usage:\n\n", BUFFER_LEN );
    mbedtls_printf( "       zeroize <FILE>\n" );
}

int main( int argc, char** argv )
{
    int exit_code = MBEDTLS_EXIT_FAILURE;
    FILE *fp;
    char buf[BUFFER_LEN];
    char *p = buf;
    char *end = p + BUFFER_LEN;
    int c;

    if( argc != 2 )
    {
        mbedtls_printf( "This program takes exactly 1 agument\n" );
        usage();
        return( exit_code );
    }

    fp = fopen( argv[1], "r" );
    if( fp == NULL )
    {
        mbedtls_printf( "Could not open file '%s'\n", argv[1] );
        return( exit_code );
    }

    while( ( c = fgetc( fp ) ) != EOF && p < end - 1 )
        *p++ = (char)c;
    *p = '\0';

    if( p - buf != 0 )
    {
        mbedtls_printf( "%s\n", buf );
        exit_code = MBEDTLS_EXIT_SUCCESS;
    }
    else
        mbedtls_printf( "The file is empty!\n" );

    fclose( fp );
    mbedtls_platform_zeroize( buf, sizeof( buf ) );

    return( exit_code );
}

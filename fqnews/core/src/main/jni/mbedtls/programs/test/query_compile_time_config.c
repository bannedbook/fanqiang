/*
 *  Query the Mbed TLS compile time configuration
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
 *  This file is part of Mbed TLS (https://tls.mbed.org)
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
#define mbedtls_printf       printf
#define MBEDTLS_EXIT_FAILURE EXIT_FAILURE
#endif

#define USAGE                                                                \
    "usage: %s <MBEDTLS_CONFIG>\n\n"                                         \
    "This program takes one command line argument which corresponds to\n"    \
    "the string representation of a Mbed TLS compile time configuration.\n"  \
    "The value 0 will be returned if this configuration is defined in the\n" \
    "Mbed TLS build and the macro expansion of that configuration will be\n" \
    "printed (if any). Otherwise, 1 will be returned.\n"

int query_config( const char *config );

int main( int argc, char *argv[] )
{
    if ( argc != 2 )
    {
        mbedtls_printf( USAGE, argv[0] );
        return( MBEDTLS_EXIT_FAILURE );
    }

    return( query_config( argv[1] ) );
}

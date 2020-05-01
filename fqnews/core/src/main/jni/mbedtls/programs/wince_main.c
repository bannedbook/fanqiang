/*
 *  Windows CE console application entry point
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

#if defined(_WIN32_WCE)

#include <windows.h>

extern int main( int, const char ** );

int _tmain( int argc, _TCHAR* targv[] )
{
    char **argv;
    int i;

    argv = ( char ** ) calloc( argc, sizeof( char * ) );

    for ( i = 0; i < argc; i++ ) {
        size_t len;
        len = _tcslen( targv[i] ) + 1;
        argv[i] = ( char * ) calloc( len, sizeof( char ) );
        wcstombs( argv[i], targv[i], len );
    }

    return main( argc, argv );
}

#endif  /* defined(_WIN32_WCE) */

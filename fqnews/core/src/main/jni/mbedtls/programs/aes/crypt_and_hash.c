/*
 *  \brief  Generic file encryption program using generic wrappers for configured
 *          security.
 *
 *  Copyright (C) 2006-2016, ARM Limited, All Rights Reserved
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

/* Enable definition of fileno() even when compiling with -std=c99. Must be
 * set before config.h, which pulls in glibc's features.h indirectly.
 * Harmless on other platforms. */
#define _POSIX_C_SOURCE 1

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

#if defined(MBEDTLS_CIPHER_C) && defined(MBEDTLS_MD_C) && \
 defined(MBEDTLS_FS_IO)
#include "mbedtls/cipher.h"
#include "mbedtls/md.h"
#include "mbedtls/platform_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#if !defined(_WIN32_WCE)
#include <io.h>
#endif
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#define MODE_ENCRYPT    0
#define MODE_DECRYPT    1

#define USAGE   \
    "\n  crypt_and_hash <mode> <input filename> <output filename> <cipher> <mbedtls_md> <key>\n" \
    "\n   <mode>: 0 = encrypt, 1 = decrypt\n" \
    "\n  example: crypt_and_hash 0 file file.aes AES-128-CBC SHA1 hex:E76B2413958B00E193\n" \
    "\n"

#if !defined(MBEDTLS_CIPHER_C) || !defined(MBEDTLS_MD_C) || \
    !defined(MBEDTLS_FS_IO)
int main( void )
{
    mbedtls_printf("MBEDTLS_CIPHER_C and/or MBEDTLS_MD_C and/or MBEDTLS_FS_IO not defined.\n");
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

int main( int argc, char *argv[] )
{
    int ret = 1, i, n;
    int exit_code = MBEDTLS_EXIT_FAILURE;
    int mode;
    size_t keylen, ilen, olen;
    FILE *fkey, *fin = NULL, *fout = NULL;

    char *p;
    unsigned char IV[16];
    unsigned char key[512];
    unsigned char digest[MBEDTLS_MD_MAX_SIZE];
    unsigned char buffer[1024];
    unsigned char output[1024];
    unsigned char diff;

    const mbedtls_cipher_info_t *cipher_info;
    const mbedtls_md_info_t *md_info;
    mbedtls_cipher_context_t cipher_ctx;
    mbedtls_md_context_t md_ctx;
#if defined(_WIN32_WCE)
    long filesize, offset;
#elif defined(_WIN32)
       LARGE_INTEGER li_size;
    __int64 filesize, offset;
#else
      off_t filesize, offset;
#endif

    mbedtls_cipher_init( &cipher_ctx );
    mbedtls_md_init( &md_ctx );

    /*
     * Parse the command-line arguments.
     */
    if( argc != 7 )
    {
        const int *list;

        mbedtls_printf( USAGE );

        mbedtls_printf( "Available ciphers:\n" );
        list = mbedtls_cipher_list();
        while( *list )
        {
            cipher_info = mbedtls_cipher_info_from_type( *list );
            mbedtls_printf( "  %s\n", cipher_info->name );
            list++;
        }

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

        goto exit;
    }

    mode = atoi( argv[1] );

    if( mode != MODE_ENCRYPT && mode != MODE_DECRYPT )
    {
        mbedtls_fprintf( stderr, "invalid operation mode\n" );
        goto exit;
    }

    if( strcmp( argv[2], argv[3] ) == 0 )
    {
        mbedtls_fprintf( stderr, "input and output filenames must differ\n" );
        goto exit;
    }

    if( ( fin = fopen( argv[2], "rb" ) ) == NULL )
    {
        mbedtls_fprintf( stderr, "fopen(%s,rb) failed\n", argv[2] );
        goto exit;
    }

    if( ( fout = fopen( argv[3], "wb+" ) ) == NULL )
    {
        mbedtls_fprintf( stderr, "fopen(%s,wb+) failed\n", argv[3] );
        goto exit;
    }

    /*
     * Read the Cipher and MD from the command line
     */
    cipher_info = mbedtls_cipher_info_from_string( argv[4] );
    if( cipher_info == NULL )
    {
        mbedtls_fprintf( stderr, "Cipher '%s' not found\n", argv[4] );
        goto exit;
    }
    if( ( ret = mbedtls_cipher_setup( &cipher_ctx, cipher_info) ) != 0 )
    {
        mbedtls_fprintf( stderr, "mbedtls_cipher_setup failed\n" );
        goto exit;
    }

    md_info = mbedtls_md_info_from_string( argv[5] );
    if( md_info == NULL )
    {
        mbedtls_fprintf( stderr, "Message Digest '%s' not found\n", argv[5] );
        goto exit;
    }

    if( mbedtls_md_setup( &md_ctx, md_info, 1 ) != 0 )
    {
        mbedtls_fprintf( stderr, "mbedtls_md_setup failed\n" );
        goto exit;
    }

    /*
     * Read the secret key from file or command line
     */
    if( ( fkey = fopen( argv[6], "rb" ) ) != NULL )
    {
        keylen = fread( key, 1, sizeof( key ), fkey );
        fclose( fkey );
    }
    else
    {
        if( memcmp( argv[6], "hex:", 4 ) == 0 )
        {
            p = &argv[6][4];
            keylen = 0;

            while( sscanf( p, "%02X", &n ) > 0 &&
                   keylen < (int) sizeof( key ) )
            {
                key[keylen++] = (unsigned char) n;
                p += 2;
            }
        }
        else
        {
            keylen = strlen( argv[6] );

            if( keylen > (int) sizeof( key ) )
                keylen = (int) sizeof( key );

            memcpy( key, argv[6], keylen );
        }
    }

#if defined(_WIN32_WCE)
    filesize = fseek( fin, 0L, SEEK_END );
#else
#if defined(_WIN32)
    /*
     * Support large files (> 2Gb) on Win32
     */
    li_size.QuadPart = 0;
    li_size.LowPart  =
        SetFilePointer( (HANDLE) _get_osfhandle( _fileno( fin ) ),
                        li_size.LowPart, &li_size.HighPart, FILE_END );

    if( li_size.LowPart == 0xFFFFFFFF && GetLastError() != NO_ERROR )
    {
        mbedtls_fprintf( stderr, "SetFilePointer(0,FILE_END) failed\n" );
        goto exit;
    }

    filesize = li_size.QuadPart;
#else
    if( ( filesize = lseek( fileno( fin ), 0, SEEK_END ) ) < 0 )
    {
        perror( "lseek" );
        goto exit;
    }
#endif
#endif

    if( fseek( fin, 0, SEEK_SET ) < 0 )
    {
        mbedtls_fprintf( stderr, "fseek(0,SEEK_SET) failed\n" );
        goto exit;
    }

    if( mode == MODE_ENCRYPT )
    {
        /*
         * Generate the initialization vector as:
         * IV = MD( filesize || filename )[0..15]
         */
        for( i = 0; i < 8; i++ )
            buffer[i] = (unsigned char)( filesize >> ( i << 3 ) );

        p = argv[2];

        mbedtls_md_starts( &md_ctx );
        mbedtls_md_update( &md_ctx, buffer, 8 );
        mbedtls_md_update( &md_ctx, (unsigned char *) p, strlen( p ) );
        mbedtls_md_finish( &md_ctx, digest );

        memcpy( IV, digest, 16 );

        /*
         * Append the IV at the beginning of the output.
         */
        if( fwrite( IV, 1, 16, fout ) != 16 )
        {
            mbedtls_fprintf( stderr, "fwrite(%d bytes) failed\n", 16 );
            goto exit;
        }

        /*
         * Hash the IV and the secret key together 8192 times
         * using the result to setup the AES context and HMAC.
         */
        memset( digest, 0,  32 );
        memcpy( digest, IV, 16 );

        for( i = 0; i < 8192; i++ )
        {
            mbedtls_md_starts( &md_ctx );
            mbedtls_md_update( &md_ctx, digest, 32 );
            mbedtls_md_update( &md_ctx, key, keylen );
            mbedtls_md_finish( &md_ctx, digest );

        }

        if( mbedtls_cipher_setkey( &cipher_ctx, digest, cipher_info->key_bitlen,
                           MBEDTLS_ENCRYPT ) != 0 )
        {
            mbedtls_fprintf( stderr, "mbedtls_cipher_setkey() returned error\n");
            goto exit;
        }
        if( mbedtls_cipher_set_iv( &cipher_ctx, IV, 16 ) != 0 )
        {
            mbedtls_fprintf( stderr, "mbedtls_cipher_set_iv() returned error\n");
            goto exit;
        }
        if( mbedtls_cipher_reset( &cipher_ctx ) != 0 )
        {
            mbedtls_fprintf( stderr, "mbedtls_cipher_reset() returned error\n");
            goto exit;
        }

        mbedtls_md_hmac_starts( &md_ctx, digest, 32 );

        /*
         * Encrypt and write the ciphertext.
         */
        for( offset = 0; offset < filesize; offset += mbedtls_cipher_get_block_size( &cipher_ctx ) )
        {
            ilen = ( (unsigned int) filesize - offset > mbedtls_cipher_get_block_size( &cipher_ctx ) ) ?
                mbedtls_cipher_get_block_size( &cipher_ctx ) : (unsigned int) ( filesize - offset );

            if( fread( buffer, 1, ilen, fin ) != ilen )
            {
                mbedtls_fprintf( stderr, "fread(%ld bytes) failed\n", (long) ilen );
                goto exit;
            }

            if( mbedtls_cipher_update( &cipher_ctx, buffer, ilen, output, &olen ) != 0 )
            {
                mbedtls_fprintf( stderr, "mbedtls_cipher_update() returned error\n");
                goto exit;
            }

            mbedtls_md_hmac_update( &md_ctx, output, olen );

            if( fwrite( output, 1, olen, fout ) != olen )
            {
                mbedtls_fprintf( stderr, "fwrite(%ld bytes) failed\n", (long) olen );
                goto exit;
            }
        }

        if( mbedtls_cipher_finish( &cipher_ctx, output, &olen ) != 0 )
        {
            mbedtls_fprintf( stderr, "mbedtls_cipher_finish() returned error\n" );
            goto exit;
        }
        mbedtls_md_hmac_update( &md_ctx, output, olen );

        if( fwrite( output, 1, olen, fout ) != olen )
        {
            mbedtls_fprintf( stderr, "fwrite(%ld bytes) failed\n", (long) olen );
            goto exit;
        }

        /*
         * Finally write the HMAC.
         */
        mbedtls_md_hmac_finish( &md_ctx, digest );

        if( fwrite( digest, 1, mbedtls_md_get_size( md_info ), fout ) != mbedtls_md_get_size( md_info ) )
        {
            mbedtls_fprintf( stderr, "fwrite(%d bytes) failed\n", mbedtls_md_get_size( md_info ) );
            goto exit;
        }
    }

    if( mode == MODE_DECRYPT )
    {
        /*
         *  The encrypted file must be structured as follows:
         *
         *        00 .. 15              Initialization Vector
         *        16 .. 31              Encrypted Block #1
         *           ..
         *      N*16 .. (N+1)*16 - 1    Encrypted Block #N
         *  (N+1)*16 .. (N+1)*16 + n    Hash(ciphertext)
         */
        if( filesize < 16 + mbedtls_md_get_size( md_info ) )
        {
            mbedtls_fprintf( stderr, "File too short to be encrypted.\n" );
            goto exit;
        }

        if( mbedtls_cipher_get_block_size( &cipher_ctx ) == 0 )
        {
            mbedtls_fprintf( stderr, "Invalid cipher block size: 0. \n" );
            goto exit;
        }

        /*
         * Check the file size.
         */
        if( cipher_info->mode != MBEDTLS_MODE_GCM &&
            ( ( filesize - mbedtls_md_get_size( md_info ) ) %
                mbedtls_cipher_get_block_size( &cipher_ctx ) ) != 0 )
        {
            mbedtls_fprintf( stderr, "File content not a multiple of the block size (%d).\n",
                     mbedtls_cipher_get_block_size( &cipher_ctx ));
            goto exit;
        }

        /*
         * Subtract the IV + HMAC length.
         */
        filesize -= ( 16 + mbedtls_md_get_size( md_info ) );

        /*
         * Read the IV and original filesize modulo 16.
         */
        if( fread( buffer, 1, 16, fin ) != 16 )
        {
            mbedtls_fprintf( stderr, "fread(%d bytes) failed\n", 16 );
            goto exit;
        }

        memcpy( IV, buffer, 16 );

        /*
         * Hash the IV and the secret key together 8192 times
         * using the result to setup the AES context and HMAC.
         */
        memset( digest, 0,  32 );
        memcpy( digest, IV, 16 );

        for( i = 0; i < 8192; i++ )
        {
            mbedtls_md_starts( &md_ctx );
            mbedtls_md_update( &md_ctx, digest, 32 );
            mbedtls_md_update( &md_ctx, key, keylen );
            mbedtls_md_finish( &md_ctx, digest );
        }

        if( mbedtls_cipher_setkey( &cipher_ctx, digest, cipher_info->key_bitlen,
                           MBEDTLS_DECRYPT ) != 0 )
        {
            mbedtls_fprintf( stderr, "mbedtls_cipher_setkey() returned error\n" );
            goto exit;
        }

        if( mbedtls_cipher_set_iv( &cipher_ctx, IV, 16 ) != 0 )
        {
            mbedtls_fprintf( stderr, "mbedtls_cipher_set_iv() returned error\n" );
            goto exit;
        }

        if( mbedtls_cipher_reset( &cipher_ctx ) != 0 )
        {
            mbedtls_fprintf( stderr, "mbedtls_cipher_reset() returned error\n" );
            goto exit;
        }

        mbedtls_md_hmac_starts( &md_ctx, digest, 32 );

        /*
         * Decrypt and write the plaintext.
         */
        for( offset = 0; offset < filesize; offset += mbedtls_cipher_get_block_size( &cipher_ctx ) )
        {
            ilen = ( (unsigned int) filesize - offset > mbedtls_cipher_get_block_size( &cipher_ctx ) ) ?
                mbedtls_cipher_get_block_size( &cipher_ctx ) : (unsigned int) ( filesize - offset );

            if( fread( buffer, 1, ilen, fin ) != ilen )
            {
                mbedtls_fprintf( stderr, "fread(%d bytes) failed\n",
                    mbedtls_cipher_get_block_size( &cipher_ctx ) );
                goto exit;
            }

            mbedtls_md_hmac_update( &md_ctx, buffer, ilen );
            if( mbedtls_cipher_update( &cipher_ctx, buffer, ilen, output,
                                       &olen ) != 0 )
            {
                mbedtls_fprintf( stderr, "mbedtls_cipher_update() returned error\n" );
                goto exit;
            }

            if( fwrite( output, 1, olen, fout ) != olen )
            {
                mbedtls_fprintf( stderr, "fwrite(%ld bytes) failed\n", (long) olen );
                goto exit;
            }
        }

        /*
         * Verify the message authentication code.
         */
        mbedtls_md_hmac_finish( &md_ctx, digest );

        if( fread( buffer, 1, mbedtls_md_get_size( md_info ), fin ) != mbedtls_md_get_size( md_info ) )
        {
            mbedtls_fprintf( stderr, "fread(%d bytes) failed\n", mbedtls_md_get_size( md_info ) );
            goto exit;
        }

        /* Use constant-time buffer comparison */
        diff = 0;
        for( i = 0; i < mbedtls_md_get_size( md_info ); i++ )
            diff |= digest[i] ^ buffer[i];

        if( diff != 0 )
        {
            mbedtls_fprintf( stderr, "HMAC check failed: wrong key, "
                             "or file corrupted.\n" );
            goto exit;
        }

        /*
         * Write the final block of data
         */
        mbedtls_cipher_finish( &cipher_ctx, output, &olen );

        if( fwrite( output, 1, olen, fout ) != olen )
        {
            mbedtls_fprintf( stderr, "fwrite(%ld bytes) failed\n", (long) olen );
            goto exit;
        }
    }

    exit_code = MBEDTLS_EXIT_SUCCESS;

exit:
    if( fin )
        fclose( fin );
    if( fout )
        fclose( fout );

    /* Zeroize all command line arguments to also cover
       the case when the user has missed or reordered some,
       in which case the key might not be in argv[6]. */
    for( i = 0; i < argc; i++ )
        mbedtls_platform_zeroize( argv[i], strlen( argv[i] ) );

    mbedtls_platform_zeroize( IV,     sizeof( IV ) );
    mbedtls_platform_zeroize( key,    sizeof( key ) );
    mbedtls_platform_zeroize( buffer, sizeof( buffer ) );
    mbedtls_platform_zeroize( output, sizeof( output ) );
    mbedtls_platform_zeroize( digest, sizeof( digest ) );

    mbedtls_cipher_free( &cipher_ctx );
    mbedtls_md_free( &md_ctx );

    return( exit_code );
}
#endif /* MBEDTLS_CIPHER_C && MBEDTLS_MD_C && MBEDTLS_FS_IO */

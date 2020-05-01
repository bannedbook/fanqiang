/*
 *  SSL certificate functionality tests
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
#define mbedtls_snprintf        snprintf
#define mbedtls_printf          printf
#define mbedtls_exit            exit
#define MBEDTLS_EXIT_SUCCESS    EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE    EXIT_FAILURE
#endif /* MBEDTLS_PLATFORM_C */

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_X509_CRT_PARSE_C) && \
    defined(MBEDTLS_FS_IO) && defined(MBEDTLS_X509_CRL_PARSE_C)
#include "mbedtls/certs.h"
#include "mbedtls/x509_crt.h"

#include <stdio.h>
#include <string.h>
#endif

#define MAX_CLIENT_CERTS    8

#if !defined(MBEDTLS_RSA_C) || !defined(MBEDTLS_X509_CRT_PARSE_C) || \
    !defined(MBEDTLS_FS_IO) || !defined(MBEDTLS_X509_CRL_PARSE_C)
int main( void )
{
    mbedtls_printf("MBEDTLS_RSA_C and/or MBEDTLS_X509_CRT_PARSE_C "
           "MBEDTLS_FS_IO and/or MBEDTLS_X509_CRL_PARSE_C "
           "not defined.\n");
    return( 0 );
}
#else
const char *client_certificates[MAX_CLIENT_CERTS] =
{
    "client1.crt",
    "client2.crt",
    "server1.crt",
    "server2.crt",
    "cert_sha224.crt",
    "cert_sha256.crt",
    "cert_sha384.crt",
    "cert_sha512.crt"
};

const char *client_private_keys[MAX_CLIENT_CERTS] =
{
    "client1.key",
    "client2.key",
    "server1.key",
    "server2.key",
    "cert_digest.key",
    "cert_digest.key",
    "cert_digest.key",
    "cert_digest.key"
};

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
    int ret = 1, i;
    int exit_code = MBEDTLS_EXIT_FAILURE;
    mbedtls_x509_crt cacert;
    mbedtls_x509_crl crl;
    char buf[10240];

    mbedtls_x509_crt_init( &cacert );
    mbedtls_x509_crl_init( &crl );

    /*
     * 1.1. Load the trusted CA
     */
    mbedtls_printf( "\n  . Loading the CA root certificate ..." );
    fflush( stdout );

    /*
     * Alternatively, you may load the CA certificates from a .pem or
     * .crt file by calling mbedtls_x509_crt_parse_file( &cacert, "myca.crt" ).
     */
    ret = mbedtls_x509_crt_parse_file( &cacert, "ssl/test-ca/test-ca.crt" );
    if( ret != 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_x509_crt_parse_file returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    mbedtls_x509_crt_info( buf, 1024, "CRT: ", &cacert );
    mbedtls_printf("%s\n", buf );

    /*
     * 1.2. Load the CRL
     */
    mbedtls_printf( "  . Loading the CRL ..." );
    fflush( stdout );

    ret = mbedtls_x509_crl_parse_file( &crl, "ssl/test-ca/crl.pem" );
    if( ret != 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_x509_crl_parse_file returned %d\n\n", ret );
        goto exit;
    }

    mbedtls_printf( " ok\n" );

    mbedtls_x509_crl_info( buf, 1024, "CRL: ", &crl );
    mbedtls_printf("%s\n", buf );

    for( i = 0; i < MAX_CLIENT_CERTS; i++ )
    {
        /*
         * 1.3. Load own certificate
         */
        char    name[512];
        uint32_t flags;
        mbedtls_x509_crt clicert;
        mbedtls_pk_context pk;

        mbedtls_x509_crt_init( &clicert );
        mbedtls_pk_init( &pk );

        mbedtls_snprintf(name, 512, "ssl/test-ca/%s", client_certificates[i]);

        mbedtls_printf( "  . Loading the client certificate %s...", name );
        fflush( stdout );

        ret = mbedtls_x509_crt_parse_file( &clicert, name );
        if( ret != 0 )
        {
            mbedtls_printf( " failed\n  !  mbedtls_x509_crt_parse_file returned %d\n\n", ret );
            goto exit;
        }

        mbedtls_printf( " ok\n" );

        /*
         * 1.4. Verify certificate validity with CA certificate
         */
        mbedtls_printf( "  . Verify the client certificate with CA certificate..." );
        fflush( stdout );

        ret = mbedtls_x509_crt_verify( &clicert, &cacert, &crl, NULL, &flags, NULL,
                               NULL );
        if( ret != 0 )
        {
            if( ret == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED )
            {
                 char vrfy_buf[512];

                 mbedtls_printf( " failed\n" );
                 mbedtls_x509_crt_verify_info( vrfy_buf, sizeof( vrfy_buf ), "  ! ", flags );
                 mbedtls_printf( "%s\n", vrfy_buf );
             }
             else
             {
                mbedtls_printf( " failed\n  !  mbedtls_x509_crt_verify returned %d\n\n", ret );
                goto exit;
            }
        }

        mbedtls_printf( " ok\n" );

        /*
         * 1.5. Load own private key
         */
        mbedtls_snprintf(name, 512, "ssl/test-ca/%s", client_private_keys[i]);

        mbedtls_printf( "  . Loading the client private key %s...", name );
        fflush( stdout );

        ret = mbedtls_pk_parse_keyfile( &pk, name, NULL );
        if( ret != 0 )
        {
            mbedtls_printf( " failed\n  !  mbedtls_pk_parse_keyfile returned %d\n\n", ret );
            goto exit;
        }

        mbedtls_printf( " ok\n" );

        /*
         * 1.6. Verify certificate validity with private key
         */
        mbedtls_printf( "  . Verify the client certificate with private key..." );
        fflush( stdout );


        /* EC NOT IMPLEMENTED YET */
        if( ! mbedtls_pk_can_do( &clicert.pk, MBEDTLS_PK_RSA ) )
        {
            mbedtls_printf( " failed\n  !  certificate's key is not RSA\n\n" );
            goto exit;
        }

        ret = mbedtls_mpi_cmp_mpi(&mbedtls_pk_rsa( pk )->N, &mbedtls_pk_rsa( clicert.pk )->N);
        if( ret != 0 )
        {
            mbedtls_printf( " failed\n  !  mbedtls_mpi_cmp_mpi for N returned %d\n\n", ret );
            goto exit;
        }

        ret = mbedtls_mpi_cmp_mpi(&mbedtls_pk_rsa( pk )->E, &mbedtls_pk_rsa( clicert.pk )->E);
        if( ret != 0 )
        {
            mbedtls_printf( " failed\n  !  mbedtls_mpi_cmp_mpi for E returned %d\n\n", ret );
            goto exit;
        }

        ret = mbedtls_rsa_check_privkey( mbedtls_pk_rsa( pk ) );
        if( ret != 0 )
        {
            mbedtls_printf( " failed\n  !  mbedtls_rsa_check_privkey returned %d\n\n", ret );
            goto exit;
        }

        mbedtls_printf( " ok\n" );

        mbedtls_x509_crt_free( &clicert );
        mbedtls_pk_free( &pk );
    }

    exit_code = MBEDTLS_EXIT_SUCCESS;

exit:
    mbedtls_x509_crt_free( &cacert );
    mbedtls_x509_crl_free( &crl );

#if defined(_WIN32)
    mbedtls_printf( "  + Press Enter to exit this program.\n" );
    fflush( stdout ); getchar();
#endif

    return( exit_code );
}
#endif /* MBEDTLS_RSA_C && MBEDTLS_X509_CRT_PARSE_C && MBEDTLS_FS_IO &&
          MBEDTLS_X509_CRL_PARSE_C */

/**
 * \file doc_ssltls.h
 *
 * \brief SSL/TLS communication module documentation file.
 */
/*
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

/**
 * @addtogroup ssltls_communication_module SSL/TLS communication module
 *
 * The SSL/TLS communication module provides the means to create an SSL/TLS
 * communication channel.
 *
 * The basic provisions are:
 * - initialise an SSL/TLS context (see \c mbedtls_ssl_init()).
 * - perform an SSL/TLS handshake (see \c mbedtls_ssl_handshake()).
 * - read/write (see \c mbedtls_ssl_read() and \c mbedtls_ssl_write()).
 * - notify a peer that connection is being closed (see \c mbedtls_ssl_close_notify()).
 *
 * Many aspects of such a channel are set through parameters and callback
 * functions:
 * - the endpoint role: client or server.
 * - the authentication mode. Should verification take place.
 * - the Host-to-host communication channel. A TCP/IP module is provided.
 * - the random number generator (RNG).
 * - the ciphers to use for encryption/decryption.
 * - session control functions.
 * - X.509 parameters for certificate-handling and key exchange.
 *
 * This module can be used to create an SSL/TLS server and client and to provide a basic
 * framework to setup and communicate through an SSL/TLS communication channel.\n
 * Note that you need to provide for several aspects yourself as mentioned above.
 */

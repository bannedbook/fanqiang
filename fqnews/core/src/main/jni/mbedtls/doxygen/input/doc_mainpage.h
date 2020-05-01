/**
 * \file doc_mainpage.h
 *
 * \brief Main page documentation file.
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
 * @mainpage mbed TLS v2.16.1 source code documentation
 *
 * This documentation describes the internal structure of mbed TLS.  It was
 * automatically generated from specially formatted comment blocks in
 * mbed TLS's source code using Doxygen.  (See
 * http://www.stack.nl/~dimitri/doxygen/ for more information on Doxygen)
 *
 * mbed TLS has a simple setup: it provides the ingredients for an SSL/TLS
 * implementation. These ingredients are listed as modules in the
 * \ref mainpage_modules "Modules section". This "Modules section" introduces
 * the high-level module concepts used throughout this documentation.\n
 * Some examples of mbed TLS usage can be found in the \ref mainpage_examples
 * "Examples section".
 *
 * @section mainpage_modules Modules
 *
 * mbed TLS supports SSLv3 up to TLSv1.2 communication by providing the
 * following:
 * - TCP/IP communication functions: listen, connect, accept, read/write.
 * - SSL/TLS communication functions: init, handshake, read/write.
 * - X.509 functions: CRT, CRL and key handling
 * - Random number generation
 * - Hashing
 * - Encryption/decryption
 *
 * Above functions are split up neatly into logical interfaces. These can be
 * used separately to provide any of the above functions or to mix-and-match
 * into an SSL server/client solution that utilises a X.509 PKI. Examples of
 * such implementations are amply provided with the source code.
 *
 * Note that mbed TLS does not provide a control channel or (multiple) session
 * handling without additional work from the developer.
 *
 * @section mainpage_examples Examples
 *
 * Example server setup:
 *
 * \b Prerequisites:
 * - X.509 certificate and private key
 * - session handling functions
 *
 * \b Setup:
 * - Load your certificate and your private RSA key (X.509 interface)
 * - Setup the listening TCP socket (TCP/IP interface)
 * - Accept incoming client connection (TCP/IP interface)
 * - Initialise as an SSL-server (SSL/TLS interface)
 *   - Set parameters, e.g. authentication, ciphers, CA-chain, key exchange
 *   - Set callback functions RNG, IO, session handling
 * - Perform an SSL-handshake (SSL/TLS interface)
 * - Read/write data (SSL/TLS interface)
 * - Close and cleanup (all interfaces)
 *
 * Example client setup:
 *
 * \b Prerequisites:
 * - X.509 certificate and private key
 * - X.509 trusted CA certificates
 *
 * \b Setup:
 * - Load the trusted CA certificates (X.509 interface)
 * - Load your certificate and your private RSA key (X.509 interface)
 * - Setup a TCP/IP connection (TCP/IP interface)
 * - Initialise as an SSL-client (SSL/TLS interface)
 *   - Set parameters, e.g. authentication mode, ciphers, CA-chain, session
 *   - Set callback functions RNG, IO
 * - Perform an SSL-handshake (SSL/TLS interface)
 * - Verify the server certificate (SSL/TLS interface)
 * - Write/read data (SSL/TLS interface)
 * - Close and cleanup (all interfaces)
 */

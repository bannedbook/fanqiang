/**
 * \file doc_rng.h
 *
 * \brief Random number generator (RNG) module documentation file.
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
 * @addtogroup rng_module Random number generator (RNG) module
 *
 * The Random number generator (RNG) module provides random number
 * generation, see \c mbedtls_ctr_drbg_random().
 *
 * The block-cipher counter-mode based deterministic random
 * bit generator (CTR_DBRG) as specified in NIST SP800-90. It needs an external
 * source of entropy. For these purposes \c mbedtls_entropy_func() can be used.
 * This is an implementation based on a simple entropy accumulator design.
 *
 * The other number generator that is included is less strong and uses the
 * HAVEGE (HArdware Volatile Entropy Gathering and Expansion) software heuristic
 * which considered unsafe for primary usage, but provides additional random
 * to the entropy pool if enables.
 *
 * Meaning that there seems to be no practical algorithm that can guess
 * the next bit with a probability larger than 1/2 in an output sequence.
 *
 * This module can be used to generate random numbers.
 */

/**
 *
 * @file tftp_opts.h
 *
 * @author   Logan Gunthorpe <logang@deltatee.com>
 *
 * @brief    Trivial File Transfer Protocol (RFC 1350) implementation options
 *
 * Copyright (c) Deltatee Enterprises Ltd. 2013
 * All rights reserved.
 *
 */

/* 
 * Redistribution and use in source and binary forms, with or without
 * modification,are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Logan Gunthorpe <logang@deltatee.com>
 *
 */

#ifndef LWIP_HDR_APPS_TFTP_OPTS_H
#define LWIP_HDR_APPS_TFTP_OPTS_H

#include "lwip/opt.h"
#include "lwip/prot/iana.h"

/**
 * @defgroup tftp_opts Options
 * @ingroup tftp
 * @{
 */

/**
 * Enable TFTP debug messages
 */
#if !defined TFTP_DEBUG || defined __DOXYGEN__
#define TFTP_DEBUG            LWIP_DBG_ON
#endif

/**
 * TFTP server port
 */
#if !defined TFTP_PORT || defined __DOXYGEN__
#define TFTP_PORT             LWIP_IANA_PORT_TFTP
#endif

/**
 * TFTP timeout
 */
#if !defined TFTP_TIMEOUT_MSECS || defined __DOXYGEN__
#define TFTP_TIMEOUT_MSECS    10000
#endif

/**
 * Max. number of retries when a file is read from server
 */
#if !defined TFTP_MAX_RETRIES || defined __DOXYGEN__
#define TFTP_MAX_RETRIES      5
#endif

/**
 * TFTP timer cyclic interval
 */
#if !defined TFTP_TIMER_MSECS || defined __DOXYGEN__
#define TFTP_TIMER_MSECS      50
#endif

/**
 * Max. length of TFTP filename
 */
#if !defined TFTP_MAX_FILENAME_LEN || defined __DOXYGEN__
#define TFTP_MAX_FILENAME_LEN 20
#endif

/**
 * Max. length of TFTP mode
 */
#if !defined TFTP_MAX_MODE_LEN || defined __DOXYGEN__
#define TFTP_MAX_MODE_LEN     7
#endif

/**
 * @}
 */

#endif /* LWIP_HDR_APPS_TFTP_OPTS_H */

/**
 * @file
 * lwIP netif implementing an IEEE 802.1D MAC Bridge
 */

/*
 * Copyright (c) 2017 Simon Goldschmidt.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
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
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Simon Goldschmidt <goldsimon@gmx.de>
 *
 */

#ifndef LWIP_HDR_NETIF_BRIDGEIF_OPTS_H
#define LWIP_HDR_NETIF_BRIDGEIF_OPTS_H

#include "lwip/opt.h"

/**
 * @defgroup bridgeif_opts Options
 * @ingroup bridgeif
 * @{
 */

/** BRIDGEIF_PORT_NETIFS_OUTPUT_DIRECT==1: set port netif's 'input' function
 * to call directly into bridgeif code and on top of that, directly call into
 * the selected forwarding port's 'linkoutput' function.
 * This means that the bridgeif input/output path is protected from concurrent access
 * but as well, *all* bridge port netif's drivers must correctly handle concurrent access!
 * == 0: get into tcpip_thread for every input packet (no multithreading)
 * ATTENTION: as ==0 relies on tcpip.h, the default depends on NO_SYS setting
 */
#ifndef BRIDGEIF_PORT_NETIFS_OUTPUT_DIRECT
#define BRIDGEIF_PORT_NETIFS_OUTPUT_DIRECT  NO_SYS
#endif

/** BRIDGEIF_EXTERNAL_FDB==1: use an external implementation for the forwarding
 * database which is possibly faster than our example implementation.
 * (Watch out for concurrent access!)
 */
#ifndef BRIDGEIF_EXTERNAL_FDB
#define BRIDGEIF_EXTERNAL_FDB               0
#endif

/** BRIDGEIF_MAX_PORTS: this is used to create a typedef used for forwarding
 * bit-fields: the number of bits required is this + 1 (for the internal/cpu port)
 * (63 is the maximum, resulting in an u64_t for the bit mask)
 * ATTENTION: this controls the maximum number of the implementation only!
 * The max. number of ports per bridge must still be passed via netif_add parameter!
 */
#ifndef BRIDGEIF_MAX_PORTS
#define BRIDGEIF_MAX_PORTS                  7
#endif

/** BRIDGEIF_DEBUG: Enable generic debugging in bridgeif.c. */
#ifndef BRIDGEIF_DEBUG
#define BRIDGEIF_DEBUG                      LWIP_DBG_OFF
#endif

/** BRIDGEIF_DEBUG: Enable FDB debugging in bridgeif.c. */
#ifndef BRIDGEIF_FDB_DEBUG
#define BRIDGEIF_FDB_DEBUG                  LWIP_DBG_OFF
#endif

/** BRIDGEIF_DEBUG: Enable forwarding debugging in bridgeif.c. */
#ifndef BRIDGEIF_FW_DEBUG
#define BRIDGEIF_FW_DEBUG                   LWIP_DBG_OFF
#endif

/**
 * @}
 */

#endif /* LWIP_HDR_NETIF_BRIDGEIF_OPTS_H */

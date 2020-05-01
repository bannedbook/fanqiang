/**
 * @file
 * MDNS responder
 */

 /*
 * Copyright (c) 2015 Verisure Innovation AB
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
 * Author: Erik Ekman <erik@kryo.se>
 *
 */

#ifndef LWIP_HDR_APPS_MDNS_OPTS_H
#define LWIP_HDR_APPS_MDNS_OPTS_H

#include "lwip/opt.h"

/**
 * @defgroup mdns_opts Options
 * @ingroup mdns
 * @{
 */

/**
 * LWIP_MDNS_RESPONDER==1: Turn on multicast DNS module. UDP must be available for MDNS
 * transport. IGMP is needed for IPv4 multicast.
 */
#ifndef LWIP_MDNS_RESPONDER
#define LWIP_MDNS_RESPONDER             0
#endif /* LWIP_MDNS_RESPONDER */

/** The maximum number of services per netif */
#ifndef MDNS_MAX_SERVICES
#define MDNS_MAX_SERVICES               1
#endif

/**
 * MDNS_DEBUG: Enable debugging for multicast DNS.
 */
#ifndef MDNS_DEBUG
#define MDNS_DEBUG                       LWIP_DBG_OFF
#endif

/**
 * @}
 */

#endif /* LWIP_HDR_APPS_MDNS_OPTS_H */


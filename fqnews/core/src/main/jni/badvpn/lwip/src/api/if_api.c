/**
 * @file
 * Interface Identification APIs from:
 *              RFC 3493: Basic Socket Interface Extensions for IPv6
 *                  Section 4: Interface Identification
 *
 * @defgroup if_api Interface Identification API
 * @ingroup socket
 */

/*
 * Copyright (c) 2017 Joel Cunningham, Garmin International, Inc. <joel.cunningham@garmin.com>
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
 * Author: Joel Cunningham <joel.cunningham@me.com>
 *
 */
#include "lwip/opt.h"

#if LWIP_SOCKET

#include "lwip/errno.h"
#include "lwip/if_api.h"
#include "lwip/netifapi.h"
#include "lwip/priv/sockets_priv.h"

/**
 * @ingroup if_api
 * Maps an interface index to its corresponding name.
 * @param ifindex interface index
 * @param ifname shall point to a buffer of at least {IF_NAMESIZE} bytes
 * @return If ifindex is an interface index, then the function shall return the
 * value supplied in ifname, which points to a buffer now containing the interface name.
 * Otherwise, the function shall return a NULL pointer.
 */
char *
lwip_if_indextoname(unsigned int ifindex, char *ifname)
{
#if LWIP_NETIF_API
  if (ifindex <= 0xff) {
    err_t err = netifapi_netif_index_to_name((u8_t)ifindex, ifname);
    if (!err && ifname[0] != '\0') {
      return ifname;
    }
  }
#else /* LWIP_NETIF_API */
  LWIP_UNUSED_ARG(ifindex);
  LWIP_UNUSED_ARG(ifname);
#endif /* LWIP_NETIF_API */
  set_errno(ENXIO);
  return NULL;
}

/**
 * @ingroup if_api
 * Returs the interface index corresponding to name ifname.
 * @param ifname Interface name
 * @return The corresponding index if ifname is the name of an interface;
 * otherwise, zero.
 */
unsigned int
lwip_if_nametoindex(const char *ifname)
{
#if LWIP_NETIF_API
  err_t err;
  u8_t idx;

  err = netifapi_netif_name_to_index(ifname, &idx);
  if (!err) {
    return idx;
  }
#else /* LWIP_NETIF_API */
  LWIP_UNUSED_ARG(ifname);
#endif /* LWIP_NETIF_API */
  return 0; /* invalid index */
}

#endif /* LWIP_SOCKET */

/**
 * @file
 * NETBIOS name service responder options
 */

/*
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
 */
#ifndef LWIP_HDR_APPS_NETBIOS_OPTS_H
#define LWIP_HDR_APPS_NETBIOS_OPTS_H

#include "lwip/opt.h"

/**
 * @defgroup netbiosns_opts Options
 * @ingroup netbiosns
 * @{
 */

/** NetBIOS name of lwip device
 * This must be uppercase until NETBIOS_STRCMP() is defined to a string
 * comparision function that is case insensitive.
 * If you want to use the netif's hostname, use this (with LWIP_NETIF_HOSTNAME):
 * (ip_current_netif() != NULL ? ip_current_netif()->hostname != NULL ? ip_current_netif()->hostname : "" : "")
 *
 * If this is not defined, netbiosns_set_name() can be called at runtime to change the name.
 */
#ifdef __DOXYGEN__
#define NETBIOS_LWIP_NAME "NETBIOSLWIPDEV"
#endif

/**
 * @}
 */

#endif /* LWIP_HDR_APPS_NETBIOS_OPTS_H */

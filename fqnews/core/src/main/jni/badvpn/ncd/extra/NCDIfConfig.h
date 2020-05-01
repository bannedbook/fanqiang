/**
 * @file NCDIfConfig.h
 * @author Ambroz Bizjak <ambrop7@gmail.com>
 * 
 * @section LICENSE
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BADVPN_NCD_NCDIFCONFIG_H
#define BADVPN_NCD_NCDIFCONFIG_H

#include <stddef.h>

#include <misc/ipaddr.h>
#include <misc/ipaddr6.h>

#define NCDIFCONFIG_FLAG_EXISTS (1 << 0)
#define NCDIFCONFIG_FLAG_UP (1 << 1)
#define NCDIFCONFIG_FLAG_RUNNING (1 << 2)

int NCDIfConfig_query (const char *ifname);

int NCDIfConfig_set_up (const char *ifname);
int NCDIfConfig_set_down (const char *ifname);

int NCDIfConfig_add_ipv4_addr (const char *ifname, struct ipv4_ifaddr ifaddr);
int NCDIfConfig_remove_ipv4_addr (const char *ifname, struct ipv4_ifaddr ifaddr);

int NCDIfConfig_add_ipv6_addr (const char *ifname, struct ipv6_ifaddr ifaddr);
int NCDIfConfig_remove_ipv6_addr (const char *ifname, struct ipv6_ifaddr ifaddr);

int NCDIfConfig_add_ipv4_route (struct ipv4_ifaddr dest, const uint32_t *gateway, int metric, const char *device);
int NCDIfConfig_remove_ipv4_route (struct ipv4_ifaddr dest, const uint32_t *gateway, int metric, const char *device);

int NCDIfConfig_add_ipv6_route (struct ipv6_ifaddr dest, const struct ipv6_addr *gateway, int metric, const char *device);
int NCDIfConfig_remove_ipv6_route (struct ipv6_ifaddr dest, const struct ipv6_addr *gateway, int metric, const char *device);

int NCDIfConfig_add_ipv4_blackhole_route (struct ipv4_ifaddr dest, int metric);
int NCDIfConfig_remove_ipv4_blackhole_route (struct ipv4_ifaddr dest, int metric);

int NCDIfConfig_add_ipv6_blackhole_route (struct ipv6_ifaddr dest, int metric);
int NCDIfConfig_remove_ipv6_blackhole_route (struct ipv6_ifaddr dest, int metric);

int NCDIfConfig_set_resolv_conf (const char *data, size_t data_len);

int NCDIfConfig_make_tuntap (const char *ifname, const char *owner, int tun);
int NCDIfConfig_remove_tuntap (const char *ifname, int tun);

#endif

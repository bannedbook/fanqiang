/*
 * ipcp.h - IP Control Protocol definitions.
 *
 * Copyright (c) 1984-2000 Carnegie Mellon University. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name "Carnegie Mellon University" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For permission or any legal
 *    details, please contact
 *      Office of Technology Transfer
 *      Carnegie Mellon University
 *      5000 Forbes Avenue
 *      Pittsburgh, PA  15213-3890
 *      (412) 268-4387, fax: (412) 268-7395
 *      tech-transfer@andrew.cmu.edu
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Computing Services
 *     at Carnegie Mellon University (http://www.cmu.edu/computing/)."
 *
 * CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id: ipcp.h,v 1.14 2002/12/04 23:03:32 paulus Exp $
 */

#include "netif/ppp/ppp_opts.h"
#if PPP_SUPPORT && PPP_IPV4_SUPPORT /* don't build if not configured for use in lwipopts.h */

#ifndef IPCP_H
#define	IPCP_H

/*
 * Options.
 */
#define CI_ADDRS	1	/* IP Addresses */
#if VJ_SUPPORT
#define CI_COMPRESSTYPE	2	/* Compression Type */
#endif /* VJ_SUPPORT */
#define	CI_ADDR		3

#if LWIP_DNS
#define CI_MS_DNS1      129	/* Primary DNS value */
#define CI_MS_DNS2      131     /* Secondary DNS value */
#endif /* LWIP_DNS */
#if 0 /* UNUSED - WINS */
#define CI_MS_WINS1     130     /* Primary WINS value */
#define CI_MS_WINS2     132	/* Secondary WINS value */
#endif /* UNUSED - WINS */

#if VJ_SUPPORT
#define MAX_STATES 16		/* from slcompress.h */

#define IPCP_VJMODE_OLD 1	/* "old" mode (option # = 0x0037) */
#define IPCP_VJMODE_RFC1172 2	/* "old-rfc"mode (option # = 0x002d) */
#define IPCP_VJMODE_RFC1332 3	/* "new-rfc"mode (option # = 0x002d, */
                                /*  maxslot and slot number compression) */

#define IPCP_VJ_COMP 0x002d	/* current value for VJ compression option*/
#define IPCP_VJ_COMP_OLD 0x0037	/* "old" (i.e, broken) value for VJ */
				/* compression option*/ 
#endif /* VJ_SUPPORT */

typedef struct ipcp_options {
    unsigned int neg_addr               :1; /* Negotiate IP Address? */
    unsigned int old_addrs              :1; /* Use old (IP-Addresses) option? */
    unsigned int req_addr               :1; /* Ask peer to send IP address? */
#if 0 /* UNUSED */
    unsigned int default_route          :1; /* Assign default route through interface? */
    unsigned int replace_default_route  :1; /* Replace default route through interface? */
#endif /* UNUSED */
#if 0 /* UNUSED - PROXY ARP */
    unsigned int proxy_arp              :1; /* Make proxy ARP entry for peer? */
#endif /* UNUSED - PROXY ARP */
#if VJ_SUPPORT
    unsigned int neg_vj                 :1; /* Van Jacobson Compression? */
    unsigned int old_vj                 :1; /* use old (short) form of VJ option? */
    unsigned int cflag                  :1;
#endif /* VJ_SUPPORT */
    unsigned int accept_local           :1; /* accept peer's value for ouraddr */
    unsigned int accept_remote          :1; /* accept peer's value for hisaddr */
#if LWIP_DNS
    unsigned int req_dns1               :1; /* Ask peer to send primary DNS address? */
    unsigned int req_dns2               :1; /* Ask peer to send secondary DNS address? */
#endif /* LWIP_DNS */

    u32_t ouraddr, hisaddr;	/* Addresses in NETWORK BYTE ORDER */
#if LWIP_DNS
    u32_t dnsaddr[2];	/* Primary and secondary MS DNS entries */
#endif /* LWIP_DNS */
#if 0 /* UNUSED - WINS */
    u32_t winsaddr[2];	/* Primary and secondary MS WINS entries */
#endif /* UNUSED - WINS */

#if VJ_SUPPORT
    u16_t vj_protocol;		/* protocol value to use in VJ option */
    u8_t  maxslotindex;		/* values for RFC1332 VJ compression neg. */
#endif /* VJ_SUPPORT */
} ipcp_options;

#if 0 /* UNUSED, already defined by lwIP */
char *ip_ntoa (u32_t);
#endif /* UNUSED, already defined by lwIP */

extern const struct protent ipcp_protent;

#endif /* IPCP_H */
#endif /* PPP_SUPPORT && PPP_IPV4_SUPPORT */

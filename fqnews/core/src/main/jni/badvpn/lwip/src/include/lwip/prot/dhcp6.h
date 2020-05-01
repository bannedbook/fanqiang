/**
 * @file
 * DHCPv6 protocol definitions
 */

/*
 * Copyright (c) 2017 Simon Goldschmidt <goldsimon@gmx.de>
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
#ifndef LWIP_HDR_PROT_DHCP6_H
#define LWIP_HDR_PROT_DHCP6_H

#include "lwip/opt.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DHCP6_CLIENT_PORT  546
#define DHCP6_SERVER_PORT  547


 /* DHCPv6 message item offsets and length */
#define DHCP6_TRANSACTION_ID_LEN   3
#define DHCP_SNAME_OFS    44U
#define DHCP_SNAME_LEN    64U
#define DHCP_FILE_OFS     108U
#define DHCP_FILE_LEN     128U
#define DHCP_MSG_LEN      236U
#define DHCP_OPTIONS_OFS  (DHCP_MSG_LEN + 4U)*/ /* 4 byte: cookie */

#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
/** minimum set of fields of any DHCPv6 message */
struct dhcp6_msg
{
  PACK_STRUCT_FLD_8(u8_t msgtype);
  PACK_STRUCT_FLD_8(u8_t transaction_id[DHCP6_TRANSACTION_ID_LEN]);
  /* options follow */
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif


/* DHCP6 client states */
typedef enum {
  DHCP6_STATE_OFF               = 0,
  DHCP6_STATE_REQUESTING_CONFIG = 1
} dhcp_state_enum_t;

/* DHCPv6 message types */
#define DHCP6_SOLICIT               1
#define DHCP6_ADVERTISE             2
#define DHCP6_REQUEST               3
#define DHCP6_CONFIRM               4
#define DHCP6_RENEW                 5
#define DHCP6_REBIND                6
#define DHCP6_REPLY                 7
#define DHCP6_RELEASE               8
#define DHCP6_DECLINE               9
#define DHCP6_RECONFIGURE           10
#define DHCP6_INFOREQUEST           11
#define DHCP6_RELAYFORW             12
#define DHCP6_RELAYREPL             13

/** DHCPv6 status codes */
#define DHCP6_STATUS_SUCCESS        0 /* Success. */
#define DHCP6_STATUS_UNSPECFAIL     1 /* Failure, reason unspecified; this status code is sent by either a client or a server to indicate a failure not explicitly specified in this document. */
#define DHCP6_STATUS_NOADDRSAVAIL   2 /* Server has no addresses available to assign to the IA(s). */
#define DHCP6_STATUS_NOBINDING      3 /* Client record (binding) unavailable. */
#define DHCP6_STATUS_NOTONLINK      4 /* The prefix for the address is not appropriate for the link to which the client is attached. */
#define DHCP6_STATUS_USEMULTICAST   5 /* Sent by a server to a client to force the client to send messages to the server using the All_DHCP_Relay_Agents_and_Servers address. */

/** DHCPv6 DUID types */
#define DHCP6_DUID_LLT              1 /* LLT: Link-layer Address Plus Time */
#define DHCP6_DUID_EN               2 /* EN: Enterprise number */
#define DHCP6_DUID_LL               3 /* LL: Link-layer Address */

/* DHCPv6 options */
#define DHCP6_OPTION_CLIENTID       1
#define DHCP6_OPTION_SERVERID       2
#define DHCP6_OPTION_IA_NA          3
#define DHCP6_OPTION_IA_TA          4
#define DHCP6_OPTION_IAADDR         5
#define DHCP6_OPTION_ORO            6
#define DHCP6_OPTION_PREFERENCE     7
#define DHCP6_OPTION_ELAPSED_TIME   8
#define DHCP6_OPTION_RELAY_MSG      9
#define DHCP6_OPTION_AUTH           11
#define DHCP6_OPTION_UNICAST        12
#define DHCP6_OPTION_STATUS_CODE    13
#define DHCP6_OPTION_RAPID_COMMIT   14
#define DHCP6_OPTION_USER_CLASS     15
#define DHCP6_OPTION_VENDOR_CLASS   16
#define DHCP6_OPTION_VENDOR_OPTS    17
#define DHCP6_OPTION_INTERFACE_ID   18
#define DHCP6_OPTION_RECONF_MSG     19
#define DHCP6_OPTION_ACCEPT         20


#ifdef __cplusplus
}
#endif

#endif /*LWIP_HDR_PROT_DHCP_H*/

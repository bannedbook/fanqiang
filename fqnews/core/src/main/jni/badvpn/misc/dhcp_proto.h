/**
 * @file dhcp_proto.h
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
 * 
 * @section DESCRIPTION
 * 
 * Definitions for the DHCP protocol.
 */

#ifndef BADVPN_MISC_DHCP_PROTO_H
#define BADVPN_MISC_DHCP_PROTO_H

#include <stdint.h>

#include <misc/packed.h>

#define DHCP_OP_BOOTREQUEST 1
#define DHCP_OP_BOOTREPLY 2

#define DHCP_HARDWARE_ADDRESS_TYPE_ETHERNET 1

#define DHCP_MAGIC 0x63825363

#define DHCP_OPTION_PAD 0
#define DHCP_OPTION_END 255

#define DHCP_OPTION_SUBNET_MASK 1
#define DHCP_OPTION_ROUTER 3
#define DHCP_OPTION_DOMAIN_NAME_SERVER 6
#define DHCP_OPTION_HOST_NAME 12
#define DHCP_OPTION_REQUESTED_IP_ADDRESS 50
#define DHCP_OPTION_IP_ADDRESS_LEASE_TIME 51
#define DHCP_OPTION_DHCP_MESSAGE_TYPE 53
#define DHCP_OPTION_DHCP_SERVER_IDENTIFIER 54
#define DHCP_OPTION_PARAMETER_REQUEST_LIST 55
#define DHCP_OPTION_MAXIMUM_MESSAGE_SIZE 57
#define DHCP_OPTION_RENEWAL_TIME_VALUE 58
#define DHCP_OPTION_REBINDING_TIME_VALUE 59
#define DHCP_OPTION_VENDOR_CLASS_IDENTIFIER 60
#define DHCP_OPTION_CLIENT_IDENTIFIER 61

#define DHCP_MESSAGE_TYPE_DISCOVER 1
#define DHCP_MESSAGE_TYPE_OFFER 2
#define DHCP_MESSAGE_TYPE_REQUEST 3
#define DHCP_MESSAGE_TYPE_DECLINE 4
#define DHCP_MESSAGE_TYPE_ACK 5
#define DHCP_MESSAGE_TYPE_NAK 6
#define DHCP_MESSAGE_TYPE_RELEASE 7

B_START_PACKED
struct dhcp_header {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    uint8_t chaddr[16];
    uint8_t sname[64];
    uint8_t file[128];
    uint32_t magic;
} B_PACKED;
B_END_PACKED

B_START_PACKED
struct dhcp_option_header {
    uint8_t type;
    uint8_t len;
} B_PACKED;
B_END_PACKED

B_START_PACKED
struct dhcp_option_dhcp_message_type {
    uint8_t type;
} B_PACKED;
B_END_PACKED

B_START_PACKED
struct dhcp_option_maximum_message_size {
    uint16_t size;
} B_PACKED;
B_END_PACKED

B_START_PACKED
struct dhcp_option_dhcp_server_identifier {
    uint32_t id;
} B_PACKED;
B_END_PACKED

B_START_PACKED
struct dhcp_option_time {
    uint32_t time;
} B_PACKED;
B_END_PACKED

B_START_PACKED
struct dhcp_option_addr {
    uint32_t addr;
} B_PACKED;
B_END_PACKED

#endif

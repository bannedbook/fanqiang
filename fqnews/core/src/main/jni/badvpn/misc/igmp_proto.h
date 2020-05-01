/**
 * @file igmp_proto.h
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
 * Definitions for the IGMP protocol.
 */

#ifndef BADVPN_MISC_IGMP_PROTO_H
#define BADVPN_MISC_IGMP_PROTO_H

#include <stdint.h>

#include <misc/packed.h>

#define IGMP_TYPE_MEMBERSHIP_QUERY 0x11
#define IGMP_TYPE_V1_MEMBERSHIP_REPORT 0x12
#define IGMP_TYPE_V2_MEMBERSHIP_REPORT 0x16
#define IGMP_TYPE_V3_MEMBERSHIP_REPORT 0x22
#define IGMP_TYPE_V2_LEAVE_GROUP 0x17

#define IGMP_RECORD_TYPE_MODE_IS_INCLUDE 1
#define IGMP_RECORD_TYPE_MODE_IS_EXCLUDE 2
#define IGMP_RECORD_TYPE_CHANGE_TO_INCLUDE_MODE 3
#define IGMP_RECORD_TYPE_CHANGE_TO_EXCLUDE_MODE 4

B_START_PACKED
struct igmp_source {
    uint32_t addr;
} B_PACKED;
B_END_PACKED

B_START_PACKED
struct igmp_base {
    uint8_t type;
    uint8_t max_resp_code;
    uint16_t checksum;
} B_PACKED;
B_END_PACKED

B_START_PACKED
struct igmp_v3_query_extra {
    uint32_t group;
    uint8_t reserved4_suppress1_qrv3;
    uint8_t qqic;
    uint16_t number_of_sources;
} B_PACKED;
B_END_PACKED

B_START_PACKED
struct igmp_v3_report_extra {
    uint16_t reserved;
    uint16_t number_of_group_records;
} B_PACKED;
B_END_PACKED

B_START_PACKED
struct igmp_v3_report_record {
    uint8_t type;
    uint8_t aux_data_len;
    uint16_t number_of_sources;
    uint32_t group;
} B_PACKED;
B_END_PACKED

B_START_PACKED
struct igmp_v2_extra {
    uint32_t group;
} B_PACKED;
B_END_PACKED

#endif

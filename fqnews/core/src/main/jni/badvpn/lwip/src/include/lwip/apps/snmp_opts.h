/**
 * @file
 * SNMP server options list
 */

/*
 * Copyright (c) 2015 Dirk Ziegelmeier
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
 * Author: Dirk Ziegelmeier
 *
 */
#ifndef LWIP_HDR_SNMP_OPTS_H
#define LWIP_HDR_SNMP_OPTS_H

#include "lwip/opt.h"

/**
 * @defgroup snmp_opts Options
 * @ingroup snmp
 * @{
 */

/**
 * LWIP_SNMP==1: This enables the lwIP SNMP agent. UDP must be available
 * for SNMP transport.
 * If you want to use your own SNMP agent, leave this disabled.
 * To integrate MIB2 of an external agent, you need to enable
 * LWIP_MIB2_CALLBACKS and MIB2_STATS. This will give you the callbacks
 * and statistics counters you need to get MIB2 working.
 */
#if !defined LWIP_SNMP || defined __DOXYGEN__
#define LWIP_SNMP                       0
#endif

/**
 * SNMP_USE_NETCONN: Use netconn API instead of raw API.
 * Makes SNMP agent run in a worker thread, so blocking operations
 * can be done in MIB calls.
 */
#if !defined SNMP_USE_NETCONN || defined __DOXYGEN__
#define SNMP_USE_NETCONN           0
#endif

/**
 * SNMP_USE_RAW: Use raw API.
 * SNMP agent does not run in a worker thread, so blocking operations
 * should not be done in MIB calls.
 */
#if !defined SNMP_USE_RAW || defined __DOXYGEN__
#define SNMP_USE_RAW               1
#endif

#if SNMP_USE_NETCONN && SNMP_USE_RAW
#error SNMP stack can use only one of the APIs {raw, netconn}
#endif

#if LWIP_SNMP && !SNMP_USE_NETCONN && !SNMP_USE_RAW
#error SNMP stack needs a receive API and UDP {raw, netconn}
#endif

#if SNMP_USE_NETCONN
/**
 * SNMP_STACK_SIZE: Stack size of SNMP netconn worker thread
 */
#if !defined SNMP_STACK_SIZE || defined __DOXYGEN__
#define SNMP_STACK_SIZE            DEFAULT_THREAD_STACKSIZE
#endif

/**
 * SNMP_THREAD_PRIO: SNMP netconn worker thread priority
 */
#if !defined SNMP_THREAD_PRIO || defined __DOXYGEN__
#define SNMP_THREAD_PRIO           DEFAULT_THREAD_PRIO
#endif
#endif /* SNMP_USE_NETCONN */

/**
 * SNMP_TRAP_DESTINATIONS: Number of trap destinations. At least one trap
 * destination is required
 */
#if !defined SNMP_TRAP_DESTINATIONS || defined __DOXYGEN__
#define SNMP_TRAP_DESTINATIONS          1
#endif

/**
 * Only allow SNMP write actions that are 'safe' (e.g. disabling netifs is not
 * a safe action and disabled when SNMP_SAFE_REQUESTS = 1).
 * Unsafe requests are disabled by default!
 */
#if !defined SNMP_SAFE_REQUESTS || defined __DOXYGEN__
#define SNMP_SAFE_REQUESTS              1
#endif

/**
 * The maximum length of strings used.
 */
#if !defined SNMP_MAX_OCTET_STRING_LEN || defined __DOXYGEN__
#define SNMP_MAX_OCTET_STRING_LEN       127
#endif

/**
 * The maximum number of Sub ID's inside an object identifier.
 * Indirectly this also limits the maximum depth of SNMP tree.
 */
#if !defined SNMP_MAX_OBJ_ID_LEN || defined __DOXYGEN__
#define SNMP_MAX_OBJ_ID_LEN             50
#endif

#if !defined SNMP_MAX_VALUE_SIZE || defined __DOXYGEN__
/**
 * The maximum size of a value.
 */
#define SNMP_MIN_VALUE_SIZE             (2 * sizeof(u32_t*)) /* size required to store the basic types (8 bytes for counter64) */
/**
 * The minimum size of a value.
 */
#define SNMP_MAX_VALUE_SIZE             LWIP_MAX(LWIP_MAX((SNMP_MAX_OCTET_STRING_LEN), sizeof(u32_t)*(SNMP_MAX_OBJ_ID_LEN)), SNMP_MIN_VALUE_SIZE)
#endif

/**
 * The snmp read-access community. Used for write-access and traps, too
 * unless SNMP_COMMUNITY_WRITE or SNMP_COMMUNITY_TRAP are enabled, respectively.
 */
#if !defined SNMP_COMMUNITY || defined __DOXYGEN__
#define SNMP_COMMUNITY                  "public"
#endif

/**
 * The snmp write-access community.
 * Set this community to "" in order to disallow any write access.
 */
#if !defined SNMP_COMMUNITY_WRITE || defined __DOXYGEN__
#define SNMP_COMMUNITY_WRITE            "private"
#endif

/**
 * The snmp community used for sending traps.
 */
#if !defined SNMP_COMMUNITY_TRAP || defined __DOXYGEN__
#define SNMP_COMMUNITY_TRAP             "public"
#endif

/**
 * The maximum length of community string.
 * If community names shall be adjusted at runtime via snmp_set_community() calls,
 * enter here the possible maximum length (+1 for terminating null character).
 */
#if !defined SNMP_MAX_COMMUNITY_STR_LEN || defined __DOXYGEN__
#define SNMP_MAX_COMMUNITY_STR_LEN LWIP_MAX(LWIP_MAX(sizeof(SNMP_COMMUNITY), sizeof(SNMP_COMMUNITY_WRITE)), sizeof(SNMP_COMMUNITY_TRAP))
#endif

/**
 * The OID identifiying the device. This may be the enterprise OID itself or any OID located below it in tree.
 */
#if !defined SNMP_DEVICE_ENTERPRISE_OID || defined __DOXYGEN__
#define SNMP_LWIP_ENTERPRISE_OID 26381
/**
 * IANA assigned enterprise ID for lwIP is 26381
 * @see http://www.iana.org/assignments/enterprise-numbers
 *
 * @note this enterprise ID is assigned to the lwIP project,
 * all object identifiers living under this ID are assigned
 * by the lwIP maintainers!
 * @note don't change this define, use snmp_set_device_enterprise_oid()
 *
 * If you need to create your own private MIB you'll need
 * to apply for your own enterprise ID with IANA:
 * http://www.iana.org/numbers.html
 */
#define SNMP_DEVICE_ENTERPRISE_OID {1, 3, 6, 1, 4, 1, SNMP_LWIP_ENTERPRISE_OID}
/**
 * Length of SNMP_DEVICE_ENTERPRISE_OID
 */
#define SNMP_DEVICE_ENTERPRISE_OID_LEN 7
#endif

/**
 * SNMP_DEBUG: Enable debugging for SNMP messages.
 */
#if !defined SNMP_DEBUG || defined __DOXYGEN__
#define SNMP_DEBUG                      LWIP_DBG_OFF
#endif

/**
 * SNMP_MIB_DEBUG: Enable debugging for SNMP MIBs.
 */
#if !defined SNMP_MIB_DEBUG || defined __DOXYGEN__
#define SNMP_MIB_DEBUG                  LWIP_DBG_OFF
#endif

/**
 * Indicates if the MIB2 implementation of LWIP SNMP stack is used.
 */
#if !defined SNMP_LWIP_MIB2 || defined __DOXYGEN__
#define SNMP_LWIP_MIB2                      LWIP_SNMP
#endif

/**
 * Value return for sysDesc field of MIB2.
 */
#if !defined SNMP_LWIP_MIB2_SYSDESC || defined __DOXYGEN__
#define SNMP_LWIP_MIB2_SYSDESC              "lwIP"
#endif

/**
 * Value return for sysName field of MIB2.
 * To make sysName field settable, call snmp_mib2_set_sysname() to provide the necessary buffers.
 */
#if !defined SNMP_LWIP_MIB2_SYSNAME || defined __DOXYGEN__
#define SNMP_LWIP_MIB2_SYSNAME              "FQDN-unk"
#endif

/**
 * Value return for sysContact field of MIB2.
 * To make sysContact field settable, call snmp_mib2_set_syscontact() to provide the necessary buffers.
 */
#if !defined SNMP_LWIP_MIB2_SYSCONTACT || defined __DOXYGEN__
#define SNMP_LWIP_MIB2_SYSCONTACT           ""
#endif

/**
 * Value return for sysLocation field of MIB2.
 * To make sysLocation field settable, call snmp_mib2_set_syslocation() to provide the necessary buffers.
 */
#if !defined SNMP_LWIP_MIB2_SYSLOCATION || defined __DOXYGEN__
#define SNMP_LWIP_MIB2_SYSLOCATION          ""
#endif

/**
 * This value is used to limit the repetitions processed in GetBulk requests (value == 0 means no limitation).
 * This may be useful to limit the load for a single request.
 * According to SNMP RFC 1905 it is allowed to not return all requested variables from a GetBulk request if system load would be too high.
 * so the effect is that the client will do more requests to gather all data.
 * For the stack this could be useful in case that SNMP processing is done in TCP/IP thread. In this situation a request with many
 * repetitions could block the thread for a longer time. Setting limit here will keep the stack more responsive.
 */
#if !defined SNMP_LWIP_GETBULK_MAX_REPETITIONS || defined __DOXYGEN__
#define SNMP_LWIP_GETBULK_MAX_REPETITIONS 0
#endif

/**
 * @}
 */

/*
   ------------------------------------
   ---------- SNMPv3 options ----------
   ------------------------------------
*/

/**
 * LWIP_SNMP_V3==1: This enables EXPERIMENTAL SNMPv3 support. LWIP_SNMP must
 * also be enabled.
 * THIS IS UNDER DEVELOPMENT AND SHOULD NOT BE ENABLED IN PRODUCTS.
 */
#ifndef LWIP_SNMP_V3
#define LWIP_SNMP_V3               0
#endif

#ifndef LWIP_SNMP_V3_MBEDTLS
#define LWIP_SNMP_V3_MBEDTLS       LWIP_SNMP_V3
#endif

#ifndef LWIP_SNMP_V3_CRYPTO
#define LWIP_SNMP_V3_CRYPTO        LWIP_SNMP_V3_MBEDTLS
#endif

#ifndef LWIP_SNMP_CONFIGURE_VERSIONS
#define LWIP_SNMP_CONFIGURE_VERSIONS 0
#endif

#endif /* LWIP_HDR_SNMP_OPTS_H */

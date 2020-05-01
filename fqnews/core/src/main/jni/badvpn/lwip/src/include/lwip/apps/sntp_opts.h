/**
 * @file
 * SNTP client options list
 */

/*
 * Copyright (c) 2007-2009 Frédéric Bernon, Simon Goldschmidt
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
 * Author: Frédéric Bernon, Simon Goldschmidt
 *
 */
#ifndef LWIP_HDR_APPS_SNTP_OPTS_H
#define LWIP_HDR_APPS_SNTP_OPTS_H

#include "lwip/opt.h"
#include "lwip/prot/iana.h"

/**
 * @defgroup sntp_opts Options
 * @ingroup sntp
 * @{
 */

/** SNTP macro to change system time in seconds
 * Define SNTP_SET_SYSTEM_TIME_US(sec, us) to set the time in microseconds
 * instead of this one if you need the additional precision. Alternatively,
 * define SNTP_SET_SYSTEM_TIME_NTP(sec, frac) in order to work with native
 * NTP timestamps instead.
 */
#if !defined SNTP_SET_SYSTEM_TIME || defined __DOXYGEN__
#define SNTP_SET_SYSTEM_TIME(sec)   LWIP_UNUSED_ARG(sec)
#endif

/** The maximum number of SNTP servers that can be set */
#if !defined SNTP_MAX_SERVERS || defined __DOXYGEN__
#define SNTP_MAX_SERVERS           LWIP_DHCP_MAX_NTP_SERVERS
#endif

/** Set this to 1 to implement the callback function called by dhcp when
 * NTP servers are received. */
#if !defined SNTP_GET_SERVERS_FROM_DHCP || defined __DOXYGEN__
#define SNTP_GET_SERVERS_FROM_DHCP LWIP_DHCP_GET_NTP_SRV
#endif

/** Set this to 1 to support DNS names (or IP address strings) to set sntp servers
 * One server address/name can be defined as default if SNTP_SERVER_DNS == 1:
 * \#define SNTP_SERVER_ADDRESS "pool.ntp.org"
 */
#if !defined SNTP_SERVER_DNS || defined __DOXYGEN__
#define SNTP_SERVER_DNS            0
#endif

/**
 * SNTP_DEBUG: Enable debugging for SNTP.
 */
#if !defined SNTP_DEBUG || defined __DOXYGEN__
#define SNTP_DEBUG                  LWIP_DBG_OFF
#endif

/** SNTP server port */
#if !defined SNTP_PORT || defined __DOXYGEN__
#define SNTP_PORT                   LWIP_IANA_PORT_SNTP
#endif

/** Sanity check:
 * Define this to
 * - 0 to turn off sanity checks (default; smaller code)
 * - >= 1 to check address and port of the response packet to ensure the
 *        response comes from the server we sent the request to.
 * - >= 2 to check returned Originate Timestamp against Transmit Timestamp
 *        sent to the server (to ensure response to older request).
 * - >= 3 @todo: discard reply if any of the VN, Stratum, or Transmit Timestamp
 *        fields is 0 or the Mode field is not 4 (unicast) or 5 (broadcast).
 * - >= 4 @todo: to check that the Root Delay and Root Dispersion fields are each
 *        greater than or equal to 0 and less than infinity, where infinity is
 *        currently a cozy number like one second. This check avoids using a
 *        server whose synchronization source has expired for a very long time.
 */
#if !defined SNTP_CHECK_RESPONSE || defined __DOXYGEN__
#define SNTP_CHECK_RESPONSE         0
#endif

/** Enable round-trip delay compensation.
 * Compensate for the round-trip delay by calculating the clock offset from
 * the originate, receive, transmit and destination timestamps, as per RFC.
 *
 * The calculation requires compiler support for 64-bit integers. Also, either
 * SNTP_SET_SYSTEM_TIME_US or SNTP_SET_SYSTEM_TIME_NTP has to be implemented
 * for setting the system clock with sub-second precision. Likewise, either
 * SNTP_GET_SYSTEM_TIME or SNTP_GET_SYSTEM_TIME_NTP needs to be implemented
 * with sub-second precision.
 *
 * Although not strictly required, it makes sense to combine this option with
 * SNTP_CHECK_RESPONSE >= 2 for sanity-checking of the received timestamps.
 * Also, in order for the round-trip calculation to work, the difference
 * between the local clock and the NTP server clock must not be larger than
 * about 34 years. If that limit is exceeded, the implementation will fall back
 * to setting the clock without compensation. In order to ensure that the local
 * clock is always within the permitted range for compensation, even at first
 * try, it may be necessary to store at least the current year in non-volatile
 * memory.
 */
#if !defined SNTP_COMP_ROUNDTRIP || defined __DOXYGEN__
#define SNTP_COMP_ROUNDTRIP         0
#endif

/** According to the RFC, this shall be a random delay
 * between 1 and 5 minutes (in milliseconds) to prevent load peaks.
 * This can be defined to a random generation function,
 * which must return the delay in milliseconds as u32_t.
 * Turned off by default.
 */
#if !defined SNTP_STARTUP_DELAY || defined __DOXYGEN__
#ifdef LWIP_RAND
#define SNTP_STARTUP_DELAY          1
#else
#define SNTP_STARTUP_DELAY          0
#endif
#endif

/** If you want the startup delay to be a function, define this
 * to a function (including the brackets) and define SNTP_STARTUP_DELAY to 1.
 */
#if !defined SNTP_STARTUP_DELAY_FUNC || defined __DOXYGEN__
#define SNTP_STARTUP_DELAY_FUNC     (LWIP_RAND() % 5000)
#endif

/** SNTP receive timeout - in milliseconds
 * Also used as retry timeout - this shouldn't be too low.
 * Default is 15 seconds. Must not be beolw 15 seconds by specification (i.e. 15000)
 */
#if !defined SNTP_RECV_TIMEOUT || defined __DOXYGEN__
#define SNTP_RECV_TIMEOUT           15000
#endif

/** SNTP update delay - in milliseconds
 * Default is 1 hour. Must not be beolw 60 seconds by specification (i.e. 60000)
 */
#if !defined SNTP_UPDATE_DELAY || defined __DOXYGEN__
#define SNTP_UPDATE_DELAY           3600000
#endif

/** SNTP macro to get system time, used with SNTP_CHECK_RESPONSE >= 2
 * to send in request and compare in response. Also used for round-trip
 * delay compensation if SNTP_COMP_ROUNDTRIP != 0.
 * Alternatively, define SNTP_GET_SYSTEM_TIME_NTP(sec, frac) in order to
 * work with native NTP timestamps instead.
 */
#if !defined SNTP_GET_SYSTEM_TIME || defined __DOXYGEN__
#define SNTP_GET_SYSTEM_TIME(sec, us)     do { (sec) = 0; (us) = 0; } while(0)
#endif

/** Default retry timeout (in milliseconds) if the response
 * received is invalid.
 * This is doubled with each retry until SNTP_RETRY_TIMEOUT_MAX is reached.
 */
#if !defined SNTP_RETRY_TIMEOUT || defined __DOXYGEN__
#define SNTP_RETRY_TIMEOUT          SNTP_RECV_TIMEOUT
#endif

/** Maximum retry timeout (in milliseconds). */
#if !defined SNTP_RETRY_TIMEOUT_MAX || defined __DOXYGEN__
#define SNTP_RETRY_TIMEOUT_MAX      (SNTP_RETRY_TIMEOUT * 10)
#endif

/** Increase retry timeout with every retry sent
 * Default is on to conform to RFC.
 */
#if !defined SNTP_RETRY_TIMEOUT_EXP || defined __DOXYGEN__
#define SNTP_RETRY_TIMEOUT_EXP      1
#endif

/**
 * @}
 */

#endif /* LWIP_HDR_APPS_SNTP_OPTS_H */

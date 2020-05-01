/**
 * @file
 * SNTP client module
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
 */


/**
 * @defgroup sntp SNTP
 * @ingroup apps
 *
 * This is simple "SNTP" client for the lwIP raw API.
 * It is a minimal implementation of SNTPv4 as specified in RFC 4330.
 *
 * For a list of some public NTP servers, see this link:
 * http://support.ntp.org/bin/view/Servers/NTPPoolServers
 *
 * @todo:
 * - complete SNTP_CHECK_RESPONSE checks 3 and 4
 */

#include "lwip/apps/sntp.h"

#include "lwip/opt.h"
#include "lwip/timeouts.h"
#include "lwip/udp.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include "lwip/dhcp.h"

#include <string.h>
#include <time.h>

#if LWIP_UDP

/* Handle support for more than one server via SNTP_MAX_SERVERS */
#if SNTP_MAX_SERVERS > 1
#define SNTP_SUPPORT_MULTIPLE_SERVERS 1
#else /* NTP_MAX_SERVERS > 1 */
#define SNTP_SUPPORT_MULTIPLE_SERVERS 0
#endif /* NTP_MAX_SERVERS > 1 */

#ifndef SNTP_SUPPRESS_DELAY_CHECK
#if SNTP_UPDATE_DELAY < 15000
#error "SNTPv4 RFC 4330 enforces a minimum update time of 15 seconds (define SNTP_SUPPRESS_DELAY_CHECK to disable this error)!"
#endif
#endif

/* the various debug levels for this file */
#define SNTP_DEBUG_TRACE        (SNTP_DEBUG | LWIP_DBG_TRACE)
#define SNTP_DEBUG_STATE        (SNTP_DEBUG | LWIP_DBG_STATE)
#define SNTP_DEBUG_WARN         (SNTP_DEBUG | LWIP_DBG_LEVEL_WARNING)
#define SNTP_DEBUG_WARN_STATE   (SNTP_DEBUG | LWIP_DBG_LEVEL_WARNING | LWIP_DBG_STATE)
#define SNTP_DEBUG_SERIOUS      (SNTP_DEBUG | LWIP_DBG_LEVEL_SERIOUS)

#define SNTP_ERR_KOD                1

/* SNTP protocol defines */
#define SNTP_MSG_LEN                48

#define SNTP_OFFSET_LI_VN_MODE      0
#define SNTP_LI_MASK                0xC0
#define SNTP_LI_NO_WARNING          (0x00 << 6)
#define SNTP_LI_LAST_MINUTE_61_SEC  (0x01 << 6)
#define SNTP_LI_LAST_MINUTE_59_SEC  (0x02 << 6)
#define SNTP_LI_ALARM_CONDITION     (0x03 << 6) /* (clock not synchronized) */

#define SNTP_VERSION_MASK           0x38
#define SNTP_VERSION                (4/* NTP Version 4*/<<3)

#define SNTP_MODE_MASK              0x07
#define SNTP_MODE_CLIENT            0x03
#define SNTP_MODE_SERVER            0x04
#define SNTP_MODE_BROADCAST         0x05

#define SNTP_OFFSET_STRATUM         1
#define SNTP_STRATUM_KOD            0x00

#define SNTP_OFFSET_ORIGINATE_TIME  24
#define SNTP_OFFSET_RECEIVE_TIME    32
#define SNTP_OFFSET_TRANSMIT_TIME   40

/* Number of seconds between 1970 and Feb 7, 2036 06:28:16 UTC (epoch 1) */
#define DIFF_SEC_1970_2036          ((s32_t)2085978496L)

/** Convert NTP timestamp fraction to microseconds.
 */
#ifndef SNTP_FRAC_TO_US
# if LWIP_HAVE_INT64
#  define SNTP_FRAC_TO_US(f)        ((u32_t)(((u64_t)(f) * 1000000UL) >> 32))
# else
#  define SNTP_FRAC_TO_US(f)        ((u32_t)(f) / 4295)
# endif
#endif /* !SNTP_FRAC_TO_US */

/* Configure behaviour depending on native, microsecond or second precision.
 * Treat NTP timestamps as signed two's-complement integers. This way,
 * timestamps that have the MSB set simply become negative offsets from
 * the epoch (Feb 7, 2036 06:28:16 UTC). Representable dates range from
 * 1968 to 2104.
 */
#ifndef SNTP_SET_SYSTEM_TIME_NTP
# ifdef SNTP_SET_SYSTEM_TIME_US
#  define SNTP_SET_SYSTEM_TIME_NTP(s, f) \
    SNTP_SET_SYSTEM_TIME_US((u32_t)((s) + DIFF_SEC_1970_2036), SNTP_FRAC_TO_US(f))
# else
#  define SNTP_SET_SYSTEM_TIME_NTP(s, f) \
    SNTP_SET_SYSTEM_TIME((u32_t)((s) + DIFF_SEC_1970_2036))
# endif
#endif /* !SNTP_SET_SYSTEM_TIME_NTP */

/* Get the system time either natively as NTP timestamp or convert from
 * Unix time in seconds and microseconds. Take care to avoid overflow if the
 * microsecond value is at the maximum of 999999. Also add 0.5 us fudge to
 * avoid special values like 0, and to mask round-off errors that would
 * otherwise break round-trip conversion identity.
 */
#ifndef SNTP_GET_SYSTEM_TIME_NTP
# define SNTP_GET_SYSTEM_TIME_NTP(s, f) do { \
    u32_t sec_, usec_; \
    SNTP_GET_SYSTEM_TIME(sec_, usec_); \
    (s) = (s32_t)(sec_ - DIFF_SEC_1970_2036); \
    (f) = usec_ * 4295 - ((usec_ * 2143) >> 16) + 2147; \
  } while (0)
#endif /* !SNTP_GET_SYSTEM_TIME_NTP */

/* Start offset of the timestamps to extract from the SNTP packet */
#define SNTP_OFFSET_TIMESTAMPS \
    (SNTP_OFFSET_TRANSMIT_TIME + 8 - sizeof(struct sntp_timestamps))

/* Round-trip delay arithmetic helpers */
#if SNTP_COMP_ROUNDTRIP
# if !LWIP_HAVE_INT64
#  error "SNTP round-trip delay compensation requires 64-bit arithmetic"
# endif
# define SNTP_SEC_FRAC_TO_S64(s, f) \
    ((s64_t)(((u64_t)(s) << 32) | (u32_t)(f)))
# define SNTP_TIMESTAMP_TO_S64(t) \
    SNTP_SEC_FRAC_TO_S64(lwip_ntohl((t).sec), lwip_ntohl((t).frac))
#endif /* SNTP_COMP_ROUNDTRIP */

/**
 * 64-bit NTP timestamp, in network byte order.
 */
struct sntp_time {
  u32_t sec;
  u32_t frac;
};

/**
 * Timestamps to be extracted from the NTP header.
 */
struct sntp_timestamps {
#if SNTP_COMP_ROUNDTRIP || SNTP_CHECK_RESPONSE >= 2
  struct sntp_time orig;
  struct sntp_time recv;
#endif
  struct sntp_time xmit;
};

/**
 * SNTP packet format (without optional fields)
 * Timestamps are coded as 64 bits:
 * - signed 32 bits seconds since Feb 07, 2036, 06:28:16 UTC (epoch 1)
 * - unsigned 32 bits seconds fraction (2^32 = 1 second)
 */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct sntp_msg {
  PACK_STRUCT_FLD_8(u8_t  li_vn_mode);
  PACK_STRUCT_FLD_8(u8_t  stratum);
  PACK_STRUCT_FLD_8(u8_t  poll);
  PACK_STRUCT_FLD_8(u8_t  precision);
  PACK_STRUCT_FIELD(u32_t root_delay);
  PACK_STRUCT_FIELD(u32_t root_dispersion);
  PACK_STRUCT_FIELD(u32_t reference_identifier);
  PACK_STRUCT_FIELD(u32_t reference_timestamp[2]);
  PACK_STRUCT_FIELD(u32_t originate_timestamp[2]);
  PACK_STRUCT_FIELD(u32_t receive_timestamp[2]);
  PACK_STRUCT_FIELD(u32_t transmit_timestamp[2]);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/* function prototypes */
static void sntp_request(void *arg);

/** The operating mode */
static u8_t sntp_opmode;

/** The UDP pcb used by the SNTP client */
static struct udp_pcb *sntp_pcb;
/** Names/Addresses of servers */
struct sntp_server {
#if SNTP_SERVER_DNS
  char *name;
#endif /* SNTP_SERVER_DNS */
  ip_addr_t addr;
};
static struct sntp_server sntp_servers[SNTP_MAX_SERVERS];

#if SNTP_GET_SERVERS_FROM_DHCP
static u8_t sntp_set_servers_from_dhcp;
#endif /* SNTP_GET_SERVERS_FROM_DHCP */
#if SNTP_SUPPORT_MULTIPLE_SERVERS
/** The currently used server (initialized to 0) */
static u8_t sntp_current_server;
#else /* SNTP_SUPPORT_MULTIPLE_SERVERS */
#define sntp_current_server 0
#endif /* SNTP_SUPPORT_MULTIPLE_SERVERS */

#if SNTP_RETRY_TIMEOUT_EXP
#define SNTP_RESET_RETRY_TIMEOUT() sntp_retry_timeout = SNTP_RETRY_TIMEOUT
/** Retry time, initialized with SNTP_RETRY_TIMEOUT and doubled with each retry. */
static u32_t sntp_retry_timeout;
#else /* SNTP_RETRY_TIMEOUT_EXP */
#define SNTP_RESET_RETRY_TIMEOUT()
#define sntp_retry_timeout SNTP_RETRY_TIMEOUT
#endif /* SNTP_RETRY_TIMEOUT_EXP */

#if SNTP_CHECK_RESPONSE >= 1
/** Saves the last server address to compare with response */
static ip_addr_t sntp_last_server_address;
#endif /* SNTP_CHECK_RESPONSE >= 1 */

#if SNTP_CHECK_RESPONSE >= 2
/** Saves the last timestamp sent (which is sent back by the server)
 * to compare against in response. Stored in network byte order. */
static struct sntp_time sntp_last_timestamp_sent;
#endif /* SNTP_CHECK_RESPONSE >= 2 */

#if defined(LWIP_DEBUG) && !defined(sntp_format_time)
/* Debug print helper. */
static const char *
sntp_format_time(s32_t sec)
{
  time_t ut;
  ut = (time_t)((time_t)sec + (time_t)DIFF_SEC_1970_2036);
  return ctime(&ut);
}
#endif /* LWIP_DEBUG && !sntp_format_time */

/**
 * SNTP processing of received timestamp
 */
static void
sntp_process(const struct sntp_timestamps *timestamps)
{
  s32_t sec;
  u32_t frac;

  sec  = (s32_t)lwip_ntohl(timestamps->xmit.sec);
  frac = lwip_ntohl(timestamps->xmit.frac);

#if SNTP_COMP_ROUNDTRIP
# if SNTP_CHECK_RESPONSE >= 2
  if (timestamps->recv.sec != 0 || timestamps->recv.frac != 0)
# endif
  {
    s32_t dest_sec;
    u32_t dest_frac;
    u32_t step_sec;

    /* Get the destination time stamp, i.e. the current system time */
    SNTP_GET_SYSTEM_TIME_NTP(dest_sec, dest_frac);

    step_sec = (dest_sec < sec) ? ((u32_t)sec - (u32_t)dest_sec)
               : ((u32_t)dest_sec - (u32_t)sec);
    /* In order to avoid overflows, skip the compensation if the clock step
     * is larger than about 34 years. */
    if ((step_sec >> 30) == 0) {
      s64_t t1, t2, t3, t4;

      t4 = SNTP_SEC_FRAC_TO_S64(dest_sec, dest_frac);
      t3 = SNTP_SEC_FRAC_TO_S64(sec, frac);
      t1 = SNTP_TIMESTAMP_TO_S64(timestamps->orig);
      t2 = SNTP_TIMESTAMP_TO_S64(timestamps->recv);
      /* Clock offset calculation according to RFC 4330 */
      t4 += ((t2 - t1) + (t3 - t4)) / 2;

      sec  = (s32_t)((u64_t)t4 >> 32);
      frac = (u32_t)((u64_t)t4);
    }
  }
#endif /* SNTP_COMP_ROUNDTRIP */

  SNTP_SET_SYSTEM_TIME_NTP(sec, frac);
  LWIP_UNUSED_ARG(frac); /* might be unused if only seconds are set */
  LWIP_DEBUGF(SNTP_DEBUG_TRACE, ("sntp_process: %s, %" U32_F " us\n",
                                 sntp_format_time(sec), SNTP_FRAC_TO_US(frac)));
}

/**
 * Initialize request struct to be sent to server.
 */
static void
sntp_initialize_request(struct sntp_msg *req)
{
  memset(req, 0, SNTP_MSG_LEN);
  req->li_vn_mode = SNTP_LI_NO_WARNING | SNTP_VERSION | SNTP_MODE_CLIENT;

#if SNTP_CHECK_RESPONSE >= 2 || SNTP_COMP_ROUNDTRIP
  {
    s32_t secs;
    u32_t sec, frac;
    /* Get the transmit timestamp */
    SNTP_GET_SYSTEM_TIME_NTP(secs, frac);
    sec  = lwip_htonl((u32_t)secs);
    frac = lwip_htonl(frac);

# if SNTP_CHECK_RESPONSE >= 2
    sntp_last_timestamp_sent.sec  = sec;
    sntp_last_timestamp_sent.frac = frac;
# endif
    req->transmit_timestamp[0] = sec;
    req->transmit_timestamp[1] = frac;
  }
#endif /* SNTP_CHECK_RESPONSE >= 2 || SNTP_COMP_ROUNDTRIP */
}

/**
 * Retry: send a new request (and increase retry timeout).
 *
 * @param arg is unused (only necessary to conform to sys_timeout)
 */
static void
sntp_retry(void *arg)
{
  LWIP_UNUSED_ARG(arg);

  LWIP_DEBUGF(SNTP_DEBUG_STATE, ("sntp_retry: Next request will be sent in %"U32_F" ms\n",
                                 sntp_retry_timeout));

  /* set up a timer to send a retry and increase the retry delay */
  sys_timeout(sntp_retry_timeout, sntp_request, NULL);

#if SNTP_RETRY_TIMEOUT_EXP
  {
    u32_t new_retry_timeout;
    /* increase the timeout for next retry */
    new_retry_timeout = sntp_retry_timeout << 1;
    /* limit to maximum timeout and prevent overflow */
    if ((new_retry_timeout <= SNTP_RETRY_TIMEOUT_MAX) &&
        (new_retry_timeout > sntp_retry_timeout)) {
      sntp_retry_timeout = new_retry_timeout;
    }
  }
#endif /* SNTP_RETRY_TIMEOUT_EXP */
}

#if SNTP_SUPPORT_MULTIPLE_SERVERS
/**
 * If Kiss-of-Death is received (or another packet parsing error),
 * try the next server or retry the current server and increase the retry
 * timeout if only one server is available.
 * (implicitly, SNTP_MAX_SERVERS > 1)
 *
 * @param arg is unused (only necessary to conform to sys_timeout)
 */
static void
sntp_try_next_server(void *arg)
{
  u8_t old_server, i;
  LWIP_UNUSED_ARG(arg);

  old_server = sntp_current_server;
  for (i = 0; i < SNTP_MAX_SERVERS - 1; i++) {
    sntp_current_server++;
    if (sntp_current_server >= SNTP_MAX_SERVERS) {
      sntp_current_server = 0;
    }
    if (!ip_addr_isany(&sntp_servers[sntp_current_server].addr)
#if SNTP_SERVER_DNS
        || (sntp_servers[sntp_current_server].name != NULL)
#endif
       ) {
      LWIP_DEBUGF(SNTP_DEBUG_STATE, ("sntp_try_next_server: Sending request to server %"U16_F"\n",
                                     (u16_t)sntp_current_server));
      /* new server: reset retry timeout */
      SNTP_RESET_RETRY_TIMEOUT();
      /* instantly send a request to the next server */
      sntp_request(NULL);
      return;
    }
  }
  /* no other valid server found */
  sntp_current_server = old_server;
  sntp_retry(NULL);
}
#else /* SNTP_SUPPORT_MULTIPLE_SERVERS */
/* Always retry on error if only one server is supported */
#define sntp_try_next_server    sntp_retry
#endif /* SNTP_SUPPORT_MULTIPLE_SERVERS */

/** UDP recv callback for the sntp pcb */
static void
sntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
  struct sntp_timestamps timestamps;
  u8_t mode;
  u8_t stratum;
  err_t err;

  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);

  err = ERR_ARG;
#if SNTP_CHECK_RESPONSE >= 1
  /* check server address and port */
  if (((sntp_opmode != SNTP_OPMODE_POLL) || ip_addr_cmp(addr, &sntp_last_server_address)) &&
      (port == SNTP_PORT))
#else /* SNTP_CHECK_RESPONSE >= 1 */
  LWIP_UNUSED_ARG(addr);
  LWIP_UNUSED_ARG(port);
#endif /* SNTP_CHECK_RESPONSE >= 1 */
  {
    /* process the response */
    if (p->tot_len == SNTP_MSG_LEN) {
      mode = pbuf_get_at(p, SNTP_OFFSET_LI_VN_MODE) & SNTP_MODE_MASK;
      /* if this is a SNTP response... */
      if (((sntp_opmode == SNTP_OPMODE_POLL)       && (mode == SNTP_MODE_SERVER)) ||
          ((sntp_opmode == SNTP_OPMODE_LISTENONLY) && (mode == SNTP_MODE_BROADCAST))) {
        stratum = pbuf_get_at(p, SNTP_OFFSET_STRATUM);

        if (stratum == SNTP_STRATUM_KOD) {
          /* Kiss-of-death packet. Use another server or increase UPDATE_DELAY. */
          err = SNTP_ERR_KOD;
          LWIP_DEBUGF(SNTP_DEBUG_STATE, ("sntp_recv: Received Kiss-of-Death\n"));
        } else {
          pbuf_copy_partial(p, &timestamps, sizeof(timestamps), SNTP_OFFSET_TIMESTAMPS);
#if SNTP_CHECK_RESPONSE >= 2
          /* check originate_timetamp against sntp_last_timestamp_sent */
          if (timestamps.orig.sec != sntp_last_timestamp_sent.sec ||
              timestamps.orig.frac != sntp_last_timestamp_sent.frac) {
            LWIP_DEBUGF(SNTP_DEBUG_WARN,
                        ("sntp_recv: Invalid originate timestamp in response\n"));
          } else
#endif /* SNTP_CHECK_RESPONSE >= 2 */
            /* @todo: add code for SNTP_CHECK_RESPONSE >= 3 and >= 4 here */
          {
            /* correct answer */
            err = ERR_OK;
          }
        }
      } else {
        LWIP_DEBUGF(SNTP_DEBUG_WARN, ("sntp_recv: Invalid mode in response: %"U16_F"\n", (u16_t)mode));
        /* wait for correct response */
        err = ERR_TIMEOUT;
      }
    } else {
      LWIP_DEBUGF(SNTP_DEBUG_WARN, ("sntp_recv: Invalid packet length: %"U16_F"\n", p->tot_len));
    }
  }
#if SNTP_CHECK_RESPONSE >= 1
  else {
    /* packet from wrong remote address or port, wait for correct response */
    err = ERR_TIMEOUT;
  }
#endif /* SNTP_CHECK_RESPONSE >= 1 */

  pbuf_free(p);

  if (err == ERR_OK) {
    /* correct packet received: process it it */
    sntp_process(&timestamps);

    /* Set up timeout for next request (only if poll response was received)*/
    if (sntp_opmode == SNTP_OPMODE_POLL) {
      u32_t sntp_update_delay;
      sys_untimeout(sntp_try_next_server, NULL);
      sys_untimeout(sntp_request, NULL);

      /* Correct response, reset retry timeout */
      SNTP_RESET_RETRY_TIMEOUT();

      sntp_update_delay = (u32_t)SNTP_UPDATE_DELAY;
      sys_timeout(sntp_update_delay, sntp_request, NULL);
      LWIP_DEBUGF(SNTP_DEBUG_STATE, ("sntp_recv: Scheduled next time request: %"U32_F" ms\n",
                                     sntp_update_delay));
    }
  } else if (err == SNTP_ERR_KOD) {
    /* KOD errors are only processed in case of an explicit poll response */
    if (sntp_opmode == SNTP_OPMODE_POLL) {
      /* Kiss-of-death packet. Use another server or increase UPDATE_DELAY. */
      sntp_try_next_server(NULL);
    }
  } else {
    /* ignore any broken packet, poll mode: retry after timeout to avoid flooding */
  }
}

/** Actually send an sntp request to a server.
 *
 * @param server_addr resolved IP address of the SNTP server
 */
static void
sntp_send_request(const ip_addr_t *server_addr)
{
  struct pbuf *p;
  p = pbuf_alloc(PBUF_TRANSPORT, SNTP_MSG_LEN, PBUF_RAM);
  if (p != NULL) {
    struct sntp_msg *sntpmsg = (struct sntp_msg *)p->payload;
    LWIP_DEBUGF(SNTP_DEBUG_STATE, ("sntp_send_request: Sending request to server\n"));
    /* initialize request message */
    sntp_initialize_request(sntpmsg);
    /* send request */
    udp_sendto(sntp_pcb, p, server_addr, SNTP_PORT);
    /* free the pbuf after sending it */
    pbuf_free(p);
    /* set up receive timeout: try next server or retry on timeout */
    sys_timeout((u32_t)SNTP_RECV_TIMEOUT, sntp_try_next_server, NULL);
#if SNTP_CHECK_RESPONSE >= 1
    /* save server address to verify it in sntp_recv */
    ip_addr_set(&sntp_last_server_address, server_addr);
#endif /* SNTP_CHECK_RESPONSE >= 1 */
  } else {
    LWIP_DEBUGF(SNTP_DEBUG_SERIOUS, ("sntp_send_request: Out of memory, trying again in %"U32_F" ms\n",
                                     (u32_t)SNTP_RETRY_TIMEOUT));
    /* out of memory: set up a timer to send a retry */
    sys_timeout((u32_t)SNTP_RETRY_TIMEOUT, sntp_request, NULL);
  }
}

#if SNTP_SERVER_DNS
/**
 * DNS found callback when using DNS names as server address.
 */
static void
sntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg)
{
  LWIP_UNUSED_ARG(hostname);
  LWIP_UNUSED_ARG(arg);

  if (ipaddr != NULL) {
    /* Address resolved, send request */
    LWIP_DEBUGF(SNTP_DEBUG_STATE, ("sntp_dns_found: Server address resolved, sending request\n"));
    sntp_send_request(ipaddr);
  } else {
    /* DNS resolving failed -> try another server */
    LWIP_DEBUGF(SNTP_DEBUG_WARN_STATE, ("sntp_dns_found: Failed to resolve server address resolved, trying next server\n"));
    sntp_try_next_server(NULL);
  }
}
#endif /* SNTP_SERVER_DNS */

/**
 * Send out an sntp request.
 *
 * @param arg is unused (only necessary to conform to sys_timeout)
 */
static void
sntp_request(void *arg)
{
  ip_addr_t sntp_server_address;
  err_t err;

  LWIP_UNUSED_ARG(arg);

  /* initialize SNTP server address */
#if SNTP_SERVER_DNS
  if (sntp_servers[sntp_current_server].name) {
    /* always resolve the name and rely on dns-internal caching & timeout */
    ip_addr_set_zero(&sntp_servers[sntp_current_server].addr);
    err = dns_gethostbyname(sntp_servers[sntp_current_server].name, &sntp_server_address,
                            sntp_dns_found, NULL);
    if (err == ERR_INPROGRESS) {
      /* DNS request sent, wait for sntp_dns_found being called */
      LWIP_DEBUGF(SNTP_DEBUG_STATE, ("sntp_request: Waiting for server address to be resolved.\n"));
      return;
    } else if (err == ERR_OK) {
      sntp_servers[sntp_current_server].addr = sntp_server_address;
    }
  } else
#endif /* SNTP_SERVER_DNS */
  {
    sntp_server_address = sntp_servers[sntp_current_server].addr;
    err = (ip_addr_isany_val(sntp_server_address)) ? ERR_ARG : ERR_OK;
  }

  if (err == ERR_OK) {
    LWIP_DEBUGF(SNTP_DEBUG_TRACE, ("sntp_request: current server address is %s\n",
                                   ipaddr_ntoa(&sntp_server_address)));
    sntp_send_request(&sntp_server_address);
  } else {
    /* address conversion failed, try another server */
    LWIP_DEBUGF(SNTP_DEBUG_WARN_STATE, ("sntp_request: Invalid server address, trying next server.\n"));
    sys_timeout((u32_t)SNTP_RETRY_TIMEOUT, sntp_try_next_server, NULL);
  }
}

/**
 * @ingroup sntp
 * Initialize this module.
 * Send out request instantly or after SNTP_STARTUP_DELAY(_FUNC).
 */
void
sntp_init(void)
{
#ifdef SNTP_SERVER_ADDRESS
#if SNTP_SERVER_DNS
  sntp_setservername(0, SNTP_SERVER_ADDRESS);
#else
#error SNTP_SERVER_ADDRESS string not supported SNTP_SERVER_DNS==0
#endif
#endif /* SNTP_SERVER_ADDRESS */

  if (sntp_pcb == NULL) {
    sntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    LWIP_ASSERT("Failed to allocate udp pcb for sntp client", sntp_pcb != NULL);
    if (sntp_pcb != NULL) {
      udp_recv(sntp_pcb, sntp_recv, NULL);

      if (sntp_opmode == SNTP_OPMODE_POLL) {
        SNTP_RESET_RETRY_TIMEOUT();
#if SNTP_STARTUP_DELAY
        sys_timeout((u32_t)SNTP_STARTUP_DELAY_FUNC, sntp_request, NULL);
#else
        sntp_request(NULL);
#endif
      } else if (sntp_opmode == SNTP_OPMODE_LISTENONLY) {
        ip_set_option(sntp_pcb, SOF_BROADCAST);
        udp_bind(sntp_pcb, IP_ANY_TYPE, SNTP_PORT);
      }
    }
  }
}

/**
 * @ingroup sntp
 * Stop this module.
 */
void
sntp_stop(void)
{
  if (sntp_pcb != NULL) {
    sys_untimeout(sntp_request, NULL);
    sys_untimeout(sntp_try_next_server, NULL);
    udp_remove(sntp_pcb);
    sntp_pcb = NULL;
  }
}

/**
 * @ingroup sntp
 * Get enabled state.
 */
u8_t sntp_enabled(void)
{
  return (sntp_pcb != NULL) ? 1 : 0;
}

/**
 * @ingroup sntp
 * Sets the operating mode.
 * @param operating_mode one of the available operating modes
 */
void
sntp_setoperatingmode(u8_t operating_mode)
{
  LWIP_ASSERT("Invalid operating mode", operating_mode <= SNTP_OPMODE_LISTENONLY);
  LWIP_ASSERT("Operating mode must not be set while SNTP client is running", sntp_pcb == NULL);
  sntp_opmode = operating_mode;
}

/**
 * @ingroup sntp
 * Gets the operating mode.
 */
u8_t
sntp_getoperatingmode(void)
{
  return sntp_opmode;
}

#if SNTP_GET_SERVERS_FROM_DHCP
/**
 * Config SNTP server handling by IP address, name, or DHCP; clear table
 * @param set_servers_from_dhcp enable or disable getting server addresses from dhcp
 */
void
sntp_servermode_dhcp(int set_servers_from_dhcp)
{
  u8_t new_mode = set_servers_from_dhcp ? 1 : 0;
  if (sntp_set_servers_from_dhcp != new_mode) {
    sntp_set_servers_from_dhcp = new_mode;
  }
}
#endif /* SNTP_GET_SERVERS_FROM_DHCP */

/**
 * @ingroup sntp
 * Initialize one of the NTP servers by IP address
 *
 * @param idx the index of the NTP server to set must be < SNTP_MAX_SERVERS
 * @param server IP address of the NTP server to set
 */
void
sntp_setserver(u8_t idx, const ip_addr_t *server)
{
  if (idx < SNTP_MAX_SERVERS) {
    if (server != NULL) {
      sntp_servers[idx].addr = (*server);
    } else {
      ip_addr_set_zero(&sntp_servers[idx].addr);
    }
#if SNTP_SERVER_DNS
    sntp_servers[idx].name = NULL;
#endif
  }
}

#if LWIP_DHCP && SNTP_GET_SERVERS_FROM_DHCP
/**
 * Initialize one of the NTP servers by IP address, required by DHCP
 *
 * @param numdns the index of the NTP server to set must be < SNTP_MAX_SERVERS
 * @param dnsserver IP address of the NTP server to set
 */
void
dhcp_set_ntp_servers(u8_t num, const ip4_addr_t *server)
{
  LWIP_DEBUGF(SNTP_DEBUG_TRACE, ("sntp: %s %u.%u.%u.%u as NTP server #%u via DHCP\n",
                                 (sntp_set_servers_from_dhcp ? "Got" : "Rejected"),
                                 ip4_addr1(server), ip4_addr2(server), ip4_addr3(server), ip4_addr4(server), num));
  if (sntp_set_servers_from_dhcp && num) {
    u8_t i;
    for (i = 0; (i < num) && (i < SNTP_MAX_SERVERS); i++) {
      ip_addr_t addr;
      ip_addr_copy_from_ip4(addr, server[i]);
      sntp_setserver(i, &addr);
    }
    for (i = num; i < SNTP_MAX_SERVERS; i++) {
      sntp_setserver(i, NULL);
    }
  }
}
#endif /* LWIP_DHCP && SNTP_GET_SERVERS_FROM_DHCP */

/**
 * @ingroup sntp
 * Obtain one of the currently configured by IP address (or DHCP) NTP servers
 *
 * @param idx the index of the NTP server
 * @return IP address of the indexed NTP server or "ip_addr_any" if the NTP
 *         server has not been configured by address (or at all).
 */
const ip_addr_t *
sntp_getserver(u8_t idx)
{
  if (idx < SNTP_MAX_SERVERS) {
    return &sntp_servers[idx].addr;
  }
  return IP_ADDR_ANY;
}

#if SNTP_SERVER_DNS
/**
 * Initialize one of the NTP servers by name
 *
 * @param numdns the index of the NTP server to set must be < SNTP_MAX_SERVERS
 * @param dnsserver DNS name of the NTP server to set, to be resolved at contact time
 */
void
sntp_setservername(u8_t idx, char *server)
{
  if (idx < SNTP_MAX_SERVERS) {
    sntp_servers[idx].name = server;
  }
}

/**
 * Obtain one of the currently configured by name NTP servers.
 *
 * @param numdns the index of the NTP server
 * @return IP address of the indexed NTP server or NULL if the NTP
 *         server has not been configured by name (or at all)
 */
char *
sntp_getservername(u8_t idx)
{
  if (idx < SNTP_MAX_SERVERS) {
    return sntp_servers[idx].name;
  }
  return NULL;
}
#endif /* SNTP_SERVER_DNS */

#endif /* LWIP_UDP */

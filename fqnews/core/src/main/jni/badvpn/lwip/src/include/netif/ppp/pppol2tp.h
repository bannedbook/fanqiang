/**
 * @file
 * Network Point to Point Protocol over Layer 2 Tunneling Protocol header file.
 *
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

#include "netif/ppp/ppp_opts.h"
#if PPP_SUPPORT && PPPOL2TP_SUPPORT /* don't build if not configured for use in lwipopts.h */

#ifndef PPPOL2TP_H
#define PPPOL2TP_H

#include "ppp.h"

/* Timeout */
#define PPPOL2TP_CONTROL_TIMEOUT         (5*1000)  /* base for quick timeout calculation */
#define PPPOL2TP_SLOW_RETRY              (60*1000) /* persistent retry interval */

#define PPPOL2TP_MAXSCCRQ                4         /* retry SCCRQ four times (quickly) */
#define PPPOL2TP_MAXICRQ                 4         /* retry IRCQ four times */
#define PPPOL2TP_MAXICCN                 4         /* retry ICCN four times */

/* L2TP header flags */
#define PPPOL2TP_HEADERFLAG_CONTROL      0x8000
#define PPPOL2TP_HEADERFLAG_LENGTH       0x4000
#define PPPOL2TP_HEADERFLAG_SEQUENCE     0x0800
#define PPPOL2TP_HEADERFLAG_OFFSET       0x0200
#define PPPOL2TP_HEADERFLAG_PRIORITY     0x0100
#define PPPOL2TP_HEADERFLAG_VERSION      0x0002

/* Mandatory bits for control: Control, Length, Sequence, Version 2 */
#define PPPOL2TP_HEADERFLAG_CONTROL_MANDATORY     (PPPOL2TP_HEADERFLAG_CONTROL|PPPOL2TP_HEADERFLAG_LENGTH|PPPOL2TP_HEADERFLAG_SEQUENCE|PPPOL2TP_HEADERFLAG_VERSION)
/* Forbidden bits for control: Offset, Priority */
#define PPPOL2TP_HEADERFLAG_CONTROL_FORBIDDEN     (PPPOL2TP_HEADERFLAG_OFFSET|PPPOL2TP_HEADERFLAG_PRIORITY)

/* Mandatory bits for data: Version 2 */
#define PPPOL2TP_HEADERFLAG_DATA_MANDATORY        (PPPOL2TP_HEADERFLAG_VERSION)

/* AVP (Attribute Value Pair) header */
#define PPPOL2TP_AVPHEADERFLAG_MANDATORY  0x8000
#define PPPOL2TP_AVPHEADERFLAG_HIDDEN     0x4000
#define PPPOL2TP_AVPHEADERFLAG_LENGTHMASK 0x03ff

/* -- AVP - Message type */
#define PPPOL2TP_AVPTYPE_MESSAGE      0 /* Message type */

/* Control Connection Management */
#define PPPOL2TP_MESSAGETYPE_SCCRQ    1 /* Start Control Connection Request */
#define PPPOL2TP_MESSAGETYPE_SCCRP    2 /* Start Control Connection Reply */
#define PPPOL2TP_MESSAGETYPE_SCCCN    3 /* Start Control Connection Connected */
#define PPPOL2TP_MESSAGETYPE_STOPCCN  4 /* Stop Control Connection Notification */
#define PPPOL2TP_MESSAGETYPE_HELLO    6 /* Hello */
/* Call Management */
#define PPPOL2TP_MESSAGETYPE_OCRQ     7 /* Outgoing Call Request */
#define PPPOL2TP_MESSAGETYPE_OCRP     8 /* Outgoing Call Reply */
#define PPPOL2TP_MESSAGETYPE_OCCN     9 /* Outgoing Call Connected */
#define PPPOL2TP_MESSAGETYPE_ICRQ    10 /* Incoming Call Request */
#define PPPOL2TP_MESSAGETYPE_ICRP    11 /* Incoming Call Reply */
#define PPPOL2TP_MESSAGETYPE_ICCN    12 /* Incoming Call Connected */
#define PPPOL2TP_MESSAGETYPE_CDN     14 /* Call Disconnect Notify */
/* Error reporting */
#define PPPOL2TP_MESSAGETYPE_WEN     15 /* WAN Error Notify */
/* PPP Session Control */
#define PPPOL2TP_MESSAGETYPE_SLI     16 /* Set Link Info */

/* -- AVP - Result code */
#define PPPOL2TP_AVPTYPE_RESULTCODE   1 /* Result code */
#define PPPOL2TP_RESULTCODE           1 /* General request to clear control connection */

/* -- AVP - Protocol version (!= L2TP Header version) */
#define PPPOL2TP_AVPTYPE_VERSION      2
#define PPPOL2TP_VERSION         0x0100 /* L2TP Protocol version 1, revision 0 */

/* -- AVP - Framing capabilities */
#define PPPOL2TP_AVPTYPE_FRAMINGCAPABILITIES           3 /* Bearer capabilities */
#define PPPOL2TP_FRAMINGCAPABILITIES          0x00000003 /* Async + Sync framing */

/* -- AVP - Bearer capabilities */
#define PPPOL2TP_AVPTYPE_BEARERCAPABILITIES           4 /* Bearer capabilities */
#define PPPOL2TP_BEARERCAPABILITIES          0x00000003 /* Analog + Digital Access */

/* -- AVP - Tie breaker */
#define PPPOL2TP_AVPTYPE_TIEBREAKER   5

/* -- AVP - Host name */
#define PPPOL2TP_AVPTYPE_HOSTNAME     7 /* Host name */
#define PPPOL2TP_HOSTNAME        "lwIP" /* FIXME: make it configurable */

/* -- AVP - Vendor name */
#define PPPOL2TP_AVPTYPE_VENDORNAME   8 /* Vendor name */
#define PPPOL2TP_VENDORNAME      "lwIP" /* FIXME: make it configurable */

/* -- AVP - Assign tunnel ID */
#define PPPOL2TP_AVPTYPE_TUNNELID     9 /* Assign Tunnel ID */

/* -- AVP - Receive window size */
#define PPPOL2TP_AVPTYPE_RECEIVEWINDOWSIZE  10 /* Receive window size */
#define PPPOL2TP_RECEIVEWINDOWSIZE           8 /* FIXME: make it configurable */

/* -- AVP - Challenge */
#define PPPOL2TP_AVPTYPE_CHALLENGE   11 /* Challenge */

/* -- AVP - Cause code */
#define PPPOL2TP_AVPTYPE_CAUSECODE   12 /* Cause code*/

/* -- AVP - Challenge response */
#define PPPOL2TP_AVPTYPE_CHALLENGERESPONSE   13 /* Challenge response */
#define PPPOL2TP_AVPTYPE_CHALLENGERESPONSE_SIZE  16

/* -- AVP - Assign session ID */
#define PPPOL2TP_AVPTYPE_SESSIONID   14 /* Assign Session ID */

/* -- AVP - Call serial number */
#define PPPOL2TP_AVPTYPE_CALLSERIALNUMBER   15 /* Call Serial Number */

/* -- AVP - Framing type */
#define PPPOL2TP_AVPTYPE_FRAMINGTYPE         19 /* Framing Type */
#define PPPOL2TP_FRAMINGTYPE         0x00000001 /* Sync framing */

/* -- AVP - TX Connect Speed */
#define PPPOL2TP_AVPTYPE_TXCONNECTSPEED      24 /* TX Connect Speed */
#define PPPOL2TP_TXCONNECTSPEED       100000000 /* Connect speed: 100 Mbits/s */

/* L2TP Session state */
#define PPPOL2TP_STATE_INITIAL     0
#define PPPOL2TP_STATE_SCCRQ_SENT  1
#define PPPOL2TP_STATE_ICRQ_SENT   2
#define PPPOL2TP_STATE_ICCN_SENT   3
#define PPPOL2TP_STATE_DATA        4

#define PPPOL2TP_OUTPUT_DATA_HEADER_LEN   6 /* Our data header len */

/*
 * PPPoL2TP interface control block.
 */
typedef struct pppol2tp_pcb_s pppol2tp_pcb;
struct pppol2tp_pcb_s {
  ppp_pcb *ppp;                /* PPP PCB */
  u8_t phase;                  /* L2TP phase */
  struct udp_pcb *udp;         /* UDP L2TP Socket */
  struct netif *netif;         /* Output interface, used as a default route */
  ip_addr_t remote_ip;         /* LNS IP Address */
  u16_t remote_port;           /* LNS port */
#if PPPOL2TP_AUTH_SUPPORT
  const u8_t *secret;          /* Secret string */
  u8_t secret_len;             /* Secret string length */
  u8_t secret_rv[16];          /* Random vector */
  u8_t challenge_hash[16];     /* Challenge response */
  u8_t send_challenge;         /* Boolean whether the next sent packet should contains a challenge response */
#endif /* PPPOL2TP_AUTH_SUPPORT */

  u16_t tunnel_port;           /* Tunnel port */
  u16_t our_ns;                /* NS to peer */
  u16_t peer_nr;               /* NR from peer */
  u16_t peer_ns;               /* NS from peer */
  u16_t source_tunnel_id;      /* Tunnel ID assigned by peer */
  u16_t remote_tunnel_id;      /* Tunnel ID assigned to peer */
  u16_t source_session_id;     /* Session ID assigned by peer */
  u16_t remote_session_id;     /* Session ID assigned to peer */

  u8_t sccrq_retried;          /* number of SCCRQ retries already done */
  u8_t icrq_retried;           /* number of ICRQ retries already done */
  u8_t iccn_retried;           /* number of ICCN retries already done */
};


/* Create a new L2TP session. */
ppp_pcb *pppol2tp_create(struct netif *pppif,
       struct netif *netif, const ip_addr_t *ipaddr, u16_t port,
       const u8_t *secret, u8_t secret_len,
       ppp_link_status_cb_fn link_status_cb, void *ctx_cb);

#endif /* PPPOL2TP_H */
#endif /* PPP_SUPPORT && PPPOL2TP_SUPPORT */

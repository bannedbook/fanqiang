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

#ifndef LWIP_PPP_OPTS_H
#define LWIP_PPP_OPTS_H

#include "lwip/opt.h"

/**
 * PPP_SUPPORT==1: Enable PPP.
 */
#ifndef PPP_SUPPORT
#define PPP_SUPPORT                     0
#endif

/**
 * PPPOE_SUPPORT==1: Enable PPP Over Ethernet
 */
#ifndef PPPOE_SUPPORT
#define PPPOE_SUPPORT                   0
#endif

/**
 * PPPOL2TP_SUPPORT==1: Enable PPP Over L2TP
 */
#ifndef PPPOL2TP_SUPPORT
#define PPPOL2TP_SUPPORT                0
#endif

/**
 * PPPOL2TP_AUTH_SUPPORT==1: Enable PPP Over L2TP Auth (enable MD5 support)
 */
#ifndef PPPOL2TP_AUTH_SUPPORT
#define PPPOL2TP_AUTH_SUPPORT           PPPOL2TP_SUPPORT
#endif

/**
 * PPPOS_SUPPORT==1: Enable PPP Over Serial
 */
#ifndef PPPOS_SUPPORT
#define PPPOS_SUPPORT                   PPP_SUPPORT
#endif

/**
 * LWIP_PPP_API==1: Enable PPP API (in pppapi.c)
 */
#ifndef LWIP_PPP_API
#define LWIP_PPP_API                    (PPP_SUPPORT && (NO_SYS == 0))
#endif

/**
 * MEMP_NUM_PPP_PCB: the number of simultaneously active PPP
 * connections (requires the PPP_SUPPORT option)
 */
#ifndef MEMP_NUM_PPP_PCB
#define MEMP_NUM_PPP_PCB                1
#endif

/**
 * PPP_NUM_TIMEOUTS_PER_PCB: the number of sys_timeouts running in parallel per
 * ppp_pcb. This is a conservative default which needs to be checked...
 */
#ifndef PPP_NUM_TIMEOUTS_PER_PCB
#define PPP_NUM_TIMEOUTS_PER_PCB        6
#endif

/* The number of sys_timeouts required for the PPP module */
#define PPP_NUM_TIMEOUTS                (PPP_SUPPORT * PPP_NUM_TIMEOUTS_PER_PCB * MEMP_NUM_PPP_PCB)

#if PPP_SUPPORT

/**
 * MEMP_NUM_PPPOS_INTERFACES: the number of concurrently active PPPoS
 * interfaces (only used with PPPOS_SUPPORT==1)
 */
#ifndef MEMP_NUM_PPPOS_INTERFACES
#define MEMP_NUM_PPPOS_INTERFACES       MEMP_NUM_PPP_PCB
#endif

/**
 * MEMP_NUM_PPPOE_INTERFACES: the number of concurrently active PPPoE
 * interfaces (only used with PPPOE_SUPPORT==1)
 */
#ifndef MEMP_NUM_PPPOE_INTERFACES
#define MEMP_NUM_PPPOE_INTERFACES       1
#endif

/**
 * MEMP_NUM_PPPOL2TP_INTERFACES: the number of concurrently active PPPoL2TP
 * interfaces (only used with PPPOL2TP_SUPPORT==1)
 */
#ifndef MEMP_NUM_PPPOL2TP_INTERFACES
#define MEMP_NUM_PPPOL2TP_INTERFACES       1
#endif

/**
 * MEMP_NUM_PPP_API_MSG: Number of concurrent PPP API messages (in pppapi.c)
 */
#ifndef MEMP_NUM_PPP_API_MSG
#define MEMP_NUM_PPP_API_MSG 5
#endif

/**
 * PPP_DEBUG: Enable debugging for PPP.
 */
#ifndef PPP_DEBUG
#define PPP_DEBUG                       LWIP_DBG_OFF
#endif

/**
 * PPP_INPROC_IRQ_SAFE==1 call pppos_input() using tcpip_callback().
 *
 * Please read the "PPPoS input path" chapter in the PPP documentation about this option.
 */
#ifndef PPP_INPROC_IRQ_SAFE
#define PPP_INPROC_IRQ_SAFE             0
#endif

/**
 * PRINTPKT_SUPPORT==1: Enable PPP print packet support
 *
 * Mandatory for debugging, it displays exchanged packet content in debug trace.
 */
#ifndef PRINTPKT_SUPPORT
#define PRINTPKT_SUPPORT                0
#endif

/**
 * PPP_IPV4_SUPPORT==1: Enable PPP IPv4 support
 */
#ifndef PPP_IPV4_SUPPORT
#define PPP_IPV4_SUPPORT                (LWIP_IPV4)
#endif

/**
 * PPP_IPV6_SUPPORT==1: Enable PPP IPv6 support
 */
#ifndef PPP_IPV6_SUPPORT
#define PPP_IPV6_SUPPORT                (LWIP_IPV6)
#endif

/**
 * PPP_NOTIFY_PHASE==1: Support PPP notify phase support
 *
 * PPP notify phase support allows you to set a callback which is
 * called on change of the internal PPP state machine.
 *
 * This can be used for example to set a LED pattern depending on the
 * current phase of the PPP session.
 */
#ifndef PPP_NOTIFY_PHASE
#define PPP_NOTIFY_PHASE                0
#endif

/**
 * pbuf_type PPP is using for LCP, PAP, CHAP, EAP, CCP, IPCP and IP6CP packets.
 *
 * Memory allocated must be single buffered for PPP to works, it requires pbuf
 * that are not going to be chained when allocated. This requires setting
 * PBUF_POOL_BUFSIZE to at least 512 bytes, which is quite huge for small systems.
 *
 * Setting PPP_USE_PBUF_RAM to 1 makes PPP use memory from heap where continuous
 * buffers are required, allowing you to use a smaller PBUF_POOL_BUFSIZE.
 */
#ifndef PPP_USE_PBUF_RAM
#define PPP_USE_PBUF_RAM                0
#endif

/**
 * PPP_FCS_TABLE: Keep a 256*2 byte table to speed up FCS calculation for PPPoS
 */
#ifndef PPP_FCS_TABLE
#define PPP_FCS_TABLE                   1
#endif

/**
 * PAP_SUPPORT==1: Support PAP.
 */
#ifndef PAP_SUPPORT
#define PAP_SUPPORT                     0
#endif

/**
 * CHAP_SUPPORT==1: Support CHAP.
 */
#ifndef CHAP_SUPPORT
#define CHAP_SUPPORT                    0
#endif

/**
 * MSCHAP_SUPPORT==1: Support MSCHAP.
 */
#ifndef MSCHAP_SUPPORT
#define MSCHAP_SUPPORT                  0
#endif
#if MSCHAP_SUPPORT
/* MSCHAP requires CHAP support */
#undef CHAP_SUPPORT
#define CHAP_SUPPORT                    1
#endif /* MSCHAP_SUPPORT */

/**
 * EAP_SUPPORT==1: Support EAP.
 */
#ifndef EAP_SUPPORT
#define EAP_SUPPORT                     0
#endif

/**
 * CCP_SUPPORT==1: Support CCP.
 */
#ifndef CCP_SUPPORT
#define CCP_SUPPORT                     0
#endif

/**
 * MPPE_SUPPORT==1: Support MPPE.
 */
#ifndef MPPE_SUPPORT
#define MPPE_SUPPORT                    0
#endif
#if MPPE_SUPPORT
/* MPPE requires CCP support */
#undef CCP_SUPPORT
#define CCP_SUPPORT                     1
/* MPPE requires MSCHAP support */
#undef MSCHAP_SUPPORT
#define MSCHAP_SUPPORT                  1
/* MSCHAP requires CHAP support */
#undef CHAP_SUPPORT
#define CHAP_SUPPORT                    1
#endif /* MPPE_SUPPORT */

/**
 * CBCP_SUPPORT==1: Support CBCP. CURRENTLY NOT SUPPORTED! DO NOT SET!
 */
#ifndef CBCP_SUPPORT
#define CBCP_SUPPORT                    0
#endif

/**
 * ECP_SUPPORT==1: Support ECP. CURRENTLY NOT SUPPORTED! DO NOT SET!
 */
#ifndef ECP_SUPPORT
#define ECP_SUPPORT                     0
#endif

/**
 * DEMAND_SUPPORT==1: Support dial on demand. CURRENTLY NOT SUPPORTED! DO NOT SET!
 */
#ifndef DEMAND_SUPPORT
#define DEMAND_SUPPORT                  0
#endif

/**
 * LQR_SUPPORT==1: Support Link Quality Report. Do nothing except exchanging some LCP packets.
 */
#ifndef LQR_SUPPORT
#define LQR_SUPPORT                     0
#endif

/**
 * PPP_SERVER==1: Enable PPP server support (waiting for incoming PPP session).
 *
 * Currently only supported for PPPoS.
 */
#ifndef PPP_SERVER
#define PPP_SERVER                      0
#endif

#if PPP_SERVER
/*
 * PPP_OUR_NAME: Our name for authentication purposes
 */
#ifndef PPP_OUR_NAME
#define PPP_OUR_NAME                    "lwIP"
#endif
#endif /* PPP_SERVER */

/**
 * VJ_SUPPORT==1: Support VJ header compression.
 */
#ifndef VJ_SUPPORT
#define VJ_SUPPORT                      1
#endif
/* VJ compression is only supported for TCP over IPv4 over PPPoS. */
#if !PPPOS_SUPPORT || !PPP_IPV4_SUPPORT || !LWIP_TCP
#undef VJ_SUPPORT
#define VJ_SUPPORT                      0
#endif /* !PPPOS_SUPPORT */

/**
 * PPP_MD5_RANDM==1: Use MD5 for better randomness.
 * Enabled by default if CHAP, EAP, or L2TP AUTH support is enabled.
 */
#ifndef PPP_MD5_RANDM
#define PPP_MD5_RANDM                   (CHAP_SUPPORT || EAP_SUPPORT || PPPOL2TP_AUTH_SUPPORT)
#endif

/**
 * PolarSSL embedded library
 *
 *
 * lwIP contains some files fetched from the latest BSD release of
 * the PolarSSL project (PolarSSL 0.10.1-bsd) for ciphers and encryption
 * methods we need for lwIP PPP support.
 *
 * The PolarSSL files were cleaned to contain only the necessary struct
 * fields and functions needed for lwIP.
 *
 * The PolarSSL API was not changed at all, so if you are already using
 * PolarSSL you can choose to skip the compilation of the included PolarSSL
 * library into lwIP.
 *
 * If you are not using the embedded copy you must include external
 * libraries into your arch/cc.h port file.
 *
 * Beware of the stack requirements which can be a lot larger if you are not
 * using our cleaned PolarSSL library.
 */

/**
 * LWIP_USE_EXTERNAL_POLARSSL: Use external PolarSSL library
 */
#ifndef LWIP_USE_EXTERNAL_POLARSSL
#define LWIP_USE_EXTERNAL_POLARSSL      0
#endif

/**
 * LWIP_USE_EXTERNAL_MBEDTLS: Use external mbed TLS library
 */
#ifndef LWIP_USE_EXTERNAL_MBEDTLS
#define LWIP_USE_EXTERNAL_MBEDTLS       0
#endif

/*
 * PPP Timeouts
 */

/**
 * FSM_DEFTIMEOUT: Timeout time in seconds
 */
#ifndef FSM_DEFTIMEOUT
#define FSM_DEFTIMEOUT                  6
#endif

/**
 * FSM_DEFMAXTERMREQS: Maximum Terminate-Request transmissions
 */
#ifndef FSM_DEFMAXTERMREQS
#define FSM_DEFMAXTERMREQS              2
#endif

/**
 * FSM_DEFMAXCONFREQS: Maximum Configure-Request transmissions
 */
#ifndef FSM_DEFMAXCONFREQS
#define FSM_DEFMAXCONFREQS              10
#endif

/**
 * FSM_DEFMAXNAKLOOPS: Maximum number of nak loops
 */
#ifndef FSM_DEFMAXNAKLOOPS
#define FSM_DEFMAXNAKLOOPS              5
#endif

/**
 * UPAP_DEFTIMEOUT: Timeout (seconds) for retransmitting req
 */
#ifndef UPAP_DEFTIMEOUT
#define UPAP_DEFTIMEOUT                 6
#endif

/**
 * UPAP_DEFTRANSMITS: Maximum number of auth-reqs to send
 */
#ifndef UPAP_DEFTRANSMITS
#define UPAP_DEFTRANSMITS               10
#endif

#if PPP_SERVER
/**
 * UPAP_DEFREQTIME: Time to wait for auth-req from peer
 */
#ifndef UPAP_DEFREQTIME
#define UPAP_DEFREQTIME                 30
#endif
#endif /* PPP_SERVER */

/**
 * CHAP_DEFTIMEOUT: Timeout (seconds) for retransmitting req
 */
#ifndef CHAP_DEFTIMEOUT
#define CHAP_DEFTIMEOUT                 6
#endif

/**
 * CHAP_DEFTRANSMITS: max # times to send challenge
 */
#ifndef CHAP_DEFTRANSMITS
#define CHAP_DEFTRANSMITS               10
#endif

#if PPP_SERVER
/**
 * CHAP_DEFRECHALLENGETIME: If this option is > 0, rechallenge the peer every n seconds
 */
#ifndef CHAP_DEFRECHALLENGETIME
#define CHAP_DEFRECHALLENGETIME         0
#endif
#endif /* PPP_SERVER */

/**
 * EAP_DEFREQTIME: Time to wait for peer request
 */
#ifndef EAP_DEFREQTIME
#define EAP_DEFREQTIME                  6
#endif

/**
 * EAP_DEFALLOWREQ: max # times to accept requests
 */
#ifndef EAP_DEFALLOWREQ
#define EAP_DEFALLOWREQ                 10
#endif

#if PPP_SERVER
/**
 * EAP_DEFTIMEOUT: Timeout (seconds) for rexmit
 */
#ifndef EAP_DEFTIMEOUT
#define EAP_DEFTIMEOUT                  6
#endif

/**
 * EAP_DEFTRANSMITS: max # times to transmit
 */
#ifndef EAP_DEFTRANSMITS
#define EAP_DEFTRANSMITS                10
#endif
#endif /* PPP_SERVER */

/**
 * LCP_DEFLOOPBACKFAIL: Default number of times we receive our magic number from the peer
 * before deciding the link is looped-back.
 */
#ifndef LCP_DEFLOOPBACKFAIL
#define LCP_DEFLOOPBACKFAIL             10
#endif

/**
 * LCP_ECHOINTERVAL: Interval in seconds between keepalive echo requests, 0 to disable.
 */
#ifndef LCP_ECHOINTERVAL
#define LCP_ECHOINTERVAL                0
#endif

/**
 * LCP_MAXECHOFAILS: Number of unanswered echo requests before failure.
 */
#ifndef LCP_MAXECHOFAILS
#define LCP_MAXECHOFAILS                3
#endif

/**
 * PPP_MAXIDLEFLAG: Max Xmit idle time (in ms) before resend flag char.
 */
#ifndef PPP_MAXIDLEFLAG
#define PPP_MAXIDLEFLAG                 100
#endif

/**
 * PPP Packet sizes
 */

/**
 * PPP_MRU: Default MRU
 */
#ifndef PPP_MRU
#define PPP_MRU                         1500
#endif

/**
 * PPP_DEFMRU: Default MRU to try
 */
#ifndef PPP_DEFMRU
#define PPP_DEFMRU                      1500
#endif

/**
 * PPP_MAXMRU: Normally limit MRU to this (pppd default = 16384)
 */
#ifndef PPP_MAXMRU
#define PPP_MAXMRU                      1500
#endif

/**
 * PPP_MINMRU: No MRUs below this
 */
#ifndef PPP_MINMRU
#define PPP_MINMRU                      128
#endif

/**
 * PPPOL2TP_DEFMRU: Default MTU and MRU for L2TP
 * Default = 1500 - PPPoE(6) - PPP Protocol(2) - IPv4 header(20) - UDP Header(8)
 * - L2TP Header(6) - HDLC Header(2) - PPP Protocol(2) - MPPE Header(2) - PPP Protocol(2)
 */
#if PPPOL2TP_SUPPORT
#ifndef PPPOL2TP_DEFMRU
#define PPPOL2TP_DEFMRU                 1450
#endif
#endif /* PPPOL2TP_SUPPORT */

/**
 * MAXNAMELEN: max length of hostname or name for auth
 */
#ifndef MAXNAMELEN
#define MAXNAMELEN                      256
#endif

/**
 * MAXSECRETLEN: max length of password or secret
 */
#ifndef MAXSECRETLEN
#define MAXSECRETLEN                    256
#endif

/* ------------------------------------------------------------------------- */

/*
 * Build triggers for embedded PolarSSL
 */
#if !LWIP_USE_EXTERNAL_POLARSSL && !LWIP_USE_EXTERNAL_MBEDTLS

/* CHAP, EAP, L2TP AUTH and MD5 Random require MD5 support */
#if CHAP_SUPPORT || EAP_SUPPORT || PPPOL2TP_AUTH_SUPPORT || PPP_MD5_RANDM
#define LWIP_INCLUDED_POLARSSL_MD5      1
#endif /* CHAP_SUPPORT || EAP_SUPPORT || PPPOL2TP_AUTH_SUPPORT || PPP_MD5_RANDM */

#if MSCHAP_SUPPORT

/* MSCHAP require MD4 support */
#define LWIP_INCLUDED_POLARSSL_MD4      1
/* MSCHAP require SHA1 support */
#define LWIP_INCLUDED_POLARSSL_SHA1     1
/* MSCHAP require DES support */
#define LWIP_INCLUDED_POLARSSL_DES      1

/* MS-CHAP support is required for MPPE */
#if MPPE_SUPPORT
/* MPPE require ARC4 support */
#define LWIP_INCLUDED_POLARSSL_ARC4     1
#endif /* MPPE_SUPPORT */

#endif /* MSCHAP_SUPPORT */

#endif /* !LWIP_USE_EXTERNAL_POLARSSL && !LWIP_USE_EXTERNAL_MBEDTLS */

/* Default value if unset */
#ifndef LWIP_INCLUDED_POLARSSL_MD4
#define LWIP_INCLUDED_POLARSSL_MD4      0
#endif /* LWIP_INCLUDED_POLARSSL_MD4 */
#ifndef LWIP_INCLUDED_POLARSSL_MD5
#define LWIP_INCLUDED_POLARSSL_MD5      0
#endif /* LWIP_INCLUDED_POLARSSL_MD5 */
#ifndef LWIP_INCLUDED_POLARSSL_SHA1
#define LWIP_INCLUDED_POLARSSL_SHA1     0
#endif /* LWIP_INCLUDED_POLARSSL_SHA1 */
#ifndef LWIP_INCLUDED_POLARSSL_DES
#define LWIP_INCLUDED_POLARSSL_DES      0
#endif /* LWIP_INCLUDED_POLARSSL_DES */
#ifndef LWIP_INCLUDED_POLARSSL_ARC4
#define LWIP_INCLUDED_POLARSSL_ARC4     0
#endif /* LWIP_INCLUDED_POLARSSL_ARC4 */

#endif /* PPP_SUPPORT */

#endif /* LWIP_PPP_OPTS_H */

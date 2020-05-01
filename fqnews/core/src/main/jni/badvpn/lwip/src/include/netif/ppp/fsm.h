/*
 * fsm.h - {Link, IP} Control Protocol Finite State Machine definitions.
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
 * $Id: fsm.h,v 1.10 2004/11/13 02:28:15 paulus Exp $
 */

#include "netif/ppp/ppp_opts.h"
#if PPP_SUPPORT /* don't build if not configured for use in lwipopts.h */

#ifndef FSM_H
#define	FSM_H

#include "ppp.h"

/*
 * Packet header = Code, id, length.
 */
#define HEADERLEN	4


/*
 *  CP (LCP, IPCP, etc.) codes.
 */
#define CONFREQ		1	/* Configuration Request */
#define CONFACK		2	/* Configuration Ack */
#define CONFNAK		3	/* Configuration Nak */
#define CONFREJ		4	/* Configuration Reject */
#define TERMREQ		5	/* Termination Request */
#define TERMACK		6	/* Termination Ack */
#define CODEREJ		7	/* Code Reject */


/*
 * Each FSM is described by an fsm structure and fsm callbacks.
 */
typedef struct fsm {
    ppp_pcb *pcb;		/* PPP Interface */
    const struct fsm_callbacks *callbacks;	/* Callback routines */
    const char *term_reason;	/* Reason for closing protocol */
    u8_t seen_ack;		/* Have received valid Ack/Nak/Rej to Req */
				  /* -- This is our only flag, we might use u_int :1 if we have more flags */
    u16_t protocol;		/* Data Link Layer Protocol field value */
    u8_t state;			/* State */
    u8_t flags;			/* Contains option bits */
    u8_t id;			/* Current id */
    u8_t reqid;			/* Current request id */
    u8_t retransmits;		/* Number of retransmissions left */
    u8_t nakloops;		/* Number of nak loops since last ack */
    u8_t rnakloops;		/* Number of naks received */
    u8_t maxnakloops;		/* Maximum number of nak loops tolerated
				   (necessary because IPCP require a custom large max nak loops value) */
    u8_t term_reason_len;	/* Length of term_reason */
} fsm;


typedef struct fsm_callbacks {
    void (*resetci)		/* Reset our Configuration Information */
		(fsm *);
    int  (*cilen)		/* Length of our Configuration Information */
		(fsm *);
    void (*addci) 		/* Add our Configuration Information */
		(fsm *, u_char *, int *);
    int  (*ackci)		/* ACK our Configuration Information */
		(fsm *, u_char *, int);
    int  (*nakci)		/* NAK our Configuration Information */
		(fsm *, u_char *, int, int);
    int  (*rejci)		/* Reject our Configuration Information */
		(fsm *, u_char *, int);
    int  (*reqci)		/* Request peer's Configuration Information */
		(fsm *, u_char *, int *, int);
    void (*up)			/* Called when fsm reaches PPP_FSM_OPENED state */
		(fsm *);
    void (*down)		/* Called when fsm leaves PPP_FSM_OPENED state */
		(fsm *);
    void (*starting)		/* Called when we want the lower layer */
		(fsm *);
    void (*finished)		/* Called when we don't want the lower layer */
		(fsm *);
    void (*protreject)		/* Called when Protocol-Reject received */
		(int);
    void (*retransmit)		/* Retransmission is necessary */
		(fsm *);
    int  (*extcode)		/* Called when unknown code received */
		(fsm *, int, int, u_char *, int);
    const char *proto_name;	/* String name for protocol (for messages) */
} fsm_callbacks;


/*
 * Link states.
 */
#define PPP_FSM_INITIAL		0	/* Down, hasn't been opened */
#define PPP_FSM_STARTING	1	/* Down, been opened */
#define PPP_FSM_CLOSED		2	/* Up, hasn't been opened */
#define PPP_FSM_STOPPED		3	/* Open, waiting for down event */
#define PPP_FSM_CLOSING		4	/* Terminating the connection, not open */
#define PPP_FSM_STOPPING	5	/* Terminating, but open */
#define PPP_FSM_REQSENT		6	/* We've sent a Config Request */
#define PPP_FSM_ACKRCVD		7	/* We've received a Config Ack */
#define PPP_FSM_ACKSENT		8	/* We've sent a Config Ack */
#define PPP_FSM_OPENED		9	/* Connection available */


/*
 * Flags - indicate options controlling FSM operation
 */
#define OPT_PASSIVE	1	/* Don't die if we don't get a response */
#define OPT_RESTART	2	/* Treat 2nd OPEN as DOWN, UP */
#define OPT_SILENT	4	/* Wait for peer to speak first */


/*
 * Timeouts.
 */
#if 0 /* moved to ppp_opts.h */
#define DEFTIMEOUT	3	/* Timeout time in seconds */
#define DEFMAXTERMREQS	2	/* Maximum Terminate-Request transmissions */
#define DEFMAXCONFREQS	10	/* Maximum Configure-Request transmissions */
#define DEFMAXNAKLOOPS	5	/* Maximum number of nak loops */
#endif /* moved to ppp_opts.h */


/*
 * Prototypes
 */
void fsm_init(fsm *f);
void fsm_lowerup(fsm *f);
void fsm_lowerdown(fsm *f);
void fsm_open(fsm *f);
void fsm_close(fsm *f, const char *reason);
void fsm_input(fsm *f, u_char *inpacket, int l);
void fsm_protreject(fsm *f);
void fsm_sdata(fsm *f, u_char code, u_char id, const u_char *data, int datalen);


#endif /* FSM_H */
#endif /* PPP_SUPPORT */

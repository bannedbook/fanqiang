/*
 * ipv6cp.c - PPP IPV6 Control Protocol.
 *
 * Copyright (c) 1999 Tommi Komulainen.  All rights reserved.
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
 * 3. The name(s) of the authors of this software must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission.
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Tommi Komulainen
 *     <Tommi.Komulainen@iki.fi>".
 *
 * THE AUTHORS OF THIS SOFTWARE DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/*  Original version, based on RFC2023 :

    Copyright (c) 1995, 1996, 1997 Francis.Dupont@inria.fr, INRIA Rocquencourt,
    Alain.Durand@imag.fr, IMAG,
    Jean-Luc.Richier@imag.fr, IMAG-LSR.

    Copyright (c) 1998, 1999 Francis.Dupont@inria.fr, GIE DYADE,
    Alain.Durand@imag.fr, IMAG,
    Jean-Luc.Richier@imag.fr, IMAG-LSR.

    Ce travail a été fait au sein du GIE DYADE (Groupement d'Intérêt
    Économique ayant pour membres BULL S.A. et l'INRIA).

    Ce logiciel informatique est disponible aux conditions
    usuelles dans la recherche, c'est-à-dire qu'il peut
    être utilisé, copié, modifié, distribué à l'unique
    condition que ce texte soit conservé afin que
    l'origine de ce logiciel soit reconnue.

    Le nom de l'Institut National de Recherche en Informatique
    et en Automatique (INRIA), de l'IMAG, ou d'une personne morale
    ou physique ayant participé à l'élaboration de ce logiciel ne peut
    être utilisé sans son accord préalable explicite.

    Ce logiciel est fourni tel quel sans aucune garantie,
    support ou responsabilité d'aucune sorte.
    Ce logiciel est dérivé de sources d'origine
    "University of California at Berkeley" et
    "Digital Equipment Corporation" couvertes par des copyrights.

    L'Institut d'Informatique et de Mathématiques Appliquées de Grenoble (IMAG)
    est une fédération d'unités mixtes de recherche du CNRS, de l'Institut National
    Polytechnique de Grenoble et de l'Université Joseph Fourier regroupant
    sept laboratoires dont le laboratoire Logiciels, Systèmes, Réseaux (LSR).

    This work has been done in the context of GIE DYADE (joint R & D venture
    between BULL S.A. and INRIA).

    This software is available with usual "research" terms
    with the aim of retain credits of the software. 
    Permission to use, copy, modify and distribute this software for any
    purpose and without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies,
    and the name of INRIA, IMAG, or any contributor not be used in advertising
    or publicity pertaining to this material without the prior explicit
    permission. The software is provided "as is" without any
    warranties, support or liabilities of any kind.
    This software is derived from source code from
    "University of California at Berkeley" and
    "Digital Equipment Corporation" protected by copyrights.

    Grenoble's Institute of Computer Science and Applied Mathematics (IMAG)
    is a federation of seven research units funded by the CNRS, National
    Polytechnic Institute of Grenoble and University Joseph Fourier.
    The research unit in Software, Systems, Networks (LSR) is member of IMAG.
*/

/*
 * Derived from :
 *
 *
 * ipcp.c - PPP IP Control Protocol.
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
 * $Id: ipv6cp.c,v 1.21 2005/08/25 23:59:34 paulus Exp $ 
 */

/*
 * @todo: 
 *
 * Proxy Neighbour Discovery.
 *
 * Better defines for selecting the ordering of
 *   interface up / set address.
 */

#include "netif/ppp/ppp_opts.h"
#if PPP_SUPPORT && PPP_IPV6_SUPPORT  /* don't build if not configured for use in lwipopts.h */

#if 0 /* UNUSED */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif /* UNUSED */

#include "netif/ppp/ppp_impl.h"
#include "netif/ppp/fsm.h"
#include "netif/ppp/ipcp.h"
#include "netif/ppp/ipv6cp.h"
#include "netif/ppp/magic.h"

/* global vars */
#if 0 /* UNUSED */
int no_ifaceid_neg = 0;
#endif /* UNUSED */

/*
 * Callbacks for fsm code.  (CI = Configuration Information)
 */
static void ipv6cp_resetci(fsm *f); /* Reset our CI */
static int  ipv6cp_cilen(fsm *f); /* Return length of our CI */
static void ipv6cp_addci(fsm *f, u_char *ucp, int *lenp); /* Add our CI */
static int  ipv6cp_ackci(fsm *f, u_char *p, int len); /* Peer ack'd our CI */
static int  ipv6cp_nakci(fsm *f, u_char *p, int len, int treat_as_reject); /* Peer nak'd our CI */
static int  ipv6cp_rejci(fsm *f, u_char *p, int len); /* Peer rej'd our CI */
static int  ipv6cp_reqci(fsm *f, u_char *inp, int *len, int reject_if_disagree); /* Rcv CI */
static void ipv6cp_up(fsm *f); /* We're UP */
static void ipv6cp_down(fsm *f); /* We're DOWN */
static void ipv6cp_finished(fsm *f); /* Don't need lower layer */

static const fsm_callbacks ipv6cp_callbacks = { /* IPV6CP callback routines */
    ipv6cp_resetci,		/* Reset our Configuration Information */
    ipv6cp_cilen,		/* Length of our Configuration Information */
    ipv6cp_addci,		/* Add our Configuration Information */
    ipv6cp_ackci,		/* ACK our Configuration Information */
    ipv6cp_nakci,		/* NAK our Configuration Information */
    ipv6cp_rejci,		/* Reject our Configuration Information */
    ipv6cp_reqci,		/* Request peer's Configuration Information */
    ipv6cp_up,			/* Called when fsm reaches OPENED state */
    ipv6cp_down,		/* Called when fsm leaves OPENED state */
    NULL,			/* Called when we want the lower layer up */
    ipv6cp_finished,		/* Called when we want the lower layer down */
    NULL,			/* Called when Protocol-Reject received */
    NULL,			/* Retransmission is necessary */
    NULL,			/* Called to handle protocol-specific codes */
    "IPV6CP"			/* String name of protocol */
};

#if PPP_OPTIONS
/*
 * Command-line options.
 */
static int setifaceid(char **arg));
static void printifaceid(option_t *,
			      void (*)(void *, char *, ...), void *));

static option_t ipv6cp_option_list[] = {
    { "ipv6", o_special, (void *)setifaceid,
      "Set interface identifiers for IPV6",
      OPT_A2PRINTER, (void *)printifaceid },

    { "+ipv6", o_bool, &ipv6cp_protent.enabled_flag,
      "Enable IPv6 and IPv6CP", OPT_PRIO | 1 },
    { "noipv6", o_bool, &ipv6cp_protent.enabled_flag,
      "Disable IPv6 and IPv6CP", OPT_PRIOSUB },
    { "-ipv6", o_bool, &ipv6cp_protent.enabled_flag,
      "Disable IPv6 and IPv6CP", OPT_PRIOSUB | OPT_ALIAS },

    { "ipv6cp-accept-local", o_bool, &ipv6cp_allowoptions[0].accept_local,
      "Accept peer's interface identifier for us", 1 },

    { "ipv6cp-use-ipaddr", o_bool, &ipv6cp_allowoptions[0].use_ip,
      "Use (default) IPv4 address as interface identifier", 1 },

    { "ipv6cp-use-persistent", o_bool, &ipv6cp_wantoptions[0].use_persistent,
      "Use uniquely-available persistent value for link local address", 1 },

    { "ipv6cp-restart", o_int, &ipv6cp_fsm[0].timeouttime,
      "Set timeout for IPv6CP", OPT_PRIO },
    { "ipv6cp-max-terminate", o_int, &ipv6cp_fsm[0].maxtermtransmits,
      "Set max #xmits for term-reqs", OPT_PRIO },
    { "ipv6cp-max-configure", o_int, &ipv6cp_fsm[0].maxconfreqtransmits,
      "Set max #xmits for conf-reqs", OPT_PRIO },
    { "ipv6cp-max-failure", o_int, &ipv6cp_fsm[0].maxnakloops,
      "Set max #conf-naks for IPv6CP", OPT_PRIO },

   { NULL }
};
#endif /* PPP_OPTIONS */

/*
 * Protocol entry points from main code.
 */
static void ipv6cp_init(ppp_pcb *pcb);
static void ipv6cp_open(ppp_pcb *pcb);
static void ipv6cp_close(ppp_pcb *pcb, const char *reason);
static void ipv6cp_lowerup(ppp_pcb *pcb);
static void ipv6cp_lowerdown(ppp_pcb *pcb);
static void ipv6cp_input(ppp_pcb *pcb, u_char *p, int len);
static void ipv6cp_protrej(ppp_pcb *pcb);
#if PPP_OPTIONS
static void ipv6_check_options(void);
#endif /* PPP_OPTIONS */
#if DEMAND_SUPPORT
static int  ipv6_demand_conf(int u);
#endif /* DEMAND_SUPPORT */
#if PRINTPKT_SUPPORT
static int ipv6cp_printpkt(const u_char *p, int plen,
		void (*printer)(void *, const char *, ...), void *arg);
#endif /* PRINTPKT_SUPPORT */
#if DEMAND_SUPPORT
static int ipv6_active_pkt(u_char *pkt, int len);
#endif /* DEMAND_SUPPORT */

const struct protent ipv6cp_protent = {
    PPP_IPV6CP,
    ipv6cp_init,
    ipv6cp_input,
    ipv6cp_protrej,
    ipv6cp_lowerup,
    ipv6cp_lowerdown,
    ipv6cp_open,
    ipv6cp_close,
#if PRINTPKT_SUPPORT
    ipv6cp_printpkt,
#endif /* PRINTPKT_SUPPORT */
#if PPP_DATAINPUT
    NULL,
#endif /* PPP_DATAINPUT */
#if PRINTPKT_SUPPORT
    "IPV6CP",
    "IPV6",
#endif /* PRINTPKT_SUPPORT */
#if PPP_OPTIONS
    ipv6cp_option_list,
    ipv6_check_options,
#endif /* PPP_OPTIONS */
#if DEMAND_SUPPORT
    ipv6_demand_conf,
    ipv6_active_pkt
#endif /* DEMAND_SUPPORT */
};

static void ipv6cp_clear_addrs(ppp_pcb *pcb, eui64_t ourid, eui64_t hisid);
#if 0 /* UNUSED */
static void ipv6cp_script(char *));
static void ipv6cp_script_done(void *));
#endif /* UNUSED */

/*
 * Lengths of configuration options.
 */
#define CILEN_VOID	2
#define CILEN_COMPRESS	4	/* length for RFC2023 compress opt. */
#define CILEN_IFACEID   10	/* RFC2472, interface identifier    */

#define CODENAME(x)	((x) == CONFACK ? "ACK" : \
			 (x) == CONFNAK ? "NAK" : "REJ")

#if 0 /* UNUSED */
/*
 * This state variable is used to ensure that we don't
 * run an ipcp-up/down script while one is already running.
 */
static enum script_state {
    s_down,
    s_up,
} ipv6cp_script_state;
static pid_t ipv6cp_script_pid;
#endif /* UNUSED */

static char *llv6_ntoa(eui64_t ifaceid);

#if PPP_OPTIONS
/*
 * setifaceid - set the interface identifiers manually
 */
static int
setifaceid(argv)
    char **argv;
{
    char *comma, *arg, c;
    ipv6cp_options *wo = &ipv6cp_wantoptions[0];
    struct in6_addr addr;
    static int prio_local, prio_remote;

#define VALIDID(a) ( (((a).s6_addr32[0] == 0) && ((a).s6_addr32[1] == 0)) && \
			(((a).s6_addr32[2] != 0) || ((a).s6_addr32[3] != 0)) )
    
    arg = *argv;
    if ((comma = strchr(arg, ',')) == NULL)
	comma = arg + strlen(arg);
    
    /* 
     * If comma first character, then no local identifier
     */
    if (comma != arg) {
	c = *comma;
	*comma = '\0';

	if (inet_pton(AF_INET6, arg, &addr) == 0 || !VALIDID(addr)) {
	    option_error("Illegal interface identifier (local): %s", arg);
	    return 0;
	}

	if (option_priority >= prio_local) {
	    eui64_copy(addr.s6_addr32[2], wo->ourid);
	    wo->opt_local = 1;
	    prio_local = option_priority;
	}
	*comma = c;
    }
    
    /*
     * If comma last character, the no remote identifier
     */
    if (*comma != 0 && *++comma != '\0') {
	if (inet_pton(AF_INET6, comma, &addr) == 0 || !VALIDID(addr)) {
	    option_error("Illegal interface identifier (remote): %s", comma);
	    return 0;
	}
	if (option_priority >= prio_remote) {
	    eui64_copy(addr.s6_addr32[2], wo->hisid);
	    wo->opt_remote = 1;
	    prio_remote = option_priority;
	}
    }

    if (override_value("+ipv6", option_priority, option_source))
	ipv6cp_protent.enabled_flag = 1;
    return 1;
}

static void
printifaceid(opt, printer, arg)
    option_t *opt;
    void (*printer)(void *, char *, ...));
    void *arg;
{
	ipv6cp_options *wo = &ipv6cp_wantoptions[0];

	if (wo->opt_local)
		printer(arg, "%s", llv6_ntoa(wo->ourid));
	printer(arg, ",");
	if (wo->opt_remote)
		printer(arg, "%s", llv6_ntoa(wo->hisid));
}
#endif /* PPP_OPTIONS */

/*
 * Make a string representation of a network address.
 */
static char *
llv6_ntoa(eui64_t ifaceid)
{
    static char b[26];

    sprintf(b, "fe80::%02x%02x:%02x%02x:%02x%02x:%02x%02x",
      ifaceid.e8[0], ifaceid.e8[1], ifaceid.e8[2], ifaceid.e8[3],
      ifaceid.e8[4], ifaceid.e8[5], ifaceid.e8[6], ifaceid.e8[7]);

    return b;
}


/*
 * ipv6cp_init - Initialize IPV6CP.
 */
static void ipv6cp_init(ppp_pcb *pcb) {
    fsm *f = &pcb->ipv6cp_fsm;
    ipv6cp_options *wo = &pcb->ipv6cp_wantoptions;
    ipv6cp_options *ao = &pcb->ipv6cp_allowoptions;

    f->pcb = pcb;
    f->protocol = PPP_IPV6CP;
    f->callbacks = &ipv6cp_callbacks;
    fsm_init(f);

#if 0 /* Not necessary, everything is cleared in ppp_new() */
    memset(wo, 0, sizeof(*wo));
    memset(ao, 0, sizeof(*ao));
#endif /* 0 */

    wo->accept_local = 1;
    wo->neg_ifaceid = 1;
    ao->neg_ifaceid = 1;

#ifdef IPV6CP_COMP
    wo->neg_vj = 1;
    ao->neg_vj = 1;
    wo->vj_protocol = IPV6CP_COMP;
#endif

}


/*
 * ipv6cp_open - IPV6CP is allowed to come up.
 */
static void ipv6cp_open(ppp_pcb *pcb) {
    fsm_open(&pcb->ipv6cp_fsm);
}


/*
 * ipv6cp_close - Take IPV6CP down.
 */
static void ipv6cp_close(ppp_pcb *pcb, const char *reason) {
    fsm_close(&pcb->ipv6cp_fsm, reason);
}


/*
 * ipv6cp_lowerup - The lower layer is up.
 */
static void ipv6cp_lowerup(ppp_pcb *pcb) {
    fsm_lowerup(&pcb->ipv6cp_fsm);
}


/*
 * ipv6cp_lowerdown - The lower layer is down.
 */
static void ipv6cp_lowerdown(ppp_pcb *pcb) {
    fsm_lowerdown(&pcb->ipv6cp_fsm);
}


/*
 * ipv6cp_input - Input IPV6CP packet.
 */
static void ipv6cp_input(ppp_pcb *pcb, u_char *p, int len) {
    fsm_input(&pcb->ipv6cp_fsm, p, len);
}


/*
 * ipv6cp_protrej - A Protocol-Reject was received for IPV6CP.
 *
 * Pretend the lower layer went down, so we shut up.
 */
static void ipv6cp_protrej(ppp_pcb *pcb) {
    fsm_lowerdown(&pcb->ipv6cp_fsm);
}


/*
 * ipv6cp_resetci - Reset our CI.
 */
static void ipv6cp_resetci(fsm *f) {
    ppp_pcb *pcb = f->pcb;
    ipv6cp_options *wo = &pcb->ipv6cp_wantoptions;
    ipv6cp_options *go = &pcb->ipv6cp_gotoptions;
    ipv6cp_options *ao = &pcb->ipv6cp_allowoptions;

    wo->req_ifaceid = wo->neg_ifaceid && ao->neg_ifaceid;
    
    if (!wo->opt_local) {
	eui64_magic_nz(wo->ourid);
    }
    
    *go = *wo;
    eui64_zero(go->hisid);	/* last proposed interface identifier */
}


/*
 * ipv6cp_cilen - Return length of our CI.
 */
static int ipv6cp_cilen(fsm *f) {
    ppp_pcb *pcb = f->pcb;
    ipv6cp_options *go = &pcb->ipv6cp_gotoptions;

#ifdef IPV6CP_COMP
#define LENCIVJ(neg)		(neg ? CILEN_COMPRESS : 0)
#endif /* IPV6CP_COMP */
#define LENCIIFACEID(neg)	(neg ? CILEN_IFACEID : 0)

    return (LENCIIFACEID(go->neg_ifaceid) +
#ifdef IPV6CP_COMP
	    LENCIVJ(go->neg_vj) +
#endif /* IPV6CP_COMP */
	    0);
}


/*
 * ipv6cp_addci - Add our desired CIs to a packet.
 */
static void ipv6cp_addci(fsm *f, u_char *ucp, int *lenp) {
    ppp_pcb *pcb = f->pcb;
    ipv6cp_options *go = &pcb->ipv6cp_gotoptions;
    int len = *lenp;

#ifdef IPV6CP_COMP
#define ADDCIVJ(opt, neg, val) \
    if (neg) { \
	int vjlen = CILEN_COMPRESS; \
	if (len >= vjlen) { \
	    PUTCHAR(opt, ucp); \
	    PUTCHAR(vjlen, ucp); \
	    PUTSHORT(val, ucp); \
	    len -= vjlen; \
	} else \
	    neg = 0; \
    }
#endif /* IPV6CP_COMP */

#define ADDCIIFACEID(opt, neg, val1) \
    if (neg) { \
	int idlen = CILEN_IFACEID; \
	if (len >= idlen) { \
	    PUTCHAR(opt, ucp); \
	    PUTCHAR(idlen, ucp); \
	    eui64_put(val1, ucp); \
	    len -= idlen; \
	} else \
	    neg = 0; \
    }

    ADDCIIFACEID(CI_IFACEID, go->neg_ifaceid, go->ourid);

#ifdef IPV6CP_COMP
    ADDCIVJ(CI_COMPRESSTYPE, go->neg_vj, go->vj_protocol);
#endif /* IPV6CP_COMP */

    *lenp -= len;
}


/*
 * ipv6cp_ackci - Ack our CIs.
 *
 * Returns:
 *	0 - Ack was bad.
 *	1 - Ack was good.
 */
static int ipv6cp_ackci(fsm *f, u_char *p, int len) {
    ppp_pcb *pcb = f->pcb;
    ipv6cp_options *go = &pcb->ipv6cp_gotoptions;
    u_short cilen, citype;
#ifdef IPV6CP_COMP
    u_short cishort;
#endif /* IPV6CP_COMP */
    eui64_t ifaceid;

    /*
     * CIs must be in exactly the same order that we sent...
     * Check packet length and CI length at each step.
     * If we find any deviations, then this packet is bad.
     */

#ifdef IPV6CP_COMP
#define ACKCIVJ(opt, neg, val) \
    if (neg) { \
	int vjlen = CILEN_COMPRESS; \
	if ((len -= vjlen) < 0) \
	    goto bad; \
	GETCHAR(citype, p); \
	GETCHAR(cilen, p); \
	if (cilen != vjlen || \
	    citype != opt)  \
	    goto bad; \
	GETSHORT(cishort, p); \
	if (cishort != val) \
	    goto bad; \
    }
#endif /* IPV6CP_COMP */

#define ACKCIIFACEID(opt, neg, val1) \
    if (neg) { \
	int idlen = CILEN_IFACEID; \
	if ((len -= idlen) < 0) \
	    goto bad; \
	GETCHAR(citype, p); \
	GETCHAR(cilen, p); \
	if (cilen != idlen || \
	    citype != opt) \
	    goto bad; \
	eui64_get(ifaceid, p); \
	if (! eui64_equals(val1, ifaceid)) \
	    goto bad; \
    }

    ACKCIIFACEID(CI_IFACEID, go->neg_ifaceid, go->ourid);

#ifdef IPV6CP_COMP
    ACKCIVJ(CI_COMPRESSTYPE, go->neg_vj, go->vj_protocol);
#endif /* IPV6CP_COMP */

    /*
     * If there are any remaining CIs, then this packet is bad.
     */
    if (len != 0)
	goto bad;
    return (1);

bad:
    IPV6CPDEBUG(("ipv6cp_ackci: received bad Ack!"));
    return (0);
}

/*
 * ipv6cp_nakci - Peer has sent a NAK for some of our CIs.
 * This should not modify any state if the Nak is bad
 * or if IPV6CP is in the OPENED state.
 *
 * Returns:
 *	0 - Nak was bad.
 *	1 - Nak was good.
 */
static int ipv6cp_nakci(fsm *f, u_char *p, int len, int treat_as_reject) {
    ppp_pcb *pcb = f->pcb;
    ipv6cp_options *go = &pcb->ipv6cp_gotoptions;
    u_char citype, cilen, *next;
#ifdef IPV6CP_COMP
    u_short cishort;
#endif /* IPV6CP_COMP */
    eui64_t ifaceid;
    ipv6cp_options no;		/* options we've seen Naks for */
    ipv6cp_options try_;	/* options to request next time */

    BZERO(&no, sizeof(no));
    try_ = *go;

    /*
     * Any Nak'd CIs must be in exactly the same order that we sent.
     * Check packet length and CI length at each step.
     * If we find any deviations, then this packet is bad.
     */
#define NAKCIIFACEID(opt, neg, code) \
    if (go->neg && \
	len >= (cilen = CILEN_IFACEID) && \
	p[1] == cilen && \
	p[0] == opt) { \
	len -= cilen; \
	INCPTR(2, p); \
	eui64_get(ifaceid, p); \
	no.neg = 1; \
	code \
    }

#ifdef IPV6CP_COMP
#define NAKCIVJ(opt, neg, code) \
    if (go->neg && \
	((cilen = p[1]) == CILEN_COMPRESS) && \
	len >= cilen && \
	p[0] == opt) { \
	len -= cilen; \
	INCPTR(2, p); \
	GETSHORT(cishort, p); \
	no.neg = 1; \
        code \
    }
#endif /* IPV6CP_COMP */

    /*
     * Accept the peer's idea of {our,his} interface identifier, if different
     * from our idea, only if the accept_{local,remote} flag is set.
     */
    NAKCIIFACEID(CI_IFACEID, neg_ifaceid,
		 if (treat_as_reject) {
		     try_.neg_ifaceid = 0;
		 } else if (go->accept_local) {
		     while (eui64_iszero(ifaceid) || 
			    eui64_equals(ifaceid, go->hisid)) /* bad luck */
			 eui64_magic(ifaceid);
		     try_.ourid = ifaceid;
		     IPV6CPDEBUG(("local LL address %s", llv6_ntoa(ifaceid)));
		 }
		 );

#ifdef IPV6CP_COMP
    NAKCIVJ(CI_COMPRESSTYPE, neg_vj,
	    {
		if (cishort == IPV6CP_COMP && !treat_as_reject) {
		    try_.vj_protocol = cishort;
		} else {
		    try_.neg_vj = 0;
		}
	    }
	    );
#endif /* IPV6CP_COMP */

    /*
     * There may be remaining CIs, if the peer is requesting negotiation
     * on an option that we didn't include in our request packet.
     * If they want to negotiate about interface identifier, we comply.
     * If they want us to ask for compression, we refuse.
     */
    while (len >= CILEN_VOID) {
	GETCHAR(citype, p);
	GETCHAR(cilen, p);
	if ( cilen < CILEN_VOID || (len -= cilen) < 0 )
	    goto bad;
	next = p + cilen - 2;

	switch (citype) {
#ifdef IPV6CP_COMP
	case CI_COMPRESSTYPE:
	    if (go->neg_vj || no.neg_vj ||
		(cilen != CILEN_COMPRESS))
		goto bad;
	    no.neg_vj = 1;
	    break;
#endif /* IPV6CP_COMP */
	case CI_IFACEID:
	    if (go->neg_ifaceid || no.neg_ifaceid || cilen != CILEN_IFACEID)
		goto bad;
	    try_.neg_ifaceid = 1;
	    eui64_get(ifaceid, p);
	    if (go->accept_local) {
		while (eui64_iszero(ifaceid) || 
		       eui64_equals(ifaceid, go->hisid)) /* bad luck */
		    eui64_magic(ifaceid);
		try_.ourid = ifaceid;
	    }
	    no.neg_ifaceid = 1;
	    break;
	default:
	    break;
	}
	p = next;
    }

    /* If there is still anything left, this packet is bad. */
    if (len != 0)
	goto bad;

    /*
     * OK, the Nak is good.  Now we can update state.
     */
    if (f->state != PPP_FSM_OPENED)
	*go = try_;

    return 1;

bad:
    IPV6CPDEBUG(("ipv6cp_nakci: received bad Nak!"));
    return 0;
}


/*
 * ipv6cp_rejci - Reject some of our CIs.
 */
static int ipv6cp_rejci(fsm *f, u_char *p, int len) {
    ppp_pcb *pcb = f->pcb;
    ipv6cp_options *go = &pcb->ipv6cp_gotoptions;
    u_char cilen;
#ifdef IPV6CP_COMP
    u_short cishort;
#endif /* IPV6CP_COMP */
    eui64_t ifaceid;
    ipv6cp_options try_;		/* options to request next time */

    try_ = *go;
    /*
     * Any Rejected CIs must be in exactly the same order that we sent.
     * Check packet length and CI length at each step.
     * If we find any deviations, then this packet is bad.
     */
#define REJCIIFACEID(opt, neg, val1) \
    if (go->neg && \
	len >= (cilen = CILEN_IFACEID) && \
	p[1] == cilen && \
	p[0] == opt) { \
	len -= cilen; \
	INCPTR(2, p); \
	eui64_get(ifaceid, p); \
	/* Check rejected value. */ \
	if (! eui64_equals(ifaceid, val1)) \
	    goto bad; \
	try_.neg = 0; \
    }

#ifdef IPV6CP_COMP
#define REJCIVJ(opt, neg, val) \
    if (go->neg && \
	p[1] == CILEN_COMPRESS && \
	len >= p[1] && \
	p[0] == opt) { \
	len -= p[1]; \
	INCPTR(2, p); \
	GETSHORT(cishort, p); \
	/* Check rejected value. */  \
	if (cishort != val) \
	    goto bad; \
	try_.neg = 0; \
     }
#endif /* IPV6CP_COMP */

    REJCIIFACEID(CI_IFACEID, neg_ifaceid, go->ourid);

#ifdef IPV6CP_COMP
    REJCIVJ(CI_COMPRESSTYPE, neg_vj, go->vj_protocol);
#endif /* IPV6CP_COMP */

    /*
     * If there are any remaining CIs, then this packet is bad.
     */
    if (len != 0)
	goto bad;
    /*
     * Now we can update state.
     */
    if (f->state != PPP_FSM_OPENED)
	*go = try_;
    return 1;

bad:
    IPV6CPDEBUG(("ipv6cp_rejci: received bad Reject!"));
    return 0;
}


/*
 * ipv6cp_reqci - Check the peer's requested CIs and send appropriate response.
 *
 * Returns: CONFACK, CONFNAK or CONFREJ and input packet modified
 * appropriately.  If reject_if_disagree is non-zero, doesn't return
 * CONFNAK; returns CONFREJ if it can't return CONFACK.
 *
 * inp = Requested CIs
 * len = Length of requested CIs
 *
 */
static int ipv6cp_reqci(fsm *f, u_char *inp, int *len, int reject_if_disagree) {
    ppp_pcb *pcb = f->pcb;
    ipv6cp_options *wo = &pcb->ipv6cp_wantoptions;
    ipv6cp_options *ho = &pcb->ipv6cp_hisoptions;
    ipv6cp_options *ao = &pcb->ipv6cp_allowoptions;
    ipv6cp_options *go = &pcb->ipv6cp_gotoptions;
    u_char *cip, *next;		/* Pointer to current and next CIs */
    u_short cilen, citype;	/* Parsed len, type */
#ifdef IPV6CP_COMP
    u_short cishort;		/* Parsed short value */
#endif /* IPV6CP_COMP */
    eui64_t ifaceid;		/* Parsed interface identifier */
    int rc = CONFACK;		/* Final packet return code */
    int orc;			/* Individual option return code */
    u_char *p;			/* Pointer to next char to parse */
    u_char *ucp = inp;		/* Pointer to current output char */
    int l = *len;		/* Length left */

    /*
     * Reset all his options.
     */
    BZERO(ho, sizeof(*ho));
    
    /*
     * Process all his options.
     */
    next = inp;
    while (l) {
	orc = CONFACK;			/* Assume success */
	cip = p = next;			/* Remember begining of CI */
	if (l < 2 ||			/* Not enough data for CI header or */
	    p[1] < 2 ||			/*  CI length too small or */
	    p[1] > l) {			/*  CI length too big? */
	    IPV6CPDEBUG(("ipv6cp_reqci: bad CI length!"));
	    orc = CONFREJ;		/* Reject bad CI */
	    cilen = l;			/* Reject till end of packet */
	    l = 0;			/* Don't loop again */
	    goto endswitch;
	}
	GETCHAR(citype, p);		/* Parse CI type */
	GETCHAR(cilen, p);		/* Parse CI length */
	l -= cilen;			/* Adjust remaining length */
	next += cilen;			/* Step to next CI */

	switch (citype) {		/* Check CI type */
	case CI_IFACEID:
	    IPV6CPDEBUG(("ipv6cp: received interface identifier "));

	    if (!ao->neg_ifaceid ||
		cilen != CILEN_IFACEID) {	/* Check CI length */
		orc = CONFREJ;		/* Reject CI */
		break;
	    }

	    /*
	     * If he has no interface identifier, or if we both have same 
	     * identifier then NAK it with new idea.
	     * In particular, if we don't know his identifier, but he does,
	     * then accept it.
	     */
	    eui64_get(ifaceid, p);
	    IPV6CPDEBUG(("(%s)", llv6_ntoa(ifaceid)));
	    if (eui64_iszero(ifaceid) && eui64_iszero(go->ourid)) {
		orc = CONFREJ;		/* Reject CI */
		break;
	    }
	    if (!eui64_iszero(wo->hisid) && 
		!eui64_equals(ifaceid, wo->hisid) && 
		eui64_iszero(go->hisid)) {
		    
		orc = CONFNAK;
		ifaceid = wo->hisid;
		go->hisid = ifaceid;
		DECPTR(sizeof(ifaceid), p);
		eui64_put(ifaceid, p);
	    } else
	    if (eui64_iszero(ifaceid) || eui64_equals(ifaceid, go->ourid)) {
		orc = CONFNAK;
		if (eui64_iszero(go->hisid))	/* first time, try option */
		    ifaceid = wo->hisid;
		while (eui64_iszero(ifaceid) || 
		       eui64_equals(ifaceid, go->ourid)) /* bad luck */
		    eui64_magic(ifaceid);
		go->hisid = ifaceid;
		DECPTR(sizeof(ifaceid), p);
		eui64_put(ifaceid, p);
	    }

	    ho->neg_ifaceid = 1;
	    ho->hisid = ifaceid;
	    break;

#ifdef IPV6CP_COMP
	case CI_COMPRESSTYPE:
	    IPV6CPDEBUG(("ipv6cp: received COMPRESSTYPE "));
	    if (!ao->neg_vj ||
		(cilen != CILEN_COMPRESS)) {
		orc = CONFREJ;
		break;
	    }
	    GETSHORT(cishort, p);
	    IPV6CPDEBUG(("(%d)", cishort));

	    if (!(cishort == IPV6CP_COMP)) {
		orc = CONFREJ;
		break;
	    }

	    ho->neg_vj = 1;
	    ho->vj_protocol = cishort;
	    break;
#endif /* IPV6CP_COMP */

	default:
	    orc = CONFREJ;
	    break;
	}

endswitch:
	IPV6CPDEBUG((" (%s)\n", CODENAME(orc)));

	if (orc == CONFACK &&		/* Good CI */
	    rc != CONFACK)		/*  but prior CI wasnt? */
	    continue;			/* Don't send this one */

	if (orc == CONFNAK) {		/* Nak this CI? */
	    if (reject_if_disagree)	/* Getting fed up with sending NAKs? */
		orc = CONFREJ;		/* Get tough if so */
	    else {
		if (rc == CONFREJ)	/* Rejecting prior CI? */
		    continue;		/* Don't send this one */
		if (rc == CONFACK) {	/* Ack'd all prior CIs? */
		    rc = CONFNAK;	/* Not anymore... */
		    ucp = inp;		/* Backup */
		}
	    }
	}

	if (orc == CONFREJ &&		/* Reject this CI */
	    rc != CONFREJ) {		/*  but no prior ones? */
	    rc = CONFREJ;
	    ucp = inp;			/* Backup */
	}

	/* Need to move CI? */
	if (ucp != cip)
	    MEMCPY(ucp, cip, cilen);	/* Move it */

	/* Update output pointer */
	INCPTR(cilen, ucp);
    }

    /*
     * If we aren't rejecting this packet, and we want to negotiate
     * their identifier and they didn't send their identifier, then we
     * send a NAK with a CI_IFACEID option appended.  We assume the
     * input buffer is long enough that we can append the extra
     * option safely.
     */
    if (rc != CONFREJ && !ho->neg_ifaceid &&
	wo->req_ifaceid && !reject_if_disagree) {
	if (rc == CONFACK) {
	    rc = CONFNAK;
	    ucp = inp;				/* reset pointer */
	    wo->req_ifaceid = 0;		/* don't ask again */
	}
	PUTCHAR(CI_IFACEID, ucp);
	PUTCHAR(CILEN_IFACEID, ucp);
	eui64_put(wo->hisid, ucp);
    }

    *len = ucp - inp;			/* Compute output length */
    IPV6CPDEBUG(("ipv6cp: returning Configure-%s", CODENAME(rc)));
    return (rc);			/* Return final code */
}

#if PPP_OPTIONS
/*
 * ipv6_check_options - check that any IP-related options are OK,
 * and assign appropriate defaults.
 */
static void ipv6_check_options() {
    ipv6cp_options *wo = &ipv6cp_wantoptions[0];

    if (!ipv6cp_protent.enabled_flag)
	return;

    /*
     * Persistent link-local id is only used when user has not explicitly
     * configure/hard-code the id
     */
    if ((wo->use_persistent) && (!wo->opt_local) && (!wo->opt_remote)) {

	/* 
	 * On systems where there are no Ethernet interfaces used, there
	 * may be other ways to obtain a persistent id. Right now, it
	 * will fall back to using magic [see eui64_magic] below when
	 * an EUI-48 from MAC address can't be obtained. Other possibilities
	 * include obtaining EEPROM serial numbers, or some other unique
	 * yet persistent number. On Sparc platforms, this is possible,
	 * but too bad there's no standards yet for x86 machines.
	 */
	if (ether_to_eui64(&wo->ourid)) {
	    wo->opt_local = 1;
	}
    }

    if (!wo->opt_local) {	/* init interface identifier */
	if (wo->use_ip && eui64_iszero(wo->ourid)) {
	    eui64_setlo32(wo->ourid, lwip_ntohl(ipcp_wantoptions[0].ouraddr));
	    if (!eui64_iszero(wo->ourid))
		wo->opt_local = 1;
	}
	
	while (eui64_iszero(wo->ourid))
	    eui64_magic(wo->ourid);
    }

    if (!wo->opt_remote) {
	if (wo->use_ip && eui64_iszero(wo->hisid)) {
	    eui64_setlo32(wo->hisid, lwip_ntohl(ipcp_wantoptions[0].hisaddr));
	    if (!eui64_iszero(wo->hisid))
		wo->opt_remote = 1;
	}
    }

    if (demand && (eui64_iszero(wo->ourid) || eui64_iszero(wo->hisid))) {
	option_error("local/remote LL address required for demand-dialling\n");
	exit(1);
    }
}
#endif /* PPP_OPTIONS */

#if DEMAND_SUPPORT
/*
 * ipv6_demand_conf - configure the interface as though
 * IPV6CP were up, for use with dial-on-demand.
 */
static int ipv6_demand_conf(int u) {
    ipv6cp_options *wo = &ipv6cp_wantoptions[u];

    if (!sif6up(u))
	return 0;

    if (!sif6addr(u, wo->ourid, wo->hisid))
	return 0;

    if (!sifnpmode(u, PPP_IPV6, NPMODE_QUEUE))
	return 0;

    ppp_notice("ipv6_demand_conf");
    ppp_notice("local  LL address %s", llv6_ntoa(wo->ourid));
    ppp_notice("remote LL address %s", llv6_ntoa(wo->hisid));

    return 1;
}
#endif /* DEMAND_SUPPORT */


/*
 * ipv6cp_up - IPV6CP has come UP.
 *
 * Configure the IPv6 network interface appropriately and bring it up.
 */
static void ipv6cp_up(fsm *f) {
    ppp_pcb *pcb = f->pcb;
    ipv6cp_options *wo = &pcb->ipv6cp_wantoptions;
    ipv6cp_options *ho = &pcb->ipv6cp_hisoptions;
    ipv6cp_options *go = &pcb->ipv6cp_gotoptions;

    IPV6CPDEBUG(("ipv6cp: up"));

    /*
     * We must have a non-zero LL address for both ends of the link.
     */
    if (!ho->neg_ifaceid)
	ho->hisid = wo->hisid;

#if 0 /* UNUSED */
    if(!no_ifaceid_neg) {
#endif /* UNUSED */
	if (eui64_iszero(ho->hisid)) {
	    ppp_error("Could not determine remote LL address");
	    ipv6cp_close(f->pcb, "Could not determine remote LL address");
	    return;
	}
	if (eui64_iszero(go->ourid)) {
	    ppp_error("Could not determine local LL address");
	    ipv6cp_close(f->pcb, "Could not determine local LL address");
	    return;
	}
	if (eui64_equals(go->ourid, ho->hisid)) {
	    ppp_error("local and remote LL addresses are equal");
	    ipv6cp_close(f->pcb, "local and remote LL addresses are equal");
	    return;
	}
#if 0 /* UNUSED */
    }
#endif /* UNUSED */
#if 0 /* UNUSED */
    script_setenv("LLLOCAL", llv6_ntoa(go->ourid), 0);
    script_setenv("LLREMOTE", llv6_ntoa(ho->hisid), 0);
#endif /* UNUSED */

#ifdef IPV6CP_COMP
    /* set tcp compression */
    sif6comp(f->unit, ho->neg_vj);
#endif

#if DEMAND_SUPPORT
    /*
     * If we are doing dial-on-demand, the interface is already
     * configured, so we put out any saved-up packets, then set the
     * interface to pass IPv6 packets.
     */
    if (demand) {
	if (! eui64_equals(go->ourid, wo->ourid) || 
	    ! eui64_equals(ho->hisid, wo->hisid)) {
	    if (! eui64_equals(go->ourid, wo->ourid))
		warn("Local LL address changed to %s", 
		     llv6_ntoa(go->ourid));
	    if (! eui64_equals(ho->hisid, wo->hisid))
		warn("Remote LL address changed to %s", 
		     llv6_ntoa(ho->hisid));
	    ipv6cp_clear_addrs(f->pcb, go->ourid, ho->hisid);

	    /* Set the interface to the new addresses */
	    if (!sif6addr(f->pcb, go->ourid, ho->hisid)) {
		if (debug)
		    warn("sif6addr failed");
		ipv6cp_close(f->unit, "Interface configuration failed");
		return;
	    }

	}
	demand_rexmit(PPP_IPV6);
	sifnpmode(f->unit, PPP_IPV6, NPMODE_PASS);

    } else
#endif /* DEMAND_SUPPORT */
    {
	/*
	 * Set LL addresses
	 */
	if (!sif6addr(f->pcb, go->ourid, ho->hisid)) {
	    PPPDEBUG(LOG_DEBUG, ("sif6addr failed"));
	    ipv6cp_close(f->pcb, "Interface configuration failed");
	    return;
	}

	/* bring the interface up for IPv6 */
	if (!sif6up(f->pcb)) {
	    PPPDEBUG(LOG_DEBUG, ("sif6up failed (IPV6)"));
	    ipv6cp_close(f->pcb, "Interface configuration failed");
	    return;
	}
#if DEMAND_SUPPORT
	sifnpmode(f->pcb, PPP_IPV6, NPMODE_PASS);
#endif /* DEMAND_SUPPORT */

	ppp_notice("local  LL address %s", llv6_ntoa(go->ourid));
	ppp_notice("remote LL address %s", llv6_ntoa(ho->hisid));
    }

    np_up(f->pcb, PPP_IPV6);
    pcb->ipv6cp_is_up = 1;

#if 0 /* UNUSED */
    /*
     * Execute the ipv6-up script, like this:
     *	/etc/ppp/ipv6-up interface tty speed local-LL remote-LL
     */
    if (ipv6cp_script_state == s_down && ipv6cp_script_pid == 0) {
	ipv6cp_script_state = s_up;
	ipv6cp_script(_PATH_IPV6UP);
    }
#endif /* UNUSED */
}


/*
 * ipv6cp_down - IPV6CP has gone DOWN.
 *
 * Take the IPv6 network interface down, clear its addresses
 * and delete routes through it.
 */
static void ipv6cp_down(fsm *f) {
    ppp_pcb *pcb = f->pcb;
    ipv6cp_options *go = &pcb->ipv6cp_gotoptions;
    ipv6cp_options *ho = &pcb->ipv6cp_hisoptions;

    IPV6CPDEBUG(("ipv6cp: down"));
#if PPP_STATS_SUPPORT
    update_link_stats(f->unit);
#endif /* PPP_STATS_SUPPORT */
    if (pcb->ipv6cp_is_up) {
	pcb->ipv6cp_is_up = 0;
	np_down(f->pcb, PPP_IPV6);
    }
#ifdef IPV6CP_COMP
    sif6comp(f->unit, 0);
#endif

#if DEMAND_SUPPORT
    /*
     * If we are doing dial-on-demand, set the interface
     * to queue up outgoing packets (for now).
     */
    if (demand) {
	sifnpmode(f->pcb, PPP_IPV6, NPMODE_QUEUE);
    } else
#endif /* DEMAND_SUPPORT */
    {
#if DEMAND_SUPPORT
	sifnpmode(f->pcb, PPP_IPV6, NPMODE_DROP);
#endif /* DEMAND_SUPPORT */
	ipv6cp_clear_addrs(f->pcb,
			   go->ourid,
			   ho->hisid);
	sif6down(f->pcb);
    }

#if 0 /* UNUSED */
    /* Execute the ipv6-down script */
    if (ipv6cp_script_state == s_up && ipv6cp_script_pid == 0) {
	ipv6cp_script_state = s_down;
	ipv6cp_script(_PATH_IPV6DOWN);
    }
#endif /* UNUSED */
}


/*
 * ipv6cp_clear_addrs() - clear the interface addresses, routes,
 * proxy neighbour discovery entries, etc.
 */
static void ipv6cp_clear_addrs(ppp_pcb *pcb, eui64_t ourid, eui64_t hisid) {
    cif6addr(pcb, ourid, hisid);
}


/*
 * ipv6cp_finished - possibly shut down the lower layers.
 */
static void ipv6cp_finished(fsm *f) {
    np_finished(f->pcb, PPP_IPV6);
}


#if 0 /* UNUSED */
/*
 * ipv6cp_script_done - called when the ipv6-up or ipv6-down script
 * has finished.
 */
static void
ipv6cp_script_done(arg)
    void *arg;
{
    ipv6cp_script_pid = 0;
    switch (ipv6cp_script_state) {
    case s_up:
	if (ipv6cp_fsm[0].state != PPP_FSM_OPENED) {
	    ipv6cp_script_state = s_down;
	    ipv6cp_script(_PATH_IPV6DOWN);
	}
	break;
    case s_down:
	if (ipv6cp_fsm[0].state == PPP_FSM_OPENED) {
	    ipv6cp_script_state = s_up;
	    ipv6cp_script(_PATH_IPV6UP);
	}
	break;
    }
}


/*
 * ipv6cp_script - Execute a script with arguments
 * interface-name tty-name speed local-LL remote-LL.
 */
static void
ipv6cp_script(script)
    char *script;
{
    char strspeed[32], strlocal[32], strremote[32];
    char *argv[8];

    sprintf(strspeed, "%d", baud_rate);
    strcpy(strlocal, llv6_ntoa(ipv6cp_gotoptions[0].ourid));
    strcpy(strremote, llv6_ntoa(ipv6cp_hisoptions[0].hisid));

    argv[0] = script;
    argv[1] = ifname;
    argv[2] = devnam;
    argv[3] = strspeed;
    argv[4] = strlocal;
    argv[5] = strremote;
    argv[6] = ipparam;
    argv[7] = NULL;

    ipv6cp_script_pid = run_program(script, argv, 0, ipv6cp_script_done,
				    NULL, 0);
}
#endif /* UNUSED */

#if PRINTPKT_SUPPORT
/*
 * ipv6cp_printpkt - print the contents of an IPV6CP packet.
 */
static const char* const ipv6cp_codenames[] = {
    "ConfReq", "ConfAck", "ConfNak", "ConfRej",
    "TermReq", "TermAck", "CodeRej"
};

static int ipv6cp_printpkt(const u_char *p, int plen,
		void (*printer)(void *, const char *, ...), void *arg) {
    int code, id, len, olen;
    const u_char *pstart, *optend;
#ifdef IPV6CP_COMP
    u_short cishort;
#endif /* IPV6CP_COMP */
    eui64_t ifaceid;

    if (plen < HEADERLEN)
	return 0;
    pstart = p;
    GETCHAR(code, p);
    GETCHAR(id, p);
    GETSHORT(len, p);
    if (len < HEADERLEN || len > plen)
	return 0;

    if (code >= 1 && code <= (int)LWIP_ARRAYSIZE(ipv6cp_codenames))
	printer(arg, " %s", ipv6cp_codenames[code-1]);
    else
	printer(arg, " code=0x%x", code);
    printer(arg, " id=0x%x", id);
    len -= HEADERLEN;
    switch (code) {
    case CONFREQ:
    case CONFACK:
    case CONFNAK:
    case CONFREJ:
	/* print option list */
	while (len >= 2) {
	    GETCHAR(code, p);
	    GETCHAR(olen, p);
	    p -= 2;
	    if (olen < 2 || olen > len) {
		break;
	    }
	    printer(arg, " <");
	    len -= olen;
	    optend = p + olen;
	    switch (code) {
#ifdef IPV6CP_COMP
	    case CI_COMPRESSTYPE:
		if (olen >= CILEN_COMPRESS) {
		    p += 2;
		    GETSHORT(cishort, p);
		    printer(arg, "compress ");
		    printer(arg, "0x%x", cishort);
		}
		break;
#endif /* IPV6CP_COMP */
	    case CI_IFACEID:
		if (olen == CILEN_IFACEID) {
		    p += 2;
		    eui64_get(ifaceid, p);
		    printer(arg, "addr %s", llv6_ntoa(ifaceid));
		}
		break;
	    default:
		break;
	    }
	    while (p < optend) {
		GETCHAR(code, p);
		printer(arg, " %.2x", code);
	    }
	    printer(arg, ">");
	}
	break;

    case TERMACK:
    case TERMREQ:
	if (len > 0 && *p >= ' ' && *p < 0x7f) {
	    printer(arg, " ");
	    ppp_print_string(p, len, printer, arg);
	    p += len;
	    len = 0;
	}
	break;
    default:
	break;
    }

    /* print the rest of the bytes in the packet */
    for (; len > 0; --len) {
	GETCHAR(code, p);
	printer(arg, " %.2x", code);
    }

    return p - pstart;
}
#endif /* PRINTPKT_SUPPORT */

#if DEMAND_SUPPORT
/*
 * ipv6_active_pkt - see if this IP packet is worth bringing the link up for.
 * We don't bring the link up for IP fragments or for TCP FIN packets
 * with no data.
 */
#define IP6_HDRLEN	40	/* bytes */
#define IP6_NHDR_FRAG	44	/* fragment IPv6 header */
#define TCP_HDRLEN	20
#define TH_FIN		0x01

/*
 * We use these macros because the IP header may be at an odd address,
 * and some compilers might use word loads to get th_off or ip_hl.
 */

#define get_ip6nh(x)	(((unsigned char *)(x))[6])
#define get_tcpoff(x)	(((unsigned char *)(x))[12] >> 4)
#define get_tcpflags(x)	(((unsigned char *)(x))[13])

static int ipv6_active_pkt(u_char *pkt, int len) {
    u_char *tcp;

    len -= PPP_HDRLEN;
    pkt += PPP_HDRLEN;
    if (len < IP6_HDRLEN)
	return 0;
    if (get_ip6nh(pkt) == IP6_NHDR_FRAG)
	return 0;
    if (get_ip6nh(pkt) != IPPROTO_TCP)
	return 1;
    if (len < IP6_HDRLEN + TCP_HDRLEN)
	return 0;
    tcp = pkt + IP6_HDRLEN;
    if ((get_tcpflags(tcp) & TH_FIN) != 0 && len == IP6_HDRLEN + get_tcpoff(tcp) * 4)
	return 0;
    return 1;
}
#endif /* DEMAND_SUPPORT */

#endif /* PPP_SUPPORT && PPP_IPV6_SUPPORT */

/*
 * ccp.c - PPP Compression Control Protocol.
 *
 * Copyright (c) 1994-2002 Paul Mackerras. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. The name(s) of the authors of this software must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission.
 *
 * 3. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Paul Mackerras
 *     <paulus@samba.org>".
 *
 * THE AUTHORS OF THIS SOFTWARE DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "netif/ppp/ppp_opts.h"
#if PPP_SUPPORT && CCP_SUPPORT  /* don't build if not configured for use in lwipopts.h */

#include <stdlib.h>
#include <string.h>

#include "netif/ppp/ppp_impl.h"

#include "netif/ppp/fsm.h"
#include "netif/ppp/ccp.h"

#if MPPE_SUPPORT
#include "netif/ppp/lcp.h"	/* lcp_close(), lcp_fsm */
#include "netif/ppp/mppe.h"	/* mppe_init() */
#endif /* MPPE_SUPPORT */

/*
 * Unfortunately there is a bug in zlib which means that using a
 * size of 8 (window size = 256) for Deflate compression will cause
 * buffer overruns and kernel crashes in the deflate module.
 * Until this is fixed we only accept sizes in the range 9 .. 15.
 * Thanks to James Carlson for pointing this out.
 */
#define DEFLATE_MIN_WORKS	9

/*
 * Command-line options.
 */
#if PPP_OPTIONS
static int setbsdcomp (char **);
static int setdeflate (char **);
static char bsd_value[8];
static char deflate_value[8];

/*
 * Option variables.
 */
#if MPPE_SUPPORT
bool refuse_mppe_stateful = 1;		/* Allow stateful mode? */
#endif /* MPPE_SUPPORT */

static option_t ccp_option_list[] = {
    { "noccp", o_bool, &ccp_protent.enabled_flag,
      "Disable CCP negotiation" },
    { "-ccp", o_bool, &ccp_protent.enabled_flag,
      "Disable CCP negotiation", OPT_ALIAS },

    { "bsdcomp", o_special, (void *)setbsdcomp,
      "Request BSD-Compress packet compression",
      OPT_PRIO | OPT_A2STRVAL | OPT_STATIC, bsd_value },
    { "nobsdcomp", o_bool, &ccp_wantoptions[0].bsd_compress,
      "don't allow BSD-Compress", OPT_PRIOSUB | OPT_A2CLR,
      &ccp_allowoptions[0].bsd_compress },
    { "-bsdcomp", o_bool, &ccp_wantoptions[0].bsd_compress,
      "don't allow BSD-Compress", OPT_ALIAS | OPT_PRIOSUB | OPT_A2CLR,
      &ccp_allowoptions[0].bsd_compress },

    { "deflate", o_special, (void *)setdeflate,
      "request Deflate compression",
      OPT_PRIO | OPT_A2STRVAL | OPT_STATIC, deflate_value },
    { "nodeflate", o_bool, &ccp_wantoptions[0].deflate,
      "don't allow Deflate compression", OPT_PRIOSUB | OPT_A2CLR,
      &ccp_allowoptions[0].deflate },
    { "-deflate", o_bool, &ccp_wantoptions[0].deflate,
      "don't allow Deflate compression", OPT_ALIAS | OPT_PRIOSUB | OPT_A2CLR,
      &ccp_allowoptions[0].deflate },

    { "nodeflatedraft", o_bool, &ccp_wantoptions[0].deflate_draft,
      "don't use draft deflate #", OPT_A2COPY,
      &ccp_allowoptions[0].deflate_draft },

    { "predictor1", o_bool, &ccp_wantoptions[0].predictor_1,
      "request Predictor-1", OPT_PRIO | 1 },
    { "nopredictor1", o_bool, &ccp_wantoptions[0].predictor_1,
      "don't allow Predictor-1", OPT_PRIOSUB | OPT_A2CLR,
      &ccp_allowoptions[0].predictor_1 },
    { "-predictor1", o_bool, &ccp_wantoptions[0].predictor_1,
      "don't allow Predictor-1", OPT_ALIAS | OPT_PRIOSUB | OPT_A2CLR,
      &ccp_allowoptions[0].predictor_1 },

#if MPPE_SUPPORT
    /* MPPE options are symmetrical ... we only set wantoptions here */
    { "require-mppe", o_bool, &ccp_wantoptions[0].mppe,
      "require MPPE encryption",
      OPT_PRIO | MPPE_OPT_40 | MPPE_OPT_128 },
    { "+mppe", o_bool, &ccp_wantoptions[0].mppe,
      "require MPPE encryption",
      OPT_ALIAS | OPT_PRIO | MPPE_OPT_40 | MPPE_OPT_128 },
    { "nomppe", o_bool, &ccp_wantoptions[0].mppe,
      "don't allow MPPE encryption", OPT_PRIO },
    { "-mppe", o_bool, &ccp_wantoptions[0].mppe,
      "don't allow MPPE encryption", OPT_ALIAS | OPT_PRIO },

    /* We use ccp_allowoptions[0].mppe as a junk var ... it is reset later */
    { "require-mppe-40", o_bool, &ccp_allowoptions[0].mppe,
      "require MPPE 40-bit encryption", OPT_PRIO | OPT_A2OR | MPPE_OPT_40,
      &ccp_wantoptions[0].mppe },
    { "+mppe-40", o_bool, &ccp_allowoptions[0].mppe,
      "require MPPE 40-bit encryption", OPT_PRIO | OPT_A2OR | MPPE_OPT_40,
      &ccp_wantoptions[0].mppe },
    { "nomppe-40", o_bool, &ccp_allowoptions[0].mppe,
      "don't allow MPPE 40-bit encryption",
      OPT_PRIOSUB | OPT_A2CLRB | MPPE_OPT_40, &ccp_wantoptions[0].mppe },
    { "-mppe-40", o_bool, &ccp_allowoptions[0].mppe,
      "don't allow MPPE 40-bit encryption",
      OPT_ALIAS | OPT_PRIOSUB | OPT_A2CLRB | MPPE_OPT_40,
      &ccp_wantoptions[0].mppe },

    { "require-mppe-128", o_bool, &ccp_allowoptions[0].mppe,
      "require MPPE 128-bit encryption", OPT_PRIO | OPT_A2OR | MPPE_OPT_128,
      &ccp_wantoptions[0].mppe },
    { "+mppe-128", o_bool, &ccp_allowoptions[0].mppe,
      "require MPPE 128-bit encryption",
      OPT_ALIAS | OPT_PRIO | OPT_A2OR | MPPE_OPT_128,
      &ccp_wantoptions[0].mppe },
    { "nomppe-128", o_bool, &ccp_allowoptions[0].mppe,
      "don't allow MPPE 128-bit encryption",
      OPT_PRIOSUB | OPT_A2CLRB | MPPE_OPT_128, &ccp_wantoptions[0].mppe },
    { "-mppe-128", o_bool, &ccp_allowoptions[0].mppe,
      "don't allow MPPE 128-bit encryption",
      OPT_ALIAS | OPT_PRIOSUB | OPT_A2CLRB | MPPE_OPT_128,
      &ccp_wantoptions[0].mppe },

    /* strange one; we always request stateless, but will we allow stateful? */
    { "mppe-stateful", o_bool, &refuse_mppe_stateful,
      "allow MPPE stateful mode", OPT_PRIO },
    { "nomppe-stateful", o_bool, &refuse_mppe_stateful,
      "disallow MPPE stateful mode", OPT_PRIO | 1 },
#endif /* MPPE_SUPPORT */

    { NULL }
};
#endif /* PPP_OPTIONS */

/*
 * Protocol entry points from main code.
 */
static void ccp_init(ppp_pcb *pcb);
static void ccp_open(ppp_pcb *pcb);
static void ccp_close(ppp_pcb *pcb, const char *reason);
static void ccp_lowerup(ppp_pcb *pcb);
static void ccp_lowerdown(ppp_pcb *pcb);
static void ccp_input(ppp_pcb *pcb, u_char *pkt, int len);
static void ccp_protrej(ppp_pcb *pcb);
#if PRINTPKT_SUPPORT
static int ccp_printpkt(const u_char *p, int plen, void (*printer) (void *, const char *, ...), void *arg);
#endif /* PRINTPKT_SUPPORT */
#if PPP_DATAINPUT
static void ccp_datainput(ppp_pcb *pcb, u_char *pkt, int len);
#endif /* PPP_DATAINPUT */

const struct protent ccp_protent = {
    PPP_CCP,
    ccp_init,
    ccp_input,
    ccp_protrej,
    ccp_lowerup,
    ccp_lowerdown,
    ccp_open,
    ccp_close,
#if PRINTPKT_SUPPORT
    ccp_printpkt,
#endif /* PRINTPKT_SUPPORT */
#if PPP_DATAINPUT
    ccp_datainput,
#endif /* PPP_DATAINPUT */
#if PRINTPKT_SUPPORT
    "CCP",
    "Compressed",
#endif /* PRINTPKT_SUPPORT */
#if PPP_OPTIONS
    ccp_option_list,
    NULL,
#endif /* PPP_OPTIONS */
#if DEMAND_SUPPORT
    NULL,
    NULL
#endif /* DEMAND_SUPPORT */
};

/*
 * Callbacks for fsm code.
 */
static void ccp_resetci (fsm *);
static int  ccp_cilen (fsm *);
static void ccp_addci (fsm *, u_char *, int *);
static int  ccp_ackci (fsm *, u_char *, int);
static int  ccp_nakci (fsm *, u_char *, int, int);
static int  ccp_rejci (fsm *, u_char *, int);
static int  ccp_reqci (fsm *, u_char *, int *, int);
static void ccp_up (fsm *);
static void ccp_down (fsm *);
static int  ccp_extcode (fsm *, int, int, u_char *, int);
static void ccp_rack_timeout (void *);
static const char *method_name (ccp_options *, ccp_options *);

static const fsm_callbacks ccp_callbacks = {
    ccp_resetci,
    ccp_cilen,
    ccp_addci,
    ccp_ackci,
    ccp_nakci,
    ccp_rejci,
    ccp_reqci,
    ccp_up,
    ccp_down,
    NULL,
    NULL,
    NULL,
    NULL,
    ccp_extcode,
    "CCP"
};

/*
 * Do we want / did we get any compression?
 */
static int ccp_anycompress(ccp_options *opt) {
    return (0
#if DEFLATE_SUPPORT
	|| (opt)->deflate
#endif /* DEFLATE_SUPPORT */
#if BSDCOMPRESS_SUPPORT
	|| (opt)->bsd_compress
#endif /* BSDCOMPRESS_SUPPORT */
#if PREDICTOR_SUPPORT
	|| (opt)->predictor_1 || (opt)->predictor_2
#endif /* PREDICTOR_SUPPORT */
#if MPPE_SUPPORT
	|| (opt)->mppe
#endif /* MPPE_SUPPORT */
	);
}

/*
 * Local state (mainly for handling reset-reqs and reset-acks).
 */
#define RACK_PENDING	1	/* waiting for reset-ack */
#define RREQ_REPEAT	2	/* send another reset-req if no reset-ack */

#define RACKTIMEOUT	1	/* second */

#if PPP_OPTIONS
/*
 * Option parsing
 */
static int
setbsdcomp(argv)
    char **argv;
{
    int rbits, abits;
    char *str, *endp;

    str = *argv;
    abits = rbits = strtol(str, &endp, 0);
    if (endp != str && *endp == ',') {
	str = endp + 1;
	abits = strtol(str, &endp, 0);
    }
    if (*endp != 0 || endp == str) {
	option_error("invalid parameter '%s' for bsdcomp option", *argv);
	return 0;
    }
    if ((rbits != 0 && (rbits < BSD_MIN_BITS || rbits > BSD_MAX_BITS))
	|| (abits != 0 && (abits < BSD_MIN_BITS || abits > BSD_MAX_BITS))) {
	option_error("bsdcomp option values must be 0 or %d .. %d",
		     BSD_MIN_BITS, BSD_MAX_BITS);
	return 0;
    }
    if (rbits > 0) {
	ccp_wantoptions[0].bsd_compress = 1;
	ccp_wantoptions[0].bsd_bits = rbits;
    } else
	ccp_wantoptions[0].bsd_compress = 0;
    if (abits > 0) {
	ccp_allowoptions[0].bsd_compress = 1;
	ccp_allowoptions[0].bsd_bits = abits;
    } else
	ccp_allowoptions[0].bsd_compress = 0;
    ppp_slprintf(bsd_value, sizeof(bsd_value),
	     rbits == abits? "%d": "%d,%d", rbits, abits);

    return 1;
}

static int
setdeflate(argv)
    char **argv;
{
    int rbits, abits;
    char *str, *endp;

    str = *argv;
    abits = rbits = strtol(str, &endp, 0);
    if (endp != str && *endp == ',') {
	str = endp + 1;
	abits = strtol(str, &endp, 0);
    }
    if (*endp != 0 || endp == str) {
	option_error("invalid parameter '%s' for deflate option", *argv);
	return 0;
    }
    if ((rbits != 0 && (rbits < DEFLATE_MIN_SIZE || rbits > DEFLATE_MAX_SIZE))
	|| (abits != 0 && (abits < DEFLATE_MIN_SIZE
			  || abits > DEFLATE_MAX_SIZE))) {
	option_error("deflate option values must be 0 or %d .. %d",
		     DEFLATE_MIN_SIZE, DEFLATE_MAX_SIZE);
	return 0;
    }
    if (rbits == DEFLATE_MIN_SIZE || abits == DEFLATE_MIN_SIZE) {
	if (rbits == DEFLATE_MIN_SIZE)
	    rbits = DEFLATE_MIN_WORKS;
	if (abits == DEFLATE_MIN_SIZE)
	    abits = DEFLATE_MIN_WORKS;
	warn("deflate option value of %d changed to %d to avoid zlib bug",
	     DEFLATE_MIN_SIZE, DEFLATE_MIN_WORKS);
    }
    if (rbits > 0) {
	ccp_wantoptions[0].deflate = 1;
	ccp_wantoptions[0].deflate_size = rbits;
    } else
	ccp_wantoptions[0].deflate = 0;
    if (abits > 0) {
	ccp_allowoptions[0].deflate = 1;
	ccp_allowoptions[0].deflate_size = abits;
    } else
	ccp_allowoptions[0].deflate = 0;
    ppp_slprintf(deflate_value, sizeof(deflate_value),
	     rbits == abits? "%d": "%d,%d", rbits, abits);

    return 1;
}
#endif /* PPP_OPTIONS */

/*
 * ccp_init - initialize CCP.
 */
static void ccp_init(ppp_pcb *pcb) {
    fsm *f = &pcb->ccp_fsm;

    f->pcb = pcb;
    f->protocol = PPP_CCP;
    f->callbacks = &ccp_callbacks;
    fsm_init(f);

#if 0 /* Not necessary, everything is cleared in ppp_new() */
    memset(wo, 0, sizeof(*wo));
    memset(go, 0, sizeof(*go));
    memset(ao, 0, sizeof(*ao));
    memset(ho, 0, sizeof(*ho));
#endif /* 0 */

#if DEFLATE_SUPPORT
    wo->deflate = 1;
    wo->deflate_size = DEFLATE_MAX_SIZE;
    wo->deflate_correct = 1;
    wo->deflate_draft = 1;
    ao->deflate = 1;
    ao->deflate_size = DEFLATE_MAX_SIZE;
    ao->deflate_correct = 1;
    ao->deflate_draft = 1;
#endif /* DEFLATE_SUPPORT */

#if BSDCOMPRESS_SUPPORT
    wo->bsd_compress = 1;
    wo->bsd_bits = BSD_MAX_BITS;
    ao->bsd_compress = 1;
    ao->bsd_bits = BSD_MAX_BITS;
#endif /* BSDCOMPRESS_SUPPORT */

#if PREDICTOR_SUPPORT
    ao->predictor_1 = 1;
#endif /* PREDICTOR_SUPPORT */
}

/*
 * ccp_open - CCP is allowed to come up.
 */
static void ccp_open(ppp_pcb *pcb) {
    fsm *f = &pcb->ccp_fsm;
    ccp_options *go = &pcb->ccp_gotoptions;

    if (f->state != PPP_FSM_OPENED)
	ccp_set(pcb, 1, 0, 0, 0);

    /*
     * Find out which compressors the kernel supports before
     * deciding whether to open in silent mode.
     */
    ccp_resetci(f);
    if (!ccp_anycompress(go))
	f->flags |= OPT_SILENT;

    fsm_open(f);
}

/*
 * ccp_close - Terminate CCP.
 */
static void ccp_close(ppp_pcb *pcb, const char *reason) {
    fsm *f = &pcb->ccp_fsm;
    ccp_set(pcb, 0, 0, 0, 0);
    fsm_close(f, reason);
}

/*
 * ccp_lowerup - we may now transmit CCP packets.
 */
static void ccp_lowerup(ppp_pcb *pcb) {
    fsm *f = &pcb->ccp_fsm;
    fsm_lowerup(f);
}

/*
 * ccp_lowerdown - we may not transmit CCP packets.
 */
static void ccp_lowerdown(ppp_pcb *pcb) {
    fsm *f = &pcb->ccp_fsm;
    fsm_lowerdown(f);
}

/*
 * ccp_input - process a received CCP packet.
 */
static void ccp_input(ppp_pcb *pcb, u_char *p, int len) {
    fsm *f = &pcb->ccp_fsm;
    ccp_options *go = &pcb->ccp_gotoptions;
    int oldstate;

    /*
     * Check for a terminate-request so we can print a message.
     */
    oldstate = f->state;
    fsm_input(f, p, len);
    if (oldstate == PPP_FSM_OPENED && p[0] == TERMREQ && f->state != PPP_FSM_OPENED) {
	ppp_notice("Compression disabled by peer.");
#if MPPE_SUPPORT
	if (go->mppe) {
	    ppp_error("MPPE disabled, closing LCP");
	    lcp_close(pcb, "MPPE disabled by peer");
	}
#endif /* MPPE_SUPPORT */
    }

    /*
     * If we get a terminate-ack and we're not asking for compression,
     * close CCP.
     */
    if (oldstate == PPP_FSM_REQSENT && p[0] == TERMACK
	&& !ccp_anycompress(go))
	ccp_close(pcb, "No compression negotiated");
}

/*
 * Handle a CCP-specific code.
 */
static int ccp_extcode(fsm *f, int code, int id, u_char *p, int len) {
    ppp_pcb *pcb = f->pcb;
    LWIP_UNUSED_ARG(p);
    LWIP_UNUSED_ARG(len);

    switch (code) {
    case CCP_RESETREQ:
	if (f->state != PPP_FSM_OPENED)
	    break;
	ccp_reset_comp(pcb);
	/* send a reset-ack, which the transmitter will see and
	   reset its compression state. */
	fsm_sdata(f, CCP_RESETACK, id, NULL, 0);
	break;

    case CCP_RESETACK:
	if ((pcb->ccp_localstate & RACK_PENDING) && id == f->reqid) {
	    pcb->ccp_localstate &= ~(RACK_PENDING | RREQ_REPEAT);
	    UNTIMEOUT(ccp_rack_timeout, f);
	    ccp_reset_decomp(pcb);
	}
	break;

    default:
	return 0;
    }

    return 1;
}

/*
 * ccp_protrej - peer doesn't talk CCP.
 */
static void ccp_protrej(ppp_pcb *pcb) {
    fsm *f = &pcb->ccp_fsm;
#if MPPE_SUPPORT
    ccp_options *go = &pcb->ccp_gotoptions;
#endif /* MPPE_SUPPORT */

    ccp_set(pcb, 0, 0, 0, 0);
    fsm_lowerdown(f);

#if MPPE_SUPPORT
    if (go->mppe) {
	ppp_error("MPPE required but peer negotiation failed");
	lcp_close(pcb, "MPPE required but peer negotiation failed");
    }
#endif /* MPPE_SUPPORT */

}

/*
 * ccp_resetci - initialize at start of negotiation.
 */
static void ccp_resetci(fsm *f) {
    ppp_pcb *pcb = f->pcb;
    ccp_options *go = &pcb->ccp_gotoptions;
    ccp_options *wo = &pcb->ccp_wantoptions;
#if MPPE_SUPPORT
    ccp_options *ao = &pcb->ccp_allowoptions;
#endif /* MPPE_SUPPORT */
#if DEFLATE_SUPPORT || BSDCOMPRESS_SUPPORT || PREDICTOR_SUPPORT
    u_char opt_buf[CCP_MAX_OPTION_LENGTH];
#endif /* DEFLATE_SUPPORT || BSDCOMPRESS_SUPPORT || PREDICTOR_SUPPORT */
#if DEFLATE_SUPPORT || BSDCOMPRESS_SUPPORT
    int res;
#endif /* DEFLATE_SUPPORT || BSDCOMPRESS_SUPPORT */

#if MPPE_SUPPORT
    if (pcb->settings.require_mppe) {
	wo->mppe = ao->mppe =
		    (pcb->settings.refuse_mppe_40 ? 0 : MPPE_OPT_40)
		  | (pcb->settings.refuse_mppe_128 ? 0 : MPPE_OPT_128);
    }
#endif /* MPPE_SUPPORT */

    *go = *wo;
    pcb->ccp_all_rejected = 0;

#if MPPE_SUPPORT
    if (go->mppe) {
	int auth_mschap_bits = pcb->auth_done;
	int numbits;

	/*
	 * Start with a basic sanity check: mschap[v2] auth must be in
	 * exactly one direction.  RFC 3079 says that the keys are
	 * 'derived from the credentials of the peer that initiated the call',
	 * however the PPP protocol doesn't have such a concept, and pppd
	 * cannot get this info externally.  Instead we do the best we can.
	 * NB: If MPPE is required, all other compression opts are invalid.
	 *     So, we return right away if we can't do it.
	 */

	/* Leave only the mschap auth bits set */
	auth_mschap_bits &= (CHAP_MS_WITHPEER  | CHAP_MS_PEER |
			     CHAP_MS2_WITHPEER | CHAP_MS2_PEER);
	/* Count the mschap auths */
	auth_mschap_bits >>= CHAP_MS_SHIFT;
	numbits = 0;
	do {
	    numbits += auth_mschap_bits & 1;
	    auth_mschap_bits >>= 1;
	} while (auth_mschap_bits);
	if (numbits > 1) {
	    ppp_error("MPPE required, but auth done in both directions.");
	    lcp_close(pcb, "MPPE required but not available");
	    return;
	}
	if (!numbits) {
	    ppp_error("MPPE required, but MS-CHAP[v2] auth not performed.");
	    lcp_close(pcb, "MPPE required but not available");
	    return;
	}

	/* A plugin (eg radius) may not have obtained key material. */
	if (!pcb->mppe_keys_set) {
	    ppp_error("MPPE required, but keys are not available.  "
		  "Possible plugin problem?");
	    lcp_close(pcb, "MPPE required but not available");
	    return;
	}

	/* LM auth not supported for MPPE */
	if (pcb->auth_done & (CHAP_MS_WITHPEER | CHAP_MS_PEER)) {
	    /* This might be noise */
	    if (go->mppe & MPPE_OPT_40) {
		ppp_notice("Disabling 40-bit MPPE; MS-CHAP LM not supported");
		go->mppe &= ~MPPE_OPT_40;
		wo->mppe &= ~MPPE_OPT_40;
	    }
	}

	/* Last check: can we actually negotiate something? */
	if (!(go->mppe & (MPPE_OPT_40 | MPPE_OPT_128))) {
	    /* Could be misconfig, could be 40-bit disabled above. */
	    ppp_error("MPPE required, but both 40-bit and 128-bit disabled.");
	    lcp_close(pcb, "MPPE required but not available");
	    return;
	}

	/* sync options */
	ao->mppe = go->mppe;
	/* MPPE is not compatible with other compression types */
#if BSDCOMPRESS_SUPPORT
	ao->bsd_compress = go->bsd_compress = 0;
#endif /* BSDCOMPRESS_SUPPORT */
#if PREDICTOR_SUPPORT
	ao->predictor_1  = go->predictor_1  = 0;
	ao->predictor_2  = go->predictor_2  = 0;
#endif /* PREDICTOR_SUPPORT */
#if DEFLATE_SUPPORT
	ao->deflate      = go->deflate      = 0;
#endif /* DEFLATE_SUPPORT */
    }
#endif /* MPPE_SUPPORT */

    /*
     * Check whether the kernel knows about the various
     * compression methods we might request.
     */
#if BSDCOMPRESS_SUPPORT
    /* FIXME: we don't need to test if BSD compress is available
     * if BSDCOMPRESS_SUPPORT is set, it is.
     */
    if (go->bsd_compress) {
	opt_buf[0] = CI_BSD_COMPRESS;
	opt_buf[1] = CILEN_BSD_COMPRESS;
	for (;;) {
	    if (go->bsd_bits < BSD_MIN_BITS) {
		go->bsd_compress = 0;
		break;
	    }
	    opt_buf[2] = BSD_MAKE_OPT(BSD_CURRENT_VERSION, go->bsd_bits);
	    res = ccp_test(pcb, opt_buf, CILEN_BSD_COMPRESS, 0);
	    if (res > 0) {
		break;
	    } else if (res < 0) {
		go->bsd_compress = 0;
		break;
	    }
	    go->bsd_bits--;
	}
    }
#endif /* BSDCOMPRESS_SUPPORT */
#if DEFLATE_SUPPORT
    /* FIXME: we don't need to test if deflate is available
     * if DEFLATE_SUPPORT is set, it is.
     */
    if (go->deflate) {
	if (go->deflate_correct) {
	    opt_buf[0] = CI_DEFLATE;
	    opt_buf[1] = CILEN_DEFLATE;
	    opt_buf[3] = DEFLATE_CHK_SEQUENCE;
	    for (;;) {
		if (go->deflate_size < DEFLATE_MIN_WORKS) {
		    go->deflate_correct = 0;
		    break;
		}
		opt_buf[2] = DEFLATE_MAKE_OPT(go->deflate_size);
		res = ccp_test(pcb, opt_buf, CILEN_DEFLATE, 0);
		if (res > 0) {
		    break;
		} else if (res < 0) {
		    go->deflate_correct = 0;
		    break;
		}
		go->deflate_size--;
	    }
	}
	if (go->deflate_draft) {
	    opt_buf[0] = CI_DEFLATE_DRAFT;
	    opt_buf[1] = CILEN_DEFLATE;
	    opt_buf[3] = DEFLATE_CHK_SEQUENCE;
	    for (;;) {
		if (go->deflate_size < DEFLATE_MIN_WORKS) {
		    go->deflate_draft = 0;
		    break;
		}
		opt_buf[2] = DEFLATE_MAKE_OPT(go->deflate_size);
		res = ccp_test(pcb, opt_buf, CILEN_DEFLATE, 0);
		if (res > 0) {
		    break;
		} else if (res < 0) {
		    go->deflate_draft = 0;
		    break;
		}
		go->deflate_size--;
	    }
	}
	if (!go->deflate_correct && !go->deflate_draft)
	    go->deflate = 0;
    }
#endif /* DEFLATE_SUPPORT */
#if PREDICTOR_SUPPORT
    /* FIXME: we don't need to test if predictor is available,
     * if PREDICTOR_SUPPORT is set, it is.
     */
    if (go->predictor_1) {
	opt_buf[0] = CI_PREDICTOR_1;
	opt_buf[1] = CILEN_PREDICTOR_1;
	if (ccp_test(pcb, opt_buf, CILEN_PREDICTOR_1, 0) <= 0)
	    go->predictor_1 = 0;
    }
    if (go->predictor_2) {
	opt_buf[0] = CI_PREDICTOR_2;
	opt_buf[1] = CILEN_PREDICTOR_2;
	if (ccp_test(pcb, opt_buf, CILEN_PREDICTOR_2, 0) <= 0)
	    go->predictor_2 = 0;
    }
#endif /* PREDICTOR_SUPPORT */
}

/*
 * ccp_cilen - Return total length of our configuration info.
 */
static int ccp_cilen(fsm *f) {
    ppp_pcb *pcb = f->pcb;
    ccp_options *go = &pcb->ccp_gotoptions;

    return 0
#if BSDCOMPRESS_SUPPORT
	+ (go->bsd_compress? CILEN_BSD_COMPRESS: 0)
#endif /* BSDCOMPRESS_SUPPORT */
#if DEFLATE_SUPPORT
	+ (go->deflate && go->deflate_correct? CILEN_DEFLATE: 0)
	+ (go->deflate && go->deflate_draft? CILEN_DEFLATE: 0)
#endif /* DEFLATE_SUPPORT */
#if PREDICTOR_SUPPORT
	+ (go->predictor_1? CILEN_PREDICTOR_1: 0)
	+ (go->predictor_2? CILEN_PREDICTOR_2: 0)
#endif /* PREDICTOR_SUPPORT */
#if MPPE_SUPPORT
	+ (go->mppe? CILEN_MPPE: 0)
#endif /* MPPE_SUPPORT */
	;
}

/*
 * ccp_addci - put our requests in a packet.
 */
static void ccp_addci(fsm *f, u_char *p, int *lenp) {
    ppp_pcb *pcb = f->pcb;
    ccp_options *go = &pcb->ccp_gotoptions;
    u_char *p0 = p;

    /*
     * Add the compression types that we can receive, in decreasing
     * preference order.
     */
#if MPPE_SUPPORT
    if (go->mppe) {
	p[0] = CI_MPPE;
	p[1] = CILEN_MPPE;
	MPPE_OPTS_TO_CI(go->mppe, &p[2]);
	mppe_init(pcb, &pcb->mppe_decomp, go->mppe);
	p += CILEN_MPPE;
    }
#endif /* MPPE_SUPPORT */
#if DEFLATE_SUPPORT
    if (go->deflate) {
	if (go->deflate_correct) {
	    p[0] = CI_DEFLATE;
	    p[1] = CILEN_DEFLATE;
	    p[2] = DEFLATE_MAKE_OPT(go->deflate_size);
	    p[3] = DEFLATE_CHK_SEQUENCE;
	    p += CILEN_DEFLATE;
	}
	if (go->deflate_draft) {
	    p[0] = CI_DEFLATE_DRAFT;
	    p[1] = CILEN_DEFLATE;
	    p[2] = p[2 - CILEN_DEFLATE];
	    p[3] = DEFLATE_CHK_SEQUENCE;
	    p += CILEN_DEFLATE;
	}
    }
#endif /* DEFLATE_SUPPORT */
#if BSDCOMPRESS_SUPPORT
    if (go->bsd_compress) {
	p[0] = CI_BSD_COMPRESS;
	p[1] = CILEN_BSD_COMPRESS;
	p[2] = BSD_MAKE_OPT(BSD_CURRENT_VERSION, go->bsd_bits);
	p += CILEN_BSD_COMPRESS;
    }
#endif /* BSDCOMPRESS_SUPPORT */
#if PREDICTOR_SUPPORT
    /* XXX Should Predictor 2 be preferable to Predictor 1? */
    if (go->predictor_1) {
	p[0] = CI_PREDICTOR_1;
	p[1] = CILEN_PREDICTOR_1;
	p += CILEN_PREDICTOR_1;
    }
    if (go->predictor_2) {
	p[0] = CI_PREDICTOR_2;
	p[1] = CILEN_PREDICTOR_2;
	p += CILEN_PREDICTOR_2;
    }
#endif /* PREDICTOR_SUPPORT */

    go->method = (p > p0)? p0[0]: 0;

    *lenp = p - p0;
}

/*
 * ccp_ackci - process a received configure-ack, and return
 * 1 iff the packet was OK.
 */
static int ccp_ackci(fsm *f, u_char *p, int len) {
    ppp_pcb *pcb = f->pcb;
    ccp_options *go = &pcb->ccp_gotoptions;
#if BSDCOMPRESS_SUPPORT || PREDICTOR_SUPPORT
    u_char *p0 = p;
#endif /* BSDCOMPRESS_SUPPORT || PREDICTOR_SUPPORT */

#if MPPE_SUPPORT
    if (go->mppe) {
	u_char opt_buf[CILEN_MPPE];

	opt_buf[0] = CI_MPPE;
	opt_buf[1] = CILEN_MPPE;
	MPPE_OPTS_TO_CI(go->mppe, &opt_buf[2]);
	if (len < CILEN_MPPE || memcmp(opt_buf, p, CILEN_MPPE))
	    return 0;
	p += CILEN_MPPE;
	len -= CILEN_MPPE;
	/* XXX Cope with first/fast ack */
	if (len == 0)
	    return 1;
    }
#endif /* MPPE_SUPPORT */
#if DEFLATE_SUPPORT
    if (go->deflate) {
	if (len < CILEN_DEFLATE
	    || p[0] != (go->deflate_correct? CI_DEFLATE: CI_DEFLATE_DRAFT)
	    || p[1] != CILEN_DEFLATE
	    || p[2] != DEFLATE_MAKE_OPT(go->deflate_size)
	    || p[3] != DEFLATE_CHK_SEQUENCE)
	    return 0;
	p += CILEN_DEFLATE;
	len -= CILEN_DEFLATE;
	/* XXX Cope with first/fast ack */
	if (len == 0)
	    return 1;
	if (go->deflate_correct && go->deflate_draft) {
	    if (len < CILEN_DEFLATE
		|| p[0] != CI_DEFLATE_DRAFT
		|| p[1] != CILEN_DEFLATE
		|| p[2] != DEFLATE_MAKE_OPT(go->deflate_size)
		|| p[3] != DEFLATE_CHK_SEQUENCE)
		return 0;
	    p += CILEN_DEFLATE;
	    len -= CILEN_DEFLATE;
	}
    }
#endif /* DEFLATE_SUPPORT */
#if BSDCOMPRESS_SUPPORT
    if (go->bsd_compress) {
	if (len < CILEN_BSD_COMPRESS
	    || p[0] != CI_BSD_COMPRESS || p[1] != CILEN_BSD_COMPRESS
	    || p[2] != BSD_MAKE_OPT(BSD_CURRENT_VERSION, go->bsd_bits))
	    return 0;
	p += CILEN_BSD_COMPRESS;
	len -= CILEN_BSD_COMPRESS;
	/* XXX Cope with first/fast ack */
	if (p == p0 && len == 0)
	    return 1;
    }
#endif /* BSDCOMPRESS_SUPPORT */
#if PREDICTOR_SUPPORT
    if (go->predictor_1) {
	if (len < CILEN_PREDICTOR_1
	    || p[0] != CI_PREDICTOR_1 || p[1] != CILEN_PREDICTOR_1)
	    return 0;
	p += CILEN_PREDICTOR_1;
	len -= CILEN_PREDICTOR_1;
	/* XXX Cope with first/fast ack */
	if (p == p0 && len == 0)
	    return 1;
    }
    if (go->predictor_2) {
	if (len < CILEN_PREDICTOR_2
	    || p[0] != CI_PREDICTOR_2 || p[1] != CILEN_PREDICTOR_2)
	    return 0;
	p += CILEN_PREDICTOR_2;
	len -= CILEN_PREDICTOR_2;
	/* XXX Cope with first/fast ack */
	if (p == p0 && len == 0)
	    return 1;
    }
#endif /* PREDICTOR_SUPPORT */

    if (len != 0)
	return 0;
    return 1;
}

/*
 * ccp_nakci - process received configure-nak.
 * Returns 1 iff the nak was OK.
 */
static int ccp_nakci(fsm *f, u_char *p, int len, int treat_as_reject) {
    ppp_pcb *pcb = f->pcb;
    ccp_options *go = &pcb->ccp_gotoptions;
    ccp_options no;		/* options we've seen already */
    ccp_options try_;		/* options to ask for next time */
    LWIP_UNUSED_ARG(treat_as_reject);
#if !MPPE_SUPPORT && !DEFLATE_SUPPORT && !BSDCOMPRESS_SUPPORT
    LWIP_UNUSED_ARG(p);
    LWIP_UNUSED_ARG(len);
#endif /* !MPPE_SUPPORT && !DEFLATE_SUPPORT && !BSDCOMPRESS_SUPPORT */

    memset(&no, 0, sizeof(no));
    try_ = *go;

#if MPPE_SUPPORT
    if (go->mppe && len >= CILEN_MPPE
	&& p[0] == CI_MPPE && p[1] == CILEN_MPPE) {
	no.mppe = 1;
	/*
	 * Peer wants us to use a different strength or other setting.
	 * Fail if we aren't willing to use his suggestion.
	 */
	MPPE_CI_TO_OPTS(&p[2], try_.mppe);
	if ((try_.mppe & MPPE_OPT_STATEFUL) && pcb->settings.refuse_mppe_stateful) {
	    ppp_error("Refusing MPPE stateful mode offered by peer");
	    try_.mppe = 0;
	} else if (((go->mppe | MPPE_OPT_STATEFUL) & try_.mppe) != try_.mppe) {
	    /* Peer must have set options we didn't request (suggest) */
	    try_.mppe = 0;
	}

	if (!try_.mppe) {
	    ppp_error("MPPE required but peer negotiation failed");
	    lcp_close(pcb, "MPPE required but peer negotiation failed");
	}
    }
#endif /* MPPE_SUPPORT */
#if DEFLATE_SUPPORT
    if (go->deflate && len >= CILEN_DEFLATE
	&& p[0] == (go->deflate_correct? CI_DEFLATE: CI_DEFLATE_DRAFT)
	&& p[1] == CILEN_DEFLATE) {
	no.deflate = 1;
	/*
	 * Peer wants us to use a different code size or something.
	 * Stop asking for Deflate if we don't understand his suggestion.
	 */
	if (DEFLATE_METHOD(p[2]) != DEFLATE_METHOD_VAL
	    || DEFLATE_SIZE(p[2]) < DEFLATE_MIN_WORKS
	    || p[3] != DEFLATE_CHK_SEQUENCE)
	    try_.deflate = 0;
	else if (DEFLATE_SIZE(p[2]) < go->deflate_size)
	    try_.deflate_size = DEFLATE_SIZE(p[2]);
	p += CILEN_DEFLATE;
	len -= CILEN_DEFLATE;
	if (go->deflate_correct && go->deflate_draft
	    && len >= CILEN_DEFLATE && p[0] == CI_DEFLATE_DRAFT
	    && p[1] == CILEN_DEFLATE) {
	    p += CILEN_DEFLATE;
	    len -= CILEN_DEFLATE;
	}
    }
#endif /* DEFLATE_SUPPORT */
#if BSDCOMPRESS_SUPPORT
    if (go->bsd_compress && len >= CILEN_BSD_COMPRESS
	&& p[0] == CI_BSD_COMPRESS && p[1] == CILEN_BSD_COMPRESS) {
	no.bsd_compress = 1;
	/*
	 * Peer wants us to use a different number of bits
	 * or a different version.
	 */
	if (BSD_VERSION(p[2]) != BSD_CURRENT_VERSION)
	    try_.bsd_compress = 0;
	else if (BSD_NBITS(p[2]) < go->bsd_bits)
	    try_.bsd_bits = BSD_NBITS(p[2]);
	p += CILEN_BSD_COMPRESS;
	len -= CILEN_BSD_COMPRESS;
    }
#endif /* BSDCOMPRESS_SUPPORT */

    /*
     * Predictor-1 and 2 have no options, so they can't be Naked.
     *
     * There may be remaining options but we ignore them.
     */

    if (f->state != PPP_FSM_OPENED)
	*go = try_;
    return 1;
}

/*
 * ccp_rejci - reject some of our suggested compression methods.
 */
static int ccp_rejci(fsm *f, u_char *p, int len) {
    ppp_pcb *pcb = f->pcb;
    ccp_options *go = &pcb->ccp_gotoptions;
    ccp_options try_;		/* options to request next time */

    try_ = *go;

    /*
     * Cope with empty configure-rejects by ceasing to send
     * configure-requests.
     */
    if (len == 0 && pcb->ccp_all_rejected)
	return -1;

#if MPPE_SUPPORT
    if (go->mppe && len >= CILEN_MPPE
	&& p[0] == CI_MPPE && p[1] == CILEN_MPPE) {
	ppp_error("MPPE required but peer refused");
	lcp_close(pcb, "MPPE required but peer refused");
	p += CILEN_MPPE;
	len -= CILEN_MPPE;
    }
#endif /* MPPE_SUPPORT */
#if DEFLATE_SUPPORT
    if (go->deflate_correct && len >= CILEN_DEFLATE
	&& p[0] == CI_DEFLATE && p[1] == CILEN_DEFLATE) {
	if (p[2] != DEFLATE_MAKE_OPT(go->deflate_size)
	    || p[3] != DEFLATE_CHK_SEQUENCE)
	    return 0;		/* Rej is bad */
	try_.deflate_correct = 0;
	p += CILEN_DEFLATE;
	len -= CILEN_DEFLATE;
    }
    if (go->deflate_draft && len >= CILEN_DEFLATE
	&& p[0] == CI_DEFLATE_DRAFT && p[1] == CILEN_DEFLATE) {
	if (p[2] != DEFLATE_MAKE_OPT(go->deflate_size)
	    || p[3] != DEFLATE_CHK_SEQUENCE)
	    return 0;		/* Rej is bad */
	try_.deflate_draft = 0;
	p += CILEN_DEFLATE;
	len -= CILEN_DEFLATE;
    }
    if (!try_.deflate_correct && !try_.deflate_draft)
	try_.deflate = 0;
#endif /* DEFLATE_SUPPORT */
#if BSDCOMPRESS_SUPPORT
    if (go->bsd_compress && len >= CILEN_BSD_COMPRESS
	&& p[0] == CI_BSD_COMPRESS && p[1] == CILEN_BSD_COMPRESS) {
	if (p[2] != BSD_MAKE_OPT(BSD_CURRENT_VERSION, go->bsd_bits))
	    return 0;
	try_.bsd_compress = 0;
	p += CILEN_BSD_COMPRESS;
	len -= CILEN_BSD_COMPRESS;
    }
#endif /* BSDCOMPRESS_SUPPORT */
#if PREDICTOR_SUPPORT
    if (go->predictor_1 && len >= CILEN_PREDICTOR_1
	&& p[0] == CI_PREDICTOR_1 && p[1] == CILEN_PREDICTOR_1) {
	try_.predictor_1 = 0;
	p += CILEN_PREDICTOR_1;
	len -= CILEN_PREDICTOR_1;
    }
    if (go->predictor_2 && len >= CILEN_PREDICTOR_2
	&& p[0] == CI_PREDICTOR_2 && p[1] == CILEN_PREDICTOR_2) {
	try_.predictor_2 = 0;
	p += CILEN_PREDICTOR_2;
	len -= CILEN_PREDICTOR_2;
    }
#endif /* PREDICTOR_SUPPORT */

    if (len != 0)
	return 0;

    if (f->state != PPP_FSM_OPENED)
	*go = try_;

    return 1;
}

/*
 * ccp_reqci - processed a received configure-request.
 * Returns CONFACK, CONFNAK or CONFREJ and the packet modified
 * appropriately.
 */
static int ccp_reqci(fsm *f, u_char *p, int *lenp, int dont_nak) {
    ppp_pcb *pcb = f->pcb;
    ccp_options *ho = &pcb->ccp_hisoptions;
    ccp_options *ao = &pcb->ccp_allowoptions;
    int ret, newret;
#if DEFLATE_SUPPORT || BSDCOMPRESS_SUPPORT
    int res;
    int nb;
#endif /* DEFLATE_SUPPORT || BSDCOMPRESS_SUPPORT */
    u_char *p0, *retp;
    int len, clen, type;
#if MPPE_SUPPORT
    u8_t rej_for_ci_mppe = 1;	/* Are we rejecting based on a bad/missing */
				/* CI_MPPE, or due to other options?       */
#endif /* MPPE_SUPPORT */

    ret = CONFACK;
    retp = p0 = p;
    len = *lenp;

    memset(ho, 0, sizeof(ccp_options));
    ho->method = (len > 0)? p[0]: 0;

    while (len > 0) {
	newret = CONFACK;
	if (len < 2 || p[1] < 2 || p[1] > len) {
	    /* length is bad */
	    clen = len;
	    newret = CONFREJ;

	} else {
	    type = p[0];
	    clen = p[1];

	    switch (type) {
#if MPPE_SUPPORT
	    case CI_MPPE:
		if (!ao->mppe || clen != CILEN_MPPE) {
		    newret = CONFREJ;
		    break;
		}
		MPPE_CI_TO_OPTS(&p[2], ho->mppe);

		/* Nak if anything unsupported or unknown are set. */
		if (ho->mppe & MPPE_OPT_UNSUPPORTED) {
		    newret = CONFNAK;
		    ho->mppe &= ~MPPE_OPT_UNSUPPORTED;
		}
		if (ho->mppe & MPPE_OPT_UNKNOWN) {
		    newret = CONFNAK;
		    ho->mppe &= ~MPPE_OPT_UNKNOWN;
		}

		/* Check state opt */
		if (ho->mppe & MPPE_OPT_STATEFUL) {
		    /*
		     * We can Nak and request stateless, but it's a
		     * lot easier to just assume the peer will request
		     * it if he can do it; stateful mode is bad over
		     * the Internet -- which is where we expect MPPE.
		     */
		   if (pcb->settings.refuse_mppe_stateful) {
			ppp_error("Refusing MPPE stateful mode offered by peer");
			newret = CONFREJ;
			break;
		    }
		}

		/* Find out which of {S,L} are set. */
		if ((ho->mppe & MPPE_OPT_128)
		     && (ho->mppe & MPPE_OPT_40)) {
		    /* Both are set, negotiate the strongest. */
		    newret = CONFNAK;
		    if (ao->mppe & MPPE_OPT_128)
			ho->mppe &= ~MPPE_OPT_40;
		    else if (ao->mppe & MPPE_OPT_40)
			ho->mppe &= ~MPPE_OPT_128;
		    else {
			newret = CONFREJ;
			break;
		    }
		} else if (ho->mppe & MPPE_OPT_128) {
		    if (!(ao->mppe & MPPE_OPT_128)) {
			newret = CONFREJ;
			break;
		    }
		} else if (ho->mppe & MPPE_OPT_40) {
		    if (!(ao->mppe & MPPE_OPT_40)) {
			newret = CONFREJ;
			break;
		    }
		} else {
		    /* Neither are set. */
		    /* We cannot accept this.  */
		    newret = CONFNAK;
		    /* Give the peer our idea of what can be used,
		       so it can choose and confirm */
		    ho->mppe = ao->mppe;
		}

		/* rebuild the opts */
		MPPE_OPTS_TO_CI(ho->mppe, &p[2]);
		if (newret == CONFACK) {
		    int mtu;

		    mppe_init(pcb, &pcb->mppe_comp, ho->mppe);
		    /*
		     * We need to decrease the interface MTU by MPPE_PAD
		     * because MPPE frames **grow**.  The kernel [must]
		     * allocate MPPE_PAD extra bytes in xmit buffers.
		     */
		    mtu = netif_get_mtu(pcb);
		    if (mtu)
			netif_set_mtu(pcb, mtu - MPPE_PAD);
		    else
			newret = CONFREJ;
		}

		/*
		 * We have accepted MPPE or are willing to negotiate
		 * MPPE parameters.  A CONFREJ is due to subsequent
		 * (non-MPPE) processing.
		 */
		rej_for_ci_mppe = 0;
		break;
#endif /* MPPE_SUPPORT */
#if DEFLATE_SUPPORT
	    case CI_DEFLATE:
	    case CI_DEFLATE_DRAFT:
		if (!ao->deflate || clen != CILEN_DEFLATE
		    || (!ao->deflate_correct && type == CI_DEFLATE)
		    || (!ao->deflate_draft && type == CI_DEFLATE_DRAFT)) {
		    newret = CONFREJ;
		    break;
		}

		ho->deflate = 1;
		ho->deflate_size = nb = DEFLATE_SIZE(p[2]);
		if (DEFLATE_METHOD(p[2]) != DEFLATE_METHOD_VAL
		    || p[3] != DEFLATE_CHK_SEQUENCE
		    || nb > ao->deflate_size || nb < DEFLATE_MIN_WORKS) {
		    newret = CONFNAK;
		    if (!dont_nak) {
			p[2] = DEFLATE_MAKE_OPT(ao->deflate_size);
			p[3] = DEFLATE_CHK_SEQUENCE;
			/* fall through to test this #bits below */
		    } else
			break;
		}

		/*
		 * Check whether we can do Deflate with the window
		 * size they want.  If the window is too big, reduce
		 * it until the kernel can cope and nak with that.
		 * We only check this for the first option.
		 */
		if (p == p0) {
		    for (;;) {
			res = ccp_test(pcb, p, CILEN_DEFLATE, 1);
			if (res > 0)
			    break;		/* it's OK now */
			if (res < 0 || nb == DEFLATE_MIN_WORKS || dont_nak) {
			    newret = CONFREJ;
			    p[2] = DEFLATE_MAKE_OPT(ho->deflate_size);
			    break;
			}
			newret = CONFNAK;
			--nb;
			p[2] = DEFLATE_MAKE_OPT(nb);
		    }
		}
		break;
#endif /* DEFLATE_SUPPORT */
#if BSDCOMPRESS_SUPPORT
	    case CI_BSD_COMPRESS:
		if (!ao->bsd_compress || clen != CILEN_BSD_COMPRESS) {
		    newret = CONFREJ;
		    break;
		}

		ho->bsd_compress = 1;
		ho->bsd_bits = nb = BSD_NBITS(p[2]);
		if (BSD_VERSION(p[2]) != BSD_CURRENT_VERSION
		    || nb > ao->bsd_bits || nb < BSD_MIN_BITS) {
		    newret = CONFNAK;
		    if (!dont_nak) {
			p[2] = BSD_MAKE_OPT(BSD_CURRENT_VERSION, ao->bsd_bits);
			/* fall through to test this #bits below */
		    } else
			break;
		}

		/*
		 * Check whether we can do BSD-Compress with the code
		 * size they want.  If the code size is too big, reduce
		 * it until the kernel can cope and nak with that.
		 * We only check this for the first option.
		 */
		if (p == p0) {
		    for (;;) {
			res = ccp_test(pcb, p, CILEN_BSD_COMPRESS, 1);
			if (res > 0)
			    break;
			if (res < 0 || nb == BSD_MIN_BITS || dont_nak) {
			    newret = CONFREJ;
			    p[2] = BSD_MAKE_OPT(BSD_CURRENT_VERSION,
						ho->bsd_bits);
			    break;
			}
			newret = CONFNAK;
			--nb;
			p[2] = BSD_MAKE_OPT(BSD_CURRENT_VERSION, nb);
		    }
		}
		break;
#endif /* BSDCOMPRESS_SUPPORT */
#if PREDICTOR_SUPPORT
	    case CI_PREDICTOR_1:
		if (!ao->predictor_1 || clen != CILEN_PREDICTOR_1) {
		    newret = CONFREJ;
		    break;
		}

		ho->predictor_1 = 1;
		if (p == p0
		    && ccp_test(pcb, p, CILEN_PREDICTOR_1, 1) <= 0) {
		    newret = CONFREJ;
		}
		break;

	    case CI_PREDICTOR_2:
		if (!ao->predictor_2 || clen != CILEN_PREDICTOR_2) {
		    newret = CONFREJ;
		    break;
		}

		ho->predictor_2 = 1;
		if (p == p0
		    && ccp_test(pcb, p, CILEN_PREDICTOR_2, 1) <= 0) {
		    newret = CONFREJ;
		}
		break;
#endif /* PREDICTOR_SUPPORT */

	    default:
		newret = CONFREJ;
	    }
	}

	if (newret == CONFNAK && dont_nak)
	    newret = CONFREJ;
	if (!(newret == CONFACK || (newret == CONFNAK && ret == CONFREJ))) {
	    /* we're returning this option */
	    if (newret == CONFREJ && ret == CONFNAK)
		retp = p0;
	    ret = newret;
	    if (p != retp)
		MEMCPY(retp, p, clen);
	    retp += clen;
	}

	p += clen;
	len -= clen;
    }

    if (ret != CONFACK) {
	if (ret == CONFREJ && *lenp == retp - p0)
	    pcb->ccp_all_rejected = 1;
	else
	    *lenp = retp - p0;
    }
#if MPPE_SUPPORT
    if (ret == CONFREJ && ao->mppe && rej_for_ci_mppe) {
	ppp_error("MPPE required but peer negotiation failed");
	lcp_close(pcb, "MPPE required but peer negotiation failed");
    }
#endif /* MPPE_SUPPORT */
    return ret;
}

/*
 * Make a string name for a compression method (or 2).
 */
static const char *method_name(ccp_options *opt, ccp_options *opt2) {
    static char result[64];
#if !DEFLATE_SUPPORT && !BSDCOMPRESS_SUPPORT
    LWIP_UNUSED_ARG(opt2);
#endif /* !DEFLATE_SUPPORT && !BSDCOMPRESS_SUPPORT */

    if (!ccp_anycompress(opt))
	return "(none)";
    switch (opt->method) {
#if MPPE_SUPPORT
    case CI_MPPE:
    {
	char *p = result;
	char *q = result + sizeof(result); /* 1 past result */

	ppp_slprintf(p, q - p, "MPPE ");
	p += 5;
	if (opt->mppe & MPPE_OPT_128) {
	    ppp_slprintf(p, q - p, "128-bit ");
	    p += 8;
	}
	if (opt->mppe & MPPE_OPT_40) {
	    ppp_slprintf(p, q - p, "40-bit ");
	    p += 7;
	}
	if (opt->mppe & MPPE_OPT_STATEFUL)
	    ppp_slprintf(p, q - p, "stateful");
	else
	    ppp_slprintf(p, q - p, "stateless");

	break;
    }
#endif /* MPPE_SUPPORT */
#if DEFLATE_SUPPORT
    case CI_DEFLATE:
    case CI_DEFLATE_DRAFT:
	if (opt2 != NULL && opt2->deflate_size != opt->deflate_size)
	    ppp_slprintf(result, sizeof(result), "Deflate%s (%d/%d)",
		     (opt->method == CI_DEFLATE_DRAFT? "(old#)": ""),
		     opt->deflate_size, opt2->deflate_size);
	else
	    ppp_slprintf(result, sizeof(result), "Deflate%s (%d)",
		     (opt->method == CI_DEFLATE_DRAFT? "(old#)": ""),
		     opt->deflate_size);
	break;
#endif /* DEFLATE_SUPPORT */
#if BSDCOMPRESS_SUPPORT
    case CI_BSD_COMPRESS:
	if (opt2 != NULL && opt2->bsd_bits != opt->bsd_bits)
	    ppp_slprintf(result, sizeof(result), "BSD-Compress (%d/%d)",
		     opt->bsd_bits, opt2->bsd_bits);
	else
	    ppp_slprintf(result, sizeof(result), "BSD-Compress (%d)",
		     opt->bsd_bits);
	break;
#endif /* BSDCOMPRESS_SUPPORT */
#if PREDICTOR_SUPPORT
    case CI_PREDICTOR_1:
	return "Predictor 1";
    case CI_PREDICTOR_2:
	return "Predictor 2";
#endif /* PREDICTOR_SUPPORT */
    default:
	ppp_slprintf(result, sizeof(result), "Method %d", opt->method);
    }
    return result;
}

/*
 * CCP has come up - inform the kernel driver and log a message.
 */
static void ccp_up(fsm *f) {
    ppp_pcb *pcb = f->pcb;
    ccp_options *go = &pcb->ccp_gotoptions;
    ccp_options *ho = &pcb->ccp_hisoptions;
    char method1[64];

    ccp_set(pcb, 1, 1, go->method, ho->method);
    if (ccp_anycompress(go)) {
	if (ccp_anycompress(ho)) {
	    if (go->method == ho->method) {
		ppp_notice("%s compression enabled", method_name(go, ho));
	    } else {
		ppp_strlcpy(method1, method_name(go, NULL), sizeof(method1));
		ppp_notice("%s / %s compression enabled",
		       method1, method_name(ho, NULL));
	    }
	} else
	    ppp_notice("%s receive compression enabled", method_name(go, NULL));
    } else if (ccp_anycompress(ho))
	ppp_notice("%s transmit compression enabled", method_name(ho, NULL));
#if MPPE_SUPPORT
    if (go->mppe) {
	continue_networks(pcb);		/* Bring up IP et al */
    }
#endif /* MPPE_SUPPORT */
}

/*
 * CCP has gone down - inform the kernel driver.
 */
static void ccp_down(fsm *f) {
    ppp_pcb *pcb = f->pcb;
#if MPPE_SUPPORT
    ccp_options *go = &pcb->ccp_gotoptions;
#endif /* MPPE_SUPPORT */

    if (pcb->ccp_localstate & RACK_PENDING)
	UNTIMEOUT(ccp_rack_timeout, f);
    pcb->ccp_localstate = 0;
    ccp_set(pcb, 1, 0, 0, 0);
#if MPPE_SUPPORT
    if (go->mppe) {
	go->mppe = 0;
	if (pcb->lcp_fsm.state == PPP_FSM_OPENED) {
	    /* If LCP is not already going down, make sure it does. */
	    ppp_error("MPPE disabled");
	    lcp_close(pcb, "MPPE disabled");
	}
    }
#endif /* MPPE_SUPPORT */
}

#if PRINTPKT_SUPPORT
/*
 * Print the contents of a CCP packet.
 */
static const char* const ccp_codenames[] = {
    "ConfReq", "ConfAck", "ConfNak", "ConfRej",
    "TermReq", "TermAck", "CodeRej",
    NULL, NULL, NULL, NULL, NULL, NULL,
    "ResetReq", "ResetAck",
};

static int ccp_printpkt(const u_char *p, int plen, void (*printer) (void *, const char *, ...), void *arg) {
    const u_char *p0, *optend;
    int code, id, len;
    int optlen;

    p0 = p;
    if (plen < HEADERLEN)
	return 0;
    code = p[0];
    id = p[1];
    len = (p[2] << 8) + p[3];
    if (len < HEADERLEN || len > plen)
	return 0;

    if (code >= 1 && code <= (int)LWIP_ARRAYSIZE(ccp_codenames) && ccp_codenames[code-1] != NULL)
	printer(arg, " %s", ccp_codenames[code-1]);
    else
	printer(arg, " code=0x%x", code);
    printer(arg, " id=0x%x", id);
    len -= HEADERLEN;
    p += HEADERLEN;

    switch (code) {
    case CONFREQ:
    case CONFACK:
    case CONFNAK:
    case CONFREJ:
	/* print list of possible compression methods */
	while (len >= 2) {
	    code = p[0];
	    optlen = p[1];
	    if (optlen < 2 || optlen > len)
		break;
	    printer(arg, " <");
	    len -= optlen;
	    optend = p + optlen;
	    switch (code) {
#if MPPE_SUPPORT
	    case CI_MPPE:
		if (optlen >= CILEN_MPPE) {
		    u_char mppe_opts;

		    MPPE_CI_TO_OPTS(&p[2], mppe_opts);
		    printer(arg, "mppe %s %s %s %s %s %s%s",
			    (p[2] & MPPE_H_BIT)? "+H": "-H",
			    (p[5] & MPPE_M_BIT)? "+M": "-M",
			    (p[5] & MPPE_S_BIT)? "+S": "-S",
			    (p[5] & MPPE_L_BIT)? "+L": "-L",
			    (p[5] & MPPE_D_BIT)? "+D": "-D",
			    (p[5] & MPPE_C_BIT)? "+C": "-C",
			    (mppe_opts & MPPE_OPT_UNKNOWN)? " +U": "");
		    if (mppe_opts & MPPE_OPT_UNKNOWN)
			printer(arg, " (%.2x %.2x %.2x %.2x)",
				p[2], p[3], p[4], p[5]);
		    p += CILEN_MPPE;
		}
		break;
#endif /* MPPE_SUPPORT */
#if DEFLATE_SUPPORT
	    case CI_DEFLATE:
	    case CI_DEFLATE_DRAFT:
		if (optlen >= CILEN_DEFLATE) {
		    printer(arg, "deflate%s %d",
			    (code == CI_DEFLATE_DRAFT? "(old#)": ""),
			    DEFLATE_SIZE(p[2]));
		    if (DEFLATE_METHOD(p[2]) != DEFLATE_METHOD_VAL)
			printer(arg, " method %d", DEFLATE_METHOD(p[2]));
		    if (p[3] != DEFLATE_CHK_SEQUENCE)
			printer(arg, " check %d", p[3]);
		    p += CILEN_DEFLATE;
		}
		break;
#endif /* DEFLATE_SUPPORT */
#if BSDCOMPRESS_SUPPORT
	    case CI_BSD_COMPRESS:
		if (optlen >= CILEN_BSD_COMPRESS) {
		    printer(arg, "bsd v%d %d", BSD_VERSION(p[2]),
			    BSD_NBITS(p[2]));
		    p += CILEN_BSD_COMPRESS;
		}
		break;
#endif /* BSDCOMPRESS_SUPPORT */
#if PREDICTOR_SUPPORT
	    case CI_PREDICTOR_1:
		if (optlen >= CILEN_PREDICTOR_1) {
		    printer(arg, "predictor 1");
		    p += CILEN_PREDICTOR_1;
		}
		break;
	    case CI_PREDICTOR_2:
		if (optlen >= CILEN_PREDICTOR_2) {
		    printer(arg, "predictor 2");
		    p += CILEN_PREDICTOR_2;
		}
		break;
#endif /* PREDICTOR_SUPPORT */
	    default:
                break;
	    }
	    while (p < optend)
		printer(arg, " %.2x", *p++);
	    printer(arg, ">");
	}
	break;

    case TERMACK:
    case TERMREQ:
	if (len > 0 && *p >= ' ' && *p < 0x7f) {
	    ppp_print_string(p, len, printer, arg);
	    p += len;
	    len = 0;
	}
	break;
    default:
        break;
    }

    /* dump out the rest of the packet in hex */
    while (--len >= 0)
	printer(arg, " %.2x", *p++);

    return p - p0;
}
#endif /* PRINTPKT_SUPPORT */

#if PPP_DATAINPUT
/*
 * We have received a packet that the decompressor failed to
 * decompress.  Here we would expect to issue a reset-request, but
 * Motorola has a patent on resetting the compressor as a result of
 * detecting an error in the decompressed data after decompression.
 * (See US patent 5,130,993; international patent publication number
 * WO 91/10289; Australian patent 73296/91.)
 *
 * So we ask the kernel whether the error was detected after
 * decompression; if it was, we take CCP down, thus disabling
 * compression :-(, otherwise we issue the reset-request.
 */
static void ccp_datainput(ppp_pcb *pcb, u_char *pkt, int len) {
    fsm *f;
#if MPPE_SUPPORT
    ccp_options *go = &pcb->ccp_gotoptions;
#endif /* MPPE_SUPPORT */
    LWIP_UNUSED_ARG(pkt);
    LWIP_UNUSED_ARG(len);

    f = &pcb->ccp_fsm;
    if (f->state == PPP_FSM_OPENED) {
	if (ccp_fatal_error(pcb)) {
	    /*
	     * Disable compression by taking CCP down.
	     */
	    ppp_error("Lost compression sync: disabling compression");
	    ccp_close(pcb, "Lost compression sync");
#if MPPE_SUPPORT
	    /*
	     * If we were doing MPPE, we must also take the link down.
	     */
	    if (go->mppe) {
		ppp_error("Too many MPPE errors, closing LCP");
		lcp_close(pcb, "Too many MPPE errors");
	    }
#endif /* MPPE_SUPPORT */
	} else {
	    /*
	     * Send a reset-request to reset the peer's compressor.
	     * We don't do that if we are still waiting for an
	     * acknowledgement to a previous reset-request.
	     */
	    if (!(pcb->ccp_localstate & RACK_PENDING)) {
		fsm_sdata(f, CCP_RESETREQ, f->reqid = ++f->id, NULL, 0);
		TIMEOUT(ccp_rack_timeout, f, RACKTIMEOUT);
		pcb->ccp_localstate |= RACK_PENDING;
	    } else
		pcb->ccp_localstate |= RREQ_REPEAT;
	}
    }
}
#endif /* PPP_DATAINPUT */

/*
 * We have received a packet that the decompressor failed to
 * decompress. Issue a reset-request.
 */
void ccp_resetrequest(ppp_pcb *pcb) {
    fsm *f = &pcb->ccp_fsm;

    if (f->state != PPP_FSM_OPENED)
	return;

    /*
     * Send a reset-request to reset the peer's compressor.
     * We don't do that if we are still waiting for an
     * acknowledgement to a previous reset-request.
     */
    if (!(pcb->ccp_localstate & RACK_PENDING)) {
	fsm_sdata(f, CCP_RESETREQ, f->reqid = ++f->id, NULL, 0);
	TIMEOUT(ccp_rack_timeout, f, RACKTIMEOUT);
	pcb->ccp_localstate |= RACK_PENDING;
    } else
	pcb->ccp_localstate |= RREQ_REPEAT;
}

/*
 * Timeout waiting for reset-ack.
 */
static void ccp_rack_timeout(void *arg) {
    fsm *f = (fsm*)arg;
    ppp_pcb *pcb = f->pcb;

    if (f->state == PPP_FSM_OPENED && (pcb->ccp_localstate & RREQ_REPEAT)) {
	fsm_sdata(f, CCP_RESETREQ, f->reqid, NULL, 0);
	TIMEOUT(ccp_rack_timeout, f, RACKTIMEOUT);
	pcb->ccp_localstate &= ~RREQ_REPEAT;
    } else
	pcb->ccp_localstate &= ~RACK_PENDING;
}

#endif /* PPP_SUPPORT && CCP_SUPPORT */

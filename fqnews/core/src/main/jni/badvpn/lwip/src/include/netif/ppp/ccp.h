/*
 * ccp.h - Definitions for PPP Compression Control Protocol.
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
 *
 * $Id: ccp.h,v 1.12 2004/11/04 10:02:26 paulus Exp $
 */

#include "netif/ppp/ppp_opts.h"
#if PPP_SUPPORT && CCP_SUPPORT  /* don't build if not configured for use in lwipopts.h */

#ifndef CCP_H
#define CCP_H

/*
 * CCP codes.
 */

#define CCP_CONFREQ	1
#define CCP_CONFACK	2
#define CCP_TERMREQ	5
#define CCP_TERMACK	6
#define CCP_RESETREQ	14
#define CCP_RESETACK	15

/*
 * Max # bytes for a CCP option
 */

#define CCP_MAX_OPTION_LENGTH	32

/*
 * Parts of a CCP packet.
 */

#define CCP_CODE(dp)		((dp)[0])
#define CCP_ID(dp)		((dp)[1])
#define CCP_LENGTH(dp)		(((dp)[2] << 8) + (dp)[3])
#define CCP_HDRLEN		4

#define CCP_OPT_CODE(dp)	((dp)[0])
#define CCP_OPT_LENGTH(dp)	((dp)[1])
#define CCP_OPT_MINLEN		2

#if BSDCOMPRESS_SUPPORT
/*
 * Definitions for BSD-Compress.
 */

#define CI_BSD_COMPRESS		21	/* config. option for BSD-Compress */
#define CILEN_BSD_COMPRESS	3	/* length of config. option */

/* Macros for handling the 3rd byte of the BSD-Compress config option. */
#define BSD_NBITS(x)		((x) & 0x1F)	/* number of bits requested */
#define BSD_VERSION(x)		((x) >> 5)	/* version of option format */
#define BSD_CURRENT_VERSION	1		/* current version number */
#define BSD_MAKE_OPT(v, n)	(((v) << 5) | (n))

#define BSD_MIN_BITS		9	/* smallest code size supported */
#define BSD_MAX_BITS		15	/* largest code size supported */
#endif /* BSDCOMPRESS_SUPPORT */

#if DEFLATE_SUPPORT
/*
 * Definitions for Deflate.
 */

#define CI_DEFLATE		26	/* config option for Deflate */
#define CI_DEFLATE_DRAFT	24	/* value used in original draft RFC */
#define CILEN_DEFLATE		4	/* length of its config option */

#define DEFLATE_MIN_SIZE	9
#define DEFLATE_MAX_SIZE	15
#define DEFLATE_METHOD_VAL	8
#define DEFLATE_SIZE(x)		(((x) >> 4) + 8)
#define DEFLATE_METHOD(x)	((x) & 0x0F)
#define DEFLATE_MAKE_OPT(w)	((((w) - 8) << 4) + DEFLATE_METHOD_VAL)
#define DEFLATE_CHK_SEQUENCE	0
#endif /* DEFLATE_SUPPORT */

#if MPPE_SUPPORT
/*
 * Definitions for MPPE.
 */

#define CI_MPPE                18      /* config option for MPPE */
#define CILEN_MPPE              6      /* length of config option */
#endif /* MPPE_SUPPORT */

#if PREDICTOR_SUPPORT
/*
 * Definitions for other, as yet unsupported, compression methods.
 */

#define CI_PREDICTOR_1		1	/* config option for Predictor-1 */
#define CILEN_PREDICTOR_1	2	/* length of its config option */
#define CI_PREDICTOR_2		2	/* config option for Predictor-2 */
#define CILEN_PREDICTOR_2	2	/* length of its config option */
#endif /* PREDICTOR_SUPPORT */

typedef struct ccp_options {
#if DEFLATE_SUPPORT
    unsigned int deflate          :1; /* do Deflate? */
    unsigned int deflate_correct  :1; /* use correct code for deflate? */
    unsigned int deflate_draft    :1; /* use draft RFC code for deflate? */
#endif /* DEFLATE_SUPPORT */
#if BSDCOMPRESS_SUPPORT
    unsigned int bsd_compress     :1; /* do BSD Compress? */
#endif /* BSDCOMPRESS_SUPPORT */
#if PREDICTOR_SUPPORT
    unsigned int predictor_1      :1; /* do Predictor-1? */
    unsigned int predictor_2      :1; /* do Predictor-2? */
#endif /* PREDICTOR_SUPPORT */

#if MPPE_SUPPORT
    u8_t mppe;			/* MPPE bitfield */
#endif /* MPPE_SUPPORT */
#if BSDCOMPRESS_SUPPORT
    u_short bsd_bits;		/* # bits/code for BSD Compress */
#endif /* BSDCOMPRESS_SUPPORT */
#if DEFLATE_SUPPORT
    u_short deflate_size;	/* lg(window size) for Deflate */
#endif /* DEFLATE_SUPPORT */
    u8_t method;		/* code for chosen compression method */
} ccp_options;

extern const struct protent ccp_protent;

void ccp_resetrequest(ppp_pcb *pcb);  /* Issue a reset-request. */

#endif /* CCP_H */
#endif /* PPP_SUPPORT && CCP_SUPPORT */

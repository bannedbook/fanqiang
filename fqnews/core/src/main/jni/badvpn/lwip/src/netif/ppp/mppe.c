/*
 * mppe.c - interface MPPE to the PPP code.
 *
 * By Frank Cusack <fcusack@fcusack.com>.
 * Copyright (c) 2002,2003,2004 Google, Inc.
 * All rights reserved.
 *
 * License:
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, provided that the above copyright
 * notice appears in all copies.  This software is provided without any
 * warranty, express or implied.
 *
 * Changelog:
 *      08/12/05 - Matt Domsch <Matt_Domsch@dell.com>
 *                 Only need extra skb padding on transmit, not receive.
 *      06/18/04 - Matt Domsch <Matt_Domsch@dell.com>, Oleg Makarenko <mole@quadra.ru>
 *                 Use Linux kernel 2.6 arc4 and sha1 routines rather than
 *                 providing our own.
 *      2/15/04 - TS: added #include <version.h> and testing for Kernel
 *                    version before using
 *                    MOD_DEC_USAGE_COUNT/MOD_INC_USAGE_COUNT which are
 *                    deprecated in 2.6
 */

#include "netif/ppp/ppp_opts.h"
#if PPP_SUPPORT && MPPE_SUPPORT  /* don't build if not configured for use in lwipopts.h */

#include <string.h>

#include "lwip/err.h"

#include "netif/ppp/ppp_impl.h"
#include "netif/ppp/ccp.h"
#include "netif/ppp/mppe.h"
#include "netif/ppp/pppdebug.h"
#include "netif/ppp/pppcrypt.h"

#define SHA1_SIGNATURE_SIZE 20

/* ppp_mppe_state.bits definitions */
#define MPPE_BIT_A	0x80	/* Encryption table were (re)inititalized */
#define MPPE_BIT_B	0x40	/* MPPC only (not implemented) */
#define MPPE_BIT_C	0x20	/* MPPC only (not implemented) */
#define MPPE_BIT_D	0x10	/* This is an encrypted frame */

#define MPPE_BIT_FLUSHED	MPPE_BIT_A
#define MPPE_BIT_ENCRYPTED	MPPE_BIT_D

#define MPPE_BITS(p) ((p)[0] & 0xf0)
#define MPPE_CCOUNT(p) ((((p)[0] & 0x0f) << 8) + (p)[1])
#define MPPE_CCOUNT_SPACE 0x1000	/* The size of the ccount space */

#define MPPE_OVHD	2	/* MPPE overhead/packet */
#define SANITY_MAX	1600	/* Max bogon factor we will tolerate */

/*
 * Perform the MPPE rekey algorithm, from RFC 3078, sec. 7.3.
 * Well, not what's written there, but rather what they meant.
 */
static void mppe_rekey(ppp_mppe_state * state, int initial_key)
{
	lwip_sha1_context sha1_ctx;
	u8_t sha1_digest[SHA1_SIGNATURE_SIZE];

	/*
	 * Key Derivation, from RFC 3078, RFC 3079.
	 * Equivalent to Get_Key() for MS-CHAP as described in RFC 3079.
	 */
	lwip_sha1_init(&sha1_ctx);
	lwip_sha1_starts(&sha1_ctx);
	lwip_sha1_update(&sha1_ctx, state->master_key, state->keylen);
	lwip_sha1_update(&sha1_ctx, mppe_sha1_pad1, SHA1_PAD_SIZE);
	lwip_sha1_update(&sha1_ctx, state->session_key, state->keylen);
	lwip_sha1_update(&sha1_ctx, mppe_sha1_pad2, SHA1_PAD_SIZE);
	lwip_sha1_finish(&sha1_ctx, sha1_digest);
	lwip_sha1_free(&sha1_ctx);
	MEMCPY(state->session_key, sha1_digest, state->keylen);

	if (!initial_key) {
		lwip_arc4_init(&state->arc4);
		lwip_arc4_setup(&state->arc4, sha1_digest, state->keylen);
		lwip_arc4_crypt(&state->arc4, state->session_key, state->keylen);
		lwip_arc4_free(&state->arc4);
	}
	if (state->keylen == 8) {
		/* See RFC 3078 */
		state->session_key[0] = 0xd1;
		state->session_key[1] = 0x26;
		state->session_key[2] = 0x9e;
	}
	lwip_arc4_init(&state->arc4);
	lwip_arc4_setup(&state->arc4, state->session_key, state->keylen);
}

/*
 * Set key, used by MSCHAP before mppe_init() is actually called by CCP so we
 * don't have to keep multiple copies of keys.
 */
void mppe_set_key(ppp_pcb *pcb, ppp_mppe_state *state, u8_t *key) {
	LWIP_UNUSED_ARG(pcb);
	MEMCPY(state->master_key, key, MPPE_MAX_KEY_LEN);
}

/*
 * Initialize (de)compressor state.
 */
void
mppe_init(ppp_pcb *pcb, ppp_mppe_state *state, u8_t options)
{
#if PPP_DEBUG
	const u8_t *debugstr = (const u8_t*)"mppe_comp_init";
	if (&pcb->mppe_decomp == state) {
	    debugstr = (const u8_t*)"mppe_decomp_init";
	}
#endif /* PPP_DEBUG */

	/* Save keys. */
	MEMCPY(state->session_key, state->master_key, sizeof(state->master_key));

	if (options & MPPE_OPT_128)
		state->keylen = 16;
	else if (options & MPPE_OPT_40)
		state->keylen = 8;
	else {
		PPPDEBUG(LOG_DEBUG, ("%s[%d]: unknown key length\n", debugstr,
			pcb->netif->num));
		lcp_close(pcb, "MPPE required but peer negotiation failed");
		return;
	}
	if (options & MPPE_OPT_STATEFUL)
		state->stateful = 1;

	/* Generate the initial session key. */
	mppe_rekey(state, 1);

#if PPP_DEBUG
	{
		int i;
		char mkey[sizeof(state->master_key) * 2 + 1];
		char skey[sizeof(state->session_key) * 2 + 1];

		PPPDEBUG(LOG_DEBUG, ("%s[%d]: initialized with %d-bit %s mode\n",
		       debugstr, pcb->netif->num, (state->keylen == 16) ? 128 : 40,
		       (state->stateful) ? "stateful" : "stateless"));

		for (i = 0; i < (int)sizeof(state->master_key); i++)
			sprintf(mkey + i * 2, "%02x", state->master_key[i]);
		for (i = 0; i < (int)sizeof(state->session_key); i++)
			sprintf(skey + i * 2, "%02x", state->session_key[i]);
		PPPDEBUG(LOG_DEBUG,
		       ("%s[%d]: keys: master: %s initial session: %s\n",
		       debugstr, pcb->netif->num, mkey, skey));
	}
#endif /* PPP_DEBUG */

	/*
	 * Initialize the coherency count.  The initial value is not specified
	 * in RFC 3078, but we can make a reasonable assumption that it will
	 * start at 0.  Setting it to the max here makes the comp/decomp code
	 * do the right thing (determined through experiment).
	 */
	state->ccount = MPPE_CCOUNT_SPACE - 1;

	/*
	 * Note that even though we have initialized the key table, we don't
	 * set the FLUSHED bit.  This is contrary to RFC 3078, sec. 3.1.
	 */
	state->bits = MPPE_BIT_ENCRYPTED;
}

/*
 * We received a CCP Reset-Request (actually, we are sending a Reset-Ack),
 * tell the compressor to rekey.  Note that we MUST NOT rekey for
 * every CCP Reset-Request; we only rekey on the next xmit packet.
 * We might get multiple CCP Reset-Requests if our CCP Reset-Ack is lost.
 * So, rekeying for every CCP Reset-Request is broken as the peer will not
 * know how many times we've rekeyed.  (If we rekey and THEN get another
 * CCP Reset-Request, we must rekey again.)
 */
void mppe_comp_reset(ppp_pcb *pcb, ppp_mppe_state *state)
{
	LWIP_UNUSED_ARG(pcb);
	state->bits |= MPPE_BIT_FLUSHED;
}

/*
 * Compress (encrypt) a packet.
 * It's strange to call this a compressor, since the output is always
 * MPPE_OVHD + 2 bytes larger than the input.
 */
err_t
mppe_compress(ppp_pcb *pcb, ppp_mppe_state *state, struct pbuf **pb, u16_t protocol)
{
	struct pbuf *n, *np;
	u8_t *pl;
	err_t err;

	LWIP_UNUSED_ARG(pcb);

	/* TCP stack requires that we don't change the packet payload, therefore we copy
	 * the whole packet before encryption.
	 */
	np = pbuf_alloc(PBUF_RAW, MPPE_OVHD + sizeof(protocol) + (*pb)->tot_len, PBUF_POOL);
	if (!np) {
		return ERR_MEM;
	}

	/* Hide MPPE header + protocol */
	pbuf_remove_header(np, MPPE_OVHD + sizeof(protocol));

	if ((err = pbuf_copy(np, *pb)) != ERR_OK) {
		pbuf_free(np);
		return err;
	}

	/* Reveal MPPE header + protocol */
	pbuf_add_header(np, MPPE_OVHD + sizeof(protocol));

	*pb = np;
	pl = (u8_t*)np->payload;

	state->ccount = (state->ccount + 1) % MPPE_CCOUNT_SPACE;
	PPPDEBUG(LOG_DEBUG, ("mppe_compress[%d]: ccount %d\n", pcb->netif->num, state->ccount));
	/* FIXME: use PUT* macros */
	pl[0] = state->ccount>>8;
	pl[1] = state->ccount;

	if (!state->stateful ||	/* stateless mode     */
	    ((state->ccount & 0xff) == 0xff) ||	/* "flag" packet      */
	    (state->bits & MPPE_BIT_FLUSHED)) {	/* CCP Reset-Request  */
		/* We must rekey */
		if (state->stateful) {
			PPPDEBUG(LOG_DEBUG, ("mppe_compress[%d]: rekeying\n", pcb->netif->num));
		}
		mppe_rekey(state, 0);
		state->bits |= MPPE_BIT_FLUSHED;
	}
	pl[0] |= state->bits;
	state->bits &= ~MPPE_BIT_FLUSHED;	/* reset for next xmit */
	pl += MPPE_OVHD;

	/* Add protocol */
	/* FIXME: add PFC support */
	pl[0] = protocol >> 8;
	pl[1] = protocol;

	/* Hide MPPE header */
	pbuf_remove_header(np, MPPE_OVHD);

	/* Encrypt packet */
	for (n = np; n != NULL; n = n->next) {
		lwip_arc4_crypt(&state->arc4, (u8_t*)n->payload, n->len);
		if (n->tot_len == n->len) {
			break;
		}
	}

	/* Reveal MPPE header */
	pbuf_add_header(np, MPPE_OVHD);

	return ERR_OK;
}

/*
 * We received a CCP Reset-Ack.  Just ignore it.
 */
void mppe_decomp_reset(ppp_pcb *pcb, ppp_mppe_state *state)
{
	LWIP_UNUSED_ARG(pcb);
	LWIP_UNUSED_ARG(state);
	return;
}

/*
 * Decompress (decrypt) an MPPE packet.
 */
err_t
mppe_decompress(ppp_pcb *pcb, ppp_mppe_state *state, struct pbuf **pb)
{
	struct pbuf *n0 = *pb, *n;
	u8_t *pl;
	u16_t ccount;
	u8_t flushed;

	/* MPPE Header */
	if (n0->len < MPPE_OVHD) {
		PPPDEBUG(LOG_DEBUG,
		       ("mppe_decompress[%d]: short pkt (%d)\n",
		       pcb->netif->num, n0->len));
		state->sanity_errors += 100;
		goto sanity_error;
	}

	pl = (u8_t*)n0->payload;
	flushed = MPPE_BITS(pl) & MPPE_BIT_FLUSHED;
	ccount = MPPE_CCOUNT(pl);
	PPPDEBUG(LOG_DEBUG, ("mppe_decompress[%d]: ccount %d\n",
	       pcb->netif->num, ccount));

	/* sanity checks -- terminate with extreme prejudice */
	if (!(MPPE_BITS(pl) & MPPE_BIT_ENCRYPTED)) {
		PPPDEBUG(LOG_DEBUG,
		       ("mppe_decompress[%d]: ENCRYPTED bit not set!\n",
		       pcb->netif->num));
		state->sanity_errors += 100;
		goto sanity_error;
	}
	if (!state->stateful && !flushed) {
		PPPDEBUG(LOG_DEBUG, ("mppe_decompress[%d]: FLUSHED bit not set in "
		       "stateless mode!\n", pcb->netif->num));
		state->sanity_errors += 100;
		goto sanity_error;
	}
	if (state->stateful && ((ccount & 0xff) == 0xff) && !flushed) {
		PPPDEBUG(LOG_DEBUG, ("mppe_decompress[%d]: FLUSHED bit not set on "
		       "flag packet!\n", pcb->netif->num));
		state->sanity_errors += 100;
		goto sanity_error;
	}

	/*
	 * Check the coherency count.
	 */

	if (!state->stateful) {
		/* Discard late packet */
		if ((ccount - state->ccount) % MPPE_CCOUNT_SPACE > MPPE_CCOUNT_SPACE / 2) {
			state->sanity_errors++;
			goto sanity_error;
		}

		/* RFC 3078, sec 8.1.  Rekey for every packet. */
		while (state->ccount != ccount) {
			mppe_rekey(state, 0);
			state->ccount = (state->ccount + 1) % MPPE_CCOUNT_SPACE;
		}
	} else {
		/* RFC 3078, sec 8.2. */
		if (!state->discard) {
			/* normal state */
			state->ccount = (state->ccount + 1) % MPPE_CCOUNT_SPACE;
			if (ccount != state->ccount) {
				/*
				 * (ccount > state->ccount)
				 * Packet loss detected, enter the discard state.
				 * Signal the peer to rekey (by sending a CCP Reset-Request).
				 */
				state->discard = 1;
				ccp_resetrequest(pcb);
				return ERR_BUF;
			}
		} else {
			/* discard state */
			if (!flushed) {
				/* ccp.c will be silent (no additional CCP Reset-Requests). */
				return ERR_BUF;
			} else {
				/* Rekey for every missed "flag" packet. */
				while ((ccount & ~0xff) !=
				       (state->ccount & ~0xff)) {
					mppe_rekey(state, 0);
					state->ccount =
					    (state->ccount +
					     256) % MPPE_CCOUNT_SPACE;
				}

				/* reset */
				state->discard = 0;
				state->ccount = ccount;
				/*
				 * Another problem with RFC 3078 here.  It implies that the
				 * peer need not send a Reset-Ack packet.  But RFC 1962
				 * requires it.  Hopefully, M$ does send a Reset-Ack; even
				 * though it isn't required for MPPE synchronization, it is
				 * required to reset CCP state.
				 */
			}
		}
		if (flushed)
			mppe_rekey(state, 0);
	}

	/* Hide MPPE header */
	pbuf_remove_header(n0, MPPE_OVHD);

	/* Decrypt the packet. */
	for (n = n0; n != NULL; n = n->next) {
		lwip_arc4_crypt(&state->arc4, (u8_t*)n->payload, n->len);
		if (n->tot_len == n->len) {
			break;
		}
	}

	/* good packet credit */
	state->sanity_errors >>= 1;

	return ERR_OK;

sanity_error:
	if (state->sanity_errors >= SANITY_MAX) {
		/*
		 * Take LCP down if the peer is sending too many bogons.
		 * We don't want to do this for a single or just a few
		 * instances since it could just be due to packet corruption.
		 */
		lcp_close(pcb, "Too many MPPE errors");
	}
	return ERR_BUF;
}

#endif /* PPP_SUPPORT && MPPE_SUPPORT */

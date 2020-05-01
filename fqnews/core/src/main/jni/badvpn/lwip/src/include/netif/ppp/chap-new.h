/*
 * chap-new.c - New CHAP implementation.
 *
 * Copyright (c) 2003 Paul Mackerras. All rights reserved.
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
#if PPP_SUPPORT && CHAP_SUPPORT  /* don't build if not configured for use in lwipopts.h */

#ifndef CHAP_H
#define CHAP_H

#include "ppp.h"

/*
 * CHAP packets begin with a standard header with code, id, len (2 bytes).
 */
#define CHAP_HDRLEN	4

/*
 * Values for the code field.
 */
#define CHAP_CHALLENGE	1
#define CHAP_RESPONSE	2
#define CHAP_SUCCESS	3
#define CHAP_FAILURE	4

/*
 * CHAP digest codes.
 */
#define CHAP_MD5		5
#if MSCHAP_SUPPORT
#define CHAP_MICROSOFT		0x80
#define CHAP_MICROSOFT_V2	0x81
#endif /* MSCHAP_SUPPORT */

/*
 * Semi-arbitrary limits on challenge and response fields.
 */
#define MAX_CHALLENGE_LEN	64
#define MAX_RESPONSE_LEN	64

/*
 * These limits apply to challenge and response packets we send.
 * The +4 is the +1 that we actually need rounded up.
 */
#define CHAL_MAX_PKTLEN	(PPP_HDRLEN + CHAP_HDRLEN + 4 + MAX_CHALLENGE_LEN + MAXNAMELEN)
#define RESP_MAX_PKTLEN	(PPP_HDRLEN + CHAP_HDRLEN + 4 + MAX_RESPONSE_LEN + MAXNAMELEN)

/* bitmask of supported algorithms */
#if MSCHAP_SUPPORT
#define MDTYPE_MICROSOFT_V2	0x1
#define MDTYPE_MICROSOFT	0x2
#endif /* MSCHAP_SUPPORT */
#define MDTYPE_MD5		0x4
#define MDTYPE_NONE		0

#if MSCHAP_SUPPORT
/* Return the digest alg. ID for the most preferred digest type. */
#define CHAP_DIGEST(mdtype) \
    ((mdtype) & MDTYPE_MD5)? CHAP_MD5: \
    ((mdtype) & MDTYPE_MICROSOFT_V2)? CHAP_MICROSOFT_V2: \
    ((mdtype) & MDTYPE_MICROSOFT)? CHAP_MICROSOFT: \
    0
#else /* !MSCHAP_SUPPORT */
#define CHAP_DIGEST(mdtype) \
    ((mdtype) & MDTYPE_MD5)? CHAP_MD5: \
    0
#endif /* MSCHAP_SUPPORT */

/* Return the bit flag (lsb set) for our most preferred digest type. */
#define CHAP_MDTYPE(mdtype) ((mdtype) ^ ((mdtype) - 1)) & (mdtype)

/* Return the bit flag for a given digest algorithm ID. */
#if MSCHAP_SUPPORT
#define CHAP_MDTYPE_D(digest) \
    ((digest) == CHAP_MICROSOFT_V2)? MDTYPE_MICROSOFT_V2: \
    ((digest) == CHAP_MICROSOFT)? MDTYPE_MICROSOFT: \
    ((digest) == CHAP_MD5)? MDTYPE_MD5: \
    0
#else /* !MSCHAP_SUPPORT */
#define CHAP_MDTYPE_D(digest) \
    ((digest) == CHAP_MD5)? MDTYPE_MD5: \
    0
#endif /* MSCHAP_SUPPORT */

/* Can we do the requested digest? */
#if MSCHAP_SUPPORT
#define CHAP_CANDIGEST(mdtype, digest) \
    ((digest) == CHAP_MICROSOFT_V2)? (mdtype) & MDTYPE_MICROSOFT_V2: \
    ((digest) == CHAP_MICROSOFT)? (mdtype) & MDTYPE_MICROSOFT: \
    ((digest) == CHAP_MD5)? (mdtype) & MDTYPE_MD5: \
    0
#else /* !MSCHAP_SUPPORT */
#define CHAP_CANDIGEST(mdtype, digest) \
    ((digest) == CHAP_MD5)? (mdtype) & MDTYPE_MD5: \
    0
#endif /* MSCHAP_SUPPORT */

/*
 * The code for each digest type has to supply one of these.
 */
struct chap_digest_type {
	int code;

#if PPP_SERVER
	/*
	 * Note: challenge and response arguments below are formatted as
	 * a length byte followed by the actual challenge/response data.
	 */
	void (*generate_challenge)(ppp_pcb *pcb, unsigned char *challenge);
	int (*verify_response)(ppp_pcb *pcb, int id, const char *name,
		const unsigned char *secret, int secret_len,
		const unsigned char *challenge, const unsigned char *response,
		char *message, int message_space);
#endif /* PPP_SERVER */
	void (*make_response)(ppp_pcb *pcb, unsigned char *response, int id, const char *our_name,
		const unsigned char *challenge, const char *secret, int secret_len,
		unsigned char *priv);
	int (*check_success)(ppp_pcb *pcb, unsigned char *pkt, int len, unsigned char *priv);
	void (*handle_failure)(ppp_pcb *pcb, unsigned char *pkt, int len);
};

/*
 * Each interface is described by chap structure.
 */
#if CHAP_SUPPORT
typedef struct chap_client_state {
	u8_t flags;
	const char *name;
	const struct chap_digest_type *digest;
	unsigned char priv[64];		/* private area for digest's use */
} chap_client_state;

#if PPP_SERVER
typedef struct chap_server_state {
	u8_t flags;
	u8_t id;
	const char *name;
	const struct chap_digest_type *digest;
	int challenge_xmits;
	int challenge_pktlen;
	unsigned char challenge[CHAL_MAX_PKTLEN];
} chap_server_state;
#endif /* PPP_SERVER */
#endif /* CHAP_SUPPORT */

#if 0 /* UNUSED */
/* Hook for a plugin to validate CHAP challenge */
extern int (*chap_verify_hook)(char *name, char *ourname, int id,
			const struct chap_digest_type *digest,
			unsigned char *challenge, unsigned char *response,
			char *message, int message_space);
#endif /* UNUSED */

#if PPP_SERVER
/* Called by authentication code to start authenticating the peer. */
extern void chap_auth_peer(ppp_pcb *pcb, const char *our_name, int digest_code);
#endif /* PPP_SERVER */

/* Called by auth. code to start authenticating us to the peer. */
extern void chap_auth_with_peer(ppp_pcb *pcb, const char *our_name, int digest_code);

/* Represents the CHAP protocol to the main pppd code */
extern const struct protent chap_protent;

#endif /* CHAP_H */
#endif /* PPP_SUPPORT && CHAP_SUPPORT */

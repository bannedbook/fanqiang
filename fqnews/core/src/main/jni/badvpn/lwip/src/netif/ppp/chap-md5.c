/*
 * chap-md5.c - New CHAP/MD5 implementation.
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

#if 0 /* UNUSED */
#include <stdlib.h>
#include <string.h>
#endif /* UNUSED */

#include "netif/ppp/ppp_impl.h"

#include "netif/ppp/chap-new.h"
#include "netif/ppp/chap-md5.h"
#include "netif/ppp/magic.h"
#include "netif/ppp/pppcrypt.h"

#define MD5_HASH_SIZE		16
#define MD5_MIN_CHALLENGE	17
#define MD5_MAX_CHALLENGE	24
#define MD5_MIN_MAX_POWER_OF_TWO_CHALLENGE     3   /* 2^3-1 = 7, 17+7 = 24 */

#if PPP_SERVER
static void chap_md5_generate_challenge(ppp_pcb *pcb, unsigned char *cp) {
	int clen;
	LWIP_UNUSED_ARG(pcb);

	clen = MD5_MIN_CHALLENGE + magic_pow(MD5_MIN_MAX_POWER_OF_TWO_CHALLENGE);
	*cp++ = clen;
	magic_random_bytes(cp, clen);
}

static int chap_md5_verify_response(ppp_pcb *pcb, int id, const char *name,
			 const unsigned char *secret, int secret_len,
			 const unsigned char *challenge, const unsigned char *response,
			 char *message, int message_space) {
	lwip_md5_context ctx;
	unsigned char idbyte = id;
	unsigned char hash[MD5_HASH_SIZE];
	int challenge_len, response_len;
	LWIP_UNUSED_ARG(name);
	LWIP_UNUSED_ARG(pcb);

	challenge_len = *challenge++;
	response_len = *response++;
	if (response_len == MD5_HASH_SIZE) {
		/* Generate hash of ID, secret, challenge */
		lwip_md5_init(&ctx);
		lwip_md5_starts(&ctx);
		lwip_md5_update(&ctx, &idbyte, 1);
		lwip_md5_update(&ctx, secret, secret_len);
		lwip_md5_update(&ctx, challenge, challenge_len);
		lwip_md5_finish(&ctx, hash);
		lwip_md5_free(&ctx);

		/* Test if our hash matches the peer's response */
		if (memcmp(hash, response, MD5_HASH_SIZE) == 0) {
			ppp_slprintf(message, message_space, "Access granted");
			return 1;
		}
	}
	ppp_slprintf(message, message_space, "Access denied");
	return 0;
}
#endif /* PPP_SERVER */

static void chap_md5_make_response(ppp_pcb *pcb, unsigned char *response, int id, const char *our_name,
		       const unsigned char *challenge, const char *secret, int secret_len,
		       unsigned char *private_) {
	lwip_md5_context ctx;
	unsigned char idbyte = id;
	int challenge_len = *challenge++;
	LWIP_UNUSED_ARG(our_name);
	LWIP_UNUSED_ARG(private_);
	LWIP_UNUSED_ARG(pcb);

	lwip_md5_init(&ctx);
	lwip_md5_starts(&ctx);
	lwip_md5_update(&ctx, &idbyte, 1);
	lwip_md5_update(&ctx, (const u_char *)secret, secret_len);
	lwip_md5_update(&ctx, challenge, challenge_len);
	lwip_md5_finish(&ctx, &response[1]);
	lwip_md5_free(&ctx);
	response[0] = MD5_HASH_SIZE;
}

const struct chap_digest_type md5_digest = {
	CHAP_MD5,		/* code */
#if PPP_SERVER
	chap_md5_generate_challenge,
	chap_md5_verify_response,
#endif /* PPP_SERVER */
	chap_md5_make_response,
	NULL,			/* check_success */
	NULL,			/* handle_failure */
};

#endif /* PPP_SUPPORT && CHAP_SUPPORT */

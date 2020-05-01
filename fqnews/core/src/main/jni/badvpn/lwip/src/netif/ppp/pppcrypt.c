/*
 * pppcrypt.c - PPP/DES linkage for MS-CHAP and EAP SRP-SHA1
 *
 * Extracted from chap_ms.c by James Carlson.
 *
 * Copyright (c) 1995 Eric Rosenquist.  All rights reserved.
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
 * THE AUTHORS OF THIS SOFTWARE DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "netif/ppp/ppp_opts.h"
#if PPP_SUPPORT && MSCHAP_SUPPORT /* don't build if not necessary */

#include "netif/ppp/ppp_impl.h"

#include "netif/ppp/pppcrypt.h"


static u_char pppcrypt_get_7bits(u_char *input, int startBit) {
	unsigned int word;

	word  = (unsigned)input[startBit / 8] << 8;
	word |= (unsigned)input[startBit / 8 + 1];

	word >>= 15 - (startBit % 8 + 7);

	return word & 0xFE;
}

/* IN  56 bit DES key missing parity bits
 * OUT 64 bit DES key with parity bits added
 */
void pppcrypt_56_to_64_bit_key(u_char *key, u_char * des_key) {
	des_key[0] = pppcrypt_get_7bits(key,  0);
	des_key[1] = pppcrypt_get_7bits(key,  7);
	des_key[2] = pppcrypt_get_7bits(key, 14);
	des_key[3] = pppcrypt_get_7bits(key, 21);
	des_key[4] = pppcrypt_get_7bits(key, 28);
	des_key[5] = pppcrypt_get_7bits(key, 35);
	des_key[6] = pppcrypt_get_7bits(key, 42);
	des_key[7] = pppcrypt_get_7bits(key, 49);
}

#endif /* PPP_SUPPORT && MSCHAP_SUPPORT */

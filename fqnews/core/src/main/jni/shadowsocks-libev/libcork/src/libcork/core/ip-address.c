/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>

#include "libcork/core/byte-order.h"
#include "libcork/core/error.h"
#include "libcork/core/net-addresses.h"
#include "libcork/core/types.h"

#ifndef CORK_IP_ADDRESS_DEBUG
#define CORK_IP_ADDRESS_DEBUG 0
#endif

#if CORK_IP_ADDRESS_DEBUG
#include <stdio.h>
#define DEBUG(...) \
    do { \
        fprintf(stderr, __VA_ARGS__); \
    } while (0)
#else
#define DEBUG(...) /* nothing */
#endif


/*-----------------------------------------------------------------------
 * IP addresses
 */

/*** IPv4 ***/

static inline const char *
cork_ipv4_parse(struct cork_ipv4 *addr, const char *str)
{
    const char  *ch;
    bool  seen_digit_in_octet = false;
    unsigned int  octets = 0;
    unsigned int  digit = 0;
    uint8_t  result[4];

    for (ch = str; *ch != '\0'; ch++) {
        DEBUG("%2u: %c\t", (unsigned int) (ch-str), *ch);
        switch (*ch) {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                seen_digit_in_octet = true;
                digit *= 10;
                digit += (*ch - '0');
                DEBUG("digit = %u\n", digit);
                if (CORK_UNLIKELY(digit > 255)) {
                    DEBUG("\t");
                    goto parse_error;
                }
                break;

            case '.':
                /* If this would be the fourth octet, it can't have a trailing
                 * period. */
                if (CORK_UNLIKELY(octets == 3)) {
                    goto parse_error;
                }
                DEBUG("octet %u = %u\n", octets, digit);
                result[octets] = digit;
                digit = 0;
                octets++;
                seen_digit_in_octet = false;
                break;

            default:
                /* Any other character is a parse error. */
                goto parse_error;
        }
    }

    /* If we have a valid octet at the end, and that would be the fourth octet,
     * then we've got a valid final parse. */
    DEBUG("%2u:\t", (unsigned int) (ch-str));
    if (CORK_LIKELY(seen_digit_in_octet && octets == 3)) {
#if CORK_IP_ADDRESS_DEBUG
        char  parsed_ipv4[CORK_IPV4_STRING_LENGTH];
#endif
        DEBUG("octet %u = %u\n", octets, digit);
        result[octets] = digit;
        cork_ipv4_copy(addr, result);
#if CORK_IP_ADDRESS_DEBUG
        cork_ipv4_to_raw_string(addr, parsed_ipv4);
        DEBUG("\tParsed address: %s\n", parsed_ipv4);
#endif
        return ch;
    }

parse_error:
    DEBUG("parse error\n");
    cork_parse_error("Invalid IPv4 address: \"%s\"", str);
    return NULL;
}

int
cork_ipv4_init(struct cork_ipv4 *addr, const char *str)
{
    return cork_ipv4_parse(addr, str) == NULL? -1: 0;
}

bool
cork_ipv4_equal_(const struct cork_ipv4 *addr1, const struct cork_ipv4 *addr2)
{
    return cork_ipv4_equal(addr1, addr2);
}

void
cork_ipv4_to_raw_string(const struct cork_ipv4 *addr, char *dest)
{
    snprintf(dest, CORK_IPV4_STRING_LENGTH, "%u.%u.%u.%u",
             addr->_.u8[0], addr->_.u8[1], addr->_.u8[2], addr->_.u8[3]);
}

bool
cork_ipv4_is_valid_network(const struct cork_ipv4 *addr,
                           unsigned int cidr_prefix)
{
    uint32_t  cidr_mask;

    if (cidr_prefix > 32) {
        return false;
    } else if (cidr_prefix == 32) {
        /* This handles undefined behavior for overflow bit shifts. */
        cidr_mask = 0;
    } else {
        cidr_mask = 0xffffffff >> cidr_prefix;
    }

    return (CORK_UINT32_BIG_TO_HOST(addr->_.u32) & cidr_mask) == 0;
}

/*** IPv6 ***/

int
cork_ipv6_init(struct cork_ipv6 *addr, const char *str)
{
    const char  *ch;

    uint16_t  digit = 0;
    unsigned int  before_count = 0;
    uint16_t  before_double_colon[8];
    uint16_t  after_double_colon[8];
    uint16_t  *dest = before_double_colon;

    unsigned int  digits_seen = 0;
    unsigned int  hextets_seen = 0;
    bool  another_required = true;
    bool  digit_allowed = true;
    bool  colon_allowed = true;
    bool  double_colon_allowed = true;
    bool  just_saw_colon = false;

    for (ch = str; *ch != '\0'; ch++) {
        DEBUG("%2u: %c\t", (unsigned int) (ch-str), *ch);
        switch (*ch) {
#define process_digit(base) \
                /* Make sure a digit is allowed here. */ \
                if (CORK_UNLIKELY(!digit_allowed)) { \
                    goto parse_error; \
                } \
                /* If we've already seen 4 digits, it's a parse error. */ \
                if (CORK_UNLIKELY(digits_seen == 4)) { \
                    goto parse_error; \
                } \
                \
                digits_seen++; \
                colon_allowed = true; \
                just_saw_colon = false; \
                digit <<= 4; \
                digit |= (*ch - (base)); \
                DEBUG("digit = %04x\n", digit);

            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                process_digit('0');
                break;

            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                process_digit('a'-10);
                break;

            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                process_digit('A'-10);
                break;

#undef process_digit

            case ':':
                /* We can only see a colon immediately after a hextet or as part
                 * of a double-colon. */
                if (CORK_UNLIKELY(!colon_allowed)) {
                    goto parse_error;
                }

                /* If this is a double-colon, start parsing hextets into our
                 * second array. */
                if (just_saw_colon) {
                    DEBUG("double-colon\n");
                    colon_allowed = false;
                    digit_allowed = true;
                    another_required = false;
                    double_colon_allowed = false;
                    before_count = hextets_seen;
                    dest = after_double_colon;
                    continue;
                }

                /* If this would end the eighth hextet (regardless of the
                 * placement of a double-colon), then there can't be a trailing
                 * colon. */
                if (CORK_UNLIKELY(hextets_seen == 8)) {
                    goto parse_error;
                }

                /* If this is the very beginning of the string, then we can only
                 * have a double-colon, not a single colon. */
                if (digits_seen == 0 && hextets_seen == 0) {
                    DEBUG("initial colon\n");
                    colon_allowed = true;
                    digit_allowed = false;
                    just_saw_colon = true;
                    another_required = true;
                    continue;
                }

                /* Otherwise this ends the current hextet. */
                DEBUG("hextet %u = %04x\n", hextets_seen, digit);
                *(dest++) = CORK_UINT16_HOST_TO_BIG(digit);
                digit = 0;
                hextets_seen++;
                digits_seen = 0;
                colon_allowed = double_colon_allowed;
                just_saw_colon = true;
                another_required = true;
                break;

            case '.':
            {
                /* If we see a period, then we must be in the middle of an IPv4
                 * address at the end of the IPv6 address. */
                struct cork_ipv4  *ipv4 = (struct cork_ipv4 *) dest;
                DEBUG("Detected IPv4 address %s\n", ch-digits_seen);

                /* Ensure that we have space for the two hextets that the IPv4
                 * address will take up. */
                if (CORK_UNLIKELY(hextets_seen >= 7)) {
                    goto parse_error;
                }

                /* Parse the IPv4 address directly into our current hextet
                 * buffer. */
                ch = cork_ipv4_parse(ipv4, ch - digits_seen);
                if (CORK_LIKELY(ch != NULL)) {
                    hextets_seen += 2;
                    digits_seen = 0;
                    another_required = false;

                    /* ch now points at the NUL terminator, but we're about to
                     * increment ch. */
                    ch--;
                    break;
                }

                /* The IPv4 parse failed, so we have an IPv6 parse error. */
                goto parse_error;
            }

            default:
                /* Any other character is a parse error. */
                goto parse_error;
        }
    }

    /* If we have a valid hextet at the end, and we've either seen a
     * double-colon, or we have eight hextets in total, then we've got a valid
     * final parse. */
    DEBUG("%2u:\t", (unsigned int) (ch-str));
    if (CORK_LIKELY(digits_seen > 0)) {
        /* If there are trailing digits that would form a ninth hextet
         * (regardless of the placement of a double-colon), then we have a parse
         * error. */
        if (CORK_UNLIKELY(hextets_seen == 8)) {
            goto parse_error;
        }

        DEBUG("hextet %u = %04x\n\t", hextets_seen, digit);
        *(dest++) = CORK_UINT16_HOST_TO_BIG(digit);
        hextets_seen++;
    } else if (CORK_UNLIKELY(another_required)) {
        goto parse_error;
    }

    if (!double_colon_allowed) {
        /* We've seen a double-colon, so use 0000 for any hextets that weren't
         * present. */
#if CORK_IP_ADDRESS_DEBUG
        char  parsed_result[CORK_IPV6_STRING_LENGTH];
#endif
        unsigned int  after_count = hextets_seen - before_count;
        DEBUG("Saw double-colon; %u hextets before, %u after\n",
              before_count, after_count);
        memset(addr, 0, sizeof(struct cork_ipv6));
        memcpy(addr, before_double_colon,
               sizeof(uint16_t) * before_count);
        memcpy(&addr->_.u16[8-after_count], after_double_colon,
               sizeof(uint16_t) * after_count);
#if CORK_IP_ADDRESS_DEBUG
        cork_ipv6_to_raw_string(addr, parsed_result);
        DEBUG("\tParsed address: %s\n", parsed_result);
#endif
        return 0;
    } else if (hextets_seen == 8) {
        /* No double-colon, so we must have exactly eight hextets. */
#if CORK_IP_ADDRESS_DEBUG
        char  parsed_result[CORK_IPV6_STRING_LENGTH];
#endif
        DEBUG("No double-colon\n");
        cork_ipv6_copy(addr, before_double_colon);
#if CORK_IP_ADDRESS_DEBUG
        cork_ipv6_to_raw_string(addr, parsed_result);
        DEBUG("\tParsed address: %s\n", parsed_result);
#endif
        return 0;
    }

parse_error:
    DEBUG("parse error\n");
    cork_parse_error("Invalid IPv6 address: \"%s\"", str);
    return -1;
}

bool
cork_ipv6_equal_(const struct cork_ipv6 *addr1, const struct cork_ipv6 *addr2)
{
    return cork_ipv6_equal(addr1, addr2);
}

#define NS_IN6ADDRSZ 16
#define NS_INT16SZ 2

void
cork_ipv6_to_raw_string(const struct cork_ipv6 *addr, char *dest)
{
    const uint8_t  *src = addr->_.u8;

    /*
     * Note that int32_t and int16_t need only be "at least" large enough
     * to contain a value of the specified size.  On some systems, like
     * Crays, there is no such thing as an integer variable with 16 bits.
     * Keep this in mind if you think this function should have been coded
     * to use pointer overlays.  All the world's not a VAX.
     */
    char *tp;
    struct { int base, len; } best, cur;
    unsigned int words[NS_IN6ADDRSZ / NS_INT16SZ];
    int i;

    /*
     * Preprocess:
     *      Copy the input (bytewise) array into a wordwise array.
     *      Find the longest run of 0x00's in src[] for :: shorthanding.
     */
    memset(words, '\0', sizeof words);
    for (i = 0; i < NS_IN6ADDRSZ; i++)
        words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
    best.base = -1;
    best.len = 0;
    cur.base = -1;
    cur.len = 0;
    for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
        if (words[i] == 0) {
            if (cur.base == -1)
                cur.base = i, cur.len = 1;
            else
                cur.len++;
        } else {
            if (cur.base != -1) {
                if (best.base == -1 || cur.len > best.len)
                    best = cur;
                cur.base = -1;
            }
        }
    }
    if (cur.base != -1) {
        if (best.base == -1 || cur.len > best.len)
            best = cur;
    }
    if (best.base != -1 && best.len < 2)
        best.base = -1;

    /*
     * Format the result.
     */
    tp = dest;
    for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
        /* Are we inside the best run of 0x00's? */
        if (best.base != -1 && i >= best.base &&
            i < (best.base + best.len)) {
            if (i == best.base)
                *tp++ = ':';
            continue;
        }
        /* Are we following an initial run of 0x00s or any real hex? */
        if (i != 0)
            *tp++ = ':';
        /* Is this address an encapsulated IPv4? */
        if (i == 6 && best.base == 0 &&
            (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) {
            tp += sprintf(tp, "%u.%u.%u.%u",
                          src[12], src[13], src[14], src[15]);
            break;
        }
        tp += sprintf(tp, "%x", words[i]);
    }
    /* Was it a trailing run of 0x00's? */
    if (best.base != -1 && (best.base + best.len) ==
        (NS_IN6ADDRSZ / NS_INT16SZ))
        *tp++ = ':';
    *tp++ = '\0';
}

bool
cork_ipv6_is_valid_network(const struct cork_ipv6 *addr,
                           unsigned int cidr_prefix)
{
    uint64_t  cidr_mask[2];

    if (cidr_prefix > 128) {
        return false;
    } else if (cidr_prefix == 128) {
        /* This handles undefined behavior for overflow bit shifts. */
        cidr_mask[0] = cidr_mask[1] = 0;
    } else if (cidr_prefix == 64) {
        /* This handles undefined behavior for overflow bit shifts. */
        cidr_mask[0] = 0;
        cidr_mask[1] = UINT64_C(0xffffffffffffffff);
    } else if (cidr_prefix > 64) {
        cidr_mask[0] = 0;
        cidr_mask[1] = UINT64_C(0xffffffffffffffff) >> (cidr_prefix-64);
    } else {
        cidr_mask[0] = UINT64_C(0xffffffffffffffff) >> cidr_prefix;
        cidr_mask[1] = UINT64_C(0xffffffffffffffff);
    }

    return (CORK_UINT64_BIG_TO_HOST(addr->_.u64[0] & cidr_mask[0]) == 0) &&
           (CORK_UINT64_BIG_TO_HOST(addr->_.u64[1] & cidr_mask[1]) == 0);
}


/*** IP ***/

void
cork_ip_from_ipv4_(struct cork_ip *addr, const void *src)
{
    cork_ip_from_ipv4(addr, src);
}

void
cork_ip_from_ipv6_(struct cork_ip *addr, const void *src)
{
    cork_ip_from_ipv6(addr, src);
}

int
cork_ip_init(struct cork_ip *addr, const char *str)
{
    int  rc;

    /* Try IPv4 first */
    rc = cork_ipv4_init(&addr->ip.v4, str);
    if (rc == 0) {
        /* successful parse */
        addr->version = 4;
        return 0;
    }

    /* Then try IPv6 */
    cork_error_clear();
    rc = cork_ipv6_init(&addr->ip.v6, str);
    if (rc == 0) {
        /* successful parse */
        addr->version = 6;
        return 0;
    }

    /* Parse error for both address types */
    cork_parse_error("Invalid IP address: \"%s\"", str);
    return -1;
}

bool
cork_ip_equal_(const struct cork_ip *addr1, const struct cork_ip *addr2)
{
    return cork_ip_equal(addr1, addr2);
}

void
cork_ip_to_raw_string(const struct cork_ip *addr, char *dest)
{
    switch (addr->version) {
        case 4:
            cork_ipv4_to_raw_string(&addr->ip.v4, dest);
            return;

        case 6:
            cork_ipv6_to_raw_string(&addr->ip.v6, dest);
            return;

        default:
            strncpy(dest, "<INVALID>", CORK_IP_STRING_LENGTH);
            return;
    }
}

bool
cork_ip_is_valid_network(const struct cork_ip *addr, unsigned int cidr_prefix)
{
    switch (addr->version) {
        case 4:
            return cork_ipv4_is_valid_network(&addr->ip.v4, cidr_prefix);
        case 6:
            return cork_ipv6_is_valid_network(&addr->ip.v6, cidr_prefix);
        default:
            return false;
    }
}

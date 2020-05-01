/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010-2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <string.h>

#include <libcork/core.h>

#include "ipset/bdd/nodes.h"
#include "ipset/bits.h"
#include "ipset/ipset.h"
#include "ipset/logging.h"


#define IPV4_BIT_SIZE  32
#define IPV6_BIT_SIZE  128


/* Forward declarations */

static void
process_assignment(struct ipset_iterator *iterator);

static void
expand_ipv6(struct ipset_iterator *iterator);


/**
 * Find the highest non-EITHER bit in an assignment, starting from the
 * given bit index.
 */
static unsigned int
find_last_non_either_bit(struct ipset_assignment *assignment,
                         unsigned int starting_bit)
{
    unsigned int  i;

    for (i = starting_bit; i >= 1; i--) {
        enum ipset_tribool  value = ipset_assignment_get(assignment, i);
        if (value != IPSET_EITHER) {
            return i;
        }
    }

    return 0;
}


/**
 * Create a generic IP address object from the current expanded
 * assignment.
 */
static void
create_ip_address(struct ipset_iterator *iterator)
{
    struct cork_ip  *addr = &iterator->addr;
    struct ipset_expanded_assignment  *exp = iterator->assignment_iterator;

    /* Initialize the address to all 0 bits. */
    memset(addr, 0, sizeof(struct cork_ip));

    /* Check variable 0 to see if this is an IPv4 or IPv6 address. */
    addr->version = IPSET_BIT_GET(exp->values.buf, 0)? 4: 6;

    /* Copy bits from the expanded assignment.  The number of bits to
     * copy is given as the current netmask.  We'll have calculated that
     * already based on the non-expanded assignment. */
    unsigned int  i;
    for (i = 0; i < iterator->cidr_prefix; i++) {
        IPSET_BIT_SET(&addr->ip, i, IPSET_BIT_GET(exp->values.buf, i+1));
    }

#if IPSET_DEBUG
    char  buf[CORK_IP_STRING_LENGTH];
    cork_ip_to_raw_string(addr, buf);
    DEBUG("Current IP address is %s/%u", buf, iterator->cidr_prefix);
#endif
}


/**
 * Advance the BDD iterator, taking into account that some assignments
 * need to be expanded twice.
 */
static void
advance_assignment(struct ipset_iterator *iterator)
{
    /* Check the current state of the iterator to determine how to
     * advance. */

    /* In most cases, the assignment we just finished only needed to be
     * expanded once.  So we move on to the next assignment and process
     * it. */
    if (CORK_LIKELY(iterator->multiple_expansion_state ==
                    IPSET_ITERATOR_NORMAL))
    {
        ipset_bdd_iterator_advance(iterator->bdd_iterator);
        process_assignment(iterator);
        return;
    }

    /* If the assignment needs to be expanded twice, we'll do the IPv4
     * expansion first.  If that's what we've just finished, do the IPv6
     * expansion next. */

    if (iterator->multiple_expansion_state == IPSET_ITERATOR_MULTIPLE_IPV4) {
        DEBUG("Expanding IPv6 second");

        iterator->multiple_expansion_state = IPSET_ITERATOR_MULTIPLE_IPV6;
        ipset_assignment_set
            (iterator->bdd_iterator->assignment, 0, IPSET_FALSE);
        expand_ipv6(iterator);
        return;
    }

    /* If we've just finished the IPv6 expansion, then we've finished
     * with this assignment.  Before moving on to the next one, we have
     * to reset variable 0 to EITHER (which it was before we started
     * this whole mess). */

    if (iterator->multiple_expansion_state == IPSET_ITERATOR_MULTIPLE_IPV6) {
        DEBUG("Finished both expansions");

        ipset_assignment_set
            (iterator->bdd_iterator->assignment, 0, IPSET_EITHER);
        ipset_bdd_iterator_advance(iterator->bdd_iterator);
        process_assignment(iterator);
        return;
    }
}


/**
 * Process the current expanded assignment in the current BDD
 * assignment.
 */
static void
process_expanded_assignment(struct ipset_iterator *iterator)
{
    if (iterator->assignment_iterator->finished) {
        /* If there isn't anything in the expanded assignment, advance
         * to the next BDD assignment. */

        DEBUG("Expanded assignment is finished");
        ipset_expanded_assignment_free(iterator->assignment_iterator);
        iterator->assignment_iterator = NULL;
        advance_assignment(iterator);
    } else {
        /* Otherwise, we've found a fully expanded assignment, so create
         * an IP address for it and return. */
        create_ip_address(iterator);
    }
}


/**
 * Expand the current assignment as IPv4 addresses.
 */
static void
expand_ipv4(struct ipset_iterator *iterator)
{
    unsigned int  last_bit;

    if (iterator->summarize) {
        last_bit = find_last_non_either_bit
            (iterator->bdd_iterator->assignment, IPV4_BIT_SIZE);
        DEBUG("Last non-either bit is %u", last_bit);
    } else {
        last_bit = IPV4_BIT_SIZE;
    }

    iterator->assignment_iterator =
        ipset_assignment_expand
        (iterator->bdd_iterator->assignment, last_bit + 1);
    iterator->cidr_prefix = last_bit;

    process_expanded_assignment(iterator);
}


/**
 * Expand the current assignment as IPv4 addresses.
 */
static void
expand_ipv6(struct ipset_iterator *iterator)
{
    unsigned int  last_bit;

    if (iterator->summarize) {
        last_bit = find_last_non_either_bit
            (iterator->bdd_iterator->assignment, IPV6_BIT_SIZE);
        DEBUG("Last non-either bit is %u", last_bit);
    } else {
        last_bit = IPV6_BIT_SIZE;
    }

    iterator->assignment_iterator =
        ipset_assignment_expand
        (iterator->bdd_iterator->assignment, last_bit + 1);
    iterator->cidr_prefix = last_bit;

    process_expanded_assignment(iterator);
}


/**
 * Process the current assignment in the BDD iterator.
 */

static void
process_assignment(struct ipset_iterator *iterator)
{
    while (!iterator->bdd_iterator->finished) {
        if (iterator->bdd_iterator->value == iterator->desired_value) {
            /* If the BDD iterator hasn't finished, and the result of
             * the function with this assignment matches what the caller
             * wants, then we've found an assignment to generate IP
             * addresses from.
             *
             * Try to expand this assignment, and process the first
             * expanded assignment.  We want 32 + 1 variables if the
             * current address is IPv4; 128 + 1 if it's IPv6. */

            DEBUG("Got a matching BDD assignment");
            enum ipset_tribool  address_type = ipset_assignment_get
                (iterator->bdd_iterator->assignment, 0);

            if (address_type == IPSET_FALSE) {
                /* FALSE means IPv6*/
                DEBUG("Assignment is IPv6");
                iterator->multiple_expansion_state = IPSET_ITERATOR_NORMAL;
                expand_ipv6(iterator);
                return;
            } else if (address_type == IPSET_TRUE) {
                /* TRUE means IPv4*/
                DEBUG("Assignment is IPv4");
                iterator->multiple_expansion_state = IPSET_ITERATOR_NORMAL;
                expand_ipv4(iterator);
                return;
            } else {
                /* EITHER means that this assignment contains both IPv4
                 * and IPv6 addresses.  Expand it as IPv4 first. */
                DEBUG("Assignment is both IPv4 and IPv6");
                DEBUG("Expanding IPv4 first");
                iterator->multiple_expansion_state =
                    IPSET_ITERATOR_MULTIPLE_IPV4;
                ipset_assignment_set
                    (iterator->bdd_iterator->assignment, 0, IPSET_TRUE);
                expand_ipv4(iterator);
                return;
            }
        }

        /* The BDD iterator has a value, but it doesn't match the one we
         * want.  Advance the BDD iterator and try again. */
        DEBUG("Value is %d, skipping", iterator->bdd_iterator->value);
        ipset_bdd_iterator_advance(iterator->bdd_iterator);
    }

    /* If we fall through, then the BDD iterator has finished.  That
     * means there's nothing left for the set iterator. */

    DEBUG("Set iterator is finished");
    ipset_expanded_assignment_free(iterator->assignment_iterator);
    iterator->assignment_iterator = NULL;

    ipset_bdd_iterator_free(iterator->bdd_iterator);
    iterator->bdd_iterator = NULL;
    iterator->finished = true;
}


static struct ipset_iterator *
create_iterator(struct ip_set *set, bool desired_value, bool summarize)
{
    /* First allocate the iterator itself. */
    struct ipset_iterator  *iterator = cork_new(struct ipset_iterator);
    iterator->finished = false;
    iterator->assignment_iterator = NULL;
    iterator->desired_value = desired_value;
    iterator->summarize = summarize;

    /* Then create the iterator that returns each BDD assignment. */
    DEBUG("Iterating set");
    iterator->bdd_iterator = ipset_node_iterate(set->cache, set->set_bdd);

    /* Then drill down from the current BDD assignment, creating an
     * expanded assignment for it. */
    process_assignment(iterator);
    return iterator;
}


struct ipset_iterator *
ipset_iterate(struct ip_set *set, bool desired_value)
{
    return create_iterator(set, desired_value, false);
}


struct ipset_iterator *
ipset_iterate_networks(struct ip_set *set, bool desired_value)
{
    return create_iterator(set, desired_value, true);
}


void
ipset_iterator_free(struct ipset_iterator *iterator)
{
    if (iterator->bdd_iterator != NULL) {
        ipset_bdd_iterator_free(iterator->bdd_iterator);
    }
    if (iterator->assignment_iterator != NULL) {
        ipset_expanded_assignment_free(iterator->assignment_iterator);
    }
    free(iterator);
}


void
ipset_iterator_advance(struct ipset_iterator *iterator)
{
    /* If we're already at the end of the iterator, don't do anything. */

    if (CORK_UNLIKELY(iterator->finished)) {
        return;
    }

    /* Otherwise, advance the expanded assignment iterator to the next
     * assignment, and then drill down into it. */

    DEBUG("Advancing set iterator");
    ipset_expanded_assignment_advance(iterator->assignment_iterator);
    process_expanded_assignment(iterator);
}

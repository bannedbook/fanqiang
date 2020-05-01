/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010-2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <libcork/core.h>

#include "ipset/bdd/nodes.h"
#include "ipset/logging.h"


/**
 * Add the given node ID to the node stack, and trace down from it
 * until we find a terminal node.  Assign values to the variables for
 * each nonterminal that encounter along the way.  We check low edges
 * first, so each new variable we encounter will be assigned FALSE.
 * (The high edges will be checked eventually by a call to the
 * ipset_bdd_iterator_advance() function.)
 */
static void
add_node(struct ipset_bdd_iterator *iterator, ipset_node_id node_id)
{
    /* Keep tracing down low edges until we reach a terminal. */
    while (ipset_node_get_type(node_id) == IPSET_NONTERMINAL_NODE) {
        /* Add this nonterminal node to the stack, and trace down
         * further into the tree.  We check low edges first, so set the
         * node's variable to FALSE in the assignment. */
        struct ipset_node  *node =
            ipset_node_cache_get_nonterminal(iterator->cache, node_id);

        cork_array_append(&iterator->stack, node_id);
        ipset_assignment_set(iterator->assignment, node->variable, false);

        node_id = node->low;
    }

    /* Once we find a terminal node, save it away in the iterator result
     * and return. */
    iterator->value = ipset_terminal_value(node_id);
}


struct ipset_bdd_iterator *
ipset_node_iterate(struct ipset_node_cache *cache, ipset_node_id root)
{
    /* First allocate the iterator itself, and all of its contained
     * fields. */

    struct ipset_bdd_iterator  *iterator =
        cork_new(struct ipset_bdd_iterator);
    iterator->finished = false;
    iterator->cache = cache;
    cork_array_init(&iterator->stack);
    iterator->assignment = ipset_assignment_new();

    /* Then add the root node to the iterator, tracing down until we
     * find the first terminal node. */
    add_node(iterator, root);
    return iterator;
}


void
ipset_bdd_iterator_free(struct ipset_bdd_iterator *iterator)
{
    cork_array_done(&iterator->stack);
    ipset_assignment_free(iterator->assignment);
    free(iterator);
}


void
ipset_bdd_iterator_advance(struct ipset_bdd_iterator *iterator)
{
    /* If we're already at the end of the iterator, don't do anything. */
    if (CORK_UNLIKELY(iterator->finished)) {
        return;
    }

    /* We look at the last node in the stack.  If it's currently
     * assigned a false value, then we track down its true branch.  If
     * it's got a true branch, then we pop it off and check the next to
     * last node. */

    DEBUG("Advancing BDD iterator");

    while (cork_array_size(&iterator->stack) > 0) {
        ipset_node_id  last_node_id =
            cork_array_at
            (&iterator->stack, cork_array_size(&iterator->stack) - 1);

        struct ipset_node  *last_node =
            ipset_node_cache_get_nonterminal(iterator->cache, last_node_id);

        enum ipset_tribool  current_value =
            ipset_assignment_get(iterator->assignment, last_node->variable);

        /* The current value can't be EITHER, because we definitely
         * assign a TRUE or FALSE to the variables of the nodes that we
         * encounter. */
        if (current_value == IPSET_TRUE) {
            /* We've checked both outgoing edges for this node, so pop
             * it off and look at its parent. */
            iterator->stack.size--;

            /* Before continuing, reset this node's variable to
             * indeterminate in the assignment. */
            ipset_assignment_set
                (iterator->assignment, last_node->variable, IPSET_EITHER);
        } else {
            /* We've checked this node's low edge, but not its high
             * edge.  Set the variable to TRUE in the assignment, and
             * add the high edge's node to the node stack. */
            ipset_assignment_set
                (iterator->assignment, last_node->variable, IPSET_TRUE);
            add_node(iterator, last_node->high);
            return;
        }
    }

    /* If we fall through then we ran out of nodes to check.  That means
     * the iterator is done! */
    iterator->finished = true;
}

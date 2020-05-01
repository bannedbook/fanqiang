/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010-2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>

#include <libcork/core.h>

#include "ipset/bdd/nodes.h"
#include "ipset/bits.h"
#include "ipset/logging.h"


void
ipset_node_fprint(FILE *stream, struct ipset_node *node)
{
    fprintf(stream,
            "nonterminal(x%u? " IPSET_NODE_ID_FORMAT
            ": " IPSET_NODE_ID_FORMAT ")",
            node->variable,
            IPSET_NODE_ID_VALUES(node->high),
            IPSET_NODE_ID_VALUES(node->low));
}


static cork_hash
ipset_node_hash(void *user_data, const void *key)
{
    const struct ipset_node  *node = key;
    /* Hash of "ipset_node" */
    cork_hash  hash = 0xf3b7dc44;
    hash = cork_hash_variable(hash, node->variable);
    hash = cork_hash_variable(hash, node->low);
    hash = cork_hash_variable(hash, node->high);
    return hash;
}

static bool
ipset_node_equals(void *user_data, const void *key1, const void *key2)
{
    const struct ipset_node  *node1 = key1;
    const struct ipset_node  *node2 = key2;

    if (node1 == node2) {
        return true;
    }

    return
        (node1->variable == node2->variable) &&
        (node1->low == node2->low) &&
        (node1->high == node2->high);
}


/* The free list in an ipset_node_cache is represented by a
 * singly-linked list of indices into the chunk array.  Since the
 * ipset_node instance is unused for nodes in the free list, we reuse
 * the refcount field to store the "next" index. */

#define IPSET_NULL_INDEX ((ipset_variable) -1)

struct ipset_node_cache *
ipset_node_cache_new()
{
    struct ipset_node_cache  *cache = cork_new(struct ipset_node_cache);
    cork_array_init(&cache->chunks);
    cache->largest_index = 0;
    cache->free_list = IPSET_NULL_INDEX;
    cache->node_cache = cork_hash_table_new(0, 0);
    cork_hash_table_set_hash
        (cache->node_cache, (cork_hash_f) ipset_node_hash);
    cork_hash_table_set_equals
        (cache->node_cache, (cork_equals_f) ipset_node_equals);
    return cache;
}

void
ipset_node_cache_free(struct ipset_node_cache *cache)
{
    size_t  i;
    for (i = 0; i < cork_array_size(&cache->chunks); i++) {
        free(cork_array_at(&cache->chunks, i));
    }
    cork_array_done(&cache->chunks);
    cork_hash_table_free(cache->node_cache);
    free(cache);
}


/**
 * Returns the index of a new ipset_node instance.
 */
static ipset_value
ipset_node_cache_alloc_node(struct ipset_node_cache *cache)
{
    if (cache->free_list == IPSET_NULL_INDEX) {
        /* Nothing in the free list; need to allocate a new node. */
        ipset_value  next_index = cache->largest_index++;
        ipset_value  chunk_index = next_index >> IPSET_BDD_NODE_CACHE_BIT_SIZE;
        if (chunk_index >= cork_array_size(&cache->chunks)) {
            /* We've filled up all of the existing chunks, and need to
             * create a new one. */
            DEBUG("        (allocating chunk %zu)",
                  cork_array_size(&cache->chunks));
            struct ipset_node  *new_chunk = cork_calloc
                (IPSET_BDD_NODE_CACHE_SIZE, sizeof(struct ipset_node));
            cork_array_append(&cache->chunks, new_chunk);
        }
        return next_index;
    } else {
        /* Reuse a recently freed node. */
        ipset_value  next_index = cache->free_list;
        struct ipset_node  *node =
            ipset_node_cache_get_nonterminal_by_index(cache, next_index);
        cache->free_list = node->refcount;
        return next_index;
    }
}

ipset_node_id
ipset_node_incref(struct ipset_node_cache *cache, ipset_node_id node_id)
{
    if (ipset_node_get_type(node_id) == IPSET_NONTERMINAL_NODE) {
        struct ipset_node  *node =
            ipset_node_cache_get_nonterminal(cache, node_id);
        DEBUG("        [incref " IPSET_NODE_ID_FORMAT "]",
              IPSET_NODE_ID_VALUES(node_id));
        node->refcount++;
    }
    return node_id;
}

void
ipset_node_decref(struct ipset_node_cache *cache, ipset_node_id node_id)
{
    if (ipset_node_get_type(node_id) == IPSET_NONTERMINAL_NODE) {
        struct ipset_node  *node =
            ipset_node_cache_get_nonterminal(cache, node_id);
        DEBUG("        [decref " IPSET_NODE_ID_FORMAT "]",
              IPSET_NODE_ID_VALUES(node_id));
        if (--node->refcount == 0) {
            DEBUG("        [free   " IPSET_NODE_ID_FORMAT "]",
                  IPSET_NODE_ID_VALUES(node_id));
            ipset_node_decref(cache, node->low);
            ipset_node_decref(cache, node->high);
            cork_hash_table_delete(cache->node_cache, node, NULL, NULL);

            /* Add the node to the free list */
            node->refcount = cache->free_list;
            cache->free_list = ipset_nonterminal_value(node_id);
        }
    }
}

bool
ipset_node_cache_nodes_equal(const struct ipset_node_cache *cache1,
                             ipset_node_id node_id1,
                             const struct ipset_node_cache *cache2,
                             ipset_node_id node_id2)
{
    struct ipset_node  *node1;
    struct ipset_node  *node2;

    if (ipset_node_get_type(node_id1) != ipset_node_get_type(node_id2)) {
        return false;
    }

    if (ipset_node_get_type(node_id1) == IPSET_TERMINAL_NODE) {
        return node_id1 == node_id2;
    }

    node1 = ipset_node_cache_get_nonterminal(cache1, node_id1);
    node2 = ipset_node_cache_get_nonterminal(cache2, node_id2);
    return
        (node1->variable == node2->variable) &&
        ipset_node_cache_nodes_equal(cache1, node1->low, cache2, node2->low) &&
        ipset_node_cache_nodes_equal(cache1, node1->high, cache2, node2->high);
}

ipset_node_id
ipset_node_cache_nonterminal(struct ipset_node_cache *cache,
                             ipset_variable variable,
                             ipset_node_id low, ipset_node_id high)
{
    /* Don't allow any nonterminals whose low and high subtrees are the
     * same, since the nonterminal would be redundant. */
    if (CORK_UNLIKELY(low == high)) {
        DEBUG("        [ SKIP  nonterminal(x%u? "
              IPSET_NODE_ID_FORMAT ": " IPSET_NODE_ID_FORMAT ")]",
              variable, IPSET_NODE_ID_VALUES(high), IPSET_NODE_ID_VALUES(low));
        ipset_node_decref(cache, high);
        return low;
    }

    /* Check to see if there's already a nonterminal with these contents
     * in the cache. */
    DEBUG("        [search nonterminal(x%u? "
          IPSET_NODE_ID_FORMAT ": " IPSET_NODE_ID_FORMAT ")]",
          variable, IPSET_NODE_ID_VALUES(high), IPSET_NODE_ID_VALUES(low));

    struct ipset_node  search_node;
    search_node.variable = variable;
    search_node.low = low;
    search_node.high = high;

    bool  is_new;
    struct cork_hash_table_entry  *entry =
        cork_hash_table_get_or_create
        (cache->node_cache, &search_node, &is_new);

    if (!is_new) {
        /* There's already a node with these contents, so return its ID. */
        ipset_node_id  node_id = (uintptr_t) entry->value;
        DEBUG("        [reuse  " IPSET_NODE_ID_FORMAT "]",
              IPSET_NODE_ID_VALUES(node_id));
        ipset_node_incref(cache, node_id);
        ipset_node_decref(cache, low);
        ipset_node_decref(cache, high);
        return node_id;
    } else {
        /* This node doesn't exist yet.  Allocate a permanent copy of
         * the node, add it to the cache, and then return its ID. */
        ipset_value  new_index = ipset_node_cache_alloc_node(cache);
        ipset_node_id  new_node_id = ipset_nonterminal_node_id(new_index);
        struct ipset_node  *real_node =
            ipset_node_cache_get_nonterminal_by_index(cache, new_index);
        real_node->refcount = 1;
        real_node->variable = variable;
        real_node->low = low;
        real_node->high = high;
        entry->key = real_node;
        entry->value = (void *) (uintptr_t) new_node_id;
        DEBUG("        [new    " IPSET_NODE_ID_FORMAT "]",
              IPSET_NODE_ID_VALUES(new_node_id));
        return new_node_id;
    }
}


bool
ipset_bool_array_assignment(const void *user_data, ipset_variable variable)
{
    const bool  *bool_array = (const bool *) user_data;
    return bool_array[variable];
}


bool
ipset_bit_array_assignment(const void *user_data, ipset_variable variable)
{
    return IPSET_BIT_GET(user_data, variable);
}


ipset_value
ipset_node_evaluate(const struct ipset_node_cache *cache, ipset_node_id node_id,
                    ipset_assignment_func assignment, const void *user_data)
{
    ipset_node_id  curr_node_id = node_id;
    DEBUG("Evaluating BDD node " IPSET_NODE_ID_FORMAT,
          IPSET_NODE_ID_VALUES(node_id));

    /* As long as the current node is a nonterminal, we have to check
     * the value of the current variable. */
    while (ipset_node_get_type(curr_node_id) == IPSET_NONTERMINAL_NODE) {
        /* We have to look up this variable in the assignment. */
        struct ipset_node  *node =
            ipset_node_cache_get_nonterminal(cache, curr_node_id);
        bool  this_value = assignment(user_data, node->variable);
        DEBUG("[%3u] Nonterminal " IPSET_NODE_ID_FORMAT,
              node->variable, IPSET_NODE_ID_VALUES(curr_node_id));
        DEBUG("[%3u]   x%u = %s",
              node->variable, node->variable, this_value? "TRUE": "FALSE");

        if (this_value) {
            /* This node's variable is true in the assignment vector, so
             * trace down the high subtree. */
            curr_node_id = node->high;
        } else {
            /* This node's variable is false in the assignment vector,
             * so trace down the low subtree. */
            curr_node_id = node->low;
        }
    }

    /* Once we find a terminal node, we've got the final result. */
    DEBUG("Evaluated result is %u", ipset_terminal_value(curr_node_id));
    return ipset_terminal_value(curr_node_id);
}


/* A “fake” BDD node given by an assignment. */
struct ipset_fake_node {
    ipset_variable  current_var;
    ipset_variable  var_count;
    ipset_assignment_func  assignment;
    const void  *user_data;
    ipset_value  value;
};

/* A fake BDD node representing the terminal 0 value. */
static struct ipset_fake_node  fake_terminal_0 = { 0, 0, NULL, 0, 0 };

/* We set elements in a map using the if-then-else (ITE) operator:
 *
 *   new_set = new_element? new_value: old_set
 *
 * The below is a straight copy of the standard trinary APPLY from the BDD
 * literature, but without the caching of the results.  And also with the
 * wrinkle that the F argument to ITE (i.e., new_element) is given by an
 * assignment, and not by a BDD node.  (This lets us skip constructing the BDD
 * for the assignment, saving us a few cycles.)
 */

static ipset_node_id
ipset_apply_ite(struct ipset_node_cache *cache, struct ipset_fake_node *f,
                ipset_value g, ipset_node_id h)
{
    ipset_node_id  h_low;
    ipset_node_id  h_high;
    ipset_node_id  result_low;
    ipset_node_id  result_high;

    /* If F is a terminal, then we're in one of the following two
     * cases:
     *
     *   1? G: H == G
     *   0? G: H == H
     */
    if (f->current_var == f->var_count) {
        ipset_node_id  result;
        DEBUG("[%3u] F is terminal (value %u)", f->current_var, f->value);

        if (f->value == 0) {
            DEBUG("[%3u] 0? " IPSET_NODE_ID_FORMAT ": " IPSET_NODE_ID_FORMAT
                  " = " IPSET_NODE_ID_FORMAT,
                  f->current_var,
                  IPSET_NODE_ID_VALUES(ipset_terminal_node_id(g)),
                  IPSET_NODE_ID_VALUES(h), IPSET_NODE_ID_VALUES(h));
            result = ipset_node_incref(cache, h);
        } else {
            result = ipset_terminal_node_id(g);
            DEBUG("[%3u] 1? " IPSET_NODE_ID_FORMAT ": " IPSET_NODE_ID_FORMAT
                  " = " IPSET_NODE_ID_FORMAT,
                  f->current_var, IPSET_NODE_ID_VALUES(result),
                  IPSET_NODE_ID_VALUES(h), IPSET_NODE_ID_VALUES(result));
        }

        return result;
    }

    /* F? G: G == G */
    if (h == ipset_terminal_node_id(g)) {
        DEBUG("[%3u] F? " IPSET_NODE_ID_FORMAT ": " IPSET_NODE_ID_FORMAT
              " = " IPSET_NODE_ID_FORMAT,
              f->current_var, IPSET_NODE_ID_VALUES(h),
              IPSET_NODE_ID_VALUES(h), IPSET_NODE_ID_VALUES(h));
        return h;
    }

    /* From here to the end of the function, we know that F is a
     * nonterminal. */
    DEBUG("[%3u] F is nonterminal", f->current_var);

    /* We're going to do two recursive calls, a “low” one and a “high” one.  For
     * each nonterminal that has the minimum variable number, we use its low and
     * high pointers in the respective recursive call.  For all other
     * nonterminals, and for all terminals, we use the operand itself. */

    if (ipset_node_get_type(h) == IPSET_NONTERMINAL_NODE) {
        struct ipset_node  *h_node =
            ipset_node_cache_get_nonterminal(cache, h);

        DEBUG("[%3u] H is nonterminal (variable %u)",
              f->current_var, h_node->variable);

        if (h_node->variable < f->current_var) {
            /* var(F) > var(H), so we only recurse down the H branches. */
            DEBUG("[%3u] Recursing only down H", f->current_var);
            DEBUG("[%3u]   Recursing high", f->current_var);
            result_high = ipset_apply_ite(cache, f, g, h_node->high);
            DEBUG("[%3u]   Back from high recursion", f->current_var);
            DEBUG("[%3u]   Recursing low", f->current_var);
            result_low = ipset_apply_ite(cache, f, g, h_node->low);
            DEBUG("[%3u]   Back from low recursion", f->current_var);
            return ipset_node_cache_nonterminal
                (cache, h_node->variable, result_low, result_high);
        } else if (h_node->variable == f->current_var) {
            /* var(F) == var(H), so we recurse down both branches. */
            DEBUG("[%3u] Recursing down both F and H", f->current_var);
            h_low = h_node->low;
            h_high = h_node->high;
        } else {
            /* var(F) < var(H), so we only recurse down the F branches. */
            DEBUG("[%3u] Recursing only down F", f->current_var);
            h_low = h;
            h_high = h;
        }
    } else {
        /* H in nonterminal, so we only recurse down the F branches. */
        DEBUG("[%3u] H is terminal (value %u)",
              f->current_var, ipset_terminal_value(h));
        DEBUG("[%3u] Recursing only down F", f->current_var);
        h_low = h;
        h_high = h;
    }

    /* F is a “fake” nonterminal node, since it comes from our assignment.  One
     * of its branches will be the 0 terminal, and the other will be the fake
     * nonterminal for the next variable in the assignment.  (Which one is low
     * and which one is high depends on the value of the current variable in the
     * assignment.) */

    if (f->assignment(f->user_data, f->current_var)) {
        /* The current variable is set in F.  The low branch is terminal 0; the
         * high branch is the next variable in F. */
        DEBUG("[%3u]   x[%u] is set", f->current_var, f->current_var);
        DEBUG("[%3u]   Recursing high", f->current_var);
        f->current_var++;
        result_high = ipset_apply_ite(cache, f, g, h_high);
        f->current_var--;
        DEBUG("[%3u]   Back from high recursion: " IPSET_NODE_ID_FORMAT,
              f->current_var, IPSET_NODE_ID_VALUES(result_high));
        DEBUG("[%3u]   Recursing low", f->current_var);
        fake_terminal_0.current_var = f->var_count;
        fake_terminal_0.var_count = f->var_count;
        result_low = ipset_apply_ite(cache, &fake_terminal_0, g, h_low);
        DEBUG("[%3u]   Back from low recursion: " IPSET_NODE_ID_FORMAT,
              f->current_var, IPSET_NODE_ID_VALUES(result_low));
    } else {
        /* The current variable is NOT set in F.  The high branch is terminal 0;
         * the low branch is the next variable in F. */
        DEBUG("[%3u]   x[%u] is NOT set", f->current_var, f->current_var);
        DEBUG("[%3u]   Recursing high", f->current_var);
        fake_terminal_0.current_var = f->var_count;
        fake_terminal_0.var_count = f->var_count;
        result_high = ipset_apply_ite(cache, &fake_terminal_0, g, h_high);
        DEBUG("[%3u]   Back from high recursion: " IPSET_NODE_ID_FORMAT,
              f->current_var, IPSET_NODE_ID_VALUES(result_high));
        DEBUG("[%3u]   Recursing low", f->current_var);
        f->current_var++;
        result_low = ipset_apply_ite(cache, f, g, h_low);
        f->current_var--;
        DEBUG("[%3u]   Back from low recursion: " IPSET_NODE_ID_FORMAT,
              f->current_var, IPSET_NODE_ID_VALUES(result_low));
    }

    return ipset_node_cache_nonterminal
        (cache, f->current_var, result_low, result_high);
}

ipset_node_id
ipset_node_insert(struct ipset_node_cache *cache, ipset_node_id node,
                  ipset_assignment_func assignment, const void *user_data,
                  ipset_variable var_count, ipset_value value)
{
    struct ipset_fake_node  f = { 0, var_count, assignment, user_data, 1 };
    DEBUG("Inserting new element");
    return ipset_apply_ite(cache, &f, value, node);
}

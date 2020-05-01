/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010-2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef IPSET_BDD_NODES_H
#define IPSET_BDD_NODES_H


#include <stdio.h>

#include <libcork/core.h>
#include <libcork/ds.h>


/*-----------------------------------------------------------------------
 * Preliminaries
 */

/**
 * Each variable in a BDD is referred to by number.
 */
typedef unsigned int  ipset_variable;


/**
 * Each BDD terminal represents an integer value.  The integer must be
 * non-negative, but must be within the range of the <i>signed</i>
 * integer type.
 */
typedef unsigned int  ipset_value;


/**
 * An identifier for each distinct node in a BDD.
 *
 * Internal implementation note.  Since pointers are aligned to at
 * least two bytes, the ID of a terminal node has its LSB set to 1,
 * and has the terminal value stored in the remaining bits.  The ID of
 * a nonterminal node is simply a pointer to the node struct.
 */
typedef unsigned int  ipset_node_id;


/**
 * Nodes can either be terminal or nonterminal.
 */
enum ipset_node_type {
    IPSET_NONTERMINAL_NODE = 0,
    IPSET_TERMINAL_NODE = 1
};


/**
 * Return the type of node represented by a particular node ID.
 */
#define ipset_node_get_type(node_id)  ((node_id) & 0x01)

#define IPSET_NODE_ID_FORMAT  "%s%u"
#define IPSET_NODE_ID_VALUES(node_id) \
    (ipset_node_get_type((node_id)) == IPSET_NONTERMINAL_NODE? "s": ""), \
    ((node_id) >> 1)


/*-----------------------------------------------------------------------
 * Terminal nodes
 */

/**
 * Return the value of a terminal node.  The result is undefined if
 * the node ID represents a nonterminal.
 */
#define ipset_terminal_value(node_id)  ((node_id) >> 1)

/**
 * Creates a terminal node ID from a terminal value.
 */
#define ipset_terminal_node_id(value) \
    (((value) << 1) | IPSET_TERMINAL_NODE)


/*-----------------------------------------------------------------------
 * Nonterminal nodes
 */

/**
 * A nonterminal BDD node.  This is an inner node of the BDD tree.
 * The node represents one variable in an overall variable assignment.
 * The node has two children: a “low” child and a “high” child.  The
 * low child is the subtree that applies when the node's variable is
 * false or 0; the high child is the subtree that applies when it's
 * true or 1.
 *
 * This type does not take care of ensuring that all BDD nodes are
 * reduced; that is handled by the node_cache class.
 */
struct ipset_node {
    /** The reference count for this node. */
    unsigned int  refcount;
    /** The variable that this node represents. */
    ipset_variable  variable;
    /** The subtree node for when the variable is false. */
    ipset_node_id  low;
    /** The subtree node for when the variable is true. */
    ipset_node_id  high;
};

/**
 * Return the "value" of a nonterminal node.  The value of a nonterminal
 * is the index into the node array of the cache that the node belongs
 * to.
 */
#define ipset_nonterminal_value(node_id) ((node_id) >> 1)

/**
 * Creates a nonterminal node ID from a nonterminal value.
 */
#define ipset_nonterminal_node_id(value) \
    (((value) << 1) | IPSET_NONTERMINAL_NODE)

/**
 * Print out a node object.
 */
void
ipset_node_fprint(FILE *stream, struct ipset_node *node);


/*-----------------------------------------------------------------------
 * Node caches
 */

/**
 * The log2 of the size of each chunk of BDD nodes.
 */
/* 16K elements per cache */
#define IPSET_BDD_NODE_CACHE_BIT_SIZE  6
#define IPSET_BDD_NODE_CACHE_SIZE  (1 << IPSET_BDD_NODE_CACHE_BIT_SIZE)
#define IPSET_BDD_NODE_CACHE_MASK  (IPSET_BDD_NODE_CACHE_SIZE - 1)

/**
 * A cache for BDD nodes.  By creating and retrieving nodes through
 * the cache, we ensure that a BDD is reduced.
 */
struct ipset_node_cache {
    /** The storage for the nodes managed by this cache. */
    cork_array(struct ipset_node *)  chunks;
    /** The largest nonterminal index that has been handed out. */
    ipset_value  largest_index;
    /** The index of the first node in the free list. */
    ipset_value  free_list;
    /** A cache of the nonterminal nodes, keyed by their contents. */
    struct cork_hash_table  *node_cache;
};

/**
 * Returns the index of the chunk that the given nonterminal lives in.
 */
#define ipset_nonterminal_chunk_index(index) \
    ((index) >> IPSET_BDD_NODE_CACHE_BIT_SIZE)

/**
 * Returns the offset of the given nonterminal within its chunk.
 */
#define ipset_nonterminal_chunk_offset(index) \
    ((index) & IPSET_BDD_NODE_CACHE_MASK)

/**
 * Returns a pointer to the ipset_node for a given nonterminal index.
 */
#define ipset_node_cache_get_nonterminal_by_index(cache, index) \
    (&cork_array_at(&(cache)->chunks, ipset_nonterminal_chunk_index((index))) \
     [ipset_nonterminal_chunk_offset((index))])

/**
 * Returns the ipset_node for a given nonterminal node ID.
 */
#define ipset_node_cache_get_nonterminal(cache, node_id) \
    (ipset_node_cache_get_nonterminal_by_index \
     ((cache), ipset_nonterminal_value((node_id))))

/**
 * Create a new node cache.
 */
struct ipset_node_cache *
ipset_node_cache_new(void);

/**
 * Free a node cache.
 */
void
ipset_node_cache_free(struct ipset_node_cache *cache);

/**
 * Create a new nonterminal node with the given contents, returning
 * its ID.  This function ensures that there is only one node with the
 * given contents in this cache.
 *
 * Steals references to low and high.
 */
ipset_node_id
ipset_node_cache_nonterminal(struct ipset_node_cache *cache,
                             ipset_variable variable,
                             ipset_node_id low, ipset_node_id high);


/**
 * Increment the reference count of a nonterminal node.  (This is a
 * no-op for terminal nodes.)
 */
ipset_node_id
ipset_node_incref(struct ipset_node_cache *cache, ipset_node_id node);

/**
 * Decrement the reference count of a nonterminal node.  If the
 * reference count reaches 0, the storage for the node will be
 * reclaimed.  (This is a no-op for terminal nodes.)
 */
void
ipset_node_decref(struct ipset_node_cache *cache, ipset_node_id node);


/**
 * Return the number of nodes that are reachable from the given node.
 * This does not include duplicates if a node is reachable via more
 * than one path.
 */
size_t
ipset_node_reachable_count(const struct ipset_node_cache *cache,
                           ipset_node_id node);


/**
 * Return the amount of memory used by the nodes in the given BDD.
 */
size_t
ipset_node_memory_size(const struct ipset_node_cache *cache,
                       ipset_node_id node);


/**
 * Load a BDD from an input stream.  The error field is filled in with
 * an error condition is the BDD can't be read for any reason.
 */
ipset_node_id
ipset_node_cache_load(FILE *stream, struct ipset_node_cache *cache);


/**
 * Save a BDD to an output stream.  This encodes the set using only
 * those nodes that are reachable from the BDD's root node.
 */
int
ipset_node_cache_save(struct cork_stream_consumer *stream,
                      struct ipset_node_cache *cache, ipset_node_id node);


/**
 * Compare two BDD nodes, possibly from different caches, for equality.
 */
bool
ipset_node_cache_nodes_equal(const struct ipset_node_cache *cache1,
                             ipset_node_id node1,
                             const struct ipset_node_cache *cache2,
                             ipset_node_id node2);


/**
 * Save a GraphViz dot graph for a BDD.  The graph script is written
 * to the given output stream.  This graph only includes those nodes
 * that are reachable from the BDD's root node.
 */
int
ipset_node_cache_save_dot(struct cork_stream_consumer *stream,
                          struct ipset_node_cache *cache, ipset_node_id node);


/*-----------------------------------------------------------------------
 * BDD operators
 */

/**
 * A function that provides the value for each variable in a BDD.
 */
typedef bool
(*ipset_assignment_func)(const void *user_data,
                         ipset_variable variable);

/**
 * An assignment function that gets the variable values from an array
 * of gbooleans.
 */
bool
ipset_bool_array_assignment(const void *user_data,
                            ipset_variable variable);

/**
 * An assignment function that gets the variable values from an array
 * of bits.
 */
bool
ipset_bit_array_assignment(const void *user_data,
                           ipset_variable variable);

/**
 * Evaluate a BDD given a particular assignment of variables.
 */
ipset_value
ipset_node_evaluate(const struct ipset_node_cache *cache, ipset_node_id node,
                    ipset_assignment_func assignment,
                    const void *user_data);

/**
 * Add an assignment to the BDD.
 */
ipset_node_id
ipset_node_insert(struct ipset_node_cache *cache, ipset_node_id node,
                  ipset_assignment_func assignment,
                  const void *user_data, ipset_variable variable_count,
                  ipset_value value);


/*-----------------------------------------------------------------------
 * Variable assignments
 */

/**
 * Each variable in the input to a Boolean function can be true or
 * false; it can also be EITHER, which means that the variable can be
 * either true or false in a particular assignment without affecting
 * the result of the function.
 */
enum ipset_tribool {
    IPSET_FALSE = 0,
    IPSET_TRUE = 1,
    IPSET_EITHER = 2
};


/**
 * An assignment is a mapping of variable numbers to Boolean values.
 * It represents an input to a Boolean function that maps to a
 * particular output value.  Each variable in the input to a Boolean
 * function can be true or false; it can also be EITHER, which means
 * that the variable can be either true or false in a particular
 * assignment without affecting the result of the function.
 */

struct ipset_assignment {
    /**
     * The underlying variable assignments are stored in a vector of
     * tribools.  Every variable that has a true or false value must
     * appear in the vector.  Variables that are EITHER only have to
     * appear to prevent gaps in the vector.  Any variables outside
     * the range of the vector are assumed to be EITHER.
     */
    cork_array(enum ipset_tribool)  values;
};


/**
 * Create a new assignment where all variables are indeterminite.
 */
struct ipset_assignment *
ipset_assignment_new();


/**
 * Free an assignment.
 */
void
ipset_assignment_free(struct ipset_assignment *assignment);


/**
 * Compare two assignments for equality.
 */
bool
ipset_assignment_equal(const struct ipset_assignment *assignment1,
                       const struct ipset_assignment *assignment2);


/**
 * Set the given variable, and all higher variables, to the EITHER
 * value.
 */
void
ipset_assignment_cut(struct ipset_assignment *assignment, ipset_variable var);


/**
 * Clear the assignment, setting all variables to the EITHER value.
 */
void
ipset_assignment_clear(struct ipset_assignment *assignment);


/**
 * Return the value assigned to a particular variable.
 */
enum ipset_tribool
ipset_assignment_get(struct ipset_assignment *assignment, ipset_variable var);


/**
 * Set the value assigned to a particular variable.
 */
void
ipset_assignment_set(struct ipset_assignment *assignment,
                     ipset_variable var, enum ipset_tribool value);


/*-----------------------------------------------------------------------
 * Expanded assignments
 */

/**
 * An iterator for expanding a variable assignment.  For each EITHER
 * variable in the assignment, the iterator yields a result with both
 * values.
 */
struct ipset_expanded_assignment {
    /** Whether there are any more assignments in this iterator. */
    bool finished;

    /**
     * The variable values in the current expanded assignment.  Since
     * there won't be any EITHERs in the expanded assignment, we can
     * use a byte array, and represent each variable by a single bit.
     */
    struct cork_buffer  values;

    /**
     * An array containing all of the variables that are EITHER in the
     * original assignment.
     */
    cork_array(ipset_variable)  eithers;
};


/**
 * Return an iterator that expands a variable assignment.  For each
 * variable that's EITHER in the assignment, the iterator yields a
 * result with both values.  The iterator will ensure that the
 * specified number of variables are given concrete values.
 */
struct ipset_expanded_assignment *
ipset_assignment_expand(const struct ipset_assignment *assignment,
                        ipset_variable var_count);


/**
 * Free an expanded assignment iterator.
 */
void
ipset_expanded_assignment_free(struct ipset_expanded_assignment *exp);


/**
 * Advance the iterator to the next assignment.
 */
void
ipset_expanded_assignment_advance(struct ipset_expanded_assignment *exp);


/*-----------------------------------------------------------------------
 * BDD iterators
 */

/**
 * An iterator for walking through the assignments for a given BDD
 * node.
 *
 * The iterator walks through each path in the BDD tree, stopping at
 * each terminal node.  Each time we reach a terminal node, we yield a
 * new ipset_assignment object representing the assignment of variables
 * along the current path.
 *
 * We maintain a stack of nodes leading to the current terminal, which
 * allows us to backtrack up the path to find the next terminal when
 * we increment the iterator.
 */
struct ipset_bdd_iterator {
    /** Whether there are any more assignments in this iterator. */
    bool finished;

    /** The node cache that we're iterating through. */
    struct ipset_node_cache  *cache;

    /**
     * The sequence of nonterminal nodes leading to the current
     * terminal.
     */
    cork_array(ipset_node_id)  stack;

    /** The current assignment. */
    struct ipset_assignment  *assignment;

    /**
     * The value of the BDD's function when applied to the current
     * assignment.
     */
    ipset_value  value;
};


/**
 * Return an iterator that yields all of the assignments in the given
 * BDD.  The iterator contains two items of interest.  The first is an
 * ipset_assignment providing the value that each variable takes, while
 * the second is the terminal value that is the result of the BDD's
 * function when applied to that variable assignment.
 */
struct ipset_bdd_iterator *
ipset_node_iterate(struct ipset_node_cache *cache, ipset_node_id root);


/**
 * Free a BDD iterator.
 */
void
ipset_bdd_iterator_free(struct ipset_bdd_iterator *iterator);


/**
 * Advance the iterator to the next assignment.
 */
void
ipset_bdd_iterator_advance(struct ipset_bdd_iterator *iterator);


#endif  /* IPSET_BDD_NODES_H */

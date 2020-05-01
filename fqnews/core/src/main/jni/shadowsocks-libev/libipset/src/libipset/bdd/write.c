/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010-2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <libcork/core.h>
#include <libcork/helpers/errors.h>

#include "ipset/bdd/nodes.h"
#include "ipset/logging.h"


/*-----------------------------------------------------------------------
 * Generic saving logic
 */

/**
 * On disk, we use a different node ID scheme than we do in memory.
 * Terminal node IDs are non-negative, and are equal to the terminal
 * value.  Nonterminal node IDs are negative, starting with -1.
 * Nonterminal -1 appears first on disk, then nonterminal -2, and so
 * on.
 */

typedef int  serialized_id;


/* forward declaration */
struct save_data;


/**
 * A callback that outputs any necessary header.  Should return an int
 * status code indicating whether the write was successful.
 */

typedef int
(*write_header_func)(struct save_data *save_data,
                     struct ipset_node_cache *cache,
                     ipset_node_id root);


/**
 * A callback that outputs any necessary footer.  Should return an int
 * status code indicating whether the write was successful.
 */

typedef int
(*write_footer_func)(struct save_data *save_data,
                     struct ipset_node_cache *cache,
                     ipset_node_id root);


/**
 * A callback that actually outputs a terminal node to disk.  Should
 * return an int status code indicating whether the write was successful.
 */

typedef int
(*write_terminal_func)(struct save_data *save_data,
                       ipset_value terminal_value);


/**
 * A callback that actually outputs a nonterminal node to disk.
 * Should return an int status code indicating whether the write was
 * successful.
 */

typedef int
(*write_nonterminal_func)(struct save_data *save_data,
                          serialized_id serialized_node,
                          ipset_variable variable,
                          serialized_id serialized_low,
                          serialized_id serialized_high);


/**
 * A helper struct containing all of the persistent data items needed
 * during the execution of a save.
 */

struct save_data {
    /* The node cache that we're saving nodes from. */
    struct ipset_node_cache  *cache;

    /* The output stream to save the data to. */
    struct cork_stream_consumer  *stream;

    /* The cache of serialized IDs for any nonterminals that we've
     * encountered so far. */
    struct cork_hash_table  *serialized_ids;

    /* The serialized ID to use for the next nonterminal that we
     * encounter. */
    serialized_id  next_serialized_id;

    /* The callback used to write the file header to the stream. */
    write_header_func  write_header;

    /* The callback used to write the file footer to the stream. */
    write_footer_func  write_footer;

    /* The callback used to write terminals to the stream. */
    write_terminal_func  write_terminal;

    /* The callback used to write nonterminals to the stream. */
    write_nonterminal_func  write_nonterminal;

    /* A pointer to any additional data needed by the callbacks. */
    void  *user_data;
};


/**
 * A helper function for ipset_node_save().  Outputs a nonterminal
 * node in a BDD tree, if we haven't done so already.  Ensures that
 * the children of the nonterminal are output before the nonterminal
 * is.  Returns the serialized ID of this node.
 */

static int
save_visit_node(struct save_data *save_data,
                ipset_node_id node_id, serialized_id *dest)
{
    /* Check whether we've already serialized this node. */

    struct cork_hash_table_entry  *entry;
    bool  is_new;
    entry = cork_hash_table_get_or_create
        (save_data->serialized_ids, (void *) (uintptr_t) node_id, &is_new);

    if (!is_new) {
        *dest = (intptr_t) entry->value;
        return 0;
    } else {
        if (ipset_node_get_type(node_id) == IPSET_TERMINAL_NODE) {
            /* For terminals, there isn't really anything to do — we
             * just output the terminal node and use its value as the
             * serialized ID. */

            ipset_value  value = ipset_terminal_value(node_id);

            DEBUG("Writing terminal(%d)", value);
            rii_check(save_data->write_terminal(save_data, value));
            entry->value = (void *) (intptr_t) value;
            *dest = value;
            return 0;
        } else {
            /* For nonterminals, we drill down into the node's children
             * first, then output the nonterminal node. */

            struct ipset_node  *node =
                ipset_node_cache_get_nonterminal(save_data->cache, node_id);
            DEBUG("Visiting node %u nonterminal(x%u? %u: %u)",
                  node_id, node->variable, node->high, node->low);

            /* Output the node's nonterminal children before we output
             * the node itself. */
            serialized_id  serialized_low;
            serialized_id  serialized_high;
            rii_check(save_visit_node(save_data, node->low, &serialized_low));
            rii_check(save_visit_node(save_data, node->high, &serialized_high));

            /* Output the nonterminal */
            serialized_id  result = save_data->next_serialized_id--;
            DEBUG("Writing node %u as serialized node %d = (x%u? %d: %d)",
                  node_id, result,
                  node->variable, serialized_low, serialized_high);

            entry->value = (void *) (intptr_t) result;
            *dest = result;
            return save_data->write_nonterminal
                (save_data, result, node->variable,
                 serialized_low, serialized_high);
        }
    }
}


static int
save_bdd(struct save_data *save_data,
         struct ipset_node_cache *cache, ipset_node_id root)
{
    /* First, output the file header. */

    DEBUG("Writing file header");
    rii_check(save_data->write_header(save_data, cache, root));

    /* The serialized node IDs are different than the in-memory node
     * IDs.  This means that, for our nonterminal nodes, we need a
     * mapping from internal node ID to serialized node ID. */

    DEBUG("Creating file caches");
    save_data->serialized_ids = cork_pointer_hash_table_new(0, 0);
    save_data->next_serialized_id = -1;

    /* Trace down through the BDD tree, outputting each terminal and
     * nonterminal node as they're encountered. */

    DEBUG("Writing nodes");

    serialized_id  last_serialized_id;
    ei_check(save_visit_node(save_data, root, &last_serialized_id));

    /* Finally, output the file footer and cleanup. */

    DEBUG("Writing file footer");
    ei_check(save_data->write_footer(save_data, cache, root));

    DEBUG("Freeing file caches");
    cork_hash_table_free(save_data->serialized_ids);
    return 0;

  error:
    /* If there's an error, clean up the objects that we've created
     * before returning. */
    cork_hash_table_free(save_data->serialized_ids);
    return -1;
}


/*-----------------------------------------------------------------------
 * Helper functions
 */

/**
 * Write a NUL-terminated string to a stream.  If we can't write the
 * string for some reason, return an error.
 */
static int
write_string(struct cork_stream_consumer *stream, const char *str)
{
    size_t  len = strlen(str);
    return cork_stream_consumer_data(stream, str, len, false);
}


/**
 * Write a big-endian uint8 to a stream.  If we can't write the
 * integer for some reason, return an error.
 */
static int
write_uint8(struct cork_stream_consumer *stream, uint8_t val)
{
    /* for a byte, we don't need to endian-swap */
    return cork_stream_consumer_data(stream, &val, sizeof(uint8_t), false);
}


/**
 * Write a big-endian uint16 to a stream.  If we can't write the
 * integer for some reason, return an error.
 */
static int
write_uint16(struct cork_stream_consumer *stream, uint16_t val)
{
    CORK_UINT16_HOST_TO_BIG_IN_PLACE(val);
    return cork_stream_consumer_data(stream, &val, sizeof(uint16_t), false);
}


/**
 * Write a big-endian uint32 to a stream.  If we can't write the
 * integer for some reason, return an error.
 */

static int
write_uint32(struct cork_stream_consumer *stream, uint32_t val)
{
    CORK_UINT32_HOST_TO_BIG_IN_PLACE(val);
    return cork_stream_consumer_data(stream, &val, sizeof(uint32_t), false);
}


/**
 * Write a big-endian uint64 to a stream.  If we can't write the
 * integer for some reason, return an error.
 */

static int
write_uint64(struct cork_stream_consumer *stream, uint64_t val)
{
    CORK_UINT64_HOST_TO_BIG_IN_PLACE(val);
    return cork_stream_consumer_data(stream, &val, sizeof(uint64_t), false);
}


/*-----------------------------------------------------------------------
 * V1 BDD file
 */

static const char  MAGIC_NUMBER[] = "IP set";
static const size_t  MAGIC_NUMBER_LENGTH = sizeof(MAGIC_NUMBER) - 1;


static int
write_header_v1(struct save_data *save_data,
                struct ipset_node_cache *cache, ipset_node_id root)
{
    /* Output the magic number for an IP set, and the file format
     * version that we're going to write. */
    rii_check(cork_stream_consumer_data(save_data->stream, NULL, 0, true));
    rii_check(write_string(save_data->stream, MAGIC_NUMBER));
    rii_check(write_uint16(save_data->stream, 0x0001));

    /* Determine how many reachable nodes there are, to calculate the
     * size of the set. */
    size_t  nonterminal_count = ipset_node_reachable_count(cache, root);
    size_t  set_size =
        MAGIC_NUMBER_LENGTH +    /* magic number */
        sizeof(uint16_t) +        /* version number  */
        sizeof(uint64_t) +        /* length of set */
        sizeof(uint32_t) +        /* number of nonterminals */
        (nonterminal_count *     /* for each nonterminal: */
         (sizeof(uint8_t) +       /*   variable number */
          sizeof(uint32_t) +      /*   low pointer */
          sizeof(uint32_t)        /*   high pointer */
         ));

    /* If the root is a terminal, we need to add 4 bytes to the set
     * size, for storing the terminal value. */
    if (ipset_node_get_type(root) == IPSET_TERMINAL_NODE) {
        set_size += sizeof(uint32_t);
    }

    rii_check(write_uint64(save_data->stream, set_size));
    rii_check(write_uint32(save_data->stream, nonterminal_count));
    return 0;
}


static int
write_footer_v1(struct save_data *save_data,
                struct ipset_node_cache *cache, ipset_node_id root)
{
    /* If the root is a terminal node, then we output the terminal value
     * in place of the (nonexistent) list of nonterminal nodes. */

    if (ipset_node_get_type(root) == IPSET_TERMINAL_NODE) {
        ipset_value  value = ipset_terminal_value(root);
        return write_uint32(save_data->stream, value);
    }

    return 0;
}


static int
write_terminal_v1(struct save_data *save_data, ipset_value terminal_value)
{
    /* We don't have to write anything out for a terminal in a V1 file,
     * since the terminal's value will be encoded into the node ID
     * wherever it's used. */
    return 0;
}


static int
write_nonterminal_v1(struct save_data *save_data,
                     serialized_id serialized_node,
                     ipset_variable variable,
                     serialized_id serialized_low,
                     serialized_id serialized_high)
{
    rii_check(write_uint8(save_data->stream, variable));
    rii_check(write_uint32(save_data->stream, serialized_low));
    rii_check(write_uint32(save_data->stream, serialized_high));
    return 0;
}


int
ipset_node_cache_save(struct cork_stream_consumer *stream, struct ipset_node_cache *cache,
                      ipset_node_id node)
{
    struct save_data  save_data;
    save_data.cache = cache;
    save_data.stream = stream;
    save_data.write_header = write_header_v1;
    save_data.write_footer = write_footer_v1;
    save_data.write_terminal = write_terminal_v1;
    save_data.write_nonterminal = write_nonterminal_v1;
    return save_bdd(&save_data, cache, node);
}


/*-----------------------------------------------------------------------
 * GraphViz dot file
 */

static const char  *GRAPHVIZ_HEADER =
    "strict digraph bdd {\n";

static const char  *GRAPHVIZ_FOOTER =
    "}\n";


struct dot_data {
    /* The terminal value to leave out of the dot file.  This should be
     * the default value of the set or map. */
    ipset_value  default_value;

    /* A scratch buffer */
    struct cork_buffer  scratch;
};


static int
write_header_dot(struct save_data *save_data,
                 struct ipset_node_cache *cache, ipset_node_id root)
{
    /* Output the opening clause of the GraphViz script. */
    rii_check(cork_stream_consumer_data(save_data->stream, NULL, 0, true));
    return write_string(save_data->stream, GRAPHVIZ_HEADER);
}


static int
write_footer_dot(struct save_data *save_data,
                 struct ipset_node_cache *cache, ipset_node_id root)
{
    /* Output the closing clause of the GraphViz script. */
    return write_string(save_data->stream, GRAPHVIZ_FOOTER);
}


static int
write_terminal_dot(struct save_data *save_data, ipset_value terminal_value)
{
    struct dot_data  *dot_data = save_data->user_data;

    /* If this terminal has the default value, skip it. */
    if (terminal_value == dot_data->default_value) {
        return 0;
    }

    /* Output a node for the terminal value. */
    cork_buffer_printf
        (&dot_data->scratch,
         "    t%d [shape=box, label=%d];\n",
         terminal_value, terminal_value);
    return write_string(save_data->stream, dot_data->scratch.buf);
}


static int
write_nonterminal_dot(struct save_data *save_data,
                      serialized_id serialized_node,
                      ipset_variable variable,
                      serialized_id serialized_low,
                      serialized_id serialized_high)
{
    struct dot_data  *dot_data = save_data->user_data;

    /* Include a node for the nonterminal value. */
    cork_buffer_printf
        (&dot_data->scratch,
         "    n%d [shape=circle,label=%u];\n",
         (-serialized_node), variable);

    /* Include an edge for the low pointer. */
    if (serialized_low < 0) {
        /* The low pointer is a nonterminal. */
        cork_buffer_append_printf
            (&dot_data->scratch,
             "    n%d -> n%d",
             (-serialized_node), (-serialized_low));
    } else {
        /* The low pointer is a terminal. */
        ipset_value  low_value = (ipset_value) serialized_low;

        if (low_value == dot_data->default_value) {
            /* The terminal is the default value, so instead of a real
             * terminal, connect this pointer to a dummy circle node. */
            cork_buffer_append_printf
                (&dot_data->scratch,
                 "    low%d [shape=circle,label=\"\"]\n"
                 "    n%d -> low%d",
                 (-serialized_node), (-serialized_node), (-serialized_node));
        } else {
            /* The terminal isn't a default, so go ahead and output it. */
            cork_buffer_append_printf
                (&dot_data->scratch,
                 "    n%d -> t%d",
                 (-serialized_node), serialized_low);
        }
    }

    cork_buffer_append_printf
        (&dot_data->scratch, " [style=dashed,color=red]\n");

    /* Include an edge for the high pointer. */
    if (serialized_high < 0) {
        /* The high pointer is a nonterminal. */
        cork_buffer_append_printf
            (&dot_data->scratch,
             "    n%d -> n%d",
             (-serialized_node), (-serialized_high));
    } else {
        /* The high pointer is a terminal. */
        ipset_value  high_value = (ipset_value) serialized_high;

        if (high_value == dot_data->default_value) {
            /* The terminal is the default value, so instead of a real
             * terminal, connect this pointer to a dummy circle node. */
            cork_buffer_append_printf
                (&dot_data->scratch,
                 "    high%d "
                 "[shape=circle,"
                 "fixedsize=true,"
                 "height=0.25,"
                 "width=0.25,"
                 "label=\"\"]\n"
                 "    n%d -> high%d",
                 (-serialized_node), (-serialized_node), (-serialized_node));
        } else {
            /* The terminal isn't a default, so go ahead and output it. */
            cork_buffer_append_printf
                (&dot_data->scratch,
                 "    n%d -> t%d",
                 (-serialized_node), serialized_high);
        }
    }

    cork_buffer_append_printf
        (&dot_data->scratch, " [style=solid,color=black]\n");

    /* Output the clauses to the stream. */
    return write_string(save_data->stream, dot_data->scratch.buf);
}


int
ipset_node_cache_save_dot(struct cork_stream_consumer *stream,
                          struct ipset_node_cache *cache, ipset_node_id node)
{
    struct dot_data  dot_data = {
        0                       /* default value */
    };

    struct save_data  save_data;
    save_data.cache = cache;
    save_data.stream = stream;
    save_data.write_header = write_header_dot;
    save_data.write_footer = write_footer_dot;
    save_data.write_terminal = write_terminal_dot;
    save_data.write_nonterminal = write_nonterminal_dot;
    save_data.user_data = &dot_data;
    return save_bdd(&save_data, cache, node);
}

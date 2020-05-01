/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2010-2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <libcork/core.h>
#include <libcork/ds.h>
#include <libcork/helpers/errors.h>

#include "ipset/bdd/nodes.h"
#include "ipset/errors.h"
#include "ipset/logging.h"


static const char  MAGIC_NUMBER[] = "IP set";
static const size_t  MAGIC_NUMBER_LENGTH = sizeof(MAGIC_NUMBER) - 1;


/**
 * On disk, we use a different node ID scheme than we do in memory.
 * Terminal node IDs are non-negative, and are equal to the terminal
 * value.  Nonterminal node IDs are negative, starting with -1.
 * Nonterminal -1 appears first on disk, then nonterminal -2, and so on.
 */

typedef int  serialized_id;


/**
 * Sets a libcork error based on the contents of errno.
 */
static void
create_errno_error(FILE *stream)
{
    if (ferror(stream)) {
        cork_error_set(IPSET_ERROR, IPSET_IO_ERROR, "%s", strerror(errno));
    } else {
        cork_unknown_error();
    }
}


/**
 * Read in a big-endian uint8 from a stream.  If we can't read the
 * integer for some reason, return an error.
 */
static int
read_uint8(FILE *stream, uint8_t *dest)
{
    size_t  num_read = fread(dest, sizeof(uint8_t), 1, stream);
    if (num_read != 1) {
        create_errno_error(stream);
        return -1;
    }

    /* for a byte, we don't need to endian-swap */
    return 0;
}


/**
 * Read in a big-endian uint16 from a stream.  If we can't read the
 * integer for some reason, return an error.
 */
static uint16_t
read_uint16(FILE *stream, uint16_t *dest)
{
    size_t  num_read = fread(dest, sizeof(uint16_t), 1, stream);
    if (num_read != 1) {
        create_errno_error(stream);
        return -1;
    }

    CORK_UINT16_BIG_TO_HOST_IN_PLACE(*dest);
    return 0;
}


/**
 * Read in a big-endian uint32 from a stream.  If we can't read the
 * integer for some reason, return an error.
 */
static uint32_t
read_uint32(FILE *stream, uint32_t *dest)
{
    size_t  num_read = fread(dest, sizeof(uint32_t), 1, stream);
    if (num_read != 1) {
        create_errno_error(stream);
        return -1;
    }

    CORK_UINT32_BIG_TO_HOST_IN_PLACE(*dest);
    return 0;
}


/**
 * Read in a big-endian uint64 from a stream.  If we can't read the
 * integer for some reason, return an error.
 */
static uint64_t
read_uint64(FILE *stream, uint64_t *dest)
{
    size_t  num_read = fread(dest, sizeof(uint64_t), 1, stream);
    if (num_read != 1) {
        create_errno_error(stream);
        return -1;
    }

    CORK_UINT64_BIG_TO_HOST_IN_PLACE(*dest);
    return 0;
}


/**
 * A helper function that verifies that we've read exactly as many bytes
 * as we should, returning an error otherwise.
 */
static int
verify_cap(size_t bytes_read, size_t cap)
{
    if (bytes_read < cap) {
        /* There's extra data at the end of the stream. */
        cork_error_set
            (IPSET_ERROR, IPSET_PARSE_ERROR,
             "Malformed set: extra data at end of stream.");
        return -1;
    } else if (bytes_read > cap) {
        /* We read more data than we were supposed to. */
        cork_error_set
            (IPSET_ERROR, IPSET_PARSE_ERROR,
             "Malformed set: read too much data.");
        return -1;
    }

    return 0;
}

/**
 * A helper function for reading a version 1 BDD stream.
 */
static ipset_node_id
load_v1(FILE *stream, struct ipset_node_cache *cache)
{
    DEBUG("Stream contains v1 IP set");
    ipset_node_id  result;
    struct cork_hash_table  *cache_ids = cork_pointer_hash_table_new(0, 0);

    /* We've already read in the magic number and version.  Next should
     * be the length of the encoded set. */
    uint64_t  length;
    DEBUG("Reading encoded length");
    ei_check(read_uint64(stream, &length));

    /* The length includes the magic number, version number, and the
     * length field itself.  Remove those to get the cap on the
     * remaining stream. */

    size_t  bytes_read = 0;
    size_t  cap = length -
        MAGIC_NUMBER_LENGTH -
        sizeof(uint16_t) -
        sizeof(uint64_t);

    DEBUG("Length cap is %zu bytes.", cap);

    /* Read in the number of nonterminals. */

    uint32_t  nonterminal_count;
    DEBUG("Reading number of nonterminals");
    ei_check(read_uint32(stream, &nonterminal_count));
    bytes_read += sizeof(uint32_t);

    /* If there are no nonterminals, then there's only a single terminal
     * left to read. */

    if (nonterminal_count == 0) {
        uint32_t  value;
        DEBUG("Reading single terminal value");
        ei_check(read_uint32(stream, &value));
        bytes_read += sizeof(uint32_t);

        /* We should have reached the end of the encoded set. */
        ei_check(verify_cap(bytes_read, cap));

        /* Create a terminal node for this value and return it. */
        cork_hash_table_free(cache_ids);
        return ipset_terminal_node_id(value);
    }

    /* Otherwise, read in each nonterminal.  We need to keep track of a
     * mapping between each nonterminal's ID in the stream (which are
     * number consecutively from -1), and its ID in the node cache
     * (which could be anything). */

    size_t  i;
    for (i = 0; i < nonterminal_count; i++) {
        serialized_id  serialized_id = -(i+1);

        /* Each serialized node consists of a variable index, a low
         * pointer, and a high pointer. */

        uint8_t  variable;
        ei_check(read_uint8(stream, &variable));
        bytes_read += sizeof(uint8_t);

        int32_t  low;
        ei_check(read_uint32(stream, (uint32_t *) &low));
        bytes_read += sizeof(int32_t);

        int32_t  high;
        ei_check(read_uint32(stream, (uint32_t *) &high));
        bytes_read += sizeof(int32_t);

        DEBUG("Read serialized node %d = (x%d? %" PRId32 ": %" PRId32 ")",
              serialized_id, variable, high, low);

        /* Turn the low pointer into a node ID.  If the pointer is >= 0,
         * it's a terminal value.  Otherwise, its a nonterminal ID,
         * indexing into the serialized nonterminal array.*/

        ipset_node_id  low_id;

        if (low >= 0) {
            low_id = ipset_terminal_node_id(low);
        } else {
            /* The file format guarantees that any node reference points
             * to a node earlier in the serialized array.  That means we
             * can assume that cache_ids has already been filled in for
             * this node. */

            low_id = (ipset_node_id) (uintptr_t)
                cork_hash_table_get(cache_ids, (void *) (intptr_t) low);

            DEBUG("  Serialized ID %" PRId32 " is internal ID %u",
                  low, low_id);
        }

        /* Do the same for the high pointer. */

        ipset_node_id  high_id;

        if (high >= 0) {
            high_id = ipset_terminal_node_id(high);
        } else {
            /* The file format guarantees that any node reference points
             * to a node earlier in the serialized array.  That means we
             * can assume that cache_ids has already been filled in for
             * this node. */

            high_id = (ipset_node_id) (uintptr_t)
                cork_hash_table_get(cache_ids, (void *) (intptr_t) high);

            DEBUG("  Serialized ID %" PRId32 " is internal ID %u",
                  high, high_id);
        }

        /* Create a nonterminal node in the node cache. */
        result = ipset_node_cache_nonterminal
            (cache, variable, low_id, high_id);

        DEBUG("Internal node %u = nonterminal(x%d? %u: %u)",
              result, (int) variable, high_id, low_id);

        /* Remember the internal node ID for this new node, in case any
         * later serialized nodes point to it. */

        cork_hash_table_put
            (cache_ids, (void *) (intptr_t) serialized_id,
             (void *) (uintptr_t) result, NULL, NULL, NULL);
    }

    /* We should have reached the end of the encoded set. */
    ei_check(verify_cap(bytes_read, cap));

    /* The last node is the nonterminal for the entire set. */
    cork_hash_table_free(cache_ids);
    return result;

  error:
    /* If there's an error, clean up the objects that we've created
     * before returning. */

    cork_hash_table_free(cache_ids);
    return 0;
}


ipset_node_id
ipset_node_cache_load(FILE *stream, struct ipset_node_cache *cache)
{
    size_t bytes_read;

    /* First, read in the magic number from the stream to ensure that
     * this is an IP set. */

    uint8_t  magic[MAGIC_NUMBER_LENGTH];

    DEBUG("Reading IP set magic number");
    bytes_read = fread(magic, 1, MAGIC_NUMBER_LENGTH, stream);

    if (ferror(stream)) {
        create_errno_error(stream);
        return 0;
    }

    if (bytes_read != MAGIC_NUMBER_LENGTH) {
        /* We reached EOF before reading the entire magic number. */
        cork_error_set
            (IPSET_ERROR, IPSET_PARSE_ERROR,
             "Unexpected end of file");
        return 0;
    }

    if (memcmp(magic, MAGIC_NUMBER, MAGIC_NUMBER_LENGTH) != 0) {
        /* The magic number doesn't match, so this isn't a BDD. */
        cork_error_set
            (IPSET_ERROR, IPSET_PARSE_ERROR,
             "Magic number doesn't match; this isn't an IP set.");
        return 0;
    }

    /* Read in the version number and dispatch to the right reading
     * function. */

    uint16_t  version;
    DEBUG("Reading IP set version");
    xi_check(0, read_uint16(stream, &version));

    switch (version) {
        case 0x0001:
            return load_v1(stream, cache);

        default:
            /* We don't know how to read this version number. */
            cork_error_set
                (IPSET_ERROR, IPSET_PARSE_ERROR,
                 "Unknown version number %" PRIu16, version);
            return 0;
    }
}

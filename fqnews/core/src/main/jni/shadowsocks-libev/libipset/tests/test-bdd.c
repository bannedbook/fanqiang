/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <stdlib.h>
#include <unistd.h>

#include <check.h>
#include <libcork/core.h>

#include "ipset/bdd/nodes.h"
#include "ipset/bits.h"


#define DESCRIBE_TEST  fprintf(stderr, "---\n%s\n", __func__)


/*-----------------------------------------------------------------------
 * Temporary file helper
 */

#define TEMP_FILE_TEMPLATE "/tmp/bdd-XXXXXX"

struct temp_file {
    char  *filename;
    FILE  *stream;
};

static struct temp_file *
temp_file_new(void)
{
    struct temp_file  *temp_file = cork_new(struct temp_file);
    temp_file->filename = (char *) cork_strdup(TEMP_FILE_TEMPLATE);
    temp_file->stream = NULL;
    return temp_file;
}

static void
temp_file_free(struct temp_file *temp_file)
{
    if (temp_file->stream != NULL) {
        fclose(temp_file->stream);
    }

    unlink(temp_file->filename);
    cork_strfree(temp_file->filename);
    free(temp_file);
}

static void
temp_file_open_stream(struct temp_file *temp_file)
{
    int  fd = mkstemp(temp_file->filename);
    temp_file->stream = fdopen(fd, "rb+");
}


/*-----------------------------------------------------------------------
 * Bit arrays
 */

START_TEST(test_bit_get)
{
    DESCRIBE_TEST;
    uint16_t  a = CORK_UINT16_HOST_TO_BIG(0x6012); /* 0110 0000 0001 0010 */

    fail_unless(IPSET_BIT_GET(&a,  0) == 0, "Bit 0 is incorrect");
    fail_unless(IPSET_BIT_GET(&a,  1) == 1, "Bit 1 is incorrect");
    fail_unless(IPSET_BIT_GET(&a,  2) == 1, "Bit 2 is incorrect");
    fail_unless(IPSET_BIT_GET(&a,  3) == 0, "Bit 3 is incorrect");

    fail_unless(IPSET_BIT_GET(&a,  4) == 0, "Bit 4 is incorrect");
    fail_unless(IPSET_BIT_GET(&a,  5) == 0, "Bit 5 is incorrect");
    fail_unless(IPSET_BIT_GET(&a,  6) == 0, "Bit 6 is incorrect");
    fail_unless(IPSET_BIT_GET(&a,  7) == 0, "Bit 7 is incorrect");

    fail_unless(IPSET_BIT_GET(&a,  8) == 0, "Bit 8 is incorrect");
    fail_unless(IPSET_BIT_GET(&a,  9) == 0, "Bit 9 is incorrect");
    fail_unless(IPSET_BIT_GET(&a, 10) == 0, "Bit 10 is incorrect");
    fail_unless(IPSET_BIT_GET(&a, 11) == 1, "Bit 11 is incorrect");

    fail_unless(IPSET_BIT_GET(&a, 12) == 0, "Bit 12 is incorrect");
    fail_unless(IPSET_BIT_GET(&a, 13) == 0, "Bit 13 is incorrect");
    fail_unless(IPSET_BIT_GET(&a, 14) == 1, "Bit 14 is incorrect");
    fail_unless(IPSET_BIT_GET(&a, 15) == 0, "Bit 15 is incorrect");
}
END_TEST


START_TEST(test_bit_set)
{
    DESCRIBE_TEST;
    uint16_t  a = 0xffff;        /* 0110 0000 0001 0010 */

    IPSET_BIT_SET(&a,  0, 0);
    IPSET_BIT_SET(&a,  1, 1);
    IPSET_BIT_SET(&a,  2, 1);
    IPSET_BIT_SET(&a,  3, 0);

    IPSET_BIT_SET(&a,  4, 0);
    IPSET_BIT_SET(&a,  5, 0);
    IPSET_BIT_SET(&a,  6, 0);
    IPSET_BIT_SET(&a,  7, 0);

    IPSET_BIT_SET(&a,  8, 0);
    IPSET_BIT_SET(&a,  9, 0);
    IPSET_BIT_SET(&a, 10, 0);
    IPSET_BIT_SET(&a, 11, 1);

    IPSET_BIT_SET(&a, 12, 0);
    IPSET_BIT_SET(&a, 13, 0);
    IPSET_BIT_SET(&a, 14, 1);
    IPSET_BIT_SET(&a, 15, 0);

    fail_unless(CORK_UINT16_HOST_TO_BIG(0x6012) == a,
                "Incorrect bit result: 0x%04" PRIu16, a);
}
END_TEST


/*-----------------------------------------------------------------------
 * BDD terminals
 */

START_TEST(test_bdd_false_terminal)
{
    DESCRIBE_TEST;
    struct ipset_node_cache  *cache = ipset_node_cache_new();

    ipset_node_id  n_false = ipset_terminal_node_id(false);

    fail_unless(ipset_node_get_type(n_false) == IPSET_TERMINAL_NODE,
                "False terminal has wrong type");

    fail_unless(ipset_terminal_value(n_false) == false,
                "False terminal has wrong value");

    ipset_node_decref(cache, n_false);
    ipset_node_cache_free(cache);
}
END_TEST


START_TEST(test_bdd_true_terminal)
{
    DESCRIBE_TEST;
    struct ipset_node_cache  *cache = ipset_node_cache_new();

    ipset_node_id  n_true = ipset_terminal_node_id(true);

    fail_unless(ipset_node_get_type(n_true) == IPSET_TERMINAL_NODE,
                "True terminal has wrong type");

    fail_unless(ipset_terminal_value(n_true) == true,
                "True terminal has wrong value");

    ipset_node_decref(cache, n_true);
    ipset_node_cache_free(cache);
}
END_TEST


START_TEST(test_bdd_terminal_reduced_1)
{
    DESCRIBE_TEST;
    struct ipset_node_cache  *cache = ipset_node_cache_new();

    ipset_node_id  node1 = ipset_terminal_node_id(false);
    ipset_node_id  node2 = ipset_terminal_node_id(false);

    fail_unless(node1 == node2,
                "Terminal node isn't reduced");

    ipset_node_decref(cache, node1);
    ipset_node_decref(cache, node2);
    ipset_node_cache_free(cache);
}
END_TEST


/*-----------------------------------------------------------------------
 * BDD non-terminals
 */

START_TEST(test_bdd_nonterminal_1)
{
    DESCRIBE_TEST;
    struct ipset_node_cache  *cache = ipset_node_cache_new();

    ipset_node_id  n_false = ipset_terminal_node_id(false);
    ipset_node_id  n_true = ipset_terminal_node_id(true);

    ipset_node_id  node =
        ipset_node_cache_nonterminal(cache, 0, n_false, n_true);

    fail_unless(ipset_node_get_type(node) == IPSET_NONTERMINAL_NODE,
                "Nonterminal has wrong type");

    struct ipset_node  *n = ipset_node_cache_get_nonterminal(cache, node);

    fail_unless(n->variable == 0,
                "Nonterminal has wrong variable");
    fail_unless(n->low == n_false,
                "Nonterminal has wrong low pointer");
    fail_unless(n->high == n_true,
                "Nonterminal has wrong high pointer");

    ipset_node_decref(cache, node);
    ipset_node_cache_free(cache);
}
END_TEST


START_TEST(test_bdd_nonterminal_reduced_1)
{
    DESCRIBE_TEST;
    struct ipset_node_cache  *cache = ipset_node_cache_new();

    /* If we create nonterminals via a BDD engine, they will be reduced
     * — i.e., every nonterminal with the same value will be in the same
     * memory location. */

    ipset_node_id  n_false = ipset_terminal_node_id(false);
    ipset_node_id  n_true = ipset_terminal_node_id(true);

    ipset_node_id  node1 =
        ipset_node_cache_nonterminal(cache, 0, n_false, n_true);
    ipset_node_id  node2 =
        ipset_node_cache_nonterminal(cache, 0, n_false, n_true);

    fail_unless(node1 == node2,
                "Nonterminal node isn't reduced");

    ipset_node_decref(cache, node1);
    ipset_node_decref(cache, node2);
    ipset_node_cache_free(cache);
}
END_TEST


START_TEST(test_bdd_nonterminal_reduced_2)
{
    DESCRIBE_TEST;
    struct ipset_node_cache  *cache = ipset_node_cache_new();

    /* We shouldn't have a nonterminal whose low and high subtrees are
     * equal. */

    ipset_node_id  n_false = ipset_terminal_node_id(false);

    ipset_node_id  node =
        ipset_node_cache_nonterminal(cache, 0, n_false, n_false);

    fail_unless(node == n_false,
                "Nonterminal node isn't reduced");

    ipset_node_decref(cache, node);
    ipset_node_cache_free(cache);
}
END_TEST


/*-----------------------------------------------------------------------
 * Evaluation
 */

START_TEST(test_bdd_evaluate_1)
{
    DESCRIBE_TEST;
    struct ipset_node_cache  *cache = ipset_node_cache_new();

    /* Create a BDD representing
     *   f(x) = ¬x[0]
     */

    ipset_node_id  n_false = ipset_terminal_node_id(false);
    ipset_node_id  n_true = ipset_terminal_node_id(true);

    ipset_node_id  node =
        ipset_node_cache_nonterminal(cache, 0, n_true, n_false);

    /* And test we can get the right results out of it. */

    uint8_t  input1[] = { 0x80 }; /* { TRUE } */
    bool  expected1 = false;
    fail_unless(ipset_node_evaluate
                (cache, node, ipset_bit_array_assignment, input1)
                == expected1,
                "BDD evaluates to wrong value");

    uint8_t  input2[] = { 0x00 }; /* { FALSE } */
    bool  expected2 = true;
    fail_unless(ipset_node_evaluate
                (cache, node, ipset_bit_array_assignment, input2)
                == expected2,
                "BDD evaluates to wrong value");

    ipset_node_decref(cache, node);
    ipset_node_cache_free(cache);
}
END_TEST


START_TEST(test_bdd_evaluate_2)
{
    DESCRIBE_TEST;
    struct ipset_node_cache  *cache = ipset_node_cache_new();

    /* Create a BDD representing
     *   f(x) = ¬x[0] ∧ x[1]
     */

    ipset_node_id  n_false = ipset_terminal_node_id(false);
    ipset_node_id  n_true = ipset_terminal_node_id(true);

    ipset_node_id  node1 =
        ipset_node_cache_nonterminal(cache, 1, n_false, n_true);
    ipset_node_id  node =
        ipset_node_cache_nonterminal(cache, 0, node1, n_false);

    /* And test we can get the right results out of it. */

    bool  input1[] = { true, true };
    bool  expected1 = false;
    fail_unless(ipset_node_evaluate
                (cache, node, ipset_bool_array_assignment, input1)
                == expected1,
                "BDD evaluates to wrong value");

    bool  input2[] = { true, false };
    bool  expected2 = false;
    fail_unless(ipset_node_evaluate
                (cache, node, ipset_bool_array_assignment, input2)
                == expected2,
                "BDD evaluates to wrong value");

    bool  input3[] = { false, true };
    bool  expected3 = true;
    fail_unless(ipset_node_evaluate
                (cache, node, ipset_bool_array_assignment, input3)
                == expected3,
                "BDD evaluates to wrong value");

    bool  input4[] = { false, false };
    bool  expected4 = false;
    fail_unless(ipset_node_evaluate
                (cache, node, ipset_bool_array_assignment, input4)
                == expected4,
                "BDD evaluates to wrong value");

    ipset_node_decref(cache, node);
    ipset_node_cache_free(cache);
}
END_TEST


/*-----------------------------------------------------------------------
 * Operators
 */

START_TEST(test_bdd_insert_reduced_1)
{
    DESCRIBE_TEST;
    struct ipset_node_cache  *cache = ipset_node_cache_new();

    /* Create a BDD representing
     *   f(x) = x[0] ∧ x[1]
     */
    bool  elem[] = { true, true };
    ipset_node_id  n_false0 = ipset_terminal_node_id(false);
    ipset_node_id  node0 =
        ipset_node_insert
        (cache, n_false0, ipset_bool_array_assignment, elem, 2, true);

    /* And then do it again. */
    ipset_node_id  n_false1 = ipset_terminal_node_id(false);
    ipset_node_id  node1 =
        ipset_node_insert
        (cache, n_false1, ipset_bool_array_assignment, elem, 2, true);

    /* Verify that we get the same physical node both times. */
    fail_unless(node0 == node1,
                "Insert result isn't reduced");

    ipset_node_decref(cache, n_false0);
    ipset_node_decref(cache, node0);
    ipset_node_decref(cache, n_false1);
    ipset_node_decref(cache, node1);
    ipset_node_cache_free(cache);
}
END_TEST


START_TEST(test_bdd_insert_evaluate_1)
{
    DESCRIBE_TEST;
    struct ipset_node_cache  *cache = ipset_node_cache_new();

    /* Create a BDD representing
     *   f(x) = x[0] ∧ x[1]
     */
    bool  elem[] = { true, true };
    ipset_node_id  n_false = ipset_terminal_node_id(false);
    ipset_node_id  node =
        ipset_node_insert
        (cache, n_false, ipset_bool_array_assignment, elem, 2, true);

    /* And test we can get the right results out of it. */

    bool  input1[] = { true, true };
    bool  expected1 = true;
    fail_unless(ipset_node_evaluate
                (cache, node, ipset_bool_array_assignment, input1)
                == expected1,
                "BDD evaluates to wrong value");

    bool  input2[] = { true, false };
    bool  expected2 = false;
    fail_unless(ipset_node_evaluate
                (cache, node, ipset_bool_array_assignment, input2)
                == expected2,
                "BDD evaluates to wrong value");

    bool  input3[] = { false, true };
    bool  expected3 = false;
    fail_unless(ipset_node_evaluate
                (cache, node, ipset_bool_array_assignment, input3)
                == expected3,
                "BDD evaluates to wrong value");

    bool  input4[] = { false, false };
    bool  expected4 = false;
    fail_unless(ipset_node_evaluate
                (cache, node, ipset_bool_array_assignment, input4)
                == expected4,
                "BDD evaluates to wrong value");

    ipset_node_decref(cache, n_false);
    ipset_node_decref(cache, node);
    ipset_node_cache_free(cache);
}
END_TEST


/*-----------------------------------------------------------------------
 * Memory size
 */

START_TEST(test_bdd_size_1)
{
    DESCRIBE_TEST;
    struct ipset_node_cache  *cache = ipset_node_cache_new();

    /* Create a BDD representing
     *   f(x) = (x[0] ∧ x[1]) ∨ (¬x[0] ∧ x[2])
     */
    bool  elem1[] = { true, true };
    bool  elem2[] = { false, true, true };
    bool  elem3[] = { false, false, true };

    ipset_node_id  n_false = ipset_terminal_node_id(false);
    ipset_node_id  n1 =
        ipset_node_insert
        (cache, n_false, ipset_bool_array_assignment, elem1, 2, true);
    ipset_node_id  n2 =
        ipset_node_insert
        (cache, n1, ipset_bool_array_assignment, elem2, 3, true);
    ipset_node_id  node =
        ipset_node_insert
        (cache, n2, ipset_bool_array_assignment, elem3, 3, true);

    /* And verify how big it is. */

    fail_unless(ipset_node_reachable_count(cache, node) == 3u,
                "BDD has wrong number of nodes");

    fail_unless(ipset_node_memory_size(cache, node) ==
                3u * sizeof(struct ipset_node),
                "BDD takes up wrong amount of space");

    ipset_node_decref(cache, n_false);
    ipset_node_decref(cache, n1);
    ipset_node_decref(cache, n2);
    ipset_node_decref(cache, node);
    ipset_node_cache_free(cache);
}
END_TEST


/*-----------------------------------------------------------------------
 * Serialization
 */

START_TEST(test_bdd_save_1)
{
    DESCRIBE_TEST;
    struct ipset_node_cache  *cache = ipset_node_cache_new();

    /* Create a BDD representing
     *   f(x) = true
     */
    ipset_node_id  node = ipset_terminal_node_id(true);

    /* Serialize the BDD into a string. */
    struct cork_buffer  buf = CORK_BUFFER_INIT();
    struct cork_stream_consumer  *stream =
        cork_buffer_to_stream_consumer(&buf);

    fail_unless(ipset_node_cache_save(stream, cache, node) == 0,
                "Cannot serialize BDD");

    const char  *raw_expected =
        "IP set"                             // magic number
        "\x00\x01"                           // version
        "\x00\x00\x00\x00\x00\x00\x00\x18"   // length
        "\x00\x00\x00\x00"                   // node count
        "\x00\x00\x00\x01"                   // terminal value
        ;
    const size_t  expected_length = 24;

    fail_unless(expected_length == buf.size,
                "Serialized BDD has wrong length "
                "(expected %zu, got %zu)",
                expected_length, buf.size);

    fail_unless(memcmp(raw_expected, buf.buf, expected_length) == 0,
                "Serialized BDD has incorrect data");

    cork_stream_consumer_free(stream);
    cork_buffer_done(&buf);
    ipset_node_decref(cache, node);
    ipset_node_cache_free(cache);
}
END_TEST


START_TEST(test_bdd_save_2)
{
    DESCRIBE_TEST;
    struct ipset_node_cache  *cache = ipset_node_cache_new();

    /* Create a BDD representing
     *   f(x) = (x[0] ∧ x[1]) ∨ (¬x[0] ∧ x[2])
     */
    bool  elem1[] = { true, true };
    bool  elem2[] = { false, true, true };
    bool  elem3[] = { false, false, true };

    ipset_node_id  n_false = ipset_terminal_node_id(false);
    ipset_node_id  n1 =
        ipset_node_insert
        (cache, n_false, ipset_bool_array_assignment, elem1, 2, true);
    ipset_node_id  n2 =
        ipset_node_insert
        (cache, n1, ipset_bool_array_assignment, elem2, 3, true);
    ipset_node_id  node =
        ipset_node_insert
        (cache, n2, ipset_bool_array_assignment, elem3, 3, true);

    /* Serialize the BDD into a string. */
    struct cork_buffer  buf = CORK_BUFFER_INIT();
    struct cork_stream_consumer  *stream =
        cork_buffer_to_stream_consumer(&buf);

    fail_unless(ipset_node_cache_save(stream, cache, node) == 0,
                "Cannot serialize BDD");

    const char  *raw_expected =
        "IP set"                             // magic number
        "\x00\x01"                           // version
        "\x00\x00\x00\x00\x00\x00\x00\x2f"   // length
        "\x00\x00\x00\x03"                   // node count
        // node -1
        "\x02"                               // variable
        "\x00\x00\x00\x00"                   // low
        "\x00\x00\x00\x01"                   // high
        // node -2
        "\x01"                               // variable
        "\x00\x00\x00\x00"                   // low
        "\x00\x00\x00\x01"                   // high
        // node -3
        "\x00"                               // variable
        "\xff\xff\xff\xff"                   // low
        "\xff\xff\xff\xfe"                   // high
        ;
    const size_t  expected_length = 47;

    fail_unless(expected_length == buf.size,
                "Serialized BDD has wrong length "
                "(expected %zu, got %zu)",
                expected_length, buf.size);

    fail_unless(memcmp(raw_expected, buf.buf, expected_length) == 0,
                "Serialized BDD has incorrect data");

    cork_stream_consumer_free(stream);
    cork_buffer_done(&buf);
    ipset_node_decref(cache, n_false);
    ipset_node_decref(cache, n1);
    ipset_node_decref(cache, n2);
    ipset_node_decref(cache, node);
    ipset_node_cache_free(cache);
}
END_TEST


START_TEST(test_bdd_load_1)
{
    DESCRIBE_TEST;
    struct ipset_node_cache  *cache = ipset_node_cache_new();

    /* Create a BDD representing
     *   f(x) = true
     */
    ipset_node_id  node = ipset_terminal_node_id(true);

    /* Read a BDD from a string. */
    const char  *raw =
        "IP set"                             // magic number
        "\x00\x01"                           // version
        "\x00\x00\x00\x00\x00\x00\x00\x18"   // length
        "\x00\x00\x00\x00"                   // node count
        "\x00\x00\x00\x01"                   // terminal value
        ;
    const size_t  raw_length = 24;

    struct temp_file  *temp_file = temp_file_new();
    temp_file_open_stream(temp_file);
    fwrite(raw, raw_length, 1, temp_file->stream);
    fflush(temp_file->stream);
    fseek(temp_file->stream, 0, SEEK_SET);

    ipset_node_id  read = ipset_node_cache_load(temp_file->stream, cache);
    fail_if(cork_error_occurred(),
            "Error reading BDD from stream");

    fail_unless(read == node,
                "BDD from stream doesn't match expected");

    temp_file_free(temp_file);
    ipset_node_decref(cache, node);
    ipset_node_decref(cache, read);
    ipset_node_cache_free(cache);
}
END_TEST


START_TEST(test_bdd_load_2)
{
    DESCRIBE_TEST;
    struct ipset_node_cache  *cache = ipset_node_cache_new();

    /** Create a BDD representing
     *   f(x) = (x[0] ∧ x[1]) ∨ (¬x[0] ∧ x[2])
     */
    bool  elem1[] = { true, true };
    bool  elem2[] = { false, true, true };
    bool  elem3[] = { false, false, true };

    ipset_node_id  n_false = ipset_terminal_node_id(false);
    ipset_node_id  n1 =
        ipset_node_insert
        (cache, n_false, ipset_bool_array_assignment, elem1, 2, true);
    ipset_node_id  n2 =
        ipset_node_insert
        (cache, n1, ipset_bool_array_assignment, elem2, 3, true);
    ipset_node_id  node =
        ipset_node_insert
        (cache, n2, ipset_bool_array_assignment, elem3, 3, true);

    /*
     * Read a BDD from a string.
     */

    const char  *raw =
        "IP set"                             // magic number
        "\x00\x01"                           // version
        "\x00\x00\x00\x00\x00\x00\x00\x2f"   // length
        "\x00\x00\x00\x03"                   // node count
        // node -1
        "\x02"                               // variable
        "\x00\x00\x00\x00"                   // low
        "\x00\x00\x00\x01"                   // high
        // node -2
        "\x01"                               // variable
        "\x00\x00\x00\x00"                   // low
        "\x00\x00\x00\x01"                   // high
        // node -3
        "\x00"                               // variable
        "\xff\xff\xff\xff"                   // low
        "\xff\xff\xff\xfe"                   // high
        ;
    const size_t  raw_length = 47;

    struct temp_file  *temp_file = temp_file_new();
    temp_file_open_stream(temp_file);
    fwrite(raw, raw_length, 1, temp_file->stream);
    fflush(temp_file->stream);
    fseek(temp_file->stream, 0, SEEK_SET);

    ipset_node_id  read = ipset_node_cache_load(temp_file->stream, cache);
    fail_if(cork_error_occurred(),
            "Error reading BDD from stream");

    fail_unless(read == node,
                "BDD from stream doesn't match expected");

    temp_file_free(temp_file);
    ipset_node_decref(cache, n_false);
    ipset_node_decref(cache, n1);
    ipset_node_decref(cache, n2);
    ipset_node_decref(cache, node);
    ipset_node_decref(cache, read);
    ipset_node_cache_free(cache);
}
END_TEST


/*-----------------------------------------------------------------------
 * Iteration
 */

START_TEST(test_bdd_iterate_1)
{
    DESCRIBE_TEST;
    struct ipset_node_cache  *cache = ipset_node_cache_new();

    /* Create a BDD representing
     *   f(x) = ¬x[0]
     */

    ipset_node_id  n_false = ipset_terminal_node_id(false);
    ipset_node_id  n_true = ipset_terminal_node_id(true);

    ipset_node_id  node =
        ipset_node_cache_nonterminal(cache, 0, n_true, n_false);

    /* And test that iterating the BDD gives us the expected results. */

    struct ipset_assignment  *expected = ipset_assignment_new();
    struct ipset_bdd_iterator  *it = ipset_node_iterate(cache, node);

    fail_if(it->finished,
            "Iterator should not be empty");
    ipset_assignment_clear(expected);
    ipset_assignment_set(expected, 0, IPSET_FALSE);
    fail_unless(ipset_assignment_equal(expected, it->assignment),
                "Iterator assignment 0 doesn't match");
    fail_unless(true == it->value,
                "Iterator value 0 doesn't match");

    ipset_bdd_iterator_advance(it);
    fail_if(it->finished,
            "Iterator should have more than 1 element");
    ipset_assignment_clear(expected);
    ipset_assignment_set(expected, 0, IPSET_TRUE);
    fail_unless(ipset_assignment_equal(expected, it->assignment),
                "Iterator assignment 1 doesn't match");
    fail_unless(false == it->value,
                "Iterator value 1 doesn't match (%u)", it->value);

    ipset_bdd_iterator_advance(it);
    fail_unless(it->finished,
                "Iterator should have 2 elements");

    ipset_assignment_free(expected);
    ipset_bdd_iterator_free(it);
    ipset_node_cache_free(cache);
}
END_TEST


START_TEST(test_bdd_iterate_2)
{
    DESCRIBE_TEST;
    struct ipset_node_cache  *cache = ipset_node_cache_new();

    /* Create a BDD representing
     *   f(x) = ¬x[0] ∧ x[1]
     */
    ipset_node_id  n_false = ipset_terminal_node_id(false);
    ipset_node_id  n_true = ipset_terminal_node_id(true);

    ipset_node_id  node1 =
        ipset_node_cache_nonterminal(cache, 1, n_false, n_true);
    ipset_node_id  node =
        ipset_node_cache_nonterminal(cache, 0, node1, n_false);

    /* And test that iterating the BDD gives us the expected results. */
    struct ipset_assignment  *expected = ipset_assignment_new();
    struct ipset_bdd_iterator  *it = ipset_node_iterate(cache, node);

    fail_if(it->finished,
            "Iterator should not be empty");
    ipset_assignment_clear(expected);
    ipset_assignment_set(expected, 0, IPSET_FALSE);
    ipset_assignment_set(expected, 1, IPSET_FALSE);
    fail_unless(ipset_assignment_equal(expected, it->assignment),
                "Iterator assignment 0 doesn't match");
    fail_unless(false == it->value,
                "Iterator value 0 doesn't match");

    ipset_bdd_iterator_advance(it);
    fail_if(it->finished,
            "Iterator should have more than 1 element");
    ipset_assignment_clear(expected);
    ipset_assignment_set(expected, 0, IPSET_FALSE);
    ipset_assignment_set(expected, 1, IPSET_TRUE);
    fail_unless(ipset_assignment_equal(expected, it->assignment),
                "Iterator assignment 1 doesn't match");
    fail_unless(true == it->value,
                "Iterator value 1 doesn't match (%u)", it->value);

    ipset_bdd_iterator_advance(it);
    fail_if(it->finished,
            "Iterator should have more than 2 elements");
    ipset_assignment_clear(expected);
    ipset_assignment_set(expected, 0, IPSET_TRUE);
    fail_unless(ipset_assignment_equal(expected, it->assignment),
                "Iterator assignment 2 doesn't match");
    fail_unless(false == it->value,
                "Iterator value 2 doesn't match (%u)", it->value);

    ipset_bdd_iterator_advance(it);
    fail_unless(it->finished,
                "Iterator should have 3 elements");

    ipset_assignment_free(expected);
    ipset_bdd_iterator_free(it);
    ipset_node_cache_free(cache);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

static Suite *
test_suite()
{
    Suite  *s = suite_create("bdd");

    TCase  *tc_bits = tcase_create("bits");
    tcase_add_test(tc_bits, test_bit_get);
    tcase_add_test(tc_bits, test_bit_set);
    suite_add_tcase(s, tc_bits);

    TCase  *tc_terminals = tcase_create("terminals");
    tcase_add_test(tc_terminals, test_bdd_false_terminal);
    tcase_add_test(tc_terminals, test_bdd_true_terminal);
    tcase_add_test(tc_terminals, test_bdd_terminal_reduced_1);
    suite_add_tcase(s, tc_terminals);

    TCase  *tc_nonterminals = tcase_create("nonterminals");
    tcase_add_test(tc_nonterminals, test_bdd_nonterminal_1);
    tcase_add_test(tc_nonterminals, test_bdd_nonterminal_reduced_1);
    tcase_add_test(tc_nonterminals, test_bdd_nonterminal_reduced_2);
    suite_add_tcase(s, tc_nonterminals);

    TCase  *tc_evaluation = tcase_create("evaluation");
    tcase_add_test(tc_evaluation, test_bdd_evaluate_1);
    tcase_add_test(tc_evaluation, test_bdd_evaluate_2);
    suite_add_tcase(s, tc_evaluation);

    TCase  *tc_operators = tcase_create("operators");
    tcase_add_test(tc_operators, test_bdd_insert_reduced_1);
    tcase_add_test(tc_operators, test_bdd_insert_evaluate_1);
    suite_add_tcase(s, tc_operators);

    TCase  *tc_size = tcase_create("size");
    tcase_add_test(tc_size, test_bdd_size_1);
    suite_add_tcase(s, tc_size);

    TCase  *tc_serialization = tcase_create("serialization");
    tcase_add_test(tc_serialization, test_bdd_save_1);
    tcase_add_test(tc_serialization, test_bdd_save_2);
    tcase_add_test(tc_serialization, test_bdd_load_1);
    tcase_add_test(tc_serialization, test_bdd_load_2);
    suite_add_tcase(s, tc_serialization);

    TCase  *tc_iteration = tcase_create("iteration");
    tcase_add_test(tc_iteration, test_bdd_iterate_1);
    tcase_add_test(tc_iteration, test_bdd_iterate_2);
    suite_add_tcase(s, tc_iteration);

    return s;
}


int
main(int argc, const char **argv)
{
    int  number_failed;
    Suite  *suite = test_suite();
    SRunner  *runner = srunner_create(suite);

    srunner_run_all(runner, CK_NORMAL);
    number_failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    return (number_failed == 0)? EXIT_SUCCESS: EXIT_FAILURE;
}

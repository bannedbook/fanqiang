/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2009-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <check.h>

#include "libcork/core/types.h"
#include "libcork/ds/buffer.h"
#include "libcork/ds/managed-buffer.h"
#include "libcork/ds/stream.h"

#include "helpers.h"


/*-----------------------------------------------------------------------
 * Buffers
 */

static void
check_buffers(const struct cork_buffer *buf1, const struct cork_buffer *buf2)
{
    fail_unless(cork_buffer_equal(buf1, buf2),
                "Buffers should be equal: got %zu:%s, expected %zu:%s",
                buf1->size, buf1->buf, buf2->size, buf2->buf);
}

static void
check_buffer(const struct cork_buffer *buf, const char *expected)
{
    size_t  expected_len = strlen(expected);
    fail_unless(buf->size == expected_len,
                "Unexpected buffer content: got %zu:%s, expected %zu:%s",
                buf->size, buf->buf, expected_len, expected);
}

START_TEST(test_buffer)
{
    static char  SRC[] =
        "Here is some text.";
    size_t  SRC_LEN = strlen(SRC);

    struct cork_buffer  buffer1;
    struct cork_buffer  *buffer2;
    struct cork_buffer  *buffer3;
    struct cork_buffer  buffer4;

    cork_buffer_init(&buffer1);
    fail_if_error(cork_buffer_set(&buffer1, SRC, SRC_LEN));

    fail_unless(cork_buffer_char(&buffer1, 0) == 'H',
                "Unexpected character at position 0: got %c, expected %c",
                (int) cork_buffer_char(&buffer1, 0), (int) 'H');

    fail_unless(cork_buffer_byte(&buffer1, 1) == (uint8_t) 'e',
                "Unexpected character at position 1: got %c, expected %c",
                (int) cork_buffer_byte(&buffer1, 1), (int) 'e');

    fail_if_error(buffer2 = cork_buffer_new());
    fail_if_error(cork_buffer_set_string(buffer2, SRC));
    check_buffers(&buffer1, buffer2);

    fail_if_error(buffer3 = cork_buffer_new());
    fail_if_error(cork_buffer_printf
                  (buffer3, "Here is %s text.", "some"));
    check_buffers(&buffer1, buffer3);

    cork_buffer_init(&buffer4);
    cork_buffer_copy(&buffer4, &buffer1);
    check_buffers(&buffer1, &buffer4);

    cork_buffer_done(&buffer1);
    cork_buffer_free(buffer2);
    cork_buffer_free(buffer3);
    cork_buffer_done(&buffer4);
}
END_TEST


START_TEST(test_buffer_append)
{
    static char  SRC1[] = "abcd";
    size_t  SRC1_LEN = 4;
    static char  SRC2[] = "efg";
    size_t  SRC2_LEN = 3;
    static char  SRC3[] = "hij";
    static char  SRC4[] = "kl";
    static char  EXPECTED[] = "abcdefghijkl";
    struct cork_buffer  buffer1;
    struct cork_buffer  buffer2;
    struct cork_buffer  *buffer3;

    cork_buffer_init(&buffer1);

    /*
     * Let's try setting some data, then clearing it, before we do our
     * appends.
     */

    fail_if_error(cork_buffer_set(&buffer1, SRC2, SRC2_LEN));
    cork_buffer_clear(&buffer1);

    /*
     * Okay now do the appends.
     */

    fail_if_error(cork_buffer_append(&buffer1, SRC1, SRC1_LEN));
    fail_if_error(cork_buffer_append(&buffer1, SRC2, SRC2_LEN));
    fail_if_error(cork_buffer_append_string(&buffer1, SRC3));
    fail_if_error(cork_buffer_append_string(&buffer1, SRC4));

    cork_buffer_init(&buffer2);
    fail_if_error(cork_buffer_set_string(&buffer2, EXPECTED));
    check_buffers(&buffer1, &buffer2);

    fail_if_error(buffer3 = cork_buffer_new());
    fail_if_error(cork_buffer_set(buffer3, SRC1, SRC1_LEN));
    fail_if_error(cork_buffer_append_printf
                  (buffer3, "%s%s%s", SRC2, SRC3, SRC4));
    check_buffers(&buffer1, buffer3);

    fail_unless(cork_buffer_equal(&buffer1, buffer3),
                "Buffers should be equal: got %zu:%s, expected %zu:%s",
                buffer1.size, buffer1.buf, buffer3->size, buffer3->buf);

    cork_buffer_done(&buffer1);
    cork_buffer_done(&buffer2);
    cork_buffer_free(buffer3);
}
END_TEST


START_TEST(test_buffer_slicing)
{
    static char  SRC[] =
        "Here is some text.";

    struct cork_buffer  *buffer;
    struct cork_managed_buffer  *managed;
    struct cork_slice  slice1;
    struct cork_slice  slice2;

    fail_if_error(buffer = cork_buffer_new());
    fail_if_error(cork_buffer_set_string(buffer, SRC));

    fail_if_error(managed = cork_buffer_to_managed_buffer
                  (buffer));
    cork_managed_buffer_unref(managed);

    fail_if_error(buffer = cork_buffer_new());
    fail_if_error(cork_buffer_set_string(buffer, SRC));

    fail_if_error(cork_buffer_to_slice(buffer, &slice1));

    fail_if_error(cork_slice_copy_offset(&slice2, &slice1, 2));
    cork_slice_finish(&slice2);

    fail_if_error(cork_slice_copy_offset(&slice2, &slice1, buffer->size));
    cork_slice_finish(&slice2);

    fail_if_error(cork_slice_copy_fast(&slice2, &slice1, 2, 2));
    cork_slice_finish(&slice2);

    fail_if_error(cork_slice_copy_offset_fast(&slice2, &slice1, 2));
    cork_slice_finish(&slice2);

    fail_if_error(cork_slice_copy_offset(&slice2, &slice1, 0));
    fail_if_error(cork_slice_slice_offset_fast(&slice2, 2));
    fail_if_error(cork_slice_slice_fast(&slice2, 0, 2));
    fail_if_error(cork_slice_slice(&slice1, 2, 2));
    fail_unless(cork_slice_equal(&slice1, &slice2), "Slices should be equal");
    cork_slice_finish(&slice2);

    cork_slice_finish(&slice1);
}
END_TEST


START_TEST(test_buffer_stream)
{
    static char  SRC1[] = "abcd";
    size_t  SRC1_LEN = 4;
    static char  SRC2[] = "efg";
    size_t  SRC2_LEN = 3;

    static char  EXPECTED[] = "000abcdefg";
    static size_t  EXPECTED_SIZE = 10;

    struct cork_buffer  buffer1;
    struct cork_buffer  buffer2;
    struct cork_stream_consumer  *consumer;

    cork_buffer_init(&buffer1);
    cork_buffer_append_string(&buffer1, "000");
    fail_if_error(consumer =
                  cork_buffer_to_stream_consumer(&buffer1));

    /* chunk #1 */
    fail_if_error(cork_stream_consumer_data(consumer, SRC1, SRC1_LEN, true));

    /* chunk #2 */
    fail_if_error(cork_stream_consumer_data(consumer, SRC2, SRC2_LEN, false));

    /* eof */
    fail_if_error(cork_stream_consumer_eof(consumer));

    /* check the result */
    cork_buffer_init(&buffer2);
    fail_if_error(cork_buffer_set(&buffer2, EXPECTED, EXPECTED_SIZE));
    check_buffers(&buffer1, &buffer2);

    cork_stream_consumer_free(consumer);
    cork_buffer_done(&buffer1);
    cork_buffer_done(&buffer2);
}
END_TEST


static void
check_c_string_(const char *content, size_t length,
                const char *expected)
{
    struct cork_buffer  buf = CORK_BUFFER_INIT();
    cork_buffer_append_c_string(&buf, content, length);
    check_buffer(&buf, expected);
    cork_buffer_done(&buf);
}

#define check_c_string(c)  (check_c_string_(c, sizeof(c) - 1, #c))
#define check_c_string_ex(c, e)  (check_c_string_(c, sizeof(c) - 1, e))

START_TEST(test_buffer_c_string)
{
    check_c_string("");
    check_c_string("hello world");
    check_c_string("\x00");
    check_c_string("\f\n\r\t\v");
    check_c_string_ex("\x0d\x0a", "\"\\n\\r\"");
}
END_TEST


static void
check_pretty_print_(size_t indent, const char *content, size_t length,
                    const char *expected)
{
    struct cork_buffer  buf = CORK_BUFFER_INIT();
    cork_buffer_append_binary(&buf, indent, content, length);
    check_buffer(&buf, expected);
    cork_buffer_done(&buf);
}

#define check_pretty_print(i, c, e) \
    (check_pretty_print_((i), (c), sizeof((c)) - 1, (e)))

START_TEST(test_buffer_pretty_print)
{
    check_pretty_print(0, "", "");
    check_pretty_print(0, "hello world", "hello world");
    check_pretty_print
        (0, "hello\nworld",
         "(multiline)\n"
         "hello\n"
         "world");
    check_pretty_print
        (0, "hello\n\x00world\r\x80hello again",
         "(hex)\n"
         "68 65 6c 6c 6f 0a 00 77 6f 72 6c 64 0d 80 68 65  |hello..world..he|\n"
         "6c 6c 6f 20 61 67 61 69 6e                       |llo again|");

    check_pretty_print(2, "", "");
    check_pretty_print(2, "hello world", "hello world");
    check_pretty_print
        (2, "hello\nworld",
         "(multiline)\n"
         "  hello\n"
         "  world");
    check_pretty_print
        (2, "hello\n\x00world\r\x80hello again",
         "(hex)\n"
         "  "
         "68 65 6c 6c 6f 0a 00 77 6f 72 6c 64 0d 80 68 65  |hello..world..he|\n"
         "  "
         "6c 6c 6f 20 61 67 61 69 6e                       |llo again|");

    check_pretty_print(4, "", "");
    check_pretty_print(4, "hello world", "hello world");
    check_pretty_print
        (4, "hello\nworld",
         "(multiline)\n"
         "    hello\n"
         "    world");
    check_pretty_print
        (4, "hello\n\x00world\r\x80hello again",
         "(hex)\n"
         "    "
         "68 65 6c 6c 6f 0a 00 77 6f 72 6c 64 0d 80 68 65  |hello..world..he|\n"
         "    "
         "6c 6c 6f 20 61 67 61 69 6e                       |llo again|");
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("buffer");

    TCase  *tc_buffer = tcase_create("buffer");
    tcase_add_test(tc_buffer, test_buffer);
    tcase_add_test(tc_buffer, test_buffer_append);
    tcase_add_test(tc_buffer, test_buffer_slicing);
    tcase_add_test(tc_buffer, test_buffer_stream);
    tcase_add_test(tc_buffer, test_buffer_c_string);
    tcase_add_test(tc_buffer, test_buffer_pretty_print);
    suite_add_tcase(s, tc_buffer);

    return s;
}


int
main(int argc, const char **argv)
{
    int  number_failed;
    Suite  *suite = test_suite();
    SRunner  *runner = srunner_create(suite);

    setup_allocator();
    srunner_run_all(runner, CK_NORMAL);
    number_failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    return (number_failed == 0)? EXIT_SUCCESS: EXIT_FAILURE;
}

/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <check.h>

#include "libcork/config.h"
#include "libcork/core/byte-order.h"
#include "libcork/core/error.h"
#include "libcork/core/hash.h"
#include "libcork/core/id.h"
#include "libcork/core/net-addresses.h"
#include "libcork/core/timestamp.h"
#include "libcork/core/types.h"
#include "libcork/core/u128.h"
#include "libcork/os/subprocess.h"

#include "helpers.h"


/*-----------------------------------------------------------------------
 * Core types
 */

START_TEST(test_bool)
{
    bool  value;

    value = true;
    fail_unless(value, "Unexpected true value");

    value = false;
    fail_if(value, "Unexpected false value");
}
END_TEST

START_TEST(test_int_types)
{
    /*
     * Make sure we have all of the C99 fixed-size integer types
     * available.
     */

#define TEST_INT_TYPE(type) \
    { \
        type  i = 0; \
        fail_unless(i == 0, "Unexpected value for " #type); \
    }

    TEST_INT_TYPE(int8_t);
    TEST_INT_TYPE(int16_t);
    TEST_INT_TYPE(int32_t);
    TEST_INT_TYPE(int64_t);
    TEST_INT_TYPE(uint8_t);
    TEST_INT_TYPE(uint16_t);
    TEST_INT_TYPE(uint32_t);
    TEST_INT_TYPE(uint64_t);
    TEST_INT_TYPE(size_t);
    TEST_INT_TYPE(ptrdiff_t);
    TEST_INT_TYPE(intptr_t);
    TEST_INT_TYPE(uintptr_t);

#undef TEST_INT_TYPE
}
END_TEST


START_TEST(test_int_sizeof)
{
    /*
     * Test that our CORK_SIZEOF_FOO preprocessor macros match the
     * results of the builtin sizeof operator.
     */

#define TEST_SIZEOF(TYPE, type) \
    { \
        fail_unless(CORK_SIZEOF_##TYPE == sizeof(type), \
                    "Incorrect size for " #type ": got %zu, expected %zu", \
                    (size_t) CORK_SIZEOF_##TYPE, \
                    (size_t) sizeof(type)); \
    }

    TEST_SIZEOF(SHORT, short)
    TEST_SIZEOF(SHORT, unsigned short)
    TEST_SIZEOF(INT, int)
    TEST_SIZEOF(INT, unsigned int)
    TEST_SIZEOF(LONG, long)
    TEST_SIZEOF(LONG, unsigned long)
    TEST_SIZEOF(POINTER, void *)
    TEST_SIZEOF(POINTER, int *)
    TEST_SIZEOF(POINTER, void (*)(void))

#undef TEST_SIZEOF
}
END_TEST


/*-----------------------------------------------------------------------
 * Strings
 */

static void
test_strndup(const char *string, size_t size)
{
    const char  *copy;

    copy = cork_strndup(string, size);
    if (memcmp(string, copy, size) != 0) {
        fail("cork_strndup failed");
    }
    cork_strfree(copy);

    copy = cork_xstrndup(string, size);
    fail_if(copy == NULL, "cork_xstrndup couldn't allocate copy");
    if (memcmp(string, copy, size) != 0) {
        fail("cork_xstrndup failed");
    }
    cork_strfree(copy);
}

START_TEST(test_string)
{
    DESCRIBE_TEST;
    test_strndup("", 0);
    test_strndup("abc", 3);
    test_strndup("abc\x00xyz", 7);
}
END_TEST


/*-----------------------------------------------------------------------
 * Endianness
 */

START_TEST(test_endianness)
{
#define TEST_ENDIAN(TYPE, type, sz, expected, ...) \
    { \
        union { uint8_t octets[sz]; type val; }  u = \
            { { __VA_ARGS__ } }; \
        \
        type  from_big = CORK_##TYPE##_BIG_TO_HOST(u.val); \
        fail_unless(from_big == expected, \
                    "Unexpected big-to-host " #type " value"); \
        \
        type  from_big_in_place = u.val; \
        CORK_##TYPE##_BIG_TO_HOST_IN_PLACE(from_big_in_place); \
        fail_unless(from_big_in_place == expected, \
                    "Unexpected in-place big-to-host " #type " value"); \
        \
        type  to_big = CORK_##TYPE##_HOST_TO_BIG(expected); \
        fail_unless(to_big == u.val, \
                    "Unexpected host-to-big " #type " value"); \
        \
        type  to_big_in_place = expected; \
        CORK_##TYPE##_HOST_TO_BIG_IN_PLACE(to_big_in_place); \
        fail_unless(to_big_in_place == u.val, \
                    "Unexpected in-place host-to-big " #type " value"); \
        \
        int  i; \
        for (i = 0; i < sz/2; i++) { \
            uint8_t  tmp = u.octets[i]; \
            u.octets[i] = u.octets[sz-i-1]; \
            u.octets[sz-i-1] = tmp; \
        } \
        \
        type  from_little = CORK_##TYPE##_LITTLE_TO_HOST(u.val); \
        fail_unless(from_little == expected, \
                    "Unexpected little-to-host " #type " value"); \
        \
        type  from_little_in_place = u.val; \
        CORK_##TYPE##_LITTLE_TO_HOST_IN_PLACE(from_little_in_place); \
        fail_unless(from_little_in_place == expected, \
                    "Unexpected in-place little-to-host " #type " value"); \
        \
        type  to_little = CORK_##TYPE##_HOST_TO_LITTLE(expected); \
        fail_unless(to_little == u.val, \
                    "Unexpected host-to-little " #type " value"); \
        \
        type  to_little_in_place = expected; \
        CORK_##TYPE##_HOST_TO_LITTLE_IN_PLACE(to_little_in_place); \
        fail_unless(to_little_in_place == u.val, \
                    "Unexpected in-place host-to-little " #type " value"); \
    }

    TEST_ENDIAN(UINT16, uint16_t, 2, 0x0102, 1, 2);
    TEST_ENDIAN(UINT32, uint32_t, 4, 0x01020304, 1, 2, 3, 4);
    TEST_ENDIAN(UINT64, uint64_t, 8, UINT64_C(0x0102030405060708),
                1, 2, 3, 4, 5, 6, 7, 8);

#undef TEST_ENDIAN
}
END_TEST


/*-----------------------------------------------------------------------
 * Built-in errors
 */

START_TEST(test_error_prefix)
{
    DESCRIBE_TEST;
    cork_error_clear();
    cork_error_set_printf
        (CORK_UNKNOWN_ERROR, "%u errors occurred", (unsigned int) 17);
    fail_unless_streq("Error messages",
                      "17 errors occurred",
                      cork_error_message());
    cork_error_prefix("The %s is aborting because ", "program");
    fail_unless_streq("Error messages",
                      "The program is aborting because 17 errors occurred",
                      cork_error_message());
    cork_error_clear();
}
END_TEST

START_TEST(test_system_error)
{
    DESCRIBE_TEST;
    /* Artificially flag a system error and make sure we can detect it */
    errno = ENOMEM;
    cork_error_clear();
    cork_system_error_set();
    fail_unless(cork_error_code() == ENOMEM,
                "Expected a system error");
    printf("Got error: %s\n", cork_error_message());
    cork_error_clear();
}
END_TEST


/*-----------------------------------------------------------------------
 * Hash values
 */

#define test_hash_func(func, expected, ...) \
    fail_unless(func(0, __VA_ARGS__) == expected, \
                "Unexpected hash value 0x%08" PRIx32 \
                " (expected 0x%08" PRIx32 ")", \
                func(0, __VA_ARGS__), expected);

#if CORK_HOST_ENDIANNESS == CORK_LITTLE_ENDIAN
#if CORK_SIZEOF_POINTER == 8
#define test_hash_buf(buf, len, little32, big32, little64, big64) \
    test_hash_func(cork_hash_buffer, little64, buf, len)
#define test_hash_var(var, little32, big32, little64, big64) \
    test_hash_func(cork_hash_variable, little64, var)
#else
#define test_hash_buf(buf, len, little32, big32, little64, big64) \
    test_hash_func(cork_hash_buffer, little32, buf, len)
#define test_hash_var(var, little32, big32, little64, big64) \
    test_hash_func(cork_hash_variable, little32, var)
#endif
#else
#if CORK_SIZEOF_POINTER == 8
#define test_hash_buf(buf, len, little32, big32, little64, big64) \
    test_hash_func(cork_hash_buffer, big64, buf, len)
#define test_hash_var(var, little32, big32, little64, big64) \
    test_hash_func(cork_hash_variable, big64, var)
#else
#define test_hash_buf(buf, len, little32, big32, little64, big64) \
    test_hash_func(cork_hash_buffer, big32, buf, len)
#define test_hash_var(var, little32, big32, little64, big64) \
    test_hash_func(cork_hash_variable, big32, var)
#endif
#endif


#define test_stable_hash_buf(buf, len, expected) \
    test_hash_func(cork_stable_hash_buffer, expected, buf, len)
#define test_stable_hash_var(var, expected) \
    test_hash_func(cork_stable_hash_variable, expected, var)


#define test_big_hash_func(buf, len, e1, e2) \
    do { \
        cork_big_hash  seed = CORK_BIG_HASH_INIT(); \
        cork_big_hash  expected = {cork_u128_from_64(e1, e2)}; \
        cork_big_hash  actual = cork_big_hash_buffer(seed, buf, len); \
        fail_unless(cork_big_hash_equal(actual, expected), \
                    "\nUnexpected hash value 0x%016" PRIx64 ".%016" PRIx64 \
                    "\n            (expected 0x%016" PRIx64 ".%016" PRIx64 ")", \
                    cork_u128_be64(actual.u128, 0), \
                    cork_u128_be64(actual.u128, 1), \
                    cork_u128_be64(expected.u128, 0), \
                    cork_u128_be64(expected.u128, 1)); \
    } while (0)

#if CORK_HOST_ENDIANNESS == CORK_LITTLE_ENDIAN
#if CORK_SIZEOF_POINTER == 8
#define test_big_hash_buf(buf,len,l32a,l32b,b32a,b32b,l64a,l64b,b64a,b64b) \
    test_big_hash_func(buf, len, l64a, l64b)
#else
#define test_big_hash_buf(buf,len,l32a,l32b,b32a,b32b,l64a,l64b,b64a,b64b) \
    test_big_hash_func(buf, len, l32a, l32b)
#endif
#else
#if CORK_SIZEOF_POINTER == 8
#define test_big_hash_buf(buf,len,l32a,l32b,b32a,b32b,l64a,l64b,b64a,b64b) \
    test_big_hash_func(buf, len, b64a, b64b)
#else
#define test_big_hash_buf(buf,len,l32a,l32b,b32a,b32b,l64a,l64b,b64a,b64b) \
    test_big_hash_func(buf, len, b32a, b32b)
#endif
#endif


START_TEST(test_hash)
{
    DESCRIBE_TEST;

    static const char  BUF[] = "test";
    static size_t  LEN = sizeof(BUF);
    static const char  LONG_BUF[] =
        "this is a much longer test string in the hopes that we have to "
        "go through a few iterations of the hashing loop in order to "
        "calculate the value of the hash which we are trying to compute.";
    static size_t  LONG_LEN = sizeof(LONG_BUF);
    uint32_t  val32 = 1234;
    uint64_t  val64 = 1234;
    uint32_t  stable_val32 = CORK_UINT32_HOST_TO_LITTLE(1234);
    uint64_t  stable_val64 = CORK_UINT64_HOST_TO_LITTLE(1234);

    /* without the NUL terminator */
    test_stable_hash_buf(BUF, LEN-1, 0xba6bd213);
    test_hash_buf(BUF, LEN-1,
      /* little 32 */ 0xba6bd213,
      /*    big 32 */ 0x29d175e5,
      /* little 64 */ 0xac7d28cc,
      /*    big 64 */ 0x74bde19d);
    test_big_hash_buf(BUF, LEN-1,
      /* little 32 */ 0x6f02ef30550c7d68LL, 0x550c7d68550c7d68LL,
      /*    big 32 */ 0x6f02ef30550c7d68LL, 0x550c7d68550c7d68LL,
      /* little 64 */ 0xac7d28cc74bde19dLL, 0x9a128231f9bd4d82LL,
      /*    big 64 */ 0xac7d28cc74bde19dLL, 0x9a128231f9bd4d82LL);

    /* with the NUL terminator */
    test_stable_hash_buf(BUF, LEN, 0x586fce33);
    test_hash_buf(BUF, LEN,
      /* little 32 */ 0x586fce33,
      /*    big 32 */ 0xe31d1ce0,
      /* little 64 */ 0xc3812fdf,
      /*    big 64 */ 0x4d18f852);
    test_big_hash_buf(BUF, LEN,
      /* little 32 */ 0x98c2b52b29ab177cLL, 0x29ab177c29ab177cLL,
      /*    big 32 */ 0x98c2b52b29ab177cLL, 0x29ab177c29ab177cLL,
      /* little 64 */ 0xc3812fdf4d18f852LL, 0xc81a9057aa737aecLL,
      /*    big 64 */ 0xc3812fdf4d18f852LL, 0xc81a9057aa737aecLL);

    /* without the NUL terminator */
    test_stable_hash_buf(LONG_BUF, LONG_LEN-1, 0x5caacc30);
    test_hash_buf(LONG_BUF, LONG_LEN-1,
      /* little 32 */ 0x5caacc30,
      /*    big 32 */ 0x88f94165,
      /* little 64 */ 0xcbdc2092,
      /*    big 64 */ 0x03578c96);
    test_big_hash_buf(LONG_BUF, LONG_LEN-1,
      /* little 32 */ 0x4240d5134fb7793cLL, 0xee7e281c799f335aLL,
      /*    big 32 */ 0xab564a5e029c92a4LL, 0x0bd80c741093400fLL,
      /* little 64 */ 0xcbdc20928fa72e9cLL, 0x48de52d2c680420eLL,
      /*    big 64 */ 0x5935f90a03578c96LL, 0x163e514fff9c30a8LL);

    /* with the NUL terminator */
    test_stable_hash_buf(LONG_BUF, LONG_LEN, 0x5e37d33d);
    test_hash_buf(LONG_BUF, LONG_LEN,
      /* little 32 */ 0x5e37d33d,
      /*    big 32 */ 0x4977421a,
      /* little 64 */ 0xe89ec005,
      /*    big 64 */ 0x8c919559);
    test_big_hash_buf(LONG_BUF, LONG_LEN,
      /* little 32 */ 0x63bcdcd0c2615146LL, 0x8e7fd7aaece3cab6LL,
      /*    big 32 */ 0x250b47cda3fc07fdLL, 0x840c4bb606aafbd0LL,
      /* little 64 */ 0xe89ec0054becb434LL, 0x826391b83f0b4d3eLL,
      /*    big 64 */ 0xf00a12ab8c919559LL, 0x684ecf4973c66eacLL);

    test_stable_hash_var(stable_val32, 0x6bb65380);
    test_hash_var(val32,
      /* little 32 */ 0x6bb65380,
      /*    big 32 */ 0x6bb65380,
      /* little 64 */ 0x061fecc8,
      /*    big 64 */ 0x7e1b3998);

    test_stable_hash_var(stable_val64, 0x4d5c4063);
    test_hash_var(val64,
      /* little 32 */ 0x4d5c4063,
      /*    big 32 */ 0xbaeee6e9,
      /* little 64 */ 0xb119ee69,
      /*    big 64 */ 0x267305fb);
}
END_TEST


/*-----------------------------------------------------------------------
 * IP addresses
 */

#define IPV4_TESTS(good, bad) \
    good("192.168.1.100", "192.168.1.100"); \
    good("01.002.0003.00000004", "1.2.3.4"); \
    good("010.0020.00034.00000089", "10.20.34.89"); \
    good("0100.000200.00.000", "100.200.0.0"); \
    bad("", -1); \
    bad(".", -1); \
    bad("192.168.0.", -1); \
    bad("192.168.0.1.", -1); \
    bad("192..168.0.1", -1); \
    bad("192.168.0.1.2", -1); \
    bad(".168.0.1.2", -1); \
    bad("256.0.0.0", -1); \
    bad("00256.0.0.0", -1); \
    bad("392.0.0.0", -1); \
    bad("1920.0.0.0", -1); \
    bad("stuv", -1); \

#define IPV6_TESTS(good, bad) \
    good("::", "::"); \
    good("0:0:0:0:0:0:0:0", "::"); \
    good("0000:0000:0000:0000:0000:0000:0000:0000", "::"); \
    good("fe80::", "fe80::"); \
    good("fe80:0:0:0:0:0:0:0", "fe80::"); \
    good("fe80:0000:0000:0000:0000:0000:0000:0000", "fe80::"); \
    good("::1", "::1"); \
    good("0:0:0:0:0:0:0:1", "::1"); \
    good("0000:0000:0000:0000:0000:0000:0000:0001", "::1"); \
    good("fe80::1", "fe80::1"); \
    good("fe80:0:0:0:0:0:0:1", "fe80::1"); \
    good("fe80:0000:0000:0000:0000:0000:0000:0001", "fe80::1"); \
    good("0:1:2:3:4:5:6:7", "0:1:2:3:4:5:6:7"); \
    good("1230:4567:89ab:cdef:1230:4567:89ab:cdef", \
         "1230:4567:89ab:cdef:1230:4567:89ab:cdef"); \
    good("::ffff:192.168.1.100", "::ffff:192.168.1.100"); \
    bad("", -1); \
    bad(":", -1); \
    bad("fe80:", -1); \
    bad("fe80::1::2", -1); \
    bad("1:2:3:4:5:6:7", -1); \
    bad("1:2:3:4:5:6:7:8:9", -1); \
    bad("::1:", -1); \
    bad("fe800::", -1); \
    bad("stuv", -1); \
    /* RFC 5952 recommendations */ \
    good("2001:0db8::0001", "2001:db8::1"); \
    good("2001:db8:0:0:0:0:2:1", "2001:db8::2:1"); \
    good("2001:db8:0:1:1:1:1:1", "2001:db8:0:1:1:1:1:1"); \
    good("2001:0:0:1:0:0:0:1", "2001:0:0:1::1"); \
    good("2001:db8:0:0:1:0:0:1", "2001:db8::1:0:0:1"); \
    good("0:1:A:B:C:D:E:F", "0:1:a:b:c:d:e:f"); \

START_TEST(test_ipv4_address)
{
    DESCRIBE_TEST;

#define GOOD(str, normalized) \
    { \
        struct cork_ipv4  addr; \
        fail_if_error(cork_ipv4_init(&addr, str)); \
        char  actual[CORK_IPV4_STRING_LENGTH]; \
        cork_ipv4_to_raw_string(&addr, actual); \
        fail_unless(strcmp(actual, normalized) == 0, \
                    "Unexpected string representation: " \
                    "got \"%s\", expected \"%s\"", \
                    actual, normalized); \
        \
        struct cork_ipv4  addr2; \
        cork_ipv4_init(&addr2, normalized); \
        fail_unless(cork_ipv4_equal(&addr, &addr2), \
                    "IPv4 instances should be equal"); \
    }

#define BAD(str, unused) \
    { \
        struct cork_ipv4  addr; \
        fail_unless_error \
            (cork_ipv4_init(&addr, str), \
             "Shouldn't be able to initialize IPv4 address from \"%s\"", \
             str); \
    }

    IPV4_TESTS(GOOD, BAD);
    IPV6_TESTS(BAD, BAD);

#undef GOOD
#undef BAD

    struct cork_ipv4  addr4;
    unsigned int  ipv4_cidr_good = 30;
    unsigned int  ipv4_cidr_bad_value = 24;
    unsigned int  ipv4_cidr_bad_range = 33;

    fprintf(stderr, "Testing network prefixes\n");
    cork_ipv4_init(&addr4, "1.2.3.4");
    fail_unless(cork_ipv4_is_valid_network(&addr4, ipv4_cidr_good),
                "Bad CIDR block for 1.2.3.4 and %u",
                ipv4_cidr_good);
    fail_if(cork_ipv4_is_valid_network(&addr4, ipv4_cidr_bad_value),
            "IPv4 CIDR check should fail for %u",
            ipv4_cidr_bad_value);
    fail_if(cork_ipv4_is_valid_network(&addr4, ipv4_cidr_bad_range),
            "IPv4 CIDR check should fail for %u",
            ipv4_cidr_bad_range);
}
END_TEST


START_TEST(test_ipv6_address)
{
    DESCRIBE_TEST;

#define GOOD(str, normalized) \
    { \
        struct cork_ipv6  addr; \
        fail_if_error(cork_ipv6_init(&addr, str)); \
        char  actual[CORK_IPV6_STRING_LENGTH]; \
        cork_ipv6_to_raw_string(&addr, actual); \
        fail_unless(strcmp(actual, normalized) == 0, \
                    "Unexpected string representation: " \
                    "got \"%s\", expected \"%s\"", \
                    actual, normalized); \
        \
        struct cork_ipv6  addr2; \
        cork_ipv6_init(&addr2, normalized); \
        fail_unless(cork_ipv6_equal(&addr, &addr2), \
                    "IPv6 instances should be equal"); \
    }

#define BAD(str, unused) \
    { \
        struct cork_ipv6  addr; \
        fail_unless_error \
            (cork_ipv6_init(&addr, str), \
             "Shouldn't be able to initialize IPv6 address from \"%s\"", \
             str); \
    }

    IPV6_TESTS(GOOD, BAD);
    IPV4_TESTS(BAD, BAD);

#undef GOOD
#undef BAD

    struct cork_ipv6  addr6;
    unsigned int  ipv6_cidr_good = 127;
    unsigned int  ipv6_cidr_bad_value = 64;
    unsigned int  ipv6_cidr_bad_range = 129;

    fprintf(stderr, "Testing network prefixes\n");
    cork_ipv6_init(&addr6, "fe80::200:f8ff:fe21:6000");
    fail_unless(cork_ipv6_is_valid_network(&addr6, ipv6_cidr_good),
                "Bad CIDR block %u",
                ipv6_cidr_good);
    fail_if(cork_ipv6_is_valid_network(&addr6, ipv6_cidr_bad_value),
            "IPv6 CIDR check should fail for %u",
            ipv6_cidr_bad_value);
    fail_if(cork_ipv6_is_valid_network(&addr6, ipv6_cidr_bad_range),
            "IPv6 CIDR check should fail for %u",
            ipv6_cidr_bad_range);
}
END_TEST


START_TEST(test_ip_address)
{
    DESCRIBE_TEST;
    struct cork_ip  addr;

#define GOOD(str, normalized) \
    { \
        struct cork_ip  addr; \
        fail_if_error(cork_ip_init(&addr, str)); \
        char  actual[CORK_IP_STRING_LENGTH]; \
        cork_ip_to_raw_string(&addr, actual); \
        fail_unless(strcmp(actual, normalized) == 0, \
                    "Unexpected string representation: " \
                    "got \"%s\", expected \"%s\"", \
                    actual, normalized); \
        \
        struct cork_ip  addr2; \
        cork_ip_init(&addr2, normalized); \
        fail_unless(cork_ip_equal(&addr, &addr2), \
                    "IP instances should be equal"); \
    }

#define BAD(str, unused) \
    { \
        struct cork_ip  addr; \
        fail_unless_error \
            (cork_ip_init(&addr, str), \
             "Shouldn't be able to initialize IP address from \"%s\"", \
             str); \
    }

    IPV4_TESTS(GOOD, BAD);
    IPV6_TESTS(GOOD, BAD);

#undef GOOD
#undef BAD

    struct cork_ipv4  addr4;
    struct cork_ipv6  addr6;

    fprintf(stderr, "Testing IP address versions\n");
    cork_ip_init(&addr, "192.168.1.1");
    cork_ipv4_init(&addr4, "192.168.1.1");
    fail_unless(addr.version == 4,
                "Unexpected IP address version (expected 4, got %u)",
                addr.version);
    fail_unless(cork_ipv4_equal(&addr.ip.v4, &addr4),
                "IP addresses should be equal");

    cork_ip_init(&addr, "fe80::1");
    cork_ipv6_init(&addr6, "fe80::1");
    fail_unless(addr.version == 6,
                "Unexpected IP address version (expected 6, got %u)",
                addr.version);
    fail_unless(cork_ipv6_equal(&addr.ip.v6, &addr6),
                "IP addresses should be equal");
}
END_TEST


/*-----------------------------------------------------------------------
 * Timestamps
 */

static void
test_timestamp_bad_format(cork_timestamp ts, const char *format)
{
    struct cork_buffer  buf = CORK_BUFFER_INIT();
    fail_unless_error(cork_timestamp_format_utc(ts, format, &buf));
    cork_buffer_done(&buf);
}

static void
test_timestamp_utc_format(cork_timestamp ts, const char *format,
                          const char *expected)
{
    struct cork_buffer  buf = CORK_BUFFER_INIT();
    fail_if_error(cork_timestamp_format_utc(ts, format, &buf));
    fail_unless(strcmp(buf.buf, expected) == 0,
                "Unexpected formatted UTC time "
                "(got \"%s\", expected \"%s\")",
                (char *) buf.buf, expected);
    cork_buffer_done(&buf);
}

static void
test_timestamp_local_format(cork_timestamp ts, const char *format,
                            const char *expected)
{
    struct cork_buffer  buf = CORK_BUFFER_INIT();
    fail_if_error(cork_timestamp_format_local(ts, format, &buf));
    fail_unless(strcmp(buf.buf, expected) == 0,
                "Unexpected formatted local time "
                "(got \"%s\", expected \"%s\")",
                (char *) buf.buf, expected);
    cork_buffer_done(&buf);
}

START_TEST(test_timestamp)
{
    /* All of the local times here are in America/Los_Angeles.  Down at the
     * bottom of the file we override the TZ environment variable to ensure that
     * we use a consistent local time zone in the test cases, regardless of the
     * actual time zone of the current machine. */

    static const uint32_t  TEST_TIME_1 = 700000000;
    static const char  *FORMATTED_UTC_TIME_1   = " 1992-03-07 20:26:40 ";
    static const char  *FORMATTED_LOCAL_TIME_1 = " 1992-03-07 12:26:40 ";

    static const uint32_t  TEST_TIME_2 = 1200000000;
    static const char  *FORMATTED_UTC_TIME_2   = " 2008-01-10 21:20:00 ";
    static const char  *FORMATTED_LOCAL_TIME_2 = " 2008-01-10 13:20:00 ";

    static const uint32_t  TEST_TIME_3 = 1305180745;
    static const char  *FORMATTED_UTC_TIME_3   = " 2011-05-12 06:12:25 ";
    static const char  *FORMATTED_LOCAL_TIME_3 = " 2011-05-11 23:12:25 ";

    cork_timestamp  ts;

    DESCRIBE_TEST;

#define test(unit, expected) \
    fail_unless(cork_timestamp_##unit(ts) == expected, \
                "Unexpected " #unit " portion of timestamp " \
                "(got %lu, expected %lu)", \
                (unsigned long) cork_timestamp_##unit(ts), \
                (unsigned long) expected);

#define test_format(utc, local) \
    test_timestamp_utc_format(ts, " %Y-%m-%d %H:%M:%S ", utc); \
    test_timestamp_local_format(ts, " %Y-%m-%d %H:%M:%S ", local);

    cork_timestamp_init_sec(&ts, TEST_TIME_1);
    test(sec, TEST_TIME_1);
    test(gsec, 0);
    test(msec, 0);
    test(usec, 0);
    test(nsec, 0);
    test_format(FORMATTED_UTC_TIME_1, FORMATTED_LOCAL_TIME_1);

    cork_timestamp_init_sec(&ts, TEST_TIME_2);
    test(sec, TEST_TIME_2);
    test(gsec, 0);
    test(msec, 0);
    test(usec, 0);
    test(nsec, 0);
    test_format(FORMATTED_UTC_TIME_2, FORMATTED_LOCAL_TIME_2);

    cork_timestamp_init_sec(&ts, TEST_TIME_3);
    test(sec, TEST_TIME_3);
    test(gsec, 0);
    test(msec, 0);
    test(usec, 0);
    test(nsec, 0);
    test_format(FORMATTED_UTC_TIME_3, FORMATTED_LOCAL_TIME_3);

    cork_timestamp_init_gsec(&ts, TEST_TIME_1, 1 << 30);
    test(sec, TEST_TIME_1);
    test(gsec, 1 << 30);
    test(msec, 250);
    test(usec, 250000);
    test(nsec, 250000000);

    cork_timestamp_init_msec(&ts, TEST_TIME_1, 500);
    test(sec, TEST_TIME_1);
    test(gsec, 1 << 31);
    test(msec, 500);
    test(usec, 500000);
    test(nsec, 500000000);

    cork_timestamp_init_usec(&ts, TEST_TIME_1, 500000);
    test(sec, TEST_TIME_1);
    test(gsec, 1 << 31);
    test(msec, 500);
    test(usec, 500000);
    test(nsec, 500000000);

    cork_timestamp_init_nsec(&ts, TEST_TIME_1, 500000000);
    test(sec, TEST_TIME_1);
    test(gsec, 1 << 31);
    test(msec, 500);
    test(usec, 500000);
    test(nsec, 500000000);
}
END_TEST

START_TEST(test_timestamp_format)
{
    cork_timestamp  ts;
    DESCRIBE_TEST;

    cork_timestamp_init_nsec(&ts, 0, 123456789);
    test_timestamp_bad_format(ts, "%f");
    test_timestamp_bad_format(ts, "%0f");
    test_timestamp_bad_format(ts, "%10f");
    test_timestamp_utc_format(ts, "%1f",   "1");
    test_timestamp_utc_format(ts, "%2f",   "12");
    test_timestamp_utc_format(ts, "%3f",   "123");
    test_timestamp_utc_format(ts, "%4f",   "1235");
    test_timestamp_utc_format(ts, "%5f",   "12346");
    test_timestamp_utc_format(ts, "%6f",   "123457");
    test_timestamp_utc_format(ts, "%7f",   "1234568");
    test_timestamp_utc_format(ts, "%8f",   "12345679");
    test_timestamp_utc_format(ts, "%9f",   "123456789");
    test_timestamp_utc_format(ts, "%009f", "123456789");

    cork_timestamp_init_nsec(&ts, 1200000000, 123456789);
}
END_TEST


/*-----------------------------------------------------------------------
 * 128-bit integers
 */

static void
test_one_u128_decimal(cork_u128 value, const char *expected)
{
    char  buf[CORK_U128_DECIMAL_LENGTH];
    const char  *actual = cork_u128_to_decimal(buf, value);
    fail_unless_streq("Integers", expected, actual);
}

static void
test_one_u128_hex(cork_u128 value, const char *expected)
{
    char  buf[CORK_U128_HEX_LENGTH];
    const char  *actual = cork_u128_to_hex(buf, value);
    fail_unless_streq("Integers", expected, actual);
}

static void
test_one_u128_padded_hex(cork_u128 value, const char *expected)
{
    char  buf[CORK_U128_HEX_LENGTH];
    const char  *actual = cork_u128_to_padded_hex(buf, value);
    fail_unless_streq("Integers", expected, actual);
}

static void
test_one_u128_print_from_32(uint32_t i0, uint32_t i1, uint32_t i2, uint32_t i3,
                            const char *expected_decimal,
                            const char *expected_hex,
                            const char *expected_padded_hex)
{
    cork_u128  value = cork_u128_from_32(i0, i1, i2, i3);
    test_one_u128_decimal(value, expected_decimal);
    test_one_u128_hex(value, expected_hex);
    test_one_u128_padded_hex(value, expected_padded_hex);
}

static void
test_one_u128_print_from_64(uint64_t i0, uint64_t i1,
                            const char *expected_decimal,
                            const char *expected_hex,
                            const char *expected_padded_hex)
{
    cork_u128  value = cork_u128_from_64(i0, i1);
    test_one_u128_decimal(value, expected_decimal);
    test_one_u128_hex(value, expected_hex);
    test_one_u128_padded_hex(value, expected_padded_hex);
}

START_TEST(test_u128_print)
{
    DESCRIBE_TEST;
    test_one_u128_print_from_32(
        0, 0, 0, 0,
        "0",
        "0",
        "00000000000000000000000000000000"
    );
    test_one_u128_print_from_32(
        0, 0, 0, 2,
        "2",
        "2",
        "00000000000000000000000000000002"
    );
    test_one_u128_print_from_32(
        0, 0, 0, 20,
        "20",
        "14",
        "00000000000000000000000000000014"
    );
    test_one_u128_print_from_32(
        0, 0, 0, 0xffffffff,
        "4294967295",
        "ffffffff",
        "000000000000000000000000ffffffff"
    );
    test_one_u128_print_from_32(
        0, 0, 1, 0,
        "4294967296",
        "100000000",
        "00000000000000000000000100000000"
    );
    test_one_u128_print_from_32(
        0, 0, 0xffffffff, 0xffffffff,
        "18446744073709551615",
        "ffffffffffffffff",
        "0000000000000000ffffffffffffffff"
    );
    test_one_u128_print_from_32(
        0, 1, 0, 0,
        "18446744073709551616",
        "10000000000000000",
        "00000000000000010000000000000000"
    );
    test_one_u128_print_from_64(
        0, 0,
        "0",
        "0",
        "00000000000000000000000000000000"
    );
    test_one_u128_print_from_64(
        0, 2,
        "2",
        "2",
        "00000000000000000000000000000002"
    );
    test_one_u128_print_from_64(
        0, 20,
        "20",
        "14",
        "00000000000000000000000000000014"
    );
    test_one_u128_print_from_64(
        0, UINT64_C(0xffffffffffffffff),
        "18446744073709551615",
        "ffffffffffffffff",
        "0000000000000000ffffffffffffffff"
    );
    test_one_u128_print_from_64(
        1, 0,
        "18446744073709551616",
        "10000000000000000",
        "00000000000000010000000000000000"
    );
}
END_TEST


static void
test_one_u128_add(uint64_t i0, uint64_t i1, uint64_t j0, uint64_t j1,
                  const char *expected)
{
    cork_u128  value1 = cork_u128_from_64(i0, i1);
    cork_u128  value2 = cork_u128_from_64(j0, j1);
    cork_u128  sum = cork_u128_add(value1, value2);
    test_one_u128_decimal(sum, expected);
}

START_TEST(test_u128_add)
{
    DESCRIBE_TEST;
    test_one_u128_add(0, 0, 0, 0, "0");
    test_one_u128_add(0, 1, 0, 1, "2");
    test_one_u128_add(0, 1, 0, 0xffffffff, "4294967296");
    test_one_u128_add(0, 1, 0xffffffffffffffffLL, 0xffffffffffffffffLL, "0");
}
END_TEST


static void
test_one_u128_sub(uint64_t i0, uint64_t i1, uint64_t j0, uint64_t j1,
                  const char *expected)
{
    cork_u128  value1 = cork_u128_from_64(i0, i1);
    cork_u128  value2 = cork_u128_from_64(j0, j1);
    cork_u128  diff = cork_u128_sub(value1, value2);
    test_one_u128_decimal(diff, expected);
}

START_TEST(test_u128_sub)
{
    DESCRIBE_TEST;
    test_one_u128_sub(0, 0, 0, 0, "0");
    test_one_u128_sub(0, 1, 0, 1, "0");
    test_one_u128_sub(0, 2, 0, 1, "1");
    test_one_u128_sub(0, UINT64_C(0x100000000), 0, 1, "4294967295");
    test_one_u128_sub(1, 0, 0, 1, "18446744073709551615");
    test_one_u128_sub(0, 1, 0, 2, "340282366920938463463374607431768211455");
}
END_TEST


#define test_u128_cmp(op, op_str, v1, v2, expected) \
    do { \
        bool  actual = cork_u128_##op((v1), (v2)); \
        fail_unless(actual == (expected), \
                    "%" PRIu64 ":%" PRIu64 \
                    " should %sbe " op_str " " \
                    "%" PRIu64 ":%" PRIu64, \
                    cork_u128_be64((v1), 0), cork_u128_be64((v1), 1), \
                    (expected)? "": "not ", \
                    cork_u128_be64((v2), 0), cork_u128_be64((v2), 1)); \
    } while (0)

static void
test_one_u128_eq(uint64_t i0, uint64_t i1, uint64_t j0, uint64_t j1,
                 bool expected)
{
    cork_u128  value1 = cork_u128_from_64(i0, i1);
    cork_u128  value2 = cork_u128_from_64(j0, j1);
    test_u128_cmp(eq, "==", value1, value2, expected);
    test_u128_cmp(ne, "!=", value1, value2, !expected);
}

START_TEST(test_u128_eq)
{
    DESCRIBE_TEST;
    test_one_u128_eq(0, 0, 0, 0, true);
    test_one_u128_eq(0, 0, 0, 1, false);
    test_one_u128_eq(0, 2, 0, 1, false);
    test_one_u128_eq(0, 1, 0, UINT64_C(0x100000000), false);
    test_one_u128_eq(0, UINT64_C(0x100000000), 0, UINT64_C(0x100000000), true);
}
END_TEST

static void
test_one_u128_lt(uint64_t i0, uint64_t i1, uint64_t j0, uint64_t j1,
                 bool expected)
{
    cork_u128  value1 = cork_u128_from_64(i0, i1);
    cork_u128  value2 = cork_u128_from_64(j0, j1);
    test_u128_cmp(lt, "<",  value1, value2, expected);
    test_u128_cmp(ge, ">=", value1, value2, !expected);
}

START_TEST(test_u128_lt)
{
    DESCRIBE_TEST;
    test_one_u128_lt(0, 0, 0, 0, false);
    test_one_u128_lt(0, 0, 0, 1, true);
    test_one_u128_lt(0, 2, 0, 1, false);
    test_one_u128_lt(0, 1, 0, UINT64_C(0x100000000), true);
}
END_TEST

static void
test_one_u128_gt(uint64_t i0, uint64_t i1, uint64_t j0, uint64_t j1,
                 bool expected)
{
    cork_u128  value1 = cork_u128_from_64(i0, i1);
    cork_u128  value2 = cork_u128_from_64(j0, j1);
    test_u128_cmp(gt, ">",  value1, value2, expected);
    test_u128_cmp(le, "<=", value1, value2, !expected);
}

START_TEST(test_u128_gt)
{
    DESCRIBE_TEST;
    test_one_u128_gt(0, 0, 0, 0, false);
    test_one_u128_gt(0, 1, 0, 0, true);
    test_one_u128_gt(0, 1, 0, 2, false);
    test_one_u128_gt(0, UINT64_C(0x100000000), 0, 1, true);
}
END_TEST


/*-----------------------------------------------------------------------
 * Statement expressions
 */

START_TEST(test_statement_expr)
{
#if CORK_CONFIG_HAVE_GCC_STATEMENT_EXPRS
    int  value = ({ int __x = 0; __x += 2; __x;});
    fail_unless_equal("Statement expression result", "%d", 2, value);
#endif
}
END_TEST


/*-----------------------------------------------------------------------
 * Unique identifiers
 */

START_TEST(test_uid)
{
    DESCRIBE_TEST;
    cork_uid_define(test_id_01);
    cork_uid_define(test_id_02);
    cork_uid  id1;
    cork_uid  id2;

    fail_unless_streq("UID name", "test_id_01", cork_uid_name(test_id_01));
    fail_unless_streq("UID name", "test_id_02", cork_uid_name(test_id_02));

    id1 = test_id_01;
    id2 = test_id_02;
    fail_if(cork_uid_equal(id1, id2), "Unique IDs aren't unique");

    id1 = test_id_01;
    id2 = test_id_01;
    fail_unless(cork_uid_equal(id1, id2), "Unique ID isn't equal to itself");

    id1 = test_id_01;
    id2 = CORK_UID_NONE;
    fail_if(cork_uid_equal(id1, id2), "NULL unique ID isn't unique");
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("core");

    TCase  *tc_types = tcase_create("types");
    tcase_add_test(tc_types, test_bool);
    tcase_add_test(tc_types, test_int_types);
    tcase_add_test(tc_types, test_int_sizeof);
    suite_add_tcase(s, tc_types);

    TCase  *tc_string = tcase_create("string");
    tcase_add_test(tc_string, test_string);
    suite_add_tcase(s, tc_string);

    TCase  *tc_endianness = tcase_create("endianness");
    tcase_add_test(tc_endianness, test_endianness);
    suite_add_tcase(s, tc_endianness);

    TCase  *tc_errors = tcase_create("errors");
    tcase_add_test(tc_errors, test_error_prefix);
    tcase_add_test(tc_errors, test_system_error);
    suite_add_tcase(s, tc_errors);

    TCase  *tc_hash = tcase_create("hash");
    tcase_add_test(tc_hash, test_hash);
    suite_add_tcase(s, tc_hash);

    TCase  *tc_addresses = tcase_create("net-addresses");
    tcase_add_test(tc_addresses, test_ipv4_address);
    tcase_add_test(tc_addresses, test_ipv6_address);
    tcase_add_test(tc_addresses, test_ip_address);
    suite_add_tcase(s, tc_addresses);

    TCase  *tc_timestamp = tcase_create("timestamp");
    tcase_add_test(tc_timestamp, test_timestamp);
    tcase_add_test(tc_timestamp, test_timestamp_format);
    suite_add_tcase(s, tc_timestamp);

    TCase  *tc_u128 = tcase_create("u128");
    tcase_add_test(tc_u128, test_u128_print);
    tcase_add_test(tc_u128, test_u128_add);
    tcase_add_test(tc_u128, test_u128_sub);
    tcase_add_test(tc_u128, test_u128_eq);
    tcase_add_test(tc_u128, test_u128_lt);
    tcase_add_test(tc_u128, test_u128_gt);
    suite_add_tcase(s, tc_u128);

    TCase  *tc_statement_expr = tcase_create("statement_expr");
    tcase_add_test(tc_statement_expr, test_statement_expr);
    suite_add_tcase(s, tc_statement_expr);

    TCase  *tc_uid = tcase_create("uid");
    tcase_add_test(tc_uid, test_uid);
    suite_add_tcase(s, tc_uid);

    return s;
}


int
main(int argc, const char **argv)
{
    int  number_failed;
    Suite  *suite = test_suite();
    SRunner  *runner = srunner_create(suite);

    setup_allocator();

    /* Before anything starts, override the TZ environment variable so that we
     * get consistent test results. */
    cork_env_add(NULL, "TZ", "America/Los_Angeles");

    srunner_run_all(runner, CK_NORMAL);
    number_failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    return (number_failed == 0)? EXIT_SUCCESS: EXIT_FAILURE;
}

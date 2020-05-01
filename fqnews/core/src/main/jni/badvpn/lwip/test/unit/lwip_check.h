#ifndef LWIP_HDR_LWIP_CHECK_H
#define LWIP_HDR_LWIP_CHECK_H

/* Common header file for lwIP unit tests using the check framework */

#include <config.h>
#include <check.h>
#include <stdlib.h>

#define FAIL_RET() do { fail(); return; } while(0)
#define EXPECT(x) fail_unless(x)
#define EXPECT_RET(x) do { fail_unless(x); if(!(x)) { return; }} while(0)
#define EXPECT_RETX(x, y) do { fail_unless(x); if(!(x)) { return y; }} while(0)
#define EXPECT_RETNULL(x) EXPECT_RETX(x, NULL)

typedef struct {
  TFun func;
  const char *name;
} testfunc;

#define TESTFUNC(x) {(x), "" # x "" }

/* Modified function from check.h, supplying function name */
#define tcase_add_named_test(tc,tf) \
   _tcase_add_test((tc),(tf).func,(tf).name,0, 0, 0, 1)

/** typedef for a function returning a test suite */
typedef Suite* (suite_getter_fn)(void);

/** Create a test suite */
Suite* create_suite(const char* name, testfunc *tests, size_t num_tests, SFun setup, SFun teardown);

#ifdef LWIP_UNITTESTS_LIB
int lwip_unittests_run(void)
#endif

/* helper functions */
#define SKIP_POOL(x) (1 << x)
#define SKIP_HEAP    (1 << MEMP_MAX)
void lwip_check_ensure_no_alloc(unsigned int skip);

#endif /* LWIP_HDR_LWIP_CHECK_H */

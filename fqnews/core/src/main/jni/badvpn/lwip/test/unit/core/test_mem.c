#include "test_mem.h"

#include "lwip/mem.h"
#include "lwip/stats.h"

#if !LWIP_STATS || !MEM_STATS
#error "This tests needs MEM-statistics enabled"
#endif
#if LWIP_DNS
#error "This test needs DNS turned off (as it mallocs on init)"
#endif

/* Setups/teardown functions */

static void
mem_setup(void)
{
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}

static void
mem_teardown(void)
{
  lwip_check_ensure_no_alloc(SKIP_POOL(MEMP_SYS_TIMEOUT));
}


/* Test functions */

/** Call mem_malloc, mem_free and mem_trim and check stats */
START_TEST(test_mem_one)
{
#define SIZE1   16
#define SIZE1_2 12
#define SIZE2   16
  void *p1, *p2;
  mem_size_t s1, s2;
  LWIP_UNUSED_ARG(_i);

  fail_unless(lwip_stats.mem.used == 0);

  p1 = mem_malloc(SIZE1);
  fail_unless(p1 != NULL);
  fail_unless(lwip_stats.mem.used >= SIZE1);
  s1 = lwip_stats.mem.used;

  p2 = mem_malloc(SIZE2);
  fail_unless(p2 != NULL);
  fail_unless(lwip_stats.mem.used >= SIZE2 + s1);
  s2 = lwip_stats.mem.used;

  mem_trim(p1, SIZE1_2);

  mem_free(p2);
  fail_unless(lwip_stats.mem.used <= s2 - SIZE2);

  mem_free(p1);
  fail_unless(lwip_stats.mem.used == 0);
}
END_TEST

static void malloc_keep_x(int x, int num, int size, int freestep)
{
   int i;
   void* p[16];
   LWIP_ASSERT("invalid size", size >= 0 && size < (mem_size_t)-1);
   memset(p, 0, sizeof(p));
   for(i = 0; i < num && i < 16; i++) {
      p[i] = mem_malloc((mem_size_t)size);
      fail_unless(p[i] != NULL);
   }
   for(i = 0; i < num && i < 16; i += freestep) {
      if (i == x) {
         continue;
      }
      mem_free(p[i]);
      p[i] = NULL;
   }
   for(i = 0; i < num && i < 16; i++) {
      if (i == x) {
         continue;
      }
      if (p[i] != NULL) {
         mem_free(p[i]);
         p[i] = NULL;
      }
   }
   fail_unless(p[x] != NULL);
   mem_free(p[x]);
}

START_TEST(test_mem_random)
{
  const int num = 16;
  int x;
  int size;
  int freestep;
  LWIP_UNUSED_ARG(_i);

  fail_unless(lwip_stats.mem.used == 0);

  for (x = 0; x < num; x++) {
     for (size = 1; size < 32; size++) {
        for (freestep = 1; freestep <= 3; freestep++) {
          fail_unless(lwip_stats.mem.used == 0);
          malloc_keep_x(x, num, size, freestep);
          fail_unless(lwip_stats.mem.used == 0);
        }
     }
  }
}
END_TEST

START_TEST(test_mem_invalid_free)
{
  u8_t *ptr, *ptr_low, *ptr_high;
  LWIP_UNUSED_ARG(_i);

  fail_unless(lwip_stats.mem.used == 0);
  fail_unless(lwip_stats.mem.illegal == 0);

  ptr = (u8_t *)mem_malloc(1);
  fail_unless(ptr != NULL);
  fail_unless(lwip_stats.mem.used != 0);

  ptr_low = ptr - 0x10;
  mem_free(ptr_low);
  fail_unless(lwip_stats.mem.illegal == 1);
  lwip_stats.mem.illegal = 0;

  ptr_high = ptr + (MEM_SIZE * 2);
  mem_free(ptr_high);
  fail_unless(lwip_stats.mem.illegal == 1);
  lwip_stats.mem.illegal = 0;

  mem_free(ptr);
  fail_unless(lwip_stats.mem.illegal == 0);
  fail_unless(lwip_stats.mem.used == 0);
}
END_TEST

START_TEST(test_mem_double_free)
{
  u8_t *ptr1b, *ptr1, *ptr2, *ptr3;
  LWIP_UNUSED_ARG(_i);

  fail_unless(lwip_stats.mem.used == 0);
  fail_unless(lwip_stats.mem.illegal == 0);

  ptr1 = (u8_t *)mem_malloc(1);
  fail_unless(ptr1 != NULL);
  fail_unless(lwip_stats.mem.used != 0);

  ptr2 = (u8_t *)mem_malloc(1);
  fail_unless(ptr2 != NULL);
  fail_unless(lwip_stats.mem.used != 0);

  ptr3 = (u8_t *)mem_malloc(1);
  fail_unless(ptr3 != NULL);
  fail_unless(lwip_stats.mem.used != 0);

  /* free the middle mem */
  mem_free(ptr2);
  fail_unless(lwip_stats.mem.illegal == 0);

  /* double-free of middle mem: should fail */
  mem_free(ptr2);
  fail_unless(lwip_stats.mem.illegal == 1);
  lwip_stats.mem.illegal = 0;

  /* free upper memory and try again */
  mem_free(ptr3);
  fail_unless(lwip_stats.mem.illegal == 0);

  mem_free(ptr2);
  fail_unless(lwip_stats.mem.illegal == 1);
  lwip_stats.mem.illegal = 0;

  /* free lower memory and try again */
  mem_free(ptr1);
  fail_unless(lwip_stats.mem.illegal == 0);
  fail_unless(lwip_stats.mem.used == 0);

  mem_free(ptr2);
  fail_unless(lwip_stats.mem.illegal == 1);
  fail_unless(lwip_stats.mem.used == 0);
  lwip_stats.mem.illegal = 0;

  /* reallocate lowest memory, now overlapping already freed ptr2 */
#ifndef MIN_SIZE
#define MIN_SIZE 12
#endif
  ptr1b = (u8_t *)mem_malloc(MIN_SIZE * 2);
  fail_unless(ptr1b != NULL);
  fail_unless(lwip_stats.mem.used != 0);

  mem_free(ptr2);
  fail_unless(lwip_stats.mem.illegal == 1);
  lwip_stats.mem.illegal = 0;

  memset(ptr1b, 1, MIN_SIZE * 2);

  mem_free(ptr2);
  fail_unless(lwip_stats.mem.illegal == 1);
  lwip_stats.mem.illegal = 0;

  mem_free(ptr1b);
  fail_unless(lwip_stats.mem.illegal == 0);
  fail_unless(lwip_stats.mem.used == 0);
}
END_TEST

/** Create the suite including all tests for this module */
Suite *
mem_suite(void)
{
  testfunc tests[] = {
    TESTFUNC(test_mem_one),
    TESTFUNC(test_mem_random),
    TESTFUNC(test_mem_invalid_free),
    TESTFUNC(test_mem_double_free)
  };
  return create_suite("MEM", tests, sizeof(tests)/sizeof(testfunc), mem_setup, mem_teardown);
}

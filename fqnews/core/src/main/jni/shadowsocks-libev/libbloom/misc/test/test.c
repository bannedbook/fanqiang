/*
 *  Copyright (c) 2012-2016, Jyri J. Virkki
 *  All rights reserved.
 *
 *  This file is under BSD license. See LICENSE file.
 */

#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "bloom.h"

#ifdef __linux
#include <sys/time.h>
#include <time.h>
#endif


/** ***************************************************************************
 * A few simple tests to check if it works at all.
 *
 */
static int basic()
{
  printf("----- basic -----\n");

  struct bloom bloom;

  assert(bloom_init(&bloom, 0, 1.0) == 1);
  assert(bloom_init(&bloom, 10, 0) == 1);
  assert(bloom.ready == 0);
  assert(bloom_add(&bloom, "hello world", 11) == -1);
  assert(bloom_check(&bloom, "hello world", 11) == -1);
  bloom_free(&bloom);

  assert(bloom_init(&bloom, 102, 0.1) == 0);
  assert(bloom.ready == 1);
  bloom_print(&bloom);

  assert(bloom_check(&bloom, "hello world", 11) == 0);
  assert(bloom_add(&bloom, "hello world", 11) == 0);
  assert(bloom_check(&bloom, "hello world", 11) == 1);
  assert(bloom_add(&bloom, "hello world", 11) > 0);
  assert(bloom_add(&bloom, "hello", 5) == 0);
  assert(bloom_add(&bloom, "hello", 5) > 0);
  assert(bloom_check(&bloom, "hello", 5) == 1);
  bloom_free(&bloom);

  return 0;
}


/** ***************************************************************************
 * Create a bloom filter with given parameters and add 'count' random elements
 * into it to see if collission rates are within expectations.
 *
 */
static int add_random(int entries, double error, int count,
                      int quiet, int check_error, uint8_t elem_size, int validate)
{
  if (!quiet) {
    printf("----- add_random(%d, %f, %d, %d, %d, %d, %d) -----\n",
           entries, error, count, quiet, check_error, elem_size, validate);
  }

  struct bloom bloom;
  assert(bloom_init(&bloom, entries, error) == 0);
  if (!quiet) { bloom_print(&bloom); }

  char block[elem_size];
  uint8_t * saved = NULL;
  uint8_t * savedp = NULL;
  int collisions = 0;
  int n;

  int fd = open("/dev/urandom", O_RDONLY);
  if (fd < 0) {
    printf("error: unable to open /dev/random\n");
    exit(1);
  }

  if (validate) {
    saved = (uint8_t *)malloc(elem_size * count);
    if (!saved) {
      printf("error: unable to allocate buffer for validation\n");
      exit(1);
    }
    savedp = saved;
  }

  for (n = 0; n < count; n++) {
    assert(read(fd, block, elem_size) == elem_size);
    memcpy(savedp, block, elem_size);
    savedp += elem_size;
    if (bloom_add(&bloom, (void *)block, elem_size)) { collisions++; }
  }
  close(fd);

  double er = (double)collisions / (double)count;

  if (!quiet) {
    printf("entries: %d, error: %f, count: %d, coll: %d, error: %f, "
           "bytes: %d\n",
           entries, error, count, collisions, er, bloom.bytes);
  } else {
    printf("%d %f %d %d %f %d\n",
           entries, error, count, collisions, er, bloom.bytes);
  }

  if (check_error && er > error) {
    printf("error: expected error %f but observed %f\n", error, er);
    exit(1);
  }

  if (validate) {
    for (n = 0; n < count; n++) {
      if (!bloom_check(&bloom, saved + (n * elem_size), elem_size)) {
        printf("error: data saved in filter is not there!\n");
        exit(1);
      }
    }
  }

  bloom_free(&bloom);
  if (saved) { free(saved); }
  return 0;
}


/** ***************************************************************************
 * Simple loop to compare performance.
 *
 */
static int perf_loop(int entries, int count)
{
  printf("----- perf_loop -----\n");

  struct bloom bloom;
  assert(bloom_init(&bloom, entries, 0.001) == 0);
  bloom_print(&bloom);

  int i;
  int collisions = 0;

  struct timeval tp;
  gettimeofday(&tp, NULL);
  long before = (tp.tv_sec * 1000L) + (tp.tv_usec / 1000L);

  for (i = 0; i < count; i++) {
    if (bloom_add(&bloom, (void *)&i, sizeof(int))) { collisions++; }
  }

  gettimeofday(&tp, NULL);
  long after = (tp.tv_sec * 1000L) + (tp.tv_usec / 1000L);

  printf("Added %d elements of size %d, took %d ms (collisions=%d)\n",
         count, (int)sizeof(int), (int)(after - before), collisions);

  printf("%d,%d,%ld\n", entries, bloom.bytes, after - before);

  bloom_print(&bloom);
  bloom_free(&bloom);

  return 0;
}


/** ***************************************************************************
 * Default set of basic tests.
 *
 * These should run reasonably quick so they can be run all the time.
 *
 */
static int basic_tests()
{
  int rv = 0;

  rv += basic();
  rv += add_random(10, 0.1, 10, 0, 1, 32, 1);
  rv += add_random(10000, 0.1, 10000, 0, 1, 32, 1);
  rv += add_random(10000, 0.01, 10000, 0, 1, 32, 1);
  rv += add_random(10000, 0.001, 10000, 0, 1, 32, 1);
  rv += add_random(10000, 0.0001, 10000, 0, 1, 32, 1);
  rv += add_random(1000000, 0.0001, 1000000, 0, 1, 32, 1);

  printf("\nBrought to you by libbloom-%s\n", bloom_version());

  return 0;
}


/** ***************************************************************************
 * Some longer-running tests.
 *
 */
static int larger_tests()
{
  int rv = 0;
  int e;

  printf("\nAdd 10M elements and verify (0.00001)\n");
  rv += add_random(10000000, 0.00001, 10000000, 0, 1, 32, 1);

  printf("\nChecking collision rates with filters from 100K to 1M (0.001)\n");
  for (e = 100000; e <= 1000000; e+= 100) {
    rv += add_random(e, 0.001, e, 1, 1, 8, 1);
  }

  return rv;
}


/** ***************************************************************************
 * With no options, runs brief default tests.
 *
 * With -L, runs some longer-running tests.
 *
 * To test collisions over a range of sizes: -G START END INCREMENT ERROR
 * This produces output that can be graphed with collisions/dograph
 * See also collision_test make target.
 *
 * To test collisions, run with options: -c ENTRIES ERROR COUNT
 * Where 'ENTRIES' is the expected number of entries used to initialize the
 * bloom filter and 'ERROR' is the acceptable probability of collision
 * used to initialize the bloom filter. 'COUNT' is the actual number of
 * entries inserted.
 *
 * To test performance only, run with options:  -p ENTRIES COUNT
 * Where 'ENTRIES' is the expected number of entries used to initialize the
 * bloom filter and 'COUNT' is the actual number of entries inserted.
 *
 */
int main(int argc, char **argv)
{
  // Calls return() instead of exit() just to make valgrind mark as
  // an error any reachable allocations. That makes them show up
  // when running the tests.

  int rv = 0;

  if (argc == 1) {
    printf("----- Running basic tests -----\n");
    rv = basic_tests();
    printf("----- DONE Running basic tests -----\n");
    return rv;
  }

  if (!strncmp(argv[1], "-L", 2)) {
    return larger_tests();
  }

  if (!strncmp(argv[1], "-G", 2)) {
    if (argc != 6) {
      printf("-G START END INCREMENT ERROR\n");
      return 1;
    }
    int e;
    for (e = atoi(argv[2]); e <= atoi(argv[3]); e+= atoi(argv[4])) {
      rv += add_random(e, atof(argv[5]), e, 1, 0, 32, 1);
    }
    return rv;
  }

  if (!strncmp(argv[1], "-c", 2)) {
    if (argc != 5) {
      printf("-c ENTRIES ERROR COUNT\n");
      return 1;
    }

    return add_random(atoi(argv[2]), atof(argv[3]), atoi(argv[4]), 0, 1, 32, 1);
  }

  if (!strncmp(argv[1], "-p", 2)) {
    if (argc != 4) {
      printf("-p ENTRIES COUNT\n");
    }
    return perf_loop(atoi(argv[2]), atoi(argv[3]));
  }

  return rv;
}

// Copyright 2009 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <chrono>

#include "util/benchmark.h"
#include "util/flags.h"
#include "re2/re2.h"

#ifdef _WIN32
#define snprintf _snprintf
#endif

using ::testing::Benchmark;

static Benchmark* benchmarks[10000];
static int nbenchmarks;

void Benchmark::Register() {
  lo_ = std::max(1, lo_);
  hi_ = std::max(lo_, hi_);
  benchmarks[nbenchmarks++] = this;
}

static int64_t nsec() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

static int64_t t0;
static int64_t ns;
static int64_t bytes;
static int64_t items;

void StartBenchmarkTiming() {
  if (t0 == 0) {
    t0 = nsec();
  }
}

void StopBenchmarkTiming() {
  if (t0 != 0) {
    ns += nsec() - t0;
    t0 = 0;
  }
}

void SetBenchmarkBytesProcessed(int64_t b) { bytes = b; }

void SetBenchmarkItemsProcessed(int64_t i) { items = i; }

static void RunFunc(Benchmark* b, int iters, int arg) {
  t0 = nsec();
  ns = 0;
  bytes = 0;
  items = 0;
  b->func()(iters, arg);
  StopBenchmarkTiming();
}

static int round(int n) {
  int base = 1;
  while (base * 10 < n) base *= 10;
  if (n < 2 * base) return 2 * base;
  if (n < 5 * base) return 5 * base;
  return 10 * base;
}

static void RunBench(Benchmark* b, int arg) {
  int iters, last;

  // Run once just in case it's expensive.
  iters = 1;
  RunFunc(b, iters, arg);
  while (ns < (int)1e9 && iters < (int)1e9) {
    last = iters;
    if (ns / iters == 0) {
      iters = (int)1e9;
    } else {
      iters = (int)1e9 / static_cast<int>(ns / iters);
    }
    iters = std::max(last + 1, std::min(iters + iters / 2, 100 * last));
    iters = round(iters);
    RunFunc(b, iters, arg);
  }

  char mb[100];
  char suf[100];
  mb[0] = '\0';
  suf[0] = '\0';
  if (ns > 0 && bytes > 0)
    snprintf(mb, sizeof mb, "\t%7.2f MB/s",
             ((double)bytes / 1e6) / ((double)ns / 1e9));
  if (b->has_arg()) {
    if (arg >= (1 << 20)) {
      snprintf(suf, sizeof suf, "/%dM", arg / (1 << 20));
    } else if (arg >= (1 << 10)) {
      snprintf(suf, sizeof suf, "/%dK", arg / (1 << 10));
    } else {
      snprintf(suf, sizeof suf, "/%d", arg);
    }
  }
  printf("%s%s\t%8d\t%10lld ns/op%s\n", b->name(), suf, iters,
         (long long)ns / iters, mb);
  fflush(stdout);
}

static bool WantBench(const char* name, int argc, const char** argv) {
  if (argc == 1) return true;
  for (int i = 1; i < argc; i++) {
    if (RE2::PartialMatch(name, argv[i]))
      return true;
  }
  return false;
}

int main(int argc, const char** argv) {
  for (int i = 0; i < nbenchmarks; i++) {
    Benchmark* b = benchmarks[i];
    if (!WantBench(b->name(), argc, argv))
      continue;
    for (int arg = b->lo(); arg <= b->hi(); arg <<= 1)
      RunBench(b, arg);
  }
}

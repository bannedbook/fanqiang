// Copyright 2009 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef UTIL_BENCHMARK_H_
#define UTIL_BENCHMARK_H_

#include <stdint.h>
#include <functional>

#include "util/logging.h"
#include "util/util.h"

// Globals for the old benchmark API.
void StartBenchmarkTiming();
void StopBenchmarkTiming();
void SetBenchmarkBytesProcessed(int64_t b);
void SetBenchmarkItemsProcessed(int64_t i);

namespace benchmark {

// The new benchmark API implemented as a layer over the old benchmark API.
// (Please refer to https://github.com/google/benchmark for documentation.)
class State {
 private:
  class Iterator {
   public:
    // Benchmark code looks like this:
    //
    //   for (auto _ : state) {
    //     // ...
    //   }
    //
    // We try to avoid compiler warnings about such variables being unused.
    struct ATTRIBUTE_UNUSED Value {};

    explicit Iterator(int64_t iters) : iters_(iters) {}

    bool operator!=(const Iterator& that) const {
      if (iters_ != that.iters_) {
        return true;
      } else {
        // We are about to stop the loop, so stop timing.
        StopBenchmarkTiming();
        return false;
      }
    }

    Value operator*() const {
      return Value();
    }

    Iterator& operator++() {
      --iters_;
      return *this;
    }

   private:
    int64_t iters_;
  };

 public:
  explicit State(int64_t iters)
      : iters_(iters), arg_(0), has_arg_(false) {}

  State(int64_t iters, int64_t arg)
      : iters_(iters), arg_(arg), has_arg_(true) {}

  Iterator begin() {
    // We are about to start the loop, so start timing.
    StartBenchmarkTiming();
    return Iterator(iters_);
  }

  Iterator end() {
    return Iterator(0);
  }

  void SetBytesProcessed(int64_t b) { SetBenchmarkBytesProcessed(b); }
  void SetItemsProcessed(int64_t i) { SetBenchmarkItemsProcessed(i); }
  int64_t iterations() const { return iters_; }
  // Pretend to support multiple arguments.
  int64_t range(int pos) const { CHECK(has_arg_); return arg_; }

 private:
  int64_t iters_;
  int64_t arg_;
  bool has_arg_;

  State(const State&) = delete;
  State& operator=(const State&) = delete;
};

}  // namespace benchmark

namespace testing {

class Benchmark {
 public:
  Benchmark(const char* name, void (*func)(benchmark::State&))
      : name_(name),
        func_([func](int iters, int arg) {
          benchmark::State state(iters);
          func(state);
        }),
        lo_(0),
        hi_(0),
        has_arg_(false) {
    Register();
  }

  Benchmark(const char* name, void (*func)(benchmark::State&), int lo, int hi)
      : name_(name),
        func_([func](int iters, int arg) {
          benchmark::State state(iters, arg);
          func(state);
        }),
        lo_(lo),
        hi_(hi),
        has_arg_(true) {
    Register();
  }

  // Pretend to support multiple threads.
  Benchmark* ThreadRange(int lo, int hi) { return this; }

  const char* name() const { return name_; }
  const std::function<void(int, int)>& func() const { return func_; }
  int lo() const { return lo_; }
  int hi() const { return hi_; }
  bool has_arg() const { return has_arg_; }

 private:
  void Register();

  const char* name_;
  std::function<void(int, int)> func_;
  int lo_;
  int hi_;
  bool has_arg_;

  Benchmark(const Benchmark&) = delete;
  Benchmark& operator=(const Benchmark&) = delete;
};

}  // namespace testing

#define BENCHMARK(f)                     \
  ::testing::Benchmark* _benchmark_##f = \
      (new ::testing::Benchmark(#f, f))

#define BENCHMARK_RANGE(f, lo, hi)       \
  ::testing::Benchmark* _benchmark_##f = \
      (new ::testing::Benchmark(#f, f, lo, hi))

#endif  // UTIL_BENCHMARK_H_

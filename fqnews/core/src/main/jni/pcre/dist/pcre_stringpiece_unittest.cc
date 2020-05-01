// Copyright 2003 and onwards Google Inc.
// Author: Sanjay Ghemawat

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <map>
#include <algorithm>    // for make_pair

#include "pcrecpp.h"
#include "pcre_stringpiece.h"

// CHECK dies with a fatal error if condition is not true.  It is *not*
// controlled by NDEBUG, so the check will be executed regardless of
// compilation mode.  Therefore, it is safe to do things like:
//    CHECK(fp->Write(x) == 4)
#define CHECK(condition) do {                           \
  if (!(condition)) {                                   \
    fprintf(stderr, "%s:%d: Check failed: %s\n",        \
            __FILE__, __LINE__, #condition);            \
    exit(1);                                            \
  }                                                     \
} while (0)

using pcrecpp::StringPiece;

static void CheckSTLComparator() {
  string s1("foo");
  string s2("bar");
  string s3("baz");

  StringPiece p1(s1);
  StringPiece p2(s2);
  StringPiece p3(s3);

  typedef std::map<StringPiece, int> TestMap;
  TestMap map;

  map.insert(std::make_pair(p1, 0));
  map.insert(std::make_pair(p2, 1));
  map.insert(std::make_pair(p3, 2));

  CHECK(map.size() == 3);

  TestMap::const_iterator iter = map.begin();
  CHECK(iter->second == 1);
  ++iter;
  CHECK(iter->second == 2);
  ++iter;
  CHECK(iter->second == 0);
  ++iter;
  CHECK(iter == map.end());

  TestMap::iterator new_iter = map.find("zot");
  CHECK(new_iter == map.end());

  new_iter = map.find("bar");
  CHECK(new_iter != map.end());

  map.erase(new_iter);
  CHECK(map.size() == 2);

  iter = map.begin();
  CHECK(iter->second == 2);
  ++iter;
  CHECK(iter->second == 0);
  ++iter;
  CHECK(iter == map.end());
}

static void CheckComparisonOperators() {
#define CMP_Y(op, x, y)                                         \
  CHECK( (StringPiece((x)) op StringPiece((y))));               \
  CHECK( (StringPiece((x)).compare(StringPiece((y))) op 0))

#define CMP_N(op, x, y)                                         \
  CHECK(!(StringPiece((x)) op StringPiece((y))));               \
  CHECK(!(StringPiece((x)).compare(StringPiece((y))) op 0))

  CMP_Y(==, "",   "");
  CMP_Y(==, "a",  "a");
  CMP_Y(==, "aa", "aa");
  CMP_N(==, "a",  "");
  CMP_N(==, "",   "a");
  CMP_N(==, "a",  "b");
  CMP_N(==, "a",  "aa");
  CMP_N(==, "aa", "a");

  CMP_N(!=, "",   "");
  CMP_N(!=, "a",  "a");
  CMP_N(!=, "aa", "aa");
  CMP_Y(!=, "a",  "");
  CMP_Y(!=, "",   "a");
  CMP_Y(!=, "a",  "b");
  CMP_Y(!=, "a",  "aa");
  CMP_Y(!=, "aa", "a");

  CMP_Y(<, "a",  "b");
  CMP_Y(<, "a",  "aa");
  CMP_Y(<, "aa", "b");
  CMP_Y(<, "aa", "bb");
  CMP_N(<, "a",  "a");
  CMP_N(<, "b",  "a");
  CMP_N(<, "aa", "a");
  CMP_N(<, "b",  "aa");
  CMP_N(<, "bb", "aa");

  CMP_Y(<=, "a",  "a");
  CMP_Y(<=, "a",  "b");
  CMP_Y(<=, "a",  "aa");
  CMP_Y(<=, "aa", "b");
  CMP_Y(<=, "aa", "bb");
  CMP_N(<=, "b",  "a");
  CMP_N(<=, "aa", "a");
  CMP_N(<=, "b",  "aa");
  CMP_N(<=, "bb", "aa");

  CMP_N(>=, "a",  "b");
  CMP_N(>=, "a",  "aa");
  CMP_N(>=, "aa", "b");
  CMP_N(>=, "aa", "bb");
  CMP_Y(>=, "a",  "a");
  CMP_Y(>=, "b",  "a");
  CMP_Y(>=, "aa", "a");
  CMP_Y(>=, "b",  "aa");
  CMP_Y(>=, "bb", "aa");

  CMP_N(>, "a",  "a");
  CMP_N(>, "a",  "b");
  CMP_N(>, "a",  "aa");
  CMP_N(>, "aa", "b");
  CMP_N(>, "aa", "bb");
  CMP_Y(>, "b",  "a");
  CMP_Y(>, "aa", "a");
  CMP_Y(>, "b",  "aa");
  CMP_Y(>, "bb", "aa");

#undef CMP_Y
#undef CMP_N
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  CheckComparisonOperators();
  CheckSTLComparator();

  printf("OK\n");
  return 0;
}

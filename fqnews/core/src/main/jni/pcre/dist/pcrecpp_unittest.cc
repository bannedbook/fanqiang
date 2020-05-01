// -*- coding: utf-8 -*-
//
// Copyright (c) 2005 - 2010, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Sanjay Ghemawat
//
// TODO: Test extractions for PartialMatch/Consume

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>      /* for memset and strcmp */
#include <cassert>
#include <vector>
#include "pcrecpp.h"

using pcrecpp::StringPiece;
using pcrecpp::RE;
using pcrecpp::RE_Options;
using pcrecpp::Hex;
using pcrecpp::Octal;
using pcrecpp::CRadix;

static bool VERBOSE_TEST  = false;

// CHECK dies with a fatal error if condition is not true.  It is *not*
// controlled by NDEBUG, so the check will be executed regardless of
// compilation mode.  Therefore, it is safe to do things like:
//    CHECK_EQ(fp->Write(x), 4)
#define CHECK(condition) do {                           \
  if (!(condition)) {                                   \
    fprintf(stderr, "%s:%d: Check failed: %s\n",        \
            __FILE__, __LINE__, #condition);            \
    exit(1);                                            \
  }                                                     \
} while (0)

#define CHECK_EQ(a, b)   CHECK(a == b)

static void Timing1(int num_iters) {
  // Same pattern lots of times
  RE pattern("ruby:\\d+");
  StringPiece p("ruby:1234");
  for (int j = num_iters; j > 0; j--) {
    CHECK(pattern.FullMatch(p));
  }
}

static void Timing2(int num_iters) {
  // Same pattern lots of times
  RE pattern("ruby:(\\d+)");
  int i;
  for (int j = num_iters; j > 0; j--) {
    CHECK(pattern.FullMatch("ruby:1234", &i));
    CHECK_EQ(i, 1234);
  }
}

static void Timing3(int num_iters) {
  string text_string;
  for (int j = num_iters; j > 0; j--) {
    text_string += "this is another line\n";
  }

  RE line_matcher(".*\n");
  string line;
  StringPiece text(text_string);
  int counter = 0;
  while (line_matcher.Consume(&text)) {
    counter++;
  }
  printf("Matched %d lines\n", counter);
}

#if 0  // uncomment this if you have a way of defining VirtualProcessSize()

static void LeakTest() {
  // Check for memory leaks
  unsigned long long initial_size = 0;
  for (int i = 0; i < 100000; i++) {
    if (i == 50000) {
      initial_size = VirtualProcessSize();
      printf("Size after 50000: %llu\n", initial_size);
    }
    char buf[100];  // definitely big enough
    sprintf(buf, "pat%09d", i);
    RE newre(buf);
  }
  uint64 final_size = VirtualProcessSize();
  printf("Size after 100000: %llu\n", final_size);
  const double growth = double(final_size - initial_size) / final_size;
  printf("Growth: %0.2f%%", growth * 100);
  CHECK(growth < 0.02);       // Allow < 2% growth
}

#endif

static void RadixTests() {
  printf("Testing hex\n");

#define CHECK_HEX(type, value) \
  do { \
    type v; \
    CHECK(RE("([0-9a-fA-F]+)[uUlL]*").FullMatch(#value, Hex(&v))); \
    CHECK_EQ(v, 0x ## value); \
    CHECK(RE("([0-9a-fA-FxX]+)[uUlL]*").FullMatch("0x" #value, CRadix(&v))); \
    CHECK_EQ(v, 0x ## value); \
  } while(0)

  CHECK_HEX(short,              2bad);
  CHECK_HEX(unsigned short,     2badU);
  CHECK_HEX(int,                dead);
  CHECK_HEX(unsigned int,       deadU);
  CHECK_HEX(long,               7eadbeefL);
  CHECK_HEX(unsigned long,      deadbeefUL);
#ifdef HAVE_LONG_LONG
  CHECK_HEX(long long,          12345678deadbeefLL);
#endif
#ifdef HAVE_UNSIGNED_LONG_LONG
  CHECK_HEX(unsigned long long, cafebabedeadbeefULL);
#endif

#undef CHECK_HEX

  printf("Testing octal\n");

#define CHECK_OCTAL(type, value) \
  do { \
    type v; \
    CHECK(RE("([0-7]+)[uUlL]*").FullMatch(#value, Octal(&v))); \
    CHECK_EQ(v, 0 ## value); \
    CHECK(RE("([0-9a-fA-FxX]+)[uUlL]*").FullMatch("0" #value, CRadix(&v))); \
    CHECK_EQ(v, 0 ## value); \
  } while(0)

  CHECK_OCTAL(short,              77777);
  CHECK_OCTAL(unsigned short,     177777U);
  CHECK_OCTAL(int,                17777777777);
  CHECK_OCTAL(unsigned int,       37777777777U);
  CHECK_OCTAL(long,               17777777777L);
  CHECK_OCTAL(unsigned long,      37777777777UL);
#ifdef HAVE_LONG_LONG
  CHECK_OCTAL(long long,          777777777777777777777LL);
#endif
#ifdef HAVE_UNSIGNED_LONG_LONG
  CHECK_OCTAL(unsigned long long, 1777777777777777777777ULL);
#endif

#undef CHECK_OCTAL

  printf("Testing decimal\n");

#define CHECK_DECIMAL(type, value) \
  do { \
    type v; \
    CHECK(RE("(-?[0-9]+)[uUlL]*").FullMatch(#value, &v)); \
    CHECK_EQ(v, value); \
    CHECK(RE("(-?[0-9a-fA-FxX]+)[uUlL]*").FullMatch(#value, CRadix(&v))); \
    CHECK_EQ(v, value); \
  } while(0)

  CHECK_DECIMAL(short,              -1);
  CHECK_DECIMAL(unsigned short,     9999);
  CHECK_DECIMAL(int,                -1000);
  CHECK_DECIMAL(unsigned int,       12345U);
  CHECK_DECIMAL(long,               -10000000L);
  CHECK_DECIMAL(unsigned long,      3083324652U);
#ifdef HAVE_LONG_LONG
  CHECK_DECIMAL(long long,          -100000000000000LL);
#endif
#ifdef HAVE_UNSIGNED_LONG_LONG
  CHECK_DECIMAL(unsigned long long, 1234567890987654321ULL);
#endif

#undef CHECK_DECIMAL

}

static void TestReplace() {
  printf("Testing Replace\n");

  struct ReplaceTest {
    const char *regexp;
    const char *rewrite;
    const char *original;
    const char *single;
    const char *global;
    int global_count;         // the expected return value from ReplaceAll
  };
  static const ReplaceTest tests[] = {
    { "(qu|[b-df-hj-np-tv-z]*)([a-z]+)",
      "\\2\\1ay",
      "the quick brown fox jumps over the lazy dogs.",
      "ethay quick brown fox jumps over the lazy dogs.",
      "ethay ickquay ownbray oxfay umpsjay overay ethay azylay ogsday.",
      9 },
    { "\\w+",
      "\\0-NOSPAM",
      "paul.haahr@google.com",
      "paul-NOSPAM.haahr@google.com",
      "paul-NOSPAM.haahr-NOSPAM@google-NOSPAM.com-NOSPAM",
      4 },
    { "^",
      "(START)",
      "foo",
      "(START)foo",
      "(START)foo",
      1 },
    { "^",
      "(START)",
      "",
      "(START)",
      "(START)",
      1 },
    { "$",
      "(END)",
      "",
      "(END)",
      "(END)",
      1 },
    { "b",
      "bb",
      "ababababab",
      "abbabababab",
      "abbabbabbabbabb",
       5 },
    { "b",
      "bb",
      "bbbbbb",
      "bbbbbbb",
      "bbbbbbbbbbbb",
      6 },
    { "b+",
      "bb",
      "bbbbbb",
      "bb",
      "bb",
      1 },
    { "b*",
      "bb",
      "bbbbbb",
      "bb",
      "bbbb",
      2 },
    { "b*",
      "bb",
      "aaaaa",
      "bbaaaaa",
      "bbabbabbabbabbabb",
      6 },
    { "b*",
      "bb",
      "aa\naa\n",
      "bbaa\naa\n",
      "bbabbabb\nbbabbabb\nbb",
      7 },
    { "b*",
      "bb",
      "aa\raa\r",
      "bbaa\raa\r",
      "bbabbabb\rbbabbabb\rbb",
      7 },
    { "b*",
      "bb",
      "aa\r\naa\r\n",
      "bbaa\r\naa\r\n",
      "bbabbabb\r\nbbabbabb\r\nbb",
      7 },
    // Check empty-string matching (it's tricky!)
    { "aa|b*",
      "@",
      "aa",
      "@",
      "@@",
      2 },
    { "b*|aa",
      "@",
      "aa",
      "@aa",
      "@@@",
      3 },
#ifdef SUPPORT_UTF8
    { "b*",
      "bb",
      "\xE3\x83\x9B\xE3\x83\xBC\xE3\x83\xA0\xE3\x81\xB8",   // utf8
      "bb\xE3\x83\x9B\xE3\x83\xBC\xE3\x83\xA0\xE3\x81\xB8",
      "bb\xE3\x83\x9B""bb""\xE3\x83\xBC""bb""\xE3\x83\xA0""bb""\xE3\x81\xB8""bb",
      5 },
    { "b*",
      "bb",
      "\xE3\x83\x9B\r\n\xE3\x83\xBC\r\xE3\x83\xA0\n\xE3\x81\xB8\r\n",   // utf8
      "bb\xE3\x83\x9B\r\n\xE3\x83\xBC\r\xE3\x83\xA0\n\xE3\x81\xB8\r\n",
      ("bb\xE3\x83\x9B""bb\r\nbb""\xE3\x83\xBC""bb\rbb""\xE3\x83\xA0"
       "bb\nbb""\xE3\x81\xB8""bb\r\nbb"),
      9 },
#endif
    { "", NULL, NULL, NULL, NULL, 0 }
  };

#ifdef SUPPORT_UTF8
  const bool support_utf8 = true;
#else
  const bool support_utf8 = false;
#endif

  for (const ReplaceTest *t = tests; t->original != NULL; ++t) {
    RE re(t->regexp, RE_Options(PCRE_NEWLINE_CRLF).set_utf8(support_utf8));
    assert(re.error().empty());
    string one(t->original);
    CHECK(re.Replace(t->rewrite, &one));
    CHECK_EQ(one, t->single);
    string all(t->original);
    const int replace_count = re.GlobalReplace(t->rewrite, &all);
    CHECK_EQ(all, t->global);
    CHECK_EQ(replace_count, t->global_count);
  }

  // One final test: test \r\n replacement when we're not in CRLF mode
  {
    RE re("b*", RE_Options(PCRE_NEWLINE_CR).set_utf8(support_utf8));
    assert(re.error().empty());
    string all("aa\r\naa\r\n");
    CHECK_EQ(re.GlobalReplace("bb", &all), 9);
    CHECK_EQ(all, string("bbabbabb\rbb\nbbabbabb\rbb\nbb"));
  }
  {
    RE re("b*", RE_Options(PCRE_NEWLINE_LF).set_utf8(support_utf8));
    assert(re.error().empty());
    string all("aa\r\naa\r\n");
    CHECK_EQ(re.GlobalReplace("bb", &all), 9);
    CHECK_EQ(all, string("bbabbabb\rbb\nbbabbabb\rbb\nbb"));
  }
  // TODO: test what happens when no PCRE_NEWLINE_* flag is set.
  //       Alas, the answer depends on how pcre was compiled.
}

static void TestExtract() {
  printf("Testing Extract\n");

  string s;

  CHECK(RE("(.*)@([^.]*)").Extract("\\2!\\1", "boris@kremvax.ru", &s));
  CHECK_EQ(s, "kremvax!boris");

  // check the RE interface as well
  CHECK(RE(".*").Extract("'\\0'", "foo", &s));
  CHECK_EQ(s, "'foo'");
  CHECK(!RE("bar").Extract("'\\0'", "baz", &s));
  CHECK_EQ(s, "'foo'");
}

static void TestConsume() {
  printf("Testing Consume\n");

  string word;

  string s("   aaa b!@#$@#$cccc");
  StringPiece input(s);

  RE r("\\s*(\\w+)");    // matches a word, possibly proceeded by whitespace
  CHECK(r.Consume(&input, &word));
  CHECK_EQ(word, "aaa");
  CHECK(r.Consume(&input, &word));
  CHECK_EQ(word, "b");
  CHECK(! r.Consume(&input, &word));
}

static void TestFindAndConsume() {
  printf("Testing FindAndConsume\n");

  string word;

  string s("   aaa b!@#$@#$cccc");
  StringPiece input(s);

  RE r("(\\w+)");      // matches a word
  CHECK(r.FindAndConsume(&input, &word));
  CHECK_EQ(word, "aaa");
  CHECK(r.FindAndConsume(&input, &word));
  CHECK_EQ(word, "b");
  CHECK(r.FindAndConsume(&input, &word));
  CHECK_EQ(word, "cccc");
  CHECK(! r.FindAndConsume(&input, &word));
}

static void TestMatchNumberPeculiarity() {
  printf("Testing match-number peculiarity\n");

  string word1;
  string word2;
  string word3;

  RE r("(foo)|(bar)|(baz)");
  CHECK(r.PartialMatch("foo", &word1, &word2, &word3));
  CHECK_EQ(word1, "foo");
  CHECK_EQ(word2, "");
  CHECK_EQ(word3, "");
  CHECK(r.PartialMatch("bar", &word1, &word2, &word3));
  CHECK_EQ(word1, "");
  CHECK_EQ(word2, "bar");
  CHECK_EQ(word3, "");
  CHECK(r.PartialMatch("baz", &word1, &word2, &word3));
  CHECK_EQ(word1, "");
  CHECK_EQ(word2, "");
  CHECK_EQ(word3, "baz");
  CHECK(!r.PartialMatch("f", &word1, &word2, &word3));

  string a;
  CHECK(RE("(foo)|hello").FullMatch("hello", &a));
  CHECK_EQ(a, "");
}

static void TestRecursion() {
  printf("Testing recursion\n");

  // Get one string that passes (sometimes), one that never does.
  string text_good("abcdefghijk");
  string text_bad("acdefghijkl");

  // According to pcretest, matching text_good against (\w+)*b
  // requires match_limit of at least 8192, and match_recursion_limit
  // of at least 37.

  RE_Options options_ml;
  options_ml.set_match_limit(8192);
  RE re("(\\w+)*b", options_ml);
  CHECK(re.PartialMatch(text_good) == true);
  CHECK(re.PartialMatch(text_bad) == false);
  CHECK(re.FullMatch(text_good) == false);
  CHECK(re.FullMatch(text_bad) == false);

  options_ml.set_match_limit(1024);
  RE re2("(\\w+)*b", options_ml);
  CHECK(re2.PartialMatch(text_good) == false);   // because of match_limit
  CHECK(re2.PartialMatch(text_bad) == false);
  CHECK(re2.FullMatch(text_good) == false);
  CHECK(re2.FullMatch(text_bad) == false);

  RE_Options options_mlr;
  options_mlr.set_match_limit_recursion(50);
  RE re3("(\\w+)*b", options_mlr);
  CHECK(re3.PartialMatch(text_good) == true);
  CHECK(re3.PartialMatch(text_bad) == false);
  CHECK(re3.FullMatch(text_good) == false);
  CHECK(re3.FullMatch(text_bad) == false);

  options_mlr.set_match_limit_recursion(10);
  RE re4("(\\w+)*b", options_mlr);
  CHECK(re4.PartialMatch(text_good) == false);
  CHECK(re4.PartialMatch(text_bad) == false);
  CHECK(re4.FullMatch(text_good) == false);
  CHECK(re4.FullMatch(text_bad) == false);
}

// A meta-quoted string, interpreted as a pattern, should always match
// the original unquoted string.
static void TestQuoteMeta(string unquoted, RE_Options options = RE_Options()) {
  string quoted = RE::QuoteMeta(unquoted);
  RE re(quoted, options);
  CHECK(re.FullMatch(unquoted));
}

// A string containing meaningful regexp characters, which is then meta-
// quoted, should not generally match a string the unquoted string does.
static void NegativeTestQuoteMeta(string unquoted, string should_not_match,
                                  RE_Options options = RE_Options()) {
  string quoted = RE::QuoteMeta(unquoted);
  RE re(quoted, options);
  CHECK(!re.FullMatch(should_not_match));
}

// Tests that quoted meta characters match their original strings,
// and that a few things that shouldn't match indeed do not.
static void TestQuotaMetaSimple() {
  TestQuoteMeta("foo");
  TestQuoteMeta("foo.bar");
  TestQuoteMeta("foo\\.bar");
  TestQuoteMeta("[1-9]");
  TestQuoteMeta("1.5-2.0?");
  TestQuoteMeta("\\d");
  TestQuoteMeta("Who doesn't like ice cream?");
  TestQuoteMeta("((a|b)c?d*e+[f-h]i)");
  TestQuoteMeta("((?!)xxx).*yyy");
  TestQuoteMeta("([");
  TestQuoteMeta(string("foo\0bar", 7));
}

static void TestQuoteMetaSimpleNegative() {
  NegativeTestQuoteMeta("foo", "bar");
  NegativeTestQuoteMeta("...", "bar");
  NegativeTestQuoteMeta("\\.", ".");
  NegativeTestQuoteMeta("\\.", "..");
  NegativeTestQuoteMeta("(a)", "a");
  NegativeTestQuoteMeta("(a|b)", "a");
  NegativeTestQuoteMeta("(a|b)", "(a)");
  NegativeTestQuoteMeta("(a|b)", "a|b");
  NegativeTestQuoteMeta("[0-9]", "0");
  NegativeTestQuoteMeta("[0-9]", "0-9");
  NegativeTestQuoteMeta("[0-9]", "[9]");
  NegativeTestQuoteMeta("((?!)xxx)", "xxx");
}

static void TestQuoteMetaLatin1() {
  TestQuoteMeta("3\xb2 = 9");
}

static void TestQuoteMetaUtf8() {
#ifdef SUPPORT_UTF8
  TestQuoteMeta("Pl\xc3\xa1\x63ido Domingo", pcrecpp::UTF8());
  TestQuoteMeta("xyz", pcrecpp::UTF8());            // No fancy utf8
  TestQuoteMeta("\xc2\xb0", pcrecpp::UTF8());       // 2-byte utf8 (degree symbol)
  TestQuoteMeta("27\xc2\xb0 degrees", pcrecpp::UTF8());  // As a middle character
  TestQuoteMeta("\xe2\x80\xb3", pcrecpp::UTF8());   // 3-byte utf8 (double prime)
  TestQuoteMeta("\xf0\x9d\x85\x9f", pcrecpp::UTF8()); // 4-byte utf8 (music note)
  TestQuoteMeta("27\xc2\xb0"); // Interpreted as Latin-1, but should still work
  NegativeTestQuoteMeta("27\xc2\xb0",               // 2-byte utf (degree symbol)
                        "27\\\xc2\\\xb0",
                        pcrecpp::UTF8());
#endif
}

static void TestQuoteMetaAll() {
  printf("Testing QuoteMeta\n");
  TestQuotaMetaSimple();
  TestQuoteMetaSimpleNegative();
  TestQuoteMetaLatin1();
  TestQuoteMetaUtf8();
}

//
// Options tests contributed by
// Giuseppe Maxia, CTO, Stardata s.r.l.
// July 2005
//
static void GetOneOptionResult(
                const char *option_name,
                const char *regex,
                const char *str,
                RE_Options options,
                bool full,
                string expected) {

  printf("Testing Option <%s>\n", option_name);
  if(VERBOSE_TEST)
    printf("/%s/ finds \"%s\" within \"%s\" \n",
                    regex,
                    expected.c_str(),
                    str);
  string captured("");
  if (full)
    RE(regex,options).FullMatch(str, &captured);
  else
    RE(regex,options).PartialMatch(str, &captured);
  CHECK_EQ(captured, expected);
}

static void TestOneOption(
                const char *option_name,
                const char *regex,
                const char *str,
                RE_Options options,
                bool full,
                bool assertive = true) {

  printf("Testing Option <%s>\n", option_name);
  if (VERBOSE_TEST)
    printf("'%s' %s /%s/ \n",
                  str,
                  (assertive? "matches" : "doesn't match"),
                  regex);
  if (assertive) {
    if (full)
      CHECK(RE(regex,options).FullMatch(str));
    else
      CHECK(RE(regex,options).PartialMatch(str));
  } else {
    if (full)
      CHECK(!RE(regex,options).FullMatch(str));
    else
      CHECK(!RE(regex,options).PartialMatch(str));
  }
}

static void Test_CASELESS() {
  RE_Options options;
  RE_Options options2;

  options.set_caseless(true);
  TestOneOption("CASELESS (class)",  "HELLO",    "hello", options, false);
  TestOneOption("CASELESS (class2)", "HELLO",    "hello", options2.set_caseless(true), false);
  TestOneOption("CASELESS (class)",  "^[A-Z]+$", "Hello", options, false);

  TestOneOption("CASELESS (function)", "HELLO",    "hello", pcrecpp::CASELESS(), false);
  TestOneOption("CASELESS (function)", "^[A-Z]+$", "Hello", pcrecpp::CASELESS(), false);
  options.set_caseless(false);
  TestOneOption("no CASELESS", "HELLO",    "hello", options, false, false);
}

static void Test_MULTILINE() {
  RE_Options options;
  RE_Options options2;
  const char *str = "HELLO\n" "cruel\n" "world\n";

  options.set_multiline(true);
  TestOneOption("MULTILINE (class)",    "^cruel$", str, options, false);
  TestOneOption("MULTILINE (class2)",   "^cruel$", str, options2.set_multiline(true), false);
  TestOneOption("MULTILINE (function)", "^cruel$", str, pcrecpp::MULTILINE(), false);
  options.set_multiline(false);
  TestOneOption("no MULTILINE", "^cruel$", str, options, false, false);
}

static void Test_DOTALL() {
  RE_Options options;
  RE_Options options2;
  const char *str = "HELLO\n" "cruel\n" "world";

  options.set_dotall(true);
  TestOneOption("DOTALL (class)",    "HELLO.*world", str, options, true);
  TestOneOption("DOTALL (class2)",   "HELLO.*world", str, options2.set_dotall(true), true);
  TestOneOption("DOTALL (function)",    "HELLO.*world", str, pcrecpp::DOTALL(), true);
  options.set_dotall(false);
  TestOneOption("no DOTALL", "HELLO.*world", str, options, true, false);
}

static void Test_DOLLAR_ENDONLY() {
  RE_Options options;
  RE_Options options2;
  const char *str = "HELLO world\n";

  TestOneOption("no DOLLAR_ENDONLY", "world$", str, options, false);
  options.set_dollar_endonly(true);
  TestOneOption("DOLLAR_ENDONLY 1",    "world$", str, options, false, false);
  TestOneOption("DOLLAR_ENDONLY 2",    "world$", str, options2.set_dollar_endonly(true), false, false);
}

static void Test_EXTRA() {
  RE_Options options;
  const char *str = "HELLO";

  options.set_extra(true);
  TestOneOption("EXTRA 1", "\\HELL\\O", str, options, true, false );
  TestOneOption("EXTRA 2", "\\HELL\\O", str, RE_Options().set_extra(true), true, false );
  options.set_extra(false);
  TestOneOption("no EXTRA", "\\HELL\\O", str, options, true );
}

static void Test_EXTENDED() {
  RE_Options options;
  RE_Options options2;
  const char *str = "HELLO world";

  options.set_extended(true);
  TestOneOption("EXTENDED (class)",    "HELLO world", str, options, false, false);
  TestOneOption("EXTENDED (class2)",   "HELLO world", str, options2.set_extended(true), false, false);
  TestOneOption("EXTENDED (class)",
                    "^ HE L{2} O "
                    "\\s+        "
                    "\\w+ $      ",
                    str,
                    options,
                    false);

  TestOneOption("EXTENDED (function)",    "HELLO world", str, pcrecpp::EXTENDED(), false, false);
  TestOneOption("EXTENDED (function)",
                    "^ HE L{2} O "
                    "\\s+        "
                    "\\w+ $      ",
                    str,
                    pcrecpp::EXTENDED(),
                    false);

  options.set_extended(false);
  TestOneOption("no EXTENDED", "HELLO world", str, options, false);
}

static void Test_NO_AUTO_CAPTURE() {
  RE_Options options;
  const char *str = "HELLO world";
  string captured;

  printf("Testing Option <no NO_AUTO_CAPTURE>\n");
  if (VERBOSE_TEST)
    printf("parentheses capture text\n");
  RE re("(world|universe)$", options);
  CHECK(re.Extract("\\1", str , &captured));
  CHECK_EQ(captured, "world");
  options.set_no_auto_capture(true);
  printf("testing Option <NO_AUTO_CAPTURE>\n");
  if (VERBOSE_TEST)
    printf("parentheses do not capture text\n");
  re.Extract("\\1",str, &captured );
  CHECK_EQ(captured, "world");
}

static void Test_UNGREEDY() {
  RE_Options options;
  const char *str = "HELLO, 'this' is the 'world'";

  options.set_ungreedy(true);
  GetOneOptionResult("UNGREEDY 1", "('.*')", str, options, false, "'this'" );
  GetOneOptionResult("UNGREEDY 2", "('.*')", str, RE_Options().set_ungreedy(true), false, "'this'" );
  GetOneOptionResult("UNGREEDY", "('.*?')", str, options, false, "'this' is the 'world'" );

  options.set_ungreedy(false);
  GetOneOptionResult("no UNGREEDY", "('.*')", str, options, false, "'this' is the 'world'" );
  GetOneOptionResult("no UNGREEDY", "('.*?')", str, options, false, "'this'" );
}

static void Test_all_options() {
  const char *str = "HELLO\n" "cruel\n" "world";
  RE_Options options;
  options.set_all_options(PCRE_CASELESS | PCRE_DOTALL);

  TestOneOption("all_options (CASELESS|DOTALL)", "^hello.*WORLD", str , options, false);
  options.set_all_options(0);
  TestOneOption("all_options (0)", "^hello.*WORLD", str , options, false, false);
  options.set_all_options(PCRE_MULTILINE | PCRE_EXTENDED);

  TestOneOption("all_options (MULTILINE|EXTENDED)", " ^ c r u e l $ ", str, options, false);
  TestOneOption("all_options (MULTILINE|EXTENDED) with constructor",
                  " ^ c r u e l $ ",
                  str,
                  RE_Options(PCRE_MULTILINE | PCRE_EXTENDED),
                  false);

  TestOneOption("all_options (MULTILINE|EXTENDED) with concatenation",
                  " ^ c r u e l $ ",
                  str,
                  RE_Options()
                       .set_multiline(true)
                       .set_extended(true),
                  false);

  options.set_all_options(0);
  TestOneOption("all_options (0)", "^ c r u e l $", str, options, false, false);

}

static void TestOptions() {
  printf("Testing Options\n");
  Test_CASELESS();
  Test_MULTILINE();
  Test_DOTALL();
  Test_DOLLAR_ENDONLY();
  Test_EXTENDED();
  Test_NO_AUTO_CAPTURE();
  Test_UNGREEDY();
  Test_EXTRA();
  Test_all_options();
}

static void TestConstructors() {
  printf("Testing constructors\n");

  RE_Options options;
  options.set_dotall(true);
  const char *str = "HELLO\n" "cruel\n" "world";

  RE orig("HELLO.*world", options);
  CHECK(orig.FullMatch(str));

  RE copy1(orig);
  CHECK(copy1.FullMatch(str));

  RE copy2("not a match");
  CHECK(!copy2.FullMatch(str));
  copy2 = copy1;
  CHECK(copy2.FullMatch(str));
  copy2 = orig;
  CHECK(copy2.FullMatch(str));

  // Make sure when we assign to ourselves, nothing bad happens
  orig = orig;
  copy1 = copy1;
  copy2 = copy2;
  CHECK(orig.FullMatch(str));
  CHECK(copy1.FullMatch(str));
  CHECK(copy2.FullMatch(str));
}

int main(int argc, char** argv) {
  // Treat any flag as --help
  if (argc > 1 && argv[1][0] == '-') {
    printf("Usage: %s [timing1|timing2|timing3 num-iters]\n"
           "       If 'timingX ###' is specified, run the given timing test\n"
           "       with the given number of iterations, rather than running\n"
           "       the default corectness test.\n", argv[0]);
    return 0;
  }

  if (argc > 1) {
    if ( argc == 2 || atoi(argv[2]) == 0) {
      printf("timing mode needs a num-iters argument\n");
      return 1;
    }
    if (!strcmp(argv[1], "timing1"))
      Timing1(atoi(argv[2]));
    else if (!strcmp(argv[1], "timing2"))
      Timing2(atoi(argv[2]));
    else if (!strcmp(argv[1], "timing3"))
      Timing3(atoi(argv[2]));
    else
      printf("Unknown argument '%s'\n", argv[1]);
    return 0;
  }

  printf("PCRE C++ wrapper tests\n");
  printf("Testing FullMatch\n");

  int i;
  string s;

  /***** FullMatch with no args *****/

  CHECK(RE("h.*o").FullMatch("hello"));
  CHECK(!RE("h.*o").FullMatch("othello"));     // Must be anchored at front
  CHECK(!RE("h.*o").FullMatch("hello!"));      // Must be anchored at end
  CHECK(RE("a*").FullMatch("aaaa"));           // Fullmatch with normal op
  CHECK(RE("a*?").FullMatch("aaaa"));          // Fullmatch with nongreedy op
  CHECK(RE("a*?\\z").FullMatch("aaaa"));       // Two unusual ops

  /***** FullMatch with args *****/

  // Zero-arg
  CHECK(RE("\\d+").FullMatch("1001"));

  // Single-arg
  CHECK(RE("(\\d+)").FullMatch("1001",   &i));
  CHECK_EQ(i, 1001);
  CHECK(RE("(-?\\d+)").FullMatch("-123", &i));
  CHECK_EQ(i, -123);
  CHECK(!RE("()\\d+").FullMatch("10", &i));
  CHECK(!RE("(\\d+)").FullMatch("1234567890123456789012345678901234567890",
                                &i));

  // Digits surrounding integer-arg
  CHECK(RE("1(\\d*)4").FullMatch("1234", &i));
  CHECK_EQ(i, 23);
  CHECK(RE("(\\d)\\d+").FullMatch("1234", &i));
  CHECK_EQ(i, 1);
  CHECK(RE("(-\\d)\\d+").FullMatch("-1234", &i));
  CHECK_EQ(i, -1);
  CHECK(RE("(\\d)").PartialMatch("1234", &i));
  CHECK_EQ(i, 1);
  CHECK(RE("(-\\d)").PartialMatch("-1234", &i));
  CHECK_EQ(i, -1);

  // String-arg
  CHECK(RE("h(.*)o").FullMatch("hello", &s));
  CHECK_EQ(s, string("ell"));

  // StringPiece-arg
  StringPiece sp;
  CHECK(RE("(\\w+):(\\d+)").FullMatch("ruby:1234", &sp, &i));
  CHECK_EQ(sp.size(), 4);
  CHECK(memcmp(sp.data(), "ruby", 4) == 0);
  CHECK_EQ(i, 1234);

  // Multi-arg
  CHECK(RE("(\\w+):(\\d+)").FullMatch("ruby:1234", &s, &i));
  CHECK_EQ(s, string("ruby"));
  CHECK_EQ(i, 1234);

  // Ignore non-void* NULL arg
  CHECK(RE("he(.*)lo").FullMatch("hello", (char*)NULL));
  CHECK(RE("h(.*)o").FullMatch("hello", (string*)NULL));
  CHECK(RE("h(.*)o").FullMatch("hello", (StringPiece*)NULL));
  CHECK(RE("(.*)").FullMatch("1234", (int*)NULL));
#ifdef HAVE_LONG_LONG
  CHECK(RE("(.*)").FullMatch("1234567890123456", (long long*)NULL));
#endif
  CHECK(RE("(.*)").FullMatch("123.4567890123456", (double*)NULL));
  CHECK(RE("(.*)").FullMatch("123.4567890123456", (float*)NULL));

  // Fail on non-void* NULL arg if the match doesn't parse for the given type.
  CHECK(!RE("h(.*)lo").FullMatch("hello", &s, (char*)NULL));
  CHECK(!RE("(.*)").FullMatch("hello", (int*)NULL));
  CHECK(!RE("(.*)").FullMatch("1234567890123456", (int*)NULL));
  CHECK(!RE("(.*)").FullMatch("hello", (double*)NULL));
  CHECK(!RE("(.*)").FullMatch("hello", (float*)NULL));

  // Ignored arg
  CHECK(RE("(\\w+)(:)(\\d+)").FullMatch("ruby:1234", &s, (void*)NULL, &i));
  CHECK_EQ(s, string("ruby"));
  CHECK_EQ(i, 1234);

  // Type tests
  {
    char c;
    CHECK(RE("(H)ello").FullMatch("Hello", &c));
    CHECK_EQ(c, 'H');
  }
  {
    unsigned char c;
    CHECK(RE("(H)ello").FullMatch("Hello", &c));
    CHECK_EQ(c, static_cast<unsigned char>('H'));
  }
  {
    short v;
    CHECK(RE("(-?\\d+)").FullMatch("100",     &v));    CHECK_EQ(v, 100);
    CHECK(RE("(-?\\d+)").FullMatch("-100",    &v));    CHECK_EQ(v, -100);
    CHECK(RE("(-?\\d+)").FullMatch("32767",   &v));    CHECK_EQ(v, 32767);
    CHECK(RE("(-?\\d+)").FullMatch("-32768",  &v));    CHECK_EQ(v, -32768);
    CHECK(!RE("(-?\\d+)").FullMatch("-32769", &v));
    CHECK(!RE("(-?\\d+)").FullMatch("32768",  &v));
  }
  {
    unsigned short v;
    CHECK(RE("(\\d+)").FullMatch("100",     &v));    CHECK_EQ(v, 100);
    CHECK(RE("(\\d+)").FullMatch("32767",   &v));    CHECK_EQ(v, 32767);
    CHECK(RE("(\\d+)").FullMatch("65535",   &v));    CHECK_EQ(v, 65535);
    CHECK(!RE("(\\d+)").FullMatch("65536",  &v));
  }
  {
    int v;
    static const int max_value = 0x7fffffff;
    static const int min_value = -max_value - 1;
    CHECK(RE("(-?\\d+)").FullMatch("100",         &v)); CHECK_EQ(v, 100);
    CHECK(RE("(-?\\d+)").FullMatch("-100",        &v)); CHECK_EQ(v, -100);
    CHECK(RE("(-?\\d+)").FullMatch("2147483647",  &v)); CHECK_EQ(v, max_value);
    CHECK(RE("(-?\\d+)").FullMatch("-2147483648", &v)); CHECK_EQ(v, min_value);
    CHECK(!RE("(-?\\d+)").FullMatch("-2147483649", &v));
    CHECK(!RE("(-?\\d+)").FullMatch("2147483648",  &v));
  }
  {
    unsigned int v;
    static const unsigned int max_value = 0xfffffffful;
    CHECK(RE("(\\d+)").FullMatch("100",         &v)); CHECK_EQ(v, 100);
    CHECK(RE("(\\d+)").FullMatch("4294967295",  &v)); CHECK_EQ(v, max_value);
    CHECK(!RE("(\\d+)").FullMatch("4294967296", &v));
  }
#ifdef HAVE_LONG_LONG
# if defined(__MINGW__) || defined(__MINGW32__)
#   define LLD "%I64d"
#   define LLU "%I64u"
# else
#   define LLD "%lld"
#   define LLU "%llu"
# endif
  {
    long long v;
    static const long long max_value = 0x7fffffffffffffffLL;
    static const long long min_value = -max_value - 1;
    char buf[32];  // definitely big enough for a long long

    CHECK(RE("(-?\\d+)").FullMatch("100", &v)); CHECK_EQ(v, 100);
    CHECK(RE("(-?\\d+)").FullMatch("-100",&v)); CHECK_EQ(v, -100);

    sprintf(buf, LLD, max_value);
    CHECK(RE("(-?\\d+)").FullMatch(buf,&v)); CHECK_EQ(v, max_value);

    sprintf(buf, LLD, min_value);
    CHECK(RE("(-?\\d+)").FullMatch(buf,&v)); CHECK_EQ(v, min_value);

    sprintf(buf, LLD, max_value);
    assert(buf[strlen(buf)-1] != '9');
    buf[strlen(buf)-1]++;
    CHECK(!RE("(-?\\d+)").FullMatch(buf, &v));

    sprintf(buf, LLD, min_value);
    assert(buf[strlen(buf)-1] != '9');
    buf[strlen(buf)-1]++;
    CHECK(!RE("(-?\\d+)").FullMatch(buf, &v));
  }
#endif
#if defined HAVE_UNSIGNED_LONG_LONG && defined HAVE_LONG_LONG
  {
    unsigned long long v;
    long long v2;
    static const unsigned long long max_value = 0xffffffffffffffffULL;
    char buf[32];  // definitely big enough for a unsigned long long

    CHECK(RE("(-?\\d+)").FullMatch("100",&v)); CHECK_EQ(v, 100);
    CHECK(RE("(-?\\d+)").FullMatch("-100",&v2)); CHECK_EQ(v2, -100);

    sprintf(buf, LLU, max_value);
    CHECK(RE("(-?\\d+)").FullMatch(buf,&v)); CHECK_EQ(v, max_value);

    assert(buf[strlen(buf)-1] != '9');
    buf[strlen(buf)-1]++;
    CHECK(!RE("(-?\\d+)").FullMatch(buf, &v));
  }
#endif
  {
    float v;
    CHECK(RE("(.*)").FullMatch("100", &v));
    CHECK(RE("(.*)").FullMatch("-100.", &v));
    CHECK(RE("(.*)").FullMatch("1e23", &v));
  }
  {
    double v;
    CHECK(RE("(.*)").FullMatch("100", &v));
    CHECK(RE("(.*)").FullMatch("-100.", &v));
    CHECK(RE("(.*)").FullMatch("1e23", &v));
  }

  // Check that matching is fully anchored
  CHECK(!RE("(\\d+)").FullMatch("x1001",  &i));
  CHECK(!RE("(\\d+)").FullMatch("1001x",  &i));
  CHECK(RE("x(\\d+)").FullMatch("x1001", &i)); CHECK_EQ(i, 1001);
  CHECK(RE("(\\d+)x").FullMatch("1001x", &i)); CHECK_EQ(i, 1001);

  // Braces
  CHECK(RE("[0-9a-f+.-]{5,}").FullMatch("0abcd"));
  CHECK(RE("[0-9a-f+.-]{5,}").FullMatch("0abcde"));
  CHECK(!RE("[0-9a-f+.-]{5,}").FullMatch("0abc"));

  // Complicated RE
  CHECK(RE("foo|bar|[A-Z]").FullMatch("foo"));
  CHECK(RE("foo|bar|[A-Z]").FullMatch("bar"));
  CHECK(RE("foo|bar|[A-Z]").FullMatch("X"));
  CHECK(!RE("foo|bar|[A-Z]").FullMatch("XY"));

  // Check full-match handling (needs '$' tacked on internally)
  CHECK(RE("fo|foo").FullMatch("fo"));
  CHECK(RE("fo|foo").FullMatch("foo"));
  CHECK(RE("fo|foo$").FullMatch("fo"));
  CHECK(RE("fo|foo$").FullMatch("foo"));
  CHECK(RE("foo$").FullMatch("foo"));
  CHECK(!RE("foo\\$").FullMatch("foo$bar"));
  CHECK(!RE("fo|bar").FullMatch("fox"));

  // Uncomment the following if we change the handling of '$' to
  // prevent it from matching a trailing newline
  if (false) {
    // Check that we don't get bitten by pcre's special handling of a
    // '\n' at the end of the string matching '$'
    CHECK(!RE("foo$").PartialMatch("foo\n"));
  }

  // Number of args
  int a[16];
  CHECK(RE("").FullMatch(""));

  memset(a, 0, sizeof(0));
  CHECK(RE("(\\d){1}").FullMatch("1",
                                 &a[0]));
  CHECK_EQ(a[0], 1);

  memset(a, 0, sizeof(0));
  CHECK(RE("(\\d)(\\d)").FullMatch("12",
                                   &a[0],  &a[1]));
  CHECK_EQ(a[0], 1);
  CHECK_EQ(a[1], 2);

  memset(a, 0, sizeof(0));
  CHECK(RE("(\\d)(\\d)(\\d)").FullMatch("123",
                                        &a[0],  &a[1],  &a[2]));
  CHECK_EQ(a[0], 1);
  CHECK_EQ(a[1], 2);
  CHECK_EQ(a[2], 3);

  memset(a, 0, sizeof(0));
  CHECK(RE("(\\d)(\\d)(\\d)(\\d)").FullMatch("1234",
                                             &a[0],  &a[1],  &a[2],  &a[3]));
  CHECK_EQ(a[0], 1);
  CHECK_EQ(a[1], 2);
  CHECK_EQ(a[2], 3);
  CHECK_EQ(a[3], 4);

  memset(a, 0, sizeof(0));
  CHECK(RE("(\\d)(\\d)(\\d)(\\d)(\\d)").FullMatch("12345",
                                                  &a[0],  &a[1],  &a[2],
                                                  &a[3],  &a[4]));
  CHECK_EQ(a[0], 1);
  CHECK_EQ(a[1], 2);
  CHECK_EQ(a[2], 3);
  CHECK_EQ(a[3], 4);
  CHECK_EQ(a[4], 5);

  memset(a, 0, sizeof(0));
  CHECK(RE("(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)").FullMatch("123456",
                                                       &a[0],  &a[1],  &a[2],
                                                       &a[3],  &a[4],  &a[5]));
  CHECK_EQ(a[0], 1);
  CHECK_EQ(a[1], 2);
  CHECK_EQ(a[2], 3);
  CHECK_EQ(a[3], 4);
  CHECK_EQ(a[4], 5);
  CHECK_EQ(a[5], 6);

  memset(a, 0, sizeof(0));
  CHECK(RE("(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)").FullMatch("1234567",
                                                            &a[0],  &a[1],  &a[2],  &a[3],
                                                            &a[4],  &a[5],  &a[6]));
  CHECK_EQ(a[0], 1);
  CHECK_EQ(a[1], 2);
  CHECK_EQ(a[2], 3);
  CHECK_EQ(a[3], 4);
  CHECK_EQ(a[4], 5);
  CHECK_EQ(a[5], 6);
  CHECK_EQ(a[6], 7);

  memset(a, 0, sizeof(0));
  CHECK(RE("(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)"
           "(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)").FullMatch(
               "1234567890123456",
               &a[0],  &a[1],  &a[2],  &a[3],
               &a[4],  &a[5],  &a[6],  &a[7],
               &a[8],  &a[9],  &a[10], &a[11],
               &a[12], &a[13], &a[14], &a[15]));
  CHECK_EQ(a[0], 1);
  CHECK_EQ(a[1], 2);
  CHECK_EQ(a[2], 3);
  CHECK_EQ(a[3], 4);
  CHECK_EQ(a[4], 5);
  CHECK_EQ(a[5], 6);
  CHECK_EQ(a[6], 7);
  CHECK_EQ(a[7], 8);
  CHECK_EQ(a[8], 9);
  CHECK_EQ(a[9], 0);
  CHECK_EQ(a[10], 1);
  CHECK_EQ(a[11], 2);
  CHECK_EQ(a[12], 3);
  CHECK_EQ(a[13], 4);
  CHECK_EQ(a[14], 5);
  CHECK_EQ(a[15], 6);

  /***** PartialMatch *****/

  printf("Testing PartialMatch\n");

  CHECK(RE("h.*o").PartialMatch("hello"));
  CHECK(RE("h.*o").PartialMatch("othello"));
  CHECK(RE("h.*o").PartialMatch("hello!"));
  CHECK(RE("((((((((((((((((((((x))))))))))))))))))))").PartialMatch("x"));

  /***** other tests *****/

  RadixTests();
  TestReplace();
  TestExtract();
  TestConsume();
  TestFindAndConsume();
  TestQuoteMetaAll();
  TestMatchNumberPeculiarity();

  // Check the pattern() accessor
  {
    const string kPattern = "http://([^/]+)/.*";
    const RE re(kPattern);
    CHECK_EQ(kPattern, re.pattern());
  }

  // Check RE error field.
  {
    RE re("foo");
    CHECK(re.error().empty());  // Must have no error
  }

#ifdef SUPPORT_UTF8
  // Check UTF-8 handling
  {
    printf("Testing UTF-8 handling\n");

    // Three Japanese characters (nihongo)
    const unsigned char utf8_string[] = {
         0xe6, 0x97, 0xa5, // 65e5
         0xe6, 0x9c, 0xac, // 627c
         0xe8, 0xaa, 0x9e, // 8a9e
         0
    };
    const unsigned char utf8_pattern[] = {
         '.',
         0xe6, 0x9c, 0xac, // 627c
         '.',
         0
    };

    // Both should match in either mode, bytes or UTF-8
    RE re_test1(".........");
    CHECK(re_test1.FullMatch(utf8_string));
    RE re_test2("...", pcrecpp::UTF8());
    CHECK(re_test2.FullMatch(utf8_string));

    // Check that '.' matches one byte or UTF-8 character
    // according to the mode.
    string ss;
    RE re_test3("(.)");
    CHECK(re_test3.PartialMatch(utf8_string, &ss));
    CHECK_EQ(ss, string("\xe6"));
    RE re_test4("(.)", pcrecpp::UTF8());
    CHECK(re_test4.PartialMatch(utf8_string, &ss));
    CHECK_EQ(ss, string("\xe6\x97\xa5"));

    // Check that string matches itself in either mode
    RE re_test5(utf8_string);
    CHECK(re_test5.FullMatch(utf8_string));
    RE re_test6(utf8_string, pcrecpp::UTF8());
    CHECK(re_test6.FullMatch(utf8_string));

    // Check that pattern matches string only in UTF8 mode
    RE re_test7(utf8_pattern);
    CHECK(!re_test7.FullMatch(utf8_string));
    RE re_test8(utf8_pattern, pcrecpp::UTF8());
    CHECK(re_test8.FullMatch(utf8_string));
  }

  // Check that ungreedy, UTF8 regular expressions don't match when they
  // oughtn't -- see bug 82246.
  {
    // This code always worked.
    const char* pattern = "\\w+X";
    const string target = "a aX";
    RE match_sentence(pattern);
    RE match_sentence_re(pattern, pcrecpp::UTF8());

    CHECK(!match_sentence.FullMatch(target));
    CHECK(!match_sentence_re.FullMatch(target));
  }

  {
    const char* pattern = "(?U)\\w+X";
    const string target = "a aX";
    RE match_sentence(pattern);
    RE match_sentence_re(pattern, pcrecpp::UTF8());

    CHECK(!match_sentence.FullMatch(target));
    CHECK(!match_sentence_re.FullMatch(target));
  }
#endif  /* def SUPPORT_UTF8 */

  printf("Testing error reporting\n");

  { RE re("a\\1"); CHECK(!re.error().empty()); }
  {
    RE re("a[x");
    CHECK(!re.error().empty());
  }
  {
    RE re("a[z-a]");
    CHECK(!re.error().empty());
  }
  {
    RE re("a[[:foobar:]]");
    CHECK(!re.error().empty());
  }
  {
    RE re("a(b");
    CHECK(!re.error().empty());
  }
  {
    RE re("a\\");
    CHECK(!re.error().empty());
  }

  // Test that recursion is stopped
  TestRecursion();

  // Test Options
  if (getenv("VERBOSE_TEST") != NULL)
    VERBOSE_TEST  = true;
  TestOptions();

  // Test the constructors
  TestConstructors();

  // Done
  printf("OK\n");

  return 0;
}

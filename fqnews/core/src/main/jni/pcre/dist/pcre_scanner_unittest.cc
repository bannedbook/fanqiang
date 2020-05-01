// Copyright (c) 2005, Google Inc.
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
// Author: Greg J. Badros
//
// Unittest for scanner, especially GetNextComments and GetComments()
// functionality.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>      /* for strchr */
#include <string>
#include <vector>

#include "pcrecpp.h"
#include "pcre_stringpiece.h"
#include "pcre_scanner.h"

#define FLAGS_unittest_stack_size   49152

// Dies with a fatal error if the two values are not equal.
#define CHECK_EQ(a, b)  do {                                    \
  if ( (a) != (b) ) {                                           \
    fprintf(stderr, "%s:%d: Check failed because %s != %s\n",   \
            __FILE__, __LINE__, #a, #b);                        \
    exit(1);                                                    \
  }                                                             \
} while (0)

using std::vector;
using pcrecpp::StringPiece;
using pcrecpp::Scanner;

static void TestScanner() {
  const char input[] = "\n"
                       "alpha = 1; // this sets alpha\n"
                       "bravo = 2; // bravo is set here\n"
                       "gamma = 33; /* and here is gamma */\n";

  const char *re = "(\\w+) = (\\d+);";

  Scanner s(input);
  string var;
  int number;
  s.SkipCXXComments();
  s.set_save_comments(true);
  vector<StringPiece> comments;

  s.Consume(re, &var, &number);
  CHECK_EQ(var, "alpha");
  CHECK_EQ(number, 1);
  CHECK_EQ(s.LineNumber(), 3);
  s.GetNextComments(&comments);
  CHECK_EQ(comments.size(), 1);
  CHECK_EQ(comments[0].as_string(), " // this sets alpha\n");
  comments.resize(0);

  s.Consume(re, &var, &number);
  CHECK_EQ(var, "bravo");
  CHECK_EQ(number, 2);
  s.GetNextComments(&comments);
  CHECK_EQ(comments.size(), 1);
  CHECK_EQ(comments[0].as_string(), " // bravo is set here\n");
  comments.resize(0);

  s.Consume(re, &var, &number);
  CHECK_EQ(var, "gamma");
  CHECK_EQ(number, 33);
  s.GetNextComments(&comments);
  CHECK_EQ(comments.size(), 1);
  CHECK_EQ(comments[0].as_string(), " /* and here is gamma */\n");
  comments.resize(0);

  s.GetComments(0, sizeof(input), &comments);
  CHECK_EQ(comments.size(), 3);
  CHECK_EQ(comments[0].as_string(), " // this sets alpha\n");
  CHECK_EQ(comments[1].as_string(), " // bravo is set here\n");
  CHECK_EQ(comments[2].as_string(), " /* and here is gamma */\n");
  comments.resize(0);

  s.GetComments(0, (int)(strchr(input, '/') - input), &comments);
  CHECK_EQ(comments.size(), 0);
  comments.resize(0);

  s.GetComments((int)(strchr(input, '/') - input - 1), sizeof(input),
                &comments);
  CHECK_EQ(comments.size(), 3);
  CHECK_EQ(comments[0].as_string(), " // this sets alpha\n");
  CHECK_EQ(comments[1].as_string(), " // bravo is set here\n");
  CHECK_EQ(comments[2].as_string(), " /* and here is gamma */\n");
  comments.resize(0);

  s.GetComments((int)(strchr(input, '/') - input - 1),
                (int)(strchr(input + 1, '\n') - input + 1), &comments);
  CHECK_EQ(comments.size(), 1);
  CHECK_EQ(comments[0].as_string(), " // this sets alpha\n");
  comments.resize(0);
}

static void TestBigComment() {
  string input;
  for (int i = 0; i < 1024; ++i) {
    char buf[1024];  // definitely big enough
    sprintf(buf, "    # Comment %d\n", i);
    input += buf;
  }
  input += "name = value;\n";

  Scanner s(input.c_str());
  s.SetSkipExpression("\\s+|#.*\n");

  string name;
  string value;
  s.Consume("(\\w+) = (\\w+);", &name, &value);
  CHECK_EQ(name, "name");
  CHECK_EQ(value, "value");
}

// TODO: also test scanner and big-comment in a thread with a
//       small stack size

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  TestScanner();
  TestBigComment();

  // Done
  printf("OK\n");

  return 0;
}

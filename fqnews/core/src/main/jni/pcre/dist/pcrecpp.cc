// Copyright (c) 2010, Google Inc.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>      /* for SHRT_MIN, USHRT_MAX, etc */
#include <string.h>      /* for memcpy */
#include <assert.h>
#include <errno.h>
#include <string>
#include <algorithm>

#include "pcrecpp_internal.h"
#include "pcre.h"
#include "pcrecpp.h"
#include "pcre_stringpiece.h"


namespace pcrecpp {

// Maximum number of args we can set
static const int kMaxArgs = 16;
static const int kVecSize = (1 + kMaxArgs) * 3;  // results + PCRE workspace

// Special object that stands-in for no argument
Arg RE::no_arg((void*)NULL);

// This is for ABI compatibility with old versions of pcre (pre-7.6),
// which defined a global no_arg variable instead of putting it in the
// RE class.  This works on GCC >= 3, at least.  It definitely works
// for ELF, but may not for other object formats (Mach-O, for
// instance, does not support aliases.)  We could probably have a more
// inclusive test if we ever needed it.  (Note that not only the
// __attribute__ syntax, but also __USER_LABEL_PREFIX__, are
// gnu-specific.)
#if defined(__GNUC__) && __GNUC__ >= 3 && defined(__ELF__)
# define ULP_AS_STRING(x)            ULP_AS_STRING_INTERNAL(x)
# define ULP_AS_STRING_INTERNAL(x)   #x
# define USER_LABEL_PREFIX_STR       ULP_AS_STRING(__USER_LABEL_PREFIX__)
extern Arg no_arg
  __attribute__((alias(USER_LABEL_PREFIX_STR "_ZN7pcrecpp2RE6no_argE")));
#endif

// If a regular expression has no error, its error_ field points here
static const string empty_string;

// If the user doesn't ask for any options, we just use this one
static RE_Options default_options;

void RE::Init(const string& pat, const RE_Options* options) {
  pattern_ = pat;
  if (options == NULL) {
    options_ = default_options;
  } else {
    options_ = *options;
  }
  error_ = &empty_string;
  re_full_ = NULL;
  re_partial_ = NULL;

  re_partial_ = Compile(UNANCHORED);
  if (re_partial_ != NULL) {
    re_full_ = Compile(ANCHOR_BOTH);
  }
}

void RE::Cleanup() {
  if (re_full_ != NULL)         (*pcre_free)(re_full_);
  if (re_partial_ != NULL)      (*pcre_free)(re_partial_);
  if (error_ != &empty_string)  delete error_;
}


RE::~RE() {
  Cleanup();
}


pcre* RE::Compile(Anchor anchor) {
  // First, convert RE_Options into pcre options
  int pcre_options = 0;
  pcre_options = options_.all_options();

  // Special treatment for anchoring.  This is needed because at
  // runtime pcre only provides an option for anchoring at the
  // beginning of a string (unless you use offset).
  //
  // There are three types of anchoring we want:
  //    UNANCHORED      Compile the original pattern, and use
  //                    a pcre unanchored match.
  //    ANCHOR_START    Compile the original pattern, and use
  //                    a pcre anchored match.
  //    ANCHOR_BOTH     Tack a "\z" to the end of the original pattern
  //                    and use a pcre anchored match.

  const char* compile_error;
  int eoffset;
  pcre* re;
  if (anchor != ANCHOR_BOTH) {
    re = pcre_compile(pattern_.c_str(), pcre_options,
                      &compile_error, &eoffset, NULL);
  } else {
    // Tack a '\z' at the end of RE.  Parenthesize it first so that
    // the '\z' applies to all top-level alternatives in the regexp.
    string wrapped = "(?:";  // A non-counting grouping operator
    wrapped += pattern_;
    wrapped += ")\\z";
    re = pcre_compile(wrapped.c_str(), pcre_options,
                      &compile_error, &eoffset, NULL);
  }
  if (re == NULL) {
    if (error_ == &empty_string) error_ = new string(compile_error);
  }
  return re;
}

/***** Matching interfaces *****/

bool RE::FullMatch(const StringPiece& text,
                   const Arg& ptr1,
                   const Arg& ptr2,
                   const Arg& ptr3,
                   const Arg& ptr4,
                   const Arg& ptr5,
                   const Arg& ptr6,
                   const Arg& ptr7,
                   const Arg& ptr8,
                   const Arg& ptr9,
                   const Arg& ptr10,
                   const Arg& ptr11,
                   const Arg& ptr12,
                   const Arg& ptr13,
                   const Arg& ptr14,
                   const Arg& ptr15,
                   const Arg& ptr16) const {
  const Arg* args[kMaxArgs];
  int n = 0;
  if (&ptr1  == &no_arg) goto done; args[n++] = &ptr1;
  if (&ptr2  == &no_arg) goto done; args[n++] = &ptr2;
  if (&ptr3  == &no_arg) goto done; args[n++] = &ptr3;
  if (&ptr4  == &no_arg) goto done; args[n++] = &ptr4;
  if (&ptr5  == &no_arg) goto done; args[n++] = &ptr5;
  if (&ptr6  == &no_arg) goto done; args[n++] = &ptr6;
  if (&ptr7  == &no_arg) goto done; args[n++] = &ptr7;
  if (&ptr8  == &no_arg) goto done; args[n++] = &ptr8;
  if (&ptr9  == &no_arg) goto done; args[n++] = &ptr9;
  if (&ptr10 == &no_arg) goto done; args[n++] = &ptr10;
  if (&ptr11 == &no_arg) goto done; args[n++] = &ptr11;
  if (&ptr12 == &no_arg) goto done; args[n++] = &ptr12;
  if (&ptr13 == &no_arg) goto done; args[n++] = &ptr13;
  if (&ptr14 == &no_arg) goto done; args[n++] = &ptr14;
  if (&ptr15 == &no_arg) goto done; args[n++] = &ptr15;
  if (&ptr16 == &no_arg) goto done; args[n++] = &ptr16;
 done:

  int consumed;
  int vec[kVecSize];
  return DoMatchImpl(text, ANCHOR_BOTH, &consumed, args, n, vec, kVecSize);
}

bool RE::PartialMatch(const StringPiece& text,
                      const Arg& ptr1,
                      const Arg& ptr2,
                      const Arg& ptr3,
                      const Arg& ptr4,
                      const Arg& ptr5,
                      const Arg& ptr6,
                      const Arg& ptr7,
                      const Arg& ptr8,
                      const Arg& ptr9,
                      const Arg& ptr10,
                      const Arg& ptr11,
                      const Arg& ptr12,
                      const Arg& ptr13,
                      const Arg& ptr14,
                      const Arg& ptr15,
                      const Arg& ptr16) const {
  const Arg* args[kMaxArgs];
  int n = 0;
  if (&ptr1  == &no_arg) goto done; args[n++] = &ptr1;
  if (&ptr2  == &no_arg) goto done; args[n++] = &ptr2;
  if (&ptr3  == &no_arg) goto done; args[n++] = &ptr3;
  if (&ptr4  == &no_arg) goto done; args[n++] = &ptr4;
  if (&ptr5  == &no_arg) goto done; args[n++] = &ptr5;
  if (&ptr6  == &no_arg) goto done; args[n++] = &ptr6;
  if (&ptr7  == &no_arg) goto done; args[n++] = &ptr7;
  if (&ptr8  == &no_arg) goto done; args[n++] = &ptr8;
  if (&ptr9  == &no_arg) goto done; args[n++] = &ptr9;
  if (&ptr10 == &no_arg) goto done; args[n++] = &ptr10;
  if (&ptr11 == &no_arg) goto done; args[n++] = &ptr11;
  if (&ptr12 == &no_arg) goto done; args[n++] = &ptr12;
  if (&ptr13 == &no_arg) goto done; args[n++] = &ptr13;
  if (&ptr14 == &no_arg) goto done; args[n++] = &ptr14;
  if (&ptr15 == &no_arg) goto done; args[n++] = &ptr15;
  if (&ptr16 == &no_arg) goto done; args[n++] = &ptr16;
 done:

  int consumed;
  int vec[kVecSize];
  return DoMatchImpl(text, UNANCHORED, &consumed, args, n, vec, kVecSize);
}

bool RE::Consume(StringPiece* input,
                 const Arg& ptr1,
                 const Arg& ptr2,
                 const Arg& ptr3,
                 const Arg& ptr4,
                 const Arg& ptr5,
                 const Arg& ptr6,
                 const Arg& ptr7,
                 const Arg& ptr8,
                 const Arg& ptr9,
                 const Arg& ptr10,
                 const Arg& ptr11,
                 const Arg& ptr12,
                 const Arg& ptr13,
                 const Arg& ptr14,
                 const Arg& ptr15,
                 const Arg& ptr16) const {
  const Arg* args[kMaxArgs];
  int n = 0;
  if (&ptr1  == &no_arg) goto done; args[n++] = &ptr1;
  if (&ptr2  == &no_arg) goto done; args[n++] = &ptr2;
  if (&ptr3  == &no_arg) goto done; args[n++] = &ptr3;
  if (&ptr4  == &no_arg) goto done; args[n++] = &ptr4;
  if (&ptr5  == &no_arg) goto done; args[n++] = &ptr5;
  if (&ptr6  == &no_arg) goto done; args[n++] = &ptr6;
  if (&ptr7  == &no_arg) goto done; args[n++] = &ptr7;
  if (&ptr8  == &no_arg) goto done; args[n++] = &ptr8;
  if (&ptr9  == &no_arg) goto done; args[n++] = &ptr9;
  if (&ptr10 == &no_arg) goto done; args[n++] = &ptr10;
  if (&ptr11 == &no_arg) goto done; args[n++] = &ptr11;
  if (&ptr12 == &no_arg) goto done; args[n++] = &ptr12;
  if (&ptr13 == &no_arg) goto done; args[n++] = &ptr13;
  if (&ptr14 == &no_arg) goto done; args[n++] = &ptr14;
  if (&ptr15 == &no_arg) goto done; args[n++] = &ptr15;
  if (&ptr16 == &no_arg) goto done; args[n++] = &ptr16;
 done:

  int consumed;
  int vec[kVecSize];
  if (DoMatchImpl(*input, ANCHOR_START, &consumed,
                  args, n, vec, kVecSize)) {
    input->remove_prefix(consumed);
    return true;
  } else {
    return false;
  }
}

bool RE::FindAndConsume(StringPiece* input,
                        const Arg& ptr1,
                        const Arg& ptr2,
                        const Arg& ptr3,
                        const Arg& ptr4,
                        const Arg& ptr5,
                        const Arg& ptr6,
                        const Arg& ptr7,
                        const Arg& ptr8,
                        const Arg& ptr9,
                        const Arg& ptr10,
                        const Arg& ptr11,
                        const Arg& ptr12,
                        const Arg& ptr13,
                        const Arg& ptr14,
                        const Arg& ptr15,
                        const Arg& ptr16) const {
  const Arg* args[kMaxArgs];
  int n = 0;
  if (&ptr1  == &no_arg) goto done; args[n++] = &ptr1;
  if (&ptr2  == &no_arg) goto done; args[n++] = &ptr2;
  if (&ptr3  == &no_arg) goto done; args[n++] = &ptr3;
  if (&ptr4  == &no_arg) goto done; args[n++] = &ptr4;
  if (&ptr5  == &no_arg) goto done; args[n++] = &ptr5;
  if (&ptr6  == &no_arg) goto done; args[n++] = &ptr6;
  if (&ptr7  == &no_arg) goto done; args[n++] = &ptr7;
  if (&ptr8  == &no_arg) goto done; args[n++] = &ptr8;
  if (&ptr9  == &no_arg) goto done; args[n++] = &ptr9;
  if (&ptr10 == &no_arg) goto done; args[n++] = &ptr10;
  if (&ptr11 == &no_arg) goto done; args[n++] = &ptr11;
  if (&ptr12 == &no_arg) goto done; args[n++] = &ptr12;
  if (&ptr13 == &no_arg) goto done; args[n++] = &ptr13;
  if (&ptr14 == &no_arg) goto done; args[n++] = &ptr14;
  if (&ptr15 == &no_arg) goto done; args[n++] = &ptr15;
  if (&ptr16 == &no_arg) goto done; args[n++] = &ptr16;
 done:

  int consumed;
  int vec[kVecSize];
  if (DoMatchImpl(*input, UNANCHORED, &consumed,
                  args, n, vec, kVecSize)) {
    input->remove_prefix(consumed);
    return true;
  } else {
    return false;
  }
}

bool RE::Replace(const StringPiece& rewrite,
                 string *str) const {
  int vec[kVecSize];
  int matches = TryMatch(*str, 0, UNANCHORED, true, vec, kVecSize);
  if (matches == 0)
    return false;

  string s;
  if (!Rewrite(&s, rewrite, *str, vec, matches))
    return false;

  assert(vec[0] >= 0);
  assert(vec[1] >= 0);
  str->replace(vec[0], vec[1] - vec[0], s);
  return true;
}

// Returns PCRE_NEWLINE_CRLF, PCRE_NEWLINE_CR, or PCRE_NEWLINE_LF.
// Note that PCRE_NEWLINE_CRLF is defined to be P_N_CR | P_N_LF.
// Modified by PH to add PCRE_NEWLINE_ANY and PCRE_NEWLINE_ANYCRLF.

static int NewlineMode(int pcre_options) {
  // TODO: if we can make it threadsafe, cache this var
  int newline_mode = 0;
  /* if (newline_mode) return newline_mode; */  // do this once it's cached
  if (pcre_options & (PCRE_NEWLINE_CRLF|PCRE_NEWLINE_CR|PCRE_NEWLINE_LF|
                      PCRE_NEWLINE_ANY|PCRE_NEWLINE_ANYCRLF)) {
    newline_mode = (pcre_options &
                    (PCRE_NEWLINE_CRLF|PCRE_NEWLINE_CR|PCRE_NEWLINE_LF|
                     PCRE_NEWLINE_ANY|PCRE_NEWLINE_ANYCRLF));
  } else {
    int newline;
    pcre_config(PCRE_CONFIG_NEWLINE, &newline);
    if (newline == 10)
      newline_mode = PCRE_NEWLINE_LF;
    else if (newline == 13)
      newline_mode = PCRE_NEWLINE_CR;
    else if (newline == 3338)
      newline_mode = PCRE_NEWLINE_CRLF;
    else if (newline == -1)
      newline_mode = PCRE_NEWLINE_ANY;
    else if (newline == -2)
      newline_mode = PCRE_NEWLINE_ANYCRLF;
    else
      assert(NULL == "Unexpected return value from pcre_config(NEWLINE)");
  }
  return newline_mode;
}

int RE::GlobalReplace(const StringPiece& rewrite,
                      string *str) const {
  int count = 0;
  int vec[kVecSize];
  string out;
  int start = 0;
  bool last_match_was_empty_string = false;

  while (start <= static_cast<int>(str->length())) {
    // If the previous match was for the empty string, we shouldn't
    // just match again: we'll match in the same way and get an
    // infinite loop.  Instead, we do the match in a special way:
    // anchored -- to force another try at the same position --
    // and with a flag saying that this time, ignore empty matches.
    // If this special match returns, that means there's a non-empty
    // match at this position as well, and we can continue.  If not,
    // we do what perl does, and just advance by one.
    // Notice that perl prints '@@@' for this;
    //    perl -le '$_ = "aa"; s/b*|aa/@/g; print'
    int matches;
    if (last_match_was_empty_string) {
      matches = TryMatch(*str, start, ANCHOR_START, false, vec, kVecSize);
      if (matches <= 0) {
        int matchend = start + 1;     // advance one character.
        // If the current char is CR and we're in CRLF mode, skip LF too.
        // Note it's better to call pcre_fullinfo() than to examine
        // all_options(), since options_ could have changed bewteen
        // compile-time and now, but this is simpler and safe enough.
        // Modified by PH to add ANY and ANYCRLF.
        if (matchend < static_cast<int>(str->length()) &&
            (*str)[start] == '\r' && (*str)[matchend] == '\n' &&
            (NewlineMode(options_.all_options()) == PCRE_NEWLINE_CRLF ||
             NewlineMode(options_.all_options()) == PCRE_NEWLINE_ANY ||
             NewlineMode(options_.all_options()) == PCRE_NEWLINE_ANYCRLF)) {
          matchend++;
        }
        // We also need to advance more than one char if we're in utf8 mode.
#ifdef SUPPORT_UTF8
        if (options_.utf8()) {
          while (matchend < static_cast<int>(str->length()) &&
                 ((*str)[matchend] & 0xc0) == 0x80)
            matchend++;
        }
#endif
        if (start < static_cast<int>(str->length()))
          out.append(*str, start, matchend - start);
        start = matchend;
        last_match_was_empty_string = false;
        continue;
      }
    } else {
      matches = TryMatch(*str, start, UNANCHORED, true, vec, kVecSize);
      if (matches <= 0)
        break;
    }
    int matchstart = vec[0], matchend = vec[1];
    assert(matchstart >= start);
    assert(matchend >= matchstart);
    out.append(*str, start, matchstart - start);
    Rewrite(&out, rewrite, *str, vec, matches);
    start = matchend;
    count++;
    last_match_was_empty_string = (matchstart == matchend);
  }

  if (count == 0)
    return 0;

  if (start < static_cast<int>(str->length()))
    out.append(*str, start, str->length() - start);
  swap(out, *str);
  return count;
}

bool RE::Extract(const StringPiece& rewrite,
                 const StringPiece& text,
                 string *out) const {
  int vec[kVecSize];
  int matches = TryMatch(text, 0, UNANCHORED, true, vec, kVecSize);
  if (matches == 0)
    return false;
  out->erase();
  return Rewrite(out, rewrite, text, vec, matches);
}

/*static*/ string RE::QuoteMeta(const StringPiece& unquoted) {
  string result;

  // Escape any ascii character not in [A-Za-z_0-9].
  //
  // Note that it's legal to escape a character even if it has no
  // special meaning in a regular expression -- so this function does
  // that.  (This also makes it identical to the perl function of the
  // same name; see `perldoc -f quotemeta`.)  The one exception is
  // escaping NUL: rather than doing backslash + NUL, like perl does,
  // we do '\0', because pcre itself doesn't take embedded NUL chars.
  for (int ii = 0; ii < unquoted.size(); ++ii) {
    // Note that using 'isalnum' here raises the benchmark time from
    // 32ns to 58ns:
    if (unquoted[ii] == '\0') {
      result += "\\0";
    } else if ((unquoted[ii] < 'a' || unquoted[ii] > 'z') &&
               (unquoted[ii] < 'A' || unquoted[ii] > 'Z') &&
               (unquoted[ii] < '0' || unquoted[ii] > '9') &&
               unquoted[ii] != '_' &&
               // If this is the part of a UTF8 or Latin1 character, we need
               // to copy this byte without escaping.  Experimentally this is
               // what works correctly with the regexp library.
               !(unquoted[ii] & 128)) {
      result += '\\';
      result += unquoted[ii];
    } else {
      result += unquoted[ii];
    }
  }

  return result;
}

/***** Actual matching and rewriting code *****/

int RE::TryMatch(const StringPiece& text,
                 int startpos,
                 Anchor anchor,
                 bool empty_ok,
                 int *vec,
                 int vecsize) const {
  pcre* re = (anchor == ANCHOR_BOTH) ? re_full_ : re_partial_;
  if (re == NULL) {
    //fprintf(stderr, "Matching against invalid re: %s\n", error_->c_str());
    return 0;
  }

  pcre_extra extra = { 0, 0, 0, 0, 0, 0, 0, 0 };
  if (options_.match_limit() > 0) {
    extra.flags |= PCRE_EXTRA_MATCH_LIMIT;
    extra.match_limit = options_.match_limit();
  }
  if (options_.match_limit_recursion() > 0) {
    extra.flags |= PCRE_EXTRA_MATCH_LIMIT_RECURSION;
    extra.match_limit_recursion = options_.match_limit_recursion();
  }

  // int options = 0;
  // Changed by PH as a result of bugzilla #1288
  int options = (options_.all_options() & PCRE_NO_UTF8_CHECK);

  if (anchor != UNANCHORED)
    options |= PCRE_ANCHORED;
  if (!empty_ok)
    options |= PCRE_NOTEMPTY;

  int rc = pcre_exec(re,              // The regular expression object
                     &extra,
                     (text.data() == NULL) ? "" : text.data(),
                     text.size(),
                     startpos,
                     options,
                     vec,
                     vecsize);

  // Handle errors
  if (rc == PCRE_ERROR_NOMATCH) {
    return 0;
  } else if (rc < 0) {
    //fprintf(stderr, "Unexpected return code: %d when matching '%s'\n",
    //        re, pattern_.c_str());
    return 0;
  } else if (rc == 0) {
    // pcre_exec() returns 0 as a special case when the number of
    // capturing subpatterns exceeds the size of the vector.
    // When this happens, there is a match and the output vector
    // is filled, but we miss out on the positions of the extra subpatterns.
    rc = vecsize / 2;
  }

  return rc;
}

bool RE::DoMatchImpl(const StringPiece& text,
                     Anchor anchor,
                     int* consumed,
                     const Arg* const* args,
                     int n,
                     int* vec,
                     int vecsize) const {
  assert((1 + n) * 3 <= vecsize);  // results + PCRE workspace
  int matches = TryMatch(text, 0, anchor, true, vec, vecsize);
  assert(matches >= 0);  // TryMatch never returns negatives
  if (matches == 0)
    return false;

  *consumed = vec[1];

  if (n == 0 || args == NULL) {
    // We are not interested in results
    return true;
  }

  if (NumberOfCapturingGroups() < n) {
    // RE has fewer capturing groups than number of arg pointers passed in
    return false;
  }

  // If we got here, we must have matched the whole pattern.
  // We do not need (can not do) any more checks on the value of 'matches' here
  // -- see the comment for TryMatch.
  for (int i = 0; i < n; i++) {
    const int start = vec[2*(i+1)];
    const int limit = vec[2*(i+1)+1];
    if (!args[i]->Parse(text.data() + start, limit-start)) {
      // TODO: Should we indicate what the error was?
      return false;
    }
  }

  return true;
}

bool RE::DoMatch(const StringPiece& text,
                 Anchor anchor,
                 int* consumed,
                 const Arg* const args[],
                 int n) const {
  assert(n >= 0);
  size_t const vecsize = (1 + n) * 3;  // results + PCRE workspace
                                       // (as for kVecSize)
  int space[21];   // use stack allocation for small vecsize (common case)
  int* vec = vecsize <= 21 ? space : new int[vecsize];
  bool retval = DoMatchImpl(text, anchor, consumed, args, n, vec, (int)vecsize);
  if (vec != space) delete [] vec;
  return retval;
}

bool RE::Rewrite(string *out, const StringPiece &rewrite,
                 const StringPiece &text, int *vec, int veclen) const {
  for (const char *s = rewrite.data(), *end = s + rewrite.size();
       s < end; s++) {
    int c = *s;
    if (c == '\\') {
      c = *++s;
      if (isdigit(c)) {
        int n = (c - '0');
        if (n >= veclen) {
          //fprintf(stderr, requested group %d in regexp %.*s\n",
          //        n, rewrite.size(), rewrite.data());
          return false;
        }
        int start = vec[2 * n];
        if (start >= 0)
          out->append(text.data() + start, vec[2 * n + 1] - start);
      } else if (c == '\\') {
        *out += '\\';
      } else {
        //fprintf(stderr, "invalid rewrite pattern: %.*s\n",
        //        rewrite.size(), rewrite.data());
        return false;
      }
    } else {
      *out += c;
    }
  }
  return true;
}

// Return the number of capturing subpatterns, or -1 if the
// regexp wasn't valid on construction.
int RE::NumberOfCapturingGroups() const {
  if (re_partial_ == NULL) return -1;

  int result;
  int pcre_retval = pcre_fullinfo(re_partial_,  // The regular expression object
                                  NULL,         // We did not study the pattern
                                  PCRE_INFO_CAPTURECOUNT,
                                  &result);
  assert(pcre_retval == 0);
  return result;
}

/***** Parsers for various types *****/

bool Arg::parse_null(const char* str, int n, void* dest) {
  (void)str;
  (void)n;
  // We fail if somebody asked us to store into a non-NULL void* pointer
  return (dest == NULL);
}

bool Arg::parse_string(const char* str, int n, void* dest) {
  if (dest == NULL) return true;
  reinterpret_cast<string*>(dest)->assign(str, n);
  return true;
}

bool Arg::parse_stringpiece(const char* str, int n, void* dest) {
  if (dest == NULL) return true;
  reinterpret_cast<StringPiece*>(dest)->set(str, n);
  return true;
}

bool Arg::parse_char(const char* str, int n, void* dest) {
  if (n != 1) return false;
  if (dest == NULL) return true;
  *(reinterpret_cast<char*>(dest)) = str[0];
  return true;
}

bool Arg::parse_uchar(const char* str, int n, void* dest) {
  if (n != 1) return false;
  if (dest == NULL) return true;
  *(reinterpret_cast<unsigned char*>(dest)) = str[0];
  return true;
}

// Largest number spec that we are willing to parse
static const int kMaxNumberLength = 32;

// REQUIRES "buf" must have length at least kMaxNumberLength+1
// REQUIRES "n > 0"
// Copies "str" into "buf" and null-terminates if necessary.
// Returns one of:
//      a. "str" if no termination is needed
//      b. "buf" if the string was copied and null-terminated
//      c. "" if the input was invalid and has no hope of being parsed
static const char* TerminateNumber(char* buf, const char* str, int n) {
  if ((n > 0) && isspace(*str)) {
    // We are less forgiving than the strtoxxx() routines and do not
    // allow leading spaces.
    return "";
  }

  // See if the character right after the input text may potentially
  // look like a digit.
  if (isdigit(str[n]) ||
      ((str[n] >= 'a') && (str[n] <= 'f')) ||
      ((str[n] >= 'A') && (str[n] <= 'F'))) {
    if (n > kMaxNumberLength) return ""; // Input too big to be a valid number
    memcpy(buf, str, n);
    buf[n] = '\0';
    return buf;
  } else {
    // We can parse right out of the supplied string, so return it.
    return str;
  }
}

bool Arg::parse_long_radix(const char* str,
                           int n,
                           void* dest,
                           int radix) {
  if (n == 0) return false;
  char buf[kMaxNumberLength+1];
  str = TerminateNumber(buf, str, n);
  char* end;
  errno = 0;
  long r = strtol(str, &end, radix);
  if (end != str + n) return false;   // Leftover junk
  if (errno) return false;
  if (dest == NULL) return true;
  *(reinterpret_cast<long*>(dest)) = r;
  return true;
}

bool Arg::parse_ulong_radix(const char* str,
                            int n,
                            void* dest,
                            int radix) {
  if (n == 0) return false;
  char buf[kMaxNumberLength+1];
  str = TerminateNumber(buf, str, n);
  if (str[0] == '-') return false;    // strtoul() on a negative number?!
  char* end;
  errno = 0;
  unsigned long r = strtoul(str, &end, radix);
  if (end != str + n) return false;   // Leftover junk
  if (errno) return false;
  if (dest == NULL) return true;
  *(reinterpret_cast<unsigned long*>(dest)) = r;
  return true;
}

bool Arg::parse_short_radix(const char* str,
                            int n,
                            void* dest,
                            int radix) {
  long r;
  if (!parse_long_radix(str, n, &r, radix)) return false; // Could not parse
  if (r < SHRT_MIN || r > SHRT_MAX) return false;       // Out of range
  if (dest == NULL) return true;
  *(reinterpret_cast<short*>(dest)) = static_cast<short>(r);
  return true;
}

bool Arg::parse_ushort_radix(const char* str,
                             int n,
                             void* dest,
                             int radix) {
  unsigned long r;
  if (!parse_ulong_radix(str, n, &r, radix)) return false; // Could not parse
  if (r > USHRT_MAX) return false;                      // Out of range
  if (dest == NULL) return true;
  *(reinterpret_cast<unsigned short*>(dest)) = static_cast<unsigned short>(r);
  return true;
}

bool Arg::parse_int_radix(const char* str,
                          int n,
                          void* dest,
                          int radix) {
  long r;
  if (!parse_long_radix(str, n, &r, radix)) return false; // Could not parse
  if (r < INT_MIN || r > INT_MAX) return false;         // Out of range
  if (dest == NULL) return true;
  *(reinterpret_cast<int*>(dest)) = r;
  return true;
}

bool Arg::parse_uint_radix(const char* str,
                           int n,
                           void* dest,
                           int radix) {
  unsigned long r;
  if (!parse_ulong_radix(str, n, &r, radix)) return false; // Could not parse
  if (r > UINT_MAX) return false;                       // Out of range
  if (dest == NULL) return true;
  *(reinterpret_cast<unsigned int*>(dest)) = r;
  return true;
}

bool Arg::parse_longlong_radix(const char* str,
                               int n,
                               void* dest,
                               int radix) {
#ifndef HAVE_LONG_LONG
  return false;
#else
  if (n == 0) return false;
  char buf[kMaxNumberLength+1];
  str = TerminateNumber(buf, str, n);
  char* end;
  errno = 0;
#if defined HAVE_STRTOQ
  long long r = strtoq(str, &end, radix);
#elif defined HAVE_STRTOLL
  long long r = strtoll(str, &end, radix);
#elif defined HAVE__STRTOI64
  long long r = _strtoi64(str, &end, radix);
#elif defined HAVE_STRTOIMAX
  long long r = strtoimax(str, &end, radix);
#else
#error parse_longlong_radix: cannot convert input to a long-long
#endif
  if (end != str + n) return false;   // Leftover junk
  if (errno) return false;
  if (dest == NULL) return true;
  *(reinterpret_cast<long long*>(dest)) = r;
  return true;
#endif   /* HAVE_LONG_LONG */
}

bool Arg::parse_ulonglong_radix(const char* str,
                                int n,
                                void* dest,
                                int radix) {
#ifndef HAVE_UNSIGNED_LONG_LONG
  return false;
#else
  if (n == 0) return false;
  char buf[kMaxNumberLength+1];
  str = TerminateNumber(buf, str, n);
  if (str[0] == '-') return false;    // strtoull() on a negative number?!
  char* end;
  errno = 0;
#if defined HAVE_STRTOQ
  unsigned long long r = strtouq(str, &end, radix);
#elif defined HAVE_STRTOLL
  unsigned long long r = strtoull(str, &end, radix);
#elif defined HAVE__STRTOI64
  unsigned long long r = _strtoui64(str, &end, radix);
#elif defined HAVE_STRTOIMAX
  unsigned long long r = strtoumax(str, &end, radix);
#else
#error parse_ulonglong_radix: cannot convert input to a long-long
#endif
  if (end != str + n) return false;   // Leftover junk
  if (errno) return false;
  if (dest == NULL) return true;
  *(reinterpret_cast<unsigned long long*>(dest)) = r;
  return true;
#endif   /* HAVE_UNSIGNED_LONG_LONG */
}

bool Arg::parse_double(const char* str, int n, void* dest) {
  if (n == 0) return false;
  static const int kMaxLength = 200;
  char buf[kMaxLength];
  if (n >= kMaxLength) return false;
  memcpy(buf, str, n);
  buf[n] = '\0';
  errno = 0;
  char* end;
  double r = strtod(buf, &end);
  if (end != buf + n) return false;   // Leftover junk
  if (errno) return false;
  if (dest == NULL) return true;
  *(reinterpret_cast<double*>(dest)) = r;
  return true;
}

bool Arg::parse_float(const char* str, int n, void* dest) {
  double r;
  if (!parse_double(str, n, &r)) return false;
  if (dest == NULL) return true;
  *(reinterpret_cast<float*>(dest)) = static_cast<float>(r);
  return true;
}


#define DEFINE_INTEGER_PARSERS(name)                                    \
  bool Arg::parse_##name(const char* str, int n, void* dest) {          \
    return parse_##name##_radix(str, n, dest, 10);                      \
  }                                                                     \
  bool Arg::parse_##name##_hex(const char* str, int n, void* dest) {    \
    return parse_##name##_radix(str, n, dest, 16);                      \
  }                                                                     \
  bool Arg::parse_##name##_octal(const char* str, int n, void* dest) {  \
    return parse_##name##_radix(str, n, dest, 8);                       \
  }                                                                     \
  bool Arg::parse_##name##_cradix(const char* str, int n, void* dest) { \
    return parse_##name##_radix(str, n, dest, 0);                       \
  }

DEFINE_INTEGER_PARSERS(short)      /*                                   */
DEFINE_INTEGER_PARSERS(ushort)     /*                                   */
DEFINE_INTEGER_PARSERS(int)        /* Don't use semicolons after these  */
DEFINE_INTEGER_PARSERS(uint)       /* statements because they can cause */
DEFINE_INTEGER_PARSERS(long)       /* compiler warnings if the checking */
DEFINE_INTEGER_PARSERS(ulong)      /* level is turned up high enough.   */
DEFINE_INTEGER_PARSERS(longlong)   /*                                   */
DEFINE_INTEGER_PARSERS(ulonglong)  /*                                   */

#undef DEFINE_INTEGER_PARSERS

}   // namespace pcrecpp

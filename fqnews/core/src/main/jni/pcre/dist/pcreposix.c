/*************************************************
*      Perl-Compatible Regular Expressions       *
*************************************************/

/* PCRE is a library of functions to support regular expressions whose syntax
and semantics are as close as possible to those of the Perl 5 language.

                       Written by Philip Hazel
           Copyright (c) 1997-2014 University of Cambridge

-----------------------------------------------------------------------------
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of the University of Cambridge nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------------
*/


/* This module is a wrapper that provides a POSIX API to the underlying PCRE
functions. */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


/* Ensure that the PCREPOSIX_EXP_xxx macros are set appropriately for
compiling these functions. This must come before including pcreposix.h, where
they are set for an application (using these functions) if they have not
previously been set. */

#if defined(_WIN32) && !defined(PCRE_STATIC)
#  define PCREPOSIX_EXP_DECL extern __declspec(dllexport)
#  define PCREPOSIX_EXP_DEFN __declspec(dllexport)
#endif

/* We include pcre.h before pcre_internal.h so that the PCRE library functions
are declared as "import" for Windows by defining PCRE_EXP_DECL as "import".
This is needed even though pcre_internal.h itself includes pcre.h, because it
does so after it has set PCRE_EXP_DECL to "export" if it is not already set. */

#include "pcre.h"
#include "pcre_internal.h"
#include "pcreposix.h"


/* Table to translate PCRE compile time error codes into POSIX error codes. */

static const int eint[] = {
  0,           /* no error */
  REG_EESCAPE, /* \ at end of pattern */
  REG_EESCAPE, /* \c at end of pattern */
  REG_EESCAPE, /* unrecognized character follows \ */
  REG_BADBR,   /* numbers out of order in {} quantifier */
  /* 5 */
  REG_BADBR,   /* number too big in {} quantifier */
  REG_EBRACK,  /* missing terminating ] for character class */
  REG_ECTYPE,  /* invalid escape sequence in character class */
  REG_ERANGE,  /* range out of order in character class */
  REG_BADRPT,  /* nothing to repeat */
  /* 10 */
  REG_BADRPT,  /* operand of unlimited repeat could match the empty string */
  REG_ASSERT,  /* internal error: unexpected repeat */
  REG_BADPAT,  /* unrecognized character after (? */
  REG_BADPAT,  /* POSIX named classes are supported only within a class */
  REG_EPAREN,  /* missing ) */
  /* 15 */
  REG_ESUBREG, /* reference to non-existent subpattern */
  REG_INVARG,  /* erroffset passed as NULL */
  REG_INVARG,  /* unknown option bit(s) set */
  REG_EPAREN,  /* missing ) after comment */
  REG_ESIZE,   /* parentheses nested too deeply */
  /* 20 */
  REG_ESIZE,   /* regular expression too large */
  REG_ESPACE,  /* failed to get memory */
  REG_EPAREN,  /* unmatched parentheses */
  REG_ASSERT,  /* internal error: code overflow */
  REG_BADPAT,  /* unrecognized character after (?< */
  /* 25 */
  REG_BADPAT,  /* lookbehind assertion is not fixed length */
  REG_BADPAT,  /* malformed number or name after (?( */
  REG_BADPAT,  /* conditional group contains more than two branches */
  REG_BADPAT,  /* assertion expected after (?( */
  REG_BADPAT,  /* (?R or (?[+-]digits must be followed by ) */
  /* 30 */
  REG_ECTYPE,  /* unknown POSIX class name */
  REG_BADPAT,  /* POSIX collating elements are not supported */
  REG_INVARG,  /* this version of PCRE is not compiled with PCRE_UTF8 support */
  REG_BADPAT,  /* spare error */
  REG_BADPAT,  /* character value in \x{} or \o{} is too large */
  /* 35 */
  REG_BADPAT,  /* invalid condition (?(0) */
  REG_BADPAT,  /* \C not allowed in lookbehind assertion */
  REG_EESCAPE, /* PCRE does not support \L, \l, \N, \U, or \u */
  REG_BADPAT,  /* number after (?C is > 255 */
  REG_BADPAT,  /* closing ) for (?C expected */
  /* 40 */
  REG_BADPAT,  /* recursive call could loop indefinitely */
  REG_BADPAT,  /* unrecognized character after (?P */
  REG_BADPAT,  /* syntax error in subpattern name (missing terminator) */
  REG_BADPAT,  /* two named subpatterns have the same name */
  REG_BADPAT,  /* invalid UTF-8 string */
  /* 45 */
  REG_BADPAT,  /* support for \P, \p, and \X has not been compiled */
  REG_BADPAT,  /* malformed \P or \p sequence */
  REG_BADPAT,  /* unknown property name after \P or \p */
  REG_BADPAT,  /* subpattern name is too long (maximum 32 characters) */
  REG_BADPAT,  /* too many named subpatterns (maximum 10,000) */
  /* 50 */
  REG_BADPAT,  /* repeated subpattern is too long */
  REG_BADPAT,  /* octal value is greater than \377 (not in UTF-8 mode) */
  REG_BADPAT,  /* internal error: overran compiling workspace */
  REG_BADPAT,  /* internal error: previously-checked referenced subpattern not found */
  REG_BADPAT,  /* DEFINE group contains more than one branch */
  /* 55 */
  REG_BADPAT,  /* repeating a DEFINE group is not allowed */
  REG_INVARG,  /* inconsistent NEWLINE options */
  REG_BADPAT,  /* \g is not followed followed by an (optionally braced) non-zero number */
  REG_BADPAT,  /* a numbered reference must not be zero */
  REG_BADPAT,  /* an argument is not allowed for (*ACCEPT), (*FAIL), or (*COMMIT) */
  /* 60 */
  REG_BADPAT,  /* (*VERB) not recognized */
  REG_BADPAT,  /* number is too big */
  REG_BADPAT,  /* subpattern name expected */
  REG_BADPAT,  /* digit expected after (?+ */
  REG_BADPAT,  /* ] is an invalid data character in JavaScript compatibility mode */
  /* 65 */
  REG_BADPAT,  /* different names for subpatterns of the same number are not allowed */
  REG_BADPAT,  /* (*MARK) must have an argument */
  REG_INVARG,  /* this version of PCRE is not compiled with PCRE_UCP support */
  REG_BADPAT,  /* \c must be followed by an ASCII character */
  REG_BADPAT,  /* \k is not followed by a braced, angle-bracketed, or quoted name */
  /* 70 */
  REG_BADPAT,  /* internal error: unknown opcode in find_fixedlength() */
  REG_BADPAT,  /* \N is not supported in a class */
  REG_BADPAT,  /* too many forward references */
  REG_BADPAT,  /* disallowed UTF-8/16/32 code point (>= 0xd800 && <= 0xdfff) */
  REG_BADPAT,  /* invalid UTF-16 string (should not occur) */
  /* 75 */
  REG_BADPAT,  /* overlong MARK name */
  REG_BADPAT,  /* character value in \u.... sequence is too large */
  REG_BADPAT,  /* invalid UTF-32 string (should not occur) */
  REG_BADPAT,  /* setting UTF is disabled by the application */
  REG_BADPAT,  /* non-hex character in \\x{} (closing brace missing?) */
  /* 80 */
  REG_BADPAT,  /* non-octal character in \o{} (closing brace missing?) */
  REG_BADPAT,  /* missing opening brace after \o */
  REG_BADPAT,  /* parentheses too deeply nested */
  REG_BADPAT,  /* invalid range in character class */
  REG_BADPAT,  /* group name must start with a non-digit */
  /* 85 */
  REG_BADPAT,  /* parentheses too deeply nested (stack check) */
  REG_BADPAT   /* missing digits in \x{} or \o{} */
};

/* Table of texts corresponding to POSIX error codes */

static const char *const pstring[] = {
  "",                                /* Dummy for value 0 */
  "internal error",                  /* REG_ASSERT */
  "invalid repeat counts in {}",     /* BADBR      */
  "pattern error",                   /* BADPAT     */
  "? * + invalid",                   /* BADRPT     */
  "unbalanced {}",                   /* EBRACE     */
  "unbalanced []",                   /* EBRACK     */
  "collation error - not relevant",  /* ECOLLATE   */
  "bad class",                       /* ECTYPE     */
  "bad escape sequence",             /* EESCAPE    */
  "empty expression",                /* EMPTY      */
  "unbalanced ()",                   /* EPAREN     */
  "bad range inside []",             /* ERANGE     */
  "expression too big",              /* ESIZE      */
  "failed to get memory",            /* ESPACE     */
  "bad back reference",              /* ESUBREG    */
  "bad argument",                    /* INVARG     */
  "match failed"                     /* NOMATCH    */
};




/*************************************************
*          Translate error code to string        *
*************************************************/

PCREPOSIX_EXP_DEFN size_t PCRE_CALL_CONVENTION
regerror(int errcode, const regex_t *preg, char *errbuf, size_t errbuf_size)
{
const char *message, *addmessage;
size_t length, addlength;

message = (errcode >= (int)(sizeof(pstring)/sizeof(char *)))?
  "unknown error code" : pstring[errcode];
length = strlen(message) + 1;

addmessage = " at offset ";
addlength = (preg != NULL && (int)preg->re_erroffset != -1)?
  strlen(addmessage) + 6 : 0;

if (errbuf_size > 0)
  {
  if (addlength > 0 && errbuf_size >= length + addlength)
    sprintf(errbuf, "%s%s%-6d", message, addmessage, (int)preg->re_erroffset);
  else
    {
    strncpy(errbuf, message, errbuf_size - 1);
    errbuf[errbuf_size-1] = 0;
    }
  }

return length + addlength;
}




/*************************************************
*           Free store held by a regex           *
*************************************************/

PCREPOSIX_EXP_DEFN void PCRE_CALL_CONVENTION
regfree(regex_t *preg)
{
(PUBL(free))(preg->re_pcre);
}




/*************************************************
*            Compile a regular expression        *
*************************************************/

/*
Arguments:
  preg        points to a structure for recording the compiled expression
  pattern     the pattern to compile
  cflags      compilation flags

Returns:      0 on success
              various non-zero codes on failure
*/

PCREPOSIX_EXP_DEFN int PCRE_CALL_CONVENTION
regcomp(regex_t *preg, const char *pattern, int cflags)
{
const char *errorptr;
int erroffset;
int errorcode;
int options = 0;
int re_nsub = 0;

if ((cflags & REG_ICASE) != 0)    options |= PCRE_CASELESS;
if ((cflags & REG_NEWLINE) != 0)  options |= PCRE_MULTILINE;
if ((cflags & REG_DOTALL) != 0)   options |= PCRE_DOTALL;
if ((cflags & REG_NOSUB) != 0)    options |= PCRE_NO_AUTO_CAPTURE;
if ((cflags & REG_UTF8) != 0)     options |= PCRE_UTF8;
if ((cflags & REG_UCP) != 0)      options |= PCRE_UCP;
if ((cflags & REG_UNGREEDY) != 0) options |= PCRE_UNGREEDY;

preg->re_pcre = pcre_compile2(pattern, options, &errorcode, &errorptr,
  &erroffset, NULL);
preg->re_erroffset = erroffset;

/* Safety: if the error code is too big for the translation vector (which
should not happen, but we all make mistakes), return REG_BADPAT. */

if (preg->re_pcre == NULL)
  {
  return (errorcode < (int)(sizeof(eint)/sizeof(const int)))?
    eint[errorcode] : REG_BADPAT;
  }

(void)pcre_fullinfo((const pcre *)preg->re_pcre, NULL, PCRE_INFO_CAPTURECOUNT,
  &re_nsub);
preg->re_nsub = (size_t)re_nsub;
return 0;
}




/*************************************************
*              Match a regular expression        *
*************************************************/

/* Unfortunately, PCRE requires 3 ints of working space for each captured
substring, so we have to get and release working store instead of just using
the POSIX structures as was done in earlier releases when PCRE needed only 2
ints. However, if the number of possible capturing brackets is small, use a
block of store on the stack, to reduce the use of malloc/free. The threshold is
in a macro that can be changed at configure time.

If REG_NOSUB was specified at compile time, the PCRE_NO_AUTO_CAPTURE flag will
be set. When this is the case, the nmatch and pmatch arguments are ignored, and
the only result is yes/no/error. */

PCREPOSIX_EXP_DEFN int PCRE_CALL_CONVENTION
regexec(const regex_t *preg, const char *string, size_t nmatch,
  regmatch_t pmatch[], int eflags)
{
int rc, so, eo;
int options = 0;
int *ovector = NULL;
int small_ovector[POSIX_MALLOC_THRESHOLD * 3];
BOOL allocated_ovector = FALSE;
BOOL nosub =
  (REAL_PCRE_OPTIONS((const pcre *)preg->re_pcre) & PCRE_NO_AUTO_CAPTURE) != 0;

if ((eflags & REG_NOTBOL) != 0) options |= PCRE_NOTBOL;
if ((eflags & REG_NOTEOL) != 0) options |= PCRE_NOTEOL;
if ((eflags & REG_NOTEMPTY) != 0) options |= PCRE_NOTEMPTY;

((regex_t *)preg)->re_erroffset = (size_t)(-1);  /* Only has meaning after compile */

/* When no string data is being returned, or no vector has been passed in which
to put it, ensure that nmatch is zero. Otherwise, ensure the vector for holding
the return data is large enough. */

if (nosub || pmatch == NULL) nmatch = 0;

else if (nmatch > 0)
  {
  if (nmatch <= POSIX_MALLOC_THRESHOLD)
    {
    ovector = &(small_ovector[0]);
    }
  else
    {
    if (nmatch > INT_MAX/(sizeof(int) * 3)) return REG_ESPACE;
    ovector = (int *)malloc(sizeof(int) * nmatch * 3);
    if (ovector == NULL) return REG_ESPACE;
    allocated_ovector = TRUE;
    }
  }

/* REG_STARTEND is a BSD extension, to allow for non-NUL-terminated strings.
The man page from OS X says "REG_STARTEND affects only the location of the
string, not how it is matched". That is why the "so" value is used to bump the
start location rather than being passed as a PCRE "starting offset". */

if ((eflags & REG_STARTEND) != 0)
  {
  so = pmatch[0].rm_so;
  eo = pmatch[0].rm_eo;
  }
else
  {
  so = 0;
  eo = (int)strlen(string);
  }

rc = pcre_exec((const pcre *)preg->re_pcre, NULL, string + so, (eo - so),
  0, options, ovector, (int)(nmatch * 3));

if (rc == 0) rc = (int)nmatch;    /* All captured slots were filled in */

/* Successful match */

if (rc >= 0)
  {
  size_t i;
  if (!nosub)
    {
    for (i = 0; i < (size_t)rc; i++)
      {
      pmatch[i].rm_so = ovector[i*2];
      pmatch[i].rm_eo = ovector[i*2+1];
      }
    if (allocated_ovector) free(ovector);
    for (; i < nmatch; i++) pmatch[i].rm_so = pmatch[i].rm_eo = -1;
    }
  return 0;
  }

/* Unsuccessful match */

if (allocated_ovector) free(ovector);
switch(rc)
  {
/* ========================================================================== */
  /* These cases are never obeyed. This is a fudge that causes a compile-time
  error if the vector eint, which is indexed by compile-time error number, is
  not the correct length. It seems to be the only way to do such a check at
  compile time, as the sizeof() operator does not work in the C preprocessor.
  As all the PCRE_ERROR_xxx values are negative, we can use 0 and 1. */

  case 0:
  case (sizeof(eint)/sizeof(int) == ERRCOUNT):
  return REG_ASSERT;
/* ========================================================================== */

  case PCRE_ERROR_NOMATCH: return REG_NOMATCH;
  case PCRE_ERROR_NULL: return REG_INVARG;
  case PCRE_ERROR_BADOPTION: return REG_INVARG;
  case PCRE_ERROR_BADMAGIC: return REG_INVARG;
  case PCRE_ERROR_UNKNOWN_NODE: return REG_ASSERT;
  case PCRE_ERROR_NOMEMORY: return REG_ESPACE;
  case PCRE_ERROR_MATCHLIMIT: return REG_ESPACE;
  case PCRE_ERROR_BADUTF8: return REG_INVARG;
  case PCRE_ERROR_BADUTF8_OFFSET: return REG_INVARG;
  case PCRE_ERROR_BADMODE: return REG_INVARG;
  default: return REG_ASSERT;
  }
}

/* End of pcreposix.c */

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


/* This module contains the external function pcre_compile(), along with
supporting internal functions that are not used by other modules. */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define NLBLOCK cd             /* Block containing newline information */
#define PSSTART start_pattern  /* Field containing pattern start */
#define PSEND   end_pattern    /* Field containing pattern end */

#include "pcre_internal.h"


/* When PCRE_DEBUG is defined, we need the pcre(16|32)_printint() function, which
is also used by pcretest. PCRE_DEBUG is not defined when building a production
library. We do not need to select pcre16_printint.c specially, because the
COMPILE_PCREx macro will already be appropriately set. */

#ifdef PCRE_DEBUG
/* pcre_printint.c should not include any headers */
#define PCRE_INCLUDED
#include "pcre_printint.c"
#undef PCRE_INCLUDED
#endif


/* Macro for setting individual bits in class bitmaps. */

#define SETBIT(a,b) a[(b)/8] |= (1 << ((b)&7))

/* Maximum length value to check against when making sure that the integer that
holds the compiled pattern length does not overflow. We make it a bit less than
INT_MAX to allow for adding in group terminating bytes, so that we don't have
to check them every time. */

#define OFLOW_MAX (INT_MAX - 20)

/* Definitions to allow mutual recursion */

static int
  add_list_to_class(pcre_uint8 *, pcre_uchar **, int, compile_data *,
    const pcre_uint32 *, unsigned int);

static BOOL
  compile_regex(int, pcre_uchar **, const pcre_uchar **, int *, BOOL, BOOL, int, int,
    pcre_uint32 *, pcre_int32 *, pcre_uint32 *, pcre_int32 *, branch_chain *,
    compile_data *, int *);



/*************************************************
*      Code parameters and static tables         *
*************************************************/

/* This value specifies the size of stack workspace that is used during the
first pre-compile phase that determines how much memory is required. The regex
is partly compiled into this space, but the compiled parts are discarded as
soon as they can be, so that hopefully there will never be an overrun. The code
does, however, check for an overrun. The largest amount I've seen used is 218,
so this number is very generous.

The same workspace is used during the second, actual compile phase for
remembering forward references to groups so that they can be filled in at the
end. Each entry in this list occupies LINK_SIZE bytes, so even when LINK_SIZE
is 4 there is plenty of room for most patterns. However, the memory can get
filled up by repetitions of forward references, for example patterns like
/(?1){0,1999}(b)/, and one user did hit the limit. The code has been changed so
that the workspace is expanded using malloc() in this situation. The value
below is therefore a minimum, and we put a maximum on it for safety. The
minimum is now also defined in terms of LINK_SIZE so that the use of malloc()
kicks in at the same number of forward references in all cases. */

#define COMPILE_WORK_SIZE (2048*LINK_SIZE)
#define COMPILE_WORK_SIZE_MAX (100*COMPILE_WORK_SIZE)

/* This value determines the size of the initial vector that is used for
remembering named groups during the pre-compile. It is allocated on the stack,
but if it is too small, it is expanded using malloc(), in a similar way to the
workspace. The value is the number of slots in the list. */

#define NAMED_GROUP_LIST_SIZE  20

/* The overrun tests check for a slightly smaller size so that they detect the
overrun before it actually does run off the end of the data block. */

#define WORK_SIZE_SAFETY_MARGIN (100)

/* Private flags added to firstchar and reqchar. */

#define REQ_CASELESS    (1 << 0)        /* Indicates caselessness */
#define REQ_VARY        (1 << 1)        /* Reqchar followed non-literal item */
/* Negative values for the firstchar and reqchar flags */
#define REQ_UNSET       (-2)
#define REQ_NONE        (-1)

/* Repeated character flags. */

#define UTF_LENGTH     0x10000000l      /* The char contains its length. */

/* Table for handling escaped characters in the range '0'-'z'. Positive returns
are simple data values; negative values are for special things like \d and so
on. Zero means further processing is needed (for things like \x), or the escape
is invalid. */

#ifndef EBCDIC

/* This is the "normal" table for ASCII systems or for EBCDIC systems running
in UTF-8 mode. */

static const short int escapes[] = {
     0,                       0,
     0,                       0,
     0,                       0,
     0,                       0,
     0,                       0,
     CHAR_COLON,              CHAR_SEMICOLON,
     CHAR_LESS_THAN_SIGN,     CHAR_EQUALS_SIGN,
     CHAR_GREATER_THAN_SIGN,  CHAR_QUESTION_MARK,
     CHAR_COMMERCIAL_AT,      -ESC_A,
     -ESC_B,                  -ESC_C,
     -ESC_D,                  -ESC_E,
     0,                       -ESC_G,
     -ESC_H,                  0,
     0,                       -ESC_K,
     0,                       0,
     -ESC_N,                  0,
     -ESC_P,                  -ESC_Q,
     -ESC_R,                  -ESC_S,
     0,                       0,
     -ESC_V,                  -ESC_W,
     -ESC_X,                  0,
     -ESC_Z,                  CHAR_LEFT_SQUARE_BRACKET,
     CHAR_BACKSLASH,          CHAR_RIGHT_SQUARE_BRACKET,
     CHAR_CIRCUMFLEX_ACCENT,  CHAR_UNDERSCORE,
     CHAR_GRAVE_ACCENT,       ESC_a,
     -ESC_b,                  0,
     -ESC_d,                  ESC_e,
     ESC_f,                   0,
     -ESC_h,                  0,
     0,                       -ESC_k,
     0,                       0,
     ESC_n,                   0,
     -ESC_p,                  0,
     ESC_r,                   -ESC_s,
     ESC_tee,                 0,
     -ESC_v,                  -ESC_w,
     0,                       0,
     -ESC_z
};

#else

/* This is the "abnormal" table for EBCDIC systems without UTF-8 support. */

static const short int escapes[] = {
/*  48 */     0,     0,      0,     '.',    '<',   '(',    '+',    '|',
/*  50 */   '&',     0,      0,       0,      0,     0,      0,      0,
/*  58 */     0,     0,    '!',     '$',    '*',   ')',    ';',    '~',
/*  60 */   '-',   '/',      0,       0,      0,     0,      0,      0,
/*  68 */     0,     0,    '|',     ',',    '%',   '_',    '>',    '?',
/*  70 */     0,     0,      0,       0,      0,     0,      0,      0,
/*  78 */     0,   '`',    ':',     '#',    '@',  '\'',    '=',    '"',
/*  80 */     0, ESC_a, -ESC_b,       0, -ESC_d, ESC_e,  ESC_f,      0,
/*  88 */-ESC_h,     0,      0,     '{',      0,     0,      0,      0,
/*  90 */     0,     0, -ESC_k,       0,      0, ESC_n,      0, -ESC_p,
/*  98 */     0, ESC_r,      0,     '}',      0,     0,      0,      0,
/*  A0 */     0,   '~', -ESC_s, ESC_tee,      0,-ESC_v, -ESC_w,      0,
/*  A8 */     0,-ESC_z,      0,       0,      0,   '[',      0,      0,
/*  B0 */     0,     0,      0,       0,      0,     0,      0,      0,
/*  B8 */     0,     0,      0,       0,      0,   ']',    '=',    '-',
/*  C0 */   '{',-ESC_A, -ESC_B,  -ESC_C, -ESC_D,-ESC_E,      0, -ESC_G,
/*  C8 */-ESC_H,     0,      0,       0,      0,     0,      0,      0,
/*  D0 */   '}',     0, -ESC_K,       0,      0,-ESC_N,      0, -ESC_P,
/*  D8 */-ESC_Q,-ESC_R,      0,       0,      0,     0,      0,      0,
/*  E0 */  '\\',     0, -ESC_S,       0,      0,-ESC_V, -ESC_W, -ESC_X,
/*  E8 */     0,-ESC_Z,      0,       0,      0,     0,      0,      0,
/*  F0 */     0,     0,      0,       0,      0,     0,      0,      0,
/*  F8 */     0,     0,      0,       0,      0,     0,      0,      0
};

/* We also need a table of characters that may follow \c in an EBCDIC
environment for characters 0-31. */

static unsigned char ebcdic_escape_c[] = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_";

#endif


/* Table of special "verbs" like (*PRUNE). This is a short table, so it is
searched linearly. Put all the names into a single string, in order to reduce
the number of relocations when a shared library is dynamically linked. The
string is built from string macros so that it works in UTF-8 mode on EBCDIC
platforms. */

typedef struct verbitem {
  int   len;                 /* Length of verb name */
  int   op;                  /* Op when no arg, or -1 if arg mandatory */
  int   op_arg;              /* Op when arg present, or -1 if not allowed */
} verbitem;

static const char verbnames[] =
  "\0"                       /* Empty name is a shorthand for MARK */
  STRING_MARK0
  STRING_ACCEPT0
  STRING_COMMIT0
  STRING_F0
  STRING_FAIL0
  STRING_PRUNE0
  STRING_SKIP0
  STRING_THEN;

static const verbitem verbs[] = {
  { 0, -1,        OP_MARK },
  { 4, -1,        OP_MARK },
  { 6, OP_ACCEPT, -1 },
  { 6, OP_COMMIT, -1 },
  { 1, OP_FAIL,   -1 },
  { 4, OP_FAIL,   -1 },
  { 5, OP_PRUNE,  OP_PRUNE_ARG },
  { 4, OP_SKIP,   OP_SKIP_ARG  },
  { 4, OP_THEN,   OP_THEN_ARG  }
};

static const int verbcount = sizeof(verbs)/sizeof(verbitem);


/* Substitutes for [[:<:]] and [[:>:]], which mean start and end of word in
another regex library. */

static const pcre_uchar sub_start_of_word[] = {
  CHAR_BACKSLASH, CHAR_b, CHAR_LEFT_PARENTHESIS, CHAR_QUESTION_MARK,
  CHAR_EQUALS_SIGN, CHAR_BACKSLASH, CHAR_w, CHAR_RIGHT_PARENTHESIS, '\0' };

static const pcre_uchar sub_end_of_word[] = {
  CHAR_BACKSLASH, CHAR_b, CHAR_LEFT_PARENTHESIS, CHAR_QUESTION_MARK,
  CHAR_LESS_THAN_SIGN, CHAR_EQUALS_SIGN, CHAR_BACKSLASH, CHAR_w,
  CHAR_RIGHT_PARENTHESIS, '\0' };


/* Tables of names of POSIX character classes and their lengths. The names are
now all in a single string, to reduce the number of relocations when a shared
library is dynamically loaded. The list of lengths is terminated by a zero
length entry. The first three must be alpha, lower, upper, as this is assumed
for handling case independence. The indices for graph, print, and punct are
needed, so identify them. */

static const char posix_names[] =
  STRING_alpha0 STRING_lower0 STRING_upper0 STRING_alnum0
  STRING_ascii0 STRING_blank0 STRING_cntrl0 STRING_digit0
  STRING_graph0 STRING_print0 STRING_punct0 STRING_space0
  STRING_word0  STRING_xdigit;

static const pcre_uint8 posix_name_lengths[] = {
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 6, 0 };

#define PC_GRAPH  8
#define PC_PRINT  9
#define PC_PUNCT 10


/* Table of class bit maps for each POSIX class. Each class is formed from a
base map, with an optional addition or removal of another map. Then, for some
classes, there is some additional tweaking: for [:blank:] the vertical space
characters are removed, and for [:alpha:] and [:alnum:] the underscore
character is removed. The triples in the table consist of the base map offset,
second map offset or -1 if no second map, and a non-negative value for map
addition or a negative value for map subtraction (if there are two maps). The
absolute value of the third field has these meanings: 0 => no tweaking, 1 =>
remove vertical space characters, 2 => remove underscore. */

static const int posix_class_maps[] = {
  cbit_word,  cbit_digit, -2,             /* alpha */
  cbit_lower, -1,          0,             /* lower */
  cbit_upper, -1,          0,             /* upper */
  cbit_word,  -1,          2,             /* alnum - word without underscore */
  cbit_print, cbit_cntrl,  0,             /* ascii */
  cbit_space, -1,          1,             /* blank - a GNU extension */
  cbit_cntrl, -1,          0,             /* cntrl */
  cbit_digit, -1,          0,             /* digit */
  cbit_graph, -1,          0,             /* graph */
  cbit_print, -1,          0,             /* print */
  cbit_punct, -1,          0,             /* punct */
  cbit_space, -1,          0,             /* space */
  cbit_word,  -1,          0,             /* word - a Perl extension */
  cbit_xdigit,-1,          0              /* xdigit */
};

/* Table of substitutes for \d etc when PCRE_UCP is set. They are replaced by
Unicode property escapes. */

#ifdef SUPPORT_UCP
static const pcre_uchar string_PNd[]  = {
  CHAR_BACKSLASH, CHAR_P, CHAR_LEFT_CURLY_BRACKET,
  CHAR_N, CHAR_d, CHAR_RIGHT_CURLY_BRACKET, '\0' };
static const pcre_uchar string_pNd[]  = {
  CHAR_BACKSLASH, CHAR_p, CHAR_LEFT_CURLY_BRACKET,
  CHAR_N, CHAR_d, CHAR_RIGHT_CURLY_BRACKET, '\0' };
static const pcre_uchar string_PXsp[] = {
  CHAR_BACKSLASH, CHAR_P, CHAR_LEFT_CURLY_BRACKET,
  CHAR_X, CHAR_s, CHAR_p, CHAR_RIGHT_CURLY_BRACKET, '\0' };
static const pcre_uchar string_pXsp[] = {
  CHAR_BACKSLASH, CHAR_p, CHAR_LEFT_CURLY_BRACKET,
  CHAR_X, CHAR_s, CHAR_p, CHAR_RIGHT_CURLY_BRACKET, '\0' };
static const pcre_uchar string_PXwd[] = {
  CHAR_BACKSLASH, CHAR_P, CHAR_LEFT_CURLY_BRACKET,
  CHAR_X, CHAR_w, CHAR_d, CHAR_RIGHT_CURLY_BRACKET, '\0' };
static const pcre_uchar string_pXwd[] = {
  CHAR_BACKSLASH, CHAR_p, CHAR_LEFT_CURLY_BRACKET,
  CHAR_X, CHAR_w, CHAR_d, CHAR_RIGHT_CURLY_BRACKET, '\0' };

static const pcre_uchar *substitutes[] = {
  string_PNd,           /* \D */
  string_pNd,           /* \d */
  string_PXsp,          /* \S */   /* Xsp is Perl space, but from 8.34, Perl */
  string_pXsp,          /* \s */   /* space and POSIX space are the same. */
  string_PXwd,          /* \W */
  string_pXwd           /* \w */
};

/* The POSIX class substitutes must be in the order of the POSIX class names,
defined above, and there are both positive and negative cases. NULL means no
general substitute of a Unicode property escape (\p or \P). However, for some
POSIX classes (e.g. graph, print, punct) a special property code is compiled
directly. */

static const pcre_uchar string_pL[] =   {
  CHAR_BACKSLASH, CHAR_p, CHAR_LEFT_CURLY_BRACKET,
  CHAR_L, CHAR_RIGHT_CURLY_BRACKET, '\0' };
static const pcre_uchar string_pLl[] =  {
  CHAR_BACKSLASH, CHAR_p, CHAR_LEFT_CURLY_BRACKET,
  CHAR_L, CHAR_l, CHAR_RIGHT_CURLY_BRACKET, '\0' };
static const pcre_uchar string_pLu[] =  {
  CHAR_BACKSLASH, CHAR_p, CHAR_LEFT_CURLY_BRACKET,
  CHAR_L, CHAR_u, CHAR_RIGHT_CURLY_BRACKET, '\0' };
static const pcre_uchar string_pXan[] = {
  CHAR_BACKSLASH, CHAR_p, CHAR_LEFT_CURLY_BRACKET,
  CHAR_X, CHAR_a, CHAR_n, CHAR_RIGHT_CURLY_BRACKET, '\0' };
static const pcre_uchar string_h[] =    {
  CHAR_BACKSLASH, CHAR_h, '\0' };
static const pcre_uchar string_pXps[] = {
  CHAR_BACKSLASH, CHAR_p, CHAR_LEFT_CURLY_BRACKET,
  CHAR_X, CHAR_p, CHAR_s, CHAR_RIGHT_CURLY_BRACKET, '\0' };
static const pcre_uchar string_PL[] =   {
  CHAR_BACKSLASH, CHAR_P, CHAR_LEFT_CURLY_BRACKET,
  CHAR_L, CHAR_RIGHT_CURLY_BRACKET, '\0' };
static const pcre_uchar string_PLl[] =  {
  CHAR_BACKSLASH, CHAR_P, CHAR_LEFT_CURLY_BRACKET,
  CHAR_L, CHAR_l, CHAR_RIGHT_CURLY_BRACKET, '\0' };
static const pcre_uchar string_PLu[] =  {
  CHAR_BACKSLASH, CHAR_P, CHAR_LEFT_CURLY_BRACKET,
  CHAR_L, CHAR_u, CHAR_RIGHT_CURLY_BRACKET, '\0' };
static const pcre_uchar string_PXan[] = {
  CHAR_BACKSLASH, CHAR_P, CHAR_LEFT_CURLY_BRACKET,
  CHAR_X, CHAR_a, CHAR_n, CHAR_RIGHT_CURLY_BRACKET, '\0' };
static const pcre_uchar string_H[] =    {
  CHAR_BACKSLASH, CHAR_H, '\0' };
static const pcre_uchar string_PXps[] = {
  CHAR_BACKSLASH, CHAR_P, CHAR_LEFT_CURLY_BRACKET,
  CHAR_X, CHAR_p, CHAR_s, CHAR_RIGHT_CURLY_BRACKET, '\0' };

static const pcre_uchar *posix_substitutes[] = {
  string_pL,            /* alpha */
  string_pLl,           /* lower */
  string_pLu,           /* upper */
  string_pXan,          /* alnum */
  NULL,                 /* ascii */
  string_h,             /* blank */
  NULL,                 /* cntrl */
  string_pNd,           /* digit */
  NULL,                 /* graph */
  NULL,                 /* print */
  NULL,                 /* punct */
  string_pXps,          /* space */   /* Xps is POSIX space, but from 8.34 */
  string_pXwd,          /* word  */   /* Perl and POSIX space are the same */
  NULL,                 /* xdigit */
  /* Negated cases */
  string_PL,            /* ^alpha */
  string_PLl,           /* ^lower */
  string_PLu,           /* ^upper */
  string_PXan,          /* ^alnum */
  NULL,                 /* ^ascii */
  string_H,             /* ^blank */
  NULL,                 /* ^cntrl */
  string_PNd,           /* ^digit */
  NULL,                 /* ^graph */
  NULL,                 /* ^print */
  NULL,                 /* ^punct */
  string_PXps,          /* ^space */  /* Xps is POSIX space, but from 8.34 */
  string_PXwd,          /* ^word */   /* Perl and POSIX space are the same */
  NULL                  /* ^xdigit */
};
#define POSIX_SUBSIZE (sizeof(posix_substitutes) / sizeof(pcre_uchar *))
#endif

#define STRING(a)  # a
#define XSTRING(s) STRING(s)

/* The texts of compile-time error messages. These are "char *" because they
are passed to the outside world. Do not ever re-use any error number, because
they are documented. Always add a new error instead. Messages marked DEAD below
are no longer used. This used to be a table of strings, but in order to reduce
the number of relocations needed when a shared library is loaded dynamically,
it is now one long string. We cannot use a table of offsets, because the
lengths of inserts such as XSTRING(MAX_NAME_SIZE) are not known. Instead, we
simply count through to the one we want - this isn't a performance issue
because these strings are used only when there is a compilation error.

Each substring ends with \0 to insert a null character. This includes the final
substring, so that the whole string ends with \0\0, which can be detected when
counting through. */

static const char error_texts[] =
  "no error\0"
  "\\ at end of pattern\0"
  "\\c at end of pattern\0"
  "unrecognized character follows \\\0"
  "numbers out of order in {} quantifier\0"
  /* 5 */
  "number too big in {} quantifier\0"
  "missing terminating ] for character class\0"
  "invalid escape sequence in character class\0"
  "range out of order in character class\0"
  "nothing to repeat\0"
  /* 10 */
  "internal error: invalid forward reference offset\0"
  "internal error: unexpected repeat\0"
  "unrecognized character after (? or (?-\0"
  "POSIX named classes are supported only within a class\0"
  "missing )\0"
  /* 15 */
  "reference to non-existent subpattern\0"
  "erroffset passed as NULL\0"
  "unknown option bit(s) set\0"
  "missing ) after comment\0"
  "parentheses nested too deeply\0"  /** DEAD **/
  /* 20 */
  "regular expression is too large\0"
  "failed to get memory\0"
  "unmatched parentheses\0"
  "internal error: code overflow\0"
  "unrecognized character after (?<\0"
  /* 25 */
  "lookbehind assertion is not fixed length\0"
  "malformed number or name after (?(\0"
  "conditional group contains more than two branches\0"
  "assertion expected after (?(\0"
  "(?R or (?[+-]digits must be followed by )\0"
  /* 30 */
  "unknown POSIX class name\0"
  "POSIX collating elements are not supported\0"
  "this version of PCRE is compiled without UTF support\0"
  "spare error\0"  /** DEAD **/
  "character value in \\x{} or \\o{} is too large\0"
  /* 35 */
  "invalid condition (?(0)\0"
  "\\C not allowed in lookbehind assertion\0"
  "PCRE does not support \\L, \\l, \\N{name}, \\U, or \\u\0"
  "number after (?C is > 255\0"
  "closing ) for (?C expected\0"
  /* 40 */
  "recursive call could loop indefinitely\0"
  "unrecognized character after (?P\0"
  "syntax error in subpattern name (missing terminator)\0"
  "two named subpatterns have the same name\0"
  "invalid UTF-8 string\0"
  /* 45 */
  "support for \\P, \\p, and \\X has not been compiled\0"
  "malformed \\P or \\p sequence\0"
  "unknown property name after \\P or \\p\0"
  "subpattern name is too long (maximum " XSTRING(MAX_NAME_SIZE) " characters)\0"
  "too many named subpatterns (maximum " XSTRING(MAX_NAME_COUNT) ")\0"
  /* 50 */
  "repeated subpattern is too long\0"    /** DEAD **/
  "octal value is greater than \\377 in 8-bit non-UTF-8 mode\0"
  "internal error: overran compiling workspace\0"
  "internal error: previously-checked referenced subpattern not found\0"
  "DEFINE group contains more than one branch\0"
  /* 55 */
  "repeating a DEFINE group is not allowed\0"  /** DEAD **/
  "inconsistent NEWLINE options\0"
  "\\g is not followed by a braced, angle-bracketed, or quoted name/number or by a plain number\0"
  "a numbered reference must not be zero\0"
  "an argument is not allowed for (*ACCEPT), (*FAIL), or (*COMMIT)\0"
  /* 60 */
  "(*VERB) not recognized or malformed\0"
  "number is too big\0"
  "subpattern name expected\0"
  "digit expected after (?+\0"
  "] is an invalid data character in JavaScript compatibility mode\0"
  /* 65 */
  "different names for subpatterns of the same number are not allowed\0"
  "(*MARK) must have an argument\0"
  "this version of PCRE is not compiled with Unicode property support\0"
#ifndef EBCDIC
  "\\c must be followed by an ASCII character\0"
#else
  "\\c must be followed by a letter or one of [\\]^_?\0"
#endif
  "\\k is not followed by a braced, angle-bracketed, or quoted name\0"
  /* 70 */
  "internal error: unknown opcode in find_fixedlength()\0"
  "\\N is not supported in a class\0"
  "too many forward references\0"
  "disallowed Unicode code point (>= 0xd800 && <= 0xdfff)\0"
  "invalid UTF-16 string\0"
  /* 75 */
  "name is too long in (*MARK), (*PRUNE), (*SKIP), or (*THEN)\0"
  "character value in \\u.... sequence is too large\0"
  "invalid UTF-32 string\0"
  "setting UTF is disabled by the application\0"
  "non-hex character in \\x{} (closing brace missing?)\0"
  /* 80 */
  "non-octal character in \\o{} (closing brace missing?)\0"
  "missing opening brace after \\o\0"
  "parentheses are too deeply nested\0"
  "invalid range in character class\0"
  "group name must start with a non-digit\0"
  /* 85 */
  "parentheses are too deeply nested (stack check)\0"
  "digits missing in \\x{} or \\o{}\0"
  ;

/* Table to identify digits and hex digits. This is used when compiling
patterns. Note that the tables in chartables are dependent on the locale, and
may mark arbitrary characters as digits - but the PCRE compiling code expects
to handle only 0-9, a-z, and A-Z as digits when compiling. That is why we have
a private table here. It costs 256 bytes, but it is a lot faster than doing
character value tests (at least in some simple cases I timed), and in some
applications one wants PCRE to compile efficiently as well as match
efficiently.

For convenience, we use the same bit definitions as in chartables:

  0x04   decimal digit
  0x08   hexadecimal digit

Then we can use ctype_digit and ctype_xdigit in the code. */

/* Using a simple comparison for decimal numbers rather than a memory read
is much faster, and the resulting code is simpler (the compiler turns it
into a subtraction and unsigned comparison). */

#define IS_DIGIT(x) ((x) >= CHAR_0 && (x) <= CHAR_9)

#ifndef EBCDIC

/* This is the "normal" case, for ASCII systems, and EBCDIC systems running in
UTF-8 mode. */

static const pcre_uint8 digitab[] =
  {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*   0-  7 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*   8- 15 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  16- 23 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  24- 31 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*    - '  */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  ( - /  */
  0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c, /*  0 - 7  */
  0x0c,0x0c,0x00,0x00,0x00,0x00,0x00,0x00, /*  8 - ?  */
  0x00,0x08,0x08,0x08,0x08,0x08,0x08,0x00, /*  @ - G  */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  H - O  */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  P - W  */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  X - _  */
  0x00,0x08,0x08,0x08,0x08,0x08,0x08,0x00, /*  ` - g  */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  h - o  */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  p - w  */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  x -127 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 128-135 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 136-143 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 144-151 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 152-159 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 160-167 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 168-175 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 176-183 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 184-191 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 192-199 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 200-207 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 208-215 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 216-223 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 224-231 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 232-239 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 240-247 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};/* 248-255 */

#else

/* This is the "abnormal" case, for EBCDIC systems not running in UTF-8 mode. */

static const pcre_uint8 digitab[] =
  {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*   0-  7  0 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*   8- 15    */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  16- 23 10 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  24- 31    */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  32- 39 20 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  40- 47    */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  48- 55 30 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  56- 63    */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*    - 71 40 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  72- |     */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  & - 87 50 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  88- 95    */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  - -103 60 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 104- ?     */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 112-119 70 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 120- "     */
  0x00,0x08,0x08,0x08,0x08,0x08,0x08,0x00, /* 128- g  80 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  h -143    */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 144- p  90 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  q -159    */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 160- x  A0 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  y -175    */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  ^ -183 B0 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 184-191    */
  0x00,0x08,0x08,0x08,0x08,0x08,0x08,0x00, /*  { - G  C0 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  H -207    */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  } - P  D0 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  Q -223    */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  \ - X  E0 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  Y -239    */
  0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c, /*  0 - 7  F0 */
  0x0c,0x0c,0x00,0x00,0x00,0x00,0x00,0x00};/*  8 -255    */

static const pcre_uint8 ebcdic_chartab[] = { /* chartable partial dup */
  0x80,0x00,0x00,0x00,0x00,0x01,0x00,0x00, /*   0-  7 */
  0x00,0x00,0x00,0x00,0x01,0x01,0x00,0x00, /*   8- 15 */
  0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00, /*  16- 23 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  24- 31 */
  0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00, /*  32- 39 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  40- 47 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  48- 55 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  56- 63 */
  0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*    - 71 */
  0x00,0x00,0x00,0x80,0x00,0x80,0x80,0x80, /*  72- |  */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  & - 87 */
  0x00,0x00,0x00,0x80,0x80,0x80,0x00,0x00, /*  88- 95 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  - -103 */
  0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x80, /* 104- ?  */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 112-119 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 120- "  */
  0x00,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x12, /* 128- g  */
  0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00, /*  h -143 */
  0x00,0x12,0x12,0x12,0x12,0x12,0x12,0x12, /* 144- p  */
  0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00, /*  q -159 */
  0x00,0x00,0x12,0x12,0x12,0x12,0x12,0x12, /* 160- x  */
  0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00, /*  y -175 */
  0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  ^ -183 */
  0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00, /* 184-191 */
  0x80,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x12, /*  { - G  */
  0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00, /*  H -207 */
  0x00,0x12,0x12,0x12,0x12,0x12,0x12,0x12, /*  } - P  */
  0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00, /*  Q -223 */
  0x00,0x00,0x12,0x12,0x12,0x12,0x12,0x12, /*  \ - X  */
  0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00, /*  Y -239 */
  0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c, /*  0 - 7  */
  0x1c,0x1c,0x00,0x00,0x00,0x00,0x00,0x00};/*  8 -255 */
#endif


/* This table is used to check whether auto-possessification is possible
between adjacent character-type opcodes. The left-hand (repeated) opcode is
used to select the row, and the right-hand opcode is use to select the column.
A value of 1 means that auto-possessification is OK. For example, the second
value in the first row means that \D+\d can be turned into \D++\d.

The Unicode property types (\P and \p) have to be present to fill out the table
because of what their opcode values are, but the table values should always be
zero because property types are handled separately in the code. The last four
columns apply to items that cannot be repeated, so there is no need to have
rows for them. Note that OP_DIGIT etc. are generated only when PCRE_UCP is
*not* set. When it is set, \d etc. are converted into OP_(NOT_)PROP codes. */

#define APTROWS (LAST_AUTOTAB_LEFT_OP - FIRST_AUTOTAB_OP + 1)
#define APTCOLS (LAST_AUTOTAB_RIGHT_OP - FIRST_AUTOTAB_OP + 1)

static const pcre_uint8 autoposstab[APTROWS][APTCOLS] = {
/* \D \d \S \s \W \w  . .+ \C \P \p \R \H \h \V \v \X \Z \z  $ $M */
  { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },  /* \D */
  { 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1 },  /* \d */
  { 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1 },  /* \S */
  { 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },  /* \s */
  { 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },  /* \W */
  { 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1 },  /* \w */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0 },  /* .  */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },  /* .+ */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },  /* \C */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  /* \P */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  /* \p */
  { 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0 },  /* \R */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0 },  /* \H */
  { 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0 },  /* \h */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0 },  /* \V */
  { 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0 },  /* \v */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 }   /* \X */
};


/* This table is used to check whether auto-possessification is possible
between adjacent Unicode property opcodes (OP_PROP and OP_NOTPROP). The
left-hand (repeated) opcode is used to select the row, and the right-hand
opcode is used to select the column. The values are as follows:

  0   Always return FALSE (never auto-possessify)
  1   Character groups are distinct (possessify if both are OP_PROP)
  2   Check character categories in the same group (general or particular)
  3   TRUE if the two opcodes are not the same (PROP vs NOTPROP)

  4   Check left general category vs right particular category
  5   Check right general category vs left particular category

  6   Left alphanum vs right general category
  7   Left space vs right general category
  8   Left word vs right general category

  9   Right alphanum vs left general category
 10   Right space vs left general category
 11   Right word vs left general category

 12   Left alphanum vs right particular category
 13   Left space vs right particular category
 14   Left word vs right particular category

 15   Right alphanum vs left particular category
 16   Right space vs left particular category
 17   Right word vs left particular category
*/

static const pcre_uint8 propposstab[PT_TABSIZE][PT_TABSIZE] = {
/* ANY LAMP GC  PC  SC ALNUM SPACE PXSPACE WORD CLIST UCNC */
  { 0,  0,  0,  0,  0,    0,    0,      0,   0,    0,   0 },  /* PT_ANY */
  { 0,  3,  0,  0,  0,    3,    1,      1,   0,    0,   0 },  /* PT_LAMP */
  { 0,  0,  2,  4,  0,    9,   10,     10,  11,    0,   0 },  /* PT_GC */
  { 0,  0,  5,  2,  0,   15,   16,     16,  17,    0,   0 },  /* PT_PC */
  { 0,  0,  0,  0,  2,    0,    0,      0,   0,    0,   0 },  /* PT_SC */
  { 0,  3,  6, 12,  0,    3,    1,      1,   0,    0,   0 },  /* PT_ALNUM */
  { 0,  1,  7, 13,  0,    1,    3,      3,   1,    0,   0 },  /* PT_SPACE */
  { 0,  1,  7, 13,  0,    1,    3,      3,   1,    0,   0 },  /* PT_PXSPACE */
  { 0,  0,  8, 14,  0,    0,    1,      1,   3,    0,   0 },  /* PT_WORD */
  { 0,  0,  0,  0,  0,    0,    0,      0,   0,    0,   0 },  /* PT_CLIST */
  { 0,  0,  0,  0,  0,    0,    0,      0,   0,    0,   3 }   /* PT_UCNC */
};

/* This table is used to check whether auto-possessification is possible
between adjacent Unicode property opcodes (OP_PROP and OP_NOTPROP) when one
specifies a general category and the other specifies a particular category. The
row is selected by the general category and the column by the particular
category. The value is 1 if the particular category is not part of the general
category. */

static const pcre_uint8 catposstab[7][30] = {
/* Cc Cf Cn Co Cs Ll Lm Lo Lt Lu Mc Me Mn Nd Nl No Pc Pd Pe Pf Pi Po Ps Sc Sk Sm So Zl Zp Zs */
  { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },  /* C */
  { 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },  /* L */
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },  /* M */
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },  /* N */
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1 },  /* P */
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1 },  /* S */
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 }   /* Z */
};

/* This table is used when checking ALNUM, (PX)SPACE, SPACE, and WORD against
a general or particular category. The properties in each row are those
that apply to the character set in question. Duplication means that a little
unnecessary work is done when checking, but this keeps things much simpler
because they can all use the same code. For more details see the comment where
this table is used.

Note: SPACE and PXSPACE used to be different because Perl excluded VT from
"space", but from Perl 5.18 it's included, so both categories are treated the
same here. */

static const pcre_uint8 posspropstab[3][4] = {
  { ucp_L, ucp_N, ucp_N, ucp_Nl },  /* ALNUM, 3rd and 4th values redundant */
  { ucp_Z, ucp_Z, ucp_C, ucp_Cc },  /* SPACE and PXSPACE, 2nd value redundant */
  { ucp_L, ucp_N, ucp_P, ucp_Po }   /* WORD */
};

/* This table is used when converting repeating opcodes into possessified
versions as a result of an explicit possessive quantifier such as ++. A zero
value means there is no possessified version - in those cases the item in
question must be wrapped in ONCE brackets. The table is truncated at OP_CALLOUT
because all relevant opcodes are less than that. */

static const pcre_uint8 opcode_possessify[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* 0 - 15  */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* 16 - 31 */

  0,                       /* NOTI */
  OP_POSSTAR, 0,           /* STAR, MINSTAR */
  OP_POSPLUS, 0,           /* PLUS, MINPLUS */
  OP_POSQUERY, 0,          /* QUERY, MINQUERY */
  OP_POSUPTO, 0,           /* UPTO, MINUPTO */
  0,                       /* EXACT */
  0, 0, 0, 0,              /* POS{STAR,PLUS,QUERY,UPTO} */

  OP_POSSTARI, 0,          /* STARI, MINSTARI */
  OP_POSPLUSI, 0,          /* PLUSI, MINPLUSI */
  OP_POSQUERYI, 0,         /* QUERYI, MINQUERYI */
  OP_POSUPTOI, 0,          /* UPTOI, MINUPTOI */
  0,                       /* EXACTI */
  0, 0, 0, 0,              /* POS{STARI,PLUSI,QUERYI,UPTOI} */

  OP_NOTPOSSTAR, 0,        /* NOTSTAR, NOTMINSTAR */
  OP_NOTPOSPLUS, 0,        /* NOTPLUS, NOTMINPLUS */
  OP_NOTPOSQUERY, 0,       /* NOTQUERY, NOTMINQUERY */
  OP_NOTPOSUPTO, 0,        /* NOTUPTO, NOTMINUPTO */
  0,                       /* NOTEXACT */
  0, 0, 0, 0,              /* NOTPOS{STAR,PLUS,QUERY,UPTO} */

  OP_NOTPOSSTARI, 0,       /* NOTSTARI, NOTMINSTARI */
  OP_NOTPOSPLUSI, 0,       /* NOTPLUSI, NOTMINPLUSI */
  OP_NOTPOSQUERYI, 0,      /* NOTQUERYI, NOTMINQUERYI */
  OP_NOTPOSUPTOI, 0,       /* NOTUPTOI, NOTMINUPTOI */
  0,                       /* NOTEXACTI */
  0, 0, 0, 0,              /* NOTPOS{STARI,PLUSI,QUERYI,UPTOI} */

  OP_TYPEPOSSTAR, 0,       /* TYPESTAR, TYPEMINSTAR */
  OP_TYPEPOSPLUS, 0,       /* TYPEPLUS, TYPEMINPLUS */
  OP_TYPEPOSQUERY, 0,      /* TYPEQUERY, TYPEMINQUERY */
  OP_TYPEPOSUPTO, 0,       /* TYPEUPTO, TYPEMINUPTO */
  0,                       /* TYPEEXACT */
  0, 0, 0, 0,              /* TYPEPOS{STAR,PLUS,QUERY,UPTO} */

  OP_CRPOSSTAR, 0,         /* CRSTAR, CRMINSTAR */
  OP_CRPOSPLUS, 0,         /* CRPLUS, CRMINPLUS */
  OP_CRPOSQUERY, 0,        /* CRQUERY, CRMINQUERY */
  OP_CRPOSRANGE, 0,        /* CRRANGE, CRMINRANGE */
  0, 0, 0, 0,              /* CRPOS{STAR,PLUS,QUERY,RANGE} */

  0, 0, 0,                 /* CLASS, NCLASS, XCLASS */
  0, 0,                    /* REF, REFI */
  0, 0,                    /* DNREF, DNREFI */
  0, 0                     /* RECURSE, CALLOUT */
};



/*************************************************
*            Find an error text                  *
*************************************************/

/* The error texts are now all in one long string, to save on relocations. As
some of the text is of unknown length, we can't use a table of offsets.
Instead, just count through the strings. This is not a performance issue
because it happens only when there has been a compilation error.

Argument:   the error number
Returns:    pointer to the error string
*/

static const char *
find_error_text(int n)
{
const char *s = error_texts;
for (; n > 0; n--)
  {
  while (*s++ != CHAR_NULL) {};
  if (*s == CHAR_NULL) return "Error text not found (please report)";
  }
return s;
}



/*************************************************
*           Expand the workspace                 *
*************************************************/

/* This function is called during the second compiling phase, if the number of
forward references fills the existing workspace, which is originally a block on
the stack. A larger block is obtained from malloc() unless the ultimate limit
has been reached or the increase will be rather small.

Argument: pointer to the compile data block
Returns:  0 if all went well, else an error number
*/

static int
expand_workspace(compile_data *cd)
{
pcre_uchar *newspace;
int newsize = cd->workspace_size * 2;

if (newsize > COMPILE_WORK_SIZE_MAX) newsize = COMPILE_WORK_SIZE_MAX;
if (cd->workspace_size >= COMPILE_WORK_SIZE_MAX ||
    newsize - cd->workspace_size < WORK_SIZE_SAFETY_MARGIN)
 return ERR72;

newspace = (PUBL(malloc))(IN_UCHARS(newsize));
if (newspace == NULL) return ERR21;
memcpy(newspace, cd->start_workspace, cd->workspace_size * sizeof(pcre_uchar));
cd->hwm = (pcre_uchar *)newspace + (cd->hwm - cd->start_workspace);
if (cd->workspace_size > COMPILE_WORK_SIZE)
  (PUBL(free))((void *)cd->start_workspace);
cd->start_workspace = newspace;
cd->workspace_size = newsize;
return 0;
}



/*************************************************
*            Check for counted repeat            *
*************************************************/

/* This function is called when a '{' is encountered in a place where it might
start a quantifier. It looks ahead to see if it really is a quantifier or not.
It is only a quantifier if it is one of the forms {ddd} {ddd,} or {ddd,ddd}
where the ddds are digits.

Arguments:
  p         pointer to the first char after '{'

Returns:    TRUE or FALSE
*/

static BOOL
is_counted_repeat(const pcre_uchar *p)
{
if (!IS_DIGIT(*p)) return FALSE;
p++;
while (IS_DIGIT(*p)) p++;
if (*p == CHAR_RIGHT_CURLY_BRACKET) return TRUE;

if (*p++ != CHAR_COMMA) return FALSE;
if (*p == CHAR_RIGHT_CURLY_BRACKET) return TRUE;

if (!IS_DIGIT(*p)) return FALSE;
p++;
while (IS_DIGIT(*p)) p++;

return (*p == CHAR_RIGHT_CURLY_BRACKET);
}



/*************************************************
*            Handle escapes                      *
*************************************************/

/* This function is called when a \ has been encountered. It either returns a
positive value for a simple escape such as \n, or 0 for a data character which
will be placed in chptr. A backreference to group n is returned as negative n.
When UTF-8 is enabled, a positive value greater than 255 may be returned in
chptr. On entry, ptr is pointing at the \. On exit, it is on the final
character of the escape sequence.

Arguments:
  ptrptr         points to the pattern position pointer
  chptr          points to a returned data character
  errorcodeptr   points to the errorcode variable
  bracount       number of previous extracting brackets
  options        the options bits
  isclass        TRUE if inside a character class

Returns:         zero => a data character
                 positive => a special escape sequence
                 negative => a back reference
                 on error, errorcodeptr is set
*/

static int
check_escape(const pcre_uchar **ptrptr, pcre_uint32 *chptr, int *errorcodeptr,
  int bracount, int options, BOOL isclass)
{
/* PCRE_UTF16 has the same value as PCRE_UTF8. */
BOOL utf = (options & PCRE_UTF8) != 0;
const pcre_uchar *ptr = *ptrptr + 1;
pcre_uint32 c;
int escape = 0;
int i;

GETCHARINCTEST(c, ptr);           /* Get character value, increment pointer */
ptr--;                            /* Set pointer back to the last byte */

/* If backslash is at the end of the pattern, it's an error. */

if (c == CHAR_NULL) *errorcodeptr = ERR1;

/* Non-alphanumerics are literals. For digits or letters, do an initial lookup
in a table. A non-zero result is something that can be returned immediately.
Otherwise further processing may be required. */

#ifndef EBCDIC  /* ASCII/UTF-8 coding */
/* Not alphanumeric */
else if (c < CHAR_0 || c > CHAR_z) {}
else if ((i = escapes[c - CHAR_0]) != 0)
  { if (i > 0) c = (pcre_uint32)i; else escape = -i; }

#else           /* EBCDIC coding */
/* Not alphanumeric */
else if (c < CHAR_a || (!MAX_255(c) || (ebcdic_chartab[c] & 0x0E) == 0)) {}
else if ((i = escapes[c - 0x48]) != 0)  { if (i > 0) c = (pcre_uint32)i; else escape = -i; }
#endif

/* Escapes that need further processing, or are illegal. */

else
  {
  const pcre_uchar *oldptr;
  BOOL braced, negated, overflow;
  int s;

  switch (c)
    {
    /* A number of Perl escapes are not handled by PCRE. We give an explicit
    error. */

    case CHAR_l:
    case CHAR_L:
    *errorcodeptr = ERR37;
    break;

    case CHAR_u:
    if ((options & PCRE_JAVASCRIPT_COMPAT) != 0)
      {
      /* In JavaScript, \u must be followed by four hexadecimal numbers.
      Otherwise it is a lowercase u letter. */
      if (MAX_255(ptr[1]) && (digitab[ptr[1]] & ctype_xdigit) != 0
        && MAX_255(ptr[2]) && (digitab[ptr[2]] & ctype_xdigit) != 0
        && MAX_255(ptr[3]) && (digitab[ptr[3]] & ctype_xdigit) != 0
        && MAX_255(ptr[4]) && (digitab[ptr[4]] & ctype_xdigit) != 0)
        {
        c = 0;
        for (i = 0; i < 4; ++i)
          {
          register pcre_uint32 cc = *(++ptr);
#ifndef EBCDIC  /* ASCII/UTF-8 coding */
          if (cc >= CHAR_a) cc -= 32;               /* Convert to upper case */
          c = (c << 4) + cc - ((cc < CHAR_A)? CHAR_0 : (CHAR_A - 10));
#else           /* EBCDIC coding */
          if (cc >= CHAR_a && cc <= CHAR_z) cc += 64;  /* Convert to upper case */
          c = (c << 4) + cc - ((cc >= CHAR_0)? CHAR_0 : (CHAR_A - 10));
#endif
          }

#if defined COMPILE_PCRE8
        if (c > (utf ? 0x10ffffU : 0xffU))
#elif defined COMPILE_PCRE16
        if (c > (utf ? 0x10ffffU : 0xffffU))
#elif defined COMPILE_PCRE32
        if (utf && c > 0x10ffffU)
#endif
          {
          *errorcodeptr = ERR76;
          }
        else if (utf && c >= 0xd800 && c <= 0xdfff) *errorcodeptr = ERR73;
        }
      }
    else
      *errorcodeptr = ERR37;
    break;

    case CHAR_U:
    /* In JavaScript, \U is an uppercase U letter. */
    if ((options & PCRE_JAVASCRIPT_COMPAT) == 0) *errorcodeptr = ERR37;
    break;

    /* In a character class, \g is just a literal "g". Outside a character
    class, \g must be followed by one of a number of specific things:

    (1) A number, either plain or braced. If positive, it is an absolute
    backreference. If negative, it is a relative backreference. This is a Perl
    5.10 feature.

    (2) Perl 5.10 also supports \g{name} as a reference to a named group. This
    is part of Perl's movement towards a unified syntax for back references. As
    this is synonymous with \k{name}, we fudge it up by pretending it really
    was \k.

    (3) For Oniguruma compatibility we also support \g followed by a name or a
    number either in angle brackets or in single quotes. However, these are
    (possibly recursive) subroutine calls, _not_ backreferences. Just return
    the ESC_g code (cf \k). */

    case CHAR_g:
    if (isclass) break;
    if (ptr[1] == CHAR_LESS_THAN_SIGN || ptr[1] == CHAR_APOSTROPHE)
      {
      escape = ESC_g;
      break;
      }

    /* Handle the Perl-compatible cases */

    if (ptr[1] == CHAR_LEFT_CURLY_BRACKET)
      {
      const pcre_uchar *p;
      for (p = ptr+2; *p != CHAR_NULL && *p != CHAR_RIGHT_CURLY_BRACKET; p++)
        if (*p != CHAR_MINUS && !IS_DIGIT(*p)) break;
      if (*p != CHAR_NULL && *p != CHAR_RIGHT_CURLY_BRACKET)
        {
        escape = ESC_k;
        break;
        }
      braced = TRUE;
      ptr++;
      }
    else braced = FALSE;

    if (ptr[1] == CHAR_MINUS)
      {
      negated = TRUE;
      ptr++;
      }
    else negated = FALSE;

    /* The integer range is limited by the machine's int representation. */
    s = 0;
    overflow = FALSE;
    while (IS_DIGIT(ptr[1]))
      {
      if (s > INT_MAX / 10 - 1) /* Integer overflow */
        {
        overflow = TRUE;
        break;
        }
      s = s * 10 + (int)(*(++ptr) - CHAR_0);
      }
    if (overflow) /* Integer overflow */
      {
      while (IS_DIGIT(ptr[1]))
        ptr++;
      *errorcodeptr = ERR61;
      break;
      }

    if (braced && *(++ptr) != CHAR_RIGHT_CURLY_BRACKET)
      {
      *errorcodeptr = ERR57;
      break;
      }

    if (s == 0)
      {
      *errorcodeptr = ERR58;
      break;
      }

    if (negated)
      {
      if (s > bracount)
        {
        *errorcodeptr = ERR15;
        break;
        }
      s = bracount - (s - 1);
      }

    escape = -s;
    break;

    /* The handling of escape sequences consisting of a string of digits
    starting with one that is not zero is not straightforward. Perl has changed
    over the years. Nowadays \g{} for backreferences and \o{} for octal are
    recommended to avoid the ambiguities in the old syntax.

    Outside a character class, the digits are read as a decimal number. If the
    number is less than 8 (used to be 10), or if there are that many previous
    extracting left brackets, then it is a back reference. Otherwise, up to
    three octal digits are read to form an escaped byte. Thus \123 is likely to
    be octal 123 (cf \0123, which is octal 012 followed by the literal 3). If
    the octal value is greater than 377, the least significant 8 bits are
    taken. \8 and \9 are treated as the literal characters 8 and 9.

    Inside a character class, \ followed by a digit is always either a literal
    8 or 9 or an octal number. */

    case CHAR_1: case CHAR_2: case CHAR_3: case CHAR_4: case CHAR_5:
    case CHAR_6: case CHAR_7: case CHAR_8: case CHAR_9:

    if (!isclass)
      {
      oldptr = ptr;
      /* The integer range is limited by the machine's int representation. */
      s = (int)(c -CHAR_0);
      overflow = FALSE;
      while (IS_DIGIT(ptr[1]))
        {
        if (s > INT_MAX / 10 - 1) /* Integer overflow */
          {
          overflow = TRUE;
          break;
          }
        s = s * 10 + (int)(*(++ptr) - CHAR_0);
        }
      if (overflow) /* Integer overflow */
        {
        while (IS_DIGIT(ptr[1]))
          ptr++;
        *errorcodeptr = ERR61;
        break;
        }
      if (s < 8 || s <= bracount)  /* Check for back reference */
        {
        escape = -s;
        break;
        }
      ptr = oldptr;      /* Put the pointer back and fall through */
      }

    /* Handle a digit following \ when the number is not a back reference. If
    the first digit is 8 or 9, Perl used to generate a binary zero byte and
    then treat the digit as a following literal. At least by Perl 5.18 this
    changed so as not to insert the binary zero. */

    if ((c = *ptr) >= CHAR_8) break;

    /* Fall through with a digit less than 8 */

    /* \0 always starts an octal number, but we may drop through to here with a
    larger first octal digit. The original code used just to take the least
    significant 8 bits of octal numbers (I think this is what early Perls used
    to do). Nowadays we allow for larger numbers in UTF-8 mode and 16-bit mode,
    but no more than 3 octal digits. */

    case CHAR_0:
    c -= CHAR_0;
    while(i++ < 2 && ptr[1] >= CHAR_0 && ptr[1] <= CHAR_7)
        c = c * 8 + *(++ptr) - CHAR_0;
#ifdef COMPILE_PCRE8
    if (!utf && c > 0xff) *errorcodeptr = ERR51;
#endif
    break;

    /* \o is a relatively new Perl feature, supporting a more general way of
    specifying character codes in octal. The only supported form is \o{ddd}. */

    case CHAR_o:
    if (ptr[1] != CHAR_LEFT_CURLY_BRACKET) *errorcodeptr = ERR81; else
    if (ptr[2] == CHAR_RIGHT_CURLY_BRACKET) *errorcodeptr = ERR86; else
      {
      ptr += 2;
      c = 0;
      overflow = FALSE;
      while (*ptr >= CHAR_0 && *ptr <= CHAR_7)
        {
        register pcre_uint32 cc = *ptr++;
        if (c == 0 && cc == CHAR_0) continue;     /* Leading zeroes */
#ifdef COMPILE_PCRE32
        if (c >= 0x20000000l) { overflow = TRUE; break; }
#endif
        c = (c << 3) + cc - CHAR_0 ;
#if defined COMPILE_PCRE8
        if (c > (utf ? 0x10ffffU : 0xffU)) { overflow = TRUE; break; }
#elif defined COMPILE_PCRE16
        if (c > (utf ? 0x10ffffU : 0xffffU)) { overflow = TRUE; break; }
#elif defined COMPILE_PCRE32
        if (utf && c > 0x10ffffU) { overflow = TRUE; break; }
#endif
        }
      if (overflow)
        {
        while (*ptr >= CHAR_0 && *ptr <= CHAR_7) ptr++;
        *errorcodeptr = ERR34;
        }
      else if (*ptr == CHAR_RIGHT_CURLY_BRACKET)
        {
        if (utf && c >= 0xd800 && c <= 0xdfff) *errorcodeptr = ERR73;
        }
      else *errorcodeptr = ERR80;
      }
    break;

    /* \x is complicated. In JavaScript, \x must be followed by two hexadecimal
    numbers. Otherwise it is a lowercase x letter. */

    case CHAR_x:
    if ((options & PCRE_JAVASCRIPT_COMPAT) != 0)
      {
      if (MAX_255(ptr[1]) && (digitab[ptr[1]] & ctype_xdigit) != 0
        && MAX_255(ptr[2]) && (digitab[ptr[2]] & ctype_xdigit) != 0)
        {
        c = 0;
        for (i = 0; i < 2; ++i)
          {
          register pcre_uint32 cc = *(++ptr);
#ifndef EBCDIC  /* ASCII/UTF-8 coding */
          if (cc >= CHAR_a) cc -= 32;               /* Convert to upper case */
          c = (c << 4) + cc - ((cc < CHAR_A)? CHAR_0 : (CHAR_A - 10));
#else           /* EBCDIC coding */
          if (cc >= CHAR_a && cc <= CHAR_z) cc += 64;  /* Convert to upper case */
          c = (c << 4) + cc - ((cc >= CHAR_0)? CHAR_0 : (CHAR_A - 10));
#endif
          }
        }
      }    /* End JavaScript handling */

    /* Handle \x in Perl's style. \x{ddd} is a character number which can be
    greater than 0xff in utf or non-8bit mode, but only if the ddd are hex
    digits. If not, { used to be treated as a data character. However, Perl
    seems to read hex digits up to the first non-such, and ignore the rest, so
    that, for example \x{zz} matches a binary zero. This seems crazy, so PCRE
    now gives an error. */

    else
      {
      if (ptr[1] == CHAR_LEFT_CURLY_BRACKET)
        {
        ptr += 2;
        if (*ptr == CHAR_RIGHT_CURLY_BRACKET)
          {
          *errorcodeptr = ERR86;
          break;
          }
        c = 0;
        overflow = FALSE;
        while (MAX_255(*ptr) && (digitab[*ptr] & ctype_xdigit) != 0)
          {
          register pcre_uint32 cc = *ptr++;
          if (c == 0 && cc == CHAR_0) continue;     /* Leading zeroes */

#ifdef COMPILE_PCRE32
          if (c >= 0x10000000l) { overflow = TRUE; break; }
#endif

#ifndef EBCDIC  /* ASCII/UTF-8 coding */
          if (cc >= CHAR_a) cc -= 32;               /* Convert to upper case */
          c = (c << 4) + cc - ((cc < CHAR_A)? CHAR_0 : (CHAR_A - 10));
#else           /* EBCDIC coding */
          if (cc >= CHAR_a && cc <= CHAR_z) cc += 64;  /* Convert to upper case */
          c = (c << 4) + cc - ((cc >= CHAR_0)? CHAR_0 : (CHAR_A - 10));
#endif

#if defined COMPILE_PCRE8
          if (c > (utf ? 0x10ffffU : 0xffU)) { overflow = TRUE; break; }
#elif defined COMPILE_PCRE16
          if (c > (utf ? 0x10ffffU : 0xffffU)) { overflow = TRUE; break; }
#elif defined COMPILE_PCRE32
          if (utf && c > 0x10ffffU) { overflow = TRUE; break; }
#endif
          }

        if (overflow)
          {
          while (MAX_255(*ptr) && (digitab[*ptr] & ctype_xdigit) != 0) ptr++;
          *errorcodeptr = ERR34;
          }

        else if (*ptr == CHAR_RIGHT_CURLY_BRACKET)
          {
          if (utf && c >= 0xd800 && c <= 0xdfff) *errorcodeptr = ERR73;
          }

        /* If the sequence of hex digits does not end with '}', give an error.
        We used just to recognize this construct and fall through to the normal
        \x handling, but nowadays Perl gives an error, which seems much more
        sensible, so we do too. */

        else *errorcodeptr = ERR79;
        }   /* End of \x{} processing */

      /* Read a single-byte hex-defined char (up to two hex digits after \x) */

      else
        {
        c = 0;
        while (i++ < 2 && MAX_255(ptr[1]) && (digitab[ptr[1]] & ctype_xdigit) != 0)
          {
          pcre_uint32 cc;                          /* Some compilers don't like */
          cc = *(++ptr);                           /* ++ in initializers */
#ifndef EBCDIC  /* ASCII/UTF-8 coding */
          if (cc >= CHAR_a) cc -= 32;              /* Convert to upper case */
          c = c * 16 + cc - ((cc < CHAR_A)? CHAR_0 : (CHAR_A - 10));
#else           /* EBCDIC coding */
          if (cc <= CHAR_z) cc += 64;              /* Convert to upper case */
          c = c * 16 + cc - ((cc >= CHAR_0)? CHAR_0 : (CHAR_A - 10));
#endif
          }
        }     /* End of \xdd handling */
      }       /* End of Perl-style \x handling */
    break;

    /* For \c, a following letter is upper-cased; then the 0x40 bit is flipped.
    An error is given if the byte following \c is not an ASCII character. This
    coding is ASCII-specific, but then the whole concept of \cx is
    ASCII-specific. (However, an EBCDIC equivalent has now been added.) */

    case CHAR_c:
    c = *(++ptr);
    if (c == CHAR_NULL)
      {
      *errorcodeptr = ERR2;
      break;
      }
#ifndef EBCDIC    /* ASCII/UTF-8 coding */
    if (c > 127)  /* Excludes all non-ASCII in either mode */
      {
      *errorcodeptr = ERR68;
      break;
      }
    if (c >= CHAR_a && c <= CHAR_z) c -= 32;
    c ^= 0x40;
#else             /* EBCDIC coding */
    if (c >= CHAR_a && c <= CHAR_z) c += 64;
    if (c == CHAR_QUESTION_MARK)
      c = ('\\' == 188 && '`' == 74)? 0x5f : 0xff;
    else
      {
      for (i = 0; i < 32; i++)
        {
        if (c == ebcdic_escape_c[i]) break;
        }
      if (i < 32) c = i; else *errorcodeptr = ERR68;
      }
#endif
    break;

    /* PCRE_EXTRA enables extensions to Perl in the matter of escapes. Any
    other alphanumeric following \ is an error if PCRE_EXTRA was set;
    otherwise, for Perl compatibility, it is a literal. This code looks a bit
    odd, but there used to be some cases other than the default, and there may
    be again in future, so I haven't "optimized" it. */

    default:
    if ((options & PCRE_EXTRA) != 0) switch(c)
      {
      default:
      *errorcodeptr = ERR3;
      break;
      }
    break;
    }
  }

/* Perl supports \N{name} for character names, as well as plain \N for "not
newline". PCRE does not support \N{name}. However, it does support
quantification such as \N{2,3}. */

if (escape == ESC_N && ptr[1] == CHAR_LEFT_CURLY_BRACKET &&
     !is_counted_repeat(ptr+2))
  *errorcodeptr = ERR37;

/* If PCRE_UCP is set, we change the values for \d etc. */

if ((options & PCRE_UCP) != 0 && escape >= ESC_D && escape <= ESC_w)
  escape += (ESC_DU - ESC_D);

/* Set the pointer to the final character before returning. */

*ptrptr = ptr;
*chptr = c;
return escape;
}



#ifdef SUPPORT_UCP
/*************************************************
*               Handle \P and \p                 *
*************************************************/

/* This function is called after \P or \p has been encountered, provided that
PCRE is compiled with support for Unicode properties. On entry, ptrptr is
pointing at the P or p. On exit, it is pointing at the final character of the
escape sequence.

Argument:
  ptrptr         points to the pattern position pointer
  negptr         points to a boolean that is set TRUE for negation else FALSE
  ptypeptr       points to an unsigned int that is set to the type value
  pdataptr       points to an unsigned int that is set to the detailed property value
  errorcodeptr   points to the error code variable

Returns:         TRUE if the type value was found, or FALSE for an invalid type
*/

static BOOL
get_ucp(const pcre_uchar **ptrptr, BOOL *negptr, unsigned int *ptypeptr,
  unsigned int *pdataptr, int *errorcodeptr)
{
pcre_uchar c;
int i, bot, top;
const pcre_uchar *ptr = *ptrptr;
pcre_uchar name[32];

c = *(++ptr);
if (c == CHAR_NULL) goto ERROR_RETURN;

*negptr = FALSE;

/* \P or \p can be followed by a name in {}, optionally preceded by ^ for
negation. */

if (c == CHAR_LEFT_CURLY_BRACKET)
  {
  if (ptr[1] == CHAR_CIRCUMFLEX_ACCENT)
    {
    *negptr = TRUE;
    ptr++;
    }
  for (i = 0; i < (int)(sizeof(name) / sizeof(pcre_uchar)) - 1; i++)
    {
    c = *(++ptr);
    if (c == CHAR_NULL) goto ERROR_RETURN;
    if (c == CHAR_RIGHT_CURLY_BRACKET) break;
    name[i] = c;
    }
  if (c != CHAR_RIGHT_CURLY_BRACKET) goto ERROR_RETURN;
  name[i] = 0;
  }

/* Otherwise there is just one following character */

else
  {
  name[0] = c;
  name[1] = 0;
  }

*ptrptr = ptr;

/* Search for a recognized property name using binary chop */

bot = 0;
top = PRIV(utt_size);

while (bot < top)
  {
  int r;
  i = (bot + top) >> 1;
  r = STRCMP_UC_C8(name, PRIV(utt_names) + PRIV(utt)[i].name_offset);
  if (r == 0)
    {
    *ptypeptr = PRIV(utt)[i].type;
    *pdataptr = PRIV(utt)[i].value;
    return TRUE;
    }
  if (r > 0) bot = i + 1; else top = i;
  }

*errorcodeptr = ERR47;
*ptrptr = ptr;
return FALSE;

ERROR_RETURN:
*errorcodeptr = ERR46;
*ptrptr = ptr;
return FALSE;
}
#endif



/*************************************************
*         Read repeat counts                     *
*************************************************/

/* Read an item of the form {n,m} and return the values. This is called only
after is_counted_repeat() has confirmed that a repeat-count quantifier exists,
so the syntax is guaranteed to be correct, but we need to check the values.

Arguments:
  p              pointer to first char after '{'
  minp           pointer to int for min
  maxp           pointer to int for max
                 returned as -1 if no max
  errorcodeptr   points to error code variable

Returns:         pointer to '}' on success;
                 current ptr on error, with errorcodeptr set non-zero
*/

static const pcre_uchar *
read_repeat_counts(const pcre_uchar *p, int *minp, int *maxp, int *errorcodeptr)
{
int min = 0;
int max = -1;

while (IS_DIGIT(*p))
  {
  min = min * 10 + (int)(*p++ - CHAR_0);
  if (min > 65535)
    {
    *errorcodeptr = ERR5;
    return p;
    }
  }

if (*p == CHAR_RIGHT_CURLY_BRACKET) max = min; else
  {
  if (*(++p) != CHAR_RIGHT_CURLY_BRACKET)
    {
    max = 0;
    while(IS_DIGIT(*p))
      {
      max = max * 10 + (int)(*p++ - CHAR_0);
      if (max > 65535)
        {
        *errorcodeptr = ERR5;
        return p;
        }
      }
    if (max < min)
      {
      *errorcodeptr = ERR4;
      return p;
      }
    }
  }

*minp = min;
*maxp = max;
return p;
}



/*************************************************
*      Find first significant op code            *
*************************************************/

/* This is called by several functions that scan a compiled expression looking
for a fixed first character, or an anchoring op code etc. It skips over things
that do not influence this. For some calls, it makes sense to skip negative
forward and all backward assertions, and also the \b assertion; for others it
does not.

Arguments:
  code         pointer to the start of the group
  skipassert   TRUE if certain assertions are to be skipped

Returns:       pointer to the first significant opcode
*/

static const pcre_uchar*
first_significant_code(const pcre_uchar *code, BOOL skipassert)
{
for (;;)
  {
  switch ((int)*code)
    {
    case OP_ASSERT_NOT:
    case OP_ASSERTBACK:
    case OP_ASSERTBACK_NOT:
    if (!skipassert) return code;
    do code += GET(code, 1); while (*code == OP_ALT);
    code += PRIV(OP_lengths)[*code];
    break;

    case OP_WORD_BOUNDARY:
    case OP_NOT_WORD_BOUNDARY:
    if (!skipassert) return code;
    /* Fall through */

    case OP_CALLOUT:
    case OP_CREF:
    case OP_DNCREF:
    case OP_RREF:
    case OP_DNRREF:
    case OP_DEF:
    code += PRIV(OP_lengths)[*code];
    break;

    default:
    return code;
    }
  }
/* Control never reaches here */
}



/*************************************************
*        Find the fixed length of a branch       *
*************************************************/

/* Scan a branch and compute the fixed length of subject that will match it,
if the length is fixed. This is needed for dealing with backward assertions.
In UTF8 mode, the result is in characters rather than bytes. The branch is
temporarily terminated with OP_END when this function is called.

This function is called when a backward assertion is encountered, so that if it
fails, the error message can point to the correct place in the pattern.
However, we cannot do this when the assertion contains subroutine calls,
because they can be forward references. We solve this by remembering this case
and doing the check at the end; a flag specifies which mode we are running in.

Arguments:
  code     points to the start of the pattern (the bracket)
  utf      TRUE in UTF-8 / UTF-16 / UTF-32 mode
  atend    TRUE if called when the pattern is complete
  cd       the "compile data" structure
  recurses    chain of recurse_check to catch mutual recursion

Returns:   the fixed length,
             or -1 if there is no fixed length,
             or -2 if \C was encountered (in UTF-8 mode only)
             or -3 if an OP_RECURSE item was encountered and atend is FALSE
             or -4 if an unknown opcode was encountered (internal error)
*/

static int
find_fixedlength(pcre_uchar *code, BOOL utf, BOOL atend, compile_data *cd,
  recurse_check *recurses)
{
int length = -1;
recurse_check this_recurse;
register int branchlength = 0;
register pcre_uchar *cc = code + 1 + LINK_SIZE;

/* Scan along the opcodes for this branch. If we get to the end of the
branch, check the length against that of the other branches. */

for (;;)
  {
  int d;
  pcre_uchar *ce, *cs;
  register pcre_uchar op = *cc;

  switch (op)
    {
    /* We only need to continue for OP_CBRA (normal capturing bracket) and
    OP_BRA (normal non-capturing bracket) because the other variants of these
    opcodes are all concerned with unlimited repeated groups, which of course
    are not of fixed length. */

    case OP_CBRA:
    case OP_BRA:
    case OP_ONCE:
    case OP_ONCE_NC:
    case OP_COND:
    d = find_fixedlength(cc + ((op == OP_CBRA)? IMM2_SIZE : 0), utf, atend, cd,
      recurses);
    if (d < 0) return d;
    branchlength += d;
    do cc += GET(cc, 1); while (*cc == OP_ALT);
    cc += 1 + LINK_SIZE;
    break;

    /* Reached end of a branch; if it's a ket it is the end of a nested call.
    If it's ALT it is an alternation in a nested call. An ACCEPT is effectively
    an ALT. If it is END it's the end of the outer call. All can be handled by
    the same code. Note that we must not include the OP_KETRxxx opcodes here,
    because they all imply an unlimited repeat. */

    case OP_ALT:
    case OP_KET:
    case OP_END:
    case OP_ACCEPT:
    case OP_ASSERT_ACCEPT:
    if (length < 0) length = branchlength;
      else if (length != branchlength) return -1;
    if (*cc != OP_ALT) return length;
    cc += 1 + LINK_SIZE;
    branchlength = 0;
    break;

    /* A true recursion implies not fixed length, but a subroutine call may
    be OK. If the subroutine is a forward reference, we can't deal with
    it until the end of the pattern, so return -3. */

    case OP_RECURSE:
    if (!atend) return -3;
    cs = ce = (pcre_uchar *)cd->start_code + GET(cc, 1);  /* Start subpattern */
    do ce += GET(ce, 1); while (*ce == OP_ALT);           /* End subpattern */
    if (cc > cs && cc < ce) return -1;                    /* Recursion */
    else   /* Check for mutual recursion */
      {
      recurse_check *r = recurses;
      for (r = recurses; r != NULL; r = r->prev) if (r->group == cs) break;
      if (r != NULL) return -1;   /* Mutual recursion */
      }
    this_recurse.prev = recurses;
    this_recurse.group = cs;
    d = find_fixedlength(cs + IMM2_SIZE, utf, atend, cd, &this_recurse);
    if (d < 0) return d;
    branchlength += d;
    cc += 1 + LINK_SIZE;
    break;

    /* Skip over assertive subpatterns */

    case OP_ASSERT:
    case OP_ASSERT_NOT:
    case OP_ASSERTBACK:
    case OP_ASSERTBACK_NOT:
    do cc += GET(cc, 1); while (*cc == OP_ALT);
    cc += 1 + LINK_SIZE;
    break;

    /* Skip over things that don't match chars */

    case OP_MARK:
    case OP_PRUNE_ARG:
    case OP_SKIP_ARG:
    case OP_THEN_ARG:
    cc += cc[1] + PRIV(OP_lengths)[*cc];
    break;

    case OP_CALLOUT:
    case OP_CIRC:
    case OP_CIRCM:
    case OP_CLOSE:
    case OP_COMMIT:
    case OP_CREF:
    case OP_DEF:
    case OP_DNCREF:
    case OP_DNRREF:
    case OP_DOLL:
    case OP_DOLLM:
    case OP_EOD:
    case OP_EODN:
    case OP_FAIL:
    case OP_NOT_WORD_BOUNDARY:
    case OP_PRUNE:
    case OP_REVERSE:
    case OP_RREF:
    case OP_SET_SOM:
    case OP_SKIP:
    case OP_SOD:
    case OP_SOM:
    case OP_THEN:
    case OP_WORD_BOUNDARY:
    cc += PRIV(OP_lengths)[*cc];
    break;

    /* Handle literal characters */

    case OP_CHAR:
    case OP_CHARI:
    case OP_NOT:
    case OP_NOTI:
    branchlength++;
    cc += 2;
#ifdef SUPPORT_UTF
    if (utf && HAS_EXTRALEN(cc[-1])) cc += GET_EXTRALEN(cc[-1]);
#endif
    break;

    /* Handle exact repetitions. The count is already in characters, but we
    need to skip over a multibyte character in UTF8 mode.  */

    case OP_EXACT:
    case OP_EXACTI:
    case OP_NOTEXACT:
    case OP_NOTEXACTI:
    branchlength += (int)GET2(cc,1);
    cc += 2 + IMM2_SIZE;
#ifdef SUPPORT_UTF
    if (utf && HAS_EXTRALEN(cc[-1])) cc += GET_EXTRALEN(cc[-1]);
#endif
    break;

    case OP_TYPEEXACT:
    branchlength += GET2(cc,1);
    if (cc[1 + IMM2_SIZE] == OP_PROP || cc[1 + IMM2_SIZE] == OP_NOTPROP)
      cc += 2;
    cc += 1 + IMM2_SIZE + 1;
    break;

    /* Handle single-char matchers */

    case OP_PROP:
    case OP_NOTPROP:
    cc += 2;
    /* Fall through */

    case OP_HSPACE:
    case OP_VSPACE:
    case OP_NOT_HSPACE:
    case OP_NOT_VSPACE:
    case OP_NOT_DIGIT:
    case OP_DIGIT:
    case OP_NOT_WHITESPACE:
    case OP_WHITESPACE:
    case OP_NOT_WORDCHAR:
    case OP_WORDCHAR:
    case OP_ANY:
    case OP_ALLANY:
    branchlength++;
    cc++;
    break;

    /* The single-byte matcher isn't allowed. This only happens in UTF-8 mode;
    otherwise \C is coded as OP_ALLANY. */

    case OP_ANYBYTE:
    return -2;

    /* Check a class for variable quantification */

    case OP_CLASS:
    case OP_NCLASS:
#if defined SUPPORT_UTF || defined COMPILE_PCRE16 || defined COMPILE_PCRE32
    case OP_XCLASS:
    /* The original code caused an unsigned overflow in 64 bit systems,
    so now we use a conditional statement. */
    if (op == OP_XCLASS)
      cc += GET(cc, 1);
    else
      cc += PRIV(OP_lengths)[OP_CLASS];
#else
    cc += PRIV(OP_lengths)[OP_CLASS];
#endif

    switch (*cc)
      {
      case OP_CRSTAR:
      case OP_CRMINSTAR:
      case OP_CRPLUS:
      case OP_CRMINPLUS:
      case OP_CRQUERY:
      case OP_CRMINQUERY:
      case OP_CRPOSSTAR:
      case OP_CRPOSPLUS:
      case OP_CRPOSQUERY:
      return -1;

      case OP_CRRANGE:
      case OP_CRMINRANGE:
      case OP_CRPOSRANGE:
      if (GET2(cc,1) != GET2(cc,1+IMM2_SIZE)) return -1;
      branchlength += (int)GET2(cc,1);
      cc += 1 + 2 * IMM2_SIZE;
      break;

      default:
      branchlength++;
      }
    break;

    /* Anything else is variable length */

    case OP_ANYNL:
    case OP_BRAMINZERO:
    case OP_BRAPOS:
    case OP_BRAPOSZERO:
    case OP_BRAZERO:
    case OP_CBRAPOS:
    case OP_EXTUNI:
    case OP_KETRMAX:
    case OP_KETRMIN:
    case OP_KETRPOS:
    case OP_MINPLUS:
    case OP_MINPLUSI:
    case OP_MINQUERY:
    case OP_MINQUERYI:
    case OP_MINSTAR:
    case OP_MINSTARI:
    case OP_MINUPTO:
    case OP_MINUPTOI:
    case OP_NOTMINPLUS:
    case OP_NOTMINPLUSI:
    case OP_NOTMINQUERY:
    case OP_NOTMINQUERYI:
    case OP_NOTMINSTAR:
    case OP_NOTMINSTARI:
    case OP_NOTMINUPTO:
    case OP_NOTMINUPTOI:
    case OP_NOTPLUS:
    case OP_NOTPLUSI:
    case OP_NOTPOSPLUS:
    case OP_NOTPOSPLUSI:
    case OP_NOTPOSQUERY:
    case OP_NOTPOSQUERYI:
    case OP_NOTPOSSTAR:
    case OP_NOTPOSSTARI:
    case OP_NOTPOSUPTO:
    case OP_NOTPOSUPTOI:
    case OP_NOTQUERY:
    case OP_NOTQUERYI:
    case OP_NOTSTAR:
    case OP_NOTSTARI:
    case OP_NOTUPTO:
    case OP_NOTUPTOI:
    case OP_PLUS:
    case OP_PLUSI:
    case OP_POSPLUS:
    case OP_POSPLUSI:
    case OP_POSQUERY:
    case OP_POSQUERYI:
    case OP_POSSTAR:
    case OP_POSSTARI:
    case OP_POSUPTO:
    case OP_POSUPTOI:
    case OP_QUERY:
    case OP_QUERYI:
    case OP_REF:
    case OP_REFI:
    case OP_DNREF:
    case OP_DNREFI:
    case OP_SBRA:
    case OP_SBRAPOS:
    case OP_SCBRA:
    case OP_SCBRAPOS:
    case OP_SCOND:
    case OP_SKIPZERO:
    case OP_STAR:
    case OP_STARI:
    case OP_TYPEMINPLUS:
    case OP_TYPEMINQUERY:
    case OP_TYPEMINSTAR:
    case OP_TYPEMINUPTO:
    case OP_TYPEPLUS:
    case OP_TYPEPOSPLUS:
    case OP_TYPEPOSQUERY:
    case OP_TYPEPOSSTAR:
    case OP_TYPEPOSUPTO:
    case OP_TYPEQUERY:
    case OP_TYPESTAR:
    case OP_TYPEUPTO:
    case OP_UPTO:
    case OP_UPTOI:
    return -1;

    /* Catch unrecognized opcodes so that when new ones are added they
    are not forgotten, as has happened in the past. */

    default:
    return -4;
    }
  }
/* Control never gets here */
}



/*************************************************
*    Scan compiled regex for specific bracket    *
*************************************************/

/* This little function scans through a compiled pattern until it finds a
capturing bracket with the given number, or, if the number is negative, an
instance of OP_REVERSE for a lookbehind. The function is global in the C sense
so that it can be called from pcre_study() when finding the minimum matching
length.

Arguments:
  code        points to start of expression
  utf         TRUE in UTF-8 / UTF-16 / UTF-32 mode
  number      the required bracket number or negative to find a lookbehind

Returns:      pointer to the opcode for the bracket, or NULL if not found
*/

const pcre_uchar *
PRIV(find_bracket)(const pcre_uchar *code, BOOL utf, int number)
{
for (;;)
  {
  register pcre_uchar c = *code;

  if (c == OP_END) return NULL;

  /* XCLASS is used for classes that cannot be represented just by a bit
  map. This includes negated single high-valued characters. The length in
  the table is zero; the actual length is stored in the compiled code. */

  if (c == OP_XCLASS) code += GET(code, 1);

  /* Handle recursion */

  else if (c == OP_REVERSE)
    {
    if (number < 0) return (pcre_uchar *)code;
    code += PRIV(OP_lengths)[c];
    }

  /* Handle capturing bracket */

  else if (c == OP_CBRA || c == OP_SCBRA ||
           c == OP_CBRAPOS || c == OP_SCBRAPOS)
    {
    int n = (int)GET2(code, 1+LINK_SIZE);
    if (n == number) return (pcre_uchar *)code;
    code += PRIV(OP_lengths)[c];
    }

  /* Otherwise, we can get the item's length from the table, except that for
  repeated character types, we have to test for \p and \P, which have an extra
  two bytes of parameters, and for MARK/PRUNE/SKIP/THEN with an argument, we
  must add in its length. */

  else
    {
    switch(c)
      {
      case OP_TYPESTAR:
      case OP_TYPEMINSTAR:
      case OP_TYPEPLUS:
      case OP_TYPEMINPLUS:
      case OP_TYPEQUERY:
      case OP_TYPEMINQUERY:
      case OP_TYPEPOSSTAR:
      case OP_TYPEPOSPLUS:
      case OP_TYPEPOSQUERY:
      if (code[1] == OP_PROP || code[1] == OP_NOTPROP) code += 2;
      break;

      case OP_TYPEUPTO:
      case OP_TYPEMINUPTO:
      case OP_TYPEEXACT:
      case OP_TYPEPOSUPTO:
      if (code[1 + IMM2_SIZE] == OP_PROP || code[1 + IMM2_SIZE] == OP_NOTPROP)
        code += 2;
      break;

      case OP_MARK:
      case OP_PRUNE_ARG:
      case OP_SKIP_ARG:
      case OP_THEN_ARG:
      code += code[1];
      break;
      }

    /* Add in the fixed length from the table */

    code += PRIV(OP_lengths)[c];

  /* In UTF-8 mode, opcodes that are followed by a character may be followed by
  a multi-byte character. The length in the table is a minimum, so we have to
  arrange to skip the extra bytes. */

#if defined SUPPORT_UTF && !defined COMPILE_PCRE32
    if (utf) switch(c)
      {
      case OP_CHAR:
      case OP_CHARI:
      case OP_NOT:
      case OP_NOTI:
      case OP_EXACT:
      case OP_EXACTI:
      case OP_NOTEXACT:
      case OP_NOTEXACTI:
      case OP_UPTO:
      case OP_UPTOI:
      case OP_NOTUPTO:
      case OP_NOTUPTOI:
      case OP_MINUPTO:
      case OP_MINUPTOI:
      case OP_NOTMINUPTO:
      case OP_NOTMINUPTOI:
      case OP_POSUPTO:
      case OP_POSUPTOI:
      case OP_NOTPOSUPTO:
      case OP_NOTPOSUPTOI:
      case OP_STAR:
      case OP_STARI:
      case OP_NOTSTAR:
      case OP_NOTSTARI:
      case OP_MINSTAR:
      case OP_MINSTARI:
      case OP_NOTMINSTAR:
      case OP_NOTMINSTARI:
      case OP_POSSTAR:
      case OP_POSSTARI:
      case OP_NOTPOSSTAR:
      case OP_NOTPOSSTARI:
      case OP_PLUS:
      case OP_PLUSI:
      case OP_NOTPLUS:
      case OP_NOTPLUSI:
      case OP_MINPLUS:
      case OP_MINPLUSI:
      case OP_NOTMINPLUS:
      case OP_NOTMINPLUSI:
      case OP_POSPLUS:
      case OP_POSPLUSI:
      case OP_NOTPOSPLUS:
      case OP_NOTPOSPLUSI:
      case OP_QUERY:
      case OP_QUERYI:
      case OP_NOTQUERY:
      case OP_NOTQUERYI:
      case OP_MINQUERY:
      case OP_MINQUERYI:
      case OP_NOTMINQUERY:
      case OP_NOTMINQUERYI:
      case OP_POSQUERY:
      case OP_POSQUERYI:
      case OP_NOTPOSQUERY:
      case OP_NOTPOSQUERYI:
      if (HAS_EXTRALEN(code[-1])) code += GET_EXTRALEN(code[-1]);
      break;
      }
#else
    (void)(utf);  /* Keep compiler happy by referencing function argument */
#endif
    }
  }
}



/*************************************************
*   Scan compiled regex for recursion reference  *
*************************************************/

/* This little function scans through a compiled pattern until it finds an
instance of OP_RECURSE.

Arguments:
  code        points to start of expression
  utf         TRUE in UTF-8 / UTF-16 / UTF-32 mode

Returns:      pointer to the opcode for OP_RECURSE, or NULL if not found
*/

static const pcre_uchar *
find_recurse(const pcre_uchar *code, BOOL utf)
{
for (;;)
  {
  register pcre_uchar c = *code;
  if (c == OP_END) return NULL;
  if (c == OP_RECURSE) return code;

  /* XCLASS is used for classes that cannot be represented just by a bit
  map. This includes negated single high-valued characters. The length in
  the table is zero; the actual length is stored in the compiled code. */

  if (c == OP_XCLASS) code += GET(code, 1);

  /* Otherwise, we can get the item's length from the table, except that for
  repeated character types, we have to test for \p and \P, which have an extra
  two bytes of parameters, and for MARK/PRUNE/SKIP/THEN with an argument, we
  must add in its length. */

  else
    {
    switch(c)
      {
      case OP_TYPESTAR:
      case OP_TYPEMINSTAR:
      case OP_TYPEPLUS:
      case OP_TYPEMINPLUS:
      case OP_TYPEQUERY:
      case OP_TYPEMINQUERY:
      case OP_TYPEPOSSTAR:
      case OP_TYPEPOSPLUS:
      case OP_TYPEPOSQUERY:
      if (code[1] == OP_PROP || code[1] == OP_NOTPROP) code += 2;
      break;

      case OP_TYPEPOSUPTO:
      case OP_TYPEUPTO:
      case OP_TYPEMINUPTO:
      case OP_TYPEEXACT:
      if (code[1 + IMM2_SIZE] == OP_PROP || code[1 + IMM2_SIZE] == OP_NOTPROP)
        code += 2;
      break;

      case OP_MARK:
      case OP_PRUNE_ARG:
      case OP_SKIP_ARG:
      case OP_THEN_ARG:
      code += code[1];
      break;
      }

    /* Add in the fixed length from the table */

    code += PRIV(OP_lengths)[c];

    /* In UTF-8 mode, opcodes that are followed by a character may be followed
    by a multi-byte character. The length in the table is a minimum, so we have
    to arrange to skip the extra bytes. */

#if defined SUPPORT_UTF && !defined COMPILE_PCRE32
    if (utf) switch(c)
      {
      case OP_CHAR:
      case OP_CHARI:
      case OP_NOT:
      case OP_NOTI:
      case OP_EXACT:
      case OP_EXACTI:
      case OP_NOTEXACT:
      case OP_NOTEXACTI:
      case OP_UPTO:
      case OP_UPTOI:
      case OP_NOTUPTO:
      case OP_NOTUPTOI:
      case OP_MINUPTO:
      case OP_MINUPTOI:
      case OP_NOTMINUPTO:
      case OP_NOTMINUPTOI:
      case OP_POSUPTO:
      case OP_POSUPTOI:
      case OP_NOTPOSUPTO:
      case OP_NOTPOSUPTOI:
      case OP_STAR:
      case OP_STARI:
      case OP_NOTSTAR:
      case OP_NOTSTARI:
      case OP_MINSTAR:
      case OP_MINSTARI:
      case OP_NOTMINSTAR:
      case OP_NOTMINSTARI:
      case OP_POSSTAR:
      case OP_POSSTARI:
      case OP_NOTPOSSTAR:
      case OP_NOTPOSSTARI:
      case OP_PLUS:
      case OP_PLUSI:
      case OP_NOTPLUS:
      case OP_NOTPLUSI:
      case OP_MINPLUS:
      case OP_MINPLUSI:
      case OP_NOTMINPLUS:
      case OP_NOTMINPLUSI:
      case OP_POSPLUS:
      case OP_POSPLUSI:
      case OP_NOTPOSPLUS:
      case OP_NOTPOSPLUSI:
      case OP_QUERY:
      case OP_QUERYI:
      case OP_NOTQUERY:
      case OP_NOTQUERYI:
      case OP_MINQUERY:
      case OP_MINQUERYI:
      case OP_NOTMINQUERY:
      case OP_NOTMINQUERYI:
      case OP_POSQUERY:
      case OP_POSQUERYI:
      case OP_NOTPOSQUERY:
      case OP_NOTPOSQUERYI:
      if (HAS_EXTRALEN(code[-1])) code += GET_EXTRALEN(code[-1]);
      break;
      }
#else
    (void)(utf);  /* Keep compiler happy by referencing function argument */
#endif
    }
  }
}



/*************************************************
*    Scan compiled branch for non-emptiness      *
*************************************************/

/* This function scans through a branch of a compiled pattern to see whether it
can match the empty string or not. It is called from could_be_empty()
below and from compile_branch() when checking for an unlimited repeat of a
group that can match nothing. Note that first_significant_code() skips over
backward and negative forward assertions when its final argument is TRUE. If we
hit an unclosed bracket, we return "empty" - this means we've struck an inner
bracket whose current branch will already have been scanned.

Arguments:
  code        points to start of search
  endcode     points to where to stop
  utf         TRUE if in UTF-8 / UTF-16 / UTF-32 mode
  cd          contains pointers to tables etc.
  recurses    chain of recurse_check to catch mutual recursion

Returns:      TRUE if what is matched could be empty
*/

static BOOL
could_be_empty_branch(const pcre_uchar *code, const pcre_uchar *endcode,
  BOOL utf, compile_data *cd, recurse_check *recurses)
{
register pcre_uchar c;
recurse_check this_recurse;

for (code = first_significant_code(code + PRIV(OP_lengths)[*code], TRUE);
     code < endcode;
     code = first_significant_code(code + PRIV(OP_lengths)[c], TRUE))
  {
  const pcre_uchar *ccode;

  c = *code;

  /* Skip over forward assertions; the other assertions are skipped by
  first_significant_code() with a TRUE final argument. */

  if (c == OP_ASSERT)
    {
    do code += GET(code, 1); while (*code == OP_ALT);
    c = *code;
    continue;
    }

  /* For a recursion/subroutine call, if its end has been reached, which
  implies a backward reference subroutine call, we can scan it. If it's a
  forward reference subroutine call, we can't. To detect forward reference
  we have to scan up the list that is kept in the workspace. This function is
  called only when doing the real compile, not during the pre-compile that
  measures the size of the compiled pattern. */

  if (c == OP_RECURSE)
    {
    const pcre_uchar *scode = cd->start_code + GET(code, 1);
    const pcre_uchar *endgroup = scode;
    BOOL empty_branch;

    /* Test for forward reference or uncompleted reference. This is disabled
    when called to scan a completed pattern by setting cd->start_workspace to
    NULL. */

    if (cd->start_workspace != NULL)
      {
      const pcre_uchar *tcode;
      for (tcode = cd->start_workspace; tcode < cd->hwm; tcode += LINK_SIZE)
        if ((int)GET(tcode, 0) == (int)(code + 1 - cd->start_code)) return TRUE;
      if (GET(scode, 1) == 0) return TRUE;    /* Unclosed */
      }

    /* If the reference is to a completed group, we need to detect whether this
    is a recursive call, as otherwise there will be an infinite loop. If it is
    a recursion, just skip over it. Simple recursions are easily detected. For
    mutual recursions we keep a chain on the stack. */

    do endgroup += GET(endgroup, 1); while (*endgroup == OP_ALT);
    if (code >= scode && code <= endgroup) continue;  /* Simple recursion */
    else
      {
      recurse_check *r = recurses;
      for (r = recurses; r != NULL; r = r->prev)
        if (r->group == scode) break;
      if (r != NULL) continue;   /* Mutual recursion */
      }

    /* Completed reference; scan the referenced group, remembering it on the
    stack chain to detect mutual recursions. */

    empty_branch = FALSE;
    this_recurse.prev = recurses;
    this_recurse.group = scode;

    do
      {
      if (could_be_empty_branch(scode, endcode, utf, cd, &this_recurse))
        {
        empty_branch = TRUE;
        break;
        }
      scode += GET(scode, 1);
      }
    while (*scode == OP_ALT);

    if (!empty_branch) return FALSE;  /* All branches are non-empty */
    continue;
    }

  /* Groups with zero repeats can of course be empty; skip them. */

  if (c == OP_BRAZERO || c == OP_BRAMINZERO || c == OP_SKIPZERO ||
      c == OP_BRAPOSZERO)
    {
    code += PRIV(OP_lengths)[c];
    do code += GET(code, 1); while (*code == OP_ALT);
    c = *code;
    continue;
    }

  /* A nested group that is already marked as "could be empty" can just be
  skipped. */

  if (c == OP_SBRA  || c == OP_SBRAPOS ||
      c == OP_SCBRA || c == OP_SCBRAPOS)
    {
    do code += GET(code, 1); while (*code == OP_ALT);
    c = *code;
    continue;
    }

  /* For other groups, scan the branches. */

  if (c == OP_BRA  || c == OP_BRAPOS ||
      c == OP_CBRA || c == OP_CBRAPOS ||
      c == OP_ONCE || c == OP_ONCE_NC ||
      c == OP_COND || c == OP_SCOND)
    {
    BOOL empty_branch;
    if (GET(code, 1) == 0) return TRUE;    /* Hit unclosed bracket */

    /* If a conditional group has only one branch, there is a second, implied,
    empty branch, so just skip over the conditional, because it could be empty.
    Otherwise, scan the individual branches of the group. */

    if (c == OP_COND && code[GET(code, 1)] != OP_ALT)
      code += GET(code, 1);
    else
      {
      empty_branch = FALSE;
      do
        {
        if (!empty_branch && could_be_empty_branch(code, endcode, utf, cd,
          recurses)) empty_branch = TRUE;
        code += GET(code, 1);
        }
      while (*code == OP_ALT);
      if (!empty_branch) return FALSE;   /* All branches are non-empty */
      }

    c = *code;
    continue;
    }

  /* Handle the other opcodes */

  switch (c)
    {
    /* Check for quantifiers after a class. XCLASS is used for classes that
    cannot be represented just by a bit map. This includes negated single
    high-valued characters. The length in PRIV(OP_lengths)[] is zero; the
    actual length is stored in the compiled code, so we must update "code"
    here. */

#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
    case OP_XCLASS:
    ccode = code += GET(code, 1);
    goto CHECK_CLASS_REPEAT;
#endif

    case OP_CLASS:
    case OP_NCLASS:
    ccode = code + PRIV(OP_lengths)[OP_CLASS];

#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
    CHECK_CLASS_REPEAT:
#endif

    switch (*ccode)
      {
      case OP_CRSTAR:            /* These could be empty; continue */
      case OP_CRMINSTAR:
      case OP_CRQUERY:
      case OP_CRMINQUERY:
      case OP_CRPOSSTAR:
      case OP_CRPOSQUERY:
      break;

      default:                   /* Non-repeat => class must match */
      case OP_CRPLUS:            /* These repeats aren't empty */
      case OP_CRMINPLUS:
      case OP_CRPOSPLUS:
      return FALSE;

      case OP_CRRANGE:
      case OP_CRMINRANGE:
      case OP_CRPOSRANGE:
      if (GET2(ccode, 1) > 0) return FALSE;  /* Minimum > 0 */
      break;
      }
    break;

    /* Opcodes that must match a character */

    case OP_ANY:
    case OP_ALLANY:
    case OP_ANYBYTE:

    case OP_PROP:
    case OP_NOTPROP:
    case OP_ANYNL:

    case OP_NOT_HSPACE:
    case OP_HSPACE:
    case OP_NOT_VSPACE:
    case OP_VSPACE:
    case OP_EXTUNI:

    case OP_NOT_DIGIT:
    case OP_DIGIT:
    case OP_NOT_WHITESPACE:
    case OP_WHITESPACE:
    case OP_NOT_WORDCHAR:
    case OP_WORDCHAR:

    case OP_CHAR:
    case OP_CHARI:
    case OP_NOT:
    case OP_NOTI:

    case OP_PLUS:
    case OP_PLUSI:
    case OP_MINPLUS:
    case OP_MINPLUSI:

    case OP_NOTPLUS:
    case OP_NOTPLUSI:
    case OP_NOTMINPLUS:
    case OP_NOTMINPLUSI:

    case OP_POSPLUS:
    case OP_POSPLUSI:
    case OP_NOTPOSPLUS:
    case OP_NOTPOSPLUSI:

    case OP_EXACT:
    case OP_EXACTI:
    case OP_NOTEXACT:
    case OP_NOTEXACTI:

    case OP_TYPEPLUS:
    case OP_TYPEMINPLUS:
    case OP_TYPEPOSPLUS:
    case OP_TYPEEXACT:

    return FALSE;

    /* These are going to continue, as they may be empty, but we have to
    fudge the length for the \p and \P cases. */

    case OP_TYPESTAR:
    case OP_TYPEMINSTAR:
    case OP_TYPEPOSSTAR:
    case OP_TYPEQUERY:
    case OP_TYPEMINQUERY:
    case OP_TYPEPOSQUERY:
    if (code[1] == OP_PROP || code[1] == OP_NOTPROP) code += 2;
    break;

    /* Same for these */

    case OP_TYPEUPTO:
    case OP_TYPEMINUPTO:
    case OP_TYPEPOSUPTO:
    if (code[1 + IMM2_SIZE] == OP_PROP || code[1 + IMM2_SIZE] == OP_NOTPROP)
      code += 2;
    break;

    /* End of branch */

    case OP_KET:
    case OP_KETRMAX:
    case OP_KETRMIN:
    case OP_KETRPOS:
    case OP_ALT:
    return TRUE;

    /* In UTF-8 mode, STAR, MINSTAR, POSSTAR, QUERY, MINQUERY, POSQUERY, UPTO,
    MINUPTO, and POSUPTO and their caseless and negative versions may be
    followed by a multibyte character. */

#if defined SUPPORT_UTF && !defined COMPILE_PCRE32
    case OP_STAR:
    case OP_STARI:
    case OP_NOTSTAR:
    case OP_NOTSTARI:

    case OP_MINSTAR:
    case OP_MINSTARI:
    case OP_NOTMINSTAR:
    case OP_NOTMINSTARI:

    case OP_POSSTAR:
    case OP_POSSTARI:
    case OP_NOTPOSSTAR:
    case OP_NOTPOSSTARI:

    case OP_QUERY:
    case OP_QUERYI:
    case OP_NOTQUERY:
    case OP_NOTQUERYI:

    case OP_MINQUERY:
    case OP_MINQUERYI:
    case OP_NOTMINQUERY:
    case OP_NOTMINQUERYI:

    case OP_POSQUERY:
    case OP_POSQUERYI:
    case OP_NOTPOSQUERY:
    case OP_NOTPOSQUERYI:

    if (utf && HAS_EXTRALEN(code[1])) code += GET_EXTRALEN(code[1]);
    break;

    case OP_UPTO:
    case OP_UPTOI:
    case OP_NOTUPTO:
    case OP_NOTUPTOI:

    case OP_MINUPTO:
    case OP_MINUPTOI:
    case OP_NOTMINUPTO:
    case OP_NOTMINUPTOI:

    case OP_POSUPTO:
    case OP_POSUPTOI:
    case OP_NOTPOSUPTO:
    case OP_NOTPOSUPTOI:

    if (utf && HAS_EXTRALEN(code[1 + IMM2_SIZE])) code += GET_EXTRALEN(code[1 + IMM2_SIZE]);
    break;
#endif

    /* MARK, and PRUNE/SKIP/THEN with an argument must skip over the argument
    string. */

    case OP_MARK:
    case OP_PRUNE_ARG:
    case OP_SKIP_ARG:
    case OP_THEN_ARG:
    code += code[1];
    break;

    /* None of the remaining opcodes are required to match a character. */

    default:
    break;
    }
  }

return TRUE;
}



/*************************************************
*    Scan compiled regex for non-emptiness       *
*************************************************/

/* This function is called to check for left recursive calls. We want to check
the current branch of the current pattern to see if it could match the empty
string. If it could, we must look outwards for branches at other levels,
stopping when we pass beyond the bracket which is the subject of the recursion.
This function is called only during the real compile, not during the
pre-compile.

Arguments:
  code        points to start of the recursion
  endcode     points to where to stop (current RECURSE item)
  bcptr       points to the chain of current (unclosed) branch starts
  utf         TRUE if in UTF-8 / UTF-16 / UTF-32 mode
  cd          pointers to tables etc

Returns:      TRUE if what is matched could be empty
*/

static BOOL
could_be_empty(const pcre_uchar *code, const pcre_uchar *endcode,
  branch_chain *bcptr, BOOL utf, compile_data *cd)
{
while (bcptr != NULL && bcptr->current_branch >= code)
  {
  if (!could_be_empty_branch(bcptr->current_branch, endcode, utf, cd, NULL))
    return FALSE;
  bcptr = bcptr->outer;
  }
return TRUE;
}



/*************************************************
*        Base opcode of repeated opcodes         *
*************************************************/

/* Returns the base opcode for repeated single character type opcodes. If the
opcode is not a repeated character type, it returns with the original value.

Arguments:  c opcode
Returns:    base opcode for the type
*/

static pcre_uchar
get_repeat_base(pcre_uchar c)
{
return (c > OP_TYPEPOSUPTO)? c :
       (c >= OP_TYPESTAR)?   OP_TYPESTAR :
       (c >= OP_NOTSTARI)?   OP_NOTSTARI :
       (c >= OP_NOTSTAR)?    OP_NOTSTAR :
       (c >= OP_STARI)?      OP_STARI :
                             OP_STAR;
}



#ifdef SUPPORT_UCP
/*************************************************
*        Check a character and a property        *
*************************************************/

/* This function is called by check_auto_possessive() when a property item
is adjacent to a fixed character.

Arguments:
  c            the character
  ptype        the property type
  pdata        the data for the type
  negated      TRUE if it's a negated property (\P or \p{^)

Returns:       TRUE if auto-possessifying is OK
*/

static BOOL
check_char_prop(pcre_uint32 c, unsigned int ptype, unsigned int pdata,
  BOOL negated)
{
const pcre_uint32 *p;
const ucd_record *prop = GET_UCD(c);

switch(ptype)
  {
  case PT_LAMP:
  return (prop->chartype == ucp_Lu ||
          prop->chartype == ucp_Ll ||
          prop->chartype == ucp_Lt) == negated;

  case PT_GC:
  return (pdata == PRIV(ucp_gentype)[prop->chartype]) == negated;

  case PT_PC:
  return (pdata == prop->chartype) == negated;

  case PT_SC:
  return (pdata == prop->script) == negated;

  /* These are specials */

  case PT_ALNUM:
  return (PRIV(ucp_gentype)[prop->chartype] == ucp_L ||
          PRIV(ucp_gentype)[prop->chartype] == ucp_N) == negated;

  /* Perl space used to exclude VT, but from Perl 5.18 it is included, which
  means that Perl space and POSIX space are now identical. PCRE was changed
  at release 8.34. */

  case PT_SPACE:    /* Perl space */
  case PT_PXSPACE:  /* POSIX space */
  switch(c)
    {
    HSPACE_CASES:
    VSPACE_CASES:
    return negated;

    default:
    return (PRIV(ucp_gentype)[prop->chartype] == ucp_Z) == negated;
    }
  break;  /* Control never reaches here */

  case PT_WORD:
  return (PRIV(ucp_gentype)[prop->chartype] == ucp_L ||
          PRIV(ucp_gentype)[prop->chartype] == ucp_N ||
          c == CHAR_UNDERSCORE) == negated;

  case PT_CLIST:
  p = PRIV(ucd_caseless_sets) + prop->caseset;
  for (;;)
    {
    if (c < *p) return !negated;
    if (c == *p++) return negated;
    }
  break;  /* Control never reaches here */
  }

return FALSE;
}
#endif  /* SUPPORT_UCP */



/*************************************************
*        Fill the character property list        *
*************************************************/

/* Checks whether the code points to an opcode that can take part in auto-
possessification, and if so, fills a list with its properties.

Arguments:
  code        points to start of expression
  utf         TRUE if in UTF-8 / UTF-16 / UTF-32 mode
  fcc         points to case-flipping table
  list        points to output list
              list[0] will be filled with the opcode
              list[1] will be non-zero if this opcode
                can match an empty character string
              list[2..7] depends on the opcode

Returns:      points to the start of the next opcode if *code is accepted
              NULL if *code is not accepted
*/

static const pcre_uchar *
get_chr_property_list(const pcre_uchar *code, BOOL utf,
  const pcre_uint8 *fcc, pcre_uint32 *list)
{
pcre_uchar c = *code;
pcre_uchar base;
const pcre_uchar *end;
pcre_uint32 chr;

#ifdef SUPPORT_UCP
pcre_uint32 *clist_dest;
const pcre_uint32 *clist_src;
#else
utf = utf;  /* Suppress "unused parameter" compiler warning */
#endif

list[0] = c;
list[1] = FALSE;
code++;

if (c >= OP_STAR && c <= OP_TYPEPOSUPTO)
  {
  base = get_repeat_base(c);
  c -= (base - OP_STAR);

  if (c == OP_UPTO || c == OP_MINUPTO || c == OP_EXACT || c == OP_POSUPTO)
    code += IMM2_SIZE;

  list[1] = (c != OP_PLUS && c != OP_MINPLUS && c != OP_EXACT && c != OP_POSPLUS);

  switch(base)
    {
    case OP_STAR:
    list[0] = OP_CHAR;
    break;

    case OP_STARI:
    list[0] = OP_CHARI;
    break;

    case OP_NOTSTAR:
    list[0] = OP_NOT;
    break;

    case OP_NOTSTARI:
    list[0] = OP_NOTI;
    break;

    case OP_TYPESTAR:
    list[0] = *code;
    code++;
    break;
    }
  c = list[0];
  }

switch(c)
  {
  case OP_NOT_DIGIT:
  case OP_DIGIT:
  case OP_NOT_WHITESPACE:
  case OP_WHITESPACE:
  case OP_NOT_WORDCHAR:
  case OP_WORDCHAR:
  case OP_ANY:
  case OP_ALLANY:
  case OP_ANYNL:
  case OP_NOT_HSPACE:
  case OP_HSPACE:
  case OP_NOT_VSPACE:
  case OP_VSPACE:
  case OP_EXTUNI:
  case OP_EODN:
  case OP_EOD:
  case OP_DOLL:
  case OP_DOLLM:
  return code;

  case OP_CHAR:
  case OP_NOT:
  GETCHARINCTEST(chr, code);
  list[2] = chr;
  list[3] = NOTACHAR;
  return code;

  case OP_CHARI:
  case OP_NOTI:
  list[0] = (c == OP_CHARI) ? OP_CHAR : OP_NOT;
  GETCHARINCTEST(chr, code);
  list[2] = chr;

#ifdef SUPPORT_UCP
  if (chr < 128 || (chr < 256 && !utf))
    list[3] = fcc[chr];
  else
    list[3] = UCD_OTHERCASE(chr);
#elif defined SUPPORT_UTF || !defined COMPILE_PCRE8
  list[3] = (chr < 256) ? fcc[chr] : chr;
#else
  list[3] = fcc[chr];
#endif

  /* The othercase might be the same value. */

  if (chr == list[3])
    list[3] = NOTACHAR;
  else
    list[4] = NOTACHAR;
  return code;

#ifdef SUPPORT_UCP
  case OP_PROP:
  case OP_NOTPROP:
  if (code[0] != PT_CLIST)
    {
    list[2] = code[0];
    list[3] = code[1];
    return code + 2;
    }

  /* Convert only if we have enough space. */

  clist_src = PRIV(ucd_caseless_sets) + code[1];
  clist_dest = list + 2;
  code += 2;

  do {
     if (clist_dest >= list + 8)
       {
       /* Early return if there is not enough space. This should never
       happen, since all clists are shorter than 5 character now. */
       list[2] = code[0];
       list[3] = code[1];
       return code;
       }
     *clist_dest++ = *clist_src;
     }
  while(*clist_src++ != NOTACHAR);

  /* All characters are stored. The terminating NOTACHAR
  is copied form the clist itself. */

  list[0] = (c == OP_PROP) ? OP_CHAR : OP_NOT;
  return code;
#endif

  case OP_NCLASS:
  case OP_CLASS:
#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
  case OP_XCLASS:
  if (c == OP_XCLASS)
    end = code + GET(code, 0) - 1;
  else
#endif
    end = code + 32 / sizeof(pcre_uchar);

  switch(*end)
    {
    case OP_CRSTAR:
    case OP_CRMINSTAR:
    case OP_CRQUERY:
    case OP_CRMINQUERY:
    case OP_CRPOSSTAR:
    case OP_CRPOSQUERY:
    list[1] = TRUE;
    end++;
    break;

    case OP_CRPLUS:
    case OP_CRMINPLUS:
    case OP_CRPOSPLUS:
    end++;
    break;

    case OP_CRRANGE:
    case OP_CRMINRANGE:
    case OP_CRPOSRANGE:
    list[1] = (GET2(end, 1) == 0);
    end += 1 + 2 * IMM2_SIZE;
    break;
    }
  list[2] = (pcre_uint32)(end - code);
  return end;
  }
return NULL;    /* Opcode not accepted */
}



/*************************************************
*    Scan further character sets for match       *
*************************************************/

/* Checks whether the base and the current opcode have a common character, in
which case the base cannot be possessified.

Arguments:
  code        points to the byte code
  utf         TRUE in UTF-8 / UTF-16 / UTF-32 mode
  cd          static compile data
  base_list   the data list of the base opcode

Returns:      TRUE if the auto-possessification is possible
*/

static BOOL
compare_opcodes(const pcre_uchar *code, BOOL utf, const compile_data *cd,
  const pcre_uint32 *base_list, const pcre_uchar *base_end, int *rec_limit)
{
pcre_uchar c;
pcre_uint32 list[8];
const pcre_uint32 *chr_ptr;
const pcre_uint32 *ochr_ptr;
const pcre_uint32 *list_ptr;
const pcre_uchar *next_code;
#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
const pcre_uchar *xclass_flags;
#endif
const pcre_uint8 *class_bitset;
const pcre_uint8 *set1, *set2, *set_end;
pcre_uint32 chr;
BOOL accepted, invert_bits;
BOOL entered_a_group = FALSE;

if (*rec_limit == 0) return FALSE;
--(*rec_limit);

/* Note: the base_list[1] contains whether the current opcode has greedy
(represented by a non-zero value) quantifier. This is a different from
other character type lists, which stores here that the character iterator
matches to an empty string (also represented by a non-zero value). */

for(;;)
  {
  /* All operations move the code pointer forward.
  Therefore infinite recursions are not possible. */

  c = *code;

  /* Skip over callouts */

  if (c == OP_CALLOUT)
    {
    code += PRIV(OP_lengths)[c];
    continue;
    }

  if (c == OP_ALT)
    {
    do code += GET(code, 1); while (*code == OP_ALT);
    c = *code;
    }

  switch(c)
    {
    case OP_END:
    case OP_KETRPOS:
    /* TRUE only in greedy case. The non-greedy case could be replaced by
    an OP_EXACT, but it is probably not worth it. (And note that OP_EXACT
    uses more memory, which we cannot get at this stage.) */

    return base_list[1] != 0;

    case OP_KET:
    /* If the bracket is capturing, and referenced by an OP_RECURSE, or
    it is an atomic sub-pattern (assert, once, etc.) the non-greedy case
    cannot be converted to a possessive form. */

    if (base_list[1] == 0) return FALSE;

    switch(*(code - GET(code, 1)))
      {
      case OP_ASSERT:
      case OP_ASSERT_NOT:
      case OP_ASSERTBACK:
      case OP_ASSERTBACK_NOT:
      case OP_ONCE:
      case OP_ONCE_NC:
      /* Atomic sub-patterns and assertions can always auto-possessify their
      last iterator. However, if the group was entered as a result of checking
      a previous iterator, this is not possible. */

      return !entered_a_group;
      }

    code += PRIV(OP_lengths)[c];
    continue;

    case OP_ONCE:
    case OP_ONCE_NC:
    case OP_BRA:
    case OP_CBRA:
    next_code = code + GET(code, 1);
    code += PRIV(OP_lengths)[c];

    while (*next_code == OP_ALT)
      {
      if (!compare_opcodes(code, utf, cd, base_list, base_end, rec_limit))
        return FALSE;
      code = next_code + 1 + LINK_SIZE;
      next_code += GET(next_code, 1);
      }

    entered_a_group = TRUE;
    continue;

    case OP_BRAZERO:
    case OP_BRAMINZERO:

    next_code = code + 1;
    if (*next_code != OP_BRA && *next_code != OP_CBRA
        && *next_code != OP_ONCE && *next_code != OP_ONCE_NC) return FALSE;

    do next_code += GET(next_code, 1); while (*next_code == OP_ALT);

    /* The bracket content will be checked by the
    OP_BRA/OP_CBRA case above. */
    next_code += 1 + LINK_SIZE;
    if (!compare_opcodes(next_code, utf, cd, base_list, base_end, rec_limit))
      return FALSE;

    code += PRIV(OP_lengths)[c];
    continue;

    default:
    break;
    }

  /* Check for a supported opcode, and load its properties. */

  code = get_chr_property_list(code, utf, cd->fcc, list);
  if (code == NULL) return FALSE;    /* Unsupported */

  /* If either opcode is a small character list, set pointers for comparing
  characters from that list with another list, or with a property. */

  if (base_list[0] == OP_CHAR)
    {
    chr_ptr = base_list + 2;
    list_ptr = list;
    }
  else if (list[0] == OP_CHAR)
    {
    chr_ptr = list + 2;
    list_ptr = base_list;
    }

  /* Character bitsets can also be compared to certain opcodes. */

  else if (base_list[0] == OP_CLASS || list[0] == OP_CLASS
#ifdef COMPILE_PCRE8
      /* In 8 bit, non-UTF mode, OP_CLASS and OP_NCLASS are the same. */
      || (!utf && (base_list[0] == OP_NCLASS || list[0] == OP_NCLASS))
#endif
      )
    {
#ifdef COMPILE_PCRE8
    if (base_list[0] == OP_CLASS || (!utf && base_list[0] == OP_NCLASS))
#else
    if (base_list[0] == OP_CLASS)
#endif
      {
      set1 = (pcre_uint8 *)(base_end - base_list[2]);
      list_ptr = list;
      }
    else
      {
      set1 = (pcre_uint8 *)(code - list[2]);
      list_ptr = base_list;
      }

    invert_bits = FALSE;
    switch(list_ptr[0])
      {
      case OP_CLASS:
      case OP_NCLASS:
      set2 = (pcre_uint8 *)
        ((list_ptr == list ? code : base_end) - list_ptr[2]);
      break;

#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
      case OP_XCLASS:
      xclass_flags = (list_ptr == list ? code : base_end) - list_ptr[2] + LINK_SIZE;
      if ((*xclass_flags & XCL_HASPROP) != 0) return FALSE;
      if ((*xclass_flags & XCL_MAP) == 0)
        {
        /* No bits are set for characters < 256. */
        if (list[1] == 0) return TRUE;
        /* Might be an empty repeat. */
        continue;
        }
      set2 = (pcre_uint8 *)(xclass_flags + 1);
      break;
#endif

      case OP_NOT_DIGIT:
      invert_bits = TRUE;
      /* Fall through */
      case OP_DIGIT:
      set2 = (pcre_uint8 *)(cd->cbits + cbit_digit);
      break;

      case OP_NOT_WHITESPACE:
      invert_bits = TRUE;
      /* Fall through */
      case OP_WHITESPACE:
      set2 = (pcre_uint8 *)(cd->cbits + cbit_space);
      break;

      case OP_NOT_WORDCHAR:
      invert_bits = TRUE;
      /* Fall through */
      case OP_WORDCHAR:
      set2 = (pcre_uint8 *)(cd->cbits + cbit_word);
      break;

      default:
      return FALSE;
      }

    /* Because the sets are unaligned, we need
    to perform byte comparison here. */
    set_end = set1 + 32;
    if (invert_bits)
      {
      do
        {
        if ((*set1++ & ~(*set2++)) != 0) return FALSE;
        }
      while (set1 < set_end);
      }
    else
      {
      do
        {
        if ((*set1++ & *set2++) != 0) return FALSE;
        }
      while (set1 < set_end);
      }

    if (list[1] == 0) return TRUE;
    /* Might be an empty repeat. */
    continue;
    }

  /* Some property combinations also acceptable. Unicode property opcodes are
  processed specially; the rest can be handled with a lookup table. */

  else
    {
    pcre_uint32 leftop, rightop;

    leftop = base_list[0];
    rightop = list[0];

#ifdef SUPPORT_UCP
    accepted = FALSE; /* Always set in non-unicode case. */
    if (leftop == OP_PROP || leftop == OP_NOTPROP)
      {
      if (rightop == OP_EOD)
        accepted = TRUE;
      else if (rightop == OP_PROP || rightop == OP_NOTPROP)
        {
        int n;
        const pcre_uint8 *p;
        BOOL same = leftop == rightop;
        BOOL lisprop = leftop == OP_PROP;
        BOOL risprop = rightop == OP_PROP;
        BOOL bothprop = lisprop && risprop;

        /* There's a table that specifies how each combination is to be
        processed:
          0   Always return FALSE (never auto-possessify)
          1   Character groups are distinct (possessify if both are OP_PROP)
          2   Check character categories in the same group (general or particular)
          3   Return TRUE if the two opcodes are not the same
          ... see comments below
        */

        n = propposstab[base_list[2]][list[2]];
        switch(n)
          {
          case 0: break;
          case 1: accepted = bothprop; break;
          case 2: accepted = (base_list[3] == list[3]) != same; break;
          case 3: accepted = !same; break;

          case 4:  /* Left general category, right particular category */
          accepted = risprop && catposstab[base_list[3]][list[3]] == same;
          break;

          case 5:  /* Right general category, left particular category */
          accepted = lisprop && catposstab[list[3]][base_list[3]] == same;
          break;

          /* This code is logically tricky. Think hard before fiddling with it.
          The posspropstab table has four entries per row. Each row relates to
          one of PCRE's special properties such as ALNUM or SPACE or WORD.
          Only WORD actually needs all four entries, but using repeats for the
          others means they can all use the same code below.

          The first two entries in each row are Unicode general categories, and
          apply always, because all the characters they include are part of the
          PCRE character set. The third and fourth entries are a general and a
          particular category, respectively, that include one or more relevant
          characters. One or the other is used, depending on whether the check
          is for a general or a particular category. However, in both cases the
          category contains more characters than the specials that are defined
          for the property being tested against. Therefore, it cannot be used
          in a NOTPROP case.

          Example: the row for WORD contains ucp_L, ucp_N, ucp_P, ucp_Po.
          Underscore is covered by ucp_P or ucp_Po. */

          case 6:  /* Left alphanum vs right general category */
          case 7:  /* Left space vs right general category */
          case 8:  /* Left word vs right general category */
          p = posspropstab[n-6];
          accepted = risprop && lisprop ==
            (list[3] != p[0] &&
             list[3] != p[1] &&
            (list[3] != p[2] || !lisprop));
          break;

          case 9:   /* Right alphanum vs left general category */
          case 10:  /* Right space vs left general category */
          case 11:  /* Right word vs left general category */
          p = posspropstab[n-9];
          accepted = lisprop && risprop ==
            (base_list[3] != p[0] &&
             base_list[3] != p[1] &&
            (base_list[3] != p[2] || !risprop));
          break;

          case 12:  /* Left alphanum vs right particular category */
          case 13:  /* Left space vs right particular category */
          case 14:  /* Left word vs right particular category */
          p = posspropstab[n-12];
          accepted = risprop && lisprop ==
            (catposstab[p[0]][list[3]] &&
             catposstab[p[1]][list[3]] &&
            (list[3] != p[3] || !lisprop));
          break;

          case 15:  /* Right alphanum vs left particular category */
          case 16:  /* Right space vs left particular category */
          case 17:  /* Right word vs left particular category */
          p = posspropstab[n-15];
          accepted = lisprop && risprop ==
            (catposstab[p[0]][base_list[3]] &&
             catposstab[p[1]][base_list[3]] &&
            (base_list[3] != p[3] || !risprop));
          break;
          }
        }
      }

    else
#endif  /* SUPPORT_UCP */

    accepted = leftop >= FIRST_AUTOTAB_OP && leftop <= LAST_AUTOTAB_LEFT_OP &&
           rightop >= FIRST_AUTOTAB_OP && rightop <= LAST_AUTOTAB_RIGHT_OP &&
           autoposstab[leftop - FIRST_AUTOTAB_OP][rightop - FIRST_AUTOTAB_OP];

    if (!accepted) return FALSE;

    if (list[1] == 0) return TRUE;
    /* Might be an empty repeat. */
    continue;
    }

  /* Control reaches here only if one of the items is a small character list.
  All characters are checked against the other side. */

  do
    {
    chr = *chr_ptr;

    switch(list_ptr[0])
      {
      case OP_CHAR:
      ochr_ptr = list_ptr + 2;
      do
        {
        if (chr == *ochr_ptr) return FALSE;
        ochr_ptr++;
        }
      while(*ochr_ptr != NOTACHAR);
      break;

      case OP_NOT:
      ochr_ptr = list_ptr + 2;
      do
        {
        if (chr == *ochr_ptr)
          break;
        ochr_ptr++;
        }
      while(*ochr_ptr != NOTACHAR);
      if (*ochr_ptr == NOTACHAR) return FALSE;   /* Not found */
      break;

      /* Note that OP_DIGIT etc. are generated only when PCRE_UCP is *not*
      set. When it is set, \d etc. are converted into OP_(NOT_)PROP codes. */

      case OP_DIGIT:
      if (chr < 256 && (cd->ctypes[chr] & ctype_digit) != 0) return FALSE;
      break;

      case OP_NOT_DIGIT:
      if (chr > 255 || (cd->ctypes[chr] & ctype_digit) == 0) return FALSE;
      break;

      case OP_WHITESPACE:
      if (chr < 256 && (cd->ctypes[chr] & ctype_space) != 0) return FALSE;
      break;

      case OP_NOT_WHITESPACE:
      if (chr > 255 || (cd->ctypes[chr] & ctype_space) == 0) return FALSE;
      break;

      case OP_WORDCHAR:
      if (chr < 255 && (cd->ctypes[chr] & ctype_word) != 0) return FALSE;
      break;

      case OP_NOT_WORDCHAR:
      if (chr > 255 || (cd->ctypes[chr] & ctype_word) == 0) return FALSE;
      break;

      case OP_HSPACE:
      switch(chr)
        {
        HSPACE_CASES: return FALSE;
        default: break;
        }
      break;

      case OP_NOT_HSPACE:
      switch(chr)
        {
        HSPACE_CASES: break;
        default: return FALSE;
        }
      break;

      case OP_ANYNL:
      case OP_VSPACE:
      switch(chr)
        {
        VSPACE_CASES: return FALSE;
        default: break;
        }
      break;

      case OP_NOT_VSPACE:
      switch(chr)
        {
        VSPACE_CASES: break;
        default: return FALSE;
        }
      break;

      case OP_DOLL:
      case OP_EODN:
      switch (chr)
        {
        case CHAR_CR:
        case CHAR_LF:
        case CHAR_VT:
        case CHAR_FF:
        case CHAR_NEL:
#ifndef EBCDIC
        case 0x2028:
        case 0x2029:
#endif  /* Not EBCDIC */
        return FALSE;
        }
      break;

      case OP_EOD:    /* Can always possessify before \z */
      break;

#ifdef SUPPORT_UCP
      case OP_PROP:
      case OP_NOTPROP:
      if (!check_char_prop(chr, list_ptr[2], list_ptr[3],
            list_ptr[0] == OP_NOTPROP))
        return FALSE;
      break;
#endif

      case OP_NCLASS:
      if (chr > 255) return FALSE;
      /* Fall through */

      case OP_CLASS:
      if (chr > 255) break;
      class_bitset = (pcre_uint8 *)
        ((list_ptr == list ? code : base_end) - list_ptr[2]);
      if ((class_bitset[chr >> 3] & (1 << (chr & 7))) != 0) return FALSE;
      break;

#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
      case OP_XCLASS:
      if (PRIV(xclass)(chr, (list_ptr == list ? code : base_end) -
          list_ptr[2] + LINK_SIZE, utf)) return FALSE;
      break;
#endif

      default:
      return FALSE;
      }

    chr_ptr++;
    }
  while(*chr_ptr != NOTACHAR);

  /* At least one character must be matched from this opcode. */

  if (list[1] == 0) return TRUE;
  }

/* Control never reaches here. There used to be a fail-save return FALSE; here,
but some compilers complain about an unreachable statement. */

}



/*************************************************
*    Scan compiled regex for auto-possession     *
*************************************************/

/* Replaces single character iterations with their possessive alternatives
if appropriate. This function modifies the compiled opcode!

Arguments:
  code        points to start of the byte code
  utf         TRUE in UTF-8 / UTF-16 / UTF-32 mode
  cd          static compile data

Returns:      nothing
*/

static void
auto_possessify(pcre_uchar *code, BOOL utf, const compile_data *cd)
{
register pcre_uchar c;
const pcre_uchar *end;
pcre_uchar *repeat_opcode;
pcre_uint32 list[8];
int rec_limit;

for (;;)
  {
  c = *code;

  /* When a pattern with bad UTF-8 encoding is compiled with NO_UTF_CHECK,
  it may compile without complaining, but may get into a loop here if the code
  pointer points to a bad value. This is, of course a documentated possibility,
  when NO_UTF_CHECK is set, so it isn't a bug, but we can detect this case and
  just give up on this optimization. */

  if (c >= OP_TABLE_LENGTH) return;

  if (c >= OP_STAR && c <= OP_TYPEPOSUPTO)
    {
    c -= get_repeat_base(c) - OP_STAR;
    end = (c <= OP_MINUPTO) ?
      get_chr_property_list(code, utf, cd->fcc, list) : NULL;
    list[1] = c == OP_STAR || c == OP_PLUS || c == OP_QUERY || c == OP_UPTO;

    rec_limit = 1000;
    if (end != NULL && compare_opcodes(end, utf, cd, list, end, &rec_limit))
      {
      switch(c)
        {
        case OP_STAR:
        *code += OP_POSSTAR - OP_STAR;
        break;

        case OP_MINSTAR:
        *code += OP_POSSTAR - OP_MINSTAR;
        break;

        case OP_PLUS:
        *code += OP_POSPLUS - OP_PLUS;
        break;

        case OP_MINPLUS:
        *code += OP_POSPLUS - OP_MINPLUS;
        break;

        case OP_QUERY:
        *code += OP_POSQUERY - OP_QUERY;
        break;

        case OP_MINQUERY:
        *code += OP_POSQUERY - OP_MINQUERY;
        break;

        case OP_UPTO:
        *code += OP_POSUPTO - OP_UPTO;
        break;

        case OP_MINUPTO:
        *code += OP_POSUPTO - OP_MINUPTO;
        break;
        }
      }
    c = *code;
    }
  else if (c == OP_CLASS || c == OP_NCLASS || c == OP_XCLASS)
    {
#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
    if (c == OP_XCLASS)
      repeat_opcode = code + GET(code, 1);
    else
#endif
      repeat_opcode = code + 1 + (32 / sizeof(pcre_uchar));

    c = *repeat_opcode;
    if (c >= OP_CRSTAR && c <= OP_CRMINRANGE)
      {
      /* end must not be NULL. */
      end = get_chr_property_list(code, utf, cd->fcc, list);

      list[1] = (c & 1) == 0;

      rec_limit = 1000;
      if (compare_opcodes(end, utf, cd, list, end, &rec_limit))
        {
        switch (c)
          {
          case OP_CRSTAR:
          case OP_CRMINSTAR:
          *repeat_opcode = OP_CRPOSSTAR;
          break;

          case OP_CRPLUS:
          case OP_CRMINPLUS:
          *repeat_opcode = OP_CRPOSPLUS;
          break;

          case OP_CRQUERY:
          case OP_CRMINQUERY:
          *repeat_opcode = OP_CRPOSQUERY;
          break;

          case OP_CRRANGE:
          case OP_CRMINRANGE:
          *repeat_opcode = OP_CRPOSRANGE;
          break;
          }
        }
      }
    c = *code;
    }

  switch(c)
    {
    case OP_END:
    return;

    case OP_TYPESTAR:
    case OP_TYPEMINSTAR:
    case OP_TYPEPLUS:
    case OP_TYPEMINPLUS:
    case OP_TYPEQUERY:
    case OP_TYPEMINQUERY:
    case OP_TYPEPOSSTAR:
    case OP_TYPEPOSPLUS:
    case OP_TYPEPOSQUERY:
    if (code[1] == OP_PROP || code[1] == OP_NOTPROP) code += 2;
    break;

    case OP_TYPEUPTO:
    case OP_TYPEMINUPTO:
    case OP_TYPEEXACT:
    case OP_TYPEPOSUPTO:
    if (code[1 + IMM2_SIZE] == OP_PROP || code[1 + IMM2_SIZE] == OP_NOTPROP)
      code += 2;
    break;

#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
    case OP_XCLASS:
    code += GET(code, 1);
    break;
#endif

    case OP_MARK:
    case OP_PRUNE_ARG:
    case OP_SKIP_ARG:
    case OP_THEN_ARG:
    code += code[1];
    break;
    }

  /* Add in the fixed length from the table */

  code += PRIV(OP_lengths)[c];

  /* In UTF-8 mode, opcodes that are followed by a character may be followed by
  a multi-byte character. The length in the table is a minimum, so we have to
  arrange to skip the extra bytes. */

#if defined SUPPORT_UTF && !defined COMPILE_PCRE32
  if (utf) switch(c)
    {
    case OP_CHAR:
    case OP_CHARI:
    case OP_NOT:
    case OP_NOTI:
    case OP_STAR:
    case OP_MINSTAR:
    case OP_PLUS:
    case OP_MINPLUS:
    case OP_QUERY:
    case OP_MINQUERY:
    case OP_UPTO:
    case OP_MINUPTO:
    case OP_EXACT:
    case OP_POSSTAR:
    case OP_POSPLUS:
    case OP_POSQUERY:
    case OP_POSUPTO:
    case OP_STARI:
    case OP_MINSTARI:
    case OP_PLUSI:
    case OP_MINPLUSI:
    case OP_QUERYI:
    case OP_MINQUERYI:
    case OP_UPTOI:
    case OP_MINUPTOI:
    case OP_EXACTI:
    case OP_POSSTARI:
    case OP_POSPLUSI:
    case OP_POSQUERYI:
    case OP_POSUPTOI:
    case OP_NOTSTAR:
    case OP_NOTMINSTAR:
    case OP_NOTPLUS:
    case OP_NOTMINPLUS:
    case OP_NOTQUERY:
    case OP_NOTMINQUERY:
    case OP_NOTUPTO:
    case OP_NOTMINUPTO:
    case OP_NOTEXACT:
    case OP_NOTPOSSTAR:
    case OP_NOTPOSPLUS:
    case OP_NOTPOSQUERY:
    case OP_NOTPOSUPTO:
    case OP_NOTSTARI:
    case OP_NOTMINSTARI:
    case OP_NOTPLUSI:
    case OP_NOTMINPLUSI:
    case OP_NOTQUERYI:
    case OP_NOTMINQUERYI:
    case OP_NOTUPTOI:
    case OP_NOTMINUPTOI:
    case OP_NOTEXACTI:
    case OP_NOTPOSSTARI:
    case OP_NOTPOSPLUSI:
    case OP_NOTPOSQUERYI:
    case OP_NOTPOSUPTOI:
    if (HAS_EXTRALEN(code[-1])) code += GET_EXTRALEN(code[-1]);
    break;
    }
#else
  (void)(utf);  /* Keep compiler happy by referencing function argument */
#endif
  }
}



/*************************************************
*           Check for POSIX class syntax         *
*************************************************/

/* This function is called when the sequence "[:" or "[." or "[=" is
encountered in a character class. It checks whether this is followed by a
sequence of characters terminated by a matching ":]" or ".]" or "=]". If we
reach an unescaped ']' without the special preceding character, return FALSE.

Originally, this function only recognized a sequence of letters between the
terminators, but it seems that Perl recognizes any sequence of characters,
though of course unknown POSIX names are subsequently rejected. Perl gives an
"Unknown POSIX class" error for [:f\oo:] for example, where previously PCRE
didn't consider this to be a POSIX class. Likewise for [:1234:].

The problem in trying to be exactly like Perl is in the handling of escapes. We
have to be sure that [abc[:x\]pqr] is *not* treated as containing a POSIX
class, but [abc[:x\]pqr:]] is (so that an error can be generated). The code
below handles the special cases \\ and \], but does not try to do any other
escape processing. This makes it different from Perl for cases such as
[:l\ower:] where Perl recognizes it as the POSIX class "lower" but PCRE does
not recognize "l\ower". This is a lesser evil than not diagnosing bad classes
when Perl does, I think.

A user pointed out that PCRE was rejecting [:a[:digit:]] whereas Perl was not.
It seems that the appearance of a nested POSIX class supersedes an apparent
external class. For example, [:a[:digit:]b:] matches "a", "b", ":", or
a digit.

In Perl, unescaped square brackets may also appear as part of class names. For
example, [:a[:abc]b:] gives unknown POSIX class "[:abc]b:]". However, for
[:a[:abc]b][b:] it gives unknown POSIX class "[:abc]b][b:]", which does not
seem right at all. PCRE does not allow closing square brackets in POSIX class
names.

Arguments:
  ptr      pointer to the initial [
  endptr   where to return the end pointer

Returns:   TRUE or FALSE
*/

static BOOL
check_posix_syntax(const pcre_uchar *ptr, const pcre_uchar **endptr)
{
pcre_uchar terminator;          /* Don't combine these lines; the Solaris cc */
terminator = *(++ptr);   /* compiler warns about "non-constant" initializer. */
for (++ptr; *ptr != CHAR_NULL; ptr++)
  {
  if (*ptr == CHAR_BACKSLASH &&
      (ptr[1] == CHAR_RIGHT_SQUARE_BRACKET ||
       ptr[1] == CHAR_BACKSLASH))
    ptr++;
  else if ((*ptr == CHAR_LEFT_SQUARE_BRACKET && ptr[1] == terminator) ||
            *ptr == CHAR_RIGHT_SQUARE_BRACKET) return FALSE;
  else if (*ptr == terminator && ptr[1] == CHAR_RIGHT_SQUARE_BRACKET)
    {
    *endptr = ptr;
    return TRUE;
    }
  }
return FALSE;
}




/*************************************************
*          Check POSIX class name                *
*************************************************/

/* This function is called to check the name given in a POSIX-style class entry
such as [:alnum:].

Arguments:
  ptr        points to the first letter
  len        the length of the name

Returns:     a value representing the name, or -1 if unknown
*/

static int
check_posix_name(const pcre_uchar *ptr, int len)
{
const char *pn = posix_names;
register int yield = 0;
while (posix_name_lengths[yield] != 0)
  {
  if (len == posix_name_lengths[yield] &&
    STRNCMP_UC_C8(ptr, pn, (unsigned int)len) == 0) return yield;
  pn += posix_name_lengths[yield] + 1;
  yield++;
  }
return -1;
}


/*************************************************
*    Adjust OP_RECURSE items in repeated group   *
*************************************************/

/* OP_RECURSE items contain an offset from the start of the regex to the group
that is referenced. This means that groups can be replicated for fixed
repetition simply by copying (because the recursion is allowed to refer to
earlier groups that are outside the current group). However, when a group is
optional (i.e. the minimum quantifier is zero), OP_BRAZERO or OP_SKIPZERO is
inserted before it, after it has been compiled. This means that any OP_RECURSE
items within it that refer to the group itself or any contained groups have to
have their offsets adjusted. That one of the jobs of this function. Before it
is called, the partially compiled regex must be temporarily terminated with
OP_END.

This function has been extended to cope with forward references for recursions
and subroutine calls. It must check the list of such references for the
group we are dealing with. If it finds that one of the recursions in the
current group is on this list, it does not adjust the value in the reference
(which is a group number). After the group has been scanned, all the offsets in
the forward reference list for the group are adjusted.

Arguments:
  group      points to the start of the group
  adjust     the amount by which the group is to be moved
  utf        TRUE in UTF-8 / UTF-16 / UTF-32 mode
  cd         contains pointers to tables etc.
  save_hwm_offset   the hwm forward reference offset at the start of the group

Returns:     nothing
*/

static void
adjust_recurse(pcre_uchar *group, int adjust, BOOL utf, compile_data *cd,
  size_t save_hwm_offset)
{
int offset;
pcre_uchar *hc;
pcre_uchar *ptr = group;

while ((ptr = (pcre_uchar *)find_recurse(ptr, utf)) != NULL)
  {
  for (hc = (pcre_uchar *)cd->start_workspace + save_hwm_offset; hc < cd->hwm;
       hc += LINK_SIZE)
    {
    offset = (int)GET(hc, 0);
    if (cd->start_code + offset == ptr + 1) break;
    }

  /* If we have not found this recursion on the forward reference list, adjust
  the recursion's offset if it's after the start of this group. */

  if (hc >= cd->hwm)
    {
    offset = (int)GET(ptr, 1);
    if (cd->start_code + offset >= group) PUT(ptr, 1, offset + adjust);
    }

  ptr += 1 + LINK_SIZE;
  }

/* Now adjust all forward reference offsets for the group. */

for (hc = (pcre_uchar *)cd->start_workspace + save_hwm_offset; hc < cd->hwm;
     hc += LINK_SIZE)
  {
  offset = (int)GET(hc, 0);
  PUT(hc, 0, offset + adjust);
  }
}



/*************************************************
*        Insert an automatic callout point       *
*************************************************/

/* This function is called when the PCRE_AUTO_CALLOUT option is set, to insert
callout points before each pattern item.

Arguments:
  code           current code pointer
  ptr            current pattern pointer
  cd             pointers to tables etc

Returns:         new code pointer
*/

static pcre_uchar *
auto_callout(pcre_uchar *code, const pcre_uchar *ptr, compile_data *cd)
{
*code++ = OP_CALLOUT;
*code++ = 255;
PUT(code, 0, (int)(ptr - cd->start_pattern));  /* Pattern offset */
PUT(code, LINK_SIZE, 0);                       /* Default length */
return code + 2 * LINK_SIZE;
}



/*************************************************
*         Complete a callout item                *
*************************************************/

/* A callout item contains the length of the next item in the pattern, which
we can't fill in till after we have reached the relevant point. This is used
for both automatic and manual callouts.

Arguments:
  previous_callout   points to previous callout item
  ptr                current pattern pointer
  cd                 pointers to tables etc

Returns:             nothing
*/

static void
complete_callout(pcre_uchar *previous_callout, const pcre_uchar *ptr, compile_data *cd)
{
int length = (int)(ptr - cd->start_pattern - GET(previous_callout, 2));
PUT(previous_callout, 2 + LINK_SIZE, length);
}



#ifdef SUPPORT_UCP
/*************************************************
*           Get othercase range                  *
*************************************************/

/* This function is passed the start and end of a class range, in UTF-8 mode
with UCP support. It searches up the characters, looking for ranges of
characters in the "other" case. Each call returns the next one, updating the
start address. A character with multiple other cases is returned on its own
with a special return value.

Arguments:
  cptr        points to starting character value; updated
  d           end value
  ocptr       where to put start of othercase range
  odptr       where to put end of othercase range

Yield:        -1 when no more
               0 when a range is returned
              >0 the CASESET offset for char with multiple other cases
                in this case, ocptr contains the original
*/

static int
get_othercase_range(pcre_uint32 *cptr, pcre_uint32 d, pcre_uint32 *ocptr,
  pcre_uint32 *odptr)
{
pcre_uint32 c, othercase, next;
unsigned int co;

/* Find the first character that has an other case. If it has multiple other
cases, return its case offset value. */

for (c = *cptr; c <= d; c++)
  {
  if ((co = UCD_CASESET(c)) != 0)
    {
    *ocptr = c++;   /* Character that has the set */
    *cptr = c;      /* Rest of input range */
    return (int)co;
    }
  if ((othercase = UCD_OTHERCASE(c)) != c) break;
  }

if (c > d) return -1;  /* Reached end of range */

/* Found a character that has a single other case. Search for the end of the
range, which is either the end of the input range, or a character that has zero
or more than one other cases. */

*ocptr = othercase;
next = othercase + 1;

for (++c; c <= d; c++)
  {
  if ((co = UCD_CASESET(c)) != 0 || UCD_OTHERCASE(c) != next) break;
  next++;
  }

*odptr = next - 1;     /* End of othercase range */
*cptr = c;             /* Rest of input range */
return 0;
}
#endif  /* SUPPORT_UCP */



/*************************************************
*        Add a character or range to a class     *
*************************************************/

/* This function packages up the logic of adding a character or range of
characters to a class. The character values in the arguments will be within the
valid values for the current mode (8-bit, 16-bit, UTF, etc). This function is
mutually recursive with the function immediately below.

Arguments:
  classbits     the bit map for characters < 256
  uchardptr     points to the pointer for extra data
  options       the options word
  cd            contains pointers to tables etc.
  start         start of range character
  end           end of range character

Returns:        the number of < 256 characters added
                the pointer to extra data is updated
*/

static int
add_to_class(pcre_uint8 *classbits, pcre_uchar **uchardptr, int options,
  compile_data *cd, pcre_uint32 start, pcre_uint32 end)
{
pcre_uint32 c;
pcre_uint32 classbits_end = (end <= 0xff ? end : 0xff);
int n8 = 0;

/* If caseless matching is required, scan the range and process alternate
cases. In Unicode, there are 8-bit characters that have alternate cases that
are greater than 255 and vice-versa. Sometimes we can just extend the original
range. */

if ((options & PCRE_CASELESS) != 0)
  {
#ifdef SUPPORT_UCP
  if ((options & PCRE_UTF8) != 0)
    {
    int rc;
    pcre_uint32 oc, od;

    options &= ~PCRE_CASELESS;   /* Remove for recursive calls */
    c = start;

    while ((rc = get_othercase_range(&c, end, &oc, &od)) >= 0)
      {
      /* Handle a single character that has more than one other case. */

      if (rc > 0) n8 += add_list_to_class(classbits, uchardptr, options, cd,
        PRIV(ucd_caseless_sets) + rc, oc);

      /* Do nothing if the other case range is within the original range. */

      else if (oc >= start && od <= end) continue;

      /* Extend the original range if there is overlap, noting that if oc < c, we
      can't have od > end because a subrange is always shorter than the basic
      range. Otherwise, use a recursive call to add the additional range. */

      else if (oc < start && od >= start - 1) start = oc; /* Extend downwards */
      else if (od > end && oc <= end + 1)
        {
        end = od;       /* Extend upwards */
        if (end > classbits_end) classbits_end = (end <= 0xff ? end : 0xff);
        }
      else n8 += add_to_class(classbits, uchardptr, options, cd, oc, od);
      }
    }
  else
#endif  /* SUPPORT_UCP */

  /* Not UTF-mode, or no UCP */

  for (c = start; c <= classbits_end; c++)
    {
    SETBIT(classbits, cd->fcc[c]);
    n8++;
    }
  }

/* Now handle the original range. Adjust the final value according to the bit
length - this means that the same lists of (e.g.) horizontal spaces can be used
in all cases. */

#if defined COMPILE_PCRE8
#ifdef SUPPORT_UTF
  if ((options & PCRE_UTF8) == 0)
#endif
  if (end > 0xff) end = 0xff;

#elif defined COMPILE_PCRE16
#ifdef SUPPORT_UTF
  if ((options & PCRE_UTF16) == 0)
#endif
  if (end > 0xffff) end = 0xffff;

#endif /* COMPILE_PCRE[8|16] */

/* Use the bitmap for characters < 256. Otherwise use extra data.*/

for (c = start; c <= classbits_end; c++)
  {
  /* Regardless of start, c will always be <= 255. */
  SETBIT(classbits, c);
  n8++;
  }

#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
if (start <= 0xff) start = 0xff + 1;

if (end >= start)
  {
  pcre_uchar *uchardata = *uchardptr;
#ifdef SUPPORT_UTF
  if ((options & PCRE_UTF8) != 0)  /* All UTFs use the same flag bit */
    {
    if (start < end)
      {
      *uchardata++ = XCL_RANGE;
      uchardata += PRIV(ord2utf)(start, uchardata);
      uchardata += PRIV(ord2utf)(end, uchardata);
      }
    else if (start == end)
      {
      *uchardata++ = XCL_SINGLE;
      uchardata += PRIV(ord2utf)(start, uchardata);
      }
    }
  else
#endif  /* SUPPORT_UTF */

  /* Without UTF support, character values are constrained by the bit length,
  and can only be > 256 for 16-bit and 32-bit libraries. */

#ifdef COMPILE_PCRE8
    {}
#else
  if (start < end)
    {
    *uchardata++ = XCL_RANGE;
    *uchardata++ = start;
    *uchardata++ = end;
    }
  else if (start == end)
    {
    *uchardata++ = XCL_SINGLE;
    *uchardata++ = start;
    }
#endif

  *uchardptr = uchardata;   /* Updata extra data pointer */
  }
#endif /* SUPPORT_UTF || !COMPILE_PCRE8 */

return n8;    /* Number of 8-bit characters */
}




/*************************************************
*        Add a list of characters to a class     *
*************************************************/

/* This function is used for adding a list of case-equivalent characters to a
class, and also for adding a list of horizontal or vertical whitespace. If the
list is in order (which it should be), ranges of characters are detected and
handled appropriately. This function is mutually recursive with the function
above.

Arguments:
  classbits     the bit map for characters < 256
  uchardptr     points to the pointer for extra data
  options       the options word
  cd            contains pointers to tables etc.
  p             points to row of 32-bit values, terminated by NOTACHAR
  except        character to omit; this is used when adding lists of
                  case-equivalent characters to avoid including the one we
                  already know about

Returns:        the number of < 256 characters added
                the pointer to extra data is updated
*/

static int
add_list_to_class(pcre_uint8 *classbits, pcre_uchar **uchardptr, int options,
  compile_data *cd, const pcre_uint32 *p, unsigned int except)
{
int n8 = 0;
while (p[0] < NOTACHAR)
  {
  int n = 0;
  if (p[0] != except)
    {
    while(p[n+1] == p[0] + n + 1) n++;
    n8 += add_to_class(classbits, uchardptr, options, cd, p[0], p[n]);
    }
  p += n + 1;
  }
return n8;
}



/*************************************************
*    Add characters not in a list to a class     *
*************************************************/

/* This function is used for adding the complement of a list of horizontal or
vertical whitespace to a class. The list must be in order.

Arguments:
  classbits     the bit map for characters < 256
  uchardptr     points to the pointer for extra data
  options       the options word
  cd            contains pointers to tables etc.
  p             points to row of 32-bit values, terminated by NOTACHAR

Returns:        the number of < 256 characters added
                the pointer to extra data is updated
*/

static int
add_not_list_to_class(pcre_uint8 *classbits, pcre_uchar **uchardptr,
  int options, compile_data *cd, const pcre_uint32 *p)
{
BOOL utf = (options & PCRE_UTF8) != 0;
int n8 = 0;
if (p[0] > 0)
  n8 += add_to_class(classbits, uchardptr, options, cd, 0, p[0] - 1);
while (p[0] < NOTACHAR)
  {
  while (p[1] == p[0] + 1) p++;
  n8 += add_to_class(classbits, uchardptr, options, cd, p[0] + 1,
    (p[1] == NOTACHAR) ? (utf ? 0x10ffffu : 0xffffffffu) : p[1] - 1);
  p++;
  }
return n8;
}



/*************************************************
*           Compile one branch                   *
*************************************************/

/* Scan the pattern, compiling it into the a vector. If the options are
changed during the branch, the pointer is used to change the external options
bits. This function is used during the pre-compile phase when we are trying
to find out the amount of memory needed, as well as during the real compile
phase. The value of lengthptr distinguishes the two phases.

Arguments:
  optionsptr        pointer to the option bits
  codeptr           points to the pointer to the current code point
  ptrptr            points to the current pattern pointer
  errorcodeptr      points to error code variable
  firstcharptr      place to put the first required character
  firstcharflagsptr place to put the first character flags, or a negative number
  reqcharptr        place to put the last required character
  reqcharflagsptr   place to put the last required character flags, or a negative number
  bcptr             points to current branch chain
  cond_depth        conditional nesting depth
  cd                contains pointers to tables etc.
  lengthptr         NULL during the real compile phase
                    points to length accumulator during pre-compile phase

Returns:            TRUE on success
                    FALSE, with *errorcodeptr set non-zero on error
*/

static BOOL
compile_branch(int *optionsptr, pcre_uchar **codeptr,
  const pcre_uchar **ptrptr, int *errorcodeptr,
  pcre_uint32 *firstcharptr, pcre_int32 *firstcharflagsptr,
  pcre_uint32 *reqcharptr, pcre_int32 *reqcharflagsptr,
  branch_chain *bcptr, int cond_depth,
  compile_data *cd, int *lengthptr)
{
int repeat_type, op_type;
int repeat_min = 0, repeat_max = 0;      /* To please picky compilers */
int bravalue = 0;
int greedy_default, greedy_non_default;
pcre_uint32 firstchar, reqchar;
pcre_int32 firstcharflags, reqcharflags;
pcre_uint32 zeroreqchar, zerofirstchar;
pcre_int32 zeroreqcharflags, zerofirstcharflags;
pcre_int32 req_caseopt, reqvary, tempreqvary;
int options = *optionsptr;               /* May change dynamically */
int after_manual_callout = 0;
int length_prevgroup = 0;
register pcre_uint32 c;
int escape;
register pcre_uchar *code = *codeptr;
pcre_uchar *last_code = code;
pcre_uchar *orig_code = code;
pcre_uchar *tempcode;
BOOL inescq = FALSE;
BOOL groupsetfirstchar = FALSE;
const pcre_uchar *ptr = *ptrptr;
const pcre_uchar *tempptr;
const pcre_uchar *nestptr = NULL;
pcre_uchar *previous = NULL;
pcre_uchar *previous_callout = NULL;
size_t item_hwm_offset = 0;
pcre_uint8 classbits[32];

/* We can fish out the UTF-8 setting once and for all into a BOOL, but we
must not do this for other options (e.g. PCRE_EXTENDED) because they may change
dynamically as we process the pattern. */

#ifdef SUPPORT_UTF
/* PCRE_UTF[16|32] have the same value as PCRE_UTF8. */
BOOL utf = (options & PCRE_UTF8) != 0;
#ifndef COMPILE_PCRE32
pcre_uchar utf_chars[6];
#endif
#else
BOOL utf = FALSE;
#endif

/* Helper variables for OP_XCLASS opcode (for characters > 255). We define
class_uchardata always so that it can be passed to add_to_class() always,
though it will not be used in non-UTF 8-bit cases. This avoids having to supply
alternative calls for the different cases. */

pcre_uchar *class_uchardata;
#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
BOOL xclass;
pcre_uchar *class_uchardata_base;
#endif

#ifdef PCRE_DEBUG
if (lengthptr != NULL) DPRINTF((">> start branch\n"));
#endif

/* Set up the default and non-default settings for greediness */

greedy_default = ((options & PCRE_UNGREEDY) != 0);
greedy_non_default = greedy_default ^ 1;

/* Initialize no first byte, no required byte. REQ_UNSET means "no char
matching encountered yet". It gets changed to REQ_NONE if we hit something that
matches a non-fixed char first char; reqchar just remains unset if we never
find one.

When we hit a repeat whose minimum is zero, we may have to adjust these values
to take the zero repeat into account. This is implemented by setting them to
zerofirstbyte and zeroreqchar when such a repeat is encountered. The individual
item types that can be repeated set these backoff variables appropriately. */

firstchar = reqchar = zerofirstchar = zeroreqchar = 0;
firstcharflags = reqcharflags = zerofirstcharflags = zeroreqcharflags = REQ_UNSET;

/* The variable req_caseopt contains either the REQ_CASELESS value
or zero, according to the current setting of the caseless flag. The
REQ_CASELESS leaves the lower 28 bit empty. It is added into the
firstchar or reqchar variables to record the case status of the
value. This is used only for ASCII characters. */

req_caseopt = ((options & PCRE_CASELESS) != 0)? REQ_CASELESS:0;

/* Switch on next character until the end of the branch */

for (;; ptr++)
  {
  BOOL negate_class;
  BOOL should_flip_negation;
  BOOL possessive_quantifier;
  BOOL is_quantifier;
  BOOL is_recurse;
  BOOL reset_bracount;
  int class_has_8bitchar;
  int class_one_char;
#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
  BOOL xclass_has_prop;
#endif
  int newoptions;
  int recno;
  int refsign;
  int skipbytes;
  pcre_uint32 subreqchar, subfirstchar;
  pcre_int32 subreqcharflags, subfirstcharflags;
  int terminator;
  unsigned int mclength;
  unsigned int tempbracount;
  pcre_uint32 ec;
  pcre_uchar mcbuffer[8];

  /* Get next character in the pattern */

  c = *ptr;

  /* If we are at the end of a nested substitution, revert to the outer level
  string. Nesting only happens one level deep. */

  if (c == CHAR_NULL && nestptr != NULL)
    {
    ptr = nestptr;
    nestptr = NULL;
    c = *ptr;
    }

  /* If we are in the pre-compile phase, accumulate the length used for the
  previous cycle of this loop. */

  if (lengthptr != NULL)
    {
#ifdef PCRE_DEBUG
    if (code > cd->hwm) cd->hwm = code;                 /* High water info */
#endif
    if (code > cd->start_workspace + cd->workspace_size -
        WORK_SIZE_SAFETY_MARGIN)                       /* Check for overrun */
      {
      *errorcodeptr = ERR52;
      goto FAILED;
      }

    /* There is at least one situation where code goes backwards: this is the
    case of a zero quantifier after a class (e.g. [ab]{0}). At compile time,
    the class is simply eliminated. However, it is created first, so we have to
    allow memory for it. Therefore, don't ever reduce the length at this point.
    */

    if (code < last_code) code = last_code;

    /* Paranoid check for integer overflow */

    if (OFLOW_MAX - *lengthptr < code - last_code)
      {
      *errorcodeptr = ERR20;
      goto FAILED;
      }

    *lengthptr += (int)(code - last_code);
    DPRINTF(("length=%d added %d c=%c (0x%x)\n", *lengthptr,
      (int)(code - last_code), c, c));

    /* If "previous" is set and it is not at the start of the work space, move
    it back to there, in order to avoid filling up the work space. Otherwise,
    if "previous" is NULL, reset the current code pointer to the start. */

    if (previous != NULL)
      {
      if (previous > orig_code)
        {
        memmove(orig_code, previous, IN_UCHARS(code - previous));
        code -= previous - orig_code;
        previous = orig_code;
        }
      }
    else code = orig_code;

    /* Remember where this code item starts so we can pick up the length
    next time round. */

    last_code = code;
    }

  /* In the real compile phase, just check the workspace used by the forward
  reference list. */

  else if (cd->hwm > cd->start_workspace + cd->workspace_size)
    {
    *errorcodeptr = ERR52;
    goto FAILED;
    }

  /* If in \Q...\E, check for the end; if not, we have a literal */

  if (inescq && c != CHAR_NULL)
    {
    if (c == CHAR_BACKSLASH && ptr[1] == CHAR_E)
      {
      inescq = FALSE;
      ptr++;
      continue;
      }
    else
      {
      if (previous_callout != NULL)
        {
        if (lengthptr == NULL)  /* Don't attempt in pre-compile phase */
          complete_callout(previous_callout, ptr, cd);
        previous_callout = NULL;
        }
      if ((options & PCRE_AUTO_CALLOUT) != 0)
        {
        previous_callout = code;
        code = auto_callout(code, ptr, cd);
        }
      goto NORMAL_CHAR;
      }
    /* Control does not reach here. */
    }

  /* In extended mode, skip white space and comments. We need a loop in order
  to check for more white space and more comments after a comment. */

  if ((options & PCRE_EXTENDED) != 0)
    {
    for (;;)
      {
      while (MAX_255(c) && (cd->ctypes[c] & ctype_space) != 0) c = *(++ptr);
      if (c != CHAR_NUMBER_SIGN) break;
      ptr++;
      while (*ptr != CHAR_NULL)
        {
        if (IS_NEWLINE(ptr))         /* For non-fixed-length newline cases, */
          {                          /* IS_NEWLINE sets cd->nllen. */
          ptr += cd->nllen;
          break;
          }
        ptr++;
#ifdef SUPPORT_UTF
        if (utf) FORWARDCHAR(ptr);
#endif
        }
      c = *ptr;     /* Either NULL or the char after a newline */
      }
    }

  /* See if the next thing is a quantifier. */

  is_quantifier =
    c == CHAR_ASTERISK || c == CHAR_PLUS || c == CHAR_QUESTION_MARK ||
    (c == CHAR_LEFT_CURLY_BRACKET && is_counted_repeat(ptr+1));

  /* Fill in length of a previous callout, except when the next thing is a
  quantifier or when processing a property substitution string in UCP mode. */

  if (!is_quantifier && previous_callout != NULL && nestptr == NULL &&
       after_manual_callout-- <= 0)
    {
    if (lengthptr == NULL)      /* Don't attempt in pre-compile phase */
      complete_callout(previous_callout, ptr, cd);
    previous_callout = NULL;
    }

  /* Create auto callout, except for quantifiers, or while processing property
  strings that are substituted for \w etc in UCP mode. */

  if ((options & PCRE_AUTO_CALLOUT) != 0 && !is_quantifier && nestptr == NULL)
    {
    previous_callout = code;
    code = auto_callout(code, ptr, cd);
    }

  /* Process the next pattern item. */

  switch(c)
    {
    /* ===================================================================*/
    case CHAR_NULL:                /* The branch terminates at string end */
    case CHAR_VERTICAL_LINE:       /* or | or ) */
    case CHAR_RIGHT_PARENTHESIS:
    *firstcharptr = firstchar;
    *firstcharflagsptr = firstcharflags;
    *reqcharptr = reqchar;
    *reqcharflagsptr = reqcharflags;
    *codeptr = code;
    *ptrptr = ptr;
    if (lengthptr != NULL)
      {
      if (OFLOW_MAX - *lengthptr < code - last_code)
        {
        *errorcodeptr = ERR20;
        goto FAILED;
        }
      *lengthptr += (int)(code - last_code);   /* To include callout length */
      DPRINTF((">> end branch\n"));
      }
    return TRUE;


    /* ===================================================================*/
    /* Handle single-character metacharacters. In multiline mode, ^ disables
    the setting of any following char as a first character. */

    case CHAR_CIRCUMFLEX_ACCENT:
    previous = NULL;
    if ((options & PCRE_MULTILINE) != 0)
      {
      if (firstcharflags == REQ_UNSET)
        zerofirstcharflags = firstcharflags = REQ_NONE;
      *code++ = OP_CIRCM;
      }
    else *code++ = OP_CIRC;
    break;

    case CHAR_DOLLAR_SIGN:
    previous = NULL;
    *code++ = ((options & PCRE_MULTILINE) != 0)? OP_DOLLM : OP_DOLL;
    break;

    /* There can never be a first char if '.' is first, whatever happens about
    repeats. The value of reqchar doesn't change either. */

    case CHAR_DOT:
    if (firstcharflags == REQ_UNSET) firstcharflags = REQ_NONE;
    zerofirstchar = firstchar;
    zerofirstcharflags = firstcharflags;
    zeroreqchar = reqchar;
    zeroreqcharflags = reqcharflags;
    previous = code;
    item_hwm_offset = cd->hwm - cd->start_workspace;
    *code++ = ((options & PCRE_DOTALL) != 0)? OP_ALLANY: OP_ANY;
    break;


    /* ===================================================================*/
    /* Character classes. If the included characters are all < 256, we build a
    32-byte bitmap of the permitted characters, except in the special case
    where there is only one such character. For negated classes, we build the
    map as usual, then invert it at the end. However, we use a different opcode
    so that data characters > 255 can be handled correctly.

    If the class contains characters outside the 0-255 range, a different
    opcode is compiled. It may optionally have a bit map for characters < 256,
    but those above are are explicitly listed afterwards. A flag byte tells
    whether the bitmap is present, and whether this is a negated class or not.

    In JavaScript compatibility mode, an isolated ']' causes an error. In
    default (Perl) mode, it is treated as a data character. */

    case CHAR_RIGHT_SQUARE_BRACKET:
    if ((cd->external_options & PCRE_JAVASCRIPT_COMPAT) != 0)
      {
      *errorcodeptr = ERR64;
      goto FAILED;
      }
    goto NORMAL_CHAR;

    /* In another (POSIX) regex library, the ugly syntax [[:<:]] and [[:>:]] is
    used for "start of word" and "end of word". As these are otherwise illegal
    sequences, we don't break anything by recognizing them. They are replaced
    by \b(?=\w) and \b(?<=\w) respectively. Sequences like [a[:<:]] are
    erroneous and are handled by the normal code below. */

    case CHAR_LEFT_SQUARE_BRACKET:
    if (STRNCMP_UC_C8(ptr+1, STRING_WEIRD_STARTWORD, 6) == 0)
      {
      nestptr = ptr + 7;
      ptr = sub_start_of_word - 1;
      continue;
      }

    if (STRNCMP_UC_C8(ptr+1, STRING_WEIRD_ENDWORD, 6) == 0)
      {
      nestptr = ptr + 7;
      ptr = sub_end_of_word - 1;
      continue;
      }

    /* Handle a real character class. */

    previous = code;
    item_hwm_offset = cd->hwm - cd->start_workspace;

    /* PCRE supports POSIX class stuff inside a class. Perl gives an error if
    they are encountered at the top level, so we'll do that too. */

    if ((ptr[1] == CHAR_COLON || ptr[1] == CHAR_DOT ||
         ptr[1] == CHAR_EQUALS_SIGN) &&
        check_posix_syntax(ptr, &tempptr))
      {
      *errorcodeptr = (ptr[1] == CHAR_COLON)? ERR13 : ERR31;
      goto FAILED;
      }

    /* If the first character is '^', set the negation flag and skip it. Also,
    if the first few characters (either before or after ^) are \Q\E or \E we
    skip them too. This makes for compatibility with Perl. */

    negate_class = FALSE;
    for (;;)
      {
      c = *(++ptr);
      if (c == CHAR_BACKSLASH)
        {
        if (ptr[1] == CHAR_E)
          ptr++;
        else if (STRNCMP_UC_C8(ptr + 1, STR_Q STR_BACKSLASH STR_E, 3) == 0)
          ptr += 3;
        else
          break;
        }
      else if (!negate_class && c == CHAR_CIRCUMFLEX_ACCENT)
        negate_class = TRUE;
      else break;
      }

    /* Empty classes are allowed in JavaScript compatibility mode. Otherwise,
    an initial ']' is taken as a data character -- the code below handles
    that. In JS mode, [] must always fail, so generate OP_FAIL, whereas
    [^] must match any character, so generate OP_ALLANY. */

    if (c == CHAR_RIGHT_SQUARE_BRACKET &&
        (cd->external_options & PCRE_JAVASCRIPT_COMPAT) != 0)
      {
      *code++ = negate_class? OP_ALLANY : OP_FAIL;
      if (firstcharflags == REQ_UNSET) firstcharflags = REQ_NONE;
      zerofirstchar = firstchar;
      zerofirstcharflags = firstcharflags;
      break;
      }

    /* If a class contains a negative special such as \S, we need to flip the
    negation flag at the end, so that support for characters > 255 works
    correctly (they are all included in the class). */

    should_flip_negation = FALSE;

    /* Extended class (xclass) will be used when characters > 255
    might match. */

#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
    xclass = FALSE;
    class_uchardata = code + LINK_SIZE + 2;   /* For XCLASS items */
    class_uchardata_base = class_uchardata;   /* Save the start */
#endif

    /* For optimization purposes, we track some properties of the class:
    class_has_8bitchar will be non-zero if the class contains at least one <
    256 character; class_one_char will be 1 if the class contains just one
    character; xclass_has_prop will be TRUE if unicode property checks
    are present in the class. */

    class_has_8bitchar = 0;
    class_one_char = 0;
#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
    xclass_has_prop = FALSE;
#endif

    /* Initialize the 32-char bit map to all zeros. We build the map in a
    temporary bit of memory, in case the class contains fewer than two
    8-bit characters because in that case the compiled code doesn't use the bit
    map. */

    memset(classbits, 0, 32 * sizeof(pcre_uint8));

    /* Process characters until ] is reached. By writing this as a "do" it
    means that an initial ] is taken as a data character. At the start of the
    loop, c contains the first byte of the character. */

    if (c != CHAR_NULL) do
      {
      const pcre_uchar *oldptr;

#ifdef SUPPORT_UTF
      if (utf && HAS_EXTRALEN(c))
        {                           /* Braces are required because the */
        GETCHARLEN(c, ptr, ptr);    /* macro generates multiple statements */
        }
#endif

#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
      /* In the pre-compile phase, accumulate the length of any extra
      data and reset the pointer. This is so that very large classes that
      contain a zillion > 255 characters no longer overwrite the work space
      (which is on the stack). We have to remember that there was XCLASS data,
      however. */

      if (class_uchardata > class_uchardata_base) xclass = TRUE;

      if (lengthptr != NULL && class_uchardata > class_uchardata_base)
        {
        *lengthptr += (int)(class_uchardata - class_uchardata_base);
        class_uchardata = class_uchardata_base;
        }
#endif

      /* Inside \Q...\E everything is literal except \E */

      if (inescq)
        {
        if (c == CHAR_BACKSLASH && ptr[1] == CHAR_E)  /* If we are at \E */
          {
          inescq = FALSE;                   /* Reset literal state */
          ptr++;                            /* Skip the 'E' */
          continue;                         /* Carry on with next */
          }
        goto CHECK_RANGE;                   /* Could be range if \E follows */
        }

      /* Handle POSIX class names. Perl allows a negation extension of the
      form [:^name:]. A square bracket that doesn't match the syntax is
      treated as a literal. We also recognize the POSIX constructions
      [.ch.] and [=ch=] ("collating elements") and fault them, as Perl
      5.6 and 5.8 do. */

      if (c == CHAR_LEFT_SQUARE_BRACKET &&
          (ptr[1] == CHAR_COLON || ptr[1] == CHAR_DOT ||
           ptr[1] == CHAR_EQUALS_SIGN) && check_posix_syntax(ptr, &tempptr))
        {
        BOOL local_negate = FALSE;
        int posix_class, taboffset, tabopt;
        register const pcre_uint8 *cbits = cd->cbits;
        pcre_uint8 pbits[32];

        if (ptr[1] != CHAR_COLON)
          {
          *errorcodeptr = ERR31;
          goto FAILED;
          }

        ptr += 2;
        if (*ptr == CHAR_CIRCUMFLEX_ACCENT)
          {
          local_negate = TRUE;
          should_flip_negation = TRUE;  /* Note negative special */
          ptr++;
          }

        posix_class = check_posix_name(ptr, (int)(tempptr - ptr));
        if (posix_class < 0)
          {
          *errorcodeptr = ERR30;
          goto FAILED;
          }

        /* If matching is caseless, upper and lower are converted to
        alpha. This relies on the fact that the class table starts with
        alpha, lower, upper as the first 3 entries. */

        if ((options & PCRE_CASELESS) != 0 && posix_class <= 2)
          posix_class = 0;

        /* When PCRE_UCP is set, some of the POSIX classes are converted to
        different escape sequences that use Unicode properties \p or \P. Others
        that are not available via \p or \P generate XCL_PROP/XCL_NOTPROP
        directly. */

#ifdef SUPPORT_UCP
        if ((options & PCRE_UCP) != 0)
          {
          unsigned int ptype = 0;
          int pc = posix_class + ((local_negate)? POSIX_SUBSIZE/2 : 0);

          /* The posix_substitutes table specifies which POSIX classes can be
          converted to \p or \P items. */

          if (posix_substitutes[pc] != NULL)
            {
            nestptr = tempptr + 1;
            ptr = posix_substitutes[pc] - 1;
            continue;
            }

          /* There are three other classes that generate special property calls
          that are recognized only in an XCLASS. */

          else switch(posix_class)
            {
            case PC_GRAPH:
            ptype = PT_PXGRAPH;
            /* Fall through */
            case PC_PRINT:
            if (ptype == 0) ptype = PT_PXPRINT;
            /* Fall through */
            case PC_PUNCT:
            if (ptype == 0) ptype = PT_PXPUNCT;
            *class_uchardata++ = local_negate? XCL_NOTPROP : XCL_PROP;
            *class_uchardata++ = ptype;
            *class_uchardata++ = 0;
            xclass_has_prop = TRUE;
            ptr = tempptr + 1;
            continue;

            /* For the other POSIX classes (ascii, xdigit) we are going to fall
            through to the non-UCP case and build a bit map for characters with
            code points less than 256. If we are in a negated POSIX class
            within a non-negated overall class, characters with code points
            greater than 255 must all match. In the special case where we have
            not yet generated any xclass data, and this is the final item in
            the overall class, we need do nothing: later on, the opcode
            OP_NCLASS will be used to indicate that characters greater than 255
            are acceptable. If we have already seen an xclass item or one may
            follow (we have to assume that it might if this is not the end of
            the class), explicitly match all wide codepoints. */

            default:
            if (!negate_class && local_negate &&
                (xclass || tempptr[2] != CHAR_RIGHT_SQUARE_BRACKET))
              {
              *class_uchardata++ = XCL_RANGE;
              class_uchardata += PRIV(ord2utf)(0x100, class_uchardata);
              class_uchardata += PRIV(ord2utf)(0x10ffff, class_uchardata);
              }
            break;
            }
          }
#endif
        /* In the non-UCP case, or when UCP makes no difference, we build the
        bit map for the POSIX class in a chunk of local store because we may be
        adding and subtracting from it, and we don't want to subtract bits that
        may be in the main map already. At the end we or the result into the
        bit map that is being built. */

        posix_class *= 3;

        /* Copy in the first table (always present) */

        memcpy(pbits, cbits + posix_class_maps[posix_class],
          32 * sizeof(pcre_uint8));

        /* If there is a second table, add or remove it as required. */

        taboffset = posix_class_maps[posix_class + 1];
        tabopt = posix_class_maps[posix_class + 2];

        if (taboffset >= 0)
          {
          if (tabopt >= 0)
            for (c = 0; c < 32; c++) pbits[c] |= cbits[c + taboffset];
          else
            for (c = 0; c < 32; c++) pbits[c] &= ~cbits[c + taboffset];
          }

        /* Now see if we need to remove any special characters. An option
        value of 1 removes vertical space and 2 removes underscore. */

        if (tabopt < 0) tabopt = -tabopt;
        if (tabopt == 1) pbits[1] &= ~0x3c;
          else if (tabopt == 2) pbits[11] &= 0x7f;

        /* Add the POSIX table or its complement into the main table that is
        being built and we are done. */

        if (local_negate)
          for (c = 0; c < 32; c++) classbits[c] |= ~pbits[c];
        else
          for (c = 0; c < 32; c++) classbits[c] |= pbits[c];

        ptr = tempptr + 1;
        /* Every class contains at least one < 256 character. */
        class_has_8bitchar = 1;
        /* Every class contains at least two characters. */
        class_one_char = 2;
        continue;    /* End of POSIX syntax handling */
        }

      /* Backslash may introduce a single character, or it may introduce one
      of the specials, which just set a flag. The sequence \b is a special
      case. Inside a class (and only there) it is treated as backspace. We
      assume that other escapes have more than one character in them, so
      speculatively set both class_has_8bitchar and class_one_char bigger
      than one. Unrecognized escapes fall through and are either treated
      as literal characters (by default), or are faulted if
      PCRE_EXTRA is set. */

      if (c == CHAR_BACKSLASH)
        {
        escape = check_escape(&ptr, &ec, errorcodeptr, cd->bracount, options,
          TRUE);
        if (*errorcodeptr != 0) goto FAILED;
        if (escape == 0) c = ec;
        else if (escape == ESC_b) c = CHAR_BS; /* \b is backspace in a class */
        else if (escape == ESC_N)          /* \N is not supported in a class */
          {
          *errorcodeptr = ERR71;
          goto FAILED;
          }
        else if (escape == ESC_Q)            /* Handle start of quoted string */
          {
          if (ptr[1] == CHAR_BACKSLASH && ptr[2] == CHAR_E)
            {
            ptr += 2; /* avoid empty string */
            }
          else inescq = TRUE;
          continue;
          }
        else if (escape == ESC_E) continue;  /* Ignore orphan \E */

        else
          {
          register const pcre_uint8 *cbits = cd->cbits;
          /* Every class contains at least two < 256 characters. */
          class_has_8bitchar++;
          /* Every class contains at least two characters. */
          class_one_char += 2;

          switch (escape)
            {
#ifdef SUPPORT_UCP
            case ESC_du:     /* These are the values given for \d etc */
            case ESC_DU:     /* when PCRE_UCP is set. We replace the */
            case ESC_wu:     /* escape sequence with an appropriate \p */
            case ESC_WU:     /* or \P to test Unicode properties instead */
            case ESC_su:     /* of the default ASCII testing. */
            case ESC_SU:
            nestptr = ptr;
            ptr = substitutes[escape - ESC_DU] - 1;  /* Just before substitute */
            class_has_8bitchar--;                /* Undo! */
            continue;
#endif
            case ESC_d:
            for (c = 0; c < 32; c++) classbits[c] |= cbits[c+cbit_digit];
            continue;

            case ESC_D:
            should_flip_negation = TRUE;
            for (c = 0; c < 32; c++) classbits[c] |= ~cbits[c+cbit_digit];
            continue;

            case ESC_w:
            for (c = 0; c < 32; c++) classbits[c] |= cbits[c+cbit_word];
            continue;

            case ESC_W:
            should_flip_negation = TRUE;
            for (c = 0; c < 32; c++) classbits[c] |= ~cbits[c+cbit_word];
            continue;

            /* Perl 5.004 onwards omitted VT from \s, but restored it at Perl
            5.18. Before PCRE 8.34, we had to preserve the VT bit if it was
            previously set by something earlier in the character class.
            Luckily, the value of CHAR_VT is 0x0b in both ASCII and EBCDIC, so
            we could just adjust the appropriate bit. From PCRE 8.34 we no
            longer treat \s and \S specially. */

            case ESC_s:
            for (c = 0; c < 32; c++) classbits[c] |= cbits[c+cbit_space];
            continue;

            case ESC_S:
            should_flip_negation = TRUE;
            for (c = 0; c < 32; c++) classbits[c] |= ~cbits[c+cbit_space];
            continue;

            /* The rest apply in both UCP and non-UCP cases. */

            case ESC_h:
            (void)add_list_to_class(classbits, &class_uchardata, options, cd,
              PRIV(hspace_list), NOTACHAR);
            continue;

            case ESC_H:
            (void)add_not_list_to_class(classbits, &class_uchardata, options,
              cd, PRIV(hspace_list));
            continue;

            case ESC_v:
            (void)add_list_to_class(classbits, &class_uchardata, options, cd,
              PRIV(vspace_list), NOTACHAR);
            continue;

            case ESC_V:
            (void)add_not_list_to_class(classbits, &class_uchardata, options,
              cd, PRIV(vspace_list));
            continue;

            case ESC_p:
            case ESC_P:
#ifdef SUPPORT_UCP
              {
              BOOL negated;
              unsigned int ptype = 0, pdata = 0;
              if (!get_ucp(&ptr, &negated, &ptype, &pdata, errorcodeptr))
                goto FAILED;
              *class_uchardata++ = ((escape == ESC_p) != negated)?
                XCL_PROP : XCL_NOTPROP;
              *class_uchardata++ = ptype;
              *class_uchardata++ = pdata;
              xclass_has_prop = TRUE;
              class_has_8bitchar--;                /* Undo! */
              continue;
              }
#else
            *errorcodeptr = ERR45;
            goto FAILED;
#endif
            /* Unrecognized escapes are faulted if PCRE is running in its
            strict mode. By default, for compatibility with Perl, they are
            treated as literals. */

            default:
            if ((options & PCRE_EXTRA) != 0)
              {
              *errorcodeptr = ERR7;
              goto FAILED;
              }
            class_has_8bitchar--;    /* Undo the speculative increase. */
            class_one_char -= 2;     /* Undo the speculative increase. */
            c = *ptr;                /* Get the final character and fall through */
            break;
            }
          }

        /* Fall through if the escape just defined a single character (c >= 0).
        This may be greater than 256. */

        escape = 0;

        }   /* End of backslash handling */

      /* A character may be followed by '-' to form a range. However, Perl does
      not permit ']' to be the end of the range. A '-' character at the end is
      treated as a literal. Perl ignores orphaned \E sequences entirely. The
      code for handling \Q and \E is messy. */

      CHECK_RANGE:
      while (ptr[1] == CHAR_BACKSLASH && ptr[2] == CHAR_E)
        {
        inescq = FALSE;
        ptr += 2;
        }
      oldptr = ptr;

      /* Remember if \r or \n were explicitly used */

      if (c == CHAR_CR || c == CHAR_NL) cd->external_flags |= PCRE_HASCRORLF;

      /* Check for range */

      if (!inescq && ptr[1] == CHAR_MINUS)
        {
        pcre_uint32 d;
        ptr += 2;
        while (*ptr == CHAR_BACKSLASH && ptr[1] == CHAR_E) ptr += 2;

        /* If we hit \Q (not followed by \E) at this point, go into escaped
        mode. */

        while (*ptr == CHAR_BACKSLASH && ptr[1] == CHAR_Q)
          {
          ptr += 2;
          if (*ptr == CHAR_BACKSLASH && ptr[1] == CHAR_E)
            { ptr += 2; continue; }
          inescq = TRUE;
          break;
          }

        /* Minus (hyphen) at the end of a class is treated as a literal, so put
        back the pointer and jump to handle the character that preceded it. */

        if (*ptr == CHAR_NULL || (!inescq && *ptr == CHAR_RIGHT_SQUARE_BRACKET))
          {
          ptr = oldptr;
          goto CLASS_SINGLE_CHARACTER;
          }

        /* Otherwise, we have a potential range; pick up the next character */

#ifdef SUPPORT_UTF
        if (utf)
          {                           /* Braces are required because the */
          GETCHARLEN(d, ptr, ptr);    /* macro generates multiple statements */
          }
        else
#endif
        d = *ptr;  /* Not UTF-8 mode */

        /* The second part of a range can be a single-character escape
        sequence, but not any of the other escapes. Perl treats a hyphen as a
        literal in such circumstances. However, in Perl's warning mode, a
        warning is given, so PCRE now faults it as it is almost certainly a
        mistake on the user's part. */

        if (!inescq)
          {
          if (d == CHAR_BACKSLASH)
            {
            int descape;
            descape = check_escape(&ptr, &d, errorcodeptr, cd->bracount, options, TRUE);
            if (*errorcodeptr != 0) goto FAILED;

            /* 0 means a character was put into d; \b is backspace; any other
            special causes an error. */

            if (descape != 0)
              {
              if (descape == ESC_b) d = CHAR_BS; else
                {
                *errorcodeptr = ERR83;
                goto FAILED;
                }
              }
            }

          /* A hyphen followed by a POSIX class is treated in the same way. */

          else if (d == CHAR_LEFT_SQUARE_BRACKET &&
                   (ptr[1] == CHAR_COLON || ptr[1] == CHAR_DOT ||
                    ptr[1] == CHAR_EQUALS_SIGN) &&
                   check_posix_syntax(ptr, &tempptr))
            {
            *errorcodeptr = ERR83;
            goto FAILED;
            }
          }

        /* Check that the two values are in the correct order. Optimize
        one-character ranges. */

        if (d < c)
          {
          *errorcodeptr = ERR8;
          goto FAILED;
          }
        if (d == c) goto CLASS_SINGLE_CHARACTER;  /* A few lines below */

        /* We have found a character range, so single character optimizations
        cannot be done anymore. Any value greater than 1 indicates that there
        is more than one character. */

        class_one_char = 2;

        /* Remember an explicit \r or \n, and add the range to the class. */

        if (d == CHAR_CR || d == CHAR_NL) cd->external_flags |= PCRE_HASCRORLF;

        class_has_8bitchar +=
          add_to_class(classbits, &class_uchardata, options, cd, c, d);

        continue;   /* Go get the next char in the class */
        }

      /* Handle a single character - we can get here for a normal non-escape
      char, or after \ that introduces a single character or for an apparent
      range that isn't. Only the value 1 matters for class_one_char, so don't
      increase it if it is already 2 or more ... just in case there's a class
      with a zillion characters in it. */

      CLASS_SINGLE_CHARACTER:
      if (class_one_char < 2) class_one_char++;

      /* If xclass_has_prop is false and class_one_char is 1, we have the first
      single character in the class, and there have been no prior ranges, or
      XCLASS items generated by escapes. If this is the final character in the
      class, we can optimize by turning the item into a 1-character OP_CHAR[I]
      if it's positive, or OP_NOT[I] if it's negative. In the positive case, it
      can cause firstchar to be set. Otherwise, there can be no first char if
      this item is first, whatever repeat count may follow. In the case of
      reqchar, save the previous value for reinstating. */

      if (!inescq &&
#ifdef SUPPORT_UCP
          !xclass_has_prop &&
#endif
          class_one_char == 1 && ptr[1] == CHAR_RIGHT_SQUARE_BRACKET)
        {
        ptr++;
        zeroreqchar = reqchar;
        zeroreqcharflags = reqcharflags;

        if (negate_class)
          {
#ifdef SUPPORT_UCP
          int d;
#endif
          if (firstcharflags == REQ_UNSET) firstcharflags = REQ_NONE;
          zerofirstchar = firstchar;
          zerofirstcharflags = firstcharflags;

          /* For caseless UTF-8 mode when UCP support is available, check
          whether this character has more than one other case. If so, generate
          a special OP_NOTPROP item instead of OP_NOTI. */

#ifdef SUPPORT_UCP
          if (utf && (options & PCRE_CASELESS) != 0 &&
              (d = UCD_CASESET(c)) != 0)
            {
            *code++ = OP_NOTPROP;
            *code++ = PT_CLIST;
            *code++ = d;
            }
          else
#endif
          /* Char has only one other case, or UCP not available */

            {
            *code++ = ((options & PCRE_CASELESS) != 0)? OP_NOTI: OP_NOT;
#if defined SUPPORT_UTF && !defined COMPILE_PCRE32
            if (utf && c > MAX_VALUE_FOR_SINGLE_CHAR)
              code += PRIV(ord2utf)(c, code);
            else
#endif
              *code++ = c;
            }

          /* We are finished with this character class */

          goto END_CLASS;
          }

        /* For a single, positive character, get the value into mcbuffer, and
        then we can handle this with the normal one-character code. */

#if defined SUPPORT_UTF && !defined COMPILE_PCRE32
        if (utf && c > MAX_VALUE_FOR_SINGLE_CHAR)
          mclength = PRIV(ord2utf)(c, mcbuffer);
        else
#endif
          {
          mcbuffer[0] = c;
          mclength = 1;
          }
        goto ONE_CHAR;
        }       /* End of 1-char optimization */

      /* There is more than one character in the class, or an XCLASS item
      has been generated. Add this character to the class. */

      class_has_8bitchar +=
        add_to_class(classbits, &class_uchardata, options, cd, c, c);
      }

    /* Loop until ']' reached. This "while" is the end of the "do" far above.
    If we are at the end of an internal nested string, revert to the outer
    string. */

    while (((c = *(++ptr)) != CHAR_NULL ||
           (nestptr != NULL &&
             (ptr = nestptr, nestptr = NULL, c = *(++ptr)) != CHAR_NULL)) &&
           (c != CHAR_RIGHT_SQUARE_BRACKET || inescq));

    /* Check for missing terminating ']' */

    if (c == CHAR_NULL)
      {
      *errorcodeptr = ERR6;
      goto FAILED;
      }

    /* We will need an XCLASS if data has been placed in class_uchardata. In
    the second phase this is a sufficient test. However, in the pre-compile
    phase, class_uchardata gets emptied to prevent workspace overflow, so it
    only if the very last character in the class needs XCLASS will it contain
    anything at this point. For this reason, xclass gets set TRUE above when
    uchar_classdata is emptied, and that's why this code is the way it is here
    instead of just doing a test on class_uchardata below. */

#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
    if (class_uchardata > class_uchardata_base) xclass = TRUE;
#endif

    /* If this is the first thing in the branch, there can be no first char
    setting, whatever the repeat count. Any reqchar setting must remain
    unchanged after any kind of repeat. */

    if (firstcharflags == REQ_UNSET) firstcharflags = REQ_NONE;
    zerofirstchar = firstchar;
    zerofirstcharflags = firstcharflags;
    zeroreqchar = reqchar;
    zeroreqcharflags = reqcharflags;

    /* If there are characters with values > 255, we have to compile an
    extended class, with its own opcode, unless there was a negated special
    such as \S in the class, and PCRE_UCP is not set, because in that case all
    characters > 255 are in the class, so any that were explicitly given as
    well can be ignored. If (when there are explicit characters > 255 that must
    be listed) there are no characters < 256, we can omit the bitmap in the
    actual compiled code. */

#ifdef SUPPORT_UTF
    if (xclass && (xclass_has_prop || !should_flip_negation ||
        (options & PCRE_UCP) != 0))
#elif !defined COMPILE_PCRE8
    if (xclass && (xclass_has_prop || !should_flip_negation))
#endif
#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
      {
      *class_uchardata++ = XCL_END;    /* Marks the end of extra data */
      *code++ = OP_XCLASS;
      code += LINK_SIZE;
      *code = negate_class? XCL_NOT:0;
      if (xclass_has_prop) *code |= XCL_HASPROP;

      /* If the map is required, move up the extra data to make room for it;
      otherwise just move the code pointer to the end of the extra data. */

      if (class_has_8bitchar > 0)
        {
        *code++ |= XCL_MAP;
        memmove(code + (32 / sizeof(pcre_uchar)), code,
          IN_UCHARS(class_uchardata - code));
        if (negate_class && !xclass_has_prop)
          for (c = 0; c < 32; c++) classbits[c] = ~classbits[c];
        memcpy(code, classbits, 32);
        code = class_uchardata + (32 / sizeof(pcre_uchar));
        }
      else code = class_uchardata;

      /* Now fill in the complete length of the item */

      PUT(previous, 1, (int)(code - previous));
      break;   /* End of class handling */
      }

    /* Even though any XCLASS list is now discarded, we must allow for
    its memory. */

    if (lengthptr != NULL)
      *lengthptr += (int)(class_uchardata - class_uchardata_base);
#endif

    /* If there are no characters > 255, or they are all to be included or
    excluded, set the opcode to OP_CLASS or OP_NCLASS, depending on whether the
    whole class was negated and whether there were negative specials such as \S
    (non-UCP) in the class. Then copy the 32-byte map into the code vector,
    negating it if necessary. */

    *code++ = (negate_class == should_flip_negation) ? OP_CLASS : OP_NCLASS;
    if (lengthptr == NULL)    /* Save time in the pre-compile phase */
      {
      if (negate_class)
        for (c = 0; c < 32; c++) classbits[c] = ~classbits[c];
      memcpy(code, classbits, 32);
      }
    code += 32 / sizeof(pcre_uchar);

    END_CLASS:
    break;


    /* ===================================================================*/
    /* Various kinds of repeat; '{' is not necessarily a quantifier, but this
    has been tested above. */

    case CHAR_LEFT_CURLY_BRACKET:
    if (!is_quantifier) goto NORMAL_CHAR;
    ptr = read_repeat_counts(ptr+1, &repeat_min, &repeat_max, errorcodeptr);
    if (*errorcodeptr != 0) goto FAILED;
    goto REPEAT;

    case CHAR_ASTERISK:
    repeat_min = 0;
    repeat_max = -1;
    goto REPEAT;

    case CHAR_PLUS:
    repeat_min = 1;
    repeat_max = -1;
    goto REPEAT;

    case CHAR_QUESTION_MARK:
    repeat_min = 0;
    repeat_max = 1;

    REPEAT:
    if (previous == NULL)
      {
      *errorcodeptr = ERR9;
      goto FAILED;
      }

    if (repeat_min == 0)
      {
      firstchar = zerofirstchar;    /* Adjust for zero repeat */
      firstcharflags = zerofirstcharflags;
      reqchar = zeroreqchar;        /* Ditto */
      reqcharflags = zeroreqcharflags;
      }

    /* Remember whether this is a variable length repeat */

    reqvary = (repeat_min == repeat_max)? 0 : REQ_VARY;

    op_type = 0;                    /* Default single-char op codes */
    possessive_quantifier = FALSE;  /* Default not possessive quantifier */

    /* Save start of previous item, in case we have to move it up in order to
    insert something before it. */

    tempcode = previous;

    /* Before checking for a possessive quantifier, we must skip over
    whitespace and comments in extended mode because Perl allows white space at
    this point. */

    if ((options & PCRE_EXTENDED) != 0)
      {
      const pcre_uchar *p = ptr + 1;
      for (;;)
        {
        while (MAX_255(*p) && (cd->ctypes[*p] & ctype_space) != 0) p++;
        if (*p != CHAR_NUMBER_SIGN) break;
        p++;
        while (*p != CHAR_NULL)
          {
          if (IS_NEWLINE(p))         /* For non-fixed-length newline cases, */
            {                        /* IS_NEWLINE sets cd->nllen. */
            p += cd->nllen;
            break;
            }
          p++;
#ifdef SUPPORT_UTF
          if (utf) FORWARDCHAR(p);
#endif
          }           /* Loop for comment characters */
        }             /* Loop for multiple comments */
      ptr = p - 1;    /* Character before the next significant one. */
      }

    /* If the next character is '+', we have a possessive quantifier. This
    implies greediness, whatever the setting of the PCRE_UNGREEDY option.
    If the next character is '?' this is a minimizing repeat, by default,
    but if PCRE_UNGREEDY is set, it works the other way round. We change the
    repeat type to the non-default. */

    if (ptr[1] == CHAR_PLUS)
      {
      repeat_type = 0;                  /* Force greedy */
      possessive_quantifier = TRUE;
      ptr++;
      }
    else if (ptr[1] == CHAR_QUESTION_MARK)
      {
      repeat_type = greedy_non_default;
      ptr++;
      }
    else repeat_type = greedy_default;

    /* If previous was a recursion call, wrap it in atomic brackets so that
    previous becomes the atomic group. All recursions were so wrapped in the
    past, but it no longer happens for non-repeated recursions. In fact, the
    repeated ones could be re-implemented independently so as not to need this,
    but for the moment we rely on the code for repeating groups. */

    if (*previous == OP_RECURSE)
      {
      memmove(previous + 1 + LINK_SIZE, previous, IN_UCHARS(1 + LINK_SIZE));
      *previous = OP_ONCE;
      PUT(previous, 1, 2 + 2*LINK_SIZE);
      previous[2 + 2*LINK_SIZE] = OP_KET;
      PUT(previous, 3 + 2*LINK_SIZE, 2 + 2*LINK_SIZE);
      code += 2 + 2 * LINK_SIZE;
      length_prevgroup = 3 + 3*LINK_SIZE;

      /* When actually compiling, we need to check whether this was a forward
      reference, and if so, adjust the offset. */

      if (lengthptr == NULL && cd->hwm >= cd->start_workspace + LINK_SIZE)
        {
        int offset = GET(cd->hwm, -LINK_SIZE);
        if (offset == previous + 1 - cd->start_code)
          PUT(cd->hwm, -LINK_SIZE, offset + 1 + LINK_SIZE);
        }
      }

    /* Now handle repetition for the different types of item. */

    /* If previous was a character or negated character match, abolish the item
    and generate a repeat item instead. If a char item has a minimum of more
    than one, ensure that it is set in reqchar - it might not be if a sequence
    such as x{3} is the first thing in a branch because the x will have gone
    into firstchar instead.  */

    if (*previous == OP_CHAR || *previous == OP_CHARI
        || *previous == OP_NOT || *previous == OP_NOTI)
      {
      switch (*previous)
        {
        default: /* Make compiler happy. */
        case OP_CHAR:  op_type = OP_STAR - OP_STAR; break;
        case OP_CHARI: op_type = OP_STARI - OP_STAR; break;
        case OP_NOT:   op_type = OP_NOTSTAR - OP_STAR; break;
        case OP_NOTI:  op_type = OP_NOTSTARI - OP_STAR; break;
        }

      /* Deal with UTF characters that take up more than one character. It's
      easier to write this out separately than try to macrify it. Use c to
      hold the length of the character in bytes, plus UTF_LENGTH to flag that
      it's a length rather than a small character. */

#if defined SUPPORT_UTF && !defined COMPILE_PCRE32
      if (utf && NOT_FIRSTCHAR(code[-1]))
        {
        pcre_uchar *lastchar = code - 1;
        BACKCHAR(lastchar);
        c = (int)(code - lastchar);     /* Length of UTF-8 character */
        memcpy(utf_chars, lastchar, IN_UCHARS(c)); /* Save the char */
        c |= UTF_LENGTH;                /* Flag c as a length */
        }
      else
#endif /* SUPPORT_UTF */

      /* Handle the case of a single charater - either with no UTF support, or
      with UTF disabled, or for a single character UTF character. */
        {
        c = code[-1];
        if (*previous <= OP_CHARI && repeat_min > 1)
          {
          reqchar = c;
          reqcharflags = req_caseopt | cd->req_varyopt;
          }
        }

      goto OUTPUT_SINGLE_REPEAT;   /* Code shared with single character types */
      }

    /* If previous was a character type match (\d or similar), abolish it and
    create a suitable repeat item. The code is shared with single-character
    repeats by setting op_type to add a suitable offset into repeat_type. Note
    the the Unicode property types will be present only when SUPPORT_UCP is
    defined, but we don't wrap the little bits of code here because it just
    makes it horribly messy. */

    else if (*previous < OP_EODN)
      {
      pcre_uchar *oldcode;
      int prop_type, prop_value;
      op_type = OP_TYPESTAR - OP_STAR;  /* Use type opcodes */
      c = *previous;

      OUTPUT_SINGLE_REPEAT:
      if (*previous == OP_PROP || *previous == OP_NOTPROP)
        {
        prop_type = previous[1];
        prop_value = previous[2];
        }
      else prop_type = prop_value = -1;

      oldcode = code;
      code = previous;                  /* Usually overwrite previous item */

      /* If the maximum is zero then the minimum must also be zero; Perl allows
      this case, so we do too - by simply omitting the item altogether. */

      if (repeat_max == 0) goto END_REPEAT;

      /* Combine the op_type with the repeat_type */

      repeat_type += op_type;

      /* A minimum of zero is handled either as the special case * or ?, or as
      an UPTO, with the maximum given. */

      if (repeat_min == 0)
        {
        if (repeat_max == -1) *code++ = OP_STAR + repeat_type;
          else if (repeat_max == 1) *code++ = OP_QUERY + repeat_type;
        else
          {
          *code++ = OP_UPTO + repeat_type;
          PUT2INC(code, 0, repeat_max);
          }
        }

      /* A repeat minimum of 1 is optimized into some special cases. If the
      maximum is unlimited, we use OP_PLUS. Otherwise, the original item is
      left in place and, if the maximum is greater than 1, we use OP_UPTO with
      one less than the maximum. */

      else if (repeat_min == 1)
        {
        if (repeat_max == -1)
          *code++ = OP_PLUS + repeat_type;
        else
          {
          code = oldcode;                 /* leave previous item in place */
          if (repeat_max == 1) goto END_REPEAT;
          *code++ = OP_UPTO + repeat_type;
          PUT2INC(code, 0, repeat_max - 1);
          }
        }

      /* The case {n,n} is just an EXACT, while the general case {n,m} is
      handled as an EXACT followed by an UPTO. */

      else
        {
        *code++ = OP_EXACT + op_type;  /* NB EXACT doesn't have repeat_type */
        PUT2INC(code, 0, repeat_min);

        /* If the maximum is unlimited, insert an OP_STAR. Before doing so,
        we have to insert the character for the previous code. For a repeated
        Unicode property match, there are two extra bytes that define the
        required property. In UTF-8 mode, long characters have their length in
        c, with the UTF_LENGTH bit as a flag. */

        if (repeat_max < 0)
          {
#if defined SUPPORT_UTF && !defined COMPILE_PCRE32
          if (utf && (c & UTF_LENGTH) != 0)
            {
            memcpy(code, utf_chars, IN_UCHARS(c & 7));
            code += c & 7;
            }
          else
#endif
            {
            *code++ = c;
            if (prop_type >= 0)
              {
              *code++ = prop_type;
              *code++ = prop_value;
              }
            }
          *code++ = OP_STAR + repeat_type;
          }

        /* Else insert an UPTO if the max is greater than the min, again
        preceded by the character, for the previously inserted code. If the
        UPTO is just for 1 instance, we can use QUERY instead. */

        else if (repeat_max != repeat_min)
          {
#if defined SUPPORT_UTF && !defined COMPILE_PCRE32
          if (utf && (c & UTF_LENGTH) != 0)
            {
            memcpy(code, utf_chars, IN_UCHARS(c & 7));
            code += c & 7;
            }
          else
#endif
          *code++ = c;
          if (prop_type >= 0)
            {
            *code++ = prop_type;
            *code++ = prop_value;
            }
          repeat_max -= repeat_min;

          if (repeat_max == 1)
            {
            *code++ = OP_QUERY + repeat_type;
            }
          else
            {
            *code++ = OP_UPTO + repeat_type;
            PUT2INC(code, 0, repeat_max);
            }
          }
        }

      /* The character or character type itself comes last in all cases. */

#if defined SUPPORT_UTF && !defined COMPILE_PCRE32
      if (utf && (c & UTF_LENGTH) != 0)
        {
        memcpy(code, utf_chars, IN_UCHARS(c & 7));
        code += c & 7;
        }
      else
#endif
      *code++ = c;

      /* For a repeated Unicode property match, there are two extra bytes that
      define the required property. */

#ifdef SUPPORT_UCP
      if (prop_type >= 0)
        {
        *code++ = prop_type;
        *code++ = prop_value;
        }
#endif
      }

    /* If previous was a character class or a back reference, we put the repeat
    stuff after it, but just skip the item if the repeat was {0,0}. */

    else if (*previous == OP_CLASS || *previous == OP_NCLASS ||
#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
             *previous == OP_XCLASS ||
#endif
             *previous == OP_REF   || *previous == OP_REFI ||
             *previous == OP_DNREF || *previous == OP_DNREFI)
      {
      if (repeat_max == 0)
        {
        code = previous;
        goto END_REPEAT;
        }

      if (repeat_min == 0 && repeat_max == -1)
        *code++ = OP_CRSTAR + repeat_type;
      else if (repeat_min == 1 && repeat_max == -1)
        *code++ = OP_CRPLUS + repeat_type;
      else if (repeat_min == 0 && repeat_max == 1)
        *code++ = OP_CRQUERY + repeat_type;
      else
        {
        *code++ = OP_CRRANGE + repeat_type;
        PUT2INC(code, 0, repeat_min);
        if (repeat_max == -1) repeat_max = 0;  /* 2-byte encoding for max */
        PUT2INC(code, 0, repeat_max);
        }
      }

    /* If previous was a bracket group, we may have to replicate it in certain
    cases. Note that at this point we can encounter only the "basic" bracket
    opcodes such as BRA and CBRA, as this is the place where they get converted
    into the more special varieties such as BRAPOS and SBRA. A test for >=
    OP_ASSERT and <= OP_COND includes ASSERT, ASSERT_NOT, ASSERTBACK,
    ASSERTBACK_NOT, ONCE, ONCE_NC, BRA, BRAPOS, CBRA, CBRAPOS, and COND.
    Originally, PCRE did not allow repetition of assertions, but now it does,
    for Perl compatibility. */

    else if (*previous >= OP_ASSERT && *previous <= OP_COND)
      {
      register int i;
      int len = (int)(code - previous);
      size_t base_hwm_offset = item_hwm_offset;
      pcre_uchar *bralink = NULL;
      pcre_uchar *brazeroptr = NULL;

      /* Repeating a DEFINE group is pointless, but Perl allows the syntax, so
      we just ignore the repeat. */

      if (*previous == OP_COND && previous[LINK_SIZE+1] == OP_DEF)
        goto END_REPEAT;

      /* There is no sense in actually repeating assertions. The only potential
      use of repetition is in cases when the assertion is optional. Therefore,
      if the minimum is greater than zero, just ignore the repeat. If the
      maximum is not zero or one, set it to 1. */

      if (*previous < OP_ONCE)    /* Assertion */
        {
        if (repeat_min > 0) goto END_REPEAT;
        if (repeat_max < 0 || repeat_max > 1) repeat_max = 1;
        }

      /* The case of a zero minimum is special because of the need to stick
      OP_BRAZERO in front of it, and because the group appears once in the
      data, whereas in other cases it appears the minimum number of times. For
      this reason, it is simplest to treat this case separately, as otherwise
      the code gets far too messy. There are several special subcases when the
      minimum is zero. */

      if (repeat_min == 0)
        {
        /* If the maximum is also zero, we used to just omit the group from the
        output altogether, like this:

        ** if (repeat_max == 0)
        **   {
        **   code = previous;
        **   goto END_REPEAT;
        **   }

        However, that fails when a group or a subgroup within it is referenced
        as a subroutine from elsewhere in the pattern, so now we stick in
        OP_SKIPZERO in front of it so that it is skipped on execution. As we
        don't have a list of which groups are referenced, we cannot do this
        selectively.

        If the maximum is 1 or unlimited, we just have to stick in the BRAZERO
        and do no more at this point. However, we do need to adjust any
        OP_RECURSE calls inside the group that refer to the group itself or any
        internal or forward referenced group, because the offset is from the
        start of the whole regex. Temporarily terminate the pattern while doing
        this. */

        if (repeat_max <= 1)    /* Covers 0, 1, and unlimited */
          {
          *code = OP_END;
          adjust_recurse(previous, 1, utf, cd, item_hwm_offset);
          memmove(previous + 1, previous, IN_UCHARS(len));
          code++;
          if (repeat_max == 0)
            {
            *previous++ = OP_SKIPZERO;
            goto END_REPEAT;
            }
          brazeroptr = previous;    /* Save for possessive optimizing */
          *previous++ = OP_BRAZERO + repeat_type;
          }

        /* If the maximum is greater than 1 and limited, we have to replicate
        in a nested fashion, sticking OP_BRAZERO before each set of brackets.
        The first one has to be handled carefully because it's the original
        copy, which has to be moved up. The remainder can be handled by code
        that is common with the non-zero minimum case below. We have to
        adjust the value or repeat_max, since one less copy is required. Once
        again, we may have to adjust any OP_RECURSE calls inside the group. */

        else
          {
          int offset;
          *code = OP_END;
          adjust_recurse(previous, 2 + LINK_SIZE, utf, cd, item_hwm_offset);
          memmove(previous + 2 + LINK_SIZE, previous, IN_UCHARS(len));
          code += 2 + LINK_SIZE;
          *previous++ = OP_BRAZERO + repeat_type;
          *previous++ = OP_BRA;

          /* We chain together the bracket offset fields that have to be
          filled in later when the ends of the brackets are reached. */

          offset = (bralink == NULL)? 0 : (int)(previous - bralink);
          bralink = previous;
          PUTINC(previous, 0, offset);
          }

        repeat_max--;
        }

      /* If the minimum is greater than zero, replicate the group as many
      times as necessary, and adjust the maximum to the number of subsequent
      copies that we need. If we set a first char from the group, and didn't
      set a required char, copy the latter from the former. If there are any
      forward reference subroutine calls in the group, there will be entries on
      the workspace list; replicate these with an appropriate increment. */

      else
        {
        if (repeat_min > 1)
          {
          /* In the pre-compile phase, we don't actually do the replication. We
          just adjust the length as if we had. Do some paranoid checks for
          potential integer overflow. The INT64_OR_DOUBLE type is a 64-bit
          integer type when available, otherwise double. */

          if (lengthptr != NULL)
            {
            int delta = (repeat_min - 1)*length_prevgroup;
            if ((INT64_OR_DOUBLE)(repeat_min - 1)*
                  (INT64_OR_DOUBLE)length_prevgroup >
                    (INT64_OR_DOUBLE)INT_MAX ||
                OFLOW_MAX - *lengthptr < delta)
              {
              *errorcodeptr = ERR20;
              goto FAILED;
              }
            *lengthptr += delta;
            }

          /* This is compiling for real. If there is a set first byte for
          the group, and we have not yet set a "required byte", set it. Make
          sure there is enough workspace for copying forward references before
          doing the copy. */

          else
            {
            if (groupsetfirstchar && reqcharflags < 0)
              {
              reqchar = firstchar;
              reqcharflags = firstcharflags;
              }

            for (i = 1; i < repeat_min; i++)
              {
              pcre_uchar *hc;
              size_t this_hwm_offset = cd->hwm - cd->start_workspace;
              memcpy(code, previous, IN_UCHARS(len));

              while (cd->hwm > cd->start_workspace + cd->workspace_size -
                     WORK_SIZE_SAFETY_MARGIN -
                     (this_hwm_offset - base_hwm_offset))
                {
                *errorcodeptr = expand_workspace(cd);
                if (*errorcodeptr != 0) goto FAILED;
                }

              for (hc = (pcre_uchar *)cd->start_workspace + base_hwm_offset;
                   hc < (pcre_uchar *)cd->start_workspace + this_hwm_offset;
                   hc += LINK_SIZE)
                {
                PUT(cd->hwm, 0, GET(hc, 0) + len);
                cd->hwm += LINK_SIZE;
                }
              base_hwm_offset = this_hwm_offset;
              code += len;
              }
            }
          }

        if (repeat_max > 0) repeat_max -= repeat_min;
        }

      /* This code is common to both the zero and non-zero minimum cases. If
      the maximum is limited, it replicates the group in a nested fashion,
      remembering the bracket starts on a stack. In the case of a zero minimum,
      the first one was set up above. In all cases the repeat_max now specifies
      the number of additional copies needed. Again, we must remember to
      replicate entries on the forward reference list. */

      if (repeat_max >= 0)
        {
        /* In the pre-compile phase, we don't actually do the replication. We
        just adjust the length as if we had. For each repetition we must add 1
        to the length for BRAZERO and for all but the last repetition we must
        add 2 + 2*LINKSIZE to allow for the nesting that occurs. Do some
        paranoid checks to avoid integer overflow. The INT64_OR_DOUBLE type is
        a 64-bit integer type when available, otherwise double. */

        if (lengthptr != NULL && repeat_max > 0)
          {
          int delta = repeat_max * (length_prevgroup + 1 + 2 + 2*LINK_SIZE) -
                      2 - 2*LINK_SIZE;   /* Last one doesn't nest */
          if ((INT64_OR_DOUBLE)repeat_max *
                (INT64_OR_DOUBLE)(length_prevgroup + 1 + 2 + 2*LINK_SIZE)
                  > (INT64_OR_DOUBLE)INT_MAX ||
              OFLOW_MAX - *lengthptr < delta)
            {
            *errorcodeptr = ERR20;
            goto FAILED;
            }
          *lengthptr += delta;
          }

        /* This is compiling for real */

        else for (i = repeat_max - 1; i >= 0; i--)
          {
          pcre_uchar *hc;
          size_t this_hwm_offset = cd->hwm - cd->start_workspace;

          *code++ = OP_BRAZERO + repeat_type;

          /* All but the final copy start a new nesting, maintaining the
          chain of brackets outstanding. */

          if (i != 0)
            {
            int offset;
            *code++ = OP_BRA;
            offset = (bralink == NULL)? 0 : (int)(code - bralink);
            bralink = code;
            PUTINC(code, 0, offset);
            }

          memcpy(code, previous, IN_UCHARS(len));

          /* Ensure there is enough workspace for forward references before
          copying them. */

          while (cd->hwm > cd->start_workspace + cd->workspace_size -
                 WORK_SIZE_SAFETY_MARGIN -
                 (this_hwm_offset - base_hwm_offset))
            {
            *errorcodeptr = expand_workspace(cd);
            if (*errorcodeptr != 0) goto FAILED;
            }

          for (hc = (pcre_uchar *)cd->start_workspace + base_hwm_offset;
               hc < (pcre_uchar *)cd->start_workspace + this_hwm_offset;
               hc += LINK_SIZE)
            {
            PUT(cd->hwm, 0, GET(hc, 0) + len + ((i != 0)? 2+LINK_SIZE : 1));
            cd->hwm += LINK_SIZE;
            }
          base_hwm_offset = this_hwm_offset;
          code += len;
          }

        /* Now chain through the pending brackets, and fill in their length
        fields (which are holding the chain links pro tem). */

        while (bralink != NULL)
          {
          int oldlinkoffset;
          int offset = (int)(code - bralink + 1);
          pcre_uchar *bra = code - offset;
          oldlinkoffset = GET(bra, 1);
          bralink = (oldlinkoffset == 0)? NULL : bralink - oldlinkoffset;
          *code++ = OP_KET;
          PUTINC(code, 0, offset);
          PUT(bra, 1, offset);
          }
        }

      /* If the maximum is unlimited, set a repeater in the final copy. For
      ONCE brackets, that's all we need to do. However, possessively repeated
      ONCE brackets can be converted into non-capturing brackets, as the
      behaviour of (?:xx)++ is the same as (?>xx)++ and this saves having to
      deal with possessive ONCEs specially.

      Otherwise, when we are doing the actual compile phase, check to see
      whether this group is one that could match an empty string. If so,
      convert the initial operator to the S form (e.g. OP_BRA -> OP_SBRA) so
      that runtime checking can be done. [This check is also applied to ONCE
      groups at runtime, but in a different way.]

      Then, if the quantifier was possessive and the bracket is not a
      conditional, we convert the BRA code to the POS form, and the KET code to
      KETRPOS. (It turns out to be convenient at runtime to detect this kind of
      subpattern at both the start and at the end.) The use of special opcodes
      makes it possible to reduce greatly the stack usage in pcre_exec(). If
      the group is preceded by OP_BRAZERO, convert this to OP_BRAPOSZERO.

      Then, if the minimum number of matches is 1 or 0, cancel the possessive
      flag so that the default action below, of wrapping everything inside
      atomic brackets, does not happen. When the minimum is greater than 1,
      there will be earlier copies of the group, and so we still have to wrap
      the whole thing. */

      else
        {
        pcre_uchar *ketcode = code - 1 - LINK_SIZE;
        pcre_uchar *bracode = ketcode - GET(ketcode, 1);

        /* Convert possessive ONCE brackets to non-capturing */

        if ((*bracode == OP_ONCE || *bracode == OP_ONCE_NC) &&
            possessive_quantifier) *bracode = OP_BRA;

        /* For non-possessive ONCE brackets, all we need to do is to
        set the KET. */

        if (*bracode == OP_ONCE || *bracode == OP_ONCE_NC)
          *ketcode = OP_KETRMAX + repeat_type;

        /* Handle non-ONCE brackets and possessive ONCEs (which have been
        converted to non-capturing above). */

        else
          {
          /* In the compile phase, check for empty string matching. */

          if (lengthptr == NULL)
            {
            pcre_uchar *scode = bracode;
            do
              {
              if (could_be_empty_branch(scode, ketcode, utf, cd, NULL))
                {
                *bracode += OP_SBRA - OP_BRA;
                break;
                }
              scode += GET(scode, 1);
              }
            while (*scode == OP_ALT);
            }

          /* A conditional group with only one branch has an implicit empty
          alternative branch. */

          if (*bracode == OP_COND && bracode[GET(bracode,1)] != OP_ALT)
            *bracode = OP_SCOND;

          /* Handle possessive quantifiers. */

          if (possessive_quantifier)
            {
            /* For COND brackets, we wrap the whole thing in a possessively
            repeated non-capturing bracket, because we have not invented POS
            versions of the COND opcodes. Because we are moving code along, we
            must ensure that any pending recursive references are updated. */

            if (*bracode == OP_COND || *bracode == OP_SCOND)
              {
              int nlen = (int)(code - bracode);
              *code = OP_END;
              adjust_recurse(bracode, 1 + LINK_SIZE, utf, cd, item_hwm_offset);
              memmove(bracode + 1 + LINK_SIZE, bracode, IN_UCHARS(nlen));
              code += 1 + LINK_SIZE;
              nlen += 1 + LINK_SIZE;
              *bracode = (*bracode == OP_COND)? OP_BRAPOS : OP_SBRAPOS;
              *code++ = OP_KETRPOS;
              PUTINC(code, 0, nlen);
              PUT(bracode, 1, nlen);
              }

            /* For non-COND brackets, we modify the BRA code and use KETRPOS. */

            else
              {
              *bracode += 1;              /* Switch to xxxPOS opcodes */
              *ketcode = OP_KETRPOS;
              }

            /* If the minimum is zero, mark it as possessive, then unset the
            possessive flag when the minimum is 0 or 1. */

            if (brazeroptr != NULL) *brazeroptr = OP_BRAPOSZERO;
            if (repeat_min < 2) possessive_quantifier = FALSE;
            }

          /* Non-possessive quantifier */

          else *ketcode = OP_KETRMAX + repeat_type;
          }
        }
      }

    /* If previous is OP_FAIL, it was generated by an empty class [] in
    JavaScript mode. The other ways in which OP_FAIL can be generated, that is
    by (*FAIL) or (?!) set previous to NULL, which gives a "nothing to repeat"
    error above. We can just ignore the repeat in JS case. */

    else if (*previous == OP_FAIL) goto END_REPEAT;

    /* Else there's some kind of shambles */

    else
      {
      *errorcodeptr = ERR11;
      goto FAILED;
      }

    /* If the character following a repeat is '+', possessive_quantifier is
    TRUE. For some opcodes, there are special alternative opcodes for this
    case. For anything else, we wrap the entire repeated item inside OP_ONCE
    brackets. Logically, the '+' notation is just syntactic sugar, taken from
    Sun's Java package, but the special opcodes can optimize it.

    Some (but not all) possessively repeated subpatterns have already been
    completely handled in the code just above. For them, possessive_quantifier
    is always FALSE at this stage. Note that the repeated item starts at
    tempcode, not at previous, which might be the first part of a string whose
    (former) last char we repeated. */

    if (possessive_quantifier)
      {
      int len;

      /* Possessifying an EXACT quantifier has no effect, so we can ignore it.
      However, QUERY, STAR, or UPTO may follow (for quantifiers such as {5,6},
      {5,}, or {5,10}). We skip over an EXACT item; if the length of what
      remains is greater than zero, there's a further opcode that can be
      handled. If not, do nothing, leaving the EXACT alone. */

      switch(*tempcode)
        {
        case OP_TYPEEXACT:
        tempcode += PRIV(OP_lengths)[*tempcode] +
          ((tempcode[1 + IMM2_SIZE] == OP_PROP
          || tempcode[1 + IMM2_SIZE] == OP_NOTPROP)? 2 : 0);
        break;

        /* CHAR opcodes are used for exacts whose count is 1. */

        case OP_CHAR:
        case OP_CHARI:
        case OP_NOT:
        case OP_NOTI:
        case OP_EXACT:
        case OP_EXACTI:
        case OP_NOTEXACT:
        case OP_NOTEXACTI:
        tempcode += PRIV(OP_lengths)[*tempcode];
#ifdef SUPPORT_UTF
        if (utf && HAS_EXTRALEN(tempcode[-1]))
          tempcode += GET_EXTRALEN(tempcode[-1]);
#endif
        break;

        /* For the class opcodes, the repeat operator appears at the end;
        adjust tempcode to point to it. */

        case OP_CLASS:
        case OP_NCLASS:
        tempcode += 1 + 32/sizeof(pcre_uchar);
        break;

#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
        case OP_XCLASS:
        tempcode += GET(tempcode, 1);
        break;
#endif
        }

      /* If tempcode is equal to code (which points to the end of the repeated
      item), it means we have skipped an EXACT item but there is no following
      QUERY, STAR, or UPTO; the value of len will be 0, and we do nothing. In
      all other cases, tempcode will be pointing to the repeat opcode, and will
      be less than code, so the value of len will be greater than 0. */

      len = (int)(code - tempcode);
      if (len > 0)
        {
        unsigned int repcode = *tempcode;

        /* There is a table for possessifying opcodes, all of which are less
        than OP_CALLOUT. A zero entry means there is no possessified version.
        */

        if (repcode < OP_CALLOUT && opcode_possessify[repcode] > 0)
          *tempcode = opcode_possessify[repcode];

        /* For opcode without a special possessified version, wrap the item in
        ONCE brackets. Because we are moving code along, we must ensure that any
        pending recursive references are updated. */

        else
          {
          *code = OP_END;
          adjust_recurse(tempcode, 1 + LINK_SIZE, utf, cd, item_hwm_offset);
          memmove(tempcode + 1 + LINK_SIZE, tempcode, IN_UCHARS(len));
          code += 1 + LINK_SIZE;
          len += 1 + LINK_SIZE;
          tempcode[0] = OP_ONCE;
          *code++ = OP_KET;
          PUTINC(code, 0, len);
          PUT(tempcode, 1, len);
          }
        }

#ifdef NEVER
      if (len > 0) switch (*tempcode)
        {
        case OP_STAR:  *tempcode = OP_POSSTAR; break;
        case OP_PLUS:  *tempcode = OP_POSPLUS; break;
        case OP_QUERY: *tempcode = OP_POSQUERY; break;
        case OP_UPTO:  *tempcode = OP_POSUPTO; break;

        case OP_STARI:  *tempcode = OP_POSSTARI; break;
        case OP_PLUSI:  *tempcode = OP_POSPLUSI; break;
        case OP_QUERYI: *tempcode = OP_POSQUERYI; break;
        case OP_UPTOI:  *tempcode = OP_POSUPTOI; break;

        case OP_NOTSTAR:  *tempcode = OP_NOTPOSSTAR; break;
        case OP_NOTPLUS:  *tempcode = OP_NOTPOSPLUS; break;
        case OP_NOTQUERY: *tempcode = OP_NOTPOSQUERY; break;
        case OP_NOTUPTO:  *tempcode = OP_NOTPOSUPTO; break;

        case OP_NOTSTARI:  *tempcode = OP_NOTPOSSTARI; break;
        case OP_NOTPLUSI:  *tempcode = OP_NOTPOSPLUSI; break;
        case OP_NOTQUERYI: *tempcode = OP_NOTPOSQUERYI; break;
        case OP_NOTUPTOI:  *tempcode = OP_NOTPOSUPTOI; break;

        case OP_TYPESTAR:  *tempcode = OP_TYPEPOSSTAR; break;
        case OP_TYPEPLUS:  *tempcode = OP_TYPEPOSPLUS; break;
        case OP_TYPEQUERY: *tempcode = OP_TYPEPOSQUERY; break;
        case OP_TYPEUPTO:  *tempcode = OP_TYPEPOSUPTO; break;

        case OP_CRSTAR:   *tempcode = OP_CRPOSSTAR; break;
        case OP_CRPLUS:   *tempcode = OP_CRPOSPLUS; break;
        case OP_CRQUERY:  *tempcode = OP_CRPOSQUERY; break;
        case OP_CRRANGE:  *tempcode = OP_CRPOSRANGE; break;

        /* Because we are moving code along, we must ensure that any
        pending recursive references are updated. */

        default:
        *code = OP_END;
        adjust_recurse(tempcode, 1 + LINK_SIZE, utf, cd, item_hwm_offset);
        memmove(tempcode + 1 + LINK_SIZE, tempcode, IN_UCHARS(len));
        code += 1 + LINK_SIZE;
        len += 1 + LINK_SIZE;
        tempcode[0] = OP_ONCE;
        *code++ = OP_KET;
        PUTINC(code, 0, len);
        PUT(tempcode, 1, len);
        break;
        }
#endif
      }

    /* In all case we no longer have a previous item. We also set the
    "follows varying string" flag for subsequently encountered reqchars if
    it isn't already set and we have just passed a varying length item. */

    END_REPEAT:
    previous = NULL;
    cd->req_varyopt |= reqvary;
    break;


    /* ===================================================================*/
    /* Start of nested parenthesized sub-expression, or comment or lookahead or
    lookbehind or option setting or condition or all the other extended
    parenthesis forms.  */

    case CHAR_LEFT_PARENTHESIS:
    ptr++;

    /* First deal with comments. Putting this code right at the start ensures
    that comments have no bad side effects. */

    if (ptr[0] == CHAR_QUESTION_MARK && ptr[1] == CHAR_NUMBER_SIGN)
      {
      ptr += 2;
      while (*ptr != CHAR_NULL && *ptr != CHAR_RIGHT_PARENTHESIS) ptr++;
      if (*ptr == CHAR_NULL)
        {
        *errorcodeptr = ERR18;
        goto FAILED;
        }
      continue;
      }

    /* Now deal with various "verbs" that can be introduced by '*'. */

    if (ptr[0] == CHAR_ASTERISK && (ptr[1] == ':'
         || (MAX_255(ptr[1]) && ((cd->ctypes[ptr[1]] & ctype_letter) != 0))))
      {
      int i, namelen;
      int arglen = 0;
      const char *vn = verbnames;
      const pcre_uchar *name = ptr + 1;
      const pcre_uchar *arg = NULL;
      previous = NULL;
      ptr++;
      while (MAX_255(*ptr) && (cd->ctypes[*ptr] & ctype_letter) != 0) ptr++;
      namelen = (int)(ptr - name);

      /* It appears that Perl allows any characters whatsoever, other than
      a closing parenthesis, to appear in arguments, so we no longer insist on
      letters, digits, and underscores. */

      if (*ptr == CHAR_COLON)
        {
        arg = ++ptr;
        while (*ptr != CHAR_NULL && *ptr != CHAR_RIGHT_PARENTHESIS) ptr++;
        arglen = (int)(ptr - arg);
        if ((unsigned int)arglen > MAX_MARK)
          {
          *errorcodeptr = ERR75;
          goto FAILED;
          }
        }

      if (*ptr != CHAR_RIGHT_PARENTHESIS)
        {
        *errorcodeptr = ERR60;
        goto FAILED;
        }

      /* Scan the table of verb names */

      for (i = 0; i < verbcount; i++)
        {
        if (namelen == verbs[i].len &&
            STRNCMP_UC_C8(name, vn, namelen) == 0)
          {
          int setverb;

          /* Check for open captures before ACCEPT and convert it to
          ASSERT_ACCEPT if in an assertion. */

          if (verbs[i].op == OP_ACCEPT)
            {
            open_capitem *oc;
            if (arglen != 0)
              {
              *errorcodeptr = ERR59;
              goto FAILED;
              }
            cd->had_accept = TRUE;
            for (oc = cd->open_caps; oc != NULL; oc = oc->next)
              {
              *code++ = OP_CLOSE;
              PUT2INC(code, 0, oc->number);
              }
            setverb = *code++ =
              (cd->assert_depth > 0)? OP_ASSERT_ACCEPT : OP_ACCEPT;

            /* Do not set firstchar after *ACCEPT */
            if (firstcharflags == REQ_UNSET) firstcharflags = REQ_NONE;
            }

          /* Handle other cases with/without an argument */

          else if (arglen == 0)
            {
            if (verbs[i].op < 0)   /* Argument is mandatory */
              {
              *errorcodeptr = ERR66;
              goto FAILED;
              }
            setverb = *code++ = verbs[i].op;
            }

          else
            {
            if (verbs[i].op_arg < 0)   /* Argument is forbidden */
              {
              *errorcodeptr = ERR59;
              goto FAILED;
              }
            setverb = *code++ = verbs[i].op_arg;
            if (lengthptr != NULL)    /* In pass 1 just add in the length */
              {                       /* to avoid potential workspace */
              *lengthptr += arglen;   /* overflow. */
              *code++ = 0;
              }
            else
              {
              *code++ = arglen;
              memcpy(code, arg, IN_UCHARS(arglen));
              code += arglen;
              }
            *code++ = 0;
            }

          switch (setverb)
            {
            case OP_THEN:
            case OP_THEN_ARG:
            cd->external_flags |= PCRE_HASTHEN;
            break;

            case OP_PRUNE:
            case OP_PRUNE_ARG:
            case OP_SKIP:
            case OP_SKIP_ARG:
            cd->had_pruneorskip = TRUE;
            break;
            }

          break;  /* Found verb, exit loop */
          }

        vn += verbs[i].len + 1;
        }

      if (i < verbcount) continue;    /* Successfully handled a verb */
      *errorcodeptr = ERR60;          /* Verb not recognized */
      goto FAILED;
      }

    /* Initialize for "real" parentheses */

    newoptions = options;
    skipbytes = 0;
    bravalue = OP_CBRA;
    item_hwm_offset = cd->hwm - cd->start_workspace;
    reset_bracount = FALSE;

    /* Deal with the extended parentheses; all are introduced by '?', and the
    appearance of any of them means that this is not a capturing group. */

    if (*ptr == CHAR_QUESTION_MARK)
      {
      int i, set, unset, namelen;
      int *optset;
      const pcre_uchar *name;
      pcre_uchar *slot;

      switch (*(++ptr))
        {
        /* ------------------------------------------------------------ */
        case CHAR_VERTICAL_LINE:  /* Reset capture count for each branch */
        reset_bracount = TRUE;
        cd->dupgroups = TRUE;     /* Record (?| encountered */
        /* Fall through */

        /* ------------------------------------------------------------ */
        case CHAR_COLON:          /* Non-capturing bracket */
        bravalue = OP_BRA;
        ptr++;
        break;


        /* ------------------------------------------------------------ */
        case CHAR_LEFT_PARENTHESIS:
        bravalue = OP_COND;       /* Conditional group */
        tempptr = ptr;

        /* A condition can be an assertion, a number (referring to a numbered
        group's having been set), a name (referring to a named group), or 'R',
        referring to recursion. R<digits> and R&name are also permitted for
        recursion tests.

        There are ways of testing a named group: (?(name)) is used by Python;
        Perl 5.10 onwards uses (?(<name>) or (?('name')).

        There is one unfortunate ambiguity, caused by history. 'R' can be the
        recursive thing or the name 'R' (and similarly for 'R' followed by
        digits). We look for a name first; if not found, we try the other case.

        For compatibility with auto-callouts, we allow a callout to be
        specified before a condition that is an assertion. First, check for the
        syntax of a callout; if found, adjust the temporary pointer that is
        used to check for an assertion condition. That's all that is needed! */

        if (ptr[1] == CHAR_QUESTION_MARK && ptr[2] == CHAR_C)
          {
          for (i = 3;; i++) if (!IS_DIGIT(ptr[i])) break;
          if (ptr[i] == CHAR_RIGHT_PARENTHESIS)
            tempptr += i + 1;
          }

        /* For conditions that are assertions, check the syntax, and then exit
        the switch. This will take control down to where bracketed groups,
        including assertions, are processed. */

        if (tempptr[1] == CHAR_QUESTION_MARK &&
              (tempptr[2] == CHAR_EQUALS_SIGN ||
               tempptr[2] == CHAR_EXCLAMATION_MARK ||
                 (tempptr[2] == CHAR_LESS_THAN_SIGN &&
                   (tempptr[3] == CHAR_EQUALS_SIGN ||
                    tempptr[3] == CHAR_EXCLAMATION_MARK))))
          {
          cd->iscondassert = TRUE;
          break;
          }

        /* Other conditions use OP_CREF/OP_DNCREF/OP_RREF/OP_DNRREF, and all
        need to skip at least 1+IMM2_SIZE bytes at the start of the group. */

        code[1+LINK_SIZE] = OP_CREF;
        skipbytes = 1+IMM2_SIZE;
        refsign = -1;     /* => not a number */
        namelen = -1;     /* => not a name; must set to avoid warning */
        name = NULL;      /* Always set to avoid warning */
        recno = 0;        /* Always set to avoid warning */

        /* Check for a test for recursion in a named group. */

        ptr++;
        if (*ptr == CHAR_R && ptr[1] == CHAR_AMPERSAND)
          {
          terminator = -1;
          ptr += 2;
          code[1+LINK_SIZE] = OP_RREF;    /* Change the type of test */
          }

        /* Check for a test for a named group's having been set, using the Perl
        syntax (?(<name>) or (?('name'), and also allow for the original PCRE
        syntax of (?(name) or for (?(+n), (?(-n), and just (?(n). */

        else if (*ptr == CHAR_LESS_THAN_SIGN)
          {
          terminator = CHAR_GREATER_THAN_SIGN;
          ptr++;
          }
        else if (*ptr == CHAR_APOSTROPHE)
          {
          terminator = CHAR_APOSTROPHE;
          ptr++;
          }
        else
          {
          terminator = CHAR_NULL;
          if (*ptr == CHAR_MINUS || *ptr == CHAR_PLUS) refsign = *ptr++;
            else if (IS_DIGIT(*ptr)) refsign = 0;
          }

        /* Handle a number */

        if (refsign >= 0)
          {
          while (IS_DIGIT(*ptr))
            {
            if (recno > INT_MAX / 10 - 1)  /* Integer overflow */
              {
              while (IS_DIGIT(*ptr)) ptr++;
              *errorcodeptr = ERR61;
              goto FAILED;
              }
            recno = recno * 10 + (int)(*ptr - CHAR_0);
            ptr++;
            }
          }

        /* Otherwise we expect to read a name; anything else is an error. When
        a name is one of a number of duplicates, a different opcode is used and
        it needs more memory. Unfortunately we cannot tell whether a name is a
        duplicate in the first pass, so we have to allow for more memory. */

        else
          {
          if (IS_DIGIT(*ptr))
            {
            *errorcodeptr = ERR84;
            goto FAILED;
            }
          if (!MAX_255(*ptr) || (cd->ctypes[*ptr] & ctype_word) == 0)
            {
            *errorcodeptr = ERR28;   /* Assertion expected */
            goto FAILED;
            }
          name = ptr++;
          while (MAX_255(*ptr) && (cd->ctypes[*ptr] & ctype_word) != 0)
            {
            ptr++;
            }
          namelen = (int)(ptr - name);
          if (lengthptr != NULL) skipbytes += IMM2_SIZE;
          }

        /* Check the terminator */

        if ((terminator > 0 && *ptr++ != (pcre_uchar)terminator) ||
            *ptr++ != CHAR_RIGHT_PARENTHESIS)
          {
          ptr--;                  /* Error offset */
          *errorcodeptr = ERR26;  /* Malformed number or name */
          goto FAILED;
          }

        /* Do no further checking in the pre-compile phase. */

        if (lengthptr != NULL) break;

        /* In the real compile we do the work of looking for the actual
        reference. If refsign is not negative, it means we have a number in
        recno. */

        if (refsign >= 0)
          {
          if (recno <= 0)
            {
            *errorcodeptr = ERR35;
            goto FAILED;
            }
          if (refsign != 0) recno = (refsign == CHAR_MINUS)?
            cd->bracount - recno + 1 : recno + cd->bracount;
          if (recno <= 0 || recno > cd->final_bracount)
            {
            *errorcodeptr = ERR15;
            goto FAILED;
            }
          PUT2(code, 2+LINK_SIZE, recno);
          if (recno > cd->top_backref) cd->top_backref = recno;
          break;
          }

        /* Otherwise look for the name. */

        slot = cd->name_table;
        for (i = 0; i < cd->names_found; i++)
          {
          if (STRNCMP_UC_UC(name, slot+IMM2_SIZE, namelen) == 0) break;
          slot += cd->name_entry_size;
          }

        /* Found the named subpattern. If the name is duplicated, add one to
        the opcode to change CREF/RREF into DNCREF/DNRREF and insert
        appropriate data values. Otherwise, just insert the unique subpattern
        number. */

        if (i < cd->names_found)
          {
          int offset = i++;
          int count = 1;
          recno = GET2(slot, 0);   /* Number from first found */
          if (recno > cd->top_backref) cd->top_backref = recno;
          for (; i < cd->names_found; i++)
            {
            slot += cd->name_entry_size;
            if (STRNCMP_UC_UC(name, slot+IMM2_SIZE, namelen) != 0 ||
              (slot+IMM2_SIZE)[namelen] != 0) break;
            count++;
            }

          if (count > 1)
            {
            PUT2(code, 2+LINK_SIZE, offset);
            PUT2(code, 2+LINK_SIZE+IMM2_SIZE, count);
            skipbytes += IMM2_SIZE;
            code[1+LINK_SIZE]++;
            }
          else  /* Not a duplicated name */
            {
            PUT2(code, 2+LINK_SIZE, recno);
            }
          }

        /* If terminator == CHAR_NULL it means that the name followed directly
        after the opening parenthesis [e.g. (?(abc)...] and in this case there
        are some further alternatives to try. For the cases where terminator !=
        CHAR_NULL [things like (?(<name>... or (?('name')... or (?(R&name)... ]
        we have now checked all the possibilities, so give an error. */

        else if (terminator != CHAR_NULL)
          {
          *errorcodeptr = ERR15;
          goto FAILED;
          }

        /* Check for (?(R) for recursion. Allow digits after R to specify a
        specific group number. */

        else if (*name == CHAR_R)
          {
          recno = 0;
          for (i = 1; i < namelen; i++)
            {
            if (!IS_DIGIT(name[i]))
              {
              *errorcodeptr = ERR15;
              goto FAILED;
              }
            if (recno > INT_MAX / 10 - 1)   /* Integer overflow */
              {
              *errorcodeptr = ERR61;
              goto FAILED;
              }
            recno = recno * 10 + name[i] - CHAR_0;
            }
          if (recno == 0) recno = RREF_ANY;
          code[1+LINK_SIZE] = OP_RREF;      /* Change test type */
          PUT2(code, 2+LINK_SIZE, recno);
          }

        /* Similarly, check for the (?(DEFINE) "condition", which is always
        false. */

        else if (namelen == 6 && STRNCMP_UC_C8(name, STRING_DEFINE, 6) == 0)
          {
          code[1+LINK_SIZE] = OP_DEF;
          skipbytes = 1;
          }

        /* Reference to an unidentified subpattern. */

        else
          {
          *errorcodeptr = ERR15;
          goto FAILED;
          }
        break;


        /* ------------------------------------------------------------ */
        case CHAR_EQUALS_SIGN:                 /* Positive lookahead */
        bravalue = OP_ASSERT;
        cd->assert_depth += 1;
        ptr++;
        break;

        /* Optimize (?!) to (*FAIL) unless it is quantified - which is a weird
        thing to do, but Perl allows all assertions to be quantified, and when
        they contain capturing parentheses there may be a potential use for
        this feature. Not that that applies to a quantified (?!) but we allow
        it for uniformity. */

        /* ------------------------------------------------------------ */
        case CHAR_EXCLAMATION_MARK:            /* Negative lookahead */
        ptr++;
        if (*ptr == CHAR_RIGHT_PARENTHESIS && ptr[1] != CHAR_ASTERISK &&
             ptr[1] != CHAR_PLUS && ptr[1] != CHAR_QUESTION_MARK &&
            (ptr[1] != CHAR_LEFT_CURLY_BRACKET || !is_counted_repeat(ptr+2)))
          {
          *code++ = OP_FAIL;
          previous = NULL;
          continue;
          }
        bravalue = OP_ASSERT_NOT;
        cd->assert_depth += 1;
        break;


        /* ------------------------------------------------------------ */
        case CHAR_LESS_THAN_SIGN:              /* Lookbehind or named define */
        switch (ptr[1])
          {
          case CHAR_EQUALS_SIGN:               /* Positive lookbehind */
          bravalue = OP_ASSERTBACK;
          cd->assert_depth += 1;
          ptr += 2;
          break;

          case CHAR_EXCLAMATION_MARK:          /* Negative lookbehind */
          bravalue = OP_ASSERTBACK_NOT;
          cd->assert_depth += 1;
          ptr += 2;
          break;

          default:                /* Could be name define, else bad */
          if (MAX_255(ptr[1]) && (cd->ctypes[ptr[1]] & ctype_word) != 0)
            goto DEFINE_NAME;
          ptr++;                  /* Correct offset for error */
          *errorcodeptr = ERR24;
          goto FAILED;
          }
        break;


        /* ------------------------------------------------------------ */
        case CHAR_GREATER_THAN_SIGN:           /* One-time brackets */
        bravalue = OP_ONCE;
        ptr++;
        break;


        /* ------------------------------------------------------------ */
        case CHAR_C:                 /* Callout - may be followed by digits; */
        previous_callout = code;     /* Save for later completion */
        after_manual_callout = 1;    /* Skip one item before completing */
        *code++ = OP_CALLOUT;
          {
          int n = 0;
          ptr++;
          while(IS_DIGIT(*ptr))
            n = n * 10 + *ptr++ - CHAR_0;
          if (*ptr != CHAR_RIGHT_PARENTHESIS)
            {
            *errorcodeptr = ERR39;
            goto FAILED;
            }
          if (n > 255)
            {
            *errorcodeptr = ERR38;
            goto FAILED;
            }
          *code++ = n;
          PUT(code, 0, (int)(ptr - cd->start_pattern + 1)); /* Pattern offset */
          PUT(code, LINK_SIZE, 0);                          /* Default length */
          code += 2 * LINK_SIZE;
          }
        previous = NULL;
        continue;


        /* ------------------------------------------------------------ */
        case CHAR_P:              /* Python-style named subpattern handling */
        if (*(++ptr) == CHAR_EQUALS_SIGN ||
            *ptr == CHAR_GREATER_THAN_SIGN)  /* Reference or recursion */
          {
          is_recurse = *ptr == CHAR_GREATER_THAN_SIGN;
          terminator = CHAR_RIGHT_PARENTHESIS;
          goto NAMED_REF_OR_RECURSE;
          }
        else if (*ptr != CHAR_LESS_THAN_SIGN)  /* Test for Python-style defn */
          {
          *errorcodeptr = ERR41;
          goto FAILED;
          }
        /* Fall through to handle (?P< as (?< is handled */


        /* ------------------------------------------------------------ */
        DEFINE_NAME:    /* Come here from (?< handling */
        case CHAR_APOSTROPHE:
        terminator = (*ptr == CHAR_LESS_THAN_SIGN)?
          CHAR_GREATER_THAN_SIGN : CHAR_APOSTROPHE;
        name = ++ptr;
        if (IS_DIGIT(*ptr))
          {
          *errorcodeptr = ERR84;   /* Group name must start with non-digit */
          goto FAILED;
          }
        while (MAX_255(*ptr) && (cd->ctypes[*ptr] & ctype_word) != 0) ptr++;
        namelen = (int)(ptr - name);

        /* In the pre-compile phase, do a syntax check, remember the longest
        name, and then remember the group in a vector, expanding it if
        necessary. Duplicates for the same number are skipped; other duplicates
        are checked for validity. In the actual compile, there is nothing to
        do. */

        if (lengthptr != NULL)
          {
          named_group *ng;
          pcre_uint32 number = cd->bracount + 1;

          if (*ptr != (pcre_uchar)terminator)
            {
            *errorcodeptr = ERR42;
            goto FAILED;
            }

          if (cd->names_found >= MAX_NAME_COUNT)
            {
            *errorcodeptr = ERR49;
            goto FAILED;
            }

          if (namelen + IMM2_SIZE + 1 > cd->name_entry_size)
            {
            cd->name_entry_size = namelen + IMM2_SIZE + 1;
            if (namelen > MAX_NAME_SIZE)
              {
              *errorcodeptr = ERR48;
              goto FAILED;
              }
            }

          /* Scan the list to check for duplicates. For duplicate names, if the
          number is the same, break the loop, which causes the name to be
          discarded; otherwise, if DUPNAMES is not set, give an error.
          If it is set, allow the name with a different number, but continue
          scanning in case this is a duplicate with the same number. For
          non-duplicate names, give an error if the number is duplicated. */

          ng = cd->named_groups;
          for (i = 0; i < cd->names_found; i++, ng++)
            {
            if (namelen == ng->length &&
                STRNCMP_UC_UC(name, ng->name, namelen) == 0)
              {
              if (ng->number == number) break;
              if ((options & PCRE_DUPNAMES) == 0)
                {
                *errorcodeptr = ERR43;
                goto FAILED;
                }
              cd->dupnames = TRUE;  /* Duplicate names exist */
              }
            else if (ng->number == number)
              {
              *errorcodeptr = ERR65;
              goto FAILED;
              }
            }

          if (i >= cd->names_found)     /* Not a duplicate with same number */
            {
            /* Increase the list size if necessary */

            if (cd->names_found >= cd->named_group_list_size)
              {
              int newsize = cd->named_group_list_size * 2;
              named_group *newspace = (PUBL(malloc))
                (newsize * sizeof(named_group));

              if (newspace == NULL)
                {
                *errorcodeptr = ERR21;
                goto FAILED;
                }

              memcpy(newspace, cd->named_groups,
                cd->named_group_list_size * sizeof(named_group));
              if (cd->named_group_list_size > NAMED_GROUP_LIST_SIZE)
                (PUBL(free))((void *)cd->named_groups);
              cd->named_groups = newspace;
              cd->named_group_list_size = newsize;
              }

            cd->named_groups[cd->names_found].name = name;
            cd->named_groups[cd->names_found].length = namelen;
            cd->named_groups[cd->names_found].number = number;
            cd->names_found++;
            }
          }

        ptr++;                    /* Move past > or ' in both passes. */
        goto NUMBERED_GROUP;


        /* ------------------------------------------------------------ */
        case CHAR_AMPERSAND:            /* Perl recursion/subroutine syntax */
        terminator = CHAR_RIGHT_PARENTHESIS;
        is_recurse = TRUE;
        /* Fall through */

        /* We come here from the Python syntax above that handles both
        references (?P=name) and recursion (?P>name), as well as falling
        through from the Perl recursion syntax (?&name). We also come here from
        the Perl \k<name> or \k'name' back reference syntax and the \k{name}
        .NET syntax, and the Oniguruma \g<...> and \g'...' subroutine syntax. */

        NAMED_REF_OR_RECURSE:
        name = ++ptr;
        if (IS_DIGIT(*ptr))
          {
          *errorcodeptr = ERR84;   /* Group name must start with non-digit */
          goto FAILED;
          }
        while (MAX_255(*ptr) && (cd->ctypes[*ptr] & ctype_word) != 0) ptr++;
        namelen = (int)(ptr - name);

        /* In the pre-compile phase, do a syntax check. We used to just set
        a dummy reference number, because it was not used in the first pass.
        However, with the change of recursive back references to be atomic,
        we have to look for the number so that this state can be identified, as
        otherwise the incorrect length is computed. If it's not a backwards
        reference, the dummy number will do. */

        if (lengthptr != NULL)
          {
          named_group *ng;
          recno = 0;

          if (namelen == 0)
            {
            *errorcodeptr = ERR62;
            goto FAILED;
            }
          if (*ptr != (pcre_uchar)terminator)
            {
            *errorcodeptr = ERR42;
            goto FAILED;
            }
          if (namelen > MAX_NAME_SIZE)
            {
            *errorcodeptr = ERR48;
            goto FAILED;
            }

          /* Count named back references. */

          if (!is_recurse) cd->namedrefcount++;

          /* We have to allow for a named reference to a duplicated name (this
          cannot be determined until the second pass). This needs an extra
          16-bit data item. */

          *lengthptr += IMM2_SIZE;

          /* If this is a forward reference and we are within a (?|...) group,
          the reference may end up as the number of a group which we are
          currently inside, that is, it could be a recursive reference. In the
          real compile this will be picked up and the reference wrapped with
          OP_ONCE to make it atomic, so we must space in case this occurs. */

          /* In fact, this can happen for a non-forward reference because
          another group with the same number might be created later. This
          issue is fixed "properly" in PCRE2. As PCRE1 is now in maintenance
          only mode, we finesse the bug by allowing more memory always. */

          *lengthptr += 2 + 2*LINK_SIZE;

          /* It is even worse than that. The current reference may be to an
          existing named group with a different number (so apparently not
          recursive) but which later on is also attached to a group with the
          current number. This can only happen if $(| has been previous
          encountered. In that case, we allow yet more memory, just in case.
          (Again, this is fixed "properly" in PCRE2. */

          if (cd->dupgroups) *lengthptr += 4 + 4*LINK_SIZE;

          /* Otherwise, check for recursion here. The name table does not exist
          in the first pass; instead we must scan the list of names encountered
          so far in order to get the number. If the name is not found, leave
          the value of recno as 0 for a forward reference. */

          else
            {
            ng = cd->named_groups;
            for (i = 0; i < cd->names_found; i++, ng++)
              {
              if (namelen == ng->length &&
                  STRNCMP_UC_UC(name, ng->name, namelen) == 0)
                {
                open_capitem *oc;
                recno = ng->number;
                if (is_recurse) break;
                for (oc = cd->open_caps; oc != NULL; oc = oc->next)
                  {
                  if (oc->number == recno)
                    {
                    oc->flag = TRUE;
                    break;
                    }
                  }
                }
              }
            }
          }

        /* In the real compile, search the name table. We check the name
        first, and then check that we have reached the end of the name in the
        table. That way, if the name is longer than any in the table, the
        comparison will fail without reading beyond the table entry. */

        else
          {
          slot = cd->name_table;
          for (i = 0; i < cd->names_found; i++)
            {
            if (STRNCMP_UC_UC(name, slot+IMM2_SIZE, namelen) == 0 &&
                slot[IMM2_SIZE+namelen] == 0)
              break;
            slot += cd->name_entry_size;
            }

          if (i < cd->names_found)
            {
            recno = GET2(slot, 0);
            }
          else
            {
            *errorcodeptr = ERR15;
            goto FAILED;
            }
          }

        /* In both phases, for recursions, we can now go to the code than
        handles numerical recursion. */

        if (is_recurse) goto HANDLE_RECURSION;

        /* In the second pass we must see if the name is duplicated. If so, we
        generate a different opcode. */

        if (lengthptr == NULL && cd->dupnames)
          {
          int count = 1;
          unsigned int index = i;
          pcre_uchar *cslot = slot + cd->name_entry_size;

          for (i++; i < cd->names_found; i++)
            {
            if (STRCMP_UC_UC(slot + IMM2_SIZE, cslot + IMM2_SIZE) != 0) break;
            count++;
            cslot += cd->name_entry_size;
            }

          if (count > 1)
            {
            if (firstcharflags == REQ_UNSET) firstcharflags = REQ_NONE;
            previous = code;
            item_hwm_offset = cd->hwm - cd->start_workspace;
            *code++ = ((options & PCRE_CASELESS) != 0)? OP_DNREFI : OP_DNREF;
            PUT2INC(code, 0, index);
            PUT2INC(code, 0, count);

            /* Process each potentially referenced group. */

            for (; slot < cslot; slot += cd->name_entry_size)
              {
              open_capitem *oc;
              recno = GET2(slot, 0);
              cd->backref_map |= (recno < 32)? (1 << recno) : 1;
              if (recno > cd->top_backref) cd->top_backref = recno;

              /* Check to see if this back reference is recursive, that it, it
              is inside the group that it references. A flag is set so that the
              group can be made atomic. */

              for (oc = cd->open_caps; oc != NULL; oc = oc->next)
                {
                if (oc->number == recno)
                  {
                  oc->flag = TRUE;
                  break;
                  }
                }
              }

            continue;  /* End of back ref handling */
            }
          }

        /* First pass, or a non-duplicated name. */

        goto HANDLE_REFERENCE;


        /* ------------------------------------------------------------ */
        case CHAR_R:              /* Recursion, same as (?0) */
        recno = 0;
        if (*(++ptr) != CHAR_RIGHT_PARENTHESIS)
          {
          *errorcodeptr = ERR29;
          goto FAILED;
          }
        goto HANDLE_RECURSION;


        /* ------------------------------------------------------------ */
        case CHAR_MINUS: case CHAR_PLUS:  /* Recursion or subroutine */
        case CHAR_0: case CHAR_1: case CHAR_2: case CHAR_3: case CHAR_4:
        case CHAR_5: case CHAR_6: case CHAR_7: case CHAR_8: case CHAR_9:
          {
          const pcre_uchar *called;
          terminator = CHAR_RIGHT_PARENTHESIS;

          /* Come here from the \g<...> and \g'...' code (Oniguruma
          compatibility). However, the syntax has been checked to ensure that
          the ... are a (signed) number, so that neither ERR63 nor ERR29 will
          be called on this path, nor with the jump to OTHER_CHAR_AFTER_QUERY
          ever be taken. */

          HANDLE_NUMERICAL_RECURSION:

          if ((refsign = *ptr) == CHAR_PLUS)
            {
            ptr++;
            if (!IS_DIGIT(*ptr))
              {
              *errorcodeptr = ERR63;
              goto FAILED;
              }
            }
          else if (refsign == CHAR_MINUS)
            {
            if (!IS_DIGIT(ptr[1]))
              goto OTHER_CHAR_AFTER_QUERY;
            ptr++;
            }

          recno = 0;
          while(IS_DIGIT(*ptr))
            {
            if (recno > INT_MAX / 10 - 1) /* Integer overflow */
              {
              while (IS_DIGIT(*ptr)) ptr++;
              *errorcodeptr = ERR61;
              goto FAILED;
              }
            recno = recno * 10 + *ptr++ - CHAR_0;
            }

          if (*ptr != (pcre_uchar)terminator)
            {
            *errorcodeptr = ERR29;
            goto FAILED;
            }

          if (refsign == CHAR_MINUS)
            {
            if (recno == 0)
              {
              *errorcodeptr = ERR58;
              goto FAILED;
              }
            recno = cd->bracount - recno + 1;
            if (recno <= 0)
              {
              *errorcodeptr = ERR15;
              goto FAILED;
              }
            }
          else if (refsign == CHAR_PLUS)
            {
            if (recno == 0)
              {
              *errorcodeptr = ERR58;
              goto FAILED;
              }
            recno += cd->bracount;
            }

          /* Come here from code above that handles a named recursion */

          HANDLE_RECURSION:

          previous = code;
          item_hwm_offset = cd->hwm - cd->start_workspace;
          called = cd->start_code;

          /* When we are actually compiling, find the bracket that is being
          referenced. Temporarily end the regex in case it doesn't exist before
          this point. If we end up with a forward reference, first check that
          the bracket does occur later so we can give the error (and position)
          now. Then remember this forward reference in the workspace so it can
          be filled in at the end. */

          if (lengthptr == NULL)
            {
            *code = OP_END;
            if (recno != 0)
              called = PRIV(find_bracket)(cd->start_code, utf, recno);

            /* Forward reference */

            if (called == NULL)
              {
              if (recno > cd->final_bracount)
                {
                *errorcodeptr = ERR15;
                goto FAILED;
                }

              /* Fudge the value of "called" so that when it is inserted as an
              offset below, what it actually inserted is the reference number
              of the group. Then remember the forward reference. */

              called = cd->start_code + recno;
              if (cd->hwm >= cd->start_workspace + cd->workspace_size -
                  WORK_SIZE_SAFETY_MARGIN)
                {
                *errorcodeptr = expand_workspace(cd);
                if (*errorcodeptr != 0) goto FAILED;
                }
              PUTINC(cd->hwm, 0, (int)(code + 1 - cd->start_code));
              }

            /* If not a forward reference, and the subpattern is still open,
            this is a recursive call. We check to see if this is a left
            recursion that could loop for ever, and diagnose that case. We
            must not, however, do this check if we are in a conditional
            subpattern because the condition might be testing for recursion in
            a pattern such as /(?(R)a+|(?R)b)/, which is perfectly valid.
            Forever loops are also detected at runtime, so those that occur in
            conditional subpatterns will be picked up then. */

            else if (GET(called, 1) == 0 && cond_depth <= 0 &&
                     could_be_empty(called, code, bcptr, utf, cd))
              {
              *errorcodeptr = ERR40;
              goto FAILED;
              }
            }

          /* Insert the recursion/subroutine item. It does not have a set first
          character (relevant if it is repeated, because it will then be
          wrapped with ONCE brackets). */

          *code = OP_RECURSE;
          PUT(code, 1, (int)(called - cd->start_code));
          code += 1 + LINK_SIZE;
          groupsetfirstchar = FALSE;
          }

        /* Can't determine a first byte now */

        if (firstcharflags == REQ_UNSET) firstcharflags = REQ_NONE;
        continue;


        /* ------------------------------------------------------------ */
        default:              /* Other characters: check option setting */
        OTHER_CHAR_AFTER_QUERY:
        set = unset = 0;
        optset = &set;

        while (*ptr != CHAR_RIGHT_PARENTHESIS && *ptr != CHAR_COLON)
          {
          switch (*ptr++)
            {
            case CHAR_MINUS: optset = &unset; break;

            case CHAR_J:    /* Record that it changed in the external options */
            *optset |= PCRE_DUPNAMES;
            cd->external_flags |= PCRE_JCHANGED;
            break;

            case CHAR_i: *optset |= PCRE_CASELESS; break;
            case CHAR_m: *optset |= PCRE_MULTILINE; break;
            case CHAR_s: *optset |= PCRE_DOTALL; break;
            case CHAR_x: *optset |= PCRE_EXTENDED; break;
            case CHAR_U: *optset |= PCRE_UNGREEDY; break;
            case CHAR_X: *optset |= PCRE_EXTRA; break;

            default:  *errorcodeptr = ERR12;
                      ptr--;    /* Correct the offset */
                      goto FAILED;
            }
          }

        /* Set up the changed option bits, but don't change anything yet. */

        newoptions = (options | set) & (~unset);

        /* If the options ended with ')' this is not the start of a nested
        group with option changes, so the options change at this level. If this
        item is right at the start of the pattern, the options can be
        abstracted and made external in the pre-compile phase, and ignored in
        the compile phase. This can be helpful when matching -- for instance in
        caseless checking of required bytes.

        If the code pointer is not (cd->start_code + 1 + LINK_SIZE), we are
        definitely *not* at the start of the pattern because something has been
        compiled. In the pre-compile phase, however, the code pointer can have
        that value after the start, because it gets reset as code is discarded
        during the pre-compile. However, this can happen only at top level - if
        we are within parentheses, the starting BRA will still be present. At
        any parenthesis level, the length value can be used to test if anything
        has been compiled at that level. Thus, a test for both these conditions
        is necessary to ensure we correctly detect the start of the pattern in
        both phases.

        If we are not at the pattern start, reset the greedy defaults and the
        case value for firstchar and reqchar. */

        if (*ptr == CHAR_RIGHT_PARENTHESIS)
          {
          if (code == cd->start_code + 1 + LINK_SIZE &&
               (lengthptr == NULL || *lengthptr == 2 + 2*LINK_SIZE))
            {
            cd->external_options = newoptions;
            }
          else
            {
            greedy_default = ((newoptions & PCRE_UNGREEDY) != 0);
            greedy_non_default = greedy_default ^ 1;
            req_caseopt = ((newoptions & PCRE_CASELESS) != 0)? REQ_CASELESS:0;
            }

          /* Change options at this level, and pass them back for use
          in subsequent branches. */

          *optionsptr = options = newoptions;
          previous = NULL;       /* This item can't be repeated */
          continue;              /* It is complete */
          }

        /* If the options ended with ':' we are heading into a nested group
        with possible change of options. Such groups are non-capturing and are
        not assertions of any kind. All we need to do is skip over the ':';
        the newoptions value is handled below. */

        bravalue = OP_BRA;
        ptr++;
        }     /* End of switch for character following (? */
      }       /* End of (? handling */

    /* Opening parenthesis not followed by '*' or '?'. If PCRE_NO_AUTO_CAPTURE
    is set, all unadorned brackets become non-capturing and behave like (?:...)
    brackets. */

    else if ((options & PCRE_NO_AUTO_CAPTURE) != 0)
      {
      bravalue = OP_BRA;
      }

    /* Else we have a capturing group. */

    else
      {
      NUMBERED_GROUP:
      cd->bracount += 1;
      PUT2(code, 1+LINK_SIZE, cd->bracount);
      skipbytes = IMM2_SIZE;
      }

    /* Process nested bracketed regex. First check for parentheses nested too
    deeply. */

    if ((cd->parens_depth += 1) > PARENS_NEST_LIMIT)
      {
      *errorcodeptr = ERR82;
      goto FAILED;
      }

    /* All assertions used not to be repeatable, but this was changed for Perl
    compatibility. All kinds can now be repeated except for assertions that are
    conditions (Perl also forbids these to be repeated). We copy code into a
    non-register variable (tempcode) in order to be able to pass its address
    because some compilers complain otherwise. At the start of a conditional
    group whose condition is an assertion, cd->iscondassert is set. We unset it
    here so as to allow assertions later in the group to be quantified. */

    if (bravalue >= OP_ASSERT && bravalue <= OP_ASSERTBACK_NOT &&
        cd->iscondassert)
      {
      previous = NULL;
      cd->iscondassert = FALSE;
      }
    else
      {
      previous = code;
      item_hwm_offset = cd->hwm - cd->start_workspace;
      }

    *code = bravalue;
    tempcode = code;
    tempreqvary = cd->req_varyopt;        /* Save value before bracket */
    tempbracount = cd->bracount;          /* Save value before bracket */
    length_prevgroup = 0;                 /* Initialize for pre-compile phase */

    if (!compile_regex(
         newoptions,                      /* The complete new option state */
         &tempcode,                       /* Where to put code (updated) */
         &ptr,                            /* Input pointer (updated) */
         errorcodeptr,                    /* Where to put an error message */
         (bravalue == OP_ASSERTBACK ||
          bravalue == OP_ASSERTBACK_NOT), /* TRUE if back assert */
         reset_bracount,                  /* True if (?| group */
         skipbytes,                       /* Skip over bracket number */
         cond_depth +
           ((bravalue == OP_COND)?1:0),   /* Depth of condition subpatterns */
         &subfirstchar,                   /* For possible first char */
         &subfirstcharflags,
         &subreqchar,                     /* For possible last char */
         &subreqcharflags,
         bcptr,                           /* Current branch chain */
         cd,                              /* Tables block */
         (lengthptr == NULL)? NULL :      /* Actual compile phase */
           &length_prevgroup              /* Pre-compile phase */
         ))
      goto FAILED;

    cd->parens_depth -= 1;

    /* If this was an atomic group and there are no capturing groups within it,
    generate OP_ONCE_NC instead of OP_ONCE. */

    if (bravalue == OP_ONCE && cd->bracount <= tempbracount)
      *code = OP_ONCE_NC;

    if (bravalue >= OP_ASSERT && bravalue <= OP_ASSERTBACK_NOT)
      cd->assert_depth -= 1;

    /* At the end of compiling, code is still pointing to the start of the
    group, while tempcode has been updated to point past the end of the group.
    The pattern pointer (ptr) is on the bracket.

    If this is a conditional bracket, check that there are no more than
    two branches in the group, or just one if it's a DEFINE group. We do this
    in the real compile phase, not in the pre-pass, where the whole group may
    not be available. */

    if (bravalue == OP_COND && lengthptr == NULL)
      {
      pcre_uchar *tc = code;
      int condcount = 0;

      do {
         condcount++;
         tc += GET(tc,1);
         }
      while (*tc != OP_KET);

      /* A DEFINE group is never obeyed inline (the "condition" is always
      false). It must have only one branch. */

      if (code[LINK_SIZE+1] == OP_DEF)
        {
        if (condcount > 1)
          {
          *errorcodeptr = ERR54;
          goto FAILED;
          }
        bravalue = OP_DEF;   /* Just a flag to suppress char handling below */
        }

      /* A "normal" conditional group. If there is just one branch, we must not
      make use of its firstchar or reqchar, because this is equivalent to an
      empty second branch. */

      else
        {
        if (condcount > 2)
          {
          *errorcodeptr = ERR27;
          goto FAILED;
          }
        if (condcount == 1) subfirstcharflags = subreqcharflags = REQ_NONE;
        }
      }

    /* Error if hit end of pattern */

    if (*ptr != CHAR_RIGHT_PARENTHESIS)
      {
      *errorcodeptr = ERR14;
      goto FAILED;
      }

    /* In the pre-compile phase, update the length by the length of the group,
    less the brackets at either end. Then reduce the compiled code to just a
    set of non-capturing brackets so that it doesn't use much memory if it is
    duplicated by a quantifier.*/

    if (lengthptr != NULL)
      {
      if (OFLOW_MAX - *lengthptr < length_prevgroup - 2 - 2*LINK_SIZE)
        {
        *errorcodeptr = ERR20;
        goto FAILED;
        }
      *lengthptr += length_prevgroup - 2 - 2*LINK_SIZE;
      code++;   /* This already contains bravalue */
      PUTINC(code, 0, 1 + LINK_SIZE);
      *code++ = OP_KET;
      PUTINC(code, 0, 1 + LINK_SIZE);
      break;    /* No need to waste time with special character handling */
      }

    /* Otherwise update the main code pointer to the end of the group. */

    code = tempcode;

    /* For a DEFINE group, required and first character settings are not
    relevant. */

    if (bravalue == OP_DEF) break;

    /* Handle updating of the required and first characters for other types of
    group. Update for normal brackets of all kinds, and conditions with two
    branches (see code above). If the bracket is followed by a quantifier with
    zero repeat, we have to back off. Hence the definition of zeroreqchar and
    zerofirstchar outside the main loop so that they can be accessed for the
    back off. */

    zeroreqchar = reqchar;
    zeroreqcharflags = reqcharflags;
    zerofirstchar = firstchar;
    zerofirstcharflags = firstcharflags;
    groupsetfirstchar = FALSE;

    if (bravalue >= OP_ONCE)
      {
      /* If we have not yet set a firstchar in this branch, take it from the
      subpattern, remembering that it was set here so that a repeat of more
      than one can replicate it as reqchar if necessary. If the subpattern has
      no firstchar, set "none" for the whole branch. In both cases, a zero
      repeat forces firstchar to "none". */

      if (firstcharflags == REQ_UNSET)
        {
        if (subfirstcharflags >= 0)
          {
          firstchar = subfirstchar;
          firstcharflags = subfirstcharflags;
          groupsetfirstchar = TRUE;
          }
        else firstcharflags = REQ_NONE;
        zerofirstcharflags = REQ_NONE;
        }

      /* If firstchar was previously set, convert the subpattern's firstchar
      into reqchar if there wasn't one, using the vary flag that was in
      existence beforehand. */

      else if (subfirstcharflags >= 0 && subreqcharflags < 0)
        {
        subreqchar = subfirstchar;
        subreqcharflags = subfirstcharflags | tempreqvary;
        }

      /* If the subpattern set a required byte (or set a first byte that isn't
      really the first byte - see above), set it. */

      if (subreqcharflags >= 0)
        {
        reqchar = subreqchar;
        reqcharflags = subreqcharflags;
        }
      }

    /* For a forward assertion, we take the reqchar, if set. This can be
    helpful if the pattern that follows the assertion doesn't set a different
    char. For example, it's useful for /(?=abcde).+/. We can't set firstchar
    for an assertion, however because it leads to incorrect effect for patterns
    such as /(?=a)a.+/ when the "real" "a" would then become a reqchar instead
    of a firstchar. This is overcome by a scan at the end if there's no
    firstchar, looking for an asserted first char. */

    else if (bravalue == OP_ASSERT && subreqcharflags >= 0)
      {
      reqchar = subreqchar;
      reqcharflags = subreqcharflags;
      }
    break;     /* End of processing '(' */


    /* ===================================================================*/
    /* Handle metasequences introduced by \. For ones like \d, the ESC_ values
    are arranged to be the negation of the corresponding OP_values in the
    default case when PCRE_UCP is not set. For the back references, the values
    are negative the reference number. Only back references and those types
    that consume a character may be repeated. We can test for values between
    ESC_b and ESC_Z for the latter; this may have to change if any new ones are
    ever created. */

    case CHAR_BACKSLASH:
    tempptr = ptr;
    escape = check_escape(&ptr, &ec, errorcodeptr, cd->bracount, options, FALSE);
    if (*errorcodeptr != 0) goto FAILED;

    if (escape == 0)                  /* The escape coded a single character */
      c = ec;
    else
      {
      if (escape == ESC_Q)            /* Handle start of quoted string */
        {
        if (ptr[1] == CHAR_BACKSLASH && ptr[2] == CHAR_E)
          ptr += 2;               /* avoid empty string */
            else inescq = TRUE;
        continue;
        }

      if (escape == ESC_E) continue;  /* Perl ignores an orphan \E */

      /* For metasequences that actually match a character, we disable the
      setting of a first character if it hasn't already been set. */

      if (firstcharflags == REQ_UNSET && escape > ESC_b && escape < ESC_Z)
        firstcharflags = REQ_NONE;

      /* Set values to reset to if this is followed by a zero repeat. */

      zerofirstchar = firstchar;
      zerofirstcharflags = firstcharflags;
      zeroreqchar = reqchar;
      zeroreqcharflags = reqcharflags;

      /* \g<name> or \g'name' is a subroutine call by name and \g<n> or \g'n'
      is a subroutine call by number (Oniguruma syntax). In fact, the value
      ESC_g is returned only for these cases. So we don't need to check for <
      or ' if the value is ESC_g. For the Perl syntax \g{n} the value is
      -n, and for the Perl syntax \g{name} the result is ESC_k (as
      that is a synonym for a named back reference). */

      if (escape == ESC_g)
        {
        const pcre_uchar *p;
        pcre_uint32 cf;

        item_hwm_offset = cd->hwm - cd->start_workspace;   /* Normally this is set when '(' is read */
        terminator = (*(++ptr) == CHAR_LESS_THAN_SIGN)?
          CHAR_GREATER_THAN_SIGN : CHAR_APOSTROPHE;

        /* These two statements stop the compiler for warning about possibly
        unset variables caused by the jump to HANDLE_NUMERICAL_RECURSION. In
        fact, because we do the check for a number below, the paths that
        would actually be in error are never taken. */

        skipbytes = 0;
        reset_bracount = FALSE;

        /* If it's not a signed or unsigned number, treat it as a name. */

        cf = ptr[1];
        if (cf != CHAR_PLUS && cf != CHAR_MINUS && !IS_DIGIT(cf))
          {
          is_recurse = TRUE;
          goto NAMED_REF_OR_RECURSE;
          }

        /* Signed or unsigned number (cf = ptr[1]) is known to be plus or minus
        or a digit. */

        p = ptr + 2;
        while (IS_DIGIT(*p)) p++;
        if (*p != (pcre_uchar)terminator)
          {
          *errorcodeptr = ERR57;
          goto FAILED;
          }
        ptr++;
        goto HANDLE_NUMERICAL_RECURSION;
        }

      /* \k<name> or \k'name' is a back reference by name (Perl syntax).
      We also support \k{name} (.NET syntax).  */

      if (escape == ESC_k)
        {
        if ((ptr[1] != CHAR_LESS_THAN_SIGN &&
          ptr[1] != CHAR_APOSTROPHE && ptr[1] != CHAR_LEFT_CURLY_BRACKET))
          {
          *errorcodeptr = ERR69;
          goto FAILED;
          }
        is_recurse = FALSE;
        terminator = (*(++ptr) == CHAR_LESS_THAN_SIGN)?
          CHAR_GREATER_THAN_SIGN : (*ptr == CHAR_APOSTROPHE)?
          CHAR_APOSTROPHE : CHAR_RIGHT_CURLY_BRACKET;
        goto NAMED_REF_OR_RECURSE;
        }

      /* Back references are handled specially; must disable firstchar if
      not set to cope with cases like (?=(\w+))\1: which would otherwise set
      ':' later. */

      if (escape < 0)
        {
        open_capitem *oc;
        recno = -escape;

        /* Come here from named backref handling when the reference is to a
        single group (i.e. not to a duplicated name. */

        HANDLE_REFERENCE:
        if (firstcharflags == REQ_UNSET) firstcharflags = REQ_NONE;
        previous = code;
        item_hwm_offset = cd->hwm - cd->start_workspace;
        *code++ = ((options & PCRE_CASELESS) != 0)? OP_REFI : OP_REF;
        PUT2INC(code, 0, recno);
        cd->backref_map |= (recno < 32)? (1 << recno) : 1;
        if (recno > cd->top_backref) cd->top_backref = recno;

        /* Check to see if this back reference is recursive, that it, it
        is inside the group that it references. A flag is set so that the
        group can be made atomic. */

        for (oc = cd->open_caps; oc != NULL; oc = oc->next)
          {
          if (oc->number == recno)
            {
            oc->flag = TRUE;
            break;
            }
          }
        }

      /* So are Unicode property matches, if supported. */

#ifdef SUPPORT_UCP
      else if (escape == ESC_P || escape == ESC_p)
        {
        BOOL negated;
        unsigned int ptype = 0, pdata = 0;
        if (!get_ucp(&ptr, &negated, &ptype, &pdata, errorcodeptr))
          goto FAILED;
        previous = code;
        item_hwm_offset = cd->hwm - cd->start_workspace;
        *code++ = ((escape == ESC_p) != negated)? OP_PROP : OP_NOTPROP;
        *code++ = ptype;
        *code++ = pdata;
        }
#else

      /* If Unicode properties are not supported, \X, \P, and \p are not
      allowed. */

      else if (escape == ESC_X || escape == ESC_P || escape == ESC_p)
        {
        *errorcodeptr = ERR45;
        goto FAILED;
        }
#endif

      /* For the rest (including \X when Unicode properties are supported), we
      can obtain the OP value by negating the escape value in the default
      situation when PCRE_UCP is not set. When it *is* set, we substitute
      Unicode property tests. Note that \b and \B do a one-character
      lookbehind, and \A also behaves as if it does. */

      else
        {
        if ((escape == ESC_b || escape == ESC_B || escape == ESC_A) &&
             cd->max_lookbehind == 0)
          cd->max_lookbehind = 1;
#ifdef SUPPORT_UCP
        if (escape >= ESC_DU && escape <= ESC_wu)
          {
          nestptr = ptr + 1;                   /* Where to resume */
          ptr = substitutes[escape - ESC_DU] - 1;  /* Just before substitute */
          }
        else
#endif
        /* In non-UTF-8 mode, we turn \C into OP_ALLANY instead of OP_ANYBYTE
        so that it works in DFA mode and in lookbehinds. */

          {
          previous = (escape > ESC_b && escape < ESC_Z)? code : NULL;
          item_hwm_offset = cd->hwm - cd->start_workspace;
          *code++ = (!utf && escape == ESC_C)? OP_ALLANY : escape;
          }
        }
      continue;
      }

    /* We have a data character whose value is in c. In UTF-8 mode it may have
    a value > 127. We set its representation in the length/buffer, and then
    handle it as a data character. */

#if defined SUPPORT_UTF && !defined COMPILE_PCRE32
    if (utf && c > MAX_VALUE_FOR_SINGLE_CHAR)
      mclength = PRIV(ord2utf)(c, mcbuffer);
    else
#endif

     {
     mcbuffer[0] = c;
     mclength = 1;
     }
    goto ONE_CHAR;


    /* ===================================================================*/
    /* Handle a literal character. It is guaranteed not to be whitespace or #
    when the extended flag is set. If we are in a UTF mode, it may be a
    multi-unit literal character. */

    default:
    NORMAL_CHAR:
    mclength = 1;
    mcbuffer[0] = c;

#ifdef SUPPORT_UTF
    if (utf && HAS_EXTRALEN(c))
      ACROSSCHAR(TRUE, ptr[1], mcbuffer[mclength++] = *(++ptr));
#endif

    /* At this point we have the character's bytes in mcbuffer, and the length
    in mclength. When not in UTF-8 mode, the length is always 1. */

    ONE_CHAR:
    previous = code;
    item_hwm_offset = cd->hwm - cd->start_workspace;

    /* For caseless UTF-8 mode when UCP support is available, check whether
    this character has more than one other case. If so, generate a special
    OP_PROP item instead of OP_CHARI. */

#ifdef SUPPORT_UCP
    if (utf && (options & PCRE_CASELESS) != 0)
      {
      GETCHAR(c, mcbuffer);
      if ((c = UCD_CASESET(c)) != 0)
        {
        *code++ = OP_PROP;
        *code++ = PT_CLIST;
        *code++ = c;
        if (firstcharflags == REQ_UNSET)
          firstcharflags = zerofirstcharflags = REQ_NONE;
        break;
        }
      }
#endif

    /* Caseful matches, or not one of the multicase characters. */

    *code++ = ((options & PCRE_CASELESS) != 0)? OP_CHARI : OP_CHAR;
    for (c = 0; c < mclength; c++) *code++ = mcbuffer[c];

    /* Remember if \r or \n were seen */

    if (mcbuffer[0] == CHAR_CR || mcbuffer[0] == CHAR_NL)
      cd->external_flags |= PCRE_HASCRORLF;

    /* Set the first and required bytes appropriately. If no previous first
    byte, set it from this character, but revert to none on a zero repeat.
    Otherwise, leave the firstchar value alone, and don't change it on a zero
    repeat. */

    if (firstcharflags == REQ_UNSET)
      {
      zerofirstcharflags = REQ_NONE;
      zeroreqchar = reqchar;
      zeroreqcharflags = reqcharflags;

      /* If the character is more than one byte long, we can set firstchar
      only if it is not to be matched caselessly. */

      if (mclength == 1 || req_caseopt == 0)
        {
        firstchar = mcbuffer[0] | req_caseopt;
        firstchar = mcbuffer[0];
        firstcharflags = req_caseopt;

        if (mclength != 1)
          {
          reqchar = code[-1];
          reqcharflags = cd->req_varyopt;
          }
        }
      else firstcharflags = reqcharflags = REQ_NONE;
      }

    /* firstchar was previously set; we can set reqchar only if the length is
    1 or the matching is caseful. */

    else
      {
      zerofirstchar = firstchar;
      zerofirstcharflags = firstcharflags;
      zeroreqchar = reqchar;
      zeroreqcharflags = reqcharflags;
      if (mclength == 1 || req_caseopt == 0)
        {
        reqchar = code[-1];
        reqcharflags = req_caseopt | cd->req_varyopt;
        }
      }

    break;            /* End of literal character handling */
    }
  }                   /* end of big loop */


/* Control never reaches here by falling through, only by a goto for all the
error states. Pass back the position in the pattern so that it can be displayed
to the user for diagnosing the error. */

FAILED:
*ptrptr = ptr;
return FALSE;
}



/*************************************************
*     Compile sequence of alternatives           *
*************************************************/

/* On entry, ptr is pointing past the bracket character, but on return it
points to the closing bracket, or vertical bar, or end of string. The code
variable is pointing at the byte into which the BRA operator has been stored.
This function is used during the pre-compile phase when we are trying to find
out the amount of memory needed, as well as during the real compile phase. The
value of lengthptr distinguishes the two phases.

Arguments:
  options           option bits, including any changes for this subpattern
  codeptr           -> the address of the current code pointer
  ptrptr            -> the address of the current pattern pointer
  errorcodeptr      -> pointer to error code variable
  lookbehind        TRUE if this is a lookbehind assertion
  reset_bracount    TRUE to reset the count for each branch
  skipbytes         skip this many bytes at start (for brackets and OP_COND)
  cond_depth        depth of nesting for conditional subpatterns
  firstcharptr      place to put the first required character
  firstcharflagsptr place to put the first character flags, or a negative number
  reqcharptr        place to put the last required character
  reqcharflagsptr   place to put the last required character flags, or a negative number
  bcptr             pointer to the chain of currently open branches
  cd                points to the data block with tables pointers etc.
  lengthptr         NULL during the real compile phase
                    points to length accumulator during pre-compile phase

Returns:            TRUE on success
*/

static BOOL
compile_regex(int options, pcre_uchar **codeptr, const pcre_uchar **ptrptr,
  int *errorcodeptr, BOOL lookbehind, BOOL reset_bracount, int skipbytes,
  int cond_depth,
  pcre_uint32 *firstcharptr, pcre_int32 *firstcharflagsptr,
  pcre_uint32 *reqcharptr, pcre_int32 *reqcharflagsptr,
  branch_chain *bcptr, compile_data *cd, int *lengthptr)
{
const pcre_uchar *ptr = *ptrptr;
pcre_uchar *code = *codeptr;
pcre_uchar *last_branch = code;
pcre_uchar *start_bracket = code;
pcre_uchar *reverse_count = NULL;
open_capitem capitem;
int capnumber = 0;
pcre_uint32 firstchar, reqchar;
pcre_int32 firstcharflags, reqcharflags;
pcre_uint32 branchfirstchar, branchreqchar;
pcre_int32 branchfirstcharflags, branchreqcharflags;
int length;
unsigned int orig_bracount;
unsigned int max_bracount;
branch_chain bc;
size_t save_hwm_offset;

/* If set, call the external function that checks for stack availability. */

if (PUBL(stack_guard) != NULL && PUBL(stack_guard)())
  {
  *errorcodeptr= ERR85;
  return FALSE;
  }

/* Miscellaneous initialization */

bc.outer = bcptr;
bc.current_branch = code;

firstchar = reqchar = 0;
firstcharflags = reqcharflags = REQ_UNSET;

save_hwm_offset = cd->hwm - cd->start_workspace;

/* Accumulate the length for use in the pre-compile phase. Start with the
length of the BRA and KET and any extra bytes that are required at the
beginning. We accumulate in a local variable to save frequent testing of
lenthptr for NULL. We cannot do this by looking at the value of code at the
start and end of each alternative, because compiled items are discarded during
the pre-compile phase so that the work space is not exceeded. */

length = 2 + 2*LINK_SIZE + skipbytes;

/* WARNING: If the above line is changed for any reason, you must also change
the code that abstracts option settings at the start of the pattern and makes
them global. It tests the value of length for (2 + 2*LINK_SIZE) in the
pre-compile phase to find out whether anything has yet been compiled or not. */

/* If this is a capturing subpattern, add to the chain of open capturing items
so that we can detect them if (*ACCEPT) is encountered. This is also used to
detect groups that contain recursive back references to themselves. Note that
only OP_CBRA need be tested here; changing this opcode to one of its variants,
e.g. OP_SCBRAPOS, happens later, after the group has been compiled. */

if (*code == OP_CBRA)
  {
  capnumber = GET2(code, 1 + LINK_SIZE);
  capitem.number = capnumber;
  capitem.next = cd->open_caps;
  capitem.flag = FALSE;
  cd->open_caps = &capitem;
  }

/* Offset is set zero to mark that this bracket is still open */

PUT(code, 1, 0);
code += 1 + LINK_SIZE + skipbytes;

/* Loop for each alternative branch */

orig_bracount = max_bracount = cd->bracount;
for (;;)
  {
  /* For a (?| group, reset the capturing bracket count so that each branch
  uses the same numbers. */

  if (reset_bracount) cd->bracount = orig_bracount;

  /* Set up dummy OP_REVERSE if lookbehind assertion */

  if (lookbehind)
    {
    *code++ = OP_REVERSE;
    reverse_count = code;
    PUTINC(code, 0, 0);
    length += 1 + LINK_SIZE;
    }

  /* Now compile the branch; in the pre-compile phase its length gets added
  into the length. */

  if (!compile_branch(&options, &code, &ptr, errorcodeptr, &branchfirstchar,
        &branchfirstcharflags, &branchreqchar, &branchreqcharflags, &bc,
        cond_depth, cd, (lengthptr == NULL)? NULL : &length))
    {
    *ptrptr = ptr;
    return FALSE;
    }

  /* Keep the highest bracket count in case (?| was used and some branch
  has fewer than the rest. */

  if (cd->bracount > max_bracount) max_bracount = cd->bracount;

  /* In the real compile phase, there is some post-processing to be done. */

  if (lengthptr == NULL)
    {
    /* If this is the first branch, the firstchar and reqchar values for the
    branch become the values for the regex. */

    if (*last_branch != OP_ALT)
      {
      firstchar = branchfirstchar;
      firstcharflags = branchfirstcharflags;
      reqchar = branchreqchar;
      reqcharflags = branchreqcharflags;
      }

    /* If this is not the first branch, the first char and reqchar have to
    match the values from all the previous branches, except that if the
    previous value for reqchar didn't have REQ_VARY set, it can still match,
    and we set REQ_VARY for the regex. */

    else
      {
      /* If we previously had a firstchar, but it doesn't match the new branch,
      we have to abandon the firstchar for the regex, but if there was
      previously no reqchar, it takes on the value of the old firstchar. */

      if (firstcharflags >= 0 &&
          (firstcharflags != branchfirstcharflags || firstchar != branchfirstchar))
        {
        if (reqcharflags < 0)
          {
          reqchar = firstchar;
          reqcharflags = firstcharflags;
          }
        firstcharflags = REQ_NONE;
        }

      /* If we (now or from before) have no firstchar, a firstchar from the
      branch becomes a reqchar if there isn't a branch reqchar. */

      if (firstcharflags < 0 && branchfirstcharflags >= 0 && branchreqcharflags < 0)
        {
        branchreqchar = branchfirstchar;
        branchreqcharflags = branchfirstcharflags;
        }

      /* Now ensure that the reqchars match */

      if (((reqcharflags & ~REQ_VARY) != (branchreqcharflags & ~REQ_VARY)) ||
          reqchar != branchreqchar)
        reqcharflags = REQ_NONE;
      else
        {
        reqchar = branchreqchar;
        reqcharflags |= branchreqcharflags; /* To "or" REQ_VARY */
        }
      }

    /* If lookbehind, check that this branch matches a fixed-length string, and
    put the length into the OP_REVERSE item. Temporarily mark the end of the
    branch with OP_END. If the branch contains OP_RECURSE, the result is -3
    because there may be forward references that we can't check here. Set a
    flag to cause another lookbehind check at the end. Why not do it all at the
    end? Because common, erroneous checks are picked up here and the offset of
    the problem can be shown. */

    if (lookbehind)
      {
      int fixed_length;
      *code = OP_END;
      fixed_length = find_fixedlength(last_branch,  (options & PCRE_UTF8) != 0,
        FALSE, cd, NULL);
      DPRINTF(("fixed length = %d\n", fixed_length));
      if (fixed_length == -3)
        {
        cd->check_lookbehind = TRUE;
        }
      else if (fixed_length < 0)
        {
        *errorcodeptr = (fixed_length == -2)? ERR36 :
                        (fixed_length == -4)? ERR70: ERR25;
        *ptrptr = ptr;
        return FALSE;
        }
      else
        {
        if (fixed_length > cd->max_lookbehind)
          cd->max_lookbehind = fixed_length;
        PUT(reverse_count, 0, fixed_length);
        }
      }
    }

  /* Reached end of expression, either ')' or end of pattern. In the real
  compile phase, go back through the alternative branches and reverse the chain
  of offsets, with the field in the BRA item now becoming an offset to the
  first alternative. If there are no alternatives, it points to the end of the
  group. The length in the terminating ket is always the length of the whole
  bracketed item. Return leaving the pointer at the terminating char. */

  if (*ptr != CHAR_VERTICAL_LINE)
    {
    if (lengthptr == NULL)
      {
      int branch_length = (int)(code - last_branch);
      do
        {
        int prev_length = GET(last_branch, 1);
        PUT(last_branch, 1, branch_length);
        branch_length = prev_length;
        last_branch -= branch_length;
        }
      while (branch_length > 0);
      }

    /* Fill in the ket */

    *code = OP_KET;
    PUT(code, 1, (int)(code - start_bracket));
    code += 1 + LINK_SIZE;

    /* If it was a capturing subpattern, check to see if it contained any
    recursive back references. If so, we must wrap it in atomic brackets.
    Because we are moving code along, we must ensure that any pending recursive
    references are updated. In any event, remove the block from the chain. */

    if (capnumber > 0)
      {
      if (cd->open_caps->flag)
        {
        *code = OP_END;
        adjust_recurse(start_bracket, 1 + LINK_SIZE,
          (options & PCRE_UTF8) != 0, cd, save_hwm_offset);
        memmove(start_bracket + 1 + LINK_SIZE, start_bracket,
          IN_UCHARS(code - start_bracket));
        *start_bracket = OP_ONCE;
        code += 1 + LINK_SIZE;
        PUT(start_bracket, 1, (int)(code - start_bracket));
        *code = OP_KET;
        PUT(code, 1, (int)(code - start_bracket));
        code += 1 + LINK_SIZE;
        length += 2 + 2*LINK_SIZE;
        }
      cd->open_caps = cd->open_caps->next;
      }

    /* Retain the highest bracket number, in case resetting was used. */

    cd->bracount = max_bracount;

    /* Set values to pass back */

    *codeptr = code;
    *ptrptr = ptr;
    *firstcharptr = firstchar;
    *firstcharflagsptr = firstcharflags;
    *reqcharptr = reqchar;
    *reqcharflagsptr = reqcharflags;
    if (lengthptr != NULL)
      {
      if (OFLOW_MAX - *lengthptr < length)
        {
        *errorcodeptr = ERR20;
        return FALSE;
        }
      *lengthptr += length;
      }
    return TRUE;
    }

  /* Another branch follows. In the pre-compile phase, we can move the code
  pointer back to where it was for the start of the first branch. (That is,
  pretend that each branch is the only one.)

  In the real compile phase, insert an ALT node. Its length field points back
  to the previous branch while the bracket remains open. At the end the chain
  is reversed. It's done like this so that the start of the bracket has a
  zero offset until it is closed, making it possible to detect recursion. */

  if (lengthptr != NULL)
    {
    code = *codeptr + 1 + LINK_SIZE + skipbytes;
    length += 1 + LINK_SIZE;
    }
  else
    {
    *code = OP_ALT;
    PUT(code, 1, (int)(code - last_branch));
    bc.current_branch = last_branch = code;
    code += 1 + LINK_SIZE;
    }

  ptr++;
  }
/* Control never reaches here */
}




/*************************************************
*          Check for anchored expression         *
*************************************************/

/* Try to find out if this is an anchored regular expression. Consider each
alternative branch. If they all start with OP_SOD or OP_CIRC, or with a bracket
all of whose alternatives start with OP_SOD or OP_CIRC (recurse ad lib), then
it's anchored. However, if this is a multiline pattern, then only OP_SOD will
be found, because ^ generates OP_CIRCM in that mode.

We can also consider a regex to be anchored if OP_SOM starts all its branches.
This is the code for \G, which means "match at start of match position, taking
into account the match offset".

A branch is also implicitly anchored if it starts with .* and DOTALL is set,
because that will try the rest of the pattern at all possible matching points,
so there is no point trying again.... er ....

.... except when the .* appears inside capturing parentheses, and there is a
subsequent back reference to those parentheses. We haven't enough information
to catch that case precisely.

At first, the best we could do was to detect when .* was in capturing brackets
and the highest back reference was greater than or equal to that level.
However, by keeping a bitmap of the first 31 back references, we can catch some
of the more common cases more precisely.

... A second exception is when the .* appears inside an atomic group, because
this prevents the number of characters it matches from being adjusted.

Arguments:
  code           points to start of expression (the bracket)
  bracket_map    a bitmap of which brackets we are inside while testing; this
                  handles up to substring 31; after that we just have to take
                  the less precise approach
  cd             points to the compile data block
  atomcount      atomic group level

Returns:     TRUE or FALSE
*/

static BOOL
is_anchored(register const pcre_uchar *code, unsigned int bracket_map,
  compile_data *cd, int atomcount)
{
do {
   const pcre_uchar *scode = first_significant_code(
     code + PRIV(OP_lengths)[*code], FALSE);
   register int op = *scode;

   /* Non-capturing brackets */

   if (op == OP_BRA  || op == OP_BRAPOS ||
       op == OP_SBRA || op == OP_SBRAPOS)
     {
     if (!is_anchored(scode, bracket_map, cd, atomcount)) return FALSE;
     }

   /* Capturing brackets */

   else if (op == OP_CBRA  || op == OP_CBRAPOS ||
            op == OP_SCBRA || op == OP_SCBRAPOS)
     {
     int n = GET2(scode, 1+LINK_SIZE);
     int new_map = bracket_map | ((n < 32)? (1 << n) : 1);
     if (!is_anchored(scode, new_map, cd, atomcount)) return FALSE;
     }

   /* Positive forward assertions and conditions */

   else if (op == OP_ASSERT || op == OP_COND)
     {
     if (!is_anchored(scode, bracket_map, cd, atomcount)) return FALSE;
     }

   /* Atomic groups */

   else if (op == OP_ONCE || op == OP_ONCE_NC)
     {
     if (!is_anchored(scode, bracket_map, cd, atomcount + 1))
       return FALSE;
     }

   /* .* is not anchored unless DOTALL is set (which generates OP_ALLANY) and
   it isn't in brackets that are or may be referenced or inside an atomic
   group. */

   else if ((op == OP_TYPESTAR || op == OP_TYPEMINSTAR ||
             op == OP_TYPEPOSSTAR))
     {
     if (scode[1] != OP_ALLANY || (bracket_map & cd->backref_map) != 0 ||
         atomcount > 0 || cd->had_pruneorskip)
       return FALSE;
     }

   /* Check for explicit anchoring */

   else if (op != OP_SOD && op != OP_SOM && op != OP_CIRC) return FALSE;

   code += GET(code, 1);
   }
while (*code == OP_ALT);   /* Loop for each alternative */
return TRUE;
}



/*************************************************
*         Check for starting with ^ or .*        *
*************************************************/

/* This is called to find out if every branch starts with ^ or .* so that
"first char" processing can be done to speed things up in multiline
matching and for non-DOTALL patterns that start with .* (which must start at
the beginning or after \n). As in the case of is_anchored() (see above), we
have to take account of back references to capturing brackets that contain .*
because in that case we can't make the assumption. Also, the appearance of .*
inside atomic brackets or in a pattern that contains *PRUNE or *SKIP does not
count, because once again the assumption no longer holds.

Arguments:
  code           points to start of expression (the bracket)
  bracket_map    a bitmap of which brackets we are inside while testing; this
                  handles up to substring 31; after that we just have to take
                  the less precise approach
  cd             points to the compile data
  atomcount      atomic group level

Returns:         TRUE or FALSE
*/

static BOOL
is_startline(const pcre_uchar *code, unsigned int bracket_map,
  compile_data *cd, int atomcount)
{
do {
   const pcre_uchar *scode = first_significant_code(
     code + PRIV(OP_lengths)[*code], FALSE);
   register int op = *scode;

   /* If we are at the start of a conditional assertion group, *both* the
   conditional assertion *and* what follows the condition must satisfy the test
   for start of line. Other kinds of condition fail. Note that there may be an
   auto-callout at the start of a condition. */

   if (op == OP_COND)
     {
     scode += 1 + LINK_SIZE;
     if (*scode == OP_CALLOUT) scode += PRIV(OP_lengths)[OP_CALLOUT];
     switch (*scode)
       {
       case OP_CREF:
       case OP_DNCREF:
       case OP_RREF:
       case OP_DNRREF:
       case OP_DEF:
       case OP_FAIL:
       return FALSE;

       default:     /* Assertion */
       if (!is_startline(scode, bracket_map, cd, atomcount)) return FALSE;
       do scode += GET(scode, 1); while (*scode == OP_ALT);
       scode += 1 + LINK_SIZE;
       break;
       }
     scode = first_significant_code(scode, FALSE);
     op = *scode;
     }

   /* Non-capturing brackets */

   if (op == OP_BRA  || op == OP_BRAPOS ||
       op == OP_SBRA || op == OP_SBRAPOS)
     {
     if (!is_startline(scode, bracket_map, cd, atomcount)) return FALSE;
     }

   /* Capturing brackets */

   else if (op == OP_CBRA  || op == OP_CBRAPOS ||
            op == OP_SCBRA || op == OP_SCBRAPOS)
     {
     int n = GET2(scode, 1+LINK_SIZE);
     int new_map = bracket_map | ((n < 32)? (1 << n) : 1);
     if (!is_startline(scode, new_map, cd, atomcount)) return FALSE;
     }

   /* Positive forward assertions */

   else if (op == OP_ASSERT)
     {
     if (!is_startline(scode, bracket_map, cd, atomcount)) return FALSE;
     }

   /* Atomic brackets */

   else if (op == OP_ONCE || op == OP_ONCE_NC)
     {
     if (!is_startline(scode, bracket_map, cd, atomcount + 1)) return FALSE;
     }

   /* .* means "start at start or after \n" if it isn't in atomic brackets or
   brackets that may be referenced, as long as the pattern does not contain
   *PRUNE or *SKIP, because these break the feature. Consider, for example,
   /.*?a(*PRUNE)b/ with the subject "aab", which matches "ab", i.e. not at the
   start of a line. */

   else if (op == OP_TYPESTAR || op == OP_TYPEMINSTAR || op == OP_TYPEPOSSTAR)
     {
     if (scode[1] != OP_ANY || (bracket_map & cd->backref_map) != 0 ||
         atomcount > 0 || cd->had_pruneorskip)
       return FALSE;
     }

   /* Check for explicit circumflex; anything else gives a FALSE result. Note
   in particular that this includes atomic brackets OP_ONCE and OP_ONCE_NC
   because the number of characters matched by .* cannot be adjusted inside
   them. */

   else if (op != OP_CIRC && op != OP_CIRCM) return FALSE;

   /* Move on to the next alternative */

   code += GET(code, 1);
   }
while (*code == OP_ALT);  /* Loop for each alternative */
return TRUE;
}



/*************************************************
*       Check for asserted fixed first char      *
*************************************************/

/* During compilation, the "first char" settings from forward assertions are
discarded, because they can cause conflicts with actual literals that follow.
However, if we end up without a first char setting for an unanchored pattern,
it is worth scanning the regex to see if there is an initial asserted first
char. If all branches start with the same asserted char, or with a
non-conditional bracket all of whose alternatives start with the same asserted
char (recurse ad lib), then we return that char, with the flags set to zero or
REQ_CASELESS; otherwise return zero with REQ_NONE in the flags.

Arguments:
  code       points to start of expression (the bracket)
  flags      points to the first char flags, or to REQ_NONE
  inassert   TRUE if in an assertion

Returns:     the fixed first char, or 0 with REQ_NONE in flags
*/

static pcre_uint32
find_firstassertedchar(const pcre_uchar *code, pcre_int32 *flags,
  BOOL inassert)
{
register pcre_uint32 c = 0;
int cflags = REQ_NONE;

*flags = REQ_NONE;
do {
   pcre_uint32 d;
   int dflags;
   int xl = (*code == OP_CBRA || *code == OP_SCBRA ||
             *code == OP_CBRAPOS || *code == OP_SCBRAPOS)? IMM2_SIZE:0;
   const pcre_uchar *scode = first_significant_code(code + 1+LINK_SIZE + xl,
     TRUE);
   register pcre_uchar op = *scode;

   switch(op)
     {
     default:
     return 0;

     case OP_BRA:
     case OP_BRAPOS:
     case OP_CBRA:
     case OP_SCBRA:
     case OP_CBRAPOS:
     case OP_SCBRAPOS:
     case OP_ASSERT:
     case OP_ONCE:
     case OP_ONCE_NC:
     d = find_firstassertedchar(scode, &dflags, op == OP_ASSERT);
     if (dflags < 0)
       return 0;
     if (cflags < 0) { c = d; cflags = dflags; } else if (c != d || cflags != dflags) return 0;
     break;

     case OP_EXACT:
     scode += IMM2_SIZE;
     /* Fall through */

     case OP_CHAR:
     case OP_PLUS:
     case OP_MINPLUS:
     case OP_POSPLUS:
     if (!inassert) return 0;
     if (cflags < 0) { c = scode[1]; cflags = 0; }
       else if (c != scode[1]) return 0;
     break;

     case OP_EXACTI:
     scode += IMM2_SIZE;
     /* Fall through */

     case OP_CHARI:
     case OP_PLUSI:
     case OP_MINPLUSI:
     case OP_POSPLUSI:
     if (!inassert) return 0;
     if (cflags < 0) { c = scode[1]; cflags = REQ_CASELESS; }
       else if (c != scode[1]) return 0;
     break;
     }

   code += GET(code, 1);
   }
while (*code == OP_ALT);

*flags = cflags;
return c;
}



/*************************************************
*     Add an entry to the name/number table      *
*************************************************/

/* This function is called between compiling passes to add an entry to the
name/number table, maintaining alphabetical order. Checking for permitted
and forbidden duplicates has already been done.

Arguments:
  cd           the compile data block
  name         the name to add
  length       the length of the name
  groupno      the group number

Returns:       nothing
*/

static void
add_name(compile_data *cd, const pcre_uchar *name, int length,
  unsigned int groupno)
{
int i;
pcre_uchar *slot = cd->name_table;

for (i = 0; i < cd->names_found; i++)
  {
  int crc = memcmp(name, slot+IMM2_SIZE, IN_UCHARS(length));
  if (crc == 0 && slot[IMM2_SIZE+length] != 0)
    crc = -1; /* Current name is a substring */

  /* Make space in the table and break the loop for an earlier name. For a
  duplicate or later name, carry on. We do this for duplicates so that in the
  simple case (when ?(| is not used) they are in order of their numbers. In all
  cases they are in the order in which they appear in the pattern. */

  if (crc < 0)
    {
    memmove(slot + cd->name_entry_size, slot,
      IN_UCHARS((cd->names_found - i) * cd->name_entry_size));
    break;
    }

  /* Continue the loop for a later or duplicate name */

  slot += cd->name_entry_size;
  }

PUT2(slot, 0, groupno);
memcpy(slot + IMM2_SIZE, name, IN_UCHARS(length));
slot[IMM2_SIZE + length] = 0;
cd->names_found++;
}



/*************************************************
*        Compile a Regular Expression            *
*************************************************/

/* This function takes a string and returns a pointer to a block of store
holding a compiled version of the expression. The original API for this
function had no error code return variable; it is retained for backwards
compatibility. The new function is given a new name.

Arguments:
  pattern       the regular expression
  options       various option bits
  errorcodeptr  pointer to error code variable (pcre_compile2() only)
                  can be NULL if you don't want a code value
  errorptr      pointer to pointer to error text
  erroroffset   ptr offset in pattern where error was detected
  tables        pointer to character tables or NULL

Returns:        pointer to compiled data block, or NULL on error,
                with errorptr and erroroffset set
*/

#if defined COMPILE_PCRE8
PCRE_EXP_DEFN pcre * PCRE_CALL_CONVENTION
pcre_compile(const char *pattern, int options, const char **errorptr,
  int *erroroffset, const unsigned char *tables)
#elif defined COMPILE_PCRE16
PCRE_EXP_DEFN pcre16 * PCRE_CALL_CONVENTION
pcre16_compile(PCRE_SPTR16 pattern, int options, const char **errorptr,
  int *erroroffset, const unsigned char *tables)
#elif defined COMPILE_PCRE32
PCRE_EXP_DEFN pcre32 * PCRE_CALL_CONVENTION
pcre32_compile(PCRE_SPTR32 pattern, int options, const char **errorptr,
  int *erroroffset, const unsigned char *tables)
#endif
{
#if defined COMPILE_PCRE8
return pcre_compile2(pattern, options, NULL, errorptr, erroroffset, tables);
#elif defined COMPILE_PCRE16
return pcre16_compile2(pattern, options, NULL, errorptr, erroroffset, tables);
#elif defined COMPILE_PCRE32
return pcre32_compile2(pattern, options, NULL, errorptr, erroroffset, tables);
#endif
}


#if defined COMPILE_PCRE8
PCRE_EXP_DEFN pcre * PCRE_CALL_CONVENTION
pcre_compile2(const char *pattern, int options, int *errorcodeptr,
  const char **errorptr, int *erroroffset, const unsigned char *tables)
#elif defined COMPILE_PCRE16
PCRE_EXP_DEFN pcre16 * PCRE_CALL_CONVENTION
pcre16_compile2(PCRE_SPTR16 pattern, int options, int *errorcodeptr,
  const char **errorptr, int *erroroffset, const unsigned char *tables)
#elif defined COMPILE_PCRE32
PCRE_EXP_DEFN pcre32 * PCRE_CALL_CONVENTION
pcre32_compile2(PCRE_SPTR32 pattern, int options, int *errorcodeptr,
  const char **errorptr, int *erroroffset, const unsigned char *tables)
#endif
{
REAL_PCRE *re;
int length = 1;  /* For final END opcode */
pcre_int32 firstcharflags, reqcharflags;
pcre_uint32 firstchar, reqchar;
pcre_uint32 limit_match = PCRE_UINT32_MAX;
pcre_uint32 limit_recursion = PCRE_UINT32_MAX;
int newline;
int errorcode = 0;
int skipatstart = 0;
BOOL utf;
BOOL never_utf = FALSE;
size_t size;
pcre_uchar *code;
const pcre_uchar *codestart;
const pcre_uchar *ptr;
compile_data compile_block;
compile_data *cd = &compile_block;

/* This space is used for "compiling" into during the first phase, when we are
computing the amount of memory that is needed. Compiled items are thrown away
as soon as possible, so that a fairly large buffer should be sufficient for
this purpose. The same space is used in the second phase for remembering where
to fill in forward references to subpatterns. That may overflow, in which case
new memory is obtained from malloc(). */

pcre_uchar cworkspace[COMPILE_WORK_SIZE];

/* This vector is used for remembering name groups during the pre-compile. In a
similar way to cworkspace, it can be expanded using malloc() if necessary. */

named_group named_groups[NAMED_GROUP_LIST_SIZE];

/* Set this early so that early errors get offset 0. */

ptr = (const pcre_uchar *)pattern;

/* We can't pass back an error message if errorptr is NULL; I guess the best we
can do is just return NULL, but we can set a code value if there is a code
pointer. */

if (errorptr == NULL)
  {
  if (errorcodeptr != NULL) *errorcodeptr = 99;
  return NULL;
  }

*errorptr = NULL;
if (errorcodeptr != NULL) *errorcodeptr = ERR0;

/* However, we can give a message for this error */

if (erroroffset == NULL)
  {
  errorcode = ERR16;
  goto PCRE_EARLY_ERROR_RETURN2;
  }

*erroroffset = 0;

/* Set up pointers to the individual character tables */

if (tables == NULL) tables = PRIV(default_tables);
cd->lcc = tables + lcc_offset;
cd->fcc = tables + fcc_offset;
cd->cbits = tables + cbits_offset;
cd->ctypes = tables + ctypes_offset;

/* Check that all undefined public option bits are zero */

if ((options & ~PUBLIC_COMPILE_OPTIONS) != 0)
  {
  errorcode = ERR17;
  goto PCRE_EARLY_ERROR_RETURN;
  }

/* If PCRE_NEVER_UTF is set, remember it. */

if ((options & PCRE_NEVER_UTF) != 0) never_utf = TRUE;

/* Check for global one-time settings at the start of the pattern, and remember
the offset for later. */

cd->external_flags = 0;   /* Initialize here for LIMIT_MATCH/RECURSION */

while (ptr[skipatstart] == CHAR_LEFT_PARENTHESIS &&
       ptr[skipatstart+1] == CHAR_ASTERISK)
  {
  int newnl = 0;
  int newbsr = 0;

/* For completeness and backward compatibility, (*UTFn) is supported in the
relevant libraries, but (*UTF) is generic and always supported. Note that
PCRE_UTF8 == PCRE_UTF16 == PCRE_UTF32. */

#ifdef COMPILE_PCRE8
  if (STRNCMP_UC_C8(ptr+skipatstart+2, STRING_UTF8_RIGHTPAR, 5) == 0)
    { skipatstart += 7; options |= PCRE_UTF8; continue; }
#endif
#ifdef COMPILE_PCRE16
  if (STRNCMP_UC_C8(ptr+skipatstart+2, STRING_UTF16_RIGHTPAR, 6) == 0)
    { skipatstart += 8; options |= PCRE_UTF16; continue; }
#endif
#ifdef COMPILE_PCRE32
  if (STRNCMP_UC_C8(ptr+skipatstart+2, STRING_UTF32_RIGHTPAR, 6) == 0)
    { skipatstart += 8; options |= PCRE_UTF32; continue; }
#endif

  else if (STRNCMP_UC_C8(ptr+skipatstart+2, STRING_UTF_RIGHTPAR, 4) == 0)
    { skipatstart += 6; options |= PCRE_UTF8; continue; }
  else if (STRNCMP_UC_C8(ptr+skipatstart+2, STRING_UCP_RIGHTPAR, 4) == 0)
    { skipatstart += 6; options |= PCRE_UCP; continue; }
  else if (STRNCMP_UC_C8(ptr+skipatstart+2, STRING_NO_AUTO_POSSESS_RIGHTPAR, 16) == 0)
    { skipatstart += 18; options |= PCRE_NO_AUTO_POSSESS; continue; }
  else if (STRNCMP_UC_C8(ptr+skipatstart+2, STRING_NO_START_OPT_RIGHTPAR, 13) == 0)
    { skipatstart += 15; options |= PCRE_NO_START_OPTIMIZE; continue; }

  else if (STRNCMP_UC_C8(ptr+skipatstart+2, STRING_LIMIT_MATCH_EQ, 12) == 0)
    {
    pcre_uint32 c = 0;
    int p = skipatstart + 14;
    while (isdigit(ptr[p]))
      {
      if (c > PCRE_UINT32_MAX / 10 - 1) break;   /* Integer overflow */
      c = c*10 + ptr[p++] - CHAR_0;
      }
    if (ptr[p++] != CHAR_RIGHT_PARENTHESIS) break;
    if (c < limit_match)
      {
      limit_match = c;
      cd->external_flags |= PCRE_MLSET;
      }
    skipatstart = p;
    continue;
    }

  else if (STRNCMP_UC_C8(ptr+skipatstart+2, STRING_LIMIT_RECURSION_EQ, 16) == 0)
    {
    pcre_uint32 c = 0;
    int p = skipatstart + 18;
    while (isdigit(ptr[p]))
      {
      if (c > PCRE_UINT32_MAX / 10 - 1) break;   /* Integer overflow check */
      c = c*10 + ptr[p++] - CHAR_0;
      }
    if (ptr[p++] != CHAR_RIGHT_PARENTHESIS) break;
    if (c < limit_recursion)
      {
      limit_recursion = c;
      cd->external_flags |= PCRE_RLSET;
      }
    skipatstart = p;
    continue;
    }

  if (STRNCMP_UC_C8(ptr+skipatstart+2, STRING_CR_RIGHTPAR, 3) == 0)
    { skipatstart += 5; newnl = PCRE_NEWLINE_CR; }
  else if (STRNCMP_UC_C8(ptr+skipatstart+2, STRING_LF_RIGHTPAR, 3)  == 0)
    { skipatstart += 5; newnl = PCRE_NEWLINE_LF; }
  else if (STRNCMP_UC_C8(ptr+skipatstart+2, STRING_CRLF_RIGHTPAR, 5)  == 0)
    { skipatstart += 7; newnl = PCRE_NEWLINE_CR + PCRE_NEWLINE_LF; }
  else if (STRNCMP_UC_C8(ptr+skipatstart+2, STRING_ANY_RIGHTPAR, 4) == 0)
    { skipatstart += 6; newnl = PCRE_NEWLINE_ANY; }
  else if (STRNCMP_UC_C8(ptr+skipatstart+2, STRING_ANYCRLF_RIGHTPAR, 8) == 0)
    { skipatstart += 10; newnl = PCRE_NEWLINE_ANYCRLF; }

  else if (STRNCMP_UC_C8(ptr+skipatstart+2, STRING_BSR_ANYCRLF_RIGHTPAR, 12) == 0)
    { skipatstart += 14; newbsr = PCRE_BSR_ANYCRLF; }
  else if (STRNCMP_UC_C8(ptr+skipatstart+2, STRING_BSR_UNICODE_RIGHTPAR, 12) == 0)
    { skipatstart += 14; newbsr = PCRE_BSR_UNICODE; }

  if (newnl != 0)
    options = (options & ~PCRE_NEWLINE_BITS) | newnl;
  else if (newbsr != 0)
    options = (options & ~(PCRE_BSR_ANYCRLF|PCRE_BSR_UNICODE)) | newbsr;
  else break;
  }

/* PCRE_UTF(16|32) have the same value as PCRE_UTF8. */
utf = (options & PCRE_UTF8) != 0;
if (utf && never_utf)
  {
  errorcode = ERR78;
  goto PCRE_EARLY_ERROR_RETURN2;
  }

/* Can't support UTF unless PCRE has been compiled to include the code. The
return of an error code from PRIV(valid_utf)() is a new feature, introduced in
release 8.13. It is passed back from pcre_[dfa_]exec(), but at the moment is
not used here. */

#ifdef SUPPORT_UTF
if (utf && (options & PCRE_NO_UTF8_CHECK) == 0 &&
     (errorcode = PRIV(valid_utf)((PCRE_PUCHAR)pattern, -1, erroroffset)) != 0)
  {
#if defined COMPILE_PCRE8
  errorcode = ERR44;
#elif defined COMPILE_PCRE16
  errorcode = ERR74;
#elif defined COMPILE_PCRE32
  errorcode = ERR77;
#endif
  goto PCRE_EARLY_ERROR_RETURN2;
  }
#else
if (utf)
  {
  errorcode = ERR32;
  goto PCRE_EARLY_ERROR_RETURN;
  }
#endif

/* Can't support UCP unless PCRE has been compiled to include the code. */

#ifndef SUPPORT_UCP
if ((options & PCRE_UCP) != 0)
  {
  errorcode = ERR67;
  goto PCRE_EARLY_ERROR_RETURN;
  }
#endif

/* Check validity of \R options. */

if ((options & (PCRE_BSR_ANYCRLF|PCRE_BSR_UNICODE)) ==
     (PCRE_BSR_ANYCRLF|PCRE_BSR_UNICODE))
  {
  errorcode = ERR56;
  goto PCRE_EARLY_ERROR_RETURN;
  }

/* Handle different types of newline. The three bits give seven cases. The
current code allows for fixed one- or two-byte sequences, plus "any" and
"anycrlf". */

switch (options & PCRE_NEWLINE_BITS)
  {
  case 0: newline = NEWLINE; break;   /* Build-time default */
  case PCRE_NEWLINE_CR: newline = CHAR_CR; break;
  case PCRE_NEWLINE_LF: newline = CHAR_NL; break;
  case PCRE_NEWLINE_CR+
       PCRE_NEWLINE_LF: newline = (CHAR_CR << 8) | CHAR_NL; break;
  case PCRE_NEWLINE_ANY: newline = -1; break;
  case PCRE_NEWLINE_ANYCRLF: newline = -2; break;
  default: errorcode = ERR56; goto PCRE_EARLY_ERROR_RETURN;
  }

if (newline == -2)
  {
  cd->nltype = NLTYPE_ANYCRLF;
  }
else if (newline < 0)
  {
  cd->nltype = NLTYPE_ANY;
  }
else
  {
  cd->nltype = NLTYPE_FIXED;
  if (newline > 255)
    {
    cd->nllen = 2;
    cd->nl[0] = (newline >> 8) & 255;
    cd->nl[1] = newline & 255;
    }
  else
    {
    cd->nllen = 1;
    cd->nl[0] = newline;
    }
  }

/* Maximum back reference and backref bitmap. The bitmap records up to 31 back
references to help in deciding whether (.*) can be treated as anchored or not.
*/

cd->top_backref = 0;
cd->backref_map = 0;

/* Reflect pattern for debugging output */

DPRINTF(("------------------------------------------------------------------\n"));
#ifdef PCRE_DEBUG
print_puchar(stdout, (PCRE_PUCHAR)pattern);
#endif
DPRINTF(("\n"));

/* Pretend to compile the pattern while actually just accumulating the length
of memory required. This behaviour is triggered by passing a non-NULL final
argument to compile_regex(). We pass a block of workspace (cworkspace) for it
to compile parts of the pattern into; the compiled code is discarded when it is
no longer needed, so hopefully this workspace will never overflow, though there
is a test for its doing so. */

cd->bracount = cd->final_bracount = 0;
cd->names_found = 0;
cd->name_entry_size = 0;
cd->name_table = NULL;
cd->dupnames = FALSE;
cd->dupgroups = FALSE;
cd->namedrefcount = 0;
cd->start_code = cworkspace;
cd->hwm = cworkspace;
cd->iscondassert = FALSE;
cd->start_workspace = cworkspace;
cd->workspace_size = COMPILE_WORK_SIZE;
cd->named_groups = named_groups;
cd->named_group_list_size = NAMED_GROUP_LIST_SIZE;
cd->start_pattern = (const pcre_uchar *)pattern;
cd->end_pattern = (const pcre_uchar *)(pattern + STRLEN_UC((const pcre_uchar *)pattern));
cd->req_varyopt = 0;
cd->parens_depth = 0;
cd->assert_depth = 0;
cd->max_lookbehind = 0;
cd->external_options = options;
cd->open_caps = NULL;

/* Now do the pre-compile. On error, errorcode will be set non-zero, so we
don't need to look at the result of the function here. The initial options have
been put into the cd block so that they can be changed if an option setting is
found within the regex right at the beginning. Bringing initial option settings
outside can help speed up starting point checks. */

ptr += skipatstart;
code = cworkspace;
*code = OP_BRA;

(void)compile_regex(cd->external_options, &code, &ptr, &errorcode, FALSE,
  FALSE, 0, 0, &firstchar, &firstcharflags, &reqchar, &reqcharflags, NULL,
  cd, &length);
if (errorcode != 0) goto PCRE_EARLY_ERROR_RETURN;

DPRINTF(("end pre-compile: length=%d workspace=%d\n", length,
  (int)(cd->hwm - cworkspace)));

if (length > MAX_PATTERN_SIZE)
  {
  errorcode = ERR20;
  goto PCRE_EARLY_ERROR_RETURN;
  }

/* Compute the size of the data block for storing the compiled pattern. Integer
overflow should no longer be possible because nowadays we limit the maximum
value of cd->names_found and cd->name_entry_size. */

size = sizeof(REAL_PCRE) +
  (length + cd->names_found * cd->name_entry_size) * sizeof(pcre_uchar);

/* Get the memory. */

re = (REAL_PCRE *)(PUBL(malloc))(size);
if (re == NULL)
  {
  errorcode = ERR21;
  goto PCRE_EARLY_ERROR_RETURN;
  }

/* Put in the magic number, and save the sizes, initial options, internal
flags, and character table pointer. NULL is used for the default character
tables. The nullpad field is at the end; it's there to help in the case when a
regex compiled on a system with 4-byte pointers is run on another with 8-byte
pointers. */

re->magic_number = MAGIC_NUMBER;
re->size = (int)size;
re->options = cd->external_options;
re->flags = cd->external_flags;
re->limit_match = limit_match;
re->limit_recursion = limit_recursion;
re->first_char = 0;
re->req_char = 0;
re->name_table_offset = sizeof(REAL_PCRE) / sizeof(pcre_uchar);
re->name_entry_size = cd->name_entry_size;
re->name_count = cd->names_found;
re->ref_count = 0;
re->tables = (tables == PRIV(default_tables))? NULL : tables;
re->nullpad = NULL;
#ifdef COMPILE_PCRE32
re->dummy = 0;
#else
re->dummy1 = re->dummy2 = re->dummy3 = 0;
#endif

/* The starting points of the name/number translation table and of the code are
passed around in the compile data block. The start/end pattern and initial
options are already set from the pre-compile phase, as is the name_entry_size
field. Reset the bracket count and the names_found field. Also reset the hwm
field; this time it's used for remembering forward references to subpatterns.
*/

cd->final_bracount = cd->bracount;  /* Save for checking forward references */
cd->parens_depth = 0;
cd->assert_depth = 0;
cd->bracount = 0;
cd->max_lookbehind = 0;
cd->name_table = (pcre_uchar *)re + re->name_table_offset;
codestart = cd->name_table + re->name_entry_size * re->name_count;
cd->start_code = codestart;
cd->hwm = (pcre_uchar *)(cd->start_workspace);
cd->iscondassert = FALSE;
cd->req_varyopt = 0;
cd->had_accept = FALSE;
cd->had_pruneorskip = FALSE;
cd->check_lookbehind = FALSE;
cd->open_caps = NULL;

/* If any named groups were found, create the name/number table from the list
created in the first pass. */

if (cd->names_found > 0)
  {
  int i = cd->names_found;
  named_group *ng = cd->named_groups;
  cd->names_found = 0;
  for (; i > 0; i--, ng++)
    add_name(cd, ng->name, ng->length, ng->number);
  if (cd->named_group_list_size > NAMED_GROUP_LIST_SIZE)
    (PUBL(free))((void *)cd->named_groups);
  }

/* Set up a starting, non-extracting bracket, then compile the expression. On
error, errorcode will be set non-zero, so we don't need to look at the result
of the function here. */

ptr = (const pcre_uchar *)pattern + skipatstart;
code = (pcre_uchar *)codestart;
*code = OP_BRA;
(void)compile_regex(re->options, &code, &ptr, &errorcode, FALSE, FALSE, 0, 0,
  &firstchar, &firstcharflags, &reqchar, &reqcharflags, NULL, cd, NULL);
re->top_bracket = cd->bracount;
re->top_backref = cd->top_backref;
re->max_lookbehind = cd->max_lookbehind;
re->flags = cd->external_flags | PCRE_MODE;

if (cd->had_accept)
  {
  reqchar = 0;              /* Must disable after (*ACCEPT) */
  reqcharflags = REQ_NONE;
  }

/* If not reached end of pattern on success, there's an excess bracket. */

if (errorcode == 0 && *ptr != CHAR_NULL) errorcode = ERR22;

/* Fill in the terminating state and check for disastrous overflow, but
if debugging, leave the test till after things are printed out. */

*code++ = OP_END;

#ifndef PCRE_DEBUG
if (code - codestart > length) errorcode = ERR23;
#endif

#ifdef SUPPORT_VALGRIND
/* If the estimated length exceeds the really used length, mark the extra
allocated memory as unaddressable, so that any out-of-bound reads can be
detected. */
VALGRIND_MAKE_MEM_NOACCESS(code, (length - (code - codestart)) * sizeof(pcre_uchar));
#endif

/* Fill in any forward references that are required. There may be repeated
references; optimize for them, as searching a large regex takes time. */

if (cd->hwm > cd->start_workspace)
  {
  int prev_recno = -1;
  const pcre_uchar *groupptr = NULL;
  while (errorcode == 0 && cd->hwm > cd->start_workspace)
    {
    int offset, recno;
    cd->hwm -= LINK_SIZE;
    offset = GET(cd->hwm, 0);

    /* Check that the hwm handling hasn't gone wrong. This whole area is
    rewritten in PCRE2 because there are some obscure cases. */

    if (offset == 0 || codestart[offset-1] != OP_RECURSE)
      {
      errorcode = ERR10;
      break;
      }

    recno = GET(codestart, offset);
    if (recno != prev_recno)
      {
      groupptr = PRIV(find_bracket)(codestart, utf, recno);
      prev_recno = recno;
      }
    if (groupptr == NULL) errorcode = ERR53;
      else PUT(((pcre_uchar *)codestart), offset, (int)(groupptr - codestart));
    }
  }

/* If the workspace had to be expanded, free the new memory. Set the pointer to
NULL to indicate that forward references have been filled in. */

if (cd->workspace_size > COMPILE_WORK_SIZE)
  (PUBL(free))((void *)cd->start_workspace);
cd->start_workspace = NULL;

/* Give an error if there's back reference to a non-existent capturing
subpattern. */

if (errorcode == 0 && re->top_backref > re->top_bracket) errorcode = ERR15;

/* Unless disabled, check whether any single character iterators can be
auto-possessified. The function overwrites the appropriate opcode values, so
the type of the pointer must be cast. NOTE: the intermediate variable "temp" is
used in this code because at least one compiler gives a warning about loss of
"const" attribute if the cast (pcre_uchar *)codestart is used directly in the
function call. */

if (errorcode == 0 && (options & PCRE_NO_AUTO_POSSESS) == 0)
  {
  pcre_uchar *temp = (pcre_uchar *)codestart;
  auto_possessify(temp, utf, cd);
  }

/* If there were any lookbehind assertions that contained OP_RECURSE
(recursions or subroutine calls), a flag is set for them to be checked here,
because they may contain forward references. Actual recursions cannot be fixed
length, but subroutine calls can. It is done like this so that those without
OP_RECURSE that are not fixed length get a diagnosic with a useful offset. The
exceptional ones forgo this. We scan the pattern to check that they are fixed
length, and set their lengths. */

if (errorcode == 0 && cd->check_lookbehind)
  {
  pcre_uchar *cc = (pcre_uchar *)codestart;

  /* Loop, searching for OP_REVERSE items, and process those that do not have
  their length set. (Actually, it will also re-process any that have a length
  of zero, but that is a pathological case, and it does no harm.) When we find
  one, we temporarily terminate the branch it is in while we scan it. */

  for (cc = (pcre_uchar *)PRIV(find_bracket)(codestart, utf, -1);
       cc != NULL;
       cc = (pcre_uchar *)PRIV(find_bracket)(cc, utf, -1))
    {
    if (GET(cc, 1) == 0)
      {
      int fixed_length;
      pcre_uchar *be = cc - 1 - LINK_SIZE + GET(cc, -LINK_SIZE);
      int end_op = *be;
      *be = OP_END;
      fixed_length = find_fixedlength(cc, (re->options & PCRE_UTF8) != 0, TRUE,
        cd, NULL);
      *be = end_op;
      DPRINTF(("fixed length = %d\n", fixed_length));
      if (fixed_length < 0)
        {
        errorcode = (fixed_length == -2)? ERR36 :
                    (fixed_length == -4)? ERR70 : ERR25;
        break;
        }
      if (fixed_length > cd->max_lookbehind) cd->max_lookbehind = fixed_length;
      PUT(cc, 1, fixed_length);
      }
    cc += 1 + LINK_SIZE;
    }
  }

/* Failed to compile, or error while post-processing */

if (errorcode != 0)
  {
  (PUBL(free))(re);
  PCRE_EARLY_ERROR_RETURN:
  *erroroffset = (int)(ptr - (const pcre_uchar *)pattern);
  PCRE_EARLY_ERROR_RETURN2:
  *errorptr = find_error_text(errorcode);
  if (errorcodeptr != NULL) *errorcodeptr = errorcode;
  return NULL;
  }

/* If the anchored option was not passed, set the flag if we can determine that
the pattern is anchored by virtue of ^ characters or \A or anything else, such
as starting with non-atomic .* when DOTALL is set and there are no occurrences
of *PRUNE or *SKIP.

Otherwise, if we know what the first byte has to be, save it, because that
speeds up unanchored matches no end. If not, see if we can set the
PCRE_STARTLINE flag. This is helpful for multiline matches when all branches
start with ^. and also when all branches start with non-atomic .* for
non-DOTALL matches when *PRUNE and SKIP are not present. */

if ((re->options & PCRE_ANCHORED) == 0)
  {
  if (is_anchored(codestart, 0, cd, 0)) re->options |= PCRE_ANCHORED;
  else
    {
    if (firstcharflags < 0)
      firstchar = find_firstassertedchar(codestart, &firstcharflags, FALSE);
    if (firstcharflags >= 0)   /* Remove caseless flag for non-caseable chars */
      {
#if defined COMPILE_PCRE8
      re->first_char = firstchar & 0xff;
#elif defined COMPILE_PCRE16
      re->first_char = firstchar & 0xffff;
#elif defined COMPILE_PCRE32
      re->first_char = firstchar;
#endif
      if ((firstcharflags & REQ_CASELESS) != 0)
        {
#if defined SUPPORT_UCP && !(defined COMPILE_PCRE8)
        /* We ignore non-ASCII first chars in 8 bit mode. */
        if (utf)
          {
          if (re->first_char < 128)
            {
            if (cd->fcc[re->first_char] != re->first_char)
              re->flags |= PCRE_FCH_CASELESS;
            }
          else if (UCD_OTHERCASE(re->first_char) != re->first_char)
            re->flags |= PCRE_FCH_CASELESS;
          }
        else
#endif
        if (MAX_255(re->first_char)
            && cd->fcc[re->first_char] != re->first_char)
          re->flags |= PCRE_FCH_CASELESS;
        }

      re->flags |= PCRE_FIRSTSET;
      }

    else if (is_startline(codestart, 0, cd, 0)) re->flags |= PCRE_STARTLINE;
    }
  }

/* For an anchored pattern, we use the "required byte" only if it follows a
variable length item in the regex. Remove the caseless flag for non-caseable
bytes. */

if (reqcharflags >= 0 &&
     ((re->options & PCRE_ANCHORED) == 0 || (reqcharflags & REQ_VARY) != 0))
  {
#if defined COMPILE_PCRE8
  re->req_char = reqchar & 0xff;
#elif defined COMPILE_PCRE16
  re->req_char = reqchar & 0xffff;
#elif defined COMPILE_PCRE32
  re->req_char = reqchar;
#endif
  if ((reqcharflags & REQ_CASELESS) != 0)
    {
#if defined SUPPORT_UCP && !(defined COMPILE_PCRE8)
    /* We ignore non-ASCII first chars in 8 bit mode. */
    if (utf)
      {
      if (re->req_char < 128)
        {
        if (cd->fcc[re->req_char] != re->req_char)
          re->flags |= PCRE_RCH_CASELESS;
        }
      else if (UCD_OTHERCASE(re->req_char) != re->req_char)
        re->flags |= PCRE_RCH_CASELESS;
      }
    else
#endif
    if (MAX_255(re->req_char) && cd->fcc[re->req_char] != re->req_char)
      re->flags |= PCRE_RCH_CASELESS;
    }

  re->flags |= PCRE_REQCHSET;
  }

/* Print out the compiled data if debugging is enabled. This is never the
case when building a production library. */

#ifdef PCRE_DEBUG
printf("Length = %d top_bracket = %d top_backref = %d\n",
  length, re->top_bracket, re->top_backref);

printf("Options=%08x\n", re->options);

if ((re->flags & PCRE_FIRSTSET) != 0)
  {
  pcre_uchar ch = re->first_char;
  const char *caseless =
    ((re->flags & PCRE_FCH_CASELESS) == 0)? "" : " (caseless)";
  if (PRINTABLE(ch)) printf("First char = %c%s\n", ch, caseless);
    else printf("First char = \\x%02x%s\n", ch, caseless);
  }

if ((re->flags & PCRE_REQCHSET) != 0)
  {
  pcre_uchar ch = re->req_char;
  const char *caseless =
    ((re->flags & PCRE_RCH_CASELESS) == 0)? "" : " (caseless)";
  if (PRINTABLE(ch)) printf("Req char = %c%s\n", ch, caseless);
    else printf("Req char = \\x%02x%s\n", ch, caseless);
  }

#if defined COMPILE_PCRE8
pcre_printint((pcre *)re, stdout, TRUE);
#elif defined COMPILE_PCRE16
pcre16_printint((pcre *)re, stdout, TRUE);
#elif defined COMPILE_PCRE32
pcre32_printint((pcre *)re, stdout, TRUE);
#endif

/* This check is done here in the debugging case so that the code that
was compiled can be seen. */

if (code - codestart > length)
  {
  (PUBL(free))(re);
  *errorptr = find_error_text(ERR23);
  *erroroffset = ptr - (pcre_uchar *)pattern;
  if (errorcodeptr != NULL) *errorcodeptr = ERR23;
  return NULL;
  }
#endif   /* PCRE_DEBUG */

/* Check for a pattern than can match an empty string, so that this information
can be provided to applications. */

do
  {
  if (could_be_empty_branch(codestart, code, utf, cd, NULL))
    {
    re->flags |= PCRE_MATCH_EMPTY;
    break;
    }
  codestart += GET(codestart, 1);
  }
while (*codestart == OP_ALT);

#if defined COMPILE_PCRE8
return (pcre *)re;
#elif defined COMPILE_PCRE16
return (pcre16 *)re;
#elif defined COMPILE_PCRE32
return (pcre32 *)re;
#endif
}

/* End of pcre_compile.c */

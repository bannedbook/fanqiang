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

/* This module contains pcre_exec(), the externally visible function that does
pattern matching using an NFA algorithm, trying to mimic Perl as closely as
possible. There are also some static supporting functions. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define NLBLOCK md             /* Block containing newline information */
#define PSSTART start_subject  /* Field containing processed string start */
#define PSEND   end_subject    /* Field containing processed string end */

#include "pcre_internal.h"

/* Undefine some potentially clashing cpp symbols */

#undef min
#undef max

/* The md->capture_last field uses the lower 16 bits for the last captured
substring (which can never be greater than 65535) and a bit in the top half
to mean "capture vector overflowed". This odd way of doing things was
implemented when it was realized that preserving and restoring the overflow bit
whenever the last capture number was saved/restored made for a neater
interface, and doing it this way saved on (a) another variable, which would
have increased the stack frame size (a big NO-NO in PCRE) and (b) another
separate set of save/restore instructions. The following defines are used in
implementing this. */

#define CAPLMASK    0x0000ffff    /* The bits used for last_capture */
#define OVFLMASK    0xffff0000    /* The bits used for the overflow flag */
#define OVFLBIT     0x00010000    /* The bit that is set for overflow */

/* Values for setting in md->match_function_type to indicate two special types
of call to match(). We do it this way to save on using another stack variable,
as stack usage is to be discouraged. */

#define MATCH_CONDASSERT     1  /* Called to check a condition assertion */
#define MATCH_CBEGROUP       2  /* Could-be-empty unlimited repeat group */

/* Non-error returns from the match() function. Error returns are externally
defined PCRE_ERROR_xxx codes, which are all negative. */

#define MATCH_MATCH        1
#define MATCH_NOMATCH      0

/* Special internal returns from the match() function. Make them sufficiently
negative to avoid the external error codes. */

#define MATCH_ACCEPT       (-999)
#define MATCH_KETRPOS      (-998)
#define MATCH_ONCE         (-997)
/* The next 5 must be kept together and in sequence so that a test that checks
for any one of them can use a range. */
#define MATCH_COMMIT       (-996)
#define MATCH_PRUNE        (-995)
#define MATCH_SKIP         (-994)
#define MATCH_SKIP_ARG     (-993)
#define MATCH_THEN         (-992)
#define MATCH_BACKTRACK_MAX MATCH_THEN
#define MATCH_BACKTRACK_MIN MATCH_COMMIT

/* Maximum number of ints of offset to save on the stack for recursive calls.
If the offset vector is bigger, malloc is used. This should be a multiple of 3,
because the offset vector is always a multiple of 3 long. */

#define REC_STACK_SAVE_MAX 30

/* Min and max values for the common repeats; for the maxima, 0 => infinity */

static const char rep_min[] = { 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, };
static const char rep_max[] = { 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, };

#ifdef PCRE_DEBUG
/*************************************************
*        Debugging function to print chars       *
*************************************************/

/* Print a sequence of chars in printable format, stopping at the end of the
subject if the requested.

Arguments:
  p           points to characters
  length      number to print
  is_subject  TRUE if printing from within md->start_subject
  md          pointer to matching data block, if is_subject is TRUE

Returns:     nothing
*/

static void
pchars(const pcre_uchar *p, int length, BOOL is_subject, match_data *md)
{
pcre_uint32 c;
BOOL utf = md->utf;
if (is_subject && length > md->end_subject - p) length = md->end_subject - p;
while (length-- > 0)
  if (isprint(c = UCHAR21INCTEST(p))) printf("%c", (char)c); else printf("\\x{%02x}", c);
}
#endif



/*************************************************
*          Match a back-reference                *
*************************************************/

/* Normally, if a back reference hasn't been set, the length that is passed is
negative, so the match always fails. However, in JavaScript compatibility mode,
the length passed is zero. Note that in caseless UTF-8 mode, the number of
subject bytes matched may be different to the number of reference bytes.

Arguments:
  offset      index into the offset vector
  eptr        pointer into the subject
  length      length of reference to be matched (number of bytes)
  md          points to match data block
  caseless    TRUE if caseless

Returns:      >= 0 the number of subject bytes matched
              -1 no match
              -2 partial match; always given if at end subject
*/

static int
match_ref(int offset, register PCRE_PUCHAR eptr, int length, match_data *md,
  BOOL caseless)
{
PCRE_PUCHAR eptr_start = eptr;
register PCRE_PUCHAR p = md->start_subject + md->offset_vector[offset];
#if defined SUPPORT_UTF && defined SUPPORT_UCP
BOOL utf = md->utf;
#endif

#ifdef PCRE_DEBUG
if (eptr >= md->end_subject)
  printf("matching subject <null>");
else
  {
  printf("matching subject ");
  pchars(eptr, length, TRUE, md);
  }
printf(" against backref ");
pchars(p, length, FALSE, md);
printf("\n");
#endif

/* Always fail if reference not set (and not JavaScript compatible - in that
case the length is passed as zero). */

if (length < 0) return -1;

/* Separate the caseless case for speed. In UTF-8 mode we can only do this
properly if Unicode properties are supported. Otherwise, we can check only
ASCII characters. */

if (caseless)
  {
#if defined SUPPORT_UTF && defined SUPPORT_UCP
  if (utf)
    {
    /* Match characters up to the end of the reference. NOTE: the number of
    data units matched may differ, because in UTF-8 there are some characters
    whose upper and lower case versions code have different numbers of bytes.
    For example, U+023A (2 bytes in UTF-8) is the upper case version of U+2C65
    (3 bytes in UTF-8); a sequence of 3 of the former uses 6 bytes, as does a
    sequence of two of the latter. It is important, therefore, to check the
    length along the reference, not along the subject (earlier code did this
    wrong). */

    PCRE_PUCHAR endptr = p + length;
    while (p < endptr)
      {
      pcre_uint32 c, d;
      const ucd_record *ur;
      if (eptr >= md->end_subject) return -2;   /* Partial match */
      GETCHARINC(c, eptr);
      GETCHARINC(d, p);
      ur = GET_UCD(d);
      if (c != d && c != d + ur->other_case)
        {
        const pcre_uint32 *pp = PRIV(ucd_caseless_sets) + ur->caseset;
        for (;;)
          {
          if (c < *pp) return -1;
          if (c == *pp++) break;
          }
        }
      }
    }
  else
#endif

  /* The same code works when not in UTF-8 mode and in UTF-8 mode when there
  is no UCP support. */
    {
    while (length-- > 0)
      {
      pcre_uint32 cc, cp;
      if (eptr >= md->end_subject) return -2;   /* Partial match */
      cc = UCHAR21TEST(eptr);
      cp = UCHAR21TEST(p);
      if (TABLE_GET(cp, md->lcc, cp) != TABLE_GET(cc, md->lcc, cc)) return -1;
      p++;
      eptr++;
      }
    }
  }

/* In the caseful case, we can just compare the bytes, whether or not we
are in UTF-8 mode. */

else
  {
  while (length-- > 0)
    {
    if (eptr >= md->end_subject) return -2;   /* Partial match */
    if (UCHAR21INCTEST(p) != UCHAR21INCTEST(eptr)) return -1;
    }
  }

return (int)(eptr - eptr_start);
}



/***************************************************************************
****************************************************************************
                   RECURSION IN THE match() FUNCTION

The match() function is highly recursive, though not every recursive call
increases the recursive depth. Nevertheless, some regular expressions can cause
it to recurse to a great depth. I was writing for Unix, so I just let it call
itself recursively. This uses the stack for saving everything that has to be
saved for a recursive call. On Unix, the stack can be large, and this works
fine.

It turns out that on some non-Unix-like systems there are problems with
programs that use a lot of stack. (This despite the fact that every last chip
has oodles of memory these days, and techniques for extending the stack have
been known for decades.) So....

There is a fudge, triggered by defining NO_RECURSE, which avoids recursive
calls by keeping local variables that need to be preserved in blocks of memory
obtained from malloc() instead instead of on the stack. Macros are used to
achieve this so that the actual code doesn't look very different to what it
always used to.

The original heap-recursive code used longjmp(). However, it seems that this
can be very slow on some operating systems. Following a suggestion from Stan
Switzer, the use of longjmp() has been abolished, at the cost of having to
provide a unique number for each call to RMATCH. There is no way of generating
a sequence of numbers at compile time in C. I have given them names, to make
them stand out more clearly.

Crude tests on x86 Linux show a small speedup of around 5-8%. However, on
FreeBSD, avoiding longjmp() more than halves the time taken to run the standard
tests. Furthermore, not using longjmp() means that local dynamic variables
don't have indeterminate values; this has meant that the frame size can be
reduced because the result can be "passed back" by straight setting of the
variable instead of being passed in the frame.
****************************************************************************
***************************************************************************/

/* Numbers for RMATCH calls. When this list is changed, the code at HEAP_RETURN
below must be updated in sync.  */

enum { RM1=1, RM2,  RM3,  RM4,  RM5,  RM6,  RM7,  RM8,  RM9,  RM10,
       RM11,  RM12, RM13, RM14, RM15, RM16, RM17, RM18, RM19, RM20,
       RM21,  RM22, RM23, RM24, RM25, RM26, RM27, RM28, RM29, RM30,
       RM31,  RM32, RM33, RM34, RM35, RM36, RM37, RM38, RM39, RM40,
       RM41,  RM42, RM43, RM44, RM45, RM46, RM47, RM48, RM49, RM50,
       RM51,  RM52, RM53, RM54, RM55, RM56, RM57, RM58, RM59, RM60,
       RM61,  RM62, RM63, RM64, RM65, RM66, RM67 };

/* These versions of the macros use the stack, as normal. There are debugging
versions and production versions. Note that the "rw" argument of RMATCH isn't
actually used in this definition. */

#ifndef NO_RECURSE
#define REGISTER register

#ifdef PCRE_DEBUG
#define RMATCH(ra,rb,rc,rd,re,rw) \
  { \
  printf("match() called in line %d\n", __LINE__); \
  rrc = match(ra,rb,mstart,rc,rd,re,rdepth+1); \
  printf("to line %d\n", __LINE__); \
  }
#define RRETURN(ra) \
  { \
  printf("match() returned %d from line %d\n", ra, __LINE__); \
  return ra; \
  }
#else
#define RMATCH(ra,rb,rc,rd,re,rw) \
  rrc = match(ra,rb,mstart,rc,rd,re,rdepth+1)
#define RRETURN(ra) return ra
#endif

#else


/* These versions of the macros manage a private stack on the heap. Note that
the "rd" argument of RMATCH isn't actually used in this definition. It's the md
argument of match(), which never changes. */

#define REGISTER

#define RMATCH(ra,rb,rc,rd,re,rw)\
  {\
  heapframe *newframe = frame->Xnextframe;\
  if (newframe == NULL)\
    {\
    newframe = (heapframe *)(PUBL(stack_malloc))(sizeof(heapframe));\
    if (newframe == NULL) RRETURN(PCRE_ERROR_NOMEMORY);\
    newframe->Xnextframe = NULL;\
    frame->Xnextframe = newframe;\
    }\
  frame->Xwhere = rw;\
  newframe->Xeptr = ra;\
  newframe->Xecode = rb;\
  newframe->Xmstart = mstart;\
  newframe->Xoffset_top = rc;\
  newframe->Xeptrb = re;\
  newframe->Xrdepth = frame->Xrdepth + 1;\
  newframe->Xprevframe = frame;\
  frame = newframe;\
  DPRINTF(("restarting from line %d\n", __LINE__));\
  goto HEAP_RECURSE;\
  L_##rw:\
  DPRINTF(("jumped back to line %d\n", __LINE__));\
  }

#define RRETURN(ra)\
  {\
  heapframe *oldframe = frame;\
  frame = oldframe->Xprevframe;\
  if (frame != NULL)\
    {\
    rrc = ra;\
    goto HEAP_RETURN;\
    }\
  return ra;\
  }


/* Structure for remembering the local variables in a private frame */

typedef struct heapframe {
  struct heapframe *Xprevframe;
  struct heapframe *Xnextframe;

  /* Function arguments that may change */

  PCRE_PUCHAR Xeptr;
  const pcre_uchar *Xecode;
  PCRE_PUCHAR Xmstart;
  int Xoffset_top;
  eptrblock *Xeptrb;
  unsigned int Xrdepth;

  /* Function local variables */

  PCRE_PUCHAR Xcallpat;
#ifdef SUPPORT_UTF
  PCRE_PUCHAR Xcharptr;
#endif
  PCRE_PUCHAR Xdata;
  PCRE_PUCHAR Xnext;
  PCRE_PUCHAR Xpp;
  PCRE_PUCHAR Xprev;
  PCRE_PUCHAR Xsaved_eptr;

  recursion_info Xnew_recursive;

  BOOL Xcur_is_word;
  BOOL Xcondition;
  BOOL Xprev_is_word;

#ifdef SUPPORT_UCP
  int Xprop_type;
  unsigned int Xprop_value;
  int Xprop_fail_result;
  int Xoclength;
  pcre_uchar Xocchars[6];
#endif

  int Xcodelink;
  int Xctype;
  unsigned int Xfc;
  int Xfi;
  int Xlength;
  int Xmax;
  int Xmin;
  unsigned int Xnumber;
  int Xoffset;
  unsigned int Xop;
  pcre_int32 Xsave_capture_last;
  int Xsave_offset1, Xsave_offset2, Xsave_offset3;
  int Xstacksave[REC_STACK_SAVE_MAX];

  eptrblock Xnewptrb;

  /* Where to jump back to */

  int Xwhere;

} heapframe;

#endif


/***************************************************************************
***************************************************************************/



/*************************************************
*         Match from current position            *
*************************************************/

/* This function is called recursively in many circumstances. Whenever it
returns a negative (error) response, the outer incarnation must also return the
same response. */

/* These macros pack up tests that are used for partial matching, and which
appear several times in the code. We set the "hit end" flag if the pointer is
at the end of the subject and also past the start of the subject (i.e.
something has been matched). For hard partial matching, we then return
immediately. The second one is used when we already know we are past the end of
the subject. */

#define CHECK_PARTIAL()\
  if (md->partial != 0 && eptr >= md->end_subject && \
      eptr > md->start_used_ptr) \
    { \
    md->hitend = TRUE; \
    if (md->partial > 1) RRETURN(PCRE_ERROR_PARTIAL); \
    }

#define SCHECK_PARTIAL()\
  if (md->partial != 0 && eptr > md->start_used_ptr) \
    { \
    md->hitend = TRUE; \
    if (md->partial > 1) RRETURN(PCRE_ERROR_PARTIAL); \
    }


/* Performance note: It might be tempting to extract commonly used fields from
the md structure (e.g. utf, end_subject) into individual variables to improve
performance. Tests using gcc on a SPARC disproved this; in the first case, it
made performance worse.

Arguments:
   eptr        pointer to current character in subject
   ecode       pointer to current position in compiled code
   mstart      pointer to the current match start position (can be modified
                 by encountering \K)
   offset_top  current top pointer
   md          pointer to "static" info for the match
   eptrb       pointer to chain of blocks containing eptr at start of
                 brackets - for testing for empty matches
   rdepth      the recursion depth

Returns:       MATCH_MATCH if matched            )  these values are >= 0
               MATCH_NOMATCH if failed to match  )
               a negative MATCH_xxx value for PRUNE, SKIP, etc
               a negative PCRE_ERROR_xxx value if aborted by an error condition
                 (e.g. stopped by repeated call or recursion limit)
*/

static int
match(REGISTER PCRE_PUCHAR eptr, REGISTER const pcre_uchar *ecode,
  PCRE_PUCHAR mstart, int offset_top, match_data *md, eptrblock *eptrb,
  unsigned int rdepth)
{
/* These variables do not need to be preserved over recursion in this function,
so they can be ordinary variables in all cases. Mark some of them with
"register" because they are used a lot in loops. */

register int  rrc;         /* Returns from recursive calls */
register int  i;           /* Used for loops not involving calls to RMATCH() */
register pcre_uint32 c;    /* Character values not kept over RMATCH() calls */
register BOOL utf;         /* Local copy of UTF flag for speed */

BOOL minimize, possessive; /* Quantifier options */
BOOL caseless;
int condcode;

/* When recursion is not being used, all "local" variables that have to be
preserved over calls to RMATCH() are part of a "frame". We set up the top-level
frame on the stack here; subsequent instantiations are obtained from the heap
whenever RMATCH() does a "recursion". See the macro definitions above. Putting
the top-level on the stack rather than malloc-ing them all gives a performance
boost in many cases where there is not much "recursion". */

#ifdef NO_RECURSE
heapframe *frame = (heapframe *)md->match_frames_base;

/* Copy in the original argument variables */

frame->Xeptr = eptr;
frame->Xecode = ecode;
frame->Xmstart = mstart;
frame->Xoffset_top = offset_top;
frame->Xeptrb = eptrb;
frame->Xrdepth = rdepth;

/* This is where control jumps back to to effect "recursion" */

HEAP_RECURSE:

/* Macros make the argument variables come from the current frame */

#define eptr               frame->Xeptr
#define ecode              frame->Xecode
#define mstart             frame->Xmstart
#define offset_top         frame->Xoffset_top
#define eptrb              frame->Xeptrb
#define rdepth             frame->Xrdepth

/* Ditto for the local variables */

#ifdef SUPPORT_UTF
#define charptr            frame->Xcharptr
#endif
#define callpat            frame->Xcallpat
#define codelink           frame->Xcodelink
#define data               frame->Xdata
#define next               frame->Xnext
#define pp                 frame->Xpp
#define prev               frame->Xprev
#define saved_eptr         frame->Xsaved_eptr

#define new_recursive      frame->Xnew_recursive

#define cur_is_word        frame->Xcur_is_word
#define condition          frame->Xcondition
#define prev_is_word       frame->Xprev_is_word

#ifdef SUPPORT_UCP
#define prop_type          frame->Xprop_type
#define prop_value         frame->Xprop_value
#define prop_fail_result   frame->Xprop_fail_result
#define oclength           frame->Xoclength
#define occhars            frame->Xocchars
#endif

#define ctype              frame->Xctype
#define fc                 frame->Xfc
#define fi                 frame->Xfi
#define length             frame->Xlength
#define max                frame->Xmax
#define min                frame->Xmin
#define number             frame->Xnumber
#define offset             frame->Xoffset
#define op                 frame->Xop
#define save_capture_last  frame->Xsave_capture_last
#define save_offset1       frame->Xsave_offset1
#define save_offset2       frame->Xsave_offset2
#define save_offset3       frame->Xsave_offset3
#define stacksave          frame->Xstacksave

#define newptrb            frame->Xnewptrb

/* When recursion is being used, local variables are allocated on the stack and
get preserved during recursion in the normal way. In this environment, fi and
i, and fc and c, can be the same variables. */

#else         /* NO_RECURSE not defined */
#define fi i
#define fc c

/* Many of the following variables are used only in small blocks of the code.
My normal style of coding would have declared them within each of those blocks.
However, in order to accommodate the version of this code that uses an external
"stack" implemented on the heap, it is easier to declare them all here, so the
declarations can be cut out in a block. The only declarations within blocks
below are for variables that do not have to be preserved over a recursive call
to RMATCH(). */

#ifdef SUPPORT_UTF
const pcre_uchar *charptr;
#endif
const pcre_uchar *callpat;
const pcre_uchar *data;
const pcre_uchar *next;
PCRE_PUCHAR       pp;
const pcre_uchar *prev;
PCRE_PUCHAR       saved_eptr;

recursion_info new_recursive;

BOOL cur_is_word;
BOOL condition;
BOOL prev_is_word;

#ifdef SUPPORT_UCP
int prop_type;
unsigned int prop_value;
int prop_fail_result;
int oclength;
pcre_uchar occhars[6];
#endif

int codelink;
int ctype;
int length;
int max;
int min;
unsigned int number;
int offset;
unsigned int op;
pcre_int32 save_capture_last;
int save_offset1, save_offset2, save_offset3;
int stacksave[REC_STACK_SAVE_MAX];

eptrblock newptrb;

/* There is a special fudge for calling match() in a way that causes it to
measure the size of its basic stack frame when the stack is being used for
recursion. The second argument (ecode) being NULL triggers this behaviour. It
cannot normally ever be NULL. The return is the negated value of the frame
size. */

if (ecode == NULL)
  {
  if (rdepth == 0)
    return match((PCRE_PUCHAR)&rdepth, NULL, NULL, 0, NULL, NULL, 1);
  else
    {
    int len = (char *)&rdepth - (char *)eptr;
    return (len > 0)? -len : len;
    }
  }
#endif     /* NO_RECURSE */

/* To save space on the stack and in the heap frame, I have doubled up on some
of the local variables that are used only in localised parts of the code, but
still need to be preserved over recursive calls of match(). These macros define
the alternative names that are used. */

#define allow_zero    cur_is_word
#define cbegroup      condition
#define code_offset   codelink
#define condassert    condition
#define matched_once  prev_is_word
#define foc           number
#define save_mark     data

/* These statements are here to stop the compiler complaining about unitialized
variables. */

#ifdef SUPPORT_UCP
prop_value = 0;
prop_fail_result = 0;
#endif


/* This label is used for tail recursion, which is used in a few cases even
when NO_RECURSE is not defined, in order to reduce the amount of stack that is
used. Thanks to Ian Taylor for noticing this possibility and sending the
original patch. */

TAIL_RECURSE:

/* OK, now we can get on with the real code of the function. Recursive calls
are specified by the macro RMATCH and RRETURN is used to return. When
NO_RECURSE is *not* defined, these just turn into a recursive call to match()
and a "return", respectively (possibly with some debugging if PCRE_DEBUG is
defined). However, RMATCH isn't like a function call because it's quite a
complicated macro. It has to be used in one particular way. This shouldn't,
however, impact performance when true recursion is being used. */

#ifdef SUPPORT_UTF
utf = md->utf;       /* Local copy of the flag */
#else
utf = FALSE;
#endif

/* First check that we haven't called match() too many times, or that we
haven't exceeded the recursive call limit. */

if (md->match_call_count++ >= md->match_limit) RRETURN(PCRE_ERROR_MATCHLIMIT);
if (rdepth >= md->match_limit_recursion) RRETURN(PCRE_ERROR_RECURSIONLIMIT);

/* At the start of a group with an unlimited repeat that may match an empty
string, the variable md->match_function_type is set to MATCH_CBEGROUP. It is
done this way to save having to use another function argument, which would take
up space on the stack. See also MATCH_CONDASSERT below.

When MATCH_CBEGROUP is set, add the current subject pointer to the chain of
such remembered pointers, to be checked when we hit the closing ket, in order
to break infinite loops that match no characters. When match() is called in
other circumstances, don't add to the chain. The MATCH_CBEGROUP feature must
NOT be used with tail recursion, because the memory block that is used is on
the stack, so a new one may be required for each match(). */

if (md->match_function_type == MATCH_CBEGROUP)
  {
  newptrb.epb_saved_eptr = eptr;
  newptrb.epb_prev = eptrb;
  eptrb = &newptrb;
  md->match_function_type = 0;
  }

/* Now start processing the opcodes. */

for (;;)
  {
  minimize = possessive = FALSE;
  op = *ecode;

  switch(op)
    {
    case OP_MARK:
    md->nomatch_mark = ecode + 2;
    md->mark = NULL;    /* In case previously set by assertion */
    RMATCH(eptr, ecode + PRIV(OP_lengths)[*ecode] + ecode[1], offset_top, md,
      eptrb, RM55);
    if ((rrc == MATCH_MATCH || rrc == MATCH_ACCEPT) &&
         md->mark == NULL) md->mark = ecode + 2;

    /* A return of MATCH_SKIP_ARG means that matching failed at SKIP with an
    argument, and we must check whether that argument matches this MARK's
    argument. It is passed back in md->start_match_ptr (an overloading of that
    variable). If it does match, we reset that variable to the current subject
    position and return MATCH_SKIP. Otherwise, pass back the return code
    unaltered. */

    else if (rrc == MATCH_SKIP_ARG &&
        STRCMP_UC_UC_TEST(ecode + 2, md->start_match_ptr) == 0)
      {
      md->start_match_ptr = eptr;
      RRETURN(MATCH_SKIP);
      }
    RRETURN(rrc);

    case OP_FAIL:
    RRETURN(MATCH_NOMATCH);

    case OP_COMMIT:
    RMATCH(eptr, ecode + PRIV(OP_lengths)[*ecode], offset_top, md,
      eptrb, RM52);
    if (rrc != MATCH_NOMATCH) RRETURN(rrc);
    RRETURN(MATCH_COMMIT);

    case OP_PRUNE:
    RMATCH(eptr, ecode + PRIV(OP_lengths)[*ecode], offset_top, md,
      eptrb, RM51);
    if (rrc != MATCH_NOMATCH) RRETURN(rrc);
    RRETURN(MATCH_PRUNE);

    case OP_PRUNE_ARG:
    md->nomatch_mark = ecode + 2;
    md->mark = NULL;    /* In case previously set by assertion */
    RMATCH(eptr, ecode + PRIV(OP_lengths)[*ecode] + ecode[1], offset_top, md,
      eptrb, RM56);
    if ((rrc == MATCH_MATCH || rrc == MATCH_ACCEPT) &&
         md->mark == NULL) md->mark = ecode + 2;
    if (rrc != MATCH_NOMATCH) RRETURN(rrc);
    RRETURN(MATCH_PRUNE);

    case OP_SKIP:
    RMATCH(eptr, ecode + PRIV(OP_lengths)[*ecode], offset_top, md,
      eptrb, RM53);
    if (rrc != MATCH_NOMATCH) RRETURN(rrc);
    md->start_match_ptr = eptr;   /* Pass back current position */
    RRETURN(MATCH_SKIP);

    /* Note that, for Perl compatibility, SKIP with an argument does NOT set
    nomatch_mark. When a pattern match ends with a SKIP_ARG for which there was
    not a matching mark, we have to re-run the match, ignoring the SKIP_ARG
    that failed and any that precede it (either they also failed, or were not
    triggered). To do this, we maintain a count of executed SKIP_ARGs. If a
    SKIP_ARG gets to top level, the match is re-run with md->ignore_skip_arg
    set to the count of the one that failed. */

    case OP_SKIP_ARG:
    md->skip_arg_count++;
    if (md->skip_arg_count <= md->ignore_skip_arg)
      {
      ecode += PRIV(OP_lengths)[*ecode] + ecode[1];
      break;
      }
    RMATCH(eptr, ecode + PRIV(OP_lengths)[*ecode] + ecode[1], offset_top, md,
      eptrb, RM57);
    if (rrc != MATCH_NOMATCH) RRETURN(rrc);

    /* Pass back the current skip name by overloading md->start_match_ptr and
    returning the special MATCH_SKIP_ARG return code. This will either be
    caught by a matching MARK, or get to the top, where it causes a rematch
    with md->ignore_skip_arg set to the value of md->skip_arg_count. */

    md->start_match_ptr = ecode + 2;
    RRETURN(MATCH_SKIP_ARG);

    /* For THEN (and THEN_ARG) we pass back the address of the opcode, so that
    the branch in which it occurs can be determined. Overload the start of
    match pointer to do this. */

    case OP_THEN:
    RMATCH(eptr, ecode + PRIV(OP_lengths)[*ecode], offset_top, md,
      eptrb, RM54);
    if (rrc != MATCH_NOMATCH) RRETURN(rrc);
    md->start_match_ptr = ecode;
    RRETURN(MATCH_THEN);

    case OP_THEN_ARG:
    md->nomatch_mark = ecode + 2;
    md->mark = NULL;    /* In case previously set by assertion */
    RMATCH(eptr, ecode + PRIV(OP_lengths)[*ecode] + ecode[1], offset_top,
      md, eptrb, RM58);
    if ((rrc == MATCH_MATCH || rrc == MATCH_ACCEPT) &&
         md->mark == NULL) md->mark = ecode + 2;
    if (rrc != MATCH_NOMATCH) RRETURN(rrc);
    md->start_match_ptr = ecode;
    RRETURN(MATCH_THEN);

    /* Handle an atomic group that does not contain any capturing parentheses.
    This can be handled like an assertion. Prior to 8.13, all atomic groups
    were handled this way. In 8.13, the code was changed as below for ONCE, so
    that backups pass through the group and thereby reset captured values.
    However, this uses a lot more stack, so in 8.20, atomic groups that do not
    contain any captures generate OP_ONCE_NC, which can be handled in the old,
    less stack intensive way.

    Check the alternative branches in turn - the matching won't pass the KET
    for this kind of subpattern. If any one branch matches, we carry on as at
    the end of a normal bracket, leaving the subject pointer, but resetting
    the start-of-match value in case it was changed by \K. */

    case OP_ONCE_NC:
    prev = ecode;
    saved_eptr = eptr;
    save_mark = md->mark;
    do
      {
      RMATCH(eptr, ecode + 1 + LINK_SIZE, offset_top, md, eptrb, RM64);
      if (rrc == MATCH_MATCH)  /* Note: _not_ MATCH_ACCEPT */
        {
        mstart = md->start_match_ptr;
        break;
        }
      if (rrc == MATCH_THEN)
        {
        next = ecode + GET(ecode,1);
        if (md->start_match_ptr < next &&
            (*ecode == OP_ALT || *next == OP_ALT))
          rrc = MATCH_NOMATCH;
        }

      if (rrc != MATCH_NOMATCH) RRETURN(rrc);
      ecode += GET(ecode,1);
      md->mark = save_mark;
      }
    while (*ecode == OP_ALT);

    /* If hit the end of the group (which could be repeated), fail */

    if (*ecode != OP_ONCE_NC && *ecode != OP_ALT) RRETURN(MATCH_NOMATCH);

    /* Continue as from after the group, updating the offsets high water
    mark, since extracts may have been taken. */

    do ecode += GET(ecode, 1); while (*ecode == OP_ALT);

    offset_top = md->end_offset_top;
    eptr = md->end_match_ptr;

    /* For a non-repeating ket, just continue at this level. This also
    happens for a repeating ket if no characters were matched in the group.
    This is the forcible breaking of infinite loops as implemented in Perl
    5.005. */

    if (*ecode == OP_KET || eptr == saved_eptr)
      {
      ecode += 1+LINK_SIZE;
      break;
      }

    /* The repeating kets try the rest of the pattern or restart from the
    preceding bracket, in the appropriate order. The second "call" of match()
    uses tail recursion, to avoid using another stack frame. */

    if (*ecode == OP_KETRMIN)
      {
      RMATCH(eptr, ecode + 1 + LINK_SIZE, offset_top, md, eptrb, RM65);
      if (rrc != MATCH_NOMATCH) RRETURN(rrc);
      ecode = prev;
      goto TAIL_RECURSE;
      }
    else  /* OP_KETRMAX */
      {
      RMATCH(eptr, prev, offset_top, md, eptrb, RM66);
      if (rrc != MATCH_NOMATCH) RRETURN(rrc);
      ecode += 1 + LINK_SIZE;
      goto TAIL_RECURSE;
      }
    /* Control never gets here */

    /* Handle a capturing bracket, other than those that are possessive with an
    unlimited repeat. If there is space in the offset vector, save the current
    subject position in the working slot at the top of the vector. We mustn't
    change the current values of the data slot, because they may be set from a
    previous iteration of this group, and be referred to by a reference inside
    the group. A failure to match might occur after the group has succeeded,
    if something later on doesn't match. For this reason, we need to restore
    the working value and also the values of the final offsets, in case they
    were set by a previous iteration of the same bracket.

    If there isn't enough space in the offset vector, treat this as if it were
    a non-capturing bracket. Don't worry about setting the flag for the error
    case here; that is handled in the code for KET. */

    case OP_CBRA:
    case OP_SCBRA:
    number = GET2(ecode, 1+LINK_SIZE);
    offset = number << 1;

#ifdef PCRE_DEBUG
    printf("start bracket %d\n", number);
    printf("subject=");
    pchars(eptr, 16, TRUE, md);
    printf("\n");
#endif

    if (offset < md->offset_max)
      {
      save_offset1 = md->offset_vector[offset];
      save_offset2 = md->offset_vector[offset+1];
      save_offset3 = md->offset_vector[md->offset_end - number];
      save_capture_last = md->capture_last;
      save_mark = md->mark;

      DPRINTF(("saving %d %d %d\n", save_offset1, save_offset2, save_offset3));
      md->offset_vector[md->offset_end - number] =
        (int)(eptr - md->start_subject);

      for (;;)
        {
        if (op >= OP_SBRA) md->match_function_type = MATCH_CBEGROUP;
        RMATCH(eptr, ecode + PRIV(OP_lengths)[*ecode], offset_top, md,
          eptrb, RM1);
        if (rrc == MATCH_ONCE) break;  /* Backing up through an atomic group */

        /* If we backed up to a THEN, check whether it is within the current
        branch by comparing the address of the THEN that is passed back with
        the end of the branch. If it is within the current branch, and the
        branch is one of two or more alternatives (it either starts or ends
        with OP_ALT), we have reached the limit of THEN's action, so convert
        the return code to NOMATCH, which will cause normal backtracking to
        happen from now on. Otherwise, THEN is passed back to an outer
        alternative. This implements Perl's treatment of parenthesized groups,
        where a group not containing | does not affect the current alternative,
        that is, (X) is NOT the same as (X|(*F)). */

        if (rrc == MATCH_THEN)
          {
          next = ecode + GET(ecode,1);
          if (md->start_match_ptr < next &&
              (*ecode == OP_ALT || *next == OP_ALT))
            rrc = MATCH_NOMATCH;
          }

        /* Anything other than NOMATCH is passed back. */

        if (rrc != MATCH_NOMATCH) RRETURN(rrc);
        md->capture_last = save_capture_last;
        ecode += GET(ecode, 1);
        md->mark = save_mark;
        if (*ecode != OP_ALT) break;
        }

      DPRINTF(("bracket %d failed\n", number));
      md->offset_vector[offset] = save_offset1;
      md->offset_vector[offset+1] = save_offset2;
      md->offset_vector[md->offset_end - number] = save_offset3;

      /* At this point, rrc will be one of MATCH_ONCE or MATCH_NOMATCH. */

      RRETURN(rrc);
      }

    /* FALL THROUGH ... Insufficient room for saving captured contents. Treat
    as a non-capturing bracket. */

    /* VVVVVVVVVVVVVVVVVVVVVVVVV */
    /* VVVVVVVVVVVVVVVVVVVVVVVVV */

    DPRINTF(("insufficient capture room: treat as non-capturing\n"));

    /* VVVVVVVVVVVVVVVVVVVVVVVVV */
    /* VVVVVVVVVVVVVVVVVVVVVVVVV */

    /* Non-capturing or atomic group, except for possessive with unlimited
    repeat and ONCE group with no captures. Loop for all the alternatives.

    When we get to the final alternative within the brackets, we used to return
    the result of a recursive call to match() whatever happened so it was
    possible to reduce stack usage by turning this into a tail recursion,
    except in the case of a possibly empty group. However, now that there is
    the possiblity of (*THEN) occurring in the final alternative, this
    optimization is no longer always possible.

    We can optimize if we know there are no (*THEN)s in the pattern; at present
    this is the best that can be done.

    MATCH_ONCE is returned when the end of an atomic group is successfully
    reached, but subsequent matching fails. It passes back up the tree (causing
    captured values to be reset) until the original atomic group level is
    reached. This is tested by comparing md->once_target with the start of the
    group. At this point, the return is converted into MATCH_NOMATCH so that
    previous backup points can be taken. */

    case OP_ONCE:
    case OP_BRA:
    case OP_SBRA:
    DPRINTF(("start non-capturing bracket\n"));

    for (;;)
      {
      if (op >= OP_SBRA || op == OP_ONCE)
        md->match_function_type = MATCH_CBEGROUP;

      /* If this is not a possibly empty group, and there are no (*THEN)s in
      the pattern, and this is the final alternative, optimize as described
      above. */

      else if (!md->hasthen && ecode[GET(ecode, 1)] != OP_ALT)
        {
        ecode += PRIV(OP_lengths)[*ecode];
        goto TAIL_RECURSE;
        }

      /* In all other cases, we have to make another call to match(). */

      save_mark = md->mark;
      save_capture_last = md->capture_last;
      RMATCH(eptr, ecode + PRIV(OP_lengths)[*ecode], offset_top, md, eptrb,
        RM2);

      /* See comment in the code for capturing groups above about handling
      THEN. */

      if (rrc == MATCH_THEN)
        {
        next = ecode + GET(ecode,1);
        if (md->start_match_ptr < next &&
            (*ecode == OP_ALT || *next == OP_ALT))
          rrc = MATCH_NOMATCH;
        }

      if (rrc != MATCH_NOMATCH)
        {
        if (rrc == MATCH_ONCE)
          {
          const pcre_uchar *scode = ecode;
          if (*scode != OP_ONCE)           /* If not at start, find it */
            {
            while (*scode == OP_ALT) scode += GET(scode, 1);
            scode -= GET(scode, 1);
            }
          if (md->once_target == scode) rrc = MATCH_NOMATCH;
          }
        RRETURN(rrc);
        }
      ecode += GET(ecode, 1);
      md->mark = save_mark;
      if (*ecode != OP_ALT) break;
      md->capture_last = save_capture_last;
      }

    RRETURN(MATCH_NOMATCH);

    /* Handle possessive capturing brackets with an unlimited repeat. We come
    here from BRAZERO with allow_zero set TRUE. The offset_vector values are
    handled similarly to the normal case above. However, the matching is
    different. The end of these brackets will always be OP_KETRPOS, which
    returns MATCH_KETRPOS without going further in the pattern. By this means
    we can handle the group by iteration rather than recursion, thereby
    reducing the amount of stack needed. */

    case OP_CBRAPOS:
    case OP_SCBRAPOS:
    allow_zero = FALSE;

    POSSESSIVE_CAPTURE:
    number = GET2(ecode, 1+LINK_SIZE);
    offset = number << 1;

#ifdef PCRE_DEBUG
    printf("start possessive bracket %d\n", number);
    printf("subject=");
    pchars(eptr, 16, TRUE, md);
    printf("\n");
#endif

    if (offset >= md->offset_max) goto POSSESSIVE_NON_CAPTURE;

    matched_once = FALSE;
    code_offset = (int)(ecode - md->start_code);

    save_offset1 = md->offset_vector[offset];
    save_offset2 = md->offset_vector[offset+1];
    save_offset3 = md->offset_vector[md->offset_end - number];
    save_capture_last = md->capture_last;

    DPRINTF(("saving %d %d %d\n", save_offset1, save_offset2, save_offset3));

    /* Each time round the loop, save the current subject position for use
    when the group matches. For MATCH_MATCH, the group has matched, so we
    restart it with a new subject starting position, remembering that we had
    at least one match. For MATCH_NOMATCH, carry on with the alternatives, as
    usual. If we haven't matched any alternatives in any iteration, check to
    see if a previous iteration matched. If so, the group has matched;
    continue from afterwards. Otherwise it has failed; restore the previous
    capture values before returning NOMATCH. */

    for (;;)
      {
      md->offset_vector[md->offset_end - number] =
        (int)(eptr - md->start_subject);
      if (op >= OP_SBRA) md->match_function_type = MATCH_CBEGROUP;
      RMATCH(eptr, ecode + PRIV(OP_lengths)[*ecode], offset_top, md,
        eptrb, RM63);
      if (rrc == MATCH_KETRPOS)
        {
        offset_top = md->end_offset_top;
        ecode = md->start_code + code_offset;
        save_capture_last = md->capture_last;
        matched_once = TRUE;
        mstart = md->start_match_ptr;    /* In case \K changed it */
        if (eptr == md->end_match_ptr)   /* Matched an empty string */
          {
          do ecode += GET(ecode, 1); while (*ecode == OP_ALT);
          break;
          }
        eptr = md->end_match_ptr;
        continue;
        }

      /* See comment in the code for capturing groups above about handling
      THEN. */

      if (rrc == MATCH_THEN)
        {
        next = ecode + GET(ecode,1);
        if (md->start_match_ptr < next &&
            (*ecode == OP_ALT || *next == OP_ALT))
          rrc = MATCH_NOMATCH;
        }

      if (rrc != MATCH_NOMATCH) RRETURN(rrc);
      md->capture_last = save_capture_last;
      ecode += GET(ecode, 1);
      if (*ecode != OP_ALT) break;
      }

    if (!matched_once)
      {
      md->offset_vector[offset] = save_offset1;
      md->offset_vector[offset+1] = save_offset2;
      md->offset_vector[md->offset_end - number] = save_offset3;
      }

    if (allow_zero || matched_once)
      {
      ecode += 1 + LINK_SIZE;
      break;
      }

    RRETURN(MATCH_NOMATCH);

    /* Non-capturing possessive bracket with unlimited repeat. We come here
    from BRAZERO with allow_zero = TRUE. The code is similar to the above,
    without the capturing complication. It is written out separately for speed
    and cleanliness. */

    case OP_BRAPOS:
    case OP_SBRAPOS:
    allow_zero = FALSE;

    POSSESSIVE_NON_CAPTURE:
    matched_once = FALSE;
    code_offset = (int)(ecode - md->start_code);
    save_capture_last = md->capture_last;

    for (;;)
      {
      if (op >= OP_SBRA) md->match_function_type = MATCH_CBEGROUP;
      RMATCH(eptr, ecode + PRIV(OP_lengths)[*ecode], offset_top, md,
        eptrb, RM48);
      if (rrc == MATCH_KETRPOS)
        {
        offset_top = md->end_offset_top;
        ecode = md->start_code + code_offset;
        matched_once = TRUE;
        mstart = md->start_match_ptr;   /* In case \K reset it */
        if (eptr == md->end_match_ptr)  /* Matched an empty string */
          {
          do ecode += GET(ecode, 1); while (*ecode == OP_ALT);
          break;
          }
        eptr = md->end_match_ptr;
        continue;
        }

      /* See comment in the code for capturing groups above about handling
      THEN. */

      if (rrc == MATCH_THEN)
        {
        next = ecode + GET(ecode,1);
        if (md->start_match_ptr < next &&
            (*ecode == OP_ALT || *next == OP_ALT))
          rrc = MATCH_NOMATCH;
        }

      if (rrc != MATCH_NOMATCH) RRETURN(rrc);
      ecode += GET(ecode, 1);
      if (*ecode != OP_ALT) break;
      md->capture_last = save_capture_last;
      }

    if (matched_once || allow_zero)
      {
      ecode += 1 + LINK_SIZE;
      break;
      }
    RRETURN(MATCH_NOMATCH);

    /* Control never reaches here. */

    /* Conditional group: compilation checked that there are no more than two
    branches. If the condition is false, skipping the first branch takes us
    past the end of the item if there is only one branch, but that's exactly
    what we want. */

    case OP_COND:
    case OP_SCOND:

    /* The variable codelink will be added to ecode when the condition is
    false, to get to the second branch. Setting it to the offset to the ALT
    or KET, then incrementing ecode achieves this effect. We now have ecode
    pointing to the condition or callout. */

    codelink = GET(ecode, 1);   /* Offset to the second branch */
    ecode += 1 + LINK_SIZE;     /* From this opcode */

    /* Because of the way auto-callout works during compile, a callout item is
    inserted between OP_COND and an assertion condition. */

    if (*ecode == OP_CALLOUT)
      {
      if (PUBL(callout) != NULL)
        {
        PUBL(callout_block) cb;
        cb.version          = 2;   /* Version 1 of the callout block */
        cb.callout_number   = ecode[1];
        cb.offset_vector    = md->offset_vector;
#if defined COMPILE_PCRE8
        cb.subject          = (PCRE_SPTR)md->start_subject;
#elif defined COMPILE_PCRE16
        cb.subject          = (PCRE_SPTR16)md->start_subject;
#elif defined COMPILE_PCRE32
        cb.subject          = (PCRE_SPTR32)md->start_subject;
#endif
        cb.subject_length   = (int)(md->end_subject - md->start_subject);
        cb.start_match      = (int)(mstart - md->start_subject);
        cb.current_position = (int)(eptr - md->start_subject);
        cb.pattern_position = GET(ecode, 2);
        cb.next_item_length = GET(ecode, 2 + LINK_SIZE);
        cb.capture_top      = offset_top/2;
        cb.capture_last     = md->capture_last & CAPLMASK;
        /* Internal change requires this for API compatibility. */
        if (cb.capture_last == 0) cb.capture_last = -1;
        cb.callout_data     = md->callout_data;
        cb.mark             = md->nomatch_mark;
        if ((rrc = (*PUBL(callout))(&cb)) > 0) RRETURN(MATCH_NOMATCH);
        if (rrc < 0) RRETURN(rrc);
        }

      /* Advance ecode past the callout, so it now points to the condition. We
      must adjust codelink so that the value of ecode+codelink is unchanged. */

      ecode += PRIV(OP_lengths)[OP_CALLOUT];
      codelink -= PRIV(OP_lengths)[OP_CALLOUT];
      }

    /* Test the various possible conditions */

    condition = FALSE;
    switch(condcode = *ecode)
      {
      case OP_RREF:         /* Numbered group recursion test */
      if (md->recursive != NULL)     /* Not recursing => FALSE */
        {
        unsigned int recno = GET2(ecode, 1);   /* Recursion group number*/
        condition = (recno == RREF_ANY || recno == md->recursive->group_num);
        }
      break;

      case OP_DNRREF:       /* Duplicate named group recursion test */
      if (md->recursive != NULL)
        {
        int count = GET2(ecode, 1 + IMM2_SIZE);
        pcre_uchar *slot = md->name_table + GET2(ecode, 1) * md->name_entry_size;
        while (count-- > 0)
          {
          unsigned int recno = GET2(slot, 0);
          condition = recno == md->recursive->group_num;
          if (condition) break;
          slot += md->name_entry_size;
          }
        }
      break;

      case OP_CREF:         /* Numbered group used test */
      offset = GET2(ecode, 1) << 1;  /* Doubled ref number */
      condition = offset < offset_top && md->offset_vector[offset] >= 0;
      break;

      case OP_DNCREF:      /* Duplicate named group used test */
        {
        int count = GET2(ecode, 1 + IMM2_SIZE);
        pcre_uchar *slot = md->name_table + GET2(ecode, 1) * md->name_entry_size;
        while (count-- > 0)
          {
          offset = GET2(slot, 0) << 1;
          condition = offset < offset_top && md->offset_vector[offset] >= 0;
          if (condition) break;
          slot += md->name_entry_size;
          }
        }
      break;

      case OP_DEF:     /* DEFINE - always false */
      case OP_FAIL:    /* From optimized (?!) condition */
      break;

      /* The condition is an assertion. Call match() to evaluate it - setting
      md->match_function_type to MATCH_CONDASSERT causes it to stop at the end
      of an assertion. */

      default:
      md->match_function_type = MATCH_CONDASSERT;
      RMATCH(eptr, ecode, offset_top, md, NULL, RM3);
      if (rrc == MATCH_MATCH)
        {
        if (md->end_offset_top > offset_top)
          offset_top = md->end_offset_top;  /* Captures may have happened */
        condition = TRUE;

        /* Advance ecode past the assertion to the start of the first branch,
        but adjust it so that the general choosing code below works. If the
        assertion has a quantifier that allows zero repeats we must skip over
        the BRAZERO. This is a lunatic thing to do, but somebody did! */

        if (*ecode == OP_BRAZERO) ecode++;
        ecode += GET(ecode, 1);
        while (*ecode == OP_ALT) ecode += GET(ecode, 1);
        ecode += 1 + LINK_SIZE - PRIV(OP_lengths)[condcode];
        }

      /* PCRE doesn't allow the effect of (*THEN) to escape beyond an
      assertion; it is therefore treated as NOMATCH. Any other return is an
      error. */

      else if (rrc != MATCH_NOMATCH && rrc != MATCH_THEN)
        {
        RRETURN(rrc);         /* Need braces because of following else */
        }
      break;
      }

    /* Choose branch according to the condition */

    ecode += condition? PRIV(OP_lengths)[condcode] : codelink;

    /* We are now at the branch that is to be obeyed. As there is only one, we
    can use tail recursion to avoid using another stack frame, except when
    there is unlimited repeat of a possibly empty group. In the latter case, a
    recursive call to match() is always required, unless the second alternative
    doesn't exist, in which case we can just plough on. Note that, for
    compatibility with Perl, the | in a conditional group is NOT treated as
    creating two alternatives. If a THEN is encountered in the branch, it
    propagates out to the enclosing alternative (unless nested in a deeper set
    of alternatives, of course). */

    if (condition || ecode[-(1+LINK_SIZE)] == OP_ALT)
      {
      if (op != OP_SCOND)
        {
        goto TAIL_RECURSE;
        }

      md->match_function_type = MATCH_CBEGROUP;
      RMATCH(eptr, ecode, offset_top, md, eptrb, RM49);
      RRETURN(rrc);
      }

     /* Condition false & no alternative; continue after the group. */

    else
      {
      }
    break;


    /* Before OP_ACCEPT there may be any number of OP_CLOSE opcodes,
    to close any currently open capturing brackets. */

    case OP_CLOSE:
    number = GET2(ecode, 1);   /* Must be less than 65536 */
    offset = number << 1;

#ifdef PCRE_DEBUG
      printf("end bracket %d at *ACCEPT", number);
      printf("\n");
#endif

    md->capture_last = (md->capture_last & OVFLMASK) | number;
    if (offset >= md->offset_max) md->capture_last |= OVFLBIT; else
      {
      md->offset_vector[offset] =
        md->offset_vector[md->offset_end - number];
      md->offset_vector[offset+1] = (int)(eptr - md->start_subject);

      /* If this group is at or above the current highwater mark, ensure that
      any groups between the current high water mark and this group are marked
      unset and then update the high water mark. */

      if (offset >= offset_top)
        {
        register int *iptr = md->offset_vector + offset_top;
        register int *iend = md->offset_vector + offset;
        while (iptr < iend) *iptr++ = -1;
        offset_top = offset + 2;
        }
      }
    ecode += 1 + IMM2_SIZE;
    break;


    /* End of the pattern, either real or forced. */

    case OP_END:
    case OP_ACCEPT:
    case OP_ASSERT_ACCEPT:

    /* If we have matched an empty string, fail if not in an assertion and not
    in a recursion if either PCRE_NOTEMPTY is set, or if PCRE_NOTEMPTY_ATSTART
    is set and we have matched at the start of the subject. In both cases,
    backtracking will then try other alternatives, if any. */

    if (eptr == mstart && op != OP_ASSERT_ACCEPT &&
         md->recursive == NULL &&
         (md->notempty ||
           (md->notempty_atstart &&
             mstart == md->start_subject + md->start_offset)))
      RRETURN(MATCH_NOMATCH);

    /* Otherwise, we have a match. */

    md->end_match_ptr = eptr;           /* Record where we ended */
    md->end_offset_top = offset_top;    /* and how many extracts were taken */
    md->start_match_ptr = mstart;       /* and the start (\K can modify) */

    /* For some reason, the macros don't work properly if an expression is
    given as the argument to RRETURN when the heap is in use. */

    rrc = (op == OP_END)? MATCH_MATCH : MATCH_ACCEPT;
    RRETURN(rrc);

    /* Assertion brackets. Check the alternative branches in turn - the
    matching won't pass the KET for an assertion. If any one branch matches,
    the assertion is true. Lookbehind assertions have an OP_REVERSE item at the
    start of each branch to move the current point backwards, so the code at
    this level is identical to the lookahead case. When the assertion is part
    of a condition, we want to return immediately afterwards. The caller of
    this incarnation of the match() function will have set MATCH_CONDASSERT in
    md->match_function type, and one of these opcodes will be the first opcode
    that is processed. We use a local variable that is preserved over calls to
    match() to remember this case. */

    case OP_ASSERT:
    case OP_ASSERTBACK:
    save_mark = md->mark;
    if (md->match_function_type == MATCH_CONDASSERT)
      {
      condassert = TRUE;
      md->match_function_type = 0;
      }
    else condassert = FALSE;

    /* Loop for each branch */

    do
      {
      RMATCH(eptr, ecode + 1 + LINK_SIZE, offset_top, md, NULL, RM4);

      /* A match means that the assertion is true; break out of the loop
      that matches its alternatives. */

      if (rrc == MATCH_MATCH || rrc == MATCH_ACCEPT)
        {
        mstart = md->start_match_ptr;   /* In case \K reset it */
        break;
        }

      /* If not matched, restore the previous mark setting. */

      md->mark = save_mark;

      /* See comment in the code for capturing groups above about handling
      THEN. */

      if (rrc == MATCH_THEN)
        {
        next = ecode + GET(ecode,1);
        if (md->start_match_ptr < next &&
            (*ecode == OP_ALT || *next == OP_ALT))
          rrc = MATCH_NOMATCH;
        }

      /* Anything other than NOMATCH causes the entire assertion to fail,
      passing back the return code. This includes COMMIT, SKIP, PRUNE and an
      uncaptured THEN, which means they take their normal effect. This
      consistent approach does not always have exactly the same effect as in
      Perl. */

      if (rrc != MATCH_NOMATCH) RRETURN(rrc);
      ecode += GET(ecode, 1);
      }
    while (*ecode == OP_ALT);   /* Continue for next alternative */

    /* If we have tried all the alternative branches, the assertion has
    failed. If not, we broke out after a match. */

    if (*ecode == OP_KET) RRETURN(MATCH_NOMATCH);

    /* If checking an assertion for a condition, return MATCH_MATCH. */

    if (condassert) RRETURN(MATCH_MATCH);

    /* Continue from after a successful assertion, updating the offsets high
    water mark, since extracts may have been taken during the assertion. */

    do ecode += GET(ecode,1); while (*ecode == OP_ALT);
    ecode += 1 + LINK_SIZE;
    offset_top = md->end_offset_top;
    continue;

    /* Negative assertion: all branches must fail to match for the assertion to
    succeed. */

    case OP_ASSERT_NOT:
    case OP_ASSERTBACK_NOT:
    save_mark = md->mark;
    if (md->match_function_type == MATCH_CONDASSERT)
      {
      condassert = TRUE;
      md->match_function_type = 0;
      }
    else condassert = FALSE;

    /* Loop for each alternative branch. */

    do
      {
      RMATCH(eptr, ecode + 1 + LINK_SIZE, offset_top, md, NULL, RM5);
      md->mark = save_mark;   /* Always restore the mark setting */

      switch(rrc)
        {
        case MATCH_MATCH:            /* A successful match means */
        case MATCH_ACCEPT:           /* the assertion has failed. */
        RRETURN(MATCH_NOMATCH);

        case MATCH_NOMATCH:          /* Carry on with next branch */
        break;

        /* See comment in the code for capturing groups above about handling
        THEN. */

        case MATCH_THEN:
        next = ecode + GET(ecode,1);
        if (md->start_match_ptr < next &&
            (*ecode == OP_ALT || *next == OP_ALT))
          {
          rrc = MATCH_NOMATCH;
          break;
          }
        /* Otherwise fall through. */

        /* COMMIT, SKIP, PRUNE, and an uncaptured THEN cause the whole
        assertion to fail to match, without considering any more alternatives.
        Failing to match means the assertion is true. This is a consistent
        approach, but does not always have the same effect as in Perl. */

        case MATCH_COMMIT:
        case MATCH_SKIP:
        case MATCH_SKIP_ARG:
        case MATCH_PRUNE:
        do ecode += GET(ecode,1); while (*ecode == OP_ALT);
        goto NEG_ASSERT_TRUE;   /* Break out of alternation loop */

        /* Anything else is an error */

        default:
        RRETURN(rrc);
        }

      /* Continue with next branch */

      ecode += GET(ecode,1);
      }
    while (*ecode == OP_ALT);

    /* All branches in the assertion failed to match. */

    NEG_ASSERT_TRUE:
    if (condassert) RRETURN(MATCH_MATCH);  /* Condition assertion */
    ecode += 1 + LINK_SIZE;                /* Continue with current branch */
    continue;

    /* Move the subject pointer back. This occurs only at the start of
    each branch of a lookbehind assertion. If we are too close to the start to
    move back, this match function fails. When working with UTF-8 we move
    back a number of characters, not bytes. */

    case OP_REVERSE:
#ifdef SUPPORT_UTF
    if (utf)
      {
      i = GET(ecode, 1);
      while (i-- > 0)
        {
        eptr--;
        if (eptr < md->start_subject) RRETURN(MATCH_NOMATCH);
        BACKCHAR(eptr);
        }
      }
    else
#endif

    /* No UTF-8 support, or not in UTF-8 mode: count is byte count */

      {
      eptr -= GET(ecode, 1);
      if (eptr < md->start_subject) RRETURN(MATCH_NOMATCH);
      }

    /* Save the earliest consulted character, then skip to next op code */

    if (eptr < md->start_used_ptr) md->start_used_ptr = eptr;
    ecode += 1 + LINK_SIZE;
    break;

    /* The callout item calls an external function, if one is provided, passing
    details of the match so far. This is mainly for debugging, though the
    function is able to force a failure. */

    case OP_CALLOUT:
    if (PUBL(callout) != NULL)
      {
      PUBL(callout_block) cb;
      cb.version          = 2;   /* Version 1 of the callout block */
      cb.callout_number   = ecode[1];
      cb.offset_vector    = md->offset_vector;
#if defined COMPILE_PCRE8
      cb.subject          = (PCRE_SPTR)md->start_subject;
#elif defined COMPILE_PCRE16
      cb.subject          = (PCRE_SPTR16)md->start_subject;
#elif defined COMPILE_PCRE32
      cb.subject          = (PCRE_SPTR32)md->start_subject;
#endif
      cb.subject_length   = (int)(md->end_subject - md->start_subject);
      cb.start_match      = (int)(mstart - md->start_subject);
      cb.current_position = (int)(eptr - md->start_subject);
      cb.pattern_position = GET(ecode, 2);
      cb.next_item_length = GET(ecode, 2 + LINK_SIZE);
      cb.capture_top      = offset_top/2;
      cb.capture_last     = md->capture_last & CAPLMASK;
      /* Internal change requires this for API compatibility. */
      if (cb.capture_last == 0) cb.capture_last = -1;
      cb.callout_data     = md->callout_data;
      cb.mark             = md->nomatch_mark;
      if ((rrc = (*PUBL(callout))(&cb)) > 0) RRETURN(MATCH_NOMATCH);
      if (rrc < 0) RRETURN(rrc);
      }
    ecode += 2 + 2*LINK_SIZE;
    break;

    /* Recursion either matches the current regex, or some subexpression. The
    offset data is the offset to the starting bracket from the start of the
    whole pattern. (This is so that it works from duplicated subpatterns.)

    The state of the capturing groups is preserved over recursion, and
    re-instated afterwards. We don't know how many are started and not yet
    finished (offset_top records the completed total) so we just have to save
    all the potential data. There may be up to 65535 such values, which is too
    large to put on the stack, but using malloc for small numbers seems
    expensive. As a compromise, the stack is used when there are no more than
    REC_STACK_SAVE_MAX values to store; otherwise malloc is used.

    There are also other values that have to be saved. We use a chained
    sequence of blocks that actually live on the stack. Thanks to Robin Houston
    for the original version of this logic. It has, however, been hacked around
    a lot, so he is not to blame for the current way it works. */

    case OP_RECURSE:
      {
      recursion_info *ri;
      unsigned int recno;

      callpat = md->start_code + GET(ecode, 1);
      recno = (callpat == md->start_code)? 0 :
        GET2(callpat, 1 + LINK_SIZE);

      /* Check for repeating a recursion without advancing the subject pointer.
      This should catch convoluted mutual recursions. (Some simple cases are
      caught at compile time.) */

      for (ri = md->recursive; ri != NULL; ri = ri->prevrec)
        if (recno == ri->group_num && eptr == ri->subject_position)
          RRETURN(PCRE_ERROR_RECURSELOOP);

      /* Add to "recursing stack" */

      new_recursive.group_num = recno;
      new_recursive.saved_capture_last = md->capture_last;
      new_recursive.subject_position = eptr;
      new_recursive.prevrec = md->recursive;
      md->recursive = &new_recursive;

      /* Where to continue from afterwards */

      ecode += 1 + LINK_SIZE;

      /* Now save the offset data */

      new_recursive.saved_max = md->offset_end;
      if (new_recursive.saved_max <= REC_STACK_SAVE_MAX)
        new_recursive.offset_save = stacksave;
      else
        {
        new_recursive.offset_save =
          (int *)(PUBL(malloc))(new_recursive.saved_max * sizeof(int));
        if (new_recursive.offset_save == NULL) RRETURN(PCRE_ERROR_NOMEMORY);
        }
      memcpy(new_recursive.offset_save, md->offset_vector,
            new_recursive.saved_max * sizeof(int));

      /* OK, now we can do the recursion. After processing each alternative,
      restore the offset data and the last captured value. If there were nested
      recursions, md->recursive might be changed, so reset it before looping.
      */

      DPRINTF(("Recursing into group %d\n", new_recursive.group_num));
      cbegroup = (*callpat >= OP_SBRA);
      do
        {
        if (cbegroup) md->match_function_type = MATCH_CBEGROUP;
        RMATCH(eptr, callpat + PRIV(OP_lengths)[*callpat], offset_top,
          md, eptrb, RM6);
        memcpy(md->offset_vector, new_recursive.offset_save,
            new_recursive.saved_max * sizeof(int));
        md->capture_last = new_recursive.saved_capture_last;
        md->recursive = new_recursive.prevrec;
        if (rrc == MATCH_MATCH || rrc == MATCH_ACCEPT)
          {
          DPRINTF(("Recursion matched\n"));
          if (new_recursive.offset_save != stacksave)
            (PUBL(free))(new_recursive.offset_save);

          /* Set where we got to in the subject, and reset the start in case
          it was changed by \K. This *is* propagated back out of a recursion,
          for Perl compatibility. */

          eptr = md->end_match_ptr;
          mstart = md->start_match_ptr;
          goto RECURSION_MATCHED;        /* Exit loop; end processing */
          }

        /* PCRE does not allow THEN, SKIP, PRUNE or COMMIT to escape beyond a
        recursion; they cause a NOMATCH for the entire recursion. These codes
        are defined in a range that can be tested for. */

        if (rrc >= MATCH_BACKTRACK_MIN && rrc <= MATCH_BACKTRACK_MAX)
          {
          if (new_recursive.offset_save != stacksave)
            (PUBL(free))(new_recursive.offset_save);
          RRETURN(MATCH_NOMATCH);
          }

        /* Any return code other than NOMATCH is an error. */

        if (rrc != MATCH_NOMATCH)
          {
          DPRINTF(("Recursion gave error %d\n", rrc));
          if (new_recursive.offset_save != stacksave)
            (PUBL(free))(new_recursive.offset_save);
          RRETURN(rrc);
          }

        md->recursive = &new_recursive;
        callpat += GET(callpat, 1);
        }
      while (*callpat == OP_ALT);

      DPRINTF(("Recursion didn't match\n"));
      md->recursive = new_recursive.prevrec;
      if (new_recursive.offset_save != stacksave)
        (PUBL(free))(new_recursive.offset_save);
      RRETURN(MATCH_NOMATCH);
      }

    RECURSION_MATCHED:
    break;

    /* An alternation is the end of a branch; scan along to find the end of the
    bracketed group and go to there. */

    case OP_ALT:
    do ecode += GET(ecode,1); while (*ecode == OP_ALT);
    break;

    /* BRAZERO, BRAMINZERO and SKIPZERO occur just before a bracket group,
    indicating that it may occur zero times. It may repeat infinitely, or not
    at all - i.e. it could be ()* or ()? or even (){0} in the pattern. Brackets
    with fixed upper repeat limits are compiled as a number of copies, with the
    optional ones preceded by BRAZERO or BRAMINZERO. */

    case OP_BRAZERO:
    next = ecode + 1;
    RMATCH(eptr, next, offset_top, md, eptrb, RM10);
    if (rrc != MATCH_NOMATCH) RRETURN(rrc);
    do next += GET(next, 1); while (*next == OP_ALT);
    ecode = next + 1 + LINK_SIZE;
    break;

    case OP_BRAMINZERO:
    next = ecode + 1;
    do next += GET(next, 1); while (*next == OP_ALT);
    RMATCH(eptr, next + 1+LINK_SIZE, offset_top, md, eptrb, RM11);
    if (rrc != MATCH_NOMATCH) RRETURN(rrc);
    ecode++;
    break;

    case OP_SKIPZERO:
    next = ecode+1;
    do next += GET(next,1); while (*next == OP_ALT);
    ecode = next + 1 + LINK_SIZE;
    break;

    /* BRAPOSZERO occurs before a possessive bracket group. Don't do anything
    here; just jump to the group, with allow_zero set TRUE. */

    case OP_BRAPOSZERO:
    op = *(++ecode);
    allow_zero = TRUE;
    if (op == OP_CBRAPOS || op == OP_SCBRAPOS) goto POSSESSIVE_CAPTURE;
      goto POSSESSIVE_NON_CAPTURE;

    /* End of a group, repeated or non-repeating. */

    case OP_KET:
    case OP_KETRMIN:
    case OP_KETRMAX:
    case OP_KETRPOS:
    prev = ecode - GET(ecode, 1);

    /* If this was a group that remembered the subject start, in order to break
    infinite repeats of empty string matches, retrieve the subject start from
    the chain. Otherwise, set it NULL. */

    if (*prev >= OP_SBRA || *prev == OP_ONCE)
      {
      saved_eptr = eptrb->epb_saved_eptr;   /* Value at start of group */
      eptrb = eptrb->epb_prev;              /* Backup to previous group */
      }
    else saved_eptr = NULL;

    /* If we are at the end of an assertion group or a non-capturing atomic
    group, stop matching and return MATCH_MATCH, but record the current high
    water mark for use by positive assertions. We also need to record the match
    start in case it was changed by \K. */

    if ((*prev >= OP_ASSERT && *prev <= OP_ASSERTBACK_NOT) ||
         *prev == OP_ONCE_NC)
      {
      md->end_match_ptr = eptr;      /* For ONCE_NC */
      md->end_offset_top = offset_top;
      md->start_match_ptr = mstart;
      RRETURN(MATCH_MATCH);         /* Sets md->mark */
      }

    /* For capturing groups we have to check the group number back at the start
    and if necessary complete handling an extraction by setting the offsets and
    bumping the high water mark. Whole-pattern recursion is coded as a recurse
    into group 0, so it won't be picked up here. Instead, we catch it when the
    OP_END is reached. Other recursion is handled here. We just have to record
    the current subject position and start match pointer and give a MATCH
    return. */

    if (*prev == OP_CBRA || *prev == OP_SCBRA ||
        *prev == OP_CBRAPOS || *prev == OP_SCBRAPOS)
      {
      number = GET2(prev, 1+LINK_SIZE);
      offset = number << 1;

#ifdef PCRE_DEBUG
      printf("end bracket %d", number);
      printf("\n");
#endif

      /* Handle a recursively called group. */

      if (md->recursive != NULL && md->recursive->group_num == number)
        {
        md->end_match_ptr = eptr;
        md->start_match_ptr = mstart;
        RRETURN(MATCH_MATCH);
        }

      /* Deal with capturing */

      md->capture_last = (md->capture_last & OVFLMASK) | number;
      if (offset >= md->offset_max) md->capture_last |= OVFLBIT; else
        {
        /* If offset is greater than offset_top, it means that we are
        "skipping" a capturing group, and that group's offsets must be marked
        unset. In earlier versions of PCRE, all the offsets were unset at the
        start of matching, but this doesn't work because atomic groups and
        assertions can cause a value to be set that should later be unset.
        Example: matching /(?>(a))b|(a)c/ against "ac". This sets group 1 as
        part of the atomic group, but this is not on the final matching path,
        so must be unset when 2 is set. (If there is no group 2, there is no
        problem, because offset_top will then be 2, indicating no capture.) */

        if (offset > offset_top)
          {
          register int *iptr = md->offset_vector + offset_top;
          register int *iend = md->offset_vector + offset;
          while (iptr < iend) *iptr++ = -1;
          }

        /* Now make the extraction */

        md->offset_vector[offset] =
          md->offset_vector[md->offset_end - number];
        md->offset_vector[offset+1] = (int)(eptr - md->start_subject);
        if (offset_top <= offset) offset_top = offset + 2;
        }
      }

    /* OP_KETRPOS is a possessive repeating ket. Remember the current position,
    and return the MATCH_KETRPOS. This makes it possible to do the repeats one
    at a time from the outer level, thus saving stack. This must precede the
    empty string test - in this case that test is done at the outer level. */

    if (*ecode == OP_KETRPOS)
      {
      md->start_match_ptr = mstart;    /* In case \K reset it */
      md->end_match_ptr = eptr;
      md->end_offset_top = offset_top;
      RRETURN(MATCH_KETRPOS);
      }

    /* For an ordinary non-repeating ket, just continue at this level. This
    also happens for a repeating ket if no characters were matched in the
    group. This is the forcible breaking of infinite loops as implemented in
    Perl 5.005. For a non-repeating atomic group that includes captures,
    establish a backup point by processing the rest of the pattern at a lower
    level. If this results in a NOMATCH return, pass MATCH_ONCE back to the
    original OP_ONCE level, thereby bypassing intermediate backup points, but
    resetting any captures that happened along the way. */

    if (*ecode == OP_KET || eptr == saved_eptr)
      {
      if (*prev == OP_ONCE)
        {
        RMATCH(eptr, ecode + 1 + LINK_SIZE, offset_top, md, eptrb, RM12);
        if (rrc != MATCH_NOMATCH) RRETURN(rrc);
        md->once_target = prev;  /* Level at which to change to MATCH_NOMATCH */
        RRETURN(MATCH_ONCE);
        }
      ecode += 1 + LINK_SIZE;    /* Carry on at this level */
      break;
      }

    /* The normal repeating kets try the rest of the pattern or restart from
    the preceding bracket, in the appropriate order. In the second case, we can
    use tail recursion to avoid using another stack frame, unless we have an
    an atomic group or an unlimited repeat of a group that can match an empty
    string. */

    if (*ecode == OP_KETRMIN)
      {
      RMATCH(eptr, ecode + 1 + LINK_SIZE, offset_top, md, eptrb, RM7);
      if (rrc != MATCH_NOMATCH) RRETURN(rrc);
      if (*prev == OP_ONCE)
        {
        RMATCH(eptr, prev, offset_top, md, eptrb, RM8);
        if (rrc != MATCH_NOMATCH) RRETURN(rrc);
        md->once_target = prev;  /* Level at which to change to MATCH_NOMATCH */
        RRETURN(MATCH_ONCE);
        }
      if (*prev >= OP_SBRA)    /* Could match an empty string */
        {
        RMATCH(eptr, prev, offset_top, md, eptrb, RM50);
        RRETURN(rrc);
        }
      ecode = prev;
      goto TAIL_RECURSE;
      }
    else  /* OP_KETRMAX */
      {
      RMATCH(eptr, prev, offset_top, md, eptrb, RM13);
      if (rrc == MATCH_ONCE && md->once_target == prev) rrc = MATCH_NOMATCH;
      if (rrc != MATCH_NOMATCH) RRETURN(rrc);
      if (*prev == OP_ONCE)
        {
        RMATCH(eptr, ecode + 1 + LINK_SIZE, offset_top, md, eptrb, RM9);
        if (rrc != MATCH_NOMATCH) RRETURN(rrc);
        md->once_target = prev;
        RRETURN(MATCH_ONCE);
        }
      ecode += 1 + LINK_SIZE;
      goto TAIL_RECURSE;
      }
    /* Control never gets here */

    /* Not multiline mode: start of subject assertion, unless notbol. */

    case OP_CIRC:
    if (md->notbol && eptr == md->start_subject) RRETURN(MATCH_NOMATCH);

    /* Start of subject assertion */

    case OP_SOD:
    if (eptr != md->start_subject) RRETURN(MATCH_NOMATCH);
    ecode++;
    break;

    /* Multiline mode: start of subject unless notbol, or after any newline. */

    case OP_CIRCM:
    if (md->notbol && eptr == md->start_subject) RRETURN(MATCH_NOMATCH);
    if (eptr != md->start_subject &&
        (eptr == md->end_subject || !WAS_NEWLINE(eptr)))
      RRETURN(MATCH_NOMATCH);
    ecode++;
    break;

    /* Start of match assertion */

    case OP_SOM:
    if (eptr != md->start_subject + md->start_offset) RRETURN(MATCH_NOMATCH);
    ecode++;
    break;

    /* Reset the start of match point */

    case OP_SET_SOM:
    mstart = eptr;
    ecode++;
    break;

    /* Multiline mode: assert before any newline, or before end of subject
    unless noteol is set. */

    case OP_DOLLM:
    if (eptr < md->end_subject)
      {
      if (!IS_NEWLINE(eptr))
        {
        if (md->partial != 0 &&
            eptr + 1 >= md->end_subject &&
            NLBLOCK->nltype == NLTYPE_FIXED &&
            NLBLOCK->nllen == 2 &&
            UCHAR21TEST(eptr) == NLBLOCK->nl[0])
          {
          md->hitend = TRUE;
          if (md->partial > 1) RRETURN(PCRE_ERROR_PARTIAL);
          }
        RRETURN(MATCH_NOMATCH);
        }
      }
    else
      {
      if (md->noteol) RRETURN(MATCH_NOMATCH);
      SCHECK_PARTIAL();
      }
    ecode++;
    break;

    /* Not multiline mode: assert before a terminating newline or before end of
    subject unless noteol is set. */

    case OP_DOLL:
    if (md->noteol) RRETURN(MATCH_NOMATCH);
    if (!md->endonly) goto ASSERT_NL_OR_EOS;

    /* ... else fall through for endonly */

    /* End of subject assertion (\z) */

    case OP_EOD:
    if (eptr < md->end_subject) RRETURN(MATCH_NOMATCH);
    SCHECK_PARTIAL();
    ecode++;
    break;

    /* End of subject or ending \n assertion (\Z) */

    case OP_EODN:
    ASSERT_NL_OR_EOS:
    if (eptr < md->end_subject &&
        (!IS_NEWLINE(eptr) || eptr != md->end_subject - md->nllen))
      {
      if (md->partial != 0 &&
          eptr + 1 >= md->end_subject &&
          NLBLOCK->nltype == NLTYPE_FIXED &&
          NLBLOCK->nllen == 2 &&
          UCHAR21TEST(eptr) == NLBLOCK->nl[0])
        {
        md->hitend = TRUE;
        if (md->partial > 1) RRETURN(PCRE_ERROR_PARTIAL);
        }
      RRETURN(MATCH_NOMATCH);
      }

    /* Either at end of string or \n before end. */

    SCHECK_PARTIAL();
    ecode++;
    break;

    /* Word boundary assertions */

    case OP_NOT_WORD_BOUNDARY:
    case OP_WORD_BOUNDARY:
      {

      /* Find out if the previous and current characters are "word" characters.
      It takes a bit more work in UTF-8 mode. Characters > 255 are assumed to
      be "non-word" characters. Remember the earliest consulted character for
      partial matching. */

#ifdef SUPPORT_UTF
      if (utf)
        {
        /* Get status of previous character */

        if (eptr == md->start_subject) prev_is_word = FALSE; else
          {
          PCRE_PUCHAR lastptr = eptr - 1;
          BACKCHAR(lastptr);
          if (lastptr < md->start_used_ptr) md->start_used_ptr = lastptr;
          GETCHAR(c, lastptr);
#ifdef SUPPORT_UCP
          if (md->use_ucp)
            {
            if (c == '_') prev_is_word = TRUE; else
              {
              int cat = UCD_CATEGORY(c);
              prev_is_word = (cat == ucp_L || cat == ucp_N);
              }
            }
          else
#endif
          prev_is_word = c < 256 && (md->ctypes[c] & ctype_word) != 0;
          }

        /* Get status of next character */

        if (eptr >= md->end_subject)
          {
          SCHECK_PARTIAL();
          cur_is_word = FALSE;
          }
        else
          {
          GETCHAR(c, eptr);
#ifdef SUPPORT_UCP
          if (md->use_ucp)
            {
            if (c == '_') cur_is_word = TRUE; else
              {
              int cat = UCD_CATEGORY(c);
              cur_is_word = (cat == ucp_L || cat == ucp_N);
              }
            }
          else
#endif
          cur_is_word = c < 256 && (md->ctypes[c] & ctype_word) != 0;
          }
        }
      else
#endif

      /* Not in UTF-8 mode, but we may still have PCRE_UCP set, and for
      consistency with the behaviour of \w we do use it in this case. */

        {
        /* Get status of previous character */

        if (eptr == md->start_subject) prev_is_word = FALSE; else
          {
          if (eptr <= md->start_used_ptr) md->start_used_ptr = eptr - 1;
#ifdef SUPPORT_UCP
          if (md->use_ucp)
            {
            c = eptr[-1];
            if (c == '_') prev_is_word = TRUE; else
              {
              int cat = UCD_CATEGORY(c);
              prev_is_word = (cat == ucp_L || cat == ucp_N);
              }
            }
          else
#endif
          prev_is_word = MAX_255(eptr[-1])
            && ((md->ctypes[eptr[-1]] & ctype_word) != 0);
          }

        /* Get status of next character */

        if (eptr >= md->end_subject)
          {
          SCHECK_PARTIAL();
          cur_is_word = FALSE;
          }
        else
#ifdef SUPPORT_UCP
        if (md->use_ucp)
          {
          c = *eptr;
          if (c == '_') cur_is_word = TRUE; else
            {
            int cat = UCD_CATEGORY(c);
            cur_is_word = (cat == ucp_L || cat == ucp_N);
            }
          }
        else
#endif
        cur_is_word = MAX_255(*eptr)
          && ((md->ctypes[*eptr] & ctype_word) != 0);
        }

      /* Now see if the situation is what we want */

      if ((*ecode++ == OP_WORD_BOUNDARY)?
           cur_is_word == prev_is_word : cur_is_word != prev_is_word)
        RRETURN(MATCH_NOMATCH);
      }
    break;

    /* Match any single character type except newline; have to take care with
    CRLF newlines and partial matching. */

    case OP_ANY:
    if (IS_NEWLINE(eptr)) RRETURN(MATCH_NOMATCH);
    if (md->partial != 0 &&
        eptr + 1 >= md->end_subject &&
        NLBLOCK->nltype == NLTYPE_FIXED &&
        NLBLOCK->nllen == 2 &&
        UCHAR21TEST(eptr) == NLBLOCK->nl[0])
      {
      md->hitend = TRUE;
      if (md->partial > 1) RRETURN(PCRE_ERROR_PARTIAL);
      }

    /* Fall through */

    /* Match any single character whatsoever. */

    case OP_ALLANY:
    if (eptr >= md->end_subject)   /* DO NOT merge the eptr++ here; it must */
      {                            /* not be updated before SCHECK_PARTIAL. */
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    eptr++;
#ifdef SUPPORT_UTF
    if (utf) ACROSSCHAR(eptr < md->end_subject, *eptr, eptr++);
#endif
    ecode++;
    break;

    /* Match a single byte, even in UTF-8 mode. This opcode really does match
    any byte, even newline, independent of the setting of PCRE_DOTALL. */

    case OP_ANYBYTE:
    if (eptr >= md->end_subject)   /* DO NOT merge the eptr++ here; it must */
      {                            /* not be updated before SCHECK_PARTIAL. */
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    eptr++;
    ecode++;
    break;

    case OP_NOT_DIGIT:
    if (eptr >= md->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(c, eptr);
    if (
#if defined SUPPORT_UTF || !(defined COMPILE_PCRE8)
       c < 256 &&
#endif
       (md->ctypes[c] & ctype_digit) != 0
       )
      RRETURN(MATCH_NOMATCH);
    ecode++;
    break;

    case OP_DIGIT:
    if (eptr >= md->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(c, eptr);
    if (
#if defined SUPPORT_UTF || !(defined COMPILE_PCRE8)
       c > 255 ||
#endif
       (md->ctypes[c] & ctype_digit) == 0
       )
      RRETURN(MATCH_NOMATCH);
    ecode++;
    break;

    case OP_NOT_WHITESPACE:
    if (eptr >= md->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(c, eptr);
    if (
#if defined SUPPORT_UTF || !(defined COMPILE_PCRE8)
       c < 256 &&
#endif
       (md->ctypes[c] & ctype_space) != 0
       )
      RRETURN(MATCH_NOMATCH);
    ecode++;
    break;

    case OP_WHITESPACE:
    if (eptr >= md->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(c, eptr);
    if (
#if defined SUPPORT_UTF || !(defined COMPILE_PCRE8)
       c > 255 ||
#endif
       (md->ctypes[c] & ctype_space) == 0
       )
      RRETURN(MATCH_NOMATCH);
    ecode++;
    break;

    case OP_NOT_WORDCHAR:
    if (eptr >= md->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(c, eptr);
    if (
#if defined SUPPORT_UTF || !(defined COMPILE_PCRE8)
       c < 256 &&
#endif
       (md->ctypes[c] & ctype_word) != 0
       )
      RRETURN(MATCH_NOMATCH);
    ecode++;
    break;

    case OP_WORDCHAR:
    if (eptr >= md->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(c, eptr);
    if (
#if defined SUPPORT_UTF || !(defined COMPILE_PCRE8)
       c > 255 ||
#endif
       (md->ctypes[c] & ctype_word) == 0
       )
      RRETURN(MATCH_NOMATCH);
    ecode++;
    break;

    case OP_ANYNL:
    if (eptr >= md->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(c, eptr);
    switch(c)
      {
      default: RRETURN(MATCH_NOMATCH);

      case CHAR_CR:
      if (eptr >= md->end_subject)
        {
        SCHECK_PARTIAL();
        }
      else if (UCHAR21TEST(eptr) == CHAR_LF) eptr++;
      break;

      case CHAR_LF:
      break;

      case CHAR_VT:
      case CHAR_FF:
      case CHAR_NEL:
#ifndef EBCDIC
      case 0x2028:
      case 0x2029:
#endif  /* Not EBCDIC */
      if (md->bsr_anycrlf) RRETURN(MATCH_NOMATCH);
      break;
      }
    ecode++;
    break;

    case OP_NOT_HSPACE:
    if (eptr >= md->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(c, eptr);
    switch(c)
      {
      HSPACE_CASES: RRETURN(MATCH_NOMATCH);  /* Byte and multibyte cases */
      default: break;
      }
    ecode++;
    break;

    case OP_HSPACE:
    if (eptr >= md->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(c, eptr);
    switch(c)
      {
      HSPACE_CASES: break;  /* Byte and multibyte cases */
      default: RRETURN(MATCH_NOMATCH);
      }
    ecode++;
    break;

    case OP_NOT_VSPACE:
    if (eptr >= md->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(c, eptr);
    switch(c)
      {
      VSPACE_CASES: RRETURN(MATCH_NOMATCH);
      default: break;
      }
    ecode++;
    break;

    case OP_VSPACE:
    if (eptr >= md->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(c, eptr);
    switch(c)
      {
      VSPACE_CASES: break;
      default: RRETURN(MATCH_NOMATCH);
      }
    ecode++;
    break;

#ifdef SUPPORT_UCP
    /* Check the next character by Unicode property. We will get here only
    if the support is in the binary; otherwise a compile-time error occurs. */

    case OP_PROP:
    case OP_NOTPROP:
    if (eptr >= md->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    GETCHARINCTEST(c, eptr);
      {
      const pcre_uint32 *cp;
      const ucd_record *prop = GET_UCD(c);

      switch(ecode[1])
        {
        case PT_ANY:
        if (op == OP_NOTPROP) RRETURN(MATCH_NOMATCH);
        break;

        case PT_LAMP:
        if ((prop->chartype == ucp_Lu ||
             prop->chartype == ucp_Ll ||
             prop->chartype == ucp_Lt) == (op == OP_NOTPROP))
          RRETURN(MATCH_NOMATCH);
        break;

        case PT_GC:
        if ((ecode[2] != PRIV(ucp_gentype)[prop->chartype]) == (op == OP_PROP))
          RRETURN(MATCH_NOMATCH);
        break;

        case PT_PC:
        if ((ecode[2] != prop->chartype) == (op == OP_PROP))
          RRETURN(MATCH_NOMATCH);
        break;

        case PT_SC:
        if ((ecode[2] != prop->script) == (op == OP_PROP))
          RRETURN(MATCH_NOMATCH);
        break;

        /* These are specials */

        case PT_ALNUM:
        if ((PRIV(ucp_gentype)[prop->chartype] == ucp_L ||
             PRIV(ucp_gentype)[prop->chartype] == ucp_N) == (op == OP_NOTPROP))
          RRETURN(MATCH_NOMATCH);
        break;

        /* Perl space used to exclude VT, but from Perl 5.18 it is included,
        which means that Perl space and POSIX space are now identical. PCRE
        was changed at release 8.34. */

        case PT_SPACE:    /* Perl space */
        case PT_PXSPACE:  /* POSIX space */
        switch(c)
          {
          HSPACE_CASES:
          VSPACE_CASES:
          if (op == OP_NOTPROP) RRETURN(MATCH_NOMATCH);
          break;

          default:
          if ((PRIV(ucp_gentype)[prop->chartype] == ucp_Z) ==
            (op == OP_NOTPROP)) RRETURN(MATCH_NOMATCH);
          break;
          }
        break;

        case PT_WORD:
        if ((PRIV(ucp_gentype)[prop->chartype] == ucp_L ||
             PRIV(ucp_gentype)[prop->chartype] == ucp_N ||
             c == CHAR_UNDERSCORE) == (op == OP_NOTPROP))
          RRETURN(MATCH_NOMATCH);
        break;

        case PT_CLIST:
        cp = PRIV(ucd_caseless_sets) + ecode[2];
        for (;;)
          {
          if (c < *cp)
            { if (op == OP_PROP) { RRETURN(MATCH_NOMATCH); } else break; }
          if (c == *cp++)
            { if (op == OP_PROP) break; else { RRETURN(MATCH_NOMATCH); } }
          }
        break;

        case PT_UCNC:
        if ((c == CHAR_DOLLAR_SIGN || c == CHAR_COMMERCIAL_AT ||
             c == CHAR_GRAVE_ACCENT || (c >= 0xa0 && c <= 0xd7ff) ||
             c >= 0xe000) == (op == OP_NOTPROP))
          RRETURN(MATCH_NOMATCH);
        break;

        /* This should never occur */

        default:
        RRETURN(PCRE_ERROR_INTERNAL);
        }

      ecode += 3;
      }
    break;

    /* Match an extended Unicode sequence. We will get here only if the support
    is in the binary; otherwise a compile-time error occurs. */

    case OP_EXTUNI:
    if (eptr >= md->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
    else
      {
      int lgb, rgb;
      GETCHARINCTEST(c, eptr);
      lgb = UCD_GRAPHBREAK(c);
      while (eptr < md->end_subject)
        {
        int len = 1;
        if (!utf) c = *eptr; else { GETCHARLEN(c, eptr, len); }
        rgb = UCD_GRAPHBREAK(c);
        if ((PRIV(ucp_gbtable)[lgb] & (1 << rgb)) == 0) break;
        lgb = rgb;
        eptr += len;
        }
      }
    CHECK_PARTIAL();
    ecode++;
    break;
#endif  /* SUPPORT_UCP */


    /* Match a back reference, possibly repeatedly. Look past the end of the
    item to see if there is repeat information following. The code is similar
    to that for character classes, but repeated for efficiency. Then obey
    similar code to character type repeats - written out again for speed.
    However, if the referenced string is the empty string, always treat
    it as matched, any number of times (otherwise there could be infinite
    loops). If the reference is unset, there are two possibilities:

    (a) In the default, Perl-compatible state, set the length negative;
    this ensures that every attempt at a match fails. We can't just fail
    here, because of the possibility of quantifiers with zero minima.

    (b) If the JavaScript compatibility flag is set, set the length to zero
    so that the back reference matches an empty string.

    Otherwise, set the length to the length of what was matched by the
    referenced subpattern.

    The OP_REF and OP_REFI opcodes are used for a reference to a numbered group
    or to a non-duplicated named group. For a duplicated named group, OP_DNREF
    and OP_DNREFI are used. In this case we must scan the list of groups to
    which the name refers, and use the first one that is set. */

    case OP_DNREF:
    case OP_DNREFI:
    caseless = op == OP_DNREFI;
      {
      int count = GET2(ecode, 1+IMM2_SIZE);
      pcre_uchar *slot = md->name_table + GET2(ecode, 1) * md->name_entry_size;
      ecode += 1 + 2*IMM2_SIZE;

      /* Setting the default length first and initializing 'offset' avoids
      compiler warnings in the REF_REPEAT code. */

      length = (md->jscript_compat)? 0 : -1;
      offset = 0;

      while (count-- > 0)
        {
        offset = GET2(slot, 0) << 1;
        if (offset < offset_top && md->offset_vector[offset] >= 0)
          {
          length = md->offset_vector[offset+1] - md->offset_vector[offset];
          break;
          }
        slot += md->name_entry_size;
        }
      }
    goto REF_REPEAT;

    case OP_REF:
    case OP_REFI:
    caseless = op == OP_REFI;
    offset = GET2(ecode, 1) << 1;               /* Doubled ref number */
    ecode += 1 + IMM2_SIZE;
    if (offset >= offset_top || md->offset_vector[offset] < 0)
      length = (md->jscript_compat)? 0 : -1;
    else
      length = md->offset_vector[offset+1] - md->offset_vector[offset];

    /* Set up for repetition, or handle the non-repeated case */

    REF_REPEAT:
    switch (*ecode)
      {
      case OP_CRSTAR:
      case OP_CRMINSTAR:
      case OP_CRPLUS:
      case OP_CRMINPLUS:
      case OP_CRQUERY:
      case OP_CRMINQUERY:
      c = *ecode++ - OP_CRSTAR;
      minimize = (c & 1) != 0;
      min = rep_min[c];                 /* Pick up values from tables; */
      max = rep_max[c];                 /* zero for max => infinity */
      if (max == 0) max = INT_MAX;
      break;

      case OP_CRRANGE:
      case OP_CRMINRANGE:
      minimize = (*ecode == OP_CRMINRANGE);
      min = GET2(ecode, 1);
      max = GET2(ecode, 1 + IMM2_SIZE);
      if (max == 0) max = INT_MAX;
      ecode += 1 + 2 * IMM2_SIZE;
      break;

      default:               /* No repeat follows */
      if ((length = match_ref(offset, eptr, length, md, caseless)) < 0)
        {
        if (length == -2) eptr = md->end_subject;   /* Partial match */
        CHECK_PARTIAL();
        RRETURN(MATCH_NOMATCH);
        }
      eptr += length;
      continue;              /* With the main loop */
      }

    /* Handle repeated back references. If the length of the reference is
    zero, just continue with the main loop. If the length is negative, it
    means the reference is unset in non-Java-compatible mode. If the minimum is
    zero, we can continue at the same level without recursion. For any other
    minimum, carrying on will result in NOMATCH. */

    if (length == 0) continue;
    if (length < 0 && min == 0) continue;

    /* First, ensure the minimum number of matches are present. We get back
    the length of the reference string explicitly rather than passing the
    address of eptr, so that eptr can be a register variable. */

    for (i = 1; i <= min; i++)
      {
      int slength;
      if ((slength = match_ref(offset, eptr, length, md, caseless)) < 0)
        {
        if (slength == -2) eptr = md->end_subject;   /* Partial match */
        CHECK_PARTIAL();
        RRETURN(MATCH_NOMATCH);
        }
      eptr += slength;
      }

    /* If min = max, continue at the same level without recursion.
    They are not both allowed to be zero. */

    if (min == max) continue;

    /* If minimizing, keep trying and advancing the pointer */

    if (minimize)
      {
      for (fi = min;; fi++)
        {
        int slength;
        RMATCH(eptr, ecode, offset_top, md, eptrb, RM14);
        if (rrc != MATCH_NOMATCH) RRETURN(rrc);
        if (fi >= max) RRETURN(MATCH_NOMATCH);
        if ((slength = match_ref(offset, eptr, length, md, caseless)) < 0)
          {
          if (slength == -2) eptr = md->end_subject;   /* Partial match */
          CHECK_PARTIAL();
          RRETURN(MATCH_NOMATCH);
          }
        eptr += slength;
        }
      /* Control never gets here */
      }

    /* If maximizing, find the longest string and work backwards */

    else
      {
      pp = eptr;
      for (i = min; i < max; i++)
        {
        int slength;
        if ((slength = match_ref(offset, eptr, length, md, caseless)) < 0)
          {
          /* Can't use CHECK_PARTIAL because we don't want to update eptr in
          the soft partial matching case. */

          if (slength == -2 && md->partial != 0 &&
              md->end_subject > md->start_used_ptr)
            {
            md->hitend = TRUE;
            if (md->partial > 1) RRETURN(PCRE_ERROR_PARTIAL);
            }
          break;
          }
        eptr += slength;
        }

      while (eptr >= pp)
        {
        RMATCH(eptr, ecode, offset_top, md, eptrb, RM15);
        if (rrc != MATCH_NOMATCH) RRETURN(rrc);
        eptr -= length;
        }
      RRETURN(MATCH_NOMATCH);
      }
    /* Control never gets here */

    /* Match a bit-mapped character class, possibly repeatedly. This op code is
    used when all the characters in the class have values in the range 0-255,
    and either the matching is caseful, or the characters are in the range
    0-127 when UTF-8 processing is enabled. The only difference between
    OP_CLASS and OP_NCLASS occurs when a data character outside the range is
    encountered.

    First, look past the end of the item to see if there is repeat information
    following. Then obey similar code to character type repeats - written out
    again for speed. */

    case OP_NCLASS:
    case OP_CLASS:
      {
      /* The data variable is saved across frames, so the byte map needs to
      be stored there. */
#define BYTE_MAP ((pcre_uint8 *)data)
      data = ecode + 1;                /* Save for matching */
      ecode += 1 + (32 / sizeof(pcre_uchar)); /* Advance past the item */

      switch (*ecode)
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
        c = *ecode++ - OP_CRSTAR;
        if (c < OP_CRPOSSTAR - OP_CRSTAR) minimize = (c & 1) != 0;
        else possessive = TRUE;
        min = rep_min[c];                 /* Pick up values from tables; */
        max = rep_max[c];                 /* zero for max => infinity */
        if (max == 0) max = INT_MAX;
        break;

        case OP_CRRANGE:
        case OP_CRMINRANGE:
        case OP_CRPOSRANGE:
        minimize = (*ecode == OP_CRMINRANGE);
        possessive = (*ecode == OP_CRPOSRANGE);
        min = GET2(ecode, 1);
        max = GET2(ecode, 1 + IMM2_SIZE);
        if (max == 0) max = INT_MAX;
        ecode += 1 + 2 * IMM2_SIZE;
        break;

        default:               /* No repeat follows */
        min = max = 1;
        break;
        }

      /* First, ensure the minimum number of matches are present. */

#ifdef SUPPORT_UTF
      if (utf)
        {
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          GETCHARINC(c, eptr);
          if (c > 255)
            {
            if (op == OP_CLASS) RRETURN(MATCH_NOMATCH);
            }
          else
            if ((BYTE_MAP[c/8] & (1 << (c&7))) == 0) RRETURN(MATCH_NOMATCH);
          }
        }
      else
#endif
      /* Not UTF mode */
        {
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          c = *eptr++;
#ifndef COMPILE_PCRE8
          if (c > 255)
            {
            if (op == OP_CLASS) RRETURN(MATCH_NOMATCH);
            }
          else
#endif
            if ((BYTE_MAP[c/8] & (1 << (c&7))) == 0) RRETURN(MATCH_NOMATCH);
          }
        }

      /* If max == min we can continue with the main loop without the
      need to recurse. */

      if (min == max) continue;

      /* If minimizing, keep testing the rest of the expression and advancing
      the pointer while it matches the class. */

      if (minimize)
        {
#ifdef SUPPORT_UTF
        if (utf)
          {
          for (fi = min;; fi++)
            {
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM16);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (fi >= max) RRETURN(MATCH_NOMATCH);
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINC(c, eptr);
            if (c > 255)
              {
              if (op == OP_CLASS) RRETURN(MATCH_NOMATCH);
              }
            else
              if ((BYTE_MAP[c/8] & (1 << (c&7))) == 0) RRETURN(MATCH_NOMATCH);
            }
          }
        else
#endif
        /* Not UTF mode */
          {
          for (fi = min;; fi++)
            {
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM17);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (fi >= max) RRETURN(MATCH_NOMATCH);
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            c = *eptr++;
#ifndef COMPILE_PCRE8
            if (c > 255)
              {
              if (op == OP_CLASS) RRETURN(MATCH_NOMATCH);
              }
            else
#endif
              if ((BYTE_MAP[c/8] & (1 << (c&7))) == 0) RRETURN(MATCH_NOMATCH);
            }
          }
        /* Control never gets here */
        }

      /* If maximizing, find the longest possible run, then work backwards. */

      else
        {
        pp = eptr;

#ifdef SUPPORT_UTF
        if (utf)
          {
          for (i = min; i < max; i++)
            {
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(c, eptr, len);
            if (c > 255)
              {
              if (op == OP_CLASS) break;
              }
            else
              if ((BYTE_MAP[c/8] & (1 << (c&7))) == 0) break;
            eptr += len;
            }

          if (possessive) continue;    /* No backtracking */

          for (;;)
            {
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM18);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (eptr-- == pp) break;        /* Stop if tried at original pos */
            BACKCHAR(eptr);
            }
          }
        else
#endif
          /* Not UTF mode */
          {
          for (i = min; i < max; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            c = *eptr;
#ifndef COMPILE_PCRE8
            if (c > 255)
              {
              if (op == OP_CLASS) break;
              }
            else
#endif
              if ((BYTE_MAP[c/8] & (1 << (c&7))) == 0) break;
            eptr++;
            }

          if (possessive) continue;    /* No backtracking */

          while (eptr >= pp)
            {
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM19);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            eptr--;
            }
          }

        RRETURN(MATCH_NOMATCH);
        }
#undef BYTE_MAP
      }
    /* Control never gets here */


    /* Match an extended character class. In the 8-bit library, this opcode is
    encountered only when UTF-8 mode mode is supported. In the 16-bit and
    32-bit libraries, codepoints greater than 255 may be encountered even when
    UTF is not supported. */

#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
    case OP_XCLASS:
      {
      data = ecode + 1 + LINK_SIZE;                /* Save for matching */
      ecode += GET(ecode, 1);                      /* Advance past the item */

      switch (*ecode)
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
        c = *ecode++ - OP_CRSTAR;
        if (c < OP_CRPOSSTAR - OP_CRSTAR) minimize = (c & 1) != 0;
        else possessive = TRUE;
        min = rep_min[c];                 /* Pick up values from tables; */
        max = rep_max[c];                 /* zero for max => infinity */
        if (max == 0) max = INT_MAX;
        break;

        case OP_CRRANGE:
        case OP_CRMINRANGE:
        case OP_CRPOSRANGE:
        minimize = (*ecode == OP_CRMINRANGE);
        possessive = (*ecode == OP_CRPOSRANGE);
        min = GET2(ecode, 1);
        max = GET2(ecode, 1 + IMM2_SIZE);
        if (max == 0) max = INT_MAX;
        ecode += 1 + 2 * IMM2_SIZE;
        break;

        default:               /* No repeat follows */
        min = max = 1;
        break;
        }

      /* First, ensure the minimum number of matches are present. */

      for (i = 1; i <= min; i++)
        {
        if (eptr >= md->end_subject)
          {
          SCHECK_PARTIAL();
          RRETURN(MATCH_NOMATCH);
          }
        GETCHARINCTEST(c, eptr);
        if (!PRIV(xclass)(c, data, utf)) RRETURN(MATCH_NOMATCH);
        }

      /* If max == min we can continue with the main loop without the
      need to recurse. */

      if (min == max) continue;

      /* If minimizing, keep testing the rest of the expression and advancing
      the pointer while it matches the class. */

      if (minimize)
        {
        for (fi = min;; fi++)
          {
          RMATCH(eptr, ecode, offset_top, md, eptrb, RM20);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          if (fi >= max) RRETURN(MATCH_NOMATCH);
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          GETCHARINCTEST(c, eptr);
          if (!PRIV(xclass)(c, data, utf)) RRETURN(MATCH_NOMATCH);
          }
        /* Control never gets here */
        }

      /* If maximizing, find the longest possible run, then work backwards. */

      else
        {
        pp = eptr;
        for (i = min; i < max; i++)
          {
          int len = 1;
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            break;
            }
#ifdef SUPPORT_UTF
          GETCHARLENTEST(c, eptr, len);
#else
          c = *eptr;
#endif
          if (!PRIV(xclass)(c, data, utf)) break;
          eptr += len;
          }

        if (possessive) continue;    /* No backtracking */

        for(;;)
          {
          RMATCH(eptr, ecode, offset_top, md, eptrb, RM21);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          if (eptr-- == pp) break;        /* Stop if tried at original pos */
#ifdef SUPPORT_UTF
          if (utf) BACKCHAR(eptr);
#endif
          }
        RRETURN(MATCH_NOMATCH);
        }

      /* Control never gets here */
      }
#endif    /* End of XCLASS */

    /* Match a single character, casefully */

    case OP_CHAR:
#ifdef SUPPORT_UTF
    if (utf)
      {
      length = 1;
      ecode++;
      GETCHARLEN(fc, ecode, length);
      if (length > md->end_subject - eptr)
        {
        CHECK_PARTIAL();             /* Not SCHECK_PARTIAL() */
        RRETURN(MATCH_NOMATCH);
        }
      while (length-- > 0) if (*ecode++ != UCHAR21INC(eptr)) RRETURN(MATCH_NOMATCH);
      }
    else
#endif
    /* Not UTF mode */
      {
      if (md->end_subject - eptr < 1)
        {
        SCHECK_PARTIAL();            /* This one can use SCHECK_PARTIAL() */
        RRETURN(MATCH_NOMATCH);
        }
      if (ecode[1] != *eptr++) RRETURN(MATCH_NOMATCH);
      ecode += 2;
      }
    break;

    /* Match a single character, caselessly. If we are at the end of the
    subject, give up immediately. */

    case OP_CHARI:
    if (eptr >= md->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }

#ifdef SUPPORT_UTF
    if (utf)
      {
      length = 1;
      ecode++;
      GETCHARLEN(fc, ecode, length);

      /* If the pattern character's value is < 128, we have only one byte, and
      we know that its other case must also be one byte long, so we can use the
      fast lookup table. We know that there is at least one byte left in the
      subject. */

      if (fc < 128)
        {
        pcre_uint32 cc = UCHAR21(eptr);
        if (md->lcc[fc] != TABLE_GET(cc, md->lcc, cc)) RRETURN(MATCH_NOMATCH);
        ecode++;
        eptr++;
        }

      /* Otherwise we must pick up the subject character. Note that we cannot
      use the value of "length" to check for sufficient bytes left, because the
      other case of the character may have more or fewer bytes.  */

      else
        {
        pcre_uint32 dc;
        GETCHARINC(dc, eptr);
        ecode += length;

        /* If we have Unicode property support, we can use it to test the other
        case of the character, if there is one. */

        if (fc != dc)
          {
#ifdef SUPPORT_UCP
          if (dc != UCD_OTHERCASE(fc))
#endif
            RRETURN(MATCH_NOMATCH);
          }
        }
      }
    else
#endif   /* SUPPORT_UTF */

    /* Not UTF mode */
      {
      if (TABLE_GET(ecode[1], md->lcc, ecode[1])
          != TABLE_GET(*eptr, md->lcc, *eptr)) RRETURN(MATCH_NOMATCH);
      eptr++;
      ecode += 2;
      }
    break;

    /* Match a single character repeatedly. */

    case OP_EXACT:
    case OP_EXACTI:
    min = max = GET2(ecode, 1);
    ecode += 1 + IMM2_SIZE;
    goto REPEATCHAR;

    case OP_POSUPTO:
    case OP_POSUPTOI:
    possessive = TRUE;
    /* Fall through */

    case OP_UPTO:
    case OP_UPTOI:
    case OP_MINUPTO:
    case OP_MINUPTOI:
    min = 0;
    max = GET2(ecode, 1);
    minimize = *ecode == OP_MINUPTO || *ecode == OP_MINUPTOI;
    ecode += 1 + IMM2_SIZE;
    goto REPEATCHAR;

    case OP_POSSTAR:
    case OP_POSSTARI:
    possessive = TRUE;
    min = 0;
    max = INT_MAX;
    ecode++;
    goto REPEATCHAR;

    case OP_POSPLUS:
    case OP_POSPLUSI:
    possessive = TRUE;
    min = 1;
    max = INT_MAX;
    ecode++;
    goto REPEATCHAR;

    case OP_POSQUERY:
    case OP_POSQUERYI:
    possessive = TRUE;
    min = 0;
    max = 1;
    ecode++;
    goto REPEATCHAR;

    case OP_STAR:
    case OP_STARI:
    case OP_MINSTAR:
    case OP_MINSTARI:
    case OP_PLUS:
    case OP_PLUSI:
    case OP_MINPLUS:
    case OP_MINPLUSI:
    case OP_QUERY:
    case OP_QUERYI:
    case OP_MINQUERY:
    case OP_MINQUERYI:
    c = *ecode++ - ((op < OP_STARI)? OP_STAR : OP_STARI);
    minimize = (c & 1) != 0;
    min = rep_min[c];                 /* Pick up values from tables; */
    max = rep_max[c];                 /* zero for max => infinity */
    if (max == 0) max = INT_MAX;

    /* Common code for all repeated single-character matches. We first check
    for the minimum number of characters. If the minimum equals the maximum, we
    are done. Otherwise, if minimizing, check the rest of the pattern for a
    match; if there isn't one, advance up to the maximum, one character at a
    time.

    If maximizing, advance up to the maximum number of matching characters,
    until eptr is past the end of the maximum run. If possessive, we are
    then done (no backing up). Otherwise, match at this position; anything
    other than no match is immediately returned. For nomatch, back up one
    character, unless we are matching \R and the last thing matched was
    \r\n, in which case, back up two bytes. When we reach the first optional
    character position, we can save stack by doing a tail recurse.

    The various UTF/non-UTF and caseful/caseless cases are handled separately,
    for speed. */

    REPEATCHAR:
#ifdef SUPPORT_UTF
    if (utf)
      {
      length = 1;
      charptr = ecode;
      GETCHARLEN(fc, ecode, length);
      ecode += length;

      /* Handle multibyte character matching specially here. There is
      support for caseless matching if UCP support is present. */

      if (length > 1)
        {
#ifdef SUPPORT_UCP
        pcre_uint32 othercase;
        if (op >= OP_STARI &&     /* Caseless */
            (othercase = UCD_OTHERCASE(fc)) != fc)
          oclength = PRIV(ord2utf)(othercase, occhars);
        else oclength = 0;
#endif  /* SUPPORT_UCP */

        for (i = 1; i <= min; i++)
          {
          if (eptr <= md->end_subject - length &&
            memcmp(eptr, charptr, IN_UCHARS(length)) == 0) eptr += length;
#ifdef SUPPORT_UCP
          else if (oclength > 0 &&
                   eptr <= md->end_subject - oclength &&
                   memcmp(eptr, occhars, IN_UCHARS(oclength)) == 0) eptr += oclength;
#endif  /* SUPPORT_UCP */
          else
            {
            CHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          }

        if (min == max) continue;

        if (minimize)
          {
          for (fi = min;; fi++)
            {
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM22);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (fi >= max) RRETURN(MATCH_NOMATCH);
            if (eptr <= md->end_subject - length &&
              memcmp(eptr, charptr, IN_UCHARS(length)) == 0) eptr += length;
#ifdef SUPPORT_UCP
            else if (oclength > 0 &&
                     eptr <= md->end_subject - oclength &&
                     memcmp(eptr, occhars, IN_UCHARS(oclength)) == 0) eptr += oclength;
#endif  /* SUPPORT_UCP */
            else
              {
              CHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            }
          /* Control never gets here */
          }

        else  /* Maximize */
          {
          pp = eptr;
          for (i = min; i < max; i++)
            {
            if (eptr <= md->end_subject - length &&
                memcmp(eptr, charptr, IN_UCHARS(length)) == 0) eptr += length;
#ifdef SUPPORT_UCP
            else if (oclength > 0 &&
                     eptr <= md->end_subject - oclength &&
                     memcmp(eptr, occhars, IN_UCHARS(oclength)) == 0) eptr += oclength;
#endif  /* SUPPORT_UCP */
            else
              {
              CHECK_PARTIAL();
              break;
              }
            }

          if (possessive) continue;    /* No backtracking */
          for(;;)
            {
            if (eptr <= pp) goto TAIL_RECURSE;
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM23);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
#ifdef SUPPORT_UCP
            eptr--;
            BACKCHAR(eptr);
#else   /* without SUPPORT_UCP */
            eptr -= length;
#endif  /* SUPPORT_UCP */
            }
          }
        /* Control never gets here */
        }

      /* If the length of a UTF-8 character is 1, we fall through here, and
      obey the code as for non-UTF-8 characters below, though in this case the
      value of fc will always be < 128. */
      }
    else
#endif  /* SUPPORT_UTF */
      /* When not in UTF-8 mode, load a single-byte character. */
      fc = *ecode++;

    /* The value of fc at this point is always one character, though we may
    or may not be in UTF mode. The code is duplicated for the caseless and
    caseful cases, for speed, since matching characters is likely to be quite
    common. First, ensure the minimum number of matches are present. If min =
    max, continue at the same level without recursing. Otherwise, if
    minimizing, keep trying the rest of the expression and advancing one
    matching character if failing, up to the maximum. Alternatively, if
    maximizing, find the maximum number of characters and work backwards. */

    DPRINTF(("matching %c{%d,%d} against subject %.*s\n", fc, min, max,
      max, (char *)eptr));

    if (op >= OP_STARI)  /* Caseless */
      {
#ifdef COMPILE_PCRE8
      /* fc must be < 128 if UTF is enabled. */
      foc = md->fcc[fc];
#else
#ifdef SUPPORT_UTF
#ifdef SUPPORT_UCP
      if (utf && fc > 127)
        foc = UCD_OTHERCASE(fc);
#else
      if (utf && fc > 127)
        foc = fc;
#endif /* SUPPORT_UCP */
      else
#endif /* SUPPORT_UTF */
        foc = TABLE_GET(fc, md->fcc, fc);
#endif /* COMPILE_PCRE8 */

      for (i = 1; i <= min; i++)
        {
        pcre_uint32 cc;                 /* Faster than pcre_uchar */
        if (eptr >= md->end_subject)
          {
          SCHECK_PARTIAL();
          RRETURN(MATCH_NOMATCH);
          }
        cc = UCHAR21TEST(eptr);
        if (fc != cc && foc != cc) RRETURN(MATCH_NOMATCH);
        eptr++;
        }
      if (min == max) continue;
      if (minimize)
        {
        for (fi = min;; fi++)
          {
          pcre_uint32 cc;               /* Faster than pcre_uchar */
          RMATCH(eptr, ecode, offset_top, md, eptrb, RM24);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          if (fi >= max) RRETURN(MATCH_NOMATCH);
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          cc = UCHAR21TEST(eptr);
          if (fc != cc && foc != cc) RRETURN(MATCH_NOMATCH);
          eptr++;
          }
        /* Control never gets here */
        }
      else  /* Maximize */
        {
        pp = eptr;
        for (i = min; i < max; i++)
          {
          pcre_uint32 cc;               /* Faster than pcre_uchar */
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            break;
            }
          cc = UCHAR21TEST(eptr);
          if (fc != cc && foc != cc) break;
          eptr++;
          }
        if (possessive) continue;       /* No backtracking */
        for (;;)
          {
          if (eptr == pp) goto TAIL_RECURSE;
          RMATCH(eptr, ecode, offset_top, md, eptrb, RM25);
          eptr--;
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          }
        /* Control never gets here */
        }
      }

    /* Caseful comparisons (includes all multi-byte characters) */

    else
      {
      for (i = 1; i <= min; i++)
        {
        if (eptr >= md->end_subject)
          {
          SCHECK_PARTIAL();
          RRETURN(MATCH_NOMATCH);
          }
        if (fc != UCHAR21INCTEST(eptr)) RRETURN(MATCH_NOMATCH);
        }

      if (min == max) continue;

      if (minimize)
        {
        for (fi = min;; fi++)
          {
          RMATCH(eptr, ecode, offset_top, md, eptrb, RM26);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          if (fi >= max) RRETURN(MATCH_NOMATCH);
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (fc != UCHAR21INCTEST(eptr)) RRETURN(MATCH_NOMATCH);
          }
        /* Control never gets here */
        }
      else  /* Maximize */
        {
        pp = eptr;
        for (i = min; i < max; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            break;
            }
          if (fc != UCHAR21TEST(eptr)) break;
          eptr++;
          }
        if (possessive) continue;    /* No backtracking */
        for (;;)
          {
          if (eptr == pp) goto TAIL_RECURSE;
          RMATCH(eptr, ecode, offset_top, md, eptrb, RM27);
          eptr--;
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          }
        /* Control never gets here */
        }
      }
    /* Control never gets here */

    /* Match a negated single one-byte character. The character we are
    checking can be multibyte. */

    case OP_NOT:
    case OP_NOTI:
    if (eptr >= md->end_subject)
      {
      SCHECK_PARTIAL();
      RRETURN(MATCH_NOMATCH);
      }
#ifdef SUPPORT_UTF
    if (utf)
      {
      register pcre_uint32 ch, och;

      ecode++;
      GETCHARINC(ch, ecode);
      GETCHARINC(c, eptr);

      if (op == OP_NOT)
        {
        if (ch == c) RRETURN(MATCH_NOMATCH);
        }
      else
        {
#ifdef SUPPORT_UCP
        if (ch > 127)
          och = UCD_OTHERCASE(ch);
#else
        if (ch > 127)
          och = ch;
#endif /* SUPPORT_UCP */
        else
          och = TABLE_GET(ch, md->fcc, ch);
        if (ch == c || och == c) RRETURN(MATCH_NOMATCH);
        }
      }
    else
#endif
      {
      register pcre_uint32 ch = ecode[1];
      c = *eptr++;
      if (ch == c || (op == OP_NOTI && TABLE_GET(ch, md->fcc, ch) == c))
        RRETURN(MATCH_NOMATCH);
      ecode += 2;
      }
    break;

    /* Match a negated single one-byte character repeatedly. This is almost a
    repeat of the code for a repeated single character, but I haven't found a
    nice way of commoning these up that doesn't require a test of the
    positive/negative option for each character match. Maybe that wouldn't add
    very much to the time taken, but character matching *is* what this is all
    about... */

    case OP_NOTEXACT:
    case OP_NOTEXACTI:
    min = max = GET2(ecode, 1);
    ecode += 1 + IMM2_SIZE;
    goto REPEATNOTCHAR;

    case OP_NOTUPTO:
    case OP_NOTUPTOI:
    case OP_NOTMINUPTO:
    case OP_NOTMINUPTOI:
    min = 0;
    max = GET2(ecode, 1);
    minimize = *ecode == OP_NOTMINUPTO || *ecode == OP_NOTMINUPTOI;
    ecode += 1 + IMM2_SIZE;
    goto REPEATNOTCHAR;

    case OP_NOTPOSSTAR:
    case OP_NOTPOSSTARI:
    possessive = TRUE;
    min = 0;
    max = INT_MAX;
    ecode++;
    goto REPEATNOTCHAR;

    case OP_NOTPOSPLUS:
    case OP_NOTPOSPLUSI:
    possessive = TRUE;
    min = 1;
    max = INT_MAX;
    ecode++;
    goto REPEATNOTCHAR;

    case OP_NOTPOSQUERY:
    case OP_NOTPOSQUERYI:
    possessive = TRUE;
    min = 0;
    max = 1;
    ecode++;
    goto REPEATNOTCHAR;

    case OP_NOTPOSUPTO:
    case OP_NOTPOSUPTOI:
    possessive = TRUE;
    min = 0;
    max = GET2(ecode, 1);
    ecode += 1 + IMM2_SIZE;
    goto REPEATNOTCHAR;

    case OP_NOTSTAR:
    case OP_NOTSTARI:
    case OP_NOTMINSTAR:
    case OP_NOTMINSTARI:
    case OP_NOTPLUS:
    case OP_NOTPLUSI:
    case OP_NOTMINPLUS:
    case OP_NOTMINPLUSI:
    case OP_NOTQUERY:
    case OP_NOTQUERYI:
    case OP_NOTMINQUERY:
    case OP_NOTMINQUERYI:
    c = *ecode++ - ((op >= OP_NOTSTARI)? OP_NOTSTARI: OP_NOTSTAR);
    minimize = (c & 1) != 0;
    min = rep_min[c];                 /* Pick up values from tables; */
    max = rep_max[c];                 /* zero for max => infinity */
    if (max == 0) max = INT_MAX;

    /* Common code for all repeated single-byte matches. */

    REPEATNOTCHAR:
    GETCHARINCTEST(fc, ecode);

    /* The code is duplicated for the caseless and caseful cases, for speed,
    since matching characters is likely to be quite common. First, ensure the
    minimum number of matches are present. If min = max, continue at the same
    level without recursing. Otherwise, if minimizing, keep trying the rest of
    the expression and advancing one matching character if failing, up to the
    maximum. Alternatively, if maximizing, find the maximum number of
    characters and work backwards. */

    DPRINTF(("negative matching %c{%d,%d} against subject %.*s\n", fc, min, max,
      max, (char *)eptr));

    if (op >= OP_NOTSTARI)     /* Caseless */
      {
#ifdef SUPPORT_UTF
#ifdef SUPPORT_UCP
      if (utf && fc > 127)
        foc = UCD_OTHERCASE(fc);
#else
      if (utf && fc > 127)
        foc = fc;
#endif /* SUPPORT_UCP */
      else
#endif /* SUPPORT_UTF */
        foc = TABLE_GET(fc, md->fcc, fc);

#ifdef SUPPORT_UTF
      if (utf)
        {
        register pcre_uint32 d;
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          GETCHARINC(d, eptr);
          if (fc == d || (unsigned int)foc == d) RRETURN(MATCH_NOMATCH);
          }
        }
      else
#endif  /* SUPPORT_UTF */
      /* Not UTF mode */
        {
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (fc == *eptr || foc == *eptr) RRETURN(MATCH_NOMATCH);
          eptr++;
          }
        }

      if (min == max) continue;

      if (minimize)
        {
#ifdef SUPPORT_UTF
        if (utf)
          {
          register pcre_uint32 d;
          for (fi = min;; fi++)
            {
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM28);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (fi >= max) RRETURN(MATCH_NOMATCH);
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINC(d, eptr);
            if (fc == d || (unsigned int)foc == d) RRETURN(MATCH_NOMATCH);
            }
          }
        else
#endif  /*SUPPORT_UTF */
        /* Not UTF mode */
          {
          for (fi = min;; fi++)
            {
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM29);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (fi >= max) RRETURN(MATCH_NOMATCH);
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            if (fc == *eptr || foc == *eptr) RRETURN(MATCH_NOMATCH);
            eptr++;
            }
          }
        /* Control never gets here */
        }

      /* Maximize case */

      else
        {
        pp = eptr;

#ifdef SUPPORT_UTF
        if (utf)
          {
          register pcre_uint32 d;
          for (i = min; i < max; i++)
            {
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(d, eptr, len);
            if (fc == d || (unsigned int)foc == d) break;
            eptr += len;
            }
          if (possessive) continue;    /* No backtracking */
          for(;;)
            {
            if (eptr <= pp) goto TAIL_RECURSE;
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM30);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            eptr--;
            BACKCHAR(eptr);
            }
          }
        else
#endif  /* SUPPORT_UTF */
        /* Not UTF mode */
          {
          for (i = min; i < max; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            if (fc == *eptr || foc == *eptr) break;
            eptr++;
            }
          if (possessive) continue;    /* No backtracking */
          for (;;)
            {
            if (eptr == pp) goto TAIL_RECURSE;
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM31);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            eptr--;
            }
          }
        /* Control never gets here */
        }
      }

    /* Caseful comparisons */

    else
      {
#ifdef SUPPORT_UTF
      if (utf)
        {
        register pcre_uint32 d;
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          GETCHARINC(d, eptr);
          if (fc == d) RRETURN(MATCH_NOMATCH);
          }
        }
      else
#endif
      /* Not UTF mode */
        {
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (fc == *eptr++) RRETURN(MATCH_NOMATCH);
          }
        }

      if (min == max) continue;

      if (minimize)
        {
#ifdef SUPPORT_UTF
        if (utf)
          {
          register pcre_uint32 d;
          for (fi = min;; fi++)
            {
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM32);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (fi >= max) RRETURN(MATCH_NOMATCH);
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINC(d, eptr);
            if (fc == d) RRETURN(MATCH_NOMATCH);
            }
          }
        else
#endif
        /* Not UTF mode */
          {
          for (fi = min;; fi++)
            {
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM33);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (fi >= max) RRETURN(MATCH_NOMATCH);
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            if (fc == *eptr++) RRETURN(MATCH_NOMATCH);
            }
          }
        /* Control never gets here */
        }

      /* Maximize case */

      else
        {
        pp = eptr;

#ifdef SUPPORT_UTF
        if (utf)
          {
          register pcre_uint32 d;
          for (i = min; i < max; i++)
            {
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(d, eptr, len);
            if (fc == d) break;
            eptr += len;
            }
          if (possessive) continue;    /* No backtracking */
          for(;;)
            {
            if (eptr <= pp) goto TAIL_RECURSE;
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM34);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            eptr--;
            BACKCHAR(eptr);
            }
          }
        else
#endif
        /* Not UTF mode */
          {
          for (i = min; i < max; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            if (fc == *eptr) break;
            eptr++;
            }
          if (possessive) continue;    /* No backtracking */
          for (;;)
            {
            if (eptr == pp) goto TAIL_RECURSE;
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM35);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            eptr--;
            }
          }
        /* Control never gets here */
        }
      }
    /* Control never gets here */

    /* Match a single character type repeatedly; several different opcodes
    share code. This is very similar to the code for single characters, but we
    repeat it in the interests of efficiency. */

    case OP_TYPEEXACT:
    min = max = GET2(ecode, 1);
    minimize = TRUE;
    ecode += 1 + IMM2_SIZE;
    goto REPEATTYPE;

    case OP_TYPEUPTO:
    case OP_TYPEMINUPTO:
    min = 0;
    max = GET2(ecode, 1);
    minimize = *ecode == OP_TYPEMINUPTO;
    ecode += 1 + IMM2_SIZE;
    goto REPEATTYPE;

    case OP_TYPEPOSSTAR:
    possessive = TRUE;
    min = 0;
    max = INT_MAX;
    ecode++;
    goto REPEATTYPE;

    case OP_TYPEPOSPLUS:
    possessive = TRUE;
    min = 1;
    max = INT_MAX;
    ecode++;
    goto REPEATTYPE;

    case OP_TYPEPOSQUERY:
    possessive = TRUE;
    min = 0;
    max = 1;
    ecode++;
    goto REPEATTYPE;

    case OP_TYPEPOSUPTO:
    possessive = TRUE;
    min = 0;
    max = GET2(ecode, 1);
    ecode += 1 + IMM2_SIZE;
    goto REPEATTYPE;

    case OP_TYPESTAR:
    case OP_TYPEMINSTAR:
    case OP_TYPEPLUS:
    case OP_TYPEMINPLUS:
    case OP_TYPEQUERY:
    case OP_TYPEMINQUERY:
    c = *ecode++ - OP_TYPESTAR;
    minimize = (c & 1) != 0;
    min = rep_min[c];                 /* Pick up values from tables; */
    max = rep_max[c];                 /* zero for max => infinity */
    if (max == 0) max = INT_MAX;

    /* Common code for all repeated single character type matches. Note that
    in UTF-8 mode, '.' matches a character of any length, but for the other
    character types, the valid characters are all one-byte long. */

    REPEATTYPE:
    ctype = *ecode++;      /* Code for the character type */

#ifdef SUPPORT_UCP
    if (ctype == OP_PROP || ctype == OP_NOTPROP)
      {
      prop_fail_result = ctype == OP_NOTPROP;
      prop_type = *ecode++;
      prop_value = *ecode++;
      }
    else prop_type = -1;
#endif

    /* First, ensure the minimum number of matches are present. Use inline
    code for maximizing the speed, and do the type test once at the start
    (i.e. keep it out of the loop). Separate the UTF-8 code completely as that
    is tidier. Also separate the UCP code, which can be the same for both UTF-8
    and single-bytes. */

    if (min > 0)
      {
#ifdef SUPPORT_UCP
      if (prop_type >= 0)
        {
        switch(prop_type)
          {
          case PT_ANY:
          if (prop_fail_result) RRETURN(MATCH_NOMATCH);
          for (i = 1; i <= min; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(c, eptr);
            }
          break;

          case PT_LAMP:
          for (i = 1; i <= min; i++)
            {
            int chartype;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(c, eptr);
            chartype = UCD_CHARTYPE(c);
            if ((chartype == ucp_Lu ||
                 chartype == ucp_Ll ||
                 chartype == ucp_Lt) == prop_fail_result)
              RRETURN(MATCH_NOMATCH);
            }
          break;

          case PT_GC:
          for (i = 1; i <= min; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(c, eptr);
            if ((UCD_CATEGORY(c) == prop_value) == prop_fail_result)
              RRETURN(MATCH_NOMATCH);
            }
          break;

          case PT_PC:
          for (i = 1; i <= min; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(c, eptr);
            if ((UCD_CHARTYPE(c) == prop_value) == prop_fail_result)
              RRETURN(MATCH_NOMATCH);
            }
          break;

          case PT_SC:
          for (i = 1; i <= min; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(c, eptr);
            if ((UCD_SCRIPT(c) == prop_value) == prop_fail_result)
              RRETURN(MATCH_NOMATCH);
            }
          break;

          case PT_ALNUM:
          for (i = 1; i <= min; i++)
            {
            int category;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(c, eptr);
            category = UCD_CATEGORY(c);
            if ((category == ucp_L || category == ucp_N) == prop_fail_result)
              RRETURN(MATCH_NOMATCH);
            }
          break;

          /* Perl space used to exclude VT, but from Perl 5.18 it is included,
          which means that Perl space and POSIX space are now identical. PCRE
          was changed at release 8.34. */

          case PT_SPACE:    /* Perl space */
          case PT_PXSPACE:  /* POSIX space */
          for (i = 1; i <= min; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(c, eptr);
            switch(c)
              {
              HSPACE_CASES:
              VSPACE_CASES:
              if (prop_fail_result) RRETURN(MATCH_NOMATCH);
              break;

              default:
              if ((UCD_CATEGORY(c) == ucp_Z) == prop_fail_result)
                RRETURN(MATCH_NOMATCH);
              break;
              }
            }
          break;

          case PT_WORD:
          for (i = 1; i <= min; i++)
            {
            int category;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(c, eptr);
            category = UCD_CATEGORY(c);
            if ((category == ucp_L || category == ucp_N || c == CHAR_UNDERSCORE)
                   == prop_fail_result)
              RRETURN(MATCH_NOMATCH);
            }
          break;

          case PT_CLIST:
          for (i = 1; i <= min; i++)
            {
            const pcre_uint32 *cp;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(c, eptr);
            cp = PRIV(ucd_caseless_sets) + prop_value;
            for (;;)
              {
              if (c < *cp)
                { if (prop_fail_result) break; else { RRETURN(MATCH_NOMATCH); } }
              if (c == *cp++)
                { if (prop_fail_result) { RRETURN(MATCH_NOMATCH); } else break; }
              }
            }
          break;

          case PT_UCNC:
          for (i = 1; i <= min; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(c, eptr);
            if ((c == CHAR_DOLLAR_SIGN || c == CHAR_COMMERCIAL_AT ||
                 c == CHAR_GRAVE_ACCENT || (c >= 0xa0 && c <= 0xd7ff) ||
                 c >= 0xe000) == prop_fail_result)
              RRETURN(MATCH_NOMATCH);
            }
          break;

          /* This should not occur */

          default:
          RRETURN(PCRE_ERROR_INTERNAL);
          }
        }

      /* Match extended Unicode sequences. We will get here only if the
      support is in the binary; otherwise a compile-time error occurs. */

      else if (ctype == OP_EXTUNI)
        {
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          else
            {
            int lgb, rgb;
            GETCHARINCTEST(c, eptr);
            lgb = UCD_GRAPHBREAK(c);
           while (eptr < md->end_subject)
              {
              int len = 1;
              if (!utf) c = *eptr; else { GETCHARLEN(c, eptr, len); }
              rgb = UCD_GRAPHBREAK(c);
              if ((PRIV(ucp_gbtable)[lgb] & (1 << rgb)) == 0) break;
              lgb = rgb;
              eptr += len;
              }
            }
          CHECK_PARTIAL();
          }
        }

      else
#endif     /* SUPPORT_UCP */

/* Handle all other cases when the coding is UTF-8 */

#ifdef SUPPORT_UTF
      if (utf) switch(ctype)
        {
        case OP_ANY:
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (IS_NEWLINE(eptr)) RRETURN(MATCH_NOMATCH);
          if (md->partial != 0 &&
              eptr + 1 >= md->end_subject &&
              NLBLOCK->nltype == NLTYPE_FIXED &&
              NLBLOCK->nllen == 2 &&
              UCHAR21(eptr) == NLBLOCK->nl[0])
            {
            md->hitend = TRUE;
            if (md->partial > 1) RRETURN(PCRE_ERROR_PARTIAL);
            }
          eptr++;
          ACROSSCHAR(eptr < md->end_subject, *eptr, eptr++);
          }
        break;

        case OP_ALLANY:
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          eptr++;
          ACROSSCHAR(eptr < md->end_subject, *eptr, eptr++);
          }
        break;

        case OP_ANYBYTE:
        if (eptr > md->end_subject - min) RRETURN(MATCH_NOMATCH);
        eptr += min;
        break;

        case OP_ANYNL:
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          GETCHARINC(c, eptr);
          switch(c)
            {
            default: RRETURN(MATCH_NOMATCH);

            case CHAR_CR:
            if (eptr < md->end_subject && UCHAR21(eptr) == CHAR_LF) eptr++;
            break;

            case CHAR_LF:
            break;

            case CHAR_VT:
            case CHAR_FF:
            case CHAR_NEL:
#ifndef EBCDIC
            case 0x2028:
            case 0x2029:
#endif  /* Not EBCDIC */
            if (md->bsr_anycrlf) RRETURN(MATCH_NOMATCH);
            break;
            }
          }
        break;

        case OP_NOT_HSPACE:
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          GETCHARINC(c, eptr);
          switch(c)
            {
            HSPACE_CASES: RRETURN(MATCH_NOMATCH);  /* Byte and multibyte cases */
            default: break;
            }
          }
        break;

        case OP_HSPACE:
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          GETCHARINC(c, eptr);
          switch(c)
            {
            HSPACE_CASES: break;  /* Byte and multibyte cases */
            default: RRETURN(MATCH_NOMATCH);
            }
          }
        break;

        case OP_NOT_VSPACE:
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          GETCHARINC(c, eptr);
          switch(c)
            {
            VSPACE_CASES: RRETURN(MATCH_NOMATCH);
            default: break;
            }
          }
        break;

        case OP_VSPACE:
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          GETCHARINC(c, eptr);
          switch(c)
            {
            VSPACE_CASES: break;
            default: RRETURN(MATCH_NOMATCH);
            }
          }
        break;

        case OP_NOT_DIGIT:
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          GETCHARINC(c, eptr);
          if (c < 128 && (md->ctypes[c] & ctype_digit) != 0)
            RRETURN(MATCH_NOMATCH);
          }
        break;

        case OP_DIGIT:
        for (i = 1; i <= min; i++)
          {
          pcre_uint32 cc;
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          cc = UCHAR21(eptr);
          if (cc >= 128 || (md->ctypes[cc] & ctype_digit) == 0)
            RRETURN(MATCH_NOMATCH);
          eptr++;
          /* No need to skip more bytes - we know it's a 1-byte character */
          }
        break;

        case OP_NOT_WHITESPACE:
        for (i = 1; i <= min; i++)
          {
          pcre_uint32 cc;
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          cc = UCHAR21(eptr);
          if (cc < 128 && (md->ctypes[cc] & ctype_space) != 0)
            RRETURN(MATCH_NOMATCH);
          eptr++;
          ACROSSCHAR(eptr < md->end_subject, *eptr, eptr++);
          }
        break;

        case OP_WHITESPACE:
        for (i = 1; i <= min; i++)
          {
          pcre_uint32 cc;
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          cc = UCHAR21(eptr);
          if (cc >= 128 || (md->ctypes[cc] & ctype_space) == 0)
            RRETURN(MATCH_NOMATCH);
          eptr++;
          /* No need to skip more bytes - we know it's a 1-byte character */
          }
        break;

        case OP_NOT_WORDCHAR:
        for (i = 1; i <= min; i++)
          {
          pcre_uint32 cc;
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          cc = UCHAR21(eptr);
          if (cc < 128 && (md->ctypes[cc] & ctype_word) != 0)
            RRETURN(MATCH_NOMATCH);
          eptr++;
          ACROSSCHAR(eptr < md->end_subject, *eptr, eptr++);
          }
        break;

        case OP_WORDCHAR:
        for (i = 1; i <= min; i++)
          {
          pcre_uint32 cc;
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          cc = UCHAR21(eptr);
          if (cc >= 128 || (md->ctypes[cc] & ctype_word) == 0)
            RRETURN(MATCH_NOMATCH);
          eptr++;
          /* No need to skip more bytes - we know it's a 1-byte character */
          }
        break;

        default:
        RRETURN(PCRE_ERROR_INTERNAL);
        }  /* End switch(ctype) */

      else
#endif     /* SUPPORT_UTF */

      /* Code for the non-UTF-8 case for minimum matching of operators other
      than OP_PROP and OP_NOTPROP. */

      switch(ctype)
        {
        case OP_ANY:
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (IS_NEWLINE(eptr)) RRETURN(MATCH_NOMATCH);
          if (md->partial != 0 &&
              eptr + 1 >= md->end_subject &&
              NLBLOCK->nltype == NLTYPE_FIXED &&
              NLBLOCK->nllen == 2 &&
              *eptr == NLBLOCK->nl[0])
            {
            md->hitend = TRUE;
            if (md->partial > 1) RRETURN(PCRE_ERROR_PARTIAL);
            }
          eptr++;
          }
        break;

        case OP_ALLANY:
        if (eptr > md->end_subject - min)
          {
          SCHECK_PARTIAL();
          RRETURN(MATCH_NOMATCH);
          }
        eptr += min;
        break;

        case OP_ANYBYTE:
        if (eptr > md->end_subject - min)
          {
          SCHECK_PARTIAL();
          RRETURN(MATCH_NOMATCH);
          }
        eptr += min;
        break;

        case OP_ANYNL:
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          switch(*eptr++)
            {
            default: RRETURN(MATCH_NOMATCH);

            case CHAR_CR:
            if (eptr < md->end_subject && *eptr == CHAR_LF) eptr++;
            break;

            case CHAR_LF:
            break;

            case CHAR_VT:
            case CHAR_FF:
            case CHAR_NEL:
#if defined COMPILE_PCRE16 || defined COMPILE_PCRE32
            case 0x2028:
            case 0x2029:
#endif
            if (md->bsr_anycrlf) RRETURN(MATCH_NOMATCH);
            break;
            }
          }
        break;

        case OP_NOT_HSPACE:
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          switch(*eptr++)
            {
            default: break;
            HSPACE_BYTE_CASES:
#if defined COMPILE_PCRE16 || defined COMPILE_PCRE32
            HSPACE_MULTIBYTE_CASES:
#endif
            RRETURN(MATCH_NOMATCH);
            }
          }
        break;

        case OP_HSPACE:
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          switch(*eptr++)
            {
            default: RRETURN(MATCH_NOMATCH);
            HSPACE_BYTE_CASES:
#if defined COMPILE_PCRE16 || defined COMPILE_PCRE32
            HSPACE_MULTIBYTE_CASES:
#endif
            break;
            }
          }
        break;

        case OP_NOT_VSPACE:
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          switch(*eptr++)
            {
            VSPACE_BYTE_CASES:
#if defined COMPILE_PCRE16 || defined COMPILE_PCRE32
            VSPACE_MULTIBYTE_CASES:
#endif
            RRETURN(MATCH_NOMATCH);
            default: break;
            }
          }
        break;

        case OP_VSPACE:
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          switch(*eptr++)
            {
            default: RRETURN(MATCH_NOMATCH);
            VSPACE_BYTE_CASES:
#if defined COMPILE_PCRE16 || defined COMPILE_PCRE32
            VSPACE_MULTIBYTE_CASES:
#endif
            break;
            }
          }
        break;

        case OP_NOT_DIGIT:
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (MAX_255(*eptr) && (md->ctypes[*eptr] & ctype_digit) != 0)
            RRETURN(MATCH_NOMATCH);
          eptr++;
          }
        break;

        case OP_DIGIT:
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (!MAX_255(*eptr) || (md->ctypes[*eptr] & ctype_digit) == 0)
            RRETURN(MATCH_NOMATCH);
          eptr++;
          }
        break;

        case OP_NOT_WHITESPACE:
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (MAX_255(*eptr) && (md->ctypes[*eptr] & ctype_space) != 0)
            RRETURN(MATCH_NOMATCH);
          eptr++;
          }
        break;

        case OP_WHITESPACE:
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (!MAX_255(*eptr) || (md->ctypes[*eptr] & ctype_space) == 0)
            RRETURN(MATCH_NOMATCH);
          eptr++;
          }
        break;

        case OP_NOT_WORDCHAR:
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (MAX_255(*eptr) && (md->ctypes[*eptr] & ctype_word) != 0)
            RRETURN(MATCH_NOMATCH);
          eptr++;
          }
        break;

        case OP_WORDCHAR:
        for (i = 1; i <= min; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (!MAX_255(*eptr) || (md->ctypes[*eptr] & ctype_word) == 0)
            RRETURN(MATCH_NOMATCH);
          eptr++;
          }
        break;

        default:
        RRETURN(PCRE_ERROR_INTERNAL);
        }
      }

    /* If min = max, continue at the same level without recursing */

    if (min == max) continue;

    /* If minimizing, we have to test the rest of the pattern before each
    subsequent match. Again, separate the UTF-8 case for speed, and also
    separate the UCP cases. */

    if (minimize)
      {
#ifdef SUPPORT_UCP
      if (prop_type >= 0)
        {
        switch(prop_type)
          {
          case PT_ANY:
          for (fi = min;; fi++)
            {
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM36);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (fi >= max) RRETURN(MATCH_NOMATCH);
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(c, eptr);
            if (prop_fail_result) RRETURN(MATCH_NOMATCH);
            }
          /* Control never gets here */

          case PT_LAMP:
          for (fi = min;; fi++)
            {
            int chartype;
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM37);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (fi >= max) RRETURN(MATCH_NOMATCH);
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(c, eptr);
            chartype = UCD_CHARTYPE(c);
            if ((chartype == ucp_Lu ||
                 chartype == ucp_Ll ||
                 chartype == ucp_Lt) == prop_fail_result)
              RRETURN(MATCH_NOMATCH);
            }
          /* Control never gets here */

          case PT_GC:
          for (fi = min;; fi++)
            {
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM38);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (fi >= max) RRETURN(MATCH_NOMATCH);
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(c, eptr);
            if ((UCD_CATEGORY(c) == prop_value) == prop_fail_result)
              RRETURN(MATCH_NOMATCH);
            }
          /* Control never gets here */

          case PT_PC:
          for (fi = min;; fi++)
            {
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM39);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (fi >= max) RRETURN(MATCH_NOMATCH);
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(c, eptr);
            if ((UCD_CHARTYPE(c) == prop_value) == prop_fail_result)
              RRETURN(MATCH_NOMATCH);
            }
          /* Control never gets here */

          case PT_SC:
          for (fi = min;; fi++)
            {
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM40);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (fi >= max) RRETURN(MATCH_NOMATCH);
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(c, eptr);
            if ((UCD_SCRIPT(c) == prop_value) == prop_fail_result)
              RRETURN(MATCH_NOMATCH);
            }
          /* Control never gets here */

          case PT_ALNUM:
          for (fi = min;; fi++)
            {
            int category;
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM59);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (fi >= max) RRETURN(MATCH_NOMATCH);
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(c, eptr);
            category = UCD_CATEGORY(c);
            if ((category == ucp_L || category == ucp_N) == prop_fail_result)
              RRETURN(MATCH_NOMATCH);
            }
          /* Control never gets here */

          /* Perl space used to exclude VT, but from Perl 5.18 it is included,
          which means that Perl space and POSIX space are now identical. PCRE
          was changed at release 8.34. */

          case PT_SPACE:    /* Perl space */
          case PT_PXSPACE:  /* POSIX space */
          for (fi = min;; fi++)
            {
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM61);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (fi >= max) RRETURN(MATCH_NOMATCH);
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(c, eptr);
            switch(c)
              {
              HSPACE_CASES:
              VSPACE_CASES:
              if (prop_fail_result) RRETURN(MATCH_NOMATCH);
              break;

              default:
              if ((UCD_CATEGORY(c) == ucp_Z) == prop_fail_result)
                RRETURN(MATCH_NOMATCH);
              break;
              }
            }
          /* Control never gets here */

          case PT_WORD:
          for (fi = min;; fi++)
            {
            int category;
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM62);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (fi >= max) RRETURN(MATCH_NOMATCH);
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(c, eptr);
            category = UCD_CATEGORY(c);
            if ((category == ucp_L ||
                 category == ucp_N ||
                 c == CHAR_UNDERSCORE)
                   == prop_fail_result)
              RRETURN(MATCH_NOMATCH);
            }
          /* Control never gets here */

          case PT_CLIST:
          for (fi = min;; fi++)
            {
            const pcre_uint32 *cp;
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM67);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (fi >= max) RRETURN(MATCH_NOMATCH);
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(c, eptr);
            cp = PRIV(ucd_caseless_sets) + prop_value;
            for (;;)
              {
              if (c < *cp)
                { if (prop_fail_result) break; else { RRETURN(MATCH_NOMATCH); } }
              if (c == *cp++)
                { if (prop_fail_result) { RRETURN(MATCH_NOMATCH); } else break; }
              }
            }
          /* Control never gets here */

          case PT_UCNC:
          for (fi = min;; fi++)
            {
            RMATCH(eptr, ecode, offset_top, md, eptrb, RM60);
            if (rrc != MATCH_NOMATCH) RRETURN(rrc);
            if (fi >= max) RRETURN(MATCH_NOMATCH);
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              RRETURN(MATCH_NOMATCH);
              }
            GETCHARINCTEST(c, eptr);
            if ((c == CHAR_DOLLAR_SIGN || c == CHAR_COMMERCIAL_AT ||
                 c == CHAR_GRAVE_ACCENT || (c >= 0xa0 && c <= 0xd7ff) ||
                 c >= 0xe000) == prop_fail_result)
              RRETURN(MATCH_NOMATCH);
            }
          /* Control never gets here */

          /* This should never occur */
          default:
          RRETURN(PCRE_ERROR_INTERNAL);
          }
        }

      /* Match extended Unicode sequences. We will get here only if the
      support is in the binary; otherwise a compile-time error occurs. */

      else if (ctype == OP_EXTUNI)
        {
        for (fi = min;; fi++)
          {
          RMATCH(eptr, ecode, offset_top, md, eptrb, RM41);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          if (fi >= max) RRETURN(MATCH_NOMATCH);
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          else
            {
            int lgb, rgb;
            GETCHARINCTEST(c, eptr);
            lgb = UCD_GRAPHBREAK(c);
            while (eptr < md->end_subject)
              {
              int len = 1;
              if (!utf) c = *eptr; else { GETCHARLEN(c, eptr, len); }
              rgb = UCD_GRAPHBREAK(c);
              if ((PRIV(ucp_gbtable)[lgb] & (1 << rgb)) == 0) break;
              lgb = rgb;
              eptr += len;
              }
            }
          CHECK_PARTIAL();
          }
        }
      else
#endif     /* SUPPORT_UCP */

#ifdef SUPPORT_UTF
      if (utf)
        {
        for (fi = min;; fi++)
          {
          RMATCH(eptr, ecode, offset_top, md, eptrb, RM42);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          if (fi >= max) RRETURN(MATCH_NOMATCH);
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (ctype == OP_ANY && IS_NEWLINE(eptr))
            RRETURN(MATCH_NOMATCH);
          GETCHARINC(c, eptr);
          switch(ctype)
            {
            case OP_ANY:               /* This is the non-NL case */
            if (md->partial != 0 &&    /* Take care with CRLF partial */
                eptr >= md->end_subject &&
                NLBLOCK->nltype == NLTYPE_FIXED &&
                NLBLOCK->nllen == 2 &&
                c == NLBLOCK->nl[0])
              {
              md->hitend = TRUE;
              if (md->partial > 1) RRETURN(PCRE_ERROR_PARTIAL);
              }
            break;

            case OP_ALLANY:
            case OP_ANYBYTE:
            break;

            case OP_ANYNL:
            switch(c)
              {
              default: RRETURN(MATCH_NOMATCH);
              case CHAR_CR:
              if (eptr < md->end_subject && UCHAR21(eptr) == CHAR_LF) eptr++;
              break;

              case CHAR_LF:
              break;

              case CHAR_VT:
              case CHAR_FF:
              case CHAR_NEL:
#ifndef EBCDIC
              case 0x2028:
              case 0x2029:
#endif  /* Not EBCDIC */
              if (md->bsr_anycrlf) RRETURN(MATCH_NOMATCH);
              break;
              }
            break;

            case OP_NOT_HSPACE:
            switch(c)
              {
              HSPACE_CASES: RRETURN(MATCH_NOMATCH);
              default: break;
              }
            break;

            case OP_HSPACE:
            switch(c)
              {
              HSPACE_CASES: break;
              default: RRETURN(MATCH_NOMATCH);
              }
            break;

            case OP_NOT_VSPACE:
            switch(c)
              {
              VSPACE_CASES: RRETURN(MATCH_NOMATCH);
              default: break;
              }
            break;

            case OP_VSPACE:
            switch(c)
              {
              VSPACE_CASES: break;
              default: RRETURN(MATCH_NOMATCH);
              }
            break;

            case OP_NOT_DIGIT:
            if (c < 256 && (md->ctypes[c] & ctype_digit) != 0)
              RRETURN(MATCH_NOMATCH);
            break;

            case OP_DIGIT:
            if (c >= 256 || (md->ctypes[c] & ctype_digit) == 0)
              RRETURN(MATCH_NOMATCH);
            break;

            case OP_NOT_WHITESPACE:
            if (c < 256 && (md->ctypes[c] & ctype_space) != 0)
              RRETURN(MATCH_NOMATCH);
            break;

            case OP_WHITESPACE:
            if (c >= 256 || (md->ctypes[c] & ctype_space) == 0)
              RRETURN(MATCH_NOMATCH);
            break;

            case OP_NOT_WORDCHAR:
            if (c < 256 && (md->ctypes[c] & ctype_word) != 0)
              RRETURN(MATCH_NOMATCH);
            break;

            case OP_WORDCHAR:
            if (c >= 256 || (md->ctypes[c] & ctype_word) == 0)
              RRETURN(MATCH_NOMATCH);
            break;

            default:
            RRETURN(PCRE_ERROR_INTERNAL);
            }
          }
        }
      else
#endif
      /* Not UTF mode */
        {
        for (fi = min;; fi++)
          {
          RMATCH(eptr, ecode, offset_top, md, eptrb, RM43);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          if (fi >= max) RRETURN(MATCH_NOMATCH);
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            RRETURN(MATCH_NOMATCH);
            }
          if (ctype == OP_ANY && IS_NEWLINE(eptr))
            RRETURN(MATCH_NOMATCH);
          c = *eptr++;
          switch(ctype)
            {
            case OP_ANY:               /* This is the non-NL case */
            if (md->partial != 0 &&    /* Take care with CRLF partial */
                eptr >= md->end_subject &&
                NLBLOCK->nltype == NLTYPE_FIXED &&
                NLBLOCK->nllen == 2 &&
                c == NLBLOCK->nl[0])
              {
              md->hitend = TRUE;
              if (md->partial > 1) RRETURN(PCRE_ERROR_PARTIAL);
              }
            break;

            case OP_ALLANY:
            case OP_ANYBYTE:
            break;

            case OP_ANYNL:
            switch(c)
              {
              default: RRETURN(MATCH_NOMATCH);
              case CHAR_CR:
              if (eptr < md->end_subject && *eptr == CHAR_LF) eptr++;
              break;

              case CHAR_LF:
              break;

              case CHAR_VT:
              case CHAR_FF:
              case CHAR_NEL:
#if defined COMPILE_PCRE16 || defined COMPILE_PCRE32
              case 0x2028:
              case 0x2029:
#endif
              if (md->bsr_anycrlf) RRETURN(MATCH_NOMATCH);
              break;
              }
            break;

            case OP_NOT_HSPACE:
            switch(c)
              {
              default: break;
              HSPACE_BYTE_CASES:
#if defined COMPILE_PCRE16 || defined COMPILE_PCRE32
              HSPACE_MULTIBYTE_CASES:
#endif
              RRETURN(MATCH_NOMATCH);
              }
            break;

            case OP_HSPACE:
            switch(c)
              {
              default: RRETURN(MATCH_NOMATCH);
              HSPACE_BYTE_CASES:
#if defined COMPILE_PCRE16 || defined COMPILE_PCRE32
              HSPACE_MULTIBYTE_CASES:
#endif
              break;
              }
            break;

            case OP_NOT_VSPACE:
            switch(c)
              {
              default: break;
              VSPACE_BYTE_CASES:
#if defined COMPILE_PCRE16 || defined COMPILE_PCRE32
              VSPACE_MULTIBYTE_CASES:
#endif
              RRETURN(MATCH_NOMATCH);
              }
            break;

            case OP_VSPACE:
            switch(c)
              {
              default: RRETURN(MATCH_NOMATCH);
              VSPACE_BYTE_CASES:
#if defined COMPILE_PCRE16 || defined COMPILE_PCRE32
              VSPACE_MULTIBYTE_CASES:
#endif
              break;
              }
            break;

            case OP_NOT_DIGIT:
            if (MAX_255(c) && (md->ctypes[c] & ctype_digit) != 0) RRETURN(MATCH_NOMATCH);
            break;

            case OP_DIGIT:
            if (!MAX_255(c) || (md->ctypes[c] & ctype_digit) == 0) RRETURN(MATCH_NOMATCH);
            break;

            case OP_NOT_WHITESPACE:
            if (MAX_255(c) && (md->ctypes[c] & ctype_space) != 0) RRETURN(MATCH_NOMATCH);
            break;

            case OP_WHITESPACE:
            if (!MAX_255(c) || (md->ctypes[c] & ctype_space) == 0) RRETURN(MATCH_NOMATCH);
            break;

            case OP_NOT_WORDCHAR:
            if (MAX_255(c) && (md->ctypes[c] & ctype_word) != 0) RRETURN(MATCH_NOMATCH);
            break;

            case OP_WORDCHAR:
            if (!MAX_255(c) || (md->ctypes[c] & ctype_word) == 0) RRETURN(MATCH_NOMATCH);
            break;

            default:
            RRETURN(PCRE_ERROR_INTERNAL);
            }
          }
        }
      /* Control never gets here */
      }

    /* If maximizing, it is worth using inline code for speed, doing the type
    test once at the start (i.e. keep it out of the loop). Again, keep the
    UTF-8 and UCP stuff separate. */

    else
      {
      pp = eptr;  /* Remember where we started */

#ifdef SUPPORT_UCP
      if (prop_type >= 0)
        {
        switch(prop_type)
          {
          case PT_ANY:
          for (i = min; i < max; i++)
            {
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLENTEST(c, eptr, len);
            if (prop_fail_result) break;
            eptr+= len;
            }
          break;

          case PT_LAMP:
          for (i = min; i < max; i++)
            {
            int chartype;
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLENTEST(c, eptr, len);
            chartype = UCD_CHARTYPE(c);
            if ((chartype == ucp_Lu ||
                 chartype == ucp_Ll ||
                 chartype == ucp_Lt) == prop_fail_result)
              break;
            eptr+= len;
            }
          break;

          case PT_GC:
          for (i = min; i < max; i++)
            {
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLENTEST(c, eptr, len);
            if ((UCD_CATEGORY(c) == prop_value) == prop_fail_result) break;
            eptr+= len;
            }
          break;

          case PT_PC:
          for (i = min; i < max; i++)
            {
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLENTEST(c, eptr, len);
            if ((UCD_CHARTYPE(c) == prop_value) == prop_fail_result) break;
            eptr+= len;
            }
          break;

          case PT_SC:
          for (i = min; i < max; i++)
            {
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLENTEST(c, eptr, len);
            if ((UCD_SCRIPT(c) == prop_value) == prop_fail_result) break;
            eptr+= len;
            }
          break;

          case PT_ALNUM:
          for (i = min; i < max; i++)
            {
            int category;
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLENTEST(c, eptr, len);
            category = UCD_CATEGORY(c);
            if ((category == ucp_L || category == ucp_N) == prop_fail_result)
              break;
            eptr+= len;
            }
          break;

          /* Perl space used to exclude VT, but from Perl 5.18 it is included,
          which means that Perl space and POSIX space are now identical. PCRE
          was changed at release 8.34. */

          case PT_SPACE:    /* Perl space */
          case PT_PXSPACE:  /* POSIX space */
          for (i = min; i < max; i++)
            {
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLENTEST(c, eptr, len);
            switch(c)
              {
              HSPACE_CASES:
              VSPACE_CASES:
              if (prop_fail_result) goto ENDLOOP99;  /* Break the loop */
              break;

              default:
              if ((UCD_CATEGORY(c) == ucp_Z) == prop_fail_result)
                goto ENDLOOP99;   /* Break the loop */
              break;
              }
            eptr+= len;
            }
          ENDLOOP99:
          break;

          case PT_WORD:
          for (i = min; i < max; i++)
            {
            int category;
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLENTEST(c, eptr, len);
            category = UCD_CATEGORY(c);
            if ((category == ucp_L || category == ucp_N ||
                 c == CHAR_UNDERSCORE) == prop_fail_result)
              break;
            eptr+= len;
            }
          break;

          case PT_CLIST:
          for (i = min; i < max; i++)
            {
            const pcre_uint32 *cp;
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLENTEST(c, eptr, len);
            cp = PRIV(ucd_caseless_sets) + prop_value;
            for (;;)
              {
              if (c < *cp)
                { if (prop_fail_result) break; else goto GOT_MAX; }
              if (c == *cp++)
                { if (prop_fail_result) goto GOT_MAX; else break; }
              }
            eptr += len;
            }
          GOT_MAX:
          break;

          case PT_UCNC:
          for (i = min; i < max; i++)
            {
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLENTEST(c, eptr, len);
            if ((c == CHAR_DOLLAR_SIGN || c == CHAR_COMMERCIAL_AT ||
                 c == CHAR_GRAVE_ACCENT || (c >= 0xa0 && c <= 0xd7ff) ||
                 c >= 0xe000) == prop_fail_result)
              break;
            eptr += len;
            }
          break;

          default:
          RRETURN(PCRE_ERROR_INTERNAL);
          }

        /* eptr is now past the end of the maximum run */

        if (possessive) continue;    /* No backtracking */
        for(;;)
          {
          if (eptr <= pp) goto TAIL_RECURSE;
          RMATCH(eptr, ecode, offset_top, md, eptrb, RM44);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          eptr--;
          if (utf) BACKCHAR(eptr);
          }
        }

      /* Match extended Unicode grapheme clusters. We will get here only if the
      support is in the binary; otherwise a compile-time error occurs. */

      else if (ctype == OP_EXTUNI)
        {
        for (i = min; i < max; i++)
          {
          if (eptr >= md->end_subject)
            {
            SCHECK_PARTIAL();
            break;
            }
          else
            {
            int lgb, rgb;
            GETCHARINCTEST(c, eptr);
            lgb = UCD_GRAPHBREAK(c);
            while (eptr < md->end_subject)
              {
              int len = 1;
              if (!utf) c = *eptr; else { GETCHARLEN(c, eptr, len); }
              rgb = UCD_GRAPHBREAK(c);
              if ((PRIV(ucp_gbtable)[lgb] & (1 << rgb)) == 0) break;
              lgb = rgb;
              eptr += len;
              }
            }
          CHECK_PARTIAL();
          }

        /* eptr is now past the end of the maximum run */

        if (possessive) continue;    /* No backtracking */

        /* We use <= pp rather than == pp to detect the start of the run while
        backtracking because the use of \C in UTF mode can cause BACKCHAR to
        move back past pp. This is just palliative; the use of \C in UTF mode
        is fraught with danger. */

        for(;;)
          {
          int lgb, rgb;
          PCRE_PUCHAR fptr;

          if (eptr <= pp) goto TAIL_RECURSE;   /* At start of char run */
          RMATCH(eptr, ecode, offset_top, md, eptrb, RM45);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);

          /* Backtracking over an extended grapheme cluster involves inspecting
          the previous two characters (if present) to see if a break is
          permitted between them. */

          eptr--;
          if (!utf) c = *eptr; else
            {
            BACKCHAR(eptr);
            GETCHAR(c, eptr);
            }
          rgb = UCD_GRAPHBREAK(c);

          for (;;)
            {
            if (eptr <= pp) goto TAIL_RECURSE;   /* At start of char run */
            fptr = eptr - 1;
            if (!utf) c = *fptr; else
              {
              BACKCHAR(fptr);
              GETCHAR(c, fptr);
              }
            lgb = UCD_GRAPHBREAK(c);
            if ((PRIV(ucp_gbtable)[lgb] & (1 << rgb)) == 0) break;
            eptr = fptr;
            rgb = lgb;
            }
          }
        }

      else
#endif   /* SUPPORT_UCP */

#ifdef SUPPORT_UTF
      if (utf)
        {
        switch(ctype)
          {
          case OP_ANY:
          for (i = min; i < max; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            if (IS_NEWLINE(eptr)) break;
            if (md->partial != 0 &&    /* Take care with CRLF partial */
                eptr + 1 >= md->end_subject &&
                NLBLOCK->nltype == NLTYPE_FIXED &&
                NLBLOCK->nllen == 2 &&
                UCHAR21(eptr) == NLBLOCK->nl[0])
              {
              md->hitend = TRUE;
              if (md->partial > 1) RRETURN(PCRE_ERROR_PARTIAL);
              }
            eptr++;
            ACROSSCHAR(eptr < md->end_subject, *eptr, eptr++);
            }
          break;

          case OP_ALLANY:
          if (max < INT_MAX)
            {
            for (i = min; i < max; i++)
              {
              if (eptr >= md->end_subject)
                {
                SCHECK_PARTIAL();
                break;
                }
              eptr++;
              ACROSSCHAR(eptr < md->end_subject, *eptr, eptr++);
              }
            }
          else
            {
            eptr = md->end_subject;   /* Unlimited UTF-8 repeat */
            SCHECK_PARTIAL();
            }
          break;

          /* The byte case is the same as non-UTF8 */

          case OP_ANYBYTE:
          c = max - min;
          if (c > (unsigned int)(md->end_subject - eptr))
            {
            eptr = md->end_subject;
            SCHECK_PARTIAL();
            }
          else eptr += c;
          break;

          case OP_ANYNL:
          for (i = min; i < max; i++)
            {
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(c, eptr, len);
            if (c == CHAR_CR)
              {
              if (++eptr >= md->end_subject) break;
              if (UCHAR21(eptr) == CHAR_LF) eptr++;
              }
            else
              {
              if (c != CHAR_LF &&
                  (md->bsr_anycrlf ||
                   (c != CHAR_VT && c != CHAR_FF && c != CHAR_NEL
#ifndef EBCDIC
                    && c != 0x2028 && c != 0x2029
#endif  /* Not EBCDIC */
                    )))
                break;
              eptr += len;
              }
            }
          break;

          case OP_NOT_HSPACE:
          case OP_HSPACE:
          for (i = min; i < max; i++)
            {
            BOOL gotspace;
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(c, eptr, len);
            switch(c)
              {
              HSPACE_CASES: gotspace = TRUE; break;
              default: gotspace = FALSE; break;
              }
            if (gotspace == (ctype == OP_NOT_HSPACE)) break;
            eptr += len;
            }
          break;

          case OP_NOT_VSPACE:
          case OP_VSPACE:
          for (i = min; i < max; i++)
            {
            BOOL gotspace;
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(c, eptr, len);
            switch(c)
              {
              VSPACE_CASES: gotspace = TRUE; break;
              default: gotspace = FALSE; break;
              }
            if (gotspace == (ctype == OP_NOT_VSPACE)) break;
            eptr += len;
            }
          break;

          case OP_NOT_DIGIT:
          for (i = min; i < max; i++)
            {
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(c, eptr, len);
            if (c < 256 && (md->ctypes[c] & ctype_digit) != 0) break;
            eptr+= len;
            }
          break;

          case OP_DIGIT:
          for (i = min; i < max; i++)
            {
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(c, eptr, len);
            if (c >= 256 ||(md->ctypes[c] & ctype_digit) == 0) break;
            eptr+= len;
            }
          break;

          case OP_NOT_WHITESPACE:
          for (i = min; i < max; i++)
            {
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(c, eptr, len);
            if (c < 256 && (md->ctypes[c] & ctype_space) != 0) break;
            eptr+= len;
            }
          break;

          case OP_WHITESPACE:
          for (i = min; i < max; i++)
            {
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(c, eptr, len);
            if (c >= 256 ||(md->ctypes[c] & ctype_space) == 0) break;
            eptr+= len;
            }
          break;

          case OP_NOT_WORDCHAR:
          for (i = min; i < max; i++)
            {
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(c, eptr, len);
            if (c < 256 && (md->ctypes[c] & ctype_word) != 0) break;
            eptr+= len;
            }
          break;

          case OP_WORDCHAR:
          for (i = min; i < max; i++)
            {
            int len = 1;
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            GETCHARLEN(c, eptr, len);
            if (c >= 256 || (md->ctypes[c] & ctype_word) == 0) break;
            eptr+= len;
            }
          break;

          default:
          RRETURN(PCRE_ERROR_INTERNAL);
          }

        if (possessive) continue;    /* No backtracking */
        for(;;)
          {
          if (eptr <= pp) goto TAIL_RECURSE;
          RMATCH(eptr, ecode, offset_top, md, eptrb, RM46);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          eptr--;
          BACKCHAR(eptr);
          if (ctype == OP_ANYNL && eptr > pp  && UCHAR21(eptr) == CHAR_NL &&
              UCHAR21(eptr - 1) == CHAR_CR) eptr--;
          }
        }
      else
#endif  /* SUPPORT_UTF */
      /* Not UTF mode */
        {
        switch(ctype)
          {
          case OP_ANY:
          for (i = min; i < max; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            if (IS_NEWLINE(eptr)) break;
            if (md->partial != 0 &&    /* Take care with CRLF partial */
                eptr + 1 >= md->end_subject &&
                NLBLOCK->nltype == NLTYPE_FIXED &&
                NLBLOCK->nllen == 2 &&
                *eptr == NLBLOCK->nl[0])
              {
              md->hitend = TRUE;
              if (md->partial > 1) RRETURN(PCRE_ERROR_PARTIAL);
              }
            eptr++;
            }
          break;

          case OP_ALLANY:
          case OP_ANYBYTE:
          c = max - min;
          if (c > (unsigned int)(md->end_subject - eptr))
            {
            eptr = md->end_subject;
            SCHECK_PARTIAL();
            }
          else eptr += c;
          break;

          case OP_ANYNL:
          for (i = min; i < max; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            c = *eptr;
            if (c == CHAR_CR)
              {
              if (++eptr >= md->end_subject) break;
              if (*eptr == CHAR_LF) eptr++;
              }
            else
              {
              if (c != CHAR_LF && (md->bsr_anycrlf ||
                 (c != CHAR_VT && c != CHAR_FF && c != CHAR_NEL
#if defined COMPILE_PCRE16 || defined COMPILE_PCRE32
                 && c != 0x2028 && c != 0x2029
#endif
                 ))) break;
              eptr++;
              }
            }
          break;

          case OP_NOT_HSPACE:
          for (i = min; i < max; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            switch(*eptr)
              {
              default: eptr++; break;
              HSPACE_BYTE_CASES:
#if defined COMPILE_PCRE16 || defined COMPILE_PCRE32
              HSPACE_MULTIBYTE_CASES:
#endif
              goto ENDLOOP00;
              }
            }
          ENDLOOP00:
          break;

          case OP_HSPACE:
          for (i = min; i < max; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            switch(*eptr)
              {
              default: goto ENDLOOP01;
              HSPACE_BYTE_CASES:
#if defined COMPILE_PCRE16 || defined COMPILE_PCRE32
              HSPACE_MULTIBYTE_CASES:
#endif
              eptr++; break;
              }
            }
          ENDLOOP01:
          break;

          case OP_NOT_VSPACE:
          for (i = min; i < max; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            switch(*eptr)
              {
              default: eptr++; break;
              VSPACE_BYTE_CASES:
#if defined COMPILE_PCRE16 || defined COMPILE_PCRE32
              VSPACE_MULTIBYTE_CASES:
#endif
              goto ENDLOOP02;
              }
            }
          ENDLOOP02:
          break;

          case OP_VSPACE:
          for (i = min; i < max; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            switch(*eptr)
              {
              default: goto ENDLOOP03;
              VSPACE_BYTE_CASES:
#if defined COMPILE_PCRE16 || defined COMPILE_PCRE32
              VSPACE_MULTIBYTE_CASES:
#endif
              eptr++; break;
              }
            }
          ENDLOOP03:
          break;

          case OP_NOT_DIGIT:
          for (i = min; i < max; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            if (MAX_255(*eptr) && (md->ctypes[*eptr] & ctype_digit) != 0) break;
            eptr++;
            }
          break;

          case OP_DIGIT:
          for (i = min; i < max; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            if (!MAX_255(*eptr) || (md->ctypes[*eptr] & ctype_digit) == 0) break;
            eptr++;
            }
          break;

          case OP_NOT_WHITESPACE:
          for (i = min; i < max; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            if (MAX_255(*eptr) && (md->ctypes[*eptr] & ctype_space) != 0) break;
            eptr++;
            }
          break;

          case OP_WHITESPACE:
          for (i = min; i < max; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            if (!MAX_255(*eptr) || (md->ctypes[*eptr] & ctype_space) == 0) break;
            eptr++;
            }
          break;

          case OP_NOT_WORDCHAR:
          for (i = min; i < max; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            if (MAX_255(*eptr) && (md->ctypes[*eptr] & ctype_word) != 0) break;
            eptr++;
            }
          break;

          case OP_WORDCHAR:
          for (i = min; i < max; i++)
            {
            if (eptr >= md->end_subject)
              {
              SCHECK_PARTIAL();
              break;
              }
            if (!MAX_255(*eptr) || (md->ctypes[*eptr] & ctype_word) == 0) break;
            eptr++;
            }
          break;

          default:
          RRETURN(PCRE_ERROR_INTERNAL);
          }

        if (possessive) continue;    /* No backtracking */
        for (;;)
          {
          if (eptr == pp) goto TAIL_RECURSE;
          RMATCH(eptr, ecode, offset_top, md, eptrb, RM47);
          if (rrc != MATCH_NOMATCH) RRETURN(rrc);
          eptr--;
          if (ctype == OP_ANYNL && eptr > pp  && *eptr == CHAR_LF &&
              eptr[-1] == CHAR_CR) eptr--;
          }
        }

      /* Control never gets here */
      }

    /* There's been some horrible disaster. Arrival here can only mean there is
    something seriously wrong in the code above or the OP_xxx definitions. */

    default:
    DPRINTF(("Unknown opcode %d\n", *ecode));
    RRETURN(PCRE_ERROR_UNKNOWN_OPCODE);
    }

  /* Do not stick any code in here without much thought; it is assumed
  that "continue" in the code above comes out to here to repeat the main
  loop. */

  }             /* End of main loop */
/* Control never reaches here */


/* When compiling to use the heap rather than the stack for recursive calls to
match(), the RRETURN() macro jumps here. The number that is saved in
frame->Xwhere indicates which label we actually want to return to. */

#ifdef NO_RECURSE
#define LBL(val) case val: goto L_RM##val;
HEAP_RETURN:
switch (frame->Xwhere)
  {
  LBL( 1) LBL( 2) LBL( 3) LBL( 4) LBL( 5) LBL( 6) LBL( 7) LBL( 8)
  LBL( 9) LBL(10) LBL(11) LBL(12) LBL(13) LBL(14) LBL(15) LBL(17)
  LBL(19) LBL(24) LBL(25) LBL(26) LBL(27) LBL(29) LBL(31) LBL(33)
  LBL(35) LBL(43) LBL(47) LBL(48) LBL(49) LBL(50) LBL(51) LBL(52)
  LBL(53) LBL(54) LBL(55) LBL(56) LBL(57) LBL(58) LBL(63) LBL(64)
  LBL(65) LBL(66)
#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
  LBL(20) LBL(21)
#endif
#ifdef SUPPORT_UTF
  LBL(16) LBL(18)
  LBL(22) LBL(23) LBL(28) LBL(30)
  LBL(32) LBL(34) LBL(42) LBL(46)
#ifdef SUPPORT_UCP
  LBL(36) LBL(37) LBL(38) LBL(39) LBL(40) LBL(41) LBL(44) LBL(45)
  LBL(59) LBL(60) LBL(61) LBL(62) LBL(67)
#endif  /* SUPPORT_UCP */
#endif  /* SUPPORT_UTF */
  default:
  DPRINTF(("jump error in pcre match: label %d non-existent\n", frame->Xwhere));
  return PCRE_ERROR_INTERNAL;
  }
#undef LBL
#endif  /* NO_RECURSE */
}


/***************************************************************************
****************************************************************************
                   RECURSION IN THE match() FUNCTION

Undefine all the macros that were defined above to handle this. */

#ifdef NO_RECURSE
#undef eptr
#undef ecode
#undef mstart
#undef offset_top
#undef eptrb
#undef flags

#undef callpat
#undef charptr
#undef data
#undef next
#undef pp
#undef prev
#undef saved_eptr

#undef new_recursive

#undef cur_is_word
#undef condition
#undef prev_is_word

#undef ctype
#undef length
#undef max
#undef min
#undef number
#undef offset
#undef op
#undef save_capture_last
#undef save_offset1
#undef save_offset2
#undef save_offset3
#undef stacksave

#undef newptrb

#endif

/* These two are defined as macros in both cases */

#undef fc
#undef fi

/***************************************************************************
***************************************************************************/


#ifdef NO_RECURSE
/*************************************************
*          Release allocated heap frames         *
*************************************************/

/* This function releases all the allocated frames. The base frame is on the
machine stack, and so must not be freed.

Argument: the address of the base frame
Returns:  nothing
*/

static void
release_match_heapframes (heapframe *frame_base)
{
heapframe *nextframe = frame_base->Xnextframe;
while (nextframe != NULL)
  {
  heapframe *oldframe = nextframe;
  nextframe = nextframe->Xnextframe;
  (PUBL(stack_free))(oldframe);
  }
}
#endif


/*************************************************
*         Execute a Regular Expression           *
*************************************************/

/* This function applies a compiled re to a subject string and picks out
portions of the string if it matches. Two elements in the vector are set for
each substring: the offsets to the start and end of the substring.

Arguments:
  argument_re     points to the compiled expression
  extra_data      points to extra data or is NULL
  subject         points to the subject string
  length          length of subject string (may contain binary zeros)
  start_offset    where to start in the subject string
  options         option bits
  offsets         points to a vector of ints to be filled in with offsets
  offsetcount     the number of elements in the vector

Returns:          > 0 => success; value is the number of elements filled in
                  = 0 => success, but offsets is not big enough
                   -1 => failed to match
                 < -1 => some kind of unexpected problem
*/

#if defined COMPILE_PCRE8
PCRE_EXP_DEFN int PCRE_CALL_CONVENTION
pcre_exec(const pcre *argument_re, const pcre_extra *extra_data,
  PCRE_SPTR subject, int length, int start_offset, int options, int *offsets,
  int offsetcount)
#elif defined COMPILE_PCRE16
PCRE_EXP_DEFN int PCRE_CALL_CONVENTION
pcre16_exec(const pcre16 *argument_re, const pcre16_extra *extra_data,
  PCRE_SPTR16 subject, int length, int start_offset, int options, int *offsets,
  int offsetcount)
#elif defined COMPILE_PCRE32
PCRE_EXP_DEFN int PCRE_CALL_CONVENTION
pcre32_exec(const pcre32 *argument_re, const pcre32_extra *extra_data,
  PCRE_SPTR32 subject, int length, int start_offset, int options, int *offsets,
  int offsetcount)
#endif
{
int rc, ocount, arg_offset_max;
int newline;
BOOL using_temporary_offsets = FALSE;
BOOL anchored;
BOOL startline;
BOOL firstline;
BOOL utf;
BOOL has_first_char = FALSE;
BOOL has_req_char = FALSE;
pcre_uchar first_char = 0;
pcre_uchar first_char2 = 0;
pcre_uchar req_char = 0;
pcre_uchar req_char2 = 0;
match_data match_block;
match_data *md = &match_block;
const pcre_uint8 *tables;
const pcre_uint8 *start_bits = NULL;
PCRE_PUCHAR start_match = (PCRE_PUCHAR)subject + start_offset;
PCRE_PUCHAR end_subject;
PCRE_PUCHAR start_partial = NULL;
PCRE_PUCHAR match_partial = NULL;
PCRE_PUCHAR req_char_ptr = start_match - 1;

const pcre_study_data *study;
const REAL_PCRE *re = (const REAL_PCRE *)argument_re;

#ifdef NO_RECURSE
heapframe frame_zero;
frame_zero.Xprevframe = NULL;            /* Marks the top level */
frame_zero.Xnextframe = NULL;            /* None are allocated yet */
md->match_frames_base = &frame_zero;
#endif

/* Check for the special magic call that measures the size of the stack used
per recursive call of match(). Without the funny casting for sizeof, a Windows
compiler gave this error: "unary minus operator applied to unsigned type,
result still unsigned". Hopefully the cast fixes that. */

if (re == NULL && extra_data == NULL && subject == NULL && length == -999 &&
    start_offset == -999)
#ifdef NO_RECURSE
  return -((int)sizeof(heapframe));
#else
  return match(NULL, NULL, NULL, 0, NULL, NULL, 0);
#endif

/* Plausibility checks */

if ((options & ~PUBLIC_EXEC_OPTIONS) != 0) return PCRE_ERROR_BADOPTION;
if (re == NULL || subject == NULL || (offsets == NULL && offsetcount > 0))
  return PCRE_ERROR_NULL;
if (offsetcount < 0) return PCRE_ERROR_BADCOUNT;
if (length < 0) return PCRE_ERROR_BADLENGTH;
if (start_offset < 0 || start_offset > length) return PCRE_ERROR_BADOFFSET;

/* Check that the first field in the block is the magic number. If it is not,
return with PCRE_ERROR_BADMAGIC. However, if the magic number is equal to
REVERSED_MAGIC_NUMBER we return with PCRE_ERROR_BADENDIANNESS, which
means that the pattern is likely compiled with different endianness. */

if (re->magic_number != MAGIC_NUMBER)
  return re->magic_number == REVERSED_MAGIC_NUMBER?
    PCRE_ERROR_BADENDIANNESS:PCRE_ERROR_BADMAGIC;
if ((re->flags & PCRE_MODE) == 0) return PCRE_ERROR_BADMODE;

/* These two settings are used in the code for checking a UTF-8 string that
follows immediately afterwards. Other values in the md block are used only
during "normal" pcre_exec() processing, not when the JIT support is in use,
so they are set up later. */

/* PCRE_UTF16 has the same value as PCRE_UTF8. */
utf = md->utf = (re->options & PCRE_UTF8) != 0;
md->partial = ((options & PCRE_PARTIAL_HARD) != 0)? 2 :
              ((options & PCRE_PARTIAL_SOFT) != 0)? 1 : 0;

/* Check a UTF-8 string if required. Pass back the character offset and error
code for an invalid string if a results vector is available. */

#ifdef SUPPORT_UTF
if (utf && (options & PCRE_NO_UTF8_CHECK) == 0)
  {
  int erroroffset;
  int errorcode = PRIV(valid_utf)((PCRE_PUCHAR)subject, length, &erroroffset);
  if (errorcode != 0)
    {
    if (offsetcount >= 2)
      {
      offsets[0] = erroroffset;
      offsets[1] = errorcode;
      }
#if defined COMPILE_PCRE8
    return (errorcode <= PCRE_UTF8_ERR5 && md->partial > 1)?
      PCRE_ERROR_SHORTUTF8 : PCRE_ERROR_BADUTF8;
#elif defined COMPILE_PCRE16
    return (errorcode <= PCRE_UTF16_ERR1 && md->partial > 1)?
      PCRE_ERROR_SHORTUTF16 : PCRE_ERROR_BADUTF16;
#elif defined COMPILE_PCRE32
    return PCRE_ERROR_BADUTF32;
#endif
    }
#if defined COMPILE_PCRE8 || defined COMPILE_PCRE16
  /* Check that a start_offset points to the start of a UTF character. */
  if (start_offset > 0 && start_offset < length &&
      NOT_FIRSTCHAR(((PCRE_PUCHAR)subject)[start_offset]))
    return PCRE_ERROR_BADUTF8_OFFSET;
#endif
  }
#endif

/* If the pattern was successfully studied with JIT support, run the JIT
executable instead of the rest of this function. Most options must be set at
compile time for the JIT code to be usable. Fallback to the normal code path if
an unsupported flag is set. */

#ifdef SUPPORT_JIT
if (extra_data != NULL
    && (extra_data->flags & (PCRE_EXTRA_EXECUTABLE_JIT |
                             PCRE_EXTRA_TABLES)) == PCRE_EXTRA_EXECUTABLE_JIT
    && extra_data->executable_jit != NULL
    && (options & ~PUBLIC_JIT_EXEC_OPTIONS) == 0)
  {
  rc = PRIV(jit_exec)(extra_data, (const pcre_uchar *)subject, length,
       start_offset, options, offsets, offsetcount);

  /* PCRE_ERROR_NULL means that the selected normal or partial matching
  mode is not compiled. In this case we simply fallback to interpreter. */

  if (rc != PCRE_ERROR_JIT_BADOPTION) return rc;
  }
#endif

/* Carry on with non-JIT matching. This information is for finding all the
numbers associated with a given name, for condition testing. */

md->name_table = (pcre_uchar *)re + re->name_table_offset;
md->name_count = re->name_count;
md->name_entry_size = re->name_entry_size;

/* Fish out the optional data from the extra_data structure, first setting
the default values. */

study = NULL;
md->match_limit = MATCH_LIMIT;
md->match_limit_recursion = MATCH_LIMIT_RECURSION;
md->callout_data = NULL;

/* The table pointer is always in native byte order. */

tables = re->tables;

/* The two limit values override the defaults, whatever their value. */

if (extra_data != NULL)
  {
  unsigned long int flags = extra_data->flags;
  if ((flags & PCRE_EXTRA_STUDY_DATA) != 0)
    study = (const pcre_study_data *)extra_data->study_data;
  if ((flags & PCRE_EXTRA_MATCH_LIMIT) != 0)
    md->match_limit = extra_data->match_limit;
  if ((flags & PCRE_EXTRA_MATCH_LIMIT_RECURSION) != 0)
    md->match_limit_recursion = extra_data->match_limit_recursion;
  if ((flags & PCRE_EXTRA_CALLOUT_DATA) != 0)
    md->callout_data = extra_data->callout_data;
  if ((flags & PCRE_EXTRA_TABLES) != 0) tables = extra_data->tables;
  }

/* Limits in the regex override only if they are smaller. */

if ((re->flags & PCRE_MLSET) != 0 && re->limit_match < md->match_limit)
  md->match_limit = re->limit_match;

if ((re->flags & PCRE_RLSET) != 0 &&
    re->limit_recursion < md->match_limit_recursion)
  md->match_limit_recursion = re->limit_recursion;

/* If the exec call supplied NULL for tables, use the inbuilt ones. This
is a feature that makes it possible to save compiled regex and re-use them
in other programs later. */

if (tables == NULL) tables = PRIV(default_tables);

/* Set up other data */

anchored = ((re->options | options) & PCRE_ANCHORED) != 0;
startline = (re->flags & PCRE_STARTLINE) != 0;
firstline = (re->options & PCRE_FIRSTLINE) != 0;

/* The code starts after the real_pcre block and the capture name table. */

md->start_code = (const pcre_uchar *)re + re->name_table_offset +
  re->name_count * re->name_entry_size;

md->start_subject = (PCRE_PUCHAR)subject;
md->start_offset = start_offset;
md->end_subject = md->start_subject + length;
end_subject = md->end_subject;

md->endonly = (re->options & PCRE_DOLLAR_ENDONLY) != 0;
md->use_ucp = (re->options & PCRE_UCP) != 0;
md->jscript_compat = (re->options & PCRE_JAVASCRIPT_COMPAT) != 0;
md->ignore_skip_arg = 0;

/* Some options are unpacked into BOOL variables in the hope that testing
them will be faster than individual option bits. */

md->notbol = (options & PCRE_NOTBOL) != 0;
md->noteol = (options & PCRE_NOTEOL) != 0;
md->notempty = (options & PCRE_NOTEMPTY) != 0;
md->notempty_atstart = (options & PCRE_NOTEMPTY_ATSTART) != 0;

md->hitend = FALSE;
md->mark = md->nomatch_mark = NULL;     /* In case never set */

md->recursive = NULL;                   /* No recursion at top level */
md->hasthen = (re->flags & PCRE_HASTHEN) != 0;

md->lcc = tables + lcc_offset;
md->fcc = tables + fcc_offset;
md->ctypes = tables + ctypes_offset;

/* Handle different \R options. */

switch (options & (PCRE_BSR_ANYCRLF|PCRE_BSR_UNICODE))
  {
  case 0:
  if ((re->options & (PCRE_BSR_ANYCRLF|PCRE_BSR_UNICODE)) != 0)
    md->bsr_anycrlf = (re->options & PCRE_BSR_ANYCRLF) != 0;
  else
#ifdef BSR_ANYCRLF
  md->bsr_anycrlf = TRUE;
#else
  md->bsr_anycrlf = FALSE;
#endif
  break;

  case PCRE_BSR_ANYCRLF:
  md->bsr_anycrlf = TRUE;
  break;

  case PCRE_BSR_UNICODE:
  md->bsr_anycrlf = FALSE;
  break;

  default: return PCRE_ERROR_BADNEWLINE;
  }

/* Handle different types of newline. The three bits give eight cases. If
nothing is set at run time, whatever was used at compile time applies. */

switch ((((options & PCRE_NEWLINE_BITS) == 0)? re->options :
        (pcre_uint32)options) & PCRE_NEWLINE_BITS)
  {
  case 0: newline = NEWLINE; break;   /* Compile-time default */
  case PCRE_NEWLINE_CR: newline = CHAR_CR; break;
  case PCRE_NEWLINE_LF: newline = CHAR_NL; break;
  case PCRE_NEWLINE_CR+
       PCRE_NEWLINE_LF: newline = (CHAR_CR << 8) | CHAR_NL; break;
  case PCRE_NEWLINE_ANY: newline = -1; break;
  case PCRE_NEWLINE_ANYCRLF: newline = -2; break;
  default: return PCRE_ERROR_BADNEWLINE;
  }

if (newline == -2)
  {
  md->nltype = NLTYPE_ANYCRLF;
  }
else if (newline < 0)
  {
  md->nltype = NLTYPE_ANY;
  }
else
  {
  md->nltype = NLTYPE_FIXED;
  if (newline > 255)
    {
    md->nllen = 2;
    md->nl[0] = (newline >> 8) & 255;
    md->nl[1] = newline & 255;
    }
  else
    {
    md->nllen = 1;
    md->nl[0] = newline;
    }
  }

/* Partial matching was originally supported only for a restricted set of
regexes; from release 8.00 there are no restrictions, but the bits are still
defined (though never set). So there's no harm in leaving this code. */

if (md->partial && (re->flags & PCRE_NOPARTIAL) != 0)
  return PCRE_ERROR_BADPARTIAL;

/* If the expression has got more back references than the offsets supplied can
hold, we get a temporary chunk of working store to use during the matching.
Otherwise, we can use the vector supplied, rounding down its size to a multiple
of 3. */

ocount = offsetcount - (offsetcount % 3);
arg_offset_max = (2*ocount)/3;

if (re->top_backref > 0 && re->top_backref >= ocount/3)
  {
  ocount = re->top_backref * 3 + 3;
  md->offset_vector = (int *)(PUBL(malloc))(ocount * sizeof(int));
  if (md->offset_vector == NULL) return PCRE_ERROR_NOMEMORY;
  using_temporary_offsets = TRUE;
  DPRINTF(("Got memory to hold back references\n"));
  }
else md->offset_vector = offsets;
md->offset_end = ocount;
md->offset_max = (2*ocount)/3;
md->capture_last = 0;

/* Reset the working variable associated with each extraction. These should
never be used unless previously set, but they get saved and restored, and so we
initialize them to avoid reading uninitialized locations. Also, unset the
offsets for the matched string. This is really just for tidiness with callouts,
in case they inspect these fields. */

if (md->offset_vector != NULL)
  {
  register int *iptr = md->offset_vector + ocount;
  register int *iend = iptr - re->top_bracket;
  if (iend < md->offset_vector + 2) iend = md->offset_vector + 2;
  while (--iptr >= iend) *iptr = -1;
  if (offsetcount > 0) md->offset_vector[0] = -1;
  if (offsetcount > 1) md->offset_vector[1] = -1;
  }

/* Set up the first character to match, if available. The first_char value is
never set for an anchored regular expression, but the anchoring may be forced
at run time, so we have to test for anchoring. The first char may be unset for
an unanchored pattern, of course. If there's no first char and the pattern was
studied, there may be a bitmap of possible first characters. */

if (!anchored)
  {
  if ((re->flags & PCRE_FIRSTSET) != 0)
    {
    has_first_char = TRUE;
    first_char = first_char2 = (pcre_uchar)(re->first_char);
    if ((re->flags & PCRE_FCH_CASELESS) != 0)
      {
      first_char2 = TABLE_GET(first_char, md->fcc, first_char);
#if defined SUPPORT_UCP && !(defined COMPILE_PCRE8)
      if (utf && first_char > 127)
        first_char2 = UCD_OTHERCASE(first_char);
#endif
      }
    }
  else
    if (!startline && study != NULL &&
      (study->flags & PCRE_STUDY_MAPPED) != 0)
        start_bits = study->start_bits;
  }

/* For anchored or unanchored matches, there may be a "last known required
character" set. */

if ((re->flags & PCRE_REQCHSET) != 0)
  {
  has_req_char = TRUE;
  req_char = req_char2 = (pcre_uchar)(re->req_char);
  if ((re->flags & PCRE_RCH_CASELESS) != 0)
    {
    req_char2 = TABLE_GET(req_char, md->fcc, req_char);
#if defined SUPPORT_UCP && !(defined COMPILE_PCRE8)
    if (utf && req_char > 127)
      req_char2 = UCD_OTHERCASE(req_char);
#endif
    }
  }


/* ==========================================================================*/

/* Loop for handling unanchored repeated matching attempts; for anchored regexs
the loop runs just once. */

for(;;)
  {
  PCRE_PUCHAR save_end_subject = end_subject;
  PCRE_PUCHAR new_start_match;

  /* If firstline is TRUE, the start of the match is constrained to the first
  line of a multiline string. That is, the match must be before or at the first
  newline. Implement this by temporarily adjusting end_subject so that we stop
  scanning at a newline. If the match fails at the newline, later code breaks
  this loop. */

  if (firstline)
    {
    PCRE_PUCHAR t = start_match;
#ifdef SUPPORT_UTF
    if (utf)
      {
      while (t < md->end_subject && !IS_NEWLINE(t))
        {
        t++;
        ACROSSCHAR(t < end_subject, *t, t++);
        }
      }
    else
#endif
    while (t < md->end_subject && !IS_NEWLINE(t)) t++;
    end_subject = t;
    }

  /* There are some optimizations that avoid running the match if a known
  starting point is not found, or if a known later character is not present.
  However, there is an option that disables these, for testing and for ensuring
  that all callouts do actually occur. The option can be set in the regex by
  (*NO_START_OPT) or passed in match-time options. */

  if (((options | re->options) & PCRE_NO_START_OPTIMIZE) == 0)
    {
    /* Advance to a unique first char if there is one. */

    if (has_first_char)
      {
      pcre_uchar smc;

      if (first_char != first_char2)
        while (start_match < end_subject &&
          (smc = UCHAR21TEST(start_match)) != first_char && smc != first_char2)
          start_match++;
      else
        while (start_match < end_subject && UCHAR21TEST(start_match) != first_char)
          start_match++;
      }

    /* Or to just after a linebreak for a multiline match */

    else if (startline)
      {
      if (start_match > md->start_subject + start_offset)
        {
#ifdef SUPPORT_UTF
        if (utf)
          {
          while (start_match < end_subject && !WAS_NEWLINE(start_match))
            {
            start_match++;
            ACROSSCHAR(start_match < end_subject, *start_match,
              start_match++);
            }
          }
        else
#endif
        while (start_match < end_subject && !WAS_NEWLINE(start_match))
          start_match++;

        /* If we have just passed a CR and the newline option is ANY or ANYCRLF,
        and we are now at a LF, advance the match position by one more character.
        */

        if (start_match[-1] == CHAR_CR &&
             (md->nltype == NLTYPE_ANY || md->nltype == NLTYPE_ANYCRLF) &&
             start_match < end_subject &&
             UCHAR21TEST(start_match) == CHAR_NL)
          start_match++;
        }
      }

    /* Or to a non-unique first byte after study */

    else if (start_bits != NULL)
      {
      while (start_match < end_subject)
        {
        register pcre_uint32 c = UCHAR21TEST(start_match);
#ifndef COMPILE_PCRE8
        if (c > 255) c = 255;
#endif
        if ((start_bits[c/8] & (1 << (c&7))) != 0) break;
        start_match++;
        }
      }
    }   /* Starting optimizations */

  /* Restore fudged end_subject */

  end_subject = save_end_subject;

  /* The following two optimizations are disabled for partial matching or if
  disabling is explicitly requested. */

  if (((options | re->options) & PCRE_NO_START_OPTIMIZE) == 0 && !md->partial)
    {
    /* If the pattern was studied, a minimum subject length may be set. This is
    a lower bound; no actual string of that length may actually match the
    pattern. Although the value is, strictly, in characters, we treat it as
    bytes to avoid spending too much time in this optimization. */

    if (study != NULL && (study->flags & PCRE_STUDY_MINLEN) != 0 &&
        (pcre_uint32)(end_subject - start_match) < study->minlength)
      {
      rc = MATCH_NOMATCH;
      break;
      }

    /* If req_char is set, we know that that character must appear in the
    subject for the match to succeed. If the first character is set, req_char
    must be later in the subject; otherwise the test starts at the match point.
    This optimization can save a huge amount of backtracking in patterns with
    nested unlimited repeats that aren't going to match. Writing separate code
    for cased/caseless versions makes it go faster, as does using an
    autoincrement and backing off on a match.

    HOWEVER: when the subject string is very, very long, searching to its end
    can take a long time, and give bad performance on quite ordinary patterns.
    This showed up when somebody was matching something like /^\d+C/ on a
    32-megabyte string... so we don't do this when the string is sufficiently
    long. */

    if (has_req_char && end_subject - start_match < REQ_BYTE_MAX)
      {
      register PCRE_PUCHAR p = start_match + (has_first_char? 1:0);

      /* We don't need to repeat the search if we haven't yet reached the
      place we found it at last time. */

      if (p > req_char_ptr)
        {
        if (req_char != req_char2)
          {
          while (p < end_subject)
            {
            register pcre_uint32 pp = UCHAR21INCTEST(p);
            if (pp == req_char || pp == req_char2) { p--; break; }
            }
          }
        else
          {
          while (p < end_subject)
            {
            if (UCHAR21INCTEST(p) == req_char) { p--; break; }
            }
          }

        /* If we can't find the required character, break the matching loop,
        forcing a match failure. */

        if (p >= end_subject)
          {
          rc = MATCH_NOMATCH;
          break;
          }

        /* If we have found the required character, save the point where we
        found it, so that we don't search again next time round the loop if
        the start hasn't passed this character yet. */

        req_char_ptr = p;
        }
      }
    }

#ifdef PCRE_DEBUG  /* Sigh. Some compilers never learn. */
  printf(">>>> Match against: ");
  pchars(start_match, end_subject - start_match, TRUE, md);
  printf("\n");
#endif

  /* OK, we can now run the match. If "hitend" is set afterwards, remember the
  first starting point for which a partial match was found. */

  md->start_match_ptr = start_match;
  md->start_used_ptr = start_match;
  md->match_call_count = 0;
  md->match_function_type = 0;
  md->end_offset_top = 0;
  md->skip_arg_count = 0;
  rc = match(start_match, md->start_code, start_match, 2, md, NULL, 0);
  if (md->hitend && start_partial == NULL)
    {
    start_partial = md->start_used_ptr;
    match_partial = start_match;
    }

  switch(rc)
    {
    /* If MATCH_SKIP_ARG reaches this level it means that a MARK that matched
    the SKIP's arg was not found. In this circumstance, Perl ignores the SKIP
    entirely. The only way we can do that is to re-do the match at the same
    point, with a flag to force SKIP with an argument to be ignored. Just
    treating this case as NOMATCH does not work because it does not check other
    alternatives in patterns such as A(*SKIP:A)B|AC when the subject is AC. */

    case MATCH_SKIP_ARG:
    new_start_match = start_match;
    md->ignore_skip_arg = md->skip_arg_count;
    break;

    /* SKIP passes back the next starting point explicitly, but if it is no
    greater than the match we have just done, treat it as NOMATCH. */

    case MATCH_SKIP:
    if (md->start_match_ptr > start_match)
      {
      new_start_match = md->start_match_ptr;
      break;
      }
    /* Fall through */

    /* NOMATCH and PRUNE advance by one character. THEN at this level acts
    exactly like PRUNE. Unset ignore SKIP-with-argument. */

    case MATCH_NOMATCH:
    case MATCH_PRUNE:
    case MATCH_THEN:
    md->ignore_skip_arg = 0;
    new_start_match = start_match + 1;
#ifdef SUPPORT_UTF
    if (utf)
      ACROSSCHAR(new_start_match < end_subject, *new_start_match,
        new_start_match++);
#endif
    break;

    /* COMMIT disables the bumpalong, but otherwise behaves as NOMATCH. */

    case MATCH_COMMIT:
    rc = MATCH_NOMATCH;
    goto ENDLOOP;

    /* Any other return is either a match, or some kind of error. */

    default:
    goto ENDLOOP;
    }

  /* Control reaches here for the various types of "no match at this point"
  result. Reset the code to MATCH_NOMATCH for subsequent checking. */

  rc = MATCH_NOMATCH;

  /* If PCRE_FIRSTLINE is set, the match must happen before or at the first
  newline in the subject (though it may continue over the newline). Therefore,
  if we have just failed to match, starting at a newline, do not continue. */

  if (firstline && IS_NEWLINE(start_match)) break;

  /* Advance to new matching position */

  start_match = new_start_match;

  /* Break the loop if the pattern is anchored or if we have passed the end of
  the subject. */

  if (anchored || start_match > end_subject) break;

  /* If we have just passed a CR and we are now at a LF, and the pattern does
  not contain any explicit matches for \r or \n, and the newline option is CRLF
  or ANY or ANYCRLF, advance the match position by one more character. In
  normal matching start_match will aways be greater than the first position at
  this stage, but a failed *SKIP can cause a return at the same point, which is
  why the first test exists. */

  if (start_match > (PCRE_PUCHAR)subject + start_offset &&
      start_match[-1] == CHAR_CR &&
      start_match < end_subject &&
      *start_match == CHAR_NL &&
      (re->flags & PCRE_HASCRORLF) == 0 &&
        (md->nltype == NLTYPE_ANY ||
         md->nltype == NLTYPE_ANYCRLF ||
         md->nllen == 2))
    start_match++;

  md->mark = NULL;   /* Reset for start of next match attempt */
  }                  /* End of for(;;) "bumpalong" loop */

/* ==========================================================================*/

/* We reach here when rc is not MATCH_NOMATCH, or if one of the stopping
conditions is true:

(1) The pattern is anchored or the match was failed by (*COMMIT);

(2) We are past the end of the subject;

(3) PCRE_FIRSTLINE is set and we have failed to match at a newline, because
    this option requests that a match occur at or before the first newline in
    the subject.

When we have a match and the offset vector is big enough to deal with any
backreferences, captured substring offsets will already be set up. In the case
where we had to get some local store to hold offsets for backreference
processing, copy those that we can. In this case there need not be overflow if
certain parts of the pattern were not used, even though there are more
capturing parentheses than vector slots. */

ENDLOOP:

if (rc == MATCH_MATCH || rc == MATCH_ACCEPT)
  {
  if (using_temporary_offsets)
    {
    if (arg_offset_max >= 4)
      {
      memcpy(offsets + 2, md->offset_vector + 2,
        (arg_offset_max - 2) * sizeof(int));
      DPRINTF(("Copied offsets from temporary memory\n"));
      }
    if (md->end_offset_top > arg_offset_max) md->capture_last |= OVFLBIT;
    DPRINTF(("Freeing temporary memory\n"));
    (PUBL(free))(md->offset_vector);
    }

  /* Set the return code to the number of captured strings, or 0 if there were
  too many to fit into the vector. */

  rc = ((md->capture_last & OVFLBIT) != 0 &&
         md->end_offset_top >= arg_offset_max)?
    0 : md->end_offset_top/2;

  /* If there is space in the offset vector, set any unused pairs at the end of
  the pattern to -1 for backwards compatibility. It is documented that this
  happens. In earlier versions, the whole set of potential capturing offsets
  was set to -1 each time round the loop, but this is handled differently now.
  "Gaps" are set to -1 dynamically instead (this fixes a bug). Thus, it is only
  those at the end that need unsetting here. We can't just unset them all at
  the start of the whole thing because they may get set in one branch that is
  not the final matching branch. */

  if (md->end_offset_top/2 <= re->top_bracket && offsets != NULL)
    {
    register int *iptr, *iend;
    int resetcount = 2 + re->top_bracket * 2;
    if (resetcount > offsetcount) resetcount = offsetcount;
    iptr = offsets + md->end_offset_top;
    iend = offsets + resetcount;
    while (iptr < iend) *iptr++ = -1;
    }

  /* If there is space, set up the whole thing as substring 0. The value of
  md->start_match_ptr might be modified if \K was encountered on the success
  matching path. */

  if (offsetcount < 2) rc = 0; else
    {
    offsets[0] = (int)(md->start_match_ptr - md->start_subject);
    offsets[1] = (int)(md->end_match_ptr - md->start_subject);
    }

  /* Return MARK data if requested */

  if (extra_data != NULL && (extra_data->flags & PCRE_EXTRA_MARK) != 0)
    *(extra_data->mark) = (pcre_uchar *)md->mark;
  DPRINTF((">>>> returning %d\n", rc));
#ifdef NO_RECURSE
  release_match_heapframes(&frame_zero);
#endif
  return rc;
  }

/* Control gets here if there has been an error, or if the overall match
attempt has failed at all permitted starting positions. */

if (using_temporary_offsets)
  {
  DPRINTF(("Freeing temporary memory\n"));
  (PUBL(free))(md->offset_vector);
  }

/* For anything other than nomatch or partial match, just return the code. */

if (rc != MATCH_NOMATCH && rc != PCRE_ERROR_PARTIAL)
  {
  DPRINTF((">>>> error: returning %d\n", rc));
#ifdef NO_RECURSE
  release_match_heapframes(&frame_zero);
#endif
  return rc;
  }

/* Handle partial matches - disable any mark data */

if (match_partial != NULL)
  {
  DPRINTF((">>>> returning PCRE_ERROR_PARTIAL\n"));
  md->mark = NULL;
  if (offsetcount > 1)
    {
    offsets[0] = (int)(start_partial - (PCRE_PUCHAR)subject);
    offsets[1] = (int)(end_subject - (PCRE_PUCHAR)subject);
    if (offsetcount > 2)
      offsets[2] = (int)(match_partial - (PCRE_PUCHAR)subject);
    }
  rc = PCRE_ERROR_PARTIAL;
  }

/* This is the classic nomatch case */

else
  {
  DPRINTF((">>>> returning PCRE_ERROR_NOMATCH\n"));
  rc = PCRE_ERROR_NOMATCH;
  }

/* Return the MARK data if it has been requested. */

if (extra_data != NULL && (extra_data->flags & PCRE_EXTRA_MARK) != 0)
  *(extra_data->mark) = (pcre_uchar *)md->nomatch_mark;
#ifdef NO_RECURSE
  release_match_heapframes(&frame_zero);
#endif
return rc;
}

/* End of pcre_exec.c */

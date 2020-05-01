/*************************************************
*      Perl-Compatible Regular Expressions       *
*************************************************/

/* PCRE is a library of functions to support regular expressions whose syntax
and semantics are as close as possible to those of the Perl 5 language.

                       Written by Philip Hazel
           Copyright (c) 1997-2012 University of Cambridge

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


/* This module contains the external function pcre_study(), along with local
supporting functions. */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pcre_internal.h"

#define SET_BIT(c) start_bits[c/8] |= (1 << (c&7))

/* Returns from set_start_bits() */

enum { SSB_FAIL, SSB_DONE, SSB_CONTINUE, SSB_UNKNOWN };



/*************************************************
*   Find the minimum subject length for a group  *
*************************************************/

/* Scan a parenthesized group and compute the minimum length of subject that
is needed to match it. This is a lower bound; it does not mean there is a
string of that length that matches. In UTF8 mode, the result is in characters
rather than bytes.

Arguments:
  re              compiled pattern block
  code            pointer to start of group (the bracket)
  startcode       pointer to start of the whole pattern's code
  options         the compiling options
  recurses        chain of recurse_check to catch mutual recursion
  countptr        pointer to call count (to catch over complexity)

Returns:   the minimum length
           -1 if \C in UTF-8 mode or (*ACCEPT) was encountered
           -2 internal error (missing capturing bracket)
           -3 internal error (opcode not listed)
*/

static int
find_minlength(const REAL_PCRE *re, const pcre_uchar *code,
  const pcre_uchar *startcode, int options, recurse_check *recurses,
  int *countptr)
{
int length = -1;
/* PCRE_UTF16 has the same value as PCRE_UTF8. */
BOOL utf = (options & PCRE_UTF8) != 0;
BOOL had_recurse = FALSE;
recurse_check this_recurse;
register int branchlength = 0;
register pcre_uchar *cc = (pcre_uchar *)code + 1 + LINK_SIZE;

if ((*countptr)++ > 1000) return -1;   /* too complex */

if (*code == OP_CBRA || *code == OP_SCBRA ||
    *code == OP_CBRAPOS || *code == OP_SCBRAPOS) cc += IMM2_SIZE;

/* Scan along the opcodes for this branch. If we get to the end of the
branch, check the length against that of the other branches. */

for (;;)
  {
  int d, min;
  pcre_uchar *cs, *ce;
  register pcre_uchar op = *cc;

  switch (op)
    {
    case OP_COND:
    case OP_SCOND:

    /* If there is only one branch in a condition, the implied branch has zero
    length, so we don't add anything. This covers the DEFINE "condition"
    automatically. */

    cs = cc + GET(cc, 1);
    if (*cs != OP_ALT)
      {
      cc = cs + 1 + LINK_SIZE;
      break;
      }

    /* Otherwise we can fall through and treat it the same as any other
    subpattern. */

    case OP_CBRA:
    case OP_SCBRA:
    case OP_BRA:
    case OP_SBRA:
    case OP_CBRAPOS:
    case OP_SCBRAPOS:
    case OP_BRAPOS:
    case OP_SBRAPOS:
    case OP_ONCE:
    case OP_ONCE_NC:
    d = find_minlength(re, cc, startcode, options, recurses, countptr);
    if (d < 0) return d;
    branchlength += d;
    do cc += GET(cc, 1); while (*cc == OP_ALT);
    cc += 1 + LINK_SIZE;
    break;

    /* ACCEPT makes things far too complicated; we have to give up. */

    case OP_ACCEPT:
    case OP_ASSERT_ACCEPT:
    return -1;

    /* Reached end of a branch; if it's a ket it is the end of a nested
    call. If it's ALT it is an alternation in a nested call. If it is END it's
    the end of the outer call. All can be handled by the same code. If an
    ACCEPT was previously encountered, use the length that was in force at that
    time, and pass back the shortest ACCEPT length. */

    case OP_ALT:
    case OP_KET:
    case OP_KETRMAX:
    case OP_KETRMIN:
    case OP_KETRPOS:
    case OP_END:
    if (length < 0 || (!had_recurse && branchlength < length))
      length = branchlength;
    if (op != OP_ALT) return length;
    cc += 1 + LINK_SIZE;
    branchlength = 0;
    had_recurse = FALSE;
    break;

    /* Skip over assertive subpatterns */

    case OP_ASSERT:
    case OP_ASSERT_NOT:
    case OP_ASSERTBACK:
    case OP_ASSERTBACK_NOT:
    do cc += GET(cc, 1); while (*cc == OP_ALT);
    /* Fall through */

    /* Skip over things that don't match chars */

    case OP_REVERSE:
    case OP_CREF:
    case OP_DNCREF:
    case OP_RREF:
    case OP_DNRREF:
    case OP_DEF:
    case OP_CALLOUT:
    case OP_SOD:
    case OP_SOM:
    case OP_EOD:
    case OP_EODN:
    case OP_CIRC:
    case OP_CIRCM:
    case OP_DOLL:
    case OP_DOLLM:
    case OP_NOT_WORD_BOUNDARY:
    case OP_WORD_BOUNDARY:
    cc += PRIV(OP_lengths)[*cc];
    break;

    /* Skip over a subpattern that has a {0} or {0,x} quantifier */

    case OP_BRAZERO:
    case OP_BRAMINZERO:
    case OP_BRAPOSZERO:
    case OP_SKIPZERO:
    cc += PRIV(OP_lengths)[*cc];
    do cc += GET(cc, 1); while (*cc == OP_ALT);
    cc += 1 + LINK_SIZE;
    break;

    /* Handle literal characters and + repetitions */

    case OP_CHAR:
    case OP_CHARI:
    case OP_NOT:
    case OP_NOTI:
    case OP_PLUS:
    case OP_PLUSI:
    case OP_MINPLUS:
    case OP_MINPLUSI:
    case OP_POSPLUS:
    case OP_POSPLUSI:
    case OP_NOTPLUS:
    case OP_NOTPLUSI:
    case OP_NOTMINPLUS:
    case OP_NOTMINPLUSI:
    case OP_NOTPOSPLUS:
    case OP_NOTPOSPLUSI:
    branchlength++;
    cc += 2;
#ifdef SUPPORT_UTF
    if (utf && HAS_EXTRALEN(cc[-1])) cc += GET_EXTRALEN(cc[-1]);
#endif
    break;

    case OP_TYPEPLUS:
    case OP_TYPEMINPLUS:
    case OP_TYPEPOSPLUS:
    branchlength++;
    cc += (cc[1] == OP_PROP || cc[1] == OP_NOTPROP)? 4 : 2;
    break;

    /* Handle exact repetitions. The count is already in characters, but we
    need to skip over a multibyte character in UTF8 mode.  */

    case OP_EXACT:
    case OP_EXACTI:
    case OP_NOTEXACT:
    case OP_NOTEXACTI:
    branchlength += GET2(cc,1);
    cc += 2 + IMM2_SIZE;
#ifdef SUPPORT_UTF
    if (utf && HAS_EXTRALEN(cc[-1])) cc += GET_EXTRALEN(cc[-1]);
#endif
    break;

    case OP_TYPEEXACT:
    branchlength += GET2(cc,1);
    cc += 2 + IMM2_SIZE + ((cc[1 + IMM2_SIZE] == OP_PROP
      || cc[1 + IMM2_SIZE] == OP_NOTPROP)? 2 : 0);
    break;

    /* Handle single-char non-literal matchers */

    case OP_PROP:
    case OP_NOTPROP:
    cc += 2;
    /* Fall through */

    case OP_NOT_DIGIT:
    case OP_DIGIT:
    case OP_NOT_WHITESPACE:
    case OP_WHITESPACE:
    case OP_NOT_WORDCHAR:
    case OP_WORDCHAR:
    case OP_ANY:
    case OP_ALLANY:
    case OP_EXTUNI:
    case OP_HSPACE:
    case OP_NOT_HSPACE:
    case OP_VSPACE:
    case OP_NOT_VSPACE:
    branchlength++;
    cc++;
    break;

    /* "Any newline" might match two characters, but it also might match just
    one. */

    case OP_ANYNL:
    branchlength += 1;
    cc++;
    break;

    /* The single-byte matcher means we can't proceed in UTF-8 mode. (In
    non-UTF-8 mode \C will actually be turned into OP_ALLANY, so won't ever
    appear, but leave the code, just in case.) */

    case OP_ANYBYTE:
#ifdef SUPPORT_UTF
    if (utf) return -1;
#endif
    branchlength++;
    cc++;
    break;

    /* For repeated character types, we have to test for \p and \P, which have
    an extra two bytes of parameters. */

    case OP_TYPESTAR:
    case OP_TYPEMINSTAR:
    case OP_TYPEQUERY:
    case OP_TYPEMINQUERY:
    case OP_TYPEPOSSTAR:
    case OP_TYPEPOSQUERY:
    if (cc[1] == OP_PROP || cc[1] == OP_NOTPROP) cc += 2;
    cc += PRIV(OP_lengths)[op];
    break;

    case OP_TYPEUPTO:
    case OP_TYPEMINUPTO:
    case OP_TYPEPOSUPTO:
    if (cc[1 + IMM2_SIZE] == OP_PROP
      || cc[1 + IMM2_SIZE] == OP_NOTPROP) cc += 2;
    cc += PRIV(OP_lengths)[op];
    break;

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
      case OP_CRPLUS:
      case OP_CRMINPLUS:
      case OP_CRPOSPLUS:
      branchlength++;
      /* Fall through */

      case OP_CRSTAR:
      case OP_CRMINSTAR:
      case OP_CRQUERY:
      case OP_CRMINQUERY:
      case OP_CRPOSSTAR:
      case OP_CRPOSQUERY:
      cc++;
      break;

      case OP_CRRANGE:
      case OP_CRMINRANGE:
      case OP_CRPOSRANGE:
      branchlength += GET2(cc,1);
      cc += 1 + 2 * IMM2_SIZE;
      break;

      default:
      branchlength++;
      break;
      }
    break;

    /* Backreferences and subroutine calls are treated in the same way: we find
    the minimum length for the subpattern. A recursion, however, causes an
    a flag to be set that causes the length of this branch to be ignored. The
    logic is that a recursion can only make sense if there is another
    alternation that stops the recursing. That will provide the minimum length
    (when no recursion happens). A backreference within the group that it is
    referencing behaves in the same way.

    If PCRE_JAVASCRIPT_COMPAT is set, a backreference to an unset bracket
    matches an empty string (by default it causes a matching failure), so in
    that case we must set the minimum length to zero. */

    case OP_DNREF:     /* Duplicate named pattern back reference */
    case OP_DNREFI:
    if ((options & PCRE_JAVASCRIPT_COMPAT) == 0)
      {
      int count = GET2(cc, 1+IMM2_SIZE);
      pcre_uchar *slot = (pcre_uchar *)re +
        re->name_table_offset + GET2(cc, 1) * re->name_entry_size;
      d = INT_MAX;
      while (count-- > 0)
        {
        ce = cs = (pcre_uchar *)PRIV(find_bracket)(startcode, utf, GET2(slot, 0));
        if (cs == NULL) return -2;
        do ce += GET(ce, 1); while (*ce == OP_ALT);
        if (cc > cs && cc < ce)     /* Simple recursion */
          {
          d = 0;
          had_recurse = TRUE;
          break;
          }
        else
          {
          recurse_check *r = recurses;
          for (r = recurses; r != NULL; r = r->prev) if (r->group == cs) break;
          if (r != NULL)           /* Mutual recursion */
            {
            d = 0;
            had_recurse = TRUE;
            break;
            }
          else
            {
            int dd;
            this_recurse.prev = recurses;
            this_recurse.group = cs;
            dd = find_minlength(re, cs, startcode, options, &this_recurse,
              countptr);
            if (dd < d) d = dd;
            }
          }
        slot += re->name_entry_size;
        }
      }
    else d = 0;
    cc += 1 + 2*IMM2_SIZE;
    goto REPEAT_BACK_REFERENCE;

    case OP_REF:      /* Single back reference */
    case OP_REFI:
    if ((options & PCRE_JAVASCRIPT_COMPAT) == 0)
      {
      ce = cs = (pcre_uchar *)PRIV(find_bracket)(startcode, utf, GET2(cc, 1));
      if (cs == NULL) return -2;
      do ce += GET(ce, 1); while (*ce == OP_ALT);
      if (cc > cs && cc < ce)    /* Simple recursion */
        {
        d = 0;
        had_recurse = TRUE;
        }
      else
        {
        recurse_check *r = recurses;
        for (r = recurses; r != NULL; r = r->prev) if (r->group == cs) break;
        if (r != NULL)           /* Mutual recursion */
          {
          d = 0;
          had_recurse = TRUE;
          }
        else
          {
          this_recurse.prev = recurses;
          this_recurse.group = cs;
          d = find_minlength(re, cs, startcode, options, &this_recurse,
            countptr);
          }
        }
      }
    else d = 0;
    cc += 1 + IMM2_SIZE;

    /* Handle repeated back references */

    REPEAT_BACK_REFERENCE:
    switch (*cc)
      {
      case OP_CRSTAR:
      case OP_CRMINSTAR:
      case OP_CRQUERY:
      case OP_CRMINQUERY:
      case OP_CRPOSSTAR:
      case OP_CRPOSQUERY:
      min = 0;
      cc++;
      break;

      case OP_CRPLUS:
      case OP_CRMINPLUS:
      case OP_CRPOSPLUS:
      min = 1;
      cc++;
      break;

      case OP_CRRANGE:
      case OP_CRMINRANGE:
      case OP_CRPOSRANGE:
      min = GET2(cc, 1);
      cc += 1 + 2 * IMM2_SIZE;
      break;

      default:
      min = 1;
      break;
      }

    branchlength += min * d;
    break;

    /* We can easily detect direct recursion, but not mutual recursion. This is
    caught by a recursion depth count. */

    case OP_RECURSE:
    cs = ce = (pcre_uchar *)startcode + GET(cc, 1);
    do ce += GET(ce, 1); while (*ce == OP_ALT);
    if (cc > cs && cc < ce)    /* Simple recursion */
      had_recurse = TRUE;
    else
      {
      recurse_check *r = recurses;
      for (r = recurses; r != NULL; r = r->prev) if (r->group == cs) break;
      if (r != NULL)           /* Mutual recursion */
        had_recurse = TRUE;
      else
        {
        this_recurse.prev = recurses;
        this_recurse.group = cs;
        branchlength += find_minlength(re, cs, startcode, options,
          &this_recurse, countptr);
        }
      }
    cc += 1 + LINK_SIZE;
    break;

    /* Anything else does not or need not match a character. We can get the
    item's length from the table, but for those that can match zero occurrences
    of a character, we must take special action for UTF-8 characters. As it
    happens, the "NOT" versions of these opcodes are used at present only for
    ASCII characters, so they could be omitted from this list. However, in
    future that may change, so we include them here so as not to leave a
    gotcha for a future maintainer. */

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

    cc += PRIV(OP_lengths)[op];
#ifdef SUPPORT_UTF
    if (utf && HAS_EXTRALEN(cc[-1])) cc += GET_EXTRALEN(cc[-1]);
#endif
    break;

    /* Skip these, but we need to add in the name length. */

    case OP_MARK:
    case OP_PRUNE_ARG:
    case OP_SKIP_ARG:
    case OP_THEN_ARG:
    cc += PRIV(OP_lengths)[op] + cc[1];
    break;

    /* The remaining opcodes are just skipped over. */

    case OP_CLOSE:
    case OP_COMMIT:
    case OP_FAIL:
    case OP_PRUNE:
    case OP_SET_SOM:
    case OP_SKIP:
    case OP_THEN:
    cc += PRIV(OP_lengths)[op];
    break;

    /* This should not occur: we list all opcodes explicitly so that when
    new ones get added they are properly considered. */

    default:
    return -3;
    }
  }
/* Control never gets here */
}



/*************************************************
*      Set a bit and maybe its alternate case    *
*************************************************/

/* Given a character, set its first byte's bit in the table, and also the
corresponding bit for the other version of a letter if we are caseless. In
UTF-8 mode, for characters greater than 127, we can only do the caseless thing
when Unicode property support is available.

Arguments:
  start_bits    points to the bit map
  p             points to the character
  caseless      the caseless flag
  cd            the block with char table pointers
  utf           TRUE for UTF-8 / UTF-16 / UTF-32 mode

Returns:        pointer after the character
*/

static const pcre_uchar *
set_table_bit(pcre_uint8 *start_bits, const pcre_uchar *p, BOOL caseless,
  compile_data *cd, BOOL utf)
{
pcre_uint32 c = *p;

#ifdef COMPILE_PCRE8
SET_BIT(c);

#ifdef SUPPORT_UTF
if (utf && c > 127)
  {
  GETCHARINC(c, p);
#ifdef SUPPORT_UCP
  if (caseless)
    {
    pcre_uchar buff[6];
    c = UCD_OTHERCASE(c);
    (void)PRIV(ord2utf)(c, buff);
    SET_BIT(buff[0]);
    }
#endif  /* Not SUPPORT_UCP */
  return p;
  }
#else   /* Not SUPPORT_UTF */
(void)(utf);   /* Stops warning for unused parameter */
#endif  /* SUPPORT_UTF */

/* Not UTF-8 mode, or character is less than 127. */

if (caseless && (cd->ctypes[c] & ctype_letter) != 0) SET_BIT(cd->fcc[c]);
return p + 1;
#endif  /* COMPILE_PCRE8 */

#if defined COMPILE_PCRE16 || defined COMPILE_PCRE32
if (c > 0xff)
  {
  c = 0xff;
  caseless = FALSE;
  }
SET_BIT(c);

#ifdef SUPPORT_UTF
if (utf && c > 127)
  {
  GETCHARINC(c, p);
#ifdef SUPPORT_UCP
  if (caseless)
    {
    c = UCD_OTHERCASE(c);
    if (c > 0xff)
      c = 0xff;
    SET_BIT(c);
    }
#endif  /* SUPPORT_UCP */
  return p;
  }
#else   /* Not SUPPORT_UTF */
(void)(utf);   /* Stops warning for unused parameter */
#endif  /* SUPPORT_UTF */

if (caseless && (cd->ctypes[c] & ctype_letter) != 0) SET_BIT(cd->fcc[c]);
return p + 1;
#endif
}



/*************************************************
*     Set bits for a positive character type     *
*************************************************/

/* This function sets starting bits for a character type. In UTF-8 mode, we can
only do a direct setting for bytes less than 128, as otherwise there can be
confusion with bytes in the middle of UTF-8 characters. In a "traditional"
environment, the tables will only recognize ASCII characters anyway, but in at
least one Windows environment, some higher bytes bits were set in the tables.
So we deal with that case by considering the UTF-8 encoding.

Arguments:
  start_bits     the starting bitmap
  cbit type      the type of character wanted
  table_limit    32 for non-UTF-8; 16 for UTF-8
  cd             the block with char table pointers

Returns:         nothing
*/

static void
set_type_bits(pcre_uint8 *start_bits, int cbit_type, unsigned int table_limit,
  compile_data *cd)
{
register pcre_uint32 c;
for (c = 0; c < table_limit; c++) start_bits[c] |= cd->cbits[c+cbit_type];
#if defined SUPPORT_UTF && defined COMPILE_PCRE8
if (table_limit == 32) return;
for (c = 128; c < 256; c++)
  {
  if ((cd->cbits[c/8] & (1 << (c&7))) != 0)
    {
    pcre_uchar buff[6];
    (void)PRIV(ord2utf)(c, buff);
    SET_BIT(buff[0]);
    }
  }
#endif
}


/*************************************************
*     Set bits for a negative character type     *
*************************************************/

/* This function sets starting bits for a negative character type such as \D.
In UTF-8 mode, we can only do a direct setting for bytes less than 128, as
otherwise there can be confusion with bytes in the middle of UTF-8 characters.
Unlike in the positive case, where we can set appropriate starting bits for
specific high-valued UTF-8 characters, in this case we have to set the bits for
all high-valued characters. The lowest is 0xc2, but we overkill by starting at
0xc0 (192) for simplicity.

Arguments:
  start_bits     the starting bitmap
  cbit type      the type of character wanted
  table_limit    32 for non-UTF-8; 16 for UTF-8
  cd             the block with char table pointers

Returns:         nothing
*/

static void
set_nottype_bits(pcre_uint8 *start_bits, int cbit_type, unsigned int table_limit,
  compile_data *cd)
{
register pcre_uint32 c;
for (c = 0; c < table_limit; c++) start_bits[c] |= ~cd->cbits[c+cbit_type];
#if defined SUPPORT_UTF && defined COMPILE_PCRE8
if (table_limit != 32) for (c = 24; c < 32; c++) start_bits[c] = 0xff;
#endif
}



/*************************************************
*          Create bitmap of starting bytes       *
*************************************************/

/* This function scans a compiled unanchored expression recursively and
attempts to build a bitmap of the set of possible starting bytes. As time goes
by, we may be able to get more clever at doing this. The SSB_CONTINUE return is
useful for parenthesized groups in patterns such as (a*)b where the group
provides some optional starting bytes but scanning must continue at the outer
level to find at least one mandatory byte. At the outermost level, this
function fails unless the result is SSB_DONE.

Arguments:
  code         points to an expression
  start_bits   points to a 32-byte table, initialized to 0
  utf          TRUE if in UTF-8 / UTF-16 / UTF-32 mode
  cd           the block with char table pointers

Returns:       SSB_FAIL     => Failed to find any starting bytes
               SSB_DONE     => Found mandatory starting bytes
               SSB_CONTINUE => Found optional starting bytes
               SSB_UNKNOWN  => Hit an unrecognized opcode
*/

static int
set_start_bits(const pcre_uchar *code, pcre_uint8 *start_bits, BOOL utf,
  compile_data *cd)
{
register pcre_uint32 c;
int yield = SSB_DONE;
#if defined SUPPORT_UTF && defined COMPILE_PCRE8
int table_limit = utf? 16:32;
#else
int table_limit = 32;
#endif

#if 0
/* ========================================================================= */
/* The following comment and code was inserted in January 1999. In May 2006,
when it was observed to cause compiler warnings about unused values, I took it
out again. If anybody is still using OS/2, they will have to put it back
manually. */

/* This next statement and the later reference to dummy are here in order to
trick the optimizer of the IBM C compiler for OS/2 into generating correct
code. Apparently IBM isn't going to fix the problem, and we would rather not
disable optimization (in this module it actually makes a big difference, and
the pcre module can use all the optimization it can get). */

volatile int dummy;
/* ========================================================================= */
#endif

do
  {
  BOOL try_next = TRUE;
  const pcre_uchar *tcode = code + 1 + LINK_SIZE;

  if (*code == OP_CBRA || *code == OP_SCBRA ||
      *code == OP_CBRAPOS || *code == OP_SCBRAPOS) tcode += IMM2_SIZE;

  while (try_next)    /* Loop for items in this branch */
    {
    int rc;

    switch(*tcode)
      {
      /* If we reach something we don't understand, it means a new opcode has
      been created that hasn't been added to this code. Hopefully this problem
      will be discovered during testing. */

      default:
      return SSB_UNKNOWN;

      /* Fail for a valid opcode that implies no starting bits. */

      case OP_ACCEPT:
      case OP_ASSERT_ACCEPT:
      case OP_ALLANY:
      case OP_ANY:
      case OP_ANYBYTE:
      case OP_CIRC:
      case OP_CIRCM:
      case OP_CLOSE:
      case OP_COMMIT:
      case OP_COND:
      case OP_CREF:
      case OP_DEF:
      case OP_DNCREF:
      case OP_DNREF:
      case OP_DNREFI:
      case OP_DNRREF:
      case OP_DOLL:
      case OP_DOLLM:
      case OP_END:
      case OP_EOD:
      case OP_EODN:
      case OP_EXTUNI:
      case OP_FAIL:
      case OP_MARK:
      case OP_NOT:
      case OP_NOTEXACT:
      case OP_NOTEXACTI:
      case OP_NOTI:
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
      case OP_NOTPROP:
      case OP_NOTQUERY:
      case OP_NOTQUERYI:
      case OP_NOTSTAR:
      case OP_NOTSTARI:
      case OP_NOTUPTO:
      case OP_NOTUPTOI:
      case OP_NOT_HSPACE:
      case OP_NOT_VSPACE:
      case OP_PRUNE:
      case OP_PRUNE_ARG:
      case OP_RECURSE:
      case OP_REF:
      case OP_REFI:
      case OP_REVERSE:
      case OP_RREF:
      case OP_SCOND:
      case OP_SET_SOM:
      case OP_SKIP:
      case OP_SKIP_ARG:
      case OP_SOD:
      case OP_SOM:
      case OP_THEN:
      case OP_THEN_ARG:
      return SSB_FAIL;

      /* A "real" property test implies no starting bits, but the fake property
      PT_CLIST identifies a list of characters. These lists are short, as they
      are used for characters with more than one "other case", so there is no
      point in recognizing them for OP_NOTPROP. */

      case OP_PROP:
      if (tcode[1] != PT_CLIST) return SSB_FAIL;
        {
        const pcre_uint32 *p = PRIV(ucd_caseless_sets) + tcode[2];
        while ((c = *p++) < NOTACHAR)
          {
#if defined SUPPORT_UTF && defined COMPILE_PCRE8
          if (utf)
            {
            pcre_uchar buff[6];
            (void)PRIV(ord2utf)(c, buff);
            c = buff[0];
            }
#endif
          if (c > 0xff) SET_BIT(0xff); else SET_BIT(c);
          }
        }
      try_next = FALSE;
      break;

      /* We can ignore word boundary tests. */

      case OP_WORD_BOUNDARY:
      case OP_NOT_WORD_BOUNDARY:
      tcode++;
      break;

      /* If we hit a bracket or a positive lookahead assertion, recurse to set
      bits from within the subpattern. If it can't find anything, we have to
      give up. If it finds some mandatory character(s), we are done for this
      branch. Otherwise, carry on scanning after the subpattern. */

      case OP_BRA:
      case OP_SBRA:
      case OP_CBRA:
      case OP_SCBRA:
      case OP_BRAPOS:
      case OP_SBRAPOS:
      case OP_CBRAPOS:
      case OP_SCBRAPOS:
      case OP_ONCE:
      case OP_ONCE_NC:
      case OP_ASSERT:
      rc = set_start_bits(tcode, start_bits, utf, cd);
      if (rc == SSB_FAIL || rc == SSB_UNKNOWN) return rc;
      if (rc == SSB_DONE) try_next = FALSE; else
        {
        do tcode += GET(tcode, 1); while (*tcode == OP_ALT);
        tcode += 1 + LINK_SIZE;
        }
      break;

      /* If we hit ALT or KET, it means we haven't found anything mandatory in
      this branch, though we might have found something optional. For ALT, we
      continue with the next alternative, but we have to arrange that the final
      result from subpattern is SSB_CONTINUE rather than SSB_DONE. For KET,
      return SSB_CONTINUE: if this is the top level, that indicates failure,
      but after a nested subpattern, it causes scanning to continue. */

      case OP_ALT:
      yield = SSB_CONTINUE;
      try_next = FALSE;
      break;

      case OP_KET:
      case OP_KETRMAX:
      case OP_KETRMIN:
      case OP_KETRPOS:
      return SSB_CONTINUE;

      /* Skip over callout */

      case OP_CALLOUT:
      tcode += 2 + 2*LINK_SIZE;
      break;

      /* Skip over lookbehind and negative lookahead assertions */

      case OP_ASSERT_NOT:
      case OP_ASSERTBACK:
      case OP_ASSERTBACK_NOT:
      do tcode += GET(tcode, 1); while (*tcode == OP_ALT);
      tcode += 1 + LINK_SIZE;
      break;

      /* BRAZERO does the bracket, but carries on. */

      case OP_BRAZERO:
      case OP_BRAMINZERO:
      case OP_BRAPOSZERO:
      rc = set_start_bits(++tcode, start_bits, utf, cd);
      if (rc == SSB_FAIL || rc == SSB_UNKNOWN) return rc;
/* =========================================================================
      See the comment at the head of this function concerning the next line,
      which was an old fudge for the benefit of OS/2.
      dummy = 1;
  ========================================================================= */
      do tcode += GET(tcode,1); while (*tcode == OP_ALT);
      tcode += 1 + LINK_SIZE;
      break;

      /* SKIPZERO skips the bracket. */

      case OP_SKIPZERO:
      tcode++;
      do tcode += GET(tcode,1); while (*tcode == OP_ALT);
      tcode += 1 + LINK_SIZE;
      break;

      /* Single-char * or ? sets the bit and tries the next item */

      case OP_STAR:
      case OP_MINSTAR:
      case OP_POSSTAR:
      case OP_QUERY:
      case OP_MINQUERY:
      case OP_POSQUERY:
      tcode = set_table_bit(start_bits, tcode + 1, FALSE, cd, utf);
      break;

      case OP_STARI:
      case OP_MINSTARI:
      case OP_POSSTARI:
      case OP_QUERYI:
      case OP_MINQUERYI:
      case OP_POSQUERYI:
      tcode = set_table_bit(start_bits, tcode + 1, TRUE, cd, utf);
      break;

      /* Single-char upto sets the bit and tries the next */

      case OP_UPTO:
      case OP_MINUPTO:
      case OP_POSUPTO:
      tcode = set_table_bit(start_bits, tcode + 1 + IMM2_SIZE, FALSE, cd, utf);
      break;

      case OP_UPTOI:
      case OP_MINUPTOI:
      case OP_POSUPTOI:
      tcode = set_table_bit(start_bits, tcode + 1 + IMM2_SIZE, TRUE, cd, utf);
      break;

      /* At least one single char sets the bit and stops */

      case OP_EXACT:
      tcode += IMM2_SIZE;
      /* Fall through */
      case OP_CHAR:
      case OP_PLUS:
      case OP_MINPLUS:
      case OP_POSPLUS:
      (void)set_table_bit(start_bits, tcode + 1, FALSE, cd, utf);
      try_next = FALSE;
      break;

      case OP_EXACTI:
      tcode += IMM2_SIZE;
      /* Fall through */
      case OP_CHARI:
      case OP_PLUSI:
      case OP_MINPLUSI:
      case OP_POSPLUSI:
      (void)set_table_bit(start_bits, tcode + 1, TRUE, cd, utf);
      try_next = FALSE;
      break;

      /* Special spacing and line-terminating items. These recognize specific
      lists of characters. The difference between VSPACE and ANYNL is that the
      latter can match the two-character CRLF sequence, but that is not
      relevant for finding the first character, so their code here is
      identical. */

      case OP_HSPACE:
      SET_BIT(CHAR_HT);
      SET_BIT(CHAR_SPACE);
#ifdef SUPPORT_UTF
      if (utf)
        {
#ifdef COMPILE_PCRE8
        SET_BIT(0xC2);  /* For U+00A0 */
        SET_BIT(0xE1);  /* For U+1680, U+180E */
        SET_BIT(0xE2);  /* For U+2000 - U+200A, U+202F, U+205F */
        SET_BIT(0xE3);  /* For U+3000 */
#elif defined COMPILE_PCRE16 || defined COMPILE_PCRE32
        SET_BIT(0xA0);
        SET_BIT(0xFF);  /* For characters > 255 */
#endif  /* COMPILE_PCRE[8|16|32] */
        }
      else
#endif /* SUPPORT_UTF */
        {
#ifndef EBCDIC
        SET_BIT(0xA0);
#endif  /* Not EBCDIC */
#if defined COMPILE_PCRE16 || defined COMPILE_PCRE32
        SET_BIT(0xFF);  /* For characters > 255 */
#endif  /* COMPILE_PCRE[16|32] */
        }
      try_next = FALSE;
      break;

      case OP_ANYNL:
      case OP_VSPACE:
      SET_BIT(CHAR_LF);
      SET_BIT(CHAR_VT);
      SET_BIT(CHAR_FF);
      SET_BIT(CHAR_CR);
#ifdef SUPPORT_UTF
      if (utf)
        {
#ifdef COMPILE_PCRE8
        SET_BIT(0xC2);  /* For U+0085 */
        SET_BIT(0xE2);  /* For U+2028, U+2029 */
#elif defined COMPILE_PCRE16 || defined COMPILE_PCRE32
        SET_BIT(CHAR_NEL);
        SET_BIT(0xFF);  /* For characters > 255 */
#endif  /* COMPILE_PCRE[8|16|32] */
        }
      else
#endif /* SUPPORT_UTF */
        {
        SET_BIT(CHAR_NEL);
#if defined COMPILE_PCRE16 || defined COMPILE_PCRE32
        SET_BIT(0xFF);  /* For characters > 255 */
#endif
        }
      try_next = FALSE;
      break;

      /* Single character types set the bits and stop. Note that if PCRE_UCP
      is set, we do not see these op codes because \d etc are converted to
      properties. Therefore, these apply in the case when only characters less
      than 256 are recognized to match the types. */

      case OP_NOT_DIGIT:
      set_nottype_bits(start_bits, cbit_digit, table_limit, cd);
      try_next = FALSE;
      break;

      case OP_DIGIT:
      set_type_bits(start_bits, cbit_digit, table_limit, cd);
      try_next = FALSE;
      break;

      /* The cbit_space table has vertical tab as whitespace; we no longer
      have to play fancy tricks because Perl added VT to its whitespace at
      release 5.18. PCRE added it at release 8.34. */

      case OP_NOT_WHITESPACE:
      set_nottype_bits(start_bits, cbit_space, table_limit, cd);
      try_next = FALSE;
      break;

      case OP_WHITESPACE:
      set_type_bits(start_bits, cbit_space, table_limit, cd);
      try_next = FALSE;
      break;

      case OP_NOT_WORDCHAR:
      set_nottype_bits(start_bits, cbit_word, table_limit, cd);
      try_next = FALSE;
      break;

      case OP_WORDCHAR:
      set_type_bits(start_bits, cbit_word, table_limit, cd);
      try_next = FALSE;
      break;

      /* One or more character type fudges the pointer and restarts, knowing
      it will hit a single character type and stop there. */

      case OP_TYPEPLUS:
      case OP_TYPEMINPLUS:
      case OP_TYPEPOSPLUS:
      tcode++;
      break;

      case OP_TYPEEXACT:
      tcode += 1 + IMM2_SIZE;
      break;

      /* Zero or more repeats of character types set the bits and then
      try again. */

      case OP_TYPEUPTO:
      case OP_TYPEMINUPTO:
      case OP_TYPEPOSUPTO:
      tcode += IMM2_SIZE;  /* Fall through */

      case OP_TYPESTAR:
      case OP_TYPEMINSTAR:
      case OP_TYPEPOSSTAR:
      case OP_TYPEQUERY:
      case OP_TYPEMINQUERY:
      case OP_TYPEPOSQUERY:
      switch(tcode[1])
        {
        default:
        case OP_ANY:
        case OP_ALLANY:
        return SSB_FAIL;

        case OP_HSPACE:
        SET_BIT(CHAR_HT);
        SET_BIT(CHAR_SPACE);
#ifdef SUPPORT_UTF
        if (utf)
          {
#ifdef COMPILE_PCRE8
          SET_BIT(0xC2);  /* For U+00A0 */
          SET_BIT(0xE1);  /* For U+1680, U+180E */
          SET_BIT(0xE2);  /* For U+2000 - U+200A, U+202F, U+205F */
          SET_BIT(0xE3);  /* For U+3000 */
#elif defined COMPILE_PCRE16 || defined COMPILE_PCRE32
          SET_BIT(0xA0);
          SET_BIT(0xFF);  /* For characters > 255 */
#endif  /* COMPILE_PCRE[8|16|32] */
          }
        else
#endif /* SUPPORT_UTF */
#ifndef EBCDIC
          SET_BIT(0xA0);
#endif  /* Not EBCDIC */
        break;

        case OP_ANYNL:
        case OP_VSPACE:
        SET_BIT(CHAR_LF);
        SET_BIT(CHAR_VT);
        SET_BIT(CHAR_FF);
        SET_BIT(CHAR_CR);
#ifdef SUPPORT_UTF
        if (utf)
          {
#ifdef COMPILE_PCRE8
          SET_BIT(0xC2);  /* For U+0085 */
          SET_BIT(0xE2);  /* For U+2028, U+2029 */
#elif defined COMPILE_PCRE16 || defined COMPILE_PCRE32
          SET_BIT(CHAR_NEL);
          SET_BIT(0xFF);  /* For characters > 255 */
#endif  /* COMPILE_PCRE16 */
          }
        else
#endif /* SUPPORT_UTF */
          SET_BIT(CHAR_NEL);
        break;

        case OP_NOT_DIGIT:
        set_nottype_bits(start_bits, cbit_digit, table_limit, cd);
        break;

        case OP_DIGIT:
        set_type_bits(start_bits, cbit_digit, table_limit, cd);
        break;

        /* The cbit_space table has vertical tab as whitespace; we no longer
        have to play fancy tricks because Perl added VT to its whitespace at
        release 5.18. PCRE added it at release 8.34. */

        case OP_NOT_WHITESPACE:
        set_nottype_bits(start_bits, cbit_space, table_limit, cd);
        break;

        case OP_WHITESPACE:
        set_type_bits(start_bits, cbit_space, table_limit, cd);
        break;

        case OP_NOT_WORDCHAR:
        set_nottype_bits(start_bits, cbit_word, table_limit, cd);
        break;

        case OP_WORDCHAR:
        set_type_bits(start_bits, cbit_word, table_limit, cd);
        break;
        }

      tcode += 2;
      break;

      /* Character class where all the information is in a bit map: set the
      bits and either carry on or not, according to the repeat count. If it was
      a negative class, and we are operating with UTF-8 characters, any byte
      with a value >= 0xc4 is a potentially valid starter because it starts a
      character with a value > 255. */

#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
      case OP_XCLASS:
      if ((tcode[1 + LINK_SIZE] & XCL_HASPROP) != 0)
        return SSB_FAIL;
      /* All bits are set. */
      if ((tcode[1 + LINK_SIZE] & XCL_MAP) == 0 && (tcode[1 + LINK_SIZE] & XCL_NOT) != 0)
        return SSB_FAIL;
#endif
      /* Fall through */

      case OP_NCLASS:
#if defined SUPPORT_UTF && defined COMPILE_PCRE8
      if (utf)
        {
        start_bits[24] |= 0xf0;              /* Bits for 0xc4 - 0xc8 */
        memset(start_bits+25, 0xff, 7);      /* Bits for 0xc9 - 0xff */
        }
#endif
#if defined COMPILE_PCRE16 || defined COMPILE_PCRE32
      SET_BIT(0xFF);                         /* For characters > 255 */
#endif
      /* Fall through */

      case OP_CLASS:
        {
        pcre_uint8 *map;
#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
        map = NULL;
        if (*tcode == OP_XCLASS)
          {
          if ((tcode[1 + LINK_SIZE] & XCL_MAP) != 0)
            map = (pcre_uint8 *)(tcode + 1 + LINK_SIZE + 1);
          tcode += GET(tcode, 1);
          }
        else
#endif
          {
          tcode++;
          map = (pcre_uint8 *)tcode;
          tcode += 32 / sizeof(pcre_uchar);
          }

        /* In UTF-8 mode, the bits in a bit map correspond to character
        values, not to byte values. However, the bit map we are constructing is
        for byte values. So we have to do a conversion for characters whose
        value is > 127. In fact, there are only two possible starting bytes for
        characters in the range 128 - 255. */

#if defined SUPPORT_UTF || !defined COMPILE_PCRE8
        if (map != NULL)
#endif
          {
#if defined SUPPORT_UTF && defined COMPILE_PCRE8
          if (utf)
            {
            for (c = 0; c < 16; c++) start_bits[c] |= map[c];
            for (c = 128; c < 256; c++)
              {
              if ((map[c/8] && (1 << (c&7))) != 0)
                {
                int d = (c >> 6) | 0xc0;            /* Set bit for this starter */
                start_bits[d/8] |= (1 << (d&7));    /* and then skip on to the */
                c = (c & 0xc0) + 0x40 - 1;          /* next relevant character. */
                }
              }
            }
          else
#endif
            {
            /* In non-UTF-8 mode, the two bit maps are completely compatible. */
            for (c = 0; c < 32; c++) start_bits[c] |= map[c];
            }
          }

        /* Advance past the bit map, and act on what follows. For a zero
        minimum repeat, continue; otherwise stop processing. */

        switch (*tcode)
          {
          case OP_CRSTAR:
          case OP_CRMINSTAR:
          case OP_CRQUERY:
          case OP_CRMINQUERY:
          case OP_CRPOSSTAR:
          case OP_CRPOSQUERY:
          tcode++;
          break;

          case OP_CRRANGE:
          case OP_CRMINRANGE:
          case OP_CRPOSRANGE:
          if (GET2(tcode, 1) == 0) tcode += 1 + 2 * IMM2_SIZE;
            else try_next = FALSE;
          break;

          default:
          try_next = FALSE;
          break;
          }
        }
      break; /* End of bitmap class handling */

      }      /* End of switch */
    }        /* End of try_next loop */

  code += GET(code, 1);   /* Advance to next branch */
  }
while (*code == OP_ALT);
return yield;
}





/*************************************************
*          Study a compiled expression           *
*************************************************/

/* This function is handed a compiled expression that it must study to produce
information that will speed up the matching. It returns a pcre[16]_extra block
which then gets handed back to pcre_exec().

Arguments:
  re        points to the compiled expression
  options   contains option bits
  errorptr  points to where to place error messages;
            set NULL unless error

Returns:    pointer to a pcre[16]_extra block, with study_data filled in and
              the appropriate flags set;
            NULL on error or if no optimization possible
*/

#if defined COMPILE_PCRE8
PCRE_EXP_DEFN pcre_extra * PCRE_CALL_CONVENTION
pcre_study(const pcre *external_re, int options, const char **errorptr)
#elif defined COMPILE_PCRE16
PCRE_EXP_DEFN pcre16_extra * PCRE_CALL_CONVENTION
pcre16_study(const pcre16 *external_re, int options, const char **errorptr)
#elif defined COMPILE_PCRE32
PCRE_EXP_DEFN pcre32_extra * PCRE_CALL_CONVENTION
pcre32_study(const pcre32 *external_re, int options, const char **errorptr)
#endif
{
int min;
int count = 0;
BOOL bits_set = FALSE;
pcre_uint8 start_bits[32];
PUBL(extra) *extra = NULL;
pcre_study_data *study;
const pcre_uint8 *tables;
pcre_uchar *code;
compile_data compile_block;
const REAL_PCRE *re = (const REAL_PCRE *)external_re;


*errorptr = NULL;

if (re == NULL || re->magic_number != MAGIC_NUMBER)
  {
  *errorptr = "argument is not a compiled regular expression";
  return NULL;
  }

if ((re->flags & PCRE_MODE) == 0)
  {
#if defined COMPILE_PCRE8
  *errorptr = "argument not compiled in 8 bit mode";
#elif defined COMPILE_PCRE16
  *errorptr = "argument not compiled in 16 bit mode";
#elif defined COMPILE_PCRE32
  *errorptr = "argument not compiled in 32 bit mode";
#endif
  return NULL;
  }

if ((options & ~PUBLIC_STUDY_OPTIONS) != 0)
  {
  *errorptr = "unknown or incorrect option bit(s) set";
  return NULL;
  }

code = (pcre_uchar *)re + re->name_table_offset +
  (re->name_count * re->name_entry_size);

/* For an anchored pattern, or an unanchored pattern that has a first char, or
a multiline pattern that matches only at "line starts", there is no point in
seeking a list of starting bytes. */

if ((re->options & PCRE_ANCHORED) == 0 &&
    (re->flags & (PCRE_FIRSTSET|PCRE_STARTLINE)) == 0)
  {
  int rc;

  /* Set the character tables in the block that is passed around */

  tables = re->tables;

#if defined COMPILE_PCRE8
  if (tables == NULL)
    (void)pcre_fullinfo(external_re, NULL, PCRE_INFO_DEFAULT_TABLES,
    (void *)(&tables));
#elif defined COMPILE_PCRE16
  if (tables == NULL)
    (void)pcre16_fullinfo(external_re, NULL, PCRE_INFO_DEFAULT_TABLES,
    (void *)(&tables));
#elif defined COMPILE_PCRE32
  if (tables == NULL)
    (void)pcre32_fullinfo(external_re, NULL, PCRE_INFO_DEFAULT_TABLES,
    (void *)(&tables));
#endif

  compile_block.lcc = tables + lcc_offset;
  compile_block.fcc = tables + fcc_offset;
  compile_block.cbits = tables + cbits_offset;
  compile_block.ctypes = tables + ctypes_offset;

  /* See if we can find a fixed set of initial characters for the pattern. */

  memset(start_bits, 0, 32 * sizeof(pcre_uint8));
  rc = set_start_bits(code, start_bits, (re->options & PCRE_UTF8) != 0,
    &compile_block);
  bits_set = rc == SSB_DONE;
  if (rc == SSB_UNKNOWN)
    {
    *errorptr = "internal error: opcode not recognized";
    return NULL;
    }
  }

/* Find the minimum length of subject string. */

switch(min = find_minlength(re, code, code, re->options, NULL, &count))
  {
  case -2: *errorptr = "internal error: missing capturing bracket"; return NULL;
  case -3: *errorptr = "internal error: opcode not recognized"; return NULL;
  default: break;
  }

/* If a set of starting bytes has been identified, or if the minimum length is
greater than zero, or if JIT optimization has been requested, or if
PCRE_STUDY_EXTRA_NEEDED is set, get a pcre[16]_extra block and a
pcre_study_data block. The study data is put in the latter, which is pointed to
by the former, which may also get additional data set later by the calling
program. At the moment, the size of pcre_study_data is fixed. We nevertheless
save it in a field for returning via the pcre_fullinfo() function so that if it
becomes variable in the future, we don't have to change that code. */

if (bits_set || min > 0 || (options & (
#ifdef SUPPORT_JIT
    PCRE_STUDY_JIT_COMPILE | PCRE_STUDY_JIT_PARTIAL_SOFT_COMPILE |
    PCRE_STUDY_JIT_PARTIAL_HARD_COMPILE |
#endif
    PCRE_STUDY_EXTRA_NEEDED)) != 0)
  {
  extra = (PUBL(extra) *)(PUBL(malloc))
    (sizeof(PUBL(extra)) + sizeof(pcre_study_data));
  if (extra == NULL)
    {
    *errorptr = "failed to get memory";
    return NULL;
    }

  study = (pcre_study_data *)((char *)extra + sizeof(PUBL(extra)));
  extra->flags = PCRE_EXTRA_STUDY_DATA;
  extra->study_data = study;

  study->size = sizeof(pcre_study_data);
  study->flags = 0;

  /* Set the start bits always, to avoid unset memory errors if the
  study data is written to a file, but set the flag only if any of the bits
  are set, to save time looking when none are. */

  if (bits_set)
    {
    study->flags |= PCRE_STUDY_MAPPED;
    memcpy(study->start_bits, start_bits, sizeof(start_bits));
    }
  else memset(study->start_bits, 0, 32 * sizeof(pcre_uint8));

#ifdef PCRE_DEBUG
  if (bits_set)
    {
    pcre_uint8 *ptr = start_bits;
    int i;

    printf("Start bits:\n");
    for (i = 0; i < 32; i++)
      printf("%3d: %02x%s", i * 8, *ptr++, ((i + 1) & 0x7) != 0? " " : "\n");
    }
#endif

  /* Always set the minlength value in the block, because the JIT compiler
  makes use of it. However, don't set the bit unless the length is greater than
  zero - the interpretive pcre_exec() and pcre_dfa_exec() needn't waste time
  checking the zero case. */

  if (min > 0)
    {
    study->flags |= PCRE_STUDY_MINLEN;
    study->minlength = min;
    }
  else study->minlength = 0;

  /* If JIT support was compiled and requested, attempt the JIT compilation.
  If no starting bytes were found, and the minimum length is zero, and JIT
  compilation fails, abandon the extra block and return NULL, unless
  PCRE_STUDY_EXTRA_NEEDED is set. */

#ifdef SUPPORT_JIT
  extra->executable_jit = NULL;
  if ((options & PCRE_STUDY_JIT_COMPILE) != 0)
    PRIV(jit_compile)(re, extra, JIT_COMPILE);
  if ((options & PCRE_STUDY_JIT_PARTIAL_SOFT_COMPILE) != 0)
    PRIV(jit_compile)(re, extra, JIT_PARTIAL_SOFT_COMPILE);
  if ((options & PCRE_STUDY_JIT_PARTIAL_HARD_COMPILE) != 0)
    PRIV(jit_compile)(re, extra, JIT_PARTIAL_HARD_COMPILE);

  if (study->flags == 0 && (extra->flags & PCRE_EXTRA_EXECUTABLE_JIT) == 0 &&
      (options & PCRE_STUDY_EXTRA_NEEDED) == 0)
    {
#if defined COMPILE_PCRE8
    pcre_free_study(extra);
#elif defined COMPILE_PCRE16
    pcre16_free_study(extra);
#elif defined COMPILE_PCRE32
    pcre32_free_study(extra);
#endif
    extra = NULL;
    }
#endif
  }

return extra;
}


/*************************************************
*          Free the study data                   *
*************************************************/

/* This function frees the memory that was obtained by pcre_study().

Argument:   a pointer to the pcre[16]_extra block
Returns:    nothing
*/

#if defined COMPILE_PCRE8
PCRE_EXP_DEFN void
pcre_free_study(pcre_extra *extra)
#elif defined COMPILE_PCRE16
PCRE_EXP_DEFN void
pcre16_free_study(pcre16_extra *extra)
#elif defined COMPILE_PCRE32
PCRE_EXP_DEFN void
pcre32_free_study(pcre32_extra *extra)
#endif
{
if (extra == NULL)
  return;
#ifdef SUPPORT_JIT
if ((extra->flags & PCRE_EXTRA_EXECUTABLE_JIT) != 0 &&
     extra->executable_jit != NULL)
  PRIV(jit_free)(extra->executable_jit);
#endif
PUBL(free)(extra);
}

/* End of pcre_study.c */

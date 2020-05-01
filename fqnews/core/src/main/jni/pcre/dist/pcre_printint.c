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


/* This module contains a PCRE private debugging function for printing out the
internal form of a compiled regular expression, along with some supporting
local functions. This source file is used in two places:

(1) It is #included by pcre_compile.c when it is compiled in debugging mode
(PCRE_DEBUG defined in pcre_internal.h). It is not included in production
compiles. In this case PCRE_INCLUDED is defined.

(2) It is also compiled separately and linked with pcretest.c, which can be
asked to print out a compiled regex for debugging purposes. */

#ifndef PCRE_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* For pcretest program. */
#define PRIV(name) name

/* We have to include pcre_internal.h because we need the internal info for
displaying the results of pcre_study() and we also need to know about the
internal macros, structures, and other internal data values; pcretest has
"inside information" compared to a program that strictly follows the PCRE API.

Although pcre_internal.h does itself include pcre.h, we explicitly include it
here before pcre_internal.h so that the PCRE_EXP_xxx macros get set
appropriately for an application, not for building PCRE. */

#include "pcre.h"
#include "pcre_internal.h"

/* These are the funtions that are contained within. It doesn't seem worth
having a separate .h file just for this. */

#endif /* PCRE_INCLUDED */

#ifdef PCRE_INCLUDED
static /* Keep the following function as private. */
#endif

#if defined COMPILE_PCRE8
void pcre_printint(pcre *external_re, FILE *f, BOOL print_lengths);
#elif defined COMPILE_PCRE16
void pcre16_printint(pcre *external_re, FILE *f, BOOL print_lengths);
#elif defined COMPILE_PCRE32
void pcre32_printint(pcre *external_re, FILE *f, BOOL print_lengths);
#endif

/* Macro that decides whether a character should be output as a literal or in
hexadecimal. We don't use isprint() because that can vary from system to system
(even without the use of locales) and we want the output always to be the same,
for testing purposes. */

#ifdef EBCDIC
#define PRINTABLE(c) ((c) >= 64 && (c) < 255)
#else
#define PRINTABLE(c) ((c) >= 32 && (c) < 127)
#endif

/* The table of operator names. */

static const char *priv_OP_names[] = { OP_NAME_LIST };

/* This table of operator lengths is not actually used by the working code,
but its size is needed for a check that ensures it is the correct size for the
number of opcodes (thus catching update omissions). */

static const pcre_uint8 priv_OP_lengths[] = { OP_LENGTHS };



/*************************************************
*       Print single- or multi-byte character    *
*************************************************/

static unsigned int
print_char(FILE *f, pcre_uchar *ptr, BOOL utf)
{
pcre_uint32 c = *ptr;

#ifndef SUPPORT_UTF

(void)utf;  /* Avoid compiler warning */
if (PRINTABLE(c)) fprintf(f, "%c", (char)c);
else if (c <= 0x80) fprintf(f, "\\x%02x", c);
else fprintf(f, "\\x{%x}", c);
return 0;

#else

#if defined COMPILE_PCRE8

if (!utf || (c & 0xc0) != 0xc0)
  {
  if (PRINTABLE(c)) fprintf(f, "%c", (char)c);
  else if (c < 0x80) fprintf(f, "\\x%02x", c);
  else fprintf(f, "\\x{%02x}", c);
  return 0;
  }
else
  {
  int i;
  int a = PRIV(utf8_table4)[c & 0x3f];  /* Number of additional bytes */
  int s = 6*a;
  c = (c & PRIV(utf8_table3)[a]) << s;
  for (i = 1; i <= a; i++)
    {
    /* This is a check for malformed UTF-8; it should only occur if the sanity
    check has been turned off. Rather than swallow random bytes, just stop if
    we hit a bad one. Print it with \X instead of \x as an indication. */

    if ((ptr[i] & 0xc0) != 0x80)
      {
      fprintf(f, "\\X{%x}", c);
      return i - 1;
      }

    /* The byte is OK */

    s -= 6;
    c |= (ptr[i] & 0x3f) << s;
    }
  fprintf(f, "\\x{%x}", c);
  return a;
  }

#elif defined COMPILE_PCRE16

if (!utf || (c & 0xfc00) != 0xd800)
  {
  if (PRINTABLE(c)) fprintf(f, "%c", (char)c);
  else if (c <= 0x80) fprintf(f, "\\x%02x", c);
  else fprintf(f, "\\x{%02x}", c);
  return 0;
  }
else
  {
  /* This is a check for malformed UTF-16; it should only occur if the sanity
  check has been turned off. Rather than swallow a low surrogate, just stop if
  we hit a bad one. Print it with \X instead of \x as an indication. */

  if ((ptr[1] & 0xfc00) != 0xdc00)
    {
    fprintf(f, "\\X{%x}", c);
    return 0;
    }

  c = (((c & 0x3ff) << 10) | (ptr[1] & 0x3ff)) + 0x10000;
  fprintf(f, "\\x{%x}", c);
  return 1;
  }

#elif defined COMPILE_PCRE32

if (!utf || (c & 0xfffff800u) != 0xd800u)
  {
  if (PRINTABLE(c)) fprintf(f, "%c", (char)c);
  else if (c <= 0x80) fprintf(f, "\\x%02x", c);
  else fprintf(f, "\\x{%x}", c);
  return 0;
  }
else
  {
  /* This is a check for malformed UTF-32; it should only occur if the sanity
  check has been turned off. Rather than swallow a surrogate, just stop if
  we hit one. Print it with \X instead of \x as an indication. */
  fprintf(f, "\\X{%x}", c);
  return 0;
  }

#endif /* COMPILE_PCRE[8|16|32] */

#endif /* SUPPORT_UTF */
}

/*************************************************
*  Print uchar string (regardless of utf)        *
*************************************************/

static void
print_puchar(FILE *f, PCRE_PUCHAR ptr)
{
while (*ptr != '\0')
  {
  register pcre_uint32 c = *ptr++;
  if (PRINTABLE(c)) fprintf(f, "%c", c); else fprintf(f, "\\x{%x}", c);
  }
}

/*************************************************
*          Find Unicode property name            *
*************************************************/

static const char *
get_ucpname(unsigned int ptype, unsigned int pvalue)
{
#ifdef SUPPORT_UCP
int i;
for (i = PRIV(utt_size) - 1; i >= 0; i--)
  {
  if (ptype == PRIV(utt)[i].type && pvalue == PRIV(utt)[i].value) break;
  }
return (i >= 0)? PRIV(utt_names) + PRIV(utt)[i].name_offset : "??";
#else
/* It gets harder and harder to shut off unwanted compiler warnings. */
ptype = ptype * pvalue;
return (ptype == pvalue)? "??" : "??";
#endif
}


/*************************************************
*       Print Unicode property value             *
*************************************************/

/* "Normal" properties can be printed from tables. The PT_CLIST property is a
pseudo-property that contains a pointer to a list of case-equivalent
characters. This is used only when UCP support is available and UTF mode is
selected. It should never occur otherwise, but just in case it does, have
something ready to print. */

static void
print_prop(FILE *f, pcre_uchar *code, const char *before, const char *after)
{
if (code[1] != PT_CLIST)
  {
  fprintf(f, "%s%s %s%s", before, priv_OP_names[*code], get_ucpname(code[1],
    code[2]), after);
  }
else
  {
  const char *not = (*code == OP_PROP)? "" : "not ";
#ifndef SUPPORT_UCP
  fprintf(f, "%s%sclist %d%s", before, not, code[2], after);
#else
  const pcre_uint32 *p = PRIV(ucd_caseless_sets) + code[2];
  fprintf (f, "%s%sclist", before, not);
  while (*p < NOTACHAR) fprintf(f, " %04x", *p++);
  fprintf(f, "%s", after);
#endif
  }
}




/*************************************************
*         Print compiled regex                   *
*************************************************/

/* Make this function work for a regex with integers either byte order.
However, we assume that what we are passed is a compiled regex. The
print_lengths flag controls whether offsets and lengths of items are printed.
They can be turned off from pcretest so that automatic tests on bytecode can be
written that do not depend on the value of LINK_SIZE. */

#ifdef PCRE_INCLUDED
static /* Keep the following function as private. */
#endif
#if defined COMPILE_PCRE8
void
pcre_printint(pcre *external_re, FILE *f, BOOL print_lengths)
#elif defined COMPILE_PCRE16
void
pcre16_printint(pcre *external_re, FILE *f, BOOL print_lengths)
#elif defined COMPILE_PCRE32
void
pcre32_printint(pcre *external_re, FILE *f, BOOL print_lengths)
#endif
{
REAL_PCRE *re = (REAL_PCRE *)external_re;
pcre_uchar *codestart, *code;
BOOL utf;

unsigned int options = re->options;
int offset = re->name_table_offset;
int count = re->name_count;
int size = re->name_entry_size;

if (re->magic_number != MAGIC_NUMBER)
  {
  offset = ((offset << 8) & 0xff00) | ((offset >> 8) & 0xff);
  count = ((count << 8) & 0xff00) | ((count >> 8) & 0xff);
  size = ((size << 8) & 0xff00) | ((size >> 8) & 0xff);
  options = ((options << 24) & 0xff000000) |
            ((options <<  8) & 0x00ff0000) |
            ((options >>  8) & 0x0000ff00) |
            ((options >> 24) & 0x000000ff);
  }

code = codestart = (pcre_uchar *)re + offset + count * size;
/* PCRE_UTF(16|32) have the same value as PCRE_UTF8. */
utf = (options & PCRE_UTF8) != 0;

for(;;)
  {
  pcre_uchar *ccode;
  const char *flag = "  ";
  pcre_uint32 c;
  unsigned int extra = 0;

  if (print_lengths)
    fprintf(f, "%3d ", (int)(code - codestart));
  else
    fprintf(f, "    ");

  switch(*code)
    {
/* ========================================================================== */
      /* These cases are never obeyed. This is a fudge that causes a compile-
      time error if the vectors OP_names or OP_lengths, which are indexed
      by opcode, are not the correct length. It seems to be the only way to do
      such a check at compile time, as the sizeof() operator does not work in
      the C preprocessor. */

      case OP_TABLE_LENGTH:
      case OP_TABLE_LENGTH +
        ((sizeof(priv_OP_names)/sizeof(const char *) == OP_TABLE_LENGTH) &&
        (sizeof(priv_OP_lengths) == OP_TABLE_LENGTH)):
      break;
/* ========================================================================== */

    case OP_END:
    fprintf(f, "    %s\n", priv_OP_names[*code]);
    fprintf(f, "------------------------------------------------------------------\n");
    return;

    case OP_CHAR:
    fprintf(f, "    ");
    do
      {
      code++;
      code += 1 + print_char(f, code, utf);
      }
    while (*code == OP_CHAR);
    fprintf(f, "\n");
    continue;

    case OP_CHARI:
    fprintf(f, " /i ");
    do
      {
      code++;
      code += 1 + print_char(f, code, utf);
      }
    while (*code == OP_CHARI);
    fprintf(f, "\n");
    continue;

    case OP_CBRA:
    case OP_CBRAPOS:
    case OP_SCBRA:
    case OP_SCBRAPOS:
    if (print_lengths) fprintf(f, "%3d ", GET(code, 1));
      else fprintf(f, "    ");
    fprintf(f, "%s %d", priv_OP_names[*code], GET2(code, 1+LINK_SIZE));
    break;

    case OP_BRA:
    case OP_BRAPOS:
    case OP_SBRA:
    case OP_SBRAPOS:
    case OP_KETRMAX:
    case OP_KETRMIN:
    case OP_KETRPOS:
    case OP_ALT:
    case OP_KET:
    case OP_ASSERT:
    case OP_ASSERT_NOT:
    case OP_ASSERTBACK:
    case OP_ASSERTBACK_NOT:
    case OP_ONCE:
    case OP_ONCE_NC:
    case OP_COND:
    case OP_SCOND:
    case OP_REVERSE:
    if (print_lengths) fprintf(f, "%3d ", GET(code, 1));
      else fprintf(f, "    ");
    fprintf(f, "%s", priv_OP_names[*code]);
    break;

    case OP_CLOSE:
    fprintf(f, "    %s %d", priv_OP_names[*code], GET2(code, 1));
    break;

    case OP_CREF:
    fprintf(f, "%3d %s", GET2(code,1), priv_OP_names[*code]);
    break;

    case OP_DNCREF:
      {
      pcre_uchar *entry = (pcre_uchar *)re + offset + (GET2(code, 1) * size) +
        IMM2_SIZE;
      fprintf(f, " %s Cond ref <", flag);
      print_puchar(f, entry);
      fprintf(f, ">%d", GET2(code, 1 + IMM2_SIZE));
      }
    break;

    case OP_RREF:
    c = GET2(code, 1);
    if (c == RREF_ANY)
      fprintf(f, "    Cond recurse any");
    else
      fprintf(f, "    Cond recurse %d", c);
    break;

    case OP_DNRREF:
      {
      pcre_uchar *entry = (pcre_uchar *)re + offset + (GET2(code, 1) * size) +
        IMM2_SIZE;
      fprintf(f, " %s Cond recurse <", flag);
      print_puchar(f, entry);
      fprintf(f, ">%d", GET2(code, 1 + IMM2_SIZE));
      }
    break;

    case OP_DEF:
    fprintf(f, "    Cond def");
    break;

    case OP_STARI:
    case OP_MINSTARI:
    case OP_POSSTARI:
    case OP_PLUSI:
    case OP_MINPLUSI:
    case OP_POSPLUSI:
    case OP_QUERYI:
    case OP_MINQUERYI:
    case OP_POSQUERYI:
    flag = "/i";
    /* Fall through */
    case OP_STAR:
    case OP_MINSTAR:
    case OP_POSSTAR:
    case OP_PLUS:
    case OP_MINPLUS:
    case OP_POSPLUS:
    case OP_QUERY:
    case OP_MINQUERY:
    case OP_POSQUERY:
    case OP_TYPESTAR:
    case OP_TYPEMINSTAR:
    case OP_TYPEPOSSTAR:
    case OP_TYPEPLUS:
    case OP_TYPEMINPLUS:
    case OP_TYPEPOSPLUS:
    case OP_TYPEQUERY:
    case OP_TYPEMINQUERY:
    case OP_TYPEPOSQUERY:
    fprintf(f, " %s ", flag);
    if (*code >= OP_TYPESTAR)
      {
      if (code[1] == OP_PROP || code[1] == OP_NOTPROP)
        {
        print_prop(f, code + 1, "", " ");
        extra = 2;
        }
      else fprintf(f, "%s", priv_OP_names[code[1]]);
      }
    else extra = print_char(f, code+1, utf);
    fprintf(f, "%s", priv_OP_names[*code]);
    break;

    case OP_EXACTI:
    case OP_UPTOI:
    case OP_MINUPTOI:
    case OP_POSUPTOI:
    flag = "/i";
    /* Fall through */
    case OP_EXACT:
    case OP_UPTO:
    case OP_MINUPTO:
    case OP_POSUPTO:
    fprintf(f, " %s ", flag);
    extra = print_char(f, code + 1 + IMM2_SIZE, utf);
    fprintf(f, "{");
    if (*code != OP_EXACT && *code != OP_EXACTI) fprintf(f, "0,");
    fprintf(f, "%d}", GET2(code,1));
    if (*code == OP_MINUPTO || *code == OP_MINUPTOI) fprintf(f, "?");
      else if (*code == OP_POSUPTO || *code == OP_POSUPTOI) fprintf(f, "+");
    break;

    case OP_TYPEEXACT:
    case OP_TYPEUPTO:
    case OP_TYPEMINUPTO:
    case OP_TYPEPOSUPTO:
    if (code[1 + IMM2_SIZE] == OP_PROP || code[1 + IMM2_SIZE] == OP_NOTPROP)
      {
      print_prop(f, code + IMM2_SIZE + 1, "    ", " ");
      extra = 2;
      }
    else fprintf(f, "    %s", priv_OP_names[code[1 + IMM2_SIZE]]);
    fprintf(f, "{");
    if (*code != OP_TYPEEXACT) fprintf(f, "0,");
    fprintf(f, "%d}", GET2(code,1));
    if (*code == OP_TYPEMINUPTO) fprintf(f, "?");
      else if (*code == OP_TYPEPOSUPTO) fprintf(f, "+");
    break;

    case OP_NOTI:
    flag = "/i";
    /* Fall through */
    case OP_NOT:
    fprintf(f, " %s [^", flag);
    extra = print_char(f, code + 1, utf);
    fprintf(f, "]");
    break;

    case OP_NOTSTARI:
    case OP_NOTMINSTARI:
    case OP_NOTPOSSTARI:
    case OP_NOTPLUSI:
    case OP_NOTMINPLUSI:
    case OP_NOTPOSPLUSI:
    case OP_NOTQUERYI:
    case OP_NOTMINQUERYI:
    case OP_NOTPOSQUERYI:
    flag = "/i";
    /* Fall through */

    case OP_NOTSTAR:
    case OP_NOTMINSTAR:
    case OP_NOTPOSSTAR:
    case OP_NOTPLUS:
    case OP_NOTMINPLUS:
    case OP_NOTPOSPLUS:
    case OP_NOTQUERY:
    case OP_NOTMINQUERY:
    case OP_NOTPOSQUERY:
    fprintf(f, " %s [^", flag);
    extra = print_char(f, code + 1, utf);
    fprintf(f, "]%s", priv_OP_names[*code]);
    break;

    case OP_NOTEXACTI:
    case OP_NOTUPTOI:
    case OP_NOTMINUPTOI:
    case OP_NOTPOSUPTOI:
    flag = "/i";
    /* Fall through */

    case OP_NOTEXACT:
    case OP_NOTUPTO:
    case OP_NOTMINUPTO:
    case OP_NOTPOSUPTO:
    fprintf(f, " %s [^", flag);
    extra = print_char(f, code + 1 + IMM2_SIZE, utf);
    fprintf(f, "]{");
    if (*code != OP_NOTEXACT && *code != OP_NOTEXACTI) fprintf(f, "0,");
    fprintf(f, "%d}", GET2(code,1));
    if (*code == OP_NOTMINUPTO || *code == OP_NOTMINUPTOI) fprintf(f, "?");
      else
    if (*code == OP_NOTPOSUPTO || *code == OP_NOTPOSUPTOI) fprintf(f, "+");
    break;

    case OP_RECURSE:
    if (print_lengths) fprintf(f, "%3d ", GET(code, 1));
      else fprintf(f, "    ");
    fprintf(f, "%s", priv_OP_names[*code]);
    break;

    case OP_REFI:
    flag = "/i";
    /* Fall through */
    case OP_REF:
    fprintf(f, " %s \\%d", flag, GET2(code,1));
    ccode = code + priv_OP_lengths[*code];
    goto CLASS_REF_REPEAT;

    case OP_DNREFI:
    flag = "/i";
    /* Fall through */
    case OP_DNREF:
      {
      pcre_uchar *entry = (pcre_uchar *)re + offset + (GET2(code, 1) * size) +
        IMM2_SIZE;
      fprintf(f, " %s \\k<", flag);
      print_puchar(f, entry);
      fprintf(f, ">%d", GET2(code, 1 + IMM2_SIZE));
      }
    ccode = code + priv_OP_lengths[*code];
    goto CLASS_REF_REPEAT;

    case OP_CALLOUT:
    fprintf(f, "    %s %d %d %d", priv_OP_names[*code], code[1], GET(code,2),
      GET(code, 2 + LINK_SIZE));
    break;

    case OP_PROP:
    case OP_NOTPROP:
    print_prop(f, code, "    ", "");
    break;

    /* OP_XCLASS cannot occur in 8-bit, non-UTF mode. However, there's no harm
    in having this code always here, and it makes it less messy without all
    those #ifdefs. */

    case OP_CLASS:
    case OP_NCLASS:
    case OP_XCLASS:
      {
      int i;
      unsigned int min, max;
      BOOL printmap;
      BOOL invertmap = FALSE;
      pcre_uint8 *map;
      pcre_uint8 inverted_map[32];

      fprintf(f, "    [");

      if (*code == OP_XCLASS)
        {
        extra = GET(code, 1);
        ccode = code + LINK_SIZE + 1;
        printmap = (*ccode & XCL_MAP) != 0;
        if ((*ccode & XCL_NOT) != 0)
          {
          invertmap = (*ccode & XCL_HASPROP) == 0;
          fprintf(f, "^");
          }
        ccode++;
        }
      else
        {
        printmap = TRUE;
        ccode = code + 1;
        }

      /* Print a bit map */

      if (printmap)
        {
        map = (pcre_uint8 *)ccode;
        if (invertmap)
          {
          for (i = 0; i < 32; i++) inverted_map[i] = ~map[i];
          map = inverted_map;
          }

        for (i = 0; i < 256; i++)
          {
          if ((map[i/8] & (1 << (i&7))) != 0)
            {
            int j;
            for (j = i+1; j < 256; j++)
              if ((map[j/8] & (1 << (j&7))) == 0) break;
            if (i == '-' || i == ']') fprintf(f, "\\");
            if (PRINTABLE(i)) fprintf(f, "%c", i);
              else fprintf(f, "\\x%02x", i);
            if (--j > i)
              {
              if (j != i + 1) fprintf(f, "-");
              if (j == '-' || j == ']') fprintf(f, "\\");
              if (PRINTABLE(j)) fprintf(f, "%c", j);
                else fprintf(f, "\\x%02x", j);
              }
            i = j;
            }
          }
        ccode += 32 / sizeof(pcre_uchar);
        }

      /* For an XCLASS there is always some additional data */

      if (*code == OP_XCLASS)
        {
        pcre_uchar ch;
        while ((ch = *ccode++) != XCL_END)
          {
          BOOL not = FALSE;
          const char *notch = "";

          switch(ch)
            {
            case XCL_NOTPROP:
            not = TRUE;
            notch = "^";
            /* Fall through */

            case XCL_PROP:
              {
              unsigned int ptype = *ccode++;
              unsigned int pvalue = *ccode++;

              switch(ptype)
                {
                case PT_PXGRAPH:
                fprintf(f, "[:%sgraph:]", notch);
                break;

                case PT_PXPRINT:
                fprintf(f, "[:%sprint:]", notch);
                break;

                case PT_PXPUNCT:
                fprintf(f, "[:%spunct:]", notch);
                break;

                default:
                fprintf(f, "\\%c{%s}", (not? 'P':'p'),
                  get_ucpname(ptype, pvalue));
                break;
                }
              }
            break;

            default:
            ccode += 1 + print_char(f, ccode, utf);
            if (ch == XCL_RANGE)
              {
              fprintf(f, "-");
              ccode += 1 + print_char(f, ccode, utf);
              }
            break;
            }
          }
        }

      /* Indicate a non-UTF class which was created by negation */

      fprintf(f, "]%s", (*code == OP_NCLASS)? " (neg)" : "");

      /* Handle repeats after a class or a back reference */

      CLASS_REF_REPEAT:
      switch(*ccode)
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
        fprintf(f, "%s", priv_OP_names[*ccode]);
        extra += priv_OP_lengths[*ccode];
        break;

        case OP_CRRANGE:
        case OP_CRMINRANGE:
        case OP_CRPOSRANGE:
        min = GET2(ccode,1);
        max = GET2(ccode,1 + IMM2_SIZE);
        if (max == 0) fprintf(f, "{%u,}", min);
        else fprintf(f, "{%u,%u}", min, max);
        if (*ccode == OP_CRMINRANGE) fprintf(f, "?");
        else if (*ccode == OP_CRPOSRANGE) fprintf(f, "+");
        extra += priv_OP_lengths[*ccode];
        break;

        /* Do nothing if it's not a repeat; this code stops picky compilers
        warning about the lack of a default code path. */

        default:
        break;
        }
      }
    break;

    case OP_MARK:
    case OP_PRUNE_ARG:
    case OP_SKIP_ARG:
    case OP_THEN_ARG:
    fprintf(f, "    %s ", priv_OP_names[*code]);
    print_puchar(f, code + 2);
    extra += code[1];
    break;

    case OP_THEN:
    fprintf(f, "    %s", priv_OP_names[*code]);
    break;

    case OP_CIRCM:
    case OP_DOLLM:
    flag = "/m";
    /* Fall through */

    /* Anything else is just an item with no data, but possibly a flag. */

    default:
    fprintf(f, " %s %s", flag, priv_OP_names[*code]);
    break;
    }

  code += priv_OP_lengths[*code] + extra;
  fprintf(f, "\n");
  }
}

/* End of pcre_printint.src */

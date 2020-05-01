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


/* This module contains an internal function that tests a compiled pattern to
see if it was compiled with the opposite endianness. If so, it uses an
auxiliary local function to flip the appropriate bytes. */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pcre_internal.h"


/*************************************************
*             Swap byte functions                *
*************************************************/

/* The following functions swap the bytes of a pcre_uint16
and pcre_uint32 value.

Arguments:
  value        any number

Returns:       the byte swapped value
*/

static pcre_uint32
swap_uint32(pcre_uint32 value)
{
return ((value & 0x000000ff) << 24) |
       ((value & 0x0000ff00) <<  8) |
       ((value & 0x00ff0000) >>  8) |
       (value >> 24);
}

static pcre_uint16
swap_uint16(pcre_uint16 value)
{
return (value >> 8) | (value << 8);
}


/*************************************************
*       Test for a byte-flipped compiled regex   *
*************************************************/

/* This function swaps the bytes of a compiled pattern usually
loaded form the disk. It also sets the tables pointer, which
is likely an invalid pointer after reload.

Arguments:
  argument_re     points to the compiled expression
  extra_data      points to extra data or is NULL
  tables          points to the character tables or NULL

Returns:          0 if the swap is successful, negative on error
*/

#if defined COMPILE_PCRE8
PCRE_EXP_DECL int pcre_pattern_to_host_byte_order(pcre *argument_re,
  pcre_extra *extra_data, const unsigned char *tables)
#elif defined COMPILE_PCRE16
PCRE_EXP_DECL int pcre16_pattern_to_host_byte_order(pcre16 *argument_re,
  pcre16_extra *extra_data, const unsigned char *tables)
#elif defined COMPILE_PCRE32
PCRE_EXP_DECL int pcre32_pattern_to_host_byte_order(pcre32 *argument_re,
  pcre32_extra *extra_data, const unsigned char *tables)
#endif
{
REAL_PCRE *re = (REAL_PCRE *)argument_re;
pcre_study_data *study;
#ifndef COMPILE_PCRE8
pcre_uchar *ptr;
int length;
#if defined SUPPORT_UTF && defined COMPILE_PCRE16
BOOL utf;
BOOL utf16_char;
#endif /* SUPPORT_UTF && COMPILE_PCRE16 */
#endif /* !COMPILE_PCRE8 */

if (re == NULL) return PCRE_ERROR_NULL;
if (re->magic_number == MAGIC_NUMBER)
  {
  if ((re->flags & PCRE_MODE) == 0) return PCRE_ERROR_BADMODE;
  re->tables = tables;
  return 0;
  }

if (re->magic_number != REVERSED_MAGIC_NUMBER) return PCRE_ERROR_BADMAGIC;
if ((swap_uint32(re->flags) & PCRE_MODE) == 0) return PCRE_ERROR_BADMODE;

re->magic_number = MAGIC_NUMBER;
re->size = swap_uint32(re->size);
re->options = swap_uint32(re->options);
re->flags = swap_uint32(re->flags);
re->limit_match = swap_uint32(re->limit_match);
re->limit_recursion = swap_uint32(re->limit_recursion);

#if defined COMPILE_PCRE8 || defined COMPILE_PCRE16
re->first_char = swap_uint16(re->first_char);
re->req_char = swap_uint16(re->req_char);
#elif defined COMPILE_PCRE32
re->first_char = swap_uint32(re->first_char);
re->req_char = swap_uint32(re->req_char);
#endif

re->max_lookbehind = swap_uint16(re->max_lookbehind);
re->top_bracket = swap_uint16(re->top_bracket);
re->top_backref = swap_uint16(re->top_backref);
re->name_table_offset = swap_uint16(re->name_table_offset);
re->name_entry_size = swap_uint16(re->name_entry_size);
re->name_count = swap_uint16(re->name_count);
re->ref_count = swap_uint16(re->ref_count);
re->tables = tables;

if (extra_data != NULL && (extra_data->flags & PCRE_EXTRA_STUDY_DATA) != 0)
  {
  study = (pcre_study_data *)extra_data->study_data;
  study->size = swap_uint32(study->size);
  study->flags = swap_uint32(study->flags);
  study->minlength = swap_uint32(study->minlength);
  }

#ifndef COMPILE_PCRE8
ptr = (pcre_uchar *)re + re->name_table_offset;
length = re->name_count * re->name_entry_size;
#if defined SUPPORT_UTF && defined COMPILE_PCRE16
utf = (re->options & PCRE_UTF16) != 0;
utf16_char = FALSE;
#endif /* SUPPORT_UTF && COMPILE_PCRE16 */

while(TRUE)
  {
  /* Swap previous characters. */
  while (length-- > 0)
    {
#if defined COMPILE_PCRE16
    *ptr = swap_uint16(*ptr);
#elif defined COMPILE_PCRE32
    *ptr = swap_uint32(*ptr);
#endif
    ptr++;
    }
#if defined SUPPORT_UTF && defined COMPILE_PCRE16
  if (utf16_char)
    {
    if (HAS_EXTRALEN(ptr[-1]))
      {
      /* We know that there is only one extra character in UTF-16. */
      *ptr = swap_uint16(*ptr);
      ptr++;
      }
    }
  utf16_char = FALSE;
#endif /* SUPPORT_UTF */

  /* Get next opcode. */
  length = 0;
#if defined COMPILE_PCRE16
  *ptr = swap_uint16(*ptr);
#elif defined COMPILE_PCRE32
  *ptr = swap_uint32(*ptr);
#endif
  switch (*ptr)
    {
    case OP_END:
    return 0;

#if defined SUPPORT_UTF && defined COMPILE_PCRE16
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
    if (utf) utf16_char = TRUE;
#endif
    /* Fall through. */

    default:
    length = PRIV(OP_lengths)[*ptr] - 1;
    break;

    case OP_CLASS:
    case OP_NCLASS:
    /* Skip the character bit map. */
    ptr += 32/sizeof(pcre_uchar);
    length = 0;
    break;

    case OP_XCLASS:
    /* Reverse the size of the XCLASS instance. */
    ptr++;
#if defined COMPILE_PCRE16
    *ptr = swap_uint16(*ptr);
#elif defined COMPILE_PCRE32
    *ptr = swap_uint32(*ptr);
#endif
#ifndef COMPILE_PCRE32
    if (LINK_SIZE > 1)
      {
      /* LINK_SIZE can be 1 or 2 in 16 bit mode. */
      ptr++;
      *ptr = swap_uint16(*ptr);
      }
#endif
    ptr++;
    length = (GET(ptr, -LINK_SIZE)) - (1 + LINK_SIZE + 1);
#if defined COMPILE_PCRE16
    *ptr = swap_uint16(*ptr);
#elif defined COMPILE_PCRE32
    *ptr = swap_uint32(*ptr);
#endif
    if ((*ptr & XCL_MAP) != 0)
      {
      /* Skip the character bit map. */
      ptr += 32/sizeof(pcre_uchar);
      length -= 32/sizeof(pcre_uchar);
      }
    break;
    }
  ptr++;
  }
/* Control should never reach here in 16/32 bit mode. */
#else  /* In 8-bit mode, the pattern does not need to be processed. */
return 0;
#endif /* !COMPILE_PCRE8 */
}

/* End of pcre_byte_order.c */

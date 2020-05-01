/*************************************************
*      Perl-Compatible Regular Expressions       *
*************************************************/

/* PCRE is a library of functions to support regular expressions whose syntax
and semantics are as close as possible to those of the Perl 5 language.

                       Written by Philip Hazel
           Copyright (c) 1997-2013 University of Cambridge

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


/* This module contains the external function pcre_fullinfo(), which returns
information about a compiled pattern. */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pcre_internal.h"


/*************************************************
*        Return info about compiled pattern      *
*************************************************/

/* This is a newer "info" function which has an extensible interface so
that additional items can be added compatibly.

Arguments:
  argument_re      points to compiled code
  extra_data       points extra data, or NULL
  what             what information is required
  where            where to put the information

Returns:           0 if data returned, negative on error
*/

#if defined COMPILE_PCRE8
PCRE_EXP_DEFN int PCRE_CALL_CONVENTION
pcre_fullinfo(const pcre *argument_re, const pcre_extra *extra_data,
  int what, void *where)
#elif defined COMPILE_PCRE16
PCRE_EXP_DEFN int PCRE_CALL_CONVENTION
pcre16_fullinfo(const pcre16 *argument_re, const pcre16_extra *extra_data,
  int what, void *where)
#elif defined COMPILE_PCRE32
PCRE_EXP_DEFN int PCRE_CALL_CONVENTION
pcre32_fullinfo(const pcre32 *argument_re, const pcre32_extra *extra_data,
  int what, void *where)
#endif
{
const REAL_PCRE *re = (const REAL_PCRE *)argument_re;
const pcre_study_data *study = NULL;

if (re == NULL || where == NULL) return PCRE_ERROR_NULL;

if (extra_data != NULL && (extra_data->flags & PCRE_EXTRA_STUDY_DATA) != 0)
  study = (const pcre_study_data *)extra_data->study_data;

/* Check that the first field in the block is the magic number. If it is not,
return with PCRE_ERROR_BADMAGIC. However, if the magic number is equal to
REVERSED_MAGIC_NUMBER we return with PCRE_ERROR_BADENDIANNESS, which
means that the pattern is likely compiled with different endianness. */

if (re->magic_number != MAGIC_NUMBER)
  return re->magic_number == REVERSED_MAGIC_NUMBER?
    PCRE_ERROR_BADENDIANNESS:PCRE_ERROR_BADMAGIC;

/* Check that this pattern was compiled in the correct bit mode */

if ((re->flags & PCRE_MODE) == 0) return PCRE_ERROR_BADMODE;

switch (what)
  {
  case PCRE_INFO_OPTIONS:
  *((unsigned long int *)where) = re->options & PUBLIC_COMPILE_OPTIONS;
  break;

  case PCRE_INFO_SIZE:
  *((size_t *)where) = re->size;
  break;

  case PCRE_INFO_STUDYSIZE:
  *((size_t *)where) = (study == NULL)? 0 : study->size;
  break;

  case PCRE_INFO_JITSIZE:
#ifdef SUPPORT_JIT
  *((size_t *)where) =
      (extra_data != NULL &&
      (extra_data->flags & PCRE_EXTRA_EXECUTABLE_JIT) != 0 &&
      extra_data->executable_jit != NULL)?
    PRIV(jit_get_size)(extra_data->executable_jit) : 0;
#else
  *((size_t *)where) = 0;
#endif
  break;

  case PCRE_INFO_CAPTURECOUNT:
  *((int *)where) = re->top_bracket;
  break;

  case PCRE_INFO_BACKREFMAX:
  *((int *)where) = re->top_backref;
  break;

  case PCRE_INFO_FIRSTBYTE:
  *((int *)where) =
    ((re->flags & PCRE_FIRSTSET) != 0)? (int)re->first_char :
    ((re->flags & PCRE_STARTLINE) != 0)? -1 : -2;
  break;

  case PCRE_INFO_FIRSTCHARACTER:
    *((pcre_uint32 *)where) =
      (re->flags & PCRE_FIRSTSET) != 0 ? re->first_char : 0;
    break;

  case PCRE_INFO_FIRSTCHARACTERFLAGS:
    *((int *)where) =
      ((re->flags & PCRE_FIRSTSET) != 0) ? 1 :
      ((re->flags & PCRE_STARTLINE) != 0) ? 2 : 0;
    break;

  /* Make sure we pass back the pointer to the bit vector in the external
  block, not the internal copy (with flipped integer fields). */

  case PCRE_INFO_FIRSTTABLE:
  *((const pcre_uint8 **)where) =
    (study != NULL && (study->flags & PCRE_STUDY_MAPPED) != 0)?
      ((const pcre_study_data *)extra_data->study_data)->start_bits : NULL;
  break;

  case PCRE_INFO_MINLENGTH:
  *((int *)where) =
    (study != NULL && (study->flags & PCRE_STUDY_MINLEN) != 0)?
      (int)(study->minlength) : -1;
  break;

  case PCRE_INFO_JIT:
  *((int *)where) = extra_data != NULL &&
                    (extra_data->flags & PCRE_EXTRA_EXECUTABLE_JIT) != 0 &&
                    extra_data->executable_jit != NULL;
  break;

  case PCRE_INFO_LASTLITERAL:
  *((int *)where) =
    ((re->flags & PCRE_REQCHSET) != 0)? (int)re->req_char : -1;
  break;

  case PCRE_INFO_REQUIREDCHAR:
    *((pcre_uint32 *)where) =
      ((re->flags & PCRE_REQCHSET) != 0) ? re->req_char : 0;
    break;

  case PCRE_INFO_REQUIREDCHARFLAGS:
    *((int *)where) =
      ((re->flags & PCRE_REQCHSET) != 0);
    break;

  case PCRE_INFO_NAMEENTRYSIZE:
  *((int *)where) = re->name_entry_size;
  break;

  case PCRE_INFO_NAMECOUNT:
  *((int *)where) = re->name_count;
  break;

  case PCRE_INFO_NAMETABLE:
  *((const pcre_uchar **)where) = (const pcre_uchar *)re + re->name_table_offset;
  break;

  case PCRE_INFO_DEFAULT_TABLES:
  *((const pcre_uint8 **)where) = (const pcre_uint8 *)(PRIV(default_tables));
  break;

  /* From release 8.00 this will always return TRUE because NOPARTIAL is
  no longer ever set (the restrictions have been removed). */

  case PCRE_INFO_OKPARTIAL:
  *((int *)where) = (re->flags & PCRE_NOPARTIAL) == 0;
  break;

  case PCRE_INFO_JCHANGED:
  *((int *)where) = (re->flags & PCRE_JCHANGED) != 0;
  break;

  case PCRE_INFO_HASCRORLF:
  *((int *)where) = (re->flags & PCRE_HASCRORLF) != 0;
  break;

  case PCRE_INFO_MAXLOOKBEHIND:
  *((int *)where) = re->max_lookbehind;
  break;

  case PCRE_INFO_MATCHLIMIT:
  if ((re->flags & PCRE_MLSET) == 0) return PCRE_ERROR_UNSET;
  *((pcre_uint32 *)where) = re->limit_match;
  break;

  case PCRE_INFO_RECURSIONLIMIT:
  if ((re->flags & PCRE_RLSET) == 0) return PCRE_ERROR_UNSET;
  *((pcre_uint32 *)where) = re->limit_recursion;
  break;

  case PCRE_INFO_MATCH_EMPTY:
  *((int *)where) = (re->flags & PCRE_MATCH_EMPTY) != 0;
  break;

  default: return PCRE_ERROR_BADOPTION;
  }

return 0;
}

/* End of pcre_fullinfo.c */

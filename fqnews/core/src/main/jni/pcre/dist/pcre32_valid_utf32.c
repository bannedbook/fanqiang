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


/* This module contains an internal function for validating UTF-32 character
strings. */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Generate code with 32 bit character support. */
#define COMPILE_PCRE32

#include "pcre_internal.h"

/*************************************************
*         Validate a UTF-32 string                *
*************************************************/

/* This function is called (optionally) at the start of compile or match, to
check that a supposed UTF-32 string is actually valid. The early check means
that subsequent code can assume it is dealing with a valid string. The check
can be turned off for maximum performance, but the consequences of supplying an
invalid string are then undefined.

More information about the details of the error are passed
back in the returned value:

PCRE_UTF32_ERR0  No error
PCRE_UTF32_ERR1  Surrogate character
PCRE_UTF32_ERR2  Unused (was non-character)
PCRE_UTF32_ERR3  Character > 0x10ffff

Arguments:
  string       points to the string
  length       length of string, or -1 if the string is zero-terminated
  errp         pointer to an error position offset variable

Returns:       = 0    if the string is a valid UTF-32 string
               > 0    otherwise, setting the offset of the bad character
*/

int
PRIV(valid_utf)(PCRE_PUCHAR string, int length, int *erroroffset)
{
#ifdef SUPPORT_UTF
register PCRE_PUCHAR p;
register pcre_uchar c;

if (length < 0)
  {
  for (p = string; *p != 0; p++);
  length = p - string;
  }

for (p = string; length-- > 0; p++)
  {
  c = *p;

  if ((c & 0xfffff800u) != 0xd800u)
    {
    /* Normal UTF-32 code point. Neither high nor low surrogate. */
    if (c > 0x10ffffu)
      {
      *erroroffset = p - string;
      return PCRE_UTF32_ERR3;
      }
    }
  else
    {
    /* A surrogate */
    *erroroffset = p - string;
    return PCRE_UTF32_ERR1;
    }
  }

#else  /* SUPPORT_UTF */
(void)(string);  /* Keep picky compilers happy */
(void)(length);
(void)(erroroffset);
#endif /* SUPPORT_UTF */

return PCRE_UTF32_ERR0;   /* This indicates success */
}

/* End of pcre32_valid_utf32.c */

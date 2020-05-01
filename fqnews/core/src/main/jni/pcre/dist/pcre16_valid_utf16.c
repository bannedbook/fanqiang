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


/* This module contains an internal function for validating UTF-16 character
strings. */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Generate code with 16 bit character support. */
#define COMPILE_PCRE16

#include "pcre_internal.h"


/*************************************************
*         Validate a UTF-16 string                *
*************************************************/

/* This function is called (optionally) at the start of compile or match, to
check that a supposed UTF-16 string is actually valid. The early check means
that subsequent code can assume it is dealing with a valid string. The check
can be turned off for maximum performance, but the consequences of supplying an
invalid string are then undefined.

From release 8.21 more information about the details of the error are passed
back in the returned value:

PCRE_UTF16_ERR0  No error
PCRE_UTF16_ERR1  Missing low surrogate at the end of the string
PCRE_UTF16_ERR2  Invalid low surrogate
PCRE_UTF16_ERR3  Isolated low surrogate
PCRE_UTF16_ERR4  Unused (was non-character)

Arguments:
  string       points to the string
  length       length of string, or -1 if the string is zero-terminated
  errp         pointer to an error position offset variable

Returns:       = 0    if the string is a valid UTF-16 string
               > 0    otherwise, setting the offset of the bad character
*/

int
PRIV(valid_utf)(PCRE_PUCHAR string, int length, int *erroroffset)
{
#ifdef SUPPORT_UTF
register PCRE_PUCHAR p;
register pcre_uint32 c;

if (length < 0)
  {
  for (p = string; *p != 0; p++);
  length = p - string;
  }

for (p = string; length-- > 0; p++)
  {
  c = *p;

  if ((c & 0xf800) != 0xd800)
    {
    /* Normal UTF-16 code point. Neither high nor low surrogate. */
    }
  else if ((c & 0x0400) == 0)
    {
    /* High surrogate. Must be a followed by a low surrogate. */
    if (length == 0)
      {
      *erroroffset = p - string;
      return PCRE_UTF16_ERR1;
      }
    p++;
    length--;
    if ((*p & 0xfc00) != 0xdc00)
      {
      *erroroffset = p - string;
      return PCRE_UTF16_ERR2;
      }
    }
  else
    {
    /* Isolated low surrogate. Always an error. */
    *erroroffset = p - string;
    return PCRE_UTF16_ERR3;
    }
  }

#else  /* SUPPORT_UTF */
(void)(string);  /* Keep picky compilers happy */
(void)(length);
(void)(erroroffset);
#endif /* SUPPORT_UTF */

return PCRE_UTF16_ERR0;   /* This indicates success */
}

/* End of pcre16_valid_utf16.c */

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


/* This module contains internal functions for comparing and finding the length
of strings for different data item sizes. */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pcre_internal.h"

#ifndef COMPILE_PCRE8

/*************************************************
*           Compare string utilities             *
*************************************************/

/* The following two functions compares two strings. Basically a strcmp
for non 8 bit characters.

Arguments:
  str1        first string
  str2        second string

Returns:      0 if both string are equal (like strcmp), 1 otherwise
*/

int
PRIV(strcmp_uc_uc)(const pcre_uchar *str1, const pcre_uchar *str2)
{
pcre_uchar c1;
pcre_uchar c2;

while (*str1 != '\0' || *str2 != '\0')
  {
  c1 = *str1++;
  c2 = *str2++;
  if (c1 != c2)
    return ((c1 > c2) << 1) - 1;
  }
/* Both length and characters must be equal. */
return 0;
}

#ifdef COMPILE_PCRE32

int
PRIV(strcmp_uc_uc_utf)(const pcre_uchar *str1, const pcre_uchar *str2)
{
pcre_uchar c1;
pcre_uchar c2;

while (*str1 != '\0' || *str2 != '\0')
  {
  c1 = UCHAR21INC(str1);
  c2 = UCHAR21INC(str2);
  if (c1 != c2)
    return ((c1 > c2) << 1) - 1;
  }
/* Both length and characters must be equal. */
return 0;
}

#endif /* COMPILE_PCRE32 */

int
PRIV(strcmp_uc_c8)(const pcre_uchar *str1, const char *str2)
{
const pcre_uint8 *ustr2 = (pcre_uint8 *)str2;
pcre_uchar c1;
pcre_uchar c2;

while (*str1 != '\0' || *ustr2 != '\0')
  {
  c1 = *str1++;
  c2 = (pcre_uchar)*ustr2++;
  if (c1 != c2)
    return ((c1 > c2) << 1) - 1;
  }
/* Both length and characters must be equal. */
return 0;
}

#ifdef COMPILE_PCRE32

int
PRIV(strcmp_uc_c8_utf)(const pcre_uchar *str1, const char *str2)
{
const pcre_uint8 *ustr2 = (pcre_uint8 *)str2;
pcre_uchar c1;
pcre_uchar c2;

while (*str1 != '\0' || *ustr2 != '\0')
  {
  c1 = UCHAR21INC(str1);
  c2 = (pcre_uchar)*ustr2++;
  if (c1 != c2)
    return ((c1 > c2) << 1) - 1;
  }
/* Both length and characters must be equal. */
return 0;
}

#endif /* COMPILE_PCRE32 */

/* The following two functions compares two, fixed length
strings. Basically an strncmp for non 8 bit characters.

Arguments:
  str1        first string
  str2        second string
  num         size of the string

Returns:      0 if both string are equal (like strcmp), 1 otherwise
*/

int
PRIV(strncmp_uc_uc)(const pcre_uchar *str1, const pcre_uchar *str2, unsigned int num)
{
pcre_uchar c1;
pcre_uchar c2;

while (num-- > 0)
  {
  c1 = *str1++;
  c2 = *str2++;
  if (c1 != c2)
    return ((c1 > c2) << 1) - 1;
  }
/* Both length and characters must be equal. */
return 0;
}

int
PRIV(strncmp_uc_c8)(const pcre_uchar *str1, const char *str2, unsigned int num)
{
const pcre_uint8 *ustr2 = (pcre_uint8 *)str2;
pcre_uchar c1;
pcre_uchar c2;

while (num-- > 0)
  {
  c1 = *str1++;
  c2 = (pcre_uchar)*ustr2++;
  if (c1 != c2)
    return ((c1 > c2) << 1) - 1;
  }
/* Both length and characters must be equal. */
return 0;
}

/* The following function returns with the length of
a zero terminated string. Basically an strlen for non 8 bit characters.

Arguments:
  str         string

Returns:      length of the string
*/

unsigned int
PRIV(strlen_uc)(const pcre_uchar *str)
{
unsigned int len = 0;
while (*str++ != 0)
  len++;
return len;
}

#endif /* !COMPILE_PCRE8 */

/* End of pcre_string_utils.c */

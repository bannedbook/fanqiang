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


/* This module contains the external function pcre_refcount(), which is an
auxiliary function that can be used to maintain a reference count in a compiled
pattern data block. This might be helpful in applications where the block is
shared by different users. */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pcre_internal.h"


/*************************************************
*           Maintain reference count             *
*************************************************/

/* The reference count is a 16-bit field, initialized to zero. It is not
possible to transfer a non-zero count from one host to a different host that
has a different byte order - though I can't see why anyone in their right mind
would ever want to do that!

Arguments:
  argument_re   points to compiled code
  adjust        value to add to the count

Returns:        the (possibly updated) count value (a non-negative number), or
                a negative error number
*/

#if defined COMPILE_PCRE8
PCRE_EXP_DEFN int PCRE_CALL_CONVENTION
pcre_refcount(pcre *argument_re, int adjust)
#elif defined COMPILE_PCRE16
PCRE_EXP_DEFN int PCRE_CALL_CONVENTION
pcre16_refcount(pcre16 *argument_re, int adjust)
#elif defined COMPILE_PCRE32
PCRE_EXP_DEFN int PCRE_CALL_CONVENTION
pcre32_refcount(pcre32 *argument_re, int adjust)
#endif
{
REAL_PCRE *re = (REAL_PCRE *)argument_re;
if (re == NULL) return PCRE_ERROR_NULL;
if (re->magic_number != MAGIC_NUMBER) return PCRE_ERROR_BADMAGIC;
if ((re->flags & PCRE_MODE) == 0) return PCRE_ERROR_BADMODE;
re->ref_count = (-adjust > re->ref_count)? 0 :
                (adjust + re->ref_count > 65535)? 65535 :
                re->ref_count + adjust;
return re->ref_count;
}

/* End of pcre_refcount.c */

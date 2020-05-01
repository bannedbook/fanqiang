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


/* This module contains the external function pcre_version(), which returns a
string that identifies the PCRE version that is in use. */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pcre_internal.h"


/*************************************************
*          Return version string                 *
*************************************************/

/* These macros are the standard way of turning unquoted text into C strings.
They allow macros like PCRE_MAJOR to be defined without quotes, which is
convenient for user programs that want to test its value. */

#define STRING(a)  # a
#define XSTRING(s) STRING(s)

/* A problem turned up with PCRE_PRERELEASE, which is defined empty for
production releases. Originally, it was used naively in this code:

  return XSTRING(PCRE_MAJOR)
         "." XSTRING(PCRE_MINOR)
             XSTRING(PCRE_PRERELEASE)
         " " XSTRING(PCRE_DATE);

However, when PCRE_PRERELEASE is empty, this leads to an attempted expansion of
STRING(). The C standard states: "If (before argument substitution) any
argument consists of no preprocessing tokens, the behavior is undefined." It
turns out the gcc treats this case as a single empty string - which is what we
really want - but Visual C grumbles about the lack of an argument for the
macro. Unfortunately, both are within their rights. To cope with both ways of
handling this, I had resort to some messy hackery that does a test at run time.
I could find no way of detecting that a macro is defined as an empty string at
pre-processor time. This hack uses a standard trick for avoiding calling
the STRING macro with an empty argument when doing the test. */

#if defined COMPILE_PCRE8
PCRE_EXP_DEFN const char * PCRE_CALL_CONVENTION
pcre_version(void)
#elif defined COMPILE_PCRE16
PCRE_EXP_DEFN const char * PCRE_CALL_CONVENTION
pcre16_version(void)
#elif defined COMPILE_PCRE32
PCRE_EXP_DEFN const char * PCRE_CALL_CONVENTION
pcre32_version(void)
#endif
{
return (XSTRING(Z PCRE_PRERELEASE)[1] == 0)?
  XSTRING(PCRE_MAJOR.PCRE_MINOR PCRE_DATE) :
  XSTRING(PCRE_MAJOR.PCRE_MINOR) XSTRING(PCRE_PRERELEASE PCRE_DATE);
}

/* End of pcre_version.c */

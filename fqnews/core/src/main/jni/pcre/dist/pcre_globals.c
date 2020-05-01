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


/* This module contains global variables that are exported by the PCRE library.
PCRE is thread-clean and doesn't use any global variables in the normal sense.
However, it calls memory allocation and freeing functions via the four
indirections below, and it can optionally do callouts, using the fifth
indirection. These values can be changed by the caller, but are shared between
all threads.

For MS Visual Studio and Symbian OS, there are problems in initializing these
variables to non-local functions. In these cases, therefore, an indirection via
a local function is used.

Also, when compiling for Virtual Pascal, things are done differently, and
global variables are not used. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pcre_internal.h"

#if defined _MSC_VER || defined  __SYMBIAN32__
static void* LocalPcreMalloc(size_t aSize)
  {
  return malloc(aSize);
  }
static void LocalPcreFree(void* aPtr)
  {
  free(aPtr);
  }
PCRE_EXP_DATA_DEFN void *(*PUBL(malloc))(size_t) = LocalPcreMalloc;
PCRE_EXP_DATA_DEFN void  (*PUBL(free))(void *) = LocalPcreFree;
PCRE_EXP_DATA_DEFN void *(*PUBL(stack_malloc))(size_t) = LocalPcreMalloc;
PCRE_EXP_DATA_DEFN void  (*PUBL(stack_free))(void *) = LocalPcreFree;
PCRE_EXP_DATA_DEFN int   (*PUBL(callout))(PUBL(callout_block) *) = NULL;
PCRE_EXP_DATA_DEFN int   (*PUBL(stack_guard))(void) = NULL;

#elif !defined VPCOMPAT
PCRE_EXP_DATA_DEFN void *(*PUBL(malloc))(size_t) = malloc;
PCRE_EXP_DATA_DEFN void  (*PUBL(free))(void *) = free;
PCRE_EXP_DATA_DEFN void *(*PUBL(stack_malloc))(size_t) = malloc;
PCRE_EXP_DATA_DEFN void  (*PUBL(stack_free))(void *) = free;
PCRE_EXP_DATA_DEFN int   (*PUBL(callout))(PUBL(callout_block) *) = NULL;
PCRE_EXP_DATA_DEFN int   (*PUBL(stack_guard))(void) = NULL;
#endif

/* End of pcre_globals.c */

/*************************************************
*             PCRE testing program               *
*************************************************/

/* This program was hacked up as a tester for PCRE. I really should have
written it more tidily in the first place. Will I ever learn? It has grown and
been extended and consequently is now rather, er, *very* untidy in places. The
addition of 16-bit support has made it even worse. :-(

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

/* This program now supports the testing of all of the 8-bit, 16-bit, and
32-bit PCRE libraries in a single program. This is different from the modules
such as pcre_compile.c in the library itself, which are compiled separately for
each mode. If two modes are enabled, for example, pcre_compile.c is compiled
twice. By contrast, pcretest.c is compiled only once. Therefore, it must not
make use of any of the macros from pcre_internal.h that depend on
COMPILE_PCRE8, COMPILE_PCRE16, or COMPILE_PCRE32. It does, however, make use of
SUPPORT_PCRE8, SUPPORT_PCRE16, and SUPPORT_PCRE32 to ensure that it calls only
supported library functions. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <errno.h>

/* Both libreadline and libedit are optionally supported. The user-supplied
original patch uses readline/readline.h for libedit, but in at least one system
it is installed as editline/readline.h, so the configuration code now looks for
that first, falling back to readline/readline.h. */

#if defined(SUPPORT_LIBREADLINE) || defined(SUPPORT_LIBEDIT)
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#if defined(SUPPORT_LIBREADLINE)
#include <readline/readline.h>
#include <readline/history.h>
#else
#if defined(HAVE_EDITLINE_READLINE_H)
#include <editline/readline.h>
#else
#include <readline/readline.h>
#endif
#endif
#endif

/* A number of things vary for Windows builds. Originally, pcretest opened its
input and output without "b"; then I was told that "b" was needed in some
environments, so it was added for release 5.0 to both the input and output. (It
makes no difference on Unix-like systems.) Later I was told that it is wrong
for the input on Windows. I've now abstracted the modes into two macros that
are set here, to make it easier to fiddle with them, and removed "b" from the
input mode under Windows. */

#if defined(_WIN32) || defined(WIN32)
#include <io.h>                /* For _setmode() */
#include <fcntl.h>             /* For _O_BINARY */
#define INPUT_MODE   "r"
#define OUTPUT_MODE  "wb"

#ifndef isatty
#define isatty _isatty         /* This is what Windows calls them, I'm told, */
#endif                         /* though in some environments they seem to   */
                               /* be already defined, hence the #ifndefs.    */
#ifndef fileno
#define fileno _fileno
#endif

/* A user sent this fix for Borland Builder 5 under Windows. */

#ifdef __BORLANDC__
#define _setmode(handle, mode) setmode(handle, mode)
#endif

/* Not Windows */

#else
#include <sys/time.h>          /* These two includes are needed */
#include <sys/resource.h>      /* for setrlimit(). */
#if defined NATIVE_ZOS         /* z/OS uses non-binary I/O */
#define INPUT_MODE   "r"
#define OUTPUT_MODE  "w"
#else
#define INPUT_MODE   "rb"
#define OUTPUT_MODE  "wb"
#endif
#endif

#ifdef __VMS
#include <ssdef.h>
void vms_setsymbol( char *, char *, int );
#endif


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

/* The pcre_printint() function, which prints the internal form of a compiled
regex, is held in a separate file so that (a) it can be compiled in either
8-, 16- or 32-bit mode, and (b) it can be #included directly in pcre_compile.c
when that is compiled in debug mode. */

#ifdef SUPPORT_PCRE8
void pcre_printint(pcre *external_re, FILE *f, BOOL print_lengths);
#endif
#ifdef SUPPORT_PCRE16
void pcre16_printint(pcre *external_re, FILE *f, BOOL print_lengths);
#endif
#ifdef SUPPORT_PCRE32
void pcre32_printint(pcre *external_re, FILE *f, BOOL print_lengths);
#endif

/* We need access to some of the data tables that PCRE uses. So as not to have
to keep two copies, we include the source files here, changing the names of the
external symbols to prevent clashes. */

#define PCRE_INCLUDED

#include "pcre_tables.c"
#include "pcre_ucd.c"

/* The definition of the macro PRINTABLE, which determines whether to print an
output character as-is or as a hex value when showing compiled patterns, is
the same as in the printint.src file. We uses it here in cases when the locale
has not been explicitly changed, so as to get consistent output from systems
that differ in their output from isprint() even in the "C" locale. */

#ifdef EBCDIC
#define PRINTABLE(c) ((c) >= 64 && (c) < 255)
#else
#define PRINTABLE(c) ((c) >= 32 && (c) < 127)
#endif

#define PRINTOK(c) (locale_set? isprint(c) : PRINTABLE(c))

/* Posix support is disabled in 16 or 32 bit only mode. */
#if !defined SUPPORT_PCRE8 && !defined NOPOSIX
#define NOPOSIX
#endif

/* It is possible to compile this test program without including support for
testing the POSIX interface, though this is not available via the standard
Makefile. */

#if !defined NOPOSIX
#include "pcreposix.h"
#endif

/* It is also possible, originally for the benefit of a version that was
imported into Exim, to build pcretest without support for UTF8 or UTF16 (define
NOUTF), without the interface to the DFA matcher (NODFA). In fact, we
automatically cut out the UTF support if PCRE is built without it. */

#ifndef SUPPORT_UTF
#ifndef NOUTF
#define NOUTF
#endif
#endif

/* To make the code a bit tidier for 8/16/32-bit support, we define macros
for all the pcre[16]_xxx functions (except pcre16_fullinfo, which is called
only from one place and is handled differently). I couldn't dream up any way of
using a single macro to do this in a generic way, because of the many different
argument requirements. We know that at least one of SUPPORT_PCRE8 and
SUPPORT_PCRE16 must be set. First define macros for each individual mode; then
use these in the definitions of generic macros.

**** Special note about the PCHARSxxx macros: the address of the string to be
printed is always given as two arguments: a base address followed by an offset.
The base address is cast to the correct data size for 8 or 16 bit data; the
offset is in units of this size. If the string were given as base+offset in one
argument, the casting might be incorrectly applied. */

#ifdef SUPPORT_PCRE8

#define PCHARS8(lv, p, offset, len, f) \
  lv = pchars((pcre_uint8 *)(p) + offset, len, f)

#define PCHARSV8(p, offset, len, f) \
  (void)pchars((pcre_uint8 *)(p) + offset, len, f)

#define READ_CAPTURE_NAME8(p, cn8, cn16, cn32, re) \
  p = read_capture_name8(p, cn8, re)

#define STRLEN8(p) ((int)strlen((char *)p))

#define SET_PCRE_CALLOUT8(callout) \
  pcre_callout = callout

#define SET_PCRE_STACK_GUARD8(stack_guard) \
  pcre_stack_guard = stack_guard

#define PCRE_ASSIGN_JIT_STACK8(extra, callback, userdata) \
   pcre_assign_jit_stack(extra, callback, userdata)

#define PCRE_COMPILE8(re, pat, options, error, erroffset, tables) \
  re = pcre_compile((char *)pat, options, error, erroffset, tables)

#define PCRE_COPY_NAMED_SUBSTRING8(rc, re, bptr, offsets, count, \
    namesptr, cbuffer, size) \
  rc = pcre_copy_named_substring(re, (char *)bptr, offsets, count, \
    (char *)namesptr, cbuffer, size)

#define PCRE_COPY_SUBSTRING8(rc, bptr, offsets, count, i, cbuffer, size) \
  rc = pcre_copy_substring((char *)bptr, offsets, count, i, cbuffer, size)

#define PCRE_DFA_EXEC8(count, re, extra, bptr, len, start_offset, options, \
    offsets, size_offsets, workspace, size_workspace) \
  count = pcre_dfa_exec(re, extra, (char *)bptr, len, start_offset, options, \
    offsets, size_offsets, workspace, size_workspace)

#define PCRE_EXEC8(count, re, extra, bptr, len, start_offset, options, \
    offsets, size_offsets) \
  count = pcre_exec(re, extra, (char *)bptr, len, start_offset, options, \
    offsets, size_offsets)

#define PCRE_FREE_STUDY8(extra) \
  pcre_free_study(extra)

#define PCRE_FREE_SUBSTRING8(substring) \
  pcre_free_substring(substring)

#define PCRE_FREE_SUBSTRING_LIST8(listptr) \
  pcre_free_substring_list(listptr)

#define PCRE_GET_NAMED_SUBSTRING8(rc, re, bptr, offsets, count, \
    getnamesptr, subsptr) \
  rc = pcre_get_named_substring(re, (char *)bptr, offsets, count, \
    (char *)getnamesptr, subsptr)

#define PCRE_GET_STRINGNUMBER8(n, rc, ptr) \
  n = pcre_get_stringnumber(re, (char *)ptr)

#define PCRE_GET_SUBSTRING8(rc, bptr, offsets, count, i, subsptr) \
  rc = pcre_get_substring((char *)bptr, offsets, count, i, subsptr)

#define PCRE_GET_SUBSTRING_LIST8(rc, bptr, offsets, count, listptr) \
  rc = pcre_get_substring_list((const char *)bptr, offsets, count, listptr)

#define PCRE_PATTERN_TO_HOST_BYTE_ORDER8(rc, re, extra, tables) \
  rc = pcre_pattern_to_host_byte_order(re, extra, tables)

#define PCRE_PRINTINT8(re, outfile, debug_lengths) \
  pcre_printint(re, outfile, debug_lengths)

#define PCRE_STUDY8(extra, re, options, error) \
  extra = pcre_study(re, options, error)

#define PCRE_JIT_STACK_ALLOC8(startsize, maxsize) \
  pcre_jit_stack_alloc(startsize, maxsize)

#define PCRE_JIT_STACK_FREE8(stack) \
  pcre_jit_stack_free(stack)

#define pcre8_maketables pcre_maketables

#endif /* SUPPORT_PCRE8 */

/* -----------------------------------------------------------*/

#ifdef SUPPORT_PCRE16

#define PCHARS16(lv, p, offset, len, f) \
  lv = pchars16((PCRE_SPTR16)(p) + offset, len, f)

#define PCHARSV16(p, offset, len, f) \
  (void)pchars16((PCRE_SPTR16)(p) + offset, len, f)

#define READ_CAPTURE_NAME16(p, cn8, cn16, cn32, re) \
  p = read_capture_name16(p, cn16, re)

#define STRLEN16(p) ((int)strlen16((PCRE_SPTR16)p))

#define SET_PCRE_CALLOUT16(callout) \
  pcre16_callout = (int (*)(pcre16_callout_block *))callout

#define SET_PCRE_STACK_GUARD16(stack_guard) \
  pcre16_stack_guard = (int (*)(void))stack_guard

#define PCRE_ASSIGN_JIT_STACK16(extra, callback, userdata) \
  pcre16_assign_jit_stack((pcre16_extra *)extra, \
    (pcre16_jit_callback)callback, userdata)

#define PCRE_COMPILE16(re, pat, options, error, erroffset, tables) \
  re = (pcre *)pcre16_compile((PCRE_SPTR16)pat, options, error, erroffset, \
    tables)

#define PCRE_COPY_NAMED_SUBSTRING16(rc, re, bptr, offsets, count, \
    namesptr, cbuffer, size) \
  rc = pcre16_copy_named_substring((pcre16 *)re, (PCRE_SPTR16)bptr, offsets, \
    count, (PCRE_SPTR16)namesptr, (PCRE_UCHAR16 *)cbuffer, size/2)

#define PCRE_COPY_SUBSTRING16(rc, bptr, offsets, count, i, cbuffer, size) \
  rc = pcre16_copy_substring((PCRE_SPTR16)bptr, offsets, count, i, \
    (PCRE_UCHAR16 *)cbuffer, size/2)

#define PCRE_DFA_EXEC16(count, re, extra, bptr, len, start_offset, options, \
    offsets, size_offsets, workspace, size_workspace) \
  count = pcre16_dfa_exec((pcre16 *)re, (pcre16_extra *)extra, \
    (PCRE_SPTR16)bptr, len, start_offset, options, offsets, size_offsets, \
    workspace, size_workspace)

#define PCRE_EXEC16(count, re, extra, bptr, len, start_offset, options, \
    offsets, size_offsets) \
  count = pcre16_exec((pcre16 *)re, (pcre16_extra *)extra, (PCRE_SPTR16)bptr, \
    len, start_offset, options, offsets, size_offsets)

#define PCRE_FREE_STUDY16(extra) \
  pcre16_free_study((pcre16_extra *)extra)

#define PCRE_FREE_SUBSTRING16(substring) \
  pcre16_free_substring((PCRE_SPTR16)substring)

#define PCRE_FREE_SUBSTRING_LIST16(listptr) \
  pcre16_free_substring_list((PCRE_SPTR16 *)listptr)

#define PCRE_GET_NAMED_SUBSTRING16(rc, re, bptr, offsets, count, \
    getnamesptr, subsptr) \
  rc = pcre16_get_named_substring((pcre16 *)re, (PCRE_SPTR16)bptr, offsets, \
    count, (PCRE_SPTR16)getnamesptr, (PCRE_SPTR16 *)(void*)subsptr)

#define PCRE_GET_STRINGNUMBER16(n, rc, ptr) \
  n = pcre16_get_stringnumber(re, (PCRE_SPTR16)ptr)

#define PCRE_GET_SUBSTRING16(rc, bptr, offsets, count, i, subsptr) \
  rc = pcre16_get_substring((PCRE_SPTR16)bptr, offsets, count, i, \
    (PCRE_SPTR16 *)(void*)subsptr)

#define PCRE_GET_SUBSTRING_LIST16(rc, bptr, offsets, count, listptr) \
  rc = pcre16_get_substring_list((PCRE_SPTR16)bptr, offsets, count, \
    (PCRE_SPTR16 **)(void*)listptr)

#define PCRE_PATTERN_TO_HOST_BYTE_ORDER16(rc, re, extra, tables) \
  rc = pcre16_pattern_to_host_byte_order((pcre16 *)re, (pcre16_extra *)extra, \
    tables)

#define PCRE_PRINTINT16(re, outfile, debug_lengths) \
  pcre16_printint(re, outfile, debug_lengths)

#define PCRE_STUDY16(extra, re, options, error) \
  extra = (pcre_extra *)pcre16_study((pcre16 *)re, options, error)

#define PCRE_JIT_STACK_ALLOC16(startsize, maxsize) \
  (pcre_jit_stack *)pcre16_jit_stack_alloc(startsize, maxsize)

#define PCRE_JIT_STACK_FREE16(stack) \
  pcre16_jit_stack_free((pcre16_jit_stack *)stack)

#endif /* SUPPORT_PCRE16 */

/* -----------------------------------------------------------*/

#ifdef SUPPORT_PCRE32

#define PCHARS32(lv, p, offset, len, f) \
  lv = pchars32((PCRE_SPTR32)(p) + offset, len, use_utf, f)

#define PCHARSV32(p, offset, len, f)                \
  (void)pchars32((PCRE_SPTR32)(p) + offset, len, use_utf, f)

#define READ_CAPTURE_NAME32(p, cn8, cn16, cn32, re) \
  p = read_capture_name32(p, cn32, re)

#define STRLEN32(p) ((int)strlen32((PCRE_SPTR32)p))

#define SET_PCRE_CALLOUT32(callout) \
  pcre32_callout = (int (*)(pcre32_callout_block *))callout

#define SET_PCRE_STACK_GUARD32(stack_guard) \
  pcre32_stack_guard = (int (*)(void))stack_guard

#define PCRE_ASSIGN_JIT_STACK32(extra, callback, userdata) \
  pcre32_assign_jit_stack((pcre32_extra *)extra, \
    (pcre32_jit_callback)callback, userdata)

#define PCRE_COMPILE32(re, pat, options, error, erroffset, tables) \
  re = (pcre *)pcre32_compile((PCRE_SPTR32)pat, options, error, erroffset, \
    tables)

#define PCRE_COPY_NAMED_SUBSTRING32(rc, re, bptr, offsets, count, \
    namesptr, cbuffer, size) \
  rc = pcre32_copy_named_substring((pcre32 *)re, (PCRE_SPTR32)bptr, offsets, \
    count, (PCRE_SPTR32)namesptr, (PCRE_UCHAR32 *)cbuffer, size/2)

#define PCRE_COPY_SUBSTRING32(rc, bptr, offsets, count, i, cbuffer, size) \
  rc = pcre32_copy_substring((PCRE_SPTR32)bptr, offsets, count, i, \
    (PCRE_UCHAR32 *)cbuffer, size/2)

#define PCRE_DFA_EXEC32(count, re, extra, bptr, len, start_offset, options, \
    offsets, size_offsets, workspace, size_workspace) \
  count = pcre32_dfa_exec((pcre32 *)re, (pcre32_extra *)extra, \
    (PCRE_SPTR32)bptr, len, start_offset, options, offsets, size_offsets, \
    workspace, size_workspace)

#define PCRE_EXEC32(count, re, extra, bptr, len, start_offset, options, \
    offsets, size_offsets) \
  count = pcre32_exec((pcre32 *)re, (pcre32_extra *)extra, (PCRE_SPTR32)bptr, \
    len, start_offset, options, offsets, size_offsets)

#define PCRE_FREE_STUDY32(extra) \
  pcre32_free_study((pcre32_extra *)extra)

#define PCRE_FREE_SUBSTRING32(substring) \
  pcre32_free_substring((PCRE_SPTR32)substring)

#define PCRE_FREE_SUBSTRING_LIST32(listptr) \
  pcre32_free_substring_list((PCRE_SPTR32 *)listptr)

#define PCRE_GET_NAMED_SUBSTRING32(rc, re, bptr, offsets, count, \
    getnamesptr, subsptr) \
  rc = pcre32_get_named_substring((pcre32 *)re, (PCRE_SPTR32)bptr, offsets, \
    count, (PCRE_SPTR32)getnamesptr, (PCRE_SPTR32 *)(void*)subsptr)

#define PCRE_GET_STRINGNUMBER32(n, rc, ptr) \
  n = pcre32_get_stringnumber(re, (PCRE_SPTR32)ptr)

#define PCRE_GET_SUBSTRING32(rc, bptr, offsets, count, i, subsptr) \
  rc = pcre32_get_substring((PCRE_SPTR32)bptr, offsets, count, i, \
    (PCRE_SPTR32 *)(void*)subsptr)

#define PCRE_GET_SUBSTRING_LIST32(rc, bptr, offsets, count, listptr) \
  rc = pcre32_get_substring_list((PCRE_SPTR32)bptr, offsets, count, \
    (PCRE_SPTR32 **)(void*)listptr)

#define PCRE_PATTERN_TO_HOST_BYTE_ORDER32(rc, re, extra, tables) \
  rc = pcre32_pattern_to_host_byte_order((pcre32 *)re, (pcre32_extra *)extra, \
    tables)

#define PCRE_PRINTINT32(re, outfile, debug_lengths) \
  pcre32_printint(re, outfile, debug_lengths)

#define PCRE_STUDY32(extra, re, options, error) \
  extra = (pcre_extra *)pcre32_study((pcre32 *)re, options, error)

#define PCRE_JIT_STACK_ALLOC32(startsize, maxsize) \
  (pcre_jit_stack *)pcre32_jit_stack_alloc(startsize, maxsize)

#define PCRE_JIT_STACK_FREE32(stack) \
  pcre32_jit_stack_free((pcre32_jit_stack *)stack)

#endif /* SUPPORT_PCRE32 */


/* ----- More than one mode is supported; a runtime test is needed, except for
pcre_config(), and the JIT stack functions, when it doesn't matter which
available version is called. ----- */

enum {
  PCRE8_MODE,
  PCRE16_MODE,
  PCRE32_MODE
};

#if (defined (SUPPORT_PCRE8) + defined (SUPPORT_PCRE16) + \
     defined (SUPPORT_PCRE32)) >= 2

#define CHAR_SIZE (1 << pcre_mode)

/* There doesn't seem to be an easy way of writing these macros that can cope
with the 3 pairs of bit sizes plus all three bit sizes. So just handle all the
cases separately. */

/* ----- All three modes supported ----- */

#if defined(SUPPORT_PCRE8) && defined(SUPPORT_PCRE16) && defined(SUPPORT_PCRE32)

#define PCHARS(lv, p, offset, len, f) \
  if (pcre_mode == PCRE32_MODE) \
    PCHARS32(lv, p, offset, len, f); \
  else if (pcre_mode == PCRE16_MODE) \
    PCHARS16(lv, p, offset, len, f); \
  else \
    PCHARS8(lv, p, offset, len, f)

#define PCHARSV(p, offset, len, f) \
  if (pcre_mode == PCRE32_MODE) \
    PCHARSV32(p, offset, len, f); \
  else if (pcre_mode == PCRE16_MODE) \
    PCHARSV16(p, offset, len, f); \
  else \
    PCHARSV8(p, offset, len, f)

#define READ_CAPTURE_NAME(p, cn8, cn16, cn32, re) \
  if (pcre_mode == PCRE32_MODE) \
    READ_CAPTURE_NAME32(p, cn8, cn16, cn32, re); \
  else if (pcre_mode == PCRE16_MODE) \
    READ_CAPTURE_NAME16(p, cn8, cn16, cn32, re); \
  else \
    READ_CAPTURE_NAME8(p, cn8, cn16, cn32, re)

#define SET_PCRE_CALLOUT(callout) \
  if (pcre_mode == PCRE32_MODE) \
    SET_PCRE_CALLOUT32(callout); \
  else if (pcre_mode == PCRE16_MODE) \
    SET_PCRE_CALLOUT16(callout); \
  else \
    SET_PCRE_CALLOUT8(callout)

#define SET_PCRE_STACK_GUARD(stack_guard) \
  if (pcre_mode == PCRE32_MODE) \
    SET_PCRE_STACK_GUARD32(stack_guard); \
  else if (pcre_mode == PCRE16_MODE) \
    SET_PCRE_STACK_GUARD16(stack_guard); \
  else \
    SET_PCRE_STACK_GUARD8(stack_guard)

#define STRLEN(p) (pcre_mode == PCRE32_MODE ? STRLEN32(p) : pcre_mode == PCRE16_MODE ? STRLEN16(p) : STRLEN8(p))

#define PCRE_ASSIGN_JIT_STACK(extra, callback, userdata) \
  if (pcre_mode == PCRE32_MODE) \
    PCRE_ASSIGN_JIT_STACK32(extra, callback, userdata); \
  else if (pcre_mode == PCRE16_MODE) \
    PCRE_ASSIGN_JIT_STACK16(extra, callback, userdata); \
  else \
    PCRE_ASSIGN_JIT_STACK8(extra, callback, userdata)

#define PCRE_COMPILE(re, pat, options, error, erroffset, tables) \
  if (pcre_mode == PCRE32_MODE) \
    PCRE_COMPILE32(re, pat, options, error, erroffset, tables); \
  else if (pcre_mode == PCRE16_MODE) \
    PCRE_COMPILE16(re, pat, options, error, erroffset, tables); \
  else \
    PCRE_COMPILE8(re, pat, options, error, erroffset, tables)

#define PCRE_CONFIG pcre_config

#define PCRE_COPY_NAMED_SUBSTRING(rc, re, bptr, offsets, count, \
    namesptr, cbuffer, size) \
  if (pcre_mode == PCRE32_MODE) \
    PCRE_COPY_NAMED_SUBSTRING32(rc, re, bptr, offsets, count, \
      namesptr, cbuffer, size); \
  else if (pcre_mode == PCRE16_MODE) \
    PCRE_COPY_NAMED_SUBSTRING16(rc, re, bptr, offsets, count, \
      namesptr, cbuffer, size); \
  else \
    PCRE_COPY_NAMED_SUBSTRING8(rc, re, bptr, offsets, count, \
      namesptr, cbuffer, size)

#define PCRE_COPY_SUBSTRING(rc, bptr, offsets, count, i, cbuffer, size) \
  if (pcre_mode == PCRE32_MODE) \
    PCRE_COPY_SUBSTRING32(rc, bptr, offsets, count, i, cbuffer, size); \
  else if (pcre_mode == PCRE16_MODE) \
    PCRE_COPY_SUBSTRING16(rc, bptr, offsets, count, i, cbuffer, size); \
  else \
    PCRE_COPY_SUBSTRING8(rc, bptr, offsets, count, i, cbuffer, size)

#define PCRE_DFA_EXEC(count, re, extra, bptr, len, start_offset, options, \
    offsets, size_offsets, workspace, size_workspace) \
  if (pcre_mode == PCRE32_MODE) \
    PCRE_DFA_EXEC32(count, re, extra, bptr, len, start_offset, options, \
      offsets, size_offsets, workspace, size_workspace); \
  else if (pcre_mode == PCRE16_MODE) \
    PCRE_DFA_EXEC16(count, re, extra, bptr, len, start_offset, options, \
      offsets, size_offsets, workspace, size_workspace); \
  else \
    PCRE_DFA_EXEC8(count, re, extra, bptr, len, start_offset, options, \
      offsets, size_offsets, workspace, size_workspace)

#define PCRE_EXEC(count, re, extra, bptr, len, start_offset, options, \
    offsets, size_offsets) \
  if (pcre_mode == PCRE32_MODE) \
    PCRE_EXEC32(count, re, extra, bptr, len, start_offset, options, \
      offsets, size_offsets); \
  else if (pcre_mode == PCRE16_MODE) \
    PCRE_EXEC16(count, re, extra, bptr, len, start_offset, options, \
      offsets, size_offsets); \
  else \
    PCRE_EXEC8(count, re, extra, bptr, len, start_offset, options, \
      offsets, size_offsets)

#define PCRE_FREE_STUDY(extra) \
  if (pcre_mode == PCRE32_MODE) \
    PCRE_FREE_STUDY32(extra); \
  else if (pcre_mode == PCRE16_MODE) \
    PCRE_FREE_STUDY16(extra); \
  else \
    PCRE_FREE_STUDY8(extra)

#define PCRE_FREE_SUBSTRING(substring) \
  if (pcre_mode == PCRE32_MODE) \
    PCRE_FREE_SUBSTRING32(substring); \
  else if (pcre_mode == PCRE16_MODE) \
    PCRE_FREE_SUBSTRING16(substring); \
  else \
    PCRE_FREE_SUBSTRING8(substring)

#define PCRE_FREE_SUBSTRING_LIST(listptr) \
  if (pcre_mode == PCRE32_MODE) \
    PCRE_FREE_SUBSTRING_LIST32(listptr); \
  else if (pcre_mode == PCRE16_MODE) \
    PCRE_FREE_SUBSTRING_LIST16(listptr); \
  else \
    PCRE_FREE_SUBSTRING_LIST8(listptr)

#define PCRE_GET_NAMED_SUBSTRING(rc, re, bptr, offsets, count, \
    getnamesptr, subsptr) \
  if (pcre_mode == PCRE32_MODE) \
    PCRE_GET_NAMED_SUBSTRING32(rc, re, bptr, offsets, count, \
      getnamesptr, subsptr); \
  else if (pcre_mode == PCRE16_MODE) \
    PCRE_GET_NAMED_SUBSTRING16(rc, re, bptr, offsets, count, \
      getnamesptr, subsptr); \
  else \
    PCRE_GET_NAMED_SUBSTRING8(rc, re, bptr, offsets, count, \
      getnamesptr, subsptr)

#define PCRE_GET_STRINGNUMBER(n, rc, ptr) \
  if (pcre_mode == PCRE32_MODE) \
    PCRE_GET_STRINGNUMBER32(n, rc, ptr); \
  else if (pcre_mode == PCRE16_MODE) \
    PCRE_GET_STRINGNUMBER16(n, rc, ptr); \
  else \
    PCRE_GET_STRINGNUMBER8(n, rc, ptr)

#define PCRE_GET_SUBSTRING(rc, bptr, use_offsets, count, i, subsptr) \
  if (pcre_mode == PCRE32_MODE) \
    PCRE_GET_SUBSTRING32(rc, bptr, use_offsets, count, i, subsptr); \
  else if (pcre_mode == PCRE16_MODE) \
    PCRE_GET_SUBSTRING16(rc, bptr, use_offsets, count, i, subsptr); \
  else \
    PCRE_GET_SUBSTRING8(rc, bptr, use_offsets, count, i, subsptr)

#define PCRE_GET_SUBSTRING_LIST(rc, bptr, offsets, count, listptr) \
  if (pcre_mode == PCRE32_MODE) \
    PCRE_GET_SUBSTRING_LIST32(rc, bptr, offsets, count, listptr); \
  else if (pcre_mode == PCRE16_MODE) \
    PCRE_GET_SUBSTRING_LIST16(rc, bptr, offsets, count, listptr); \
  else \
    PCRE_GET_SUBSTRING_LIST8(rc, bptr, offsets, count, listptr)

#define PCRE_JIT_STACK_ALLOC(startsize, maxsize) \
  (pcre_mode == PCRE32_MODE ? \
     PCRE_JIT_STACK_ALLOC32(startsize, maxsize) \
    : pcre_mode == PCRE16_MODE ? \
      PCRE_JIT_STACK_ALLOC16(startsize, maxsize) \
      : PCRE_JIT_STACK_ALLOC8(startsize, maxsize))

#define PCRE_JIT_STACK_FREE(stack) \
  if (pcre_mode == PCRE32_MODE) \
    PCRE_JIT_STACK_FREE32(stack); \
  else if (pcre_mode == PCRE16_MODE) \
    PCRE_JIT_STACK_FREE16(stack); \
  else \
    PCRE_JIT_STACK_FREE8(stack)

#define PCRE_MAKETABLES \
  (pcre_mode == PCRE32_MODE ? pcre32_maketables() : pcre_mode == PCRE16_MODE ? pcre16_maketables() : pcre_maketables())

#define PCRE_PATTERN_TO_HOST_BYTE_ORDER(rc, re, extra, tables) \
  if (pcre_mode == PCRE32_MODE) \
    PCRE_PATTERN_TO_HOST_BYTE_ORDER32(rc, re, extra, tables); \
  else if (pcre_mode == PCRE16_MODE) \
    PCRE_PATTERN_TO_HOST_BYTE_ORDER16(rc, re, extra, tables); \
  else \
    PCRE_PATTERN_TO_HOST_BYTE_ORDER8(rc, re, extra, tables)

#define PCRE_PRINTINT(re, outfile, debug_lengths) \
  if (pcre_mode == PCRE32_MODE) \
    PCRE_PRINTINT32(re, outfile, debug_lengths); \
  else if (pcre_mode == PCRE16_MODE) \
    PCRE_PRINTINT16(re, outfile, debug_lengths); \
  else \
    PCRE_PRINTINT8(re, outfile, debug_lengths)

#define PCRE_STUDY(extra, re, options, error) \
  if (pcre_mode == PCRE32_MODE) \
    PCRE_STUDY32(extra, re, options, error); \
  else if (pcre_mode == PCRE16_MODE) \
    PCRE_STUDY16(extra, re, options, error); \
  else \
    PCRE_STUDY8(extra, re, options, error)


/* ----- Two out of three modes are supported ----- */

#else

/* We can use some macro trickery to make a single set of definitions work in
the three different cases. */

/* ----- 32-bit and 16-bit but not 8-bit supported ----- */

#if defined(SUPPORT_PCRE32) && defined(SUPPORT_PCRE16)
#define BITONE 32
#define BITTWO 16

/* ----- 32-bit and 8-bit but not 16-bit supported ----- */

#elif defined(SUPPORT_PCRE32) && defined(SUPPORT_PCRE8)
#define BITONE 32
#define BITTWO 8

/* ----- 16-bit and 8-bit but not 32-bit supported ----- */

#else
#define BITONE 16
#define BITTWO 8
#endif

#define glue(a,b) a##b
#define G(a,b) glue(a,b)


/* ----- Common macros for two-mode cases ----- */

#define PCHARS(lv, p, offset, len, f) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(PCHARS,BITONE)(lv, p, offset, len, f); \
  else \
    G(PCHARS,BITTWO)(lv, p, offset, len, f)

#define PCHARSV(p, offset, len, f) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(PCHARSV,BITONE)(p, offset, len, f); \
  else \
    G(PCHARSV,BITTWO)(p, offset, len, f)

#define READ_CAPTURE_NAME(p, cn8, cn16, cn32, re) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(READ_CAPTURE_NAME,BITONE)(p, cn8, cn16, cn32, re); \
  else \
    G(READ_CAPTURE_NAME,BITTWO)(p, cn8, cn16, cn32, re)

#define SET_PCRE_CALLOUT(callout) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(SET_PCRE_CALLOUT,BITONE)(callout); \
  else \
    G(SET_PCRE_CALLOUT,BITTWO)(callout)

#define SET_PCRE_STACK_GUARD(stack_guard) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(SET_PCRE_STACK_GUARD,BITONE)(stack_guard); \
  else \
    G(SET_PCRE_STACK_GUARD,BITTWO)(stack_guard)

#define STRLEN(p) ((pcre_mode == G(G(PCRE,BITONE),_MODE)) ? \
  G(STRLEN,BITONE)(p) : G(STRLEN,BITTWO)(p))

#define PCRE_ASSIGN_JIT_STACK(extra, callback, userdata) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(PCRE_ASSIGN_JIT_STACK,BITONE)(extra, callback, userdata); \
  else \
    G(PCRE_ASSIGN_JIT_STACK,BITTWO)(extra, callback, userdata)

#define PCRE_COMPILE(re, pat, options, error, erroffset, tables) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(PCRE_COMPILE,BITONE)(re, pat, options, error, erroffset, tables); \
  else \
    G(PCRE_COMPILE,BITTWO)(re, pat, options, error, erroffset, tables)

#define PCRE_CONFIG G(G(pcre,BITONE),_config)

#define PCRE_COPY_NAMED_SUBSTRING(rc, re, bptr, offsets, count, \
    namesptr, cbuffer, size) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(PCRE_COPY_NAMED_SUBSTRING,BITONE)(rc, re, bptr, offsets, count, \
      namesptr, cbuffer, size); \
  else \
    G(PCRE_COPY_NAMED_SUBSTRING,BITTWO)(rc, re, bptr, offsets, count, \
      namesptr, cbuffer, size)

#define PCRE_COPY_SUBSTRING(rc, bptr, offsets, count, i, cbuffer, size) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(PCRE_COPY_SUBSTRING,BITONE)(rc, bptr, offsets, count, i, cbuffer, size); \
  else \
    G(PCRE_COPY_SUBSTRING,BITTWO)(rc, bptr, offsets, count, i, cbuffer, size)

#define PCRE_DFA_EXEC(count, re, extra, bptr, len, start_offset, options, \
    offsets, size_offsets, workspace, size_workspace) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(PCRE_DFA_EXEC,BITONE)(count, re, extra, bptr, len, start_offset, options, \
      offsets, size_offsets, workspace, size_workspace); \
  else \
    G(PCRE_DFA_EXEC,BITTWO)(count, re, extra, bptr, len, start_offset, options, \
      offsets, size_offsets, workspace, size_workspace)

#define PCRE_EXEC(count, re, extra, bptr, len, start_offset, options, \
    offsets, size_offsets) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(PCRE_EXEC,BITONE)(count, re, extra, bptr, len, start_offset, options, \
      offsets, size_offsets); \
  else \
    G(PCRE_EXEC,BITTWO)(count, re, extra, bptr, len, start_offset, options, \
      offsets, size_offsets)

#define PCRE_FREE_STUDY(extra) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(PCRE_FREE_STUDY,BITONE)(extra); \
  else \
    G(PCRE_FREE_STUDY,BITTWO)(extra)

#define PCRE_FREE_SUBSTRING(substring) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(PCRE_FREE_SUBSTRING,BITONE)(substring); \
  else \
    G(PCRE_FREE_SUBSTRING,BITTWO)(substring)

#define PCRE_FREE_SUBSTRING_LIST(listptr) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(PCRE_FREE_SUBSTRING_LIST,BITONE)(listptr); \
  else \
    G(PCRE_FREE_SUBSTRING_LIST,BITTWO)(listptr)

#define PCRE_GET_NAMED_SUBSTRING(rc, re, bptr, offsets, count, \
    getnamesptr, subsptr) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(PCRE_GET_NAMED_SUBSTRING,BITONE)(rc, re, bptr, offsets, count, \
      getnamesptr, subsptr); \
  else \
    G(PCRE_GET_NAMED_SUBSTRING,BITTWO)(rc, re, bptr, offsets, count, \
      getnamesptr, subsptr)

#define PCRE_GET_STRINGNUMBER(n, rc, ptr) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(PCRE_GET_STRINGNUMBER,BITONE)(n, rc, ptr); \
  else \
    G(PCRE_GET_STRINGNUMBER,BITTWO)(n, rc, ptr)

#define PCRE_GET_SUBSTRING(rc, bptr, use_offsets, count, i, subsptr) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(PCRE_GET_SUBSTRING,BITONE)(rc, bptr, use_offsets, count, i, subsptr); \
  else \
    G(PCRE_GET_SUBSTRING,BITTWO)(rc, bptr, use_offsets, count, i, subsptr)

#define PCRE_GET_SUBSTRING_LIST(rc, bptr, offsets, count, listptr) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(PCRE_GET_SUBSTRING_LIST,BITONE)(rc, bptr, offsets, count, listptr); \
  else \
    G(PCRE_GET_SUBSTRING_LIST,BITTWO)(rc, bptr, offsets, count, listptr)

#define PCRE_JIT_STACK_ALLOC(startsize, maxsize) \
  (pcre_mode == G(G(PCRE,BITONE),_MODE)) ? \
     G(PCRE_JIT_STACK_ALLOC,BITONE)(startsize, maxsize) \
    : G(PCRE_JIT_STACK_ALLOC,BITTWO)(startsize, maxsize)

#define PCRE_JIT_STACK_FREE(stack) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(PCRE_JIT_STACK_FREE,BITONE)(stack); \
  else \
    G(PCRE_JIT_STACK_FREE,BITTWO)(stack)

#define PCRE_MAKETABLES \
  (pcre_mode == G(G(PCRE,BITONE),_MODE)) ? \
    G(G(pcre,BITONE),_maketables)() : G(G(pcre,BITTWO),_maketables)()

#define PCRE_PATTERN_TO_HOST_BYTE_ORDER(rc, re, extra, tables) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(PCRE_PATTERN_TO_HOST_BYTE_ORDER,BITONE)(rc, re, extra, tables); \
  else \
    G(PCRE_PATTERN_TO_HOST_BYTE_ORDER,BITTWO)(rc, re, extra, tables)

#define PCRE_PRINTINT(re, outfile, debug_lengths) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(PCRE_PRINTINT,BITONE)(re, outfile, debug_lengths); \
  else \
    G(PCRE_PRINTINT,BITTWO)(re, outfile, debug_lengths)

#define PCRE_STUDY(extra, re, options, error) \
  if (pcre_mode == G(G(PCRE,BITONE),_MODE)) \
    G(PCRE_STUDY,BITONE)(extra, re, options, error); \
  else \
    G(PCRE_STUDY,BITTWO)(extra, re, options, error)

#endif  /* Two out of three modes */

/* ----- End of cases where more than one mode is supported ----- */


/* ----- Only 8-bit mode is supported ----- */

#elif defined SUPPORT_PCRE8
#define CHAR_SIZE                 1
#define PCHARS                    PCHARS8
#define PCHARSV                   PCHARSV8
#define READ_CAPTURE_NAME         READ_CAPTURE_NAME8
#define SET_PCRE_CALLOUT          SET_PCRE_CALLOUT8
#define SET_PCRE_STACK_GUARD      SET_PCRE_STACK_GUARD8
#define STRLEN                    STRLEN8
#define PCRE_ASSIGN_JIT_STACK     PCRE_ASSIGN_JIT_STACK8
#define PCRE_COMPILE              PCRE_COMPILE8
#define PCRE_CONFIG               pcre_config
#define PCRE_COPY_NAMED_SUBSTRING PCRE_COPY_NAMED_SUBSTRING8
#define PCRE_COPY_SUBSTRING       PCRE_COPY_SUBSTRING8
#define PCRE_DFA_EXEC             PCRE_DFA_EXEC8
#define PCRE_EXEC                 PCRE_EXEC8
#define PCRE_FREE_STUDY           PCRE_FREE_STUDY8
#define PCRE_FREE_SUBSTRING       PCRE_FREE_SUBSTRING8
#define PCRE_FREE_SUBSTRING_LIST  PCRE_FREE_SUBSTRING_LIST8
#define PCRE_GET_NAMED_SUBSTRING  PCRE_GET_NAMED_SUBSTRING8
#define PCRE_GET_STRINGNUMBER     PCRE_GET_STRINGNUMBER8
#define PCRE_GET_SUBSTRING        PCRE_GET_SUBSTRING8
#define PCRE_GET_SUBSTRING_LIST   PCRE_GET_SUBSTRING_LIST8
#define PCRE_JIT_STACK_ALLOC      PCRE_JIT_STACK_ALLOC8
#define PCRE_JIT_STACK_FREE       PCRE_JIT_STACK_FREE8
#define PCRE_MAKETABLES           pcre_maketables()
#define PCRE_PATTERN_TO_HOST_BYTE_ORDER PCRE_PATTERN_TO_HOST_BYTE_ORDER8
#define PCRE_PRINTINT             PCRE_PRINTINT8
#define PCRE_STUDY                PCRE_STUDY8

/* ----- Only 16-bit mode is supported ----- */

#elif defined SUPPORT_PCRE16
#define CHAR_SIZE                 2
#define PCHARS                    PCHARS16
#define PCHARSV                   PCHARSV16
#define READ_CAPTURE_NAME         READ_CAPTURE_NAME16
#define SET_PCRE_CALLOUT          SET_PCRE_CALLOUT16
#define SET_PCRE_STACK_GUARD      SET_PCRE_STACK_GUARD16
#define STRLEN                    STRLEN16
#define PCRE_ASSIGN_JIT_STACK     PCRE_ASSIGN_JIT_STACK16
#define PCRE_COMPILE              PCRE_COMPILE16
#define PCRE_CONFIG               pcre16_config
#define PCRE_COPY_NAMED_SUBSTRING PCRE_COPY_NAMED_SUBSTRING16
#define PCRE_COPY_SUBSTRING       PCRE_COPY_SUBSTRING16
#define PCRE_DFA_EXEC             PCRE_DFA_EXEC16
#define PCRE_EXEC                 PCRE_EXEC16
#define PCRE_FREE_STUDY           PCRE_FREE_STUDY16
#define PCRE_FREE_SUBSTRING       PCRE_FREE_SUBSTRING16
#define PCRE_FREE_SUBSTRING_LIST  PCRE_FREE_SUBSTRING_LIST16
#define PCRE_GET_NAMED_SUBSTRING  PCRE_GET_NAMED_SUBSTRING16
#define PCRE_GET_STRINGNUMBER     PCRE_GET_STRINGNUMBER16
#define PCRE_GET_SUBSTRING        PCRE_GET_SUBSTRING16
#define PCRE_GET_SUBSTRING_LIST   PCRE_GET_SUBSTRING_LIST16
#define PCRE_JIT_STACK_ALLOC      PCRE_JIT_STACK_ALLOC16
#define PCRE_JIT_STACK_FREE       PCRE_JIT_STACK_FREE16
#define PCRE_MAKETABLES           pcre16_maketables()
#define PCRE_PATTERN_TO_HOST_BYTE_ORDER PCRE_PATTERN_TO_HOST_BYTE_ORDER16
#define PCRE_PRINTINT             PCRE_PRINTINT16
#define PCRE_STUDY                PCRE_STUDY16

/* ----- Only 32-bit mode is supported ----- */

#elif defined SUPPORT_PCRE32
#define CHAR_SIZE                 4
#define PCHARS                    PCHARS32
#define PCHARSV                   PCHARSV32
#define READ_CAPTURE_NAME         READ_CAPTURE_NAME32
#define SET_PCRE_CALLOUT          SET_PCRE_CALLOUT32
#define SET_PCRE_STACK_GUARD      SET_PCRE_STACK_GUARD32
#define STRLEN                    STRLEN32
#define PCRE_ASSIGN_JIT_STACK     PCRE_ASSIGN_JIT_STACK32
#define PCRE_COMPILE              PCRE_COMPILE32
#define PCRE_CONFIG               pcre32_config
#define PCRE_COPY_NAMED_SUBSTRING PCRE_COPY_NAMED_SUBSTRING32
#define PCRE_COPY_SUBSTRING       PCRE_COPY_SUBSTRING32
#define PCRE_DFA_EXEC             PCRE_DFA_EXEC32
#define PCRE_EXEC                 PCRE_EXEC32
#define PCRE_FREE_STUDY           PCRE_FREE_STUDY32
#define PCRE_FREE_SUBSTRING       PCRE_FREE_SUBSTRING32
#define PCRE_FREE_SUBSTRING_LIST  PCRE_FREE_SUBSTRING_LIST32
#define PCRE_GET_NAMED_SUBSTRING  PCRE_GET_NAMED_SUBSTRING32
#define PCRE_GET_STRINGNUMBER     PCRE_GET_STRINGNUMBER32
#define PCRE_GET_SUBSTRING        PCRE_GET_SUBSTRING32
#define PCRE_GET_SUBSTRING_LIST   PCRE_GET_SUBSTRING_LIST32
#define PCRE_JIT_STACK_ALLOC      PCRE_JIT_STACK_ALLOC32
#define PCRE_JIT_STACK_FREE       PCRE_JIT_STACK_FREE32
#define PCRE_MAKETABLES           pcre32_maketables()
#define PCRE_PATTERN_TO_HOST_BYTE_ORDER PCRE_PATTERN_TO_HOST_BYTE_ORDER32
#define PCRE_PRINTINT             PCRE_PRINTINT32
#define PCRE_STUDY                PCRE_STUDY32

#endif

/* ----- End of mode-specific function call macros ----- */


/* Other parameters */

#ifndef CLOCKS_PER_SEC
#ifdef CLK_TCK
#define CLOCKS_PER_SEC CLK_TCK
#else
#define CLOCKS_PER_SEC 100
#endif
#endif

#if !defined NODFA
#define DFA_WS_DIMENSION 1000
#endif

/* This is the default loop count for timing. */

#define LOOPREPEAT 500000

/* Static variables */

static FILE *outfile;
static int log_store = 0;
static int callout_count;
static int callout_extra;
static int callout_fail_count;
static int callout_fail_id;
static int debug_lengths;
static int first_callout;
static int jit_was_used;
static int locale_set = 0;
static int show_malloc;
static int stack_guard_return;
static int use_utf;
static const unsigned char *last_callout_mark = NULL;

/* The buffers grow automatically if very long input lines are encountered. */

static int buffer_size = 50000;
static pcre_uint8 *buffer = NULL;
static pcre_uint8 *pbuffer = NULL;

/* Just as a safety check, make sure that COMPILE_PCRE[16|32] are *not* set. */

#ifdef COMPILE_PCRE16
#error COMPILE_PCRE16 must not be set when compiling pcretest.c
#endif

#ifdef COMPILE_PCRE32
#error COMPILE_PCRE32 must not be set when compiling pcretest.c
#endif

/* We need buffers for building 16/32-bit strings, and the tables of operator
lengths that are used for 16/32-bit compiling, in order to swap bytes in a
pattern for saving/reloading testing. Luckily, the data for these tables is
defined as a macro. However, we must ensure that LINK_SIZE and IMM2_SIZE (which
are used in the tables) are adjusted appropriately for the 16/32-bit world.
LINK_SIZE is also used later in this program. */

#ifdef SUPPORT_PCRE16
#undef IMM2_SIZE
#define IMM2_SIZE 1

#if LINK_SIZE == 2
#undef LINK_SIZE
#define LINK_SIZE 1
#elif LINK_SIZE == 3 || LINK_SIZE == 4
#undef LINK_SIZE
#define LINK_SIZE 2
#else
#error LINK_SIZE must be either 2, 3, or 4
#endif

static int buffer16_size = 0;
static pcre_uint16 *buffer16 = NULL;
static const pcre_uint16 OP_lengths16[] = { OP_LENGTHS };
#endif  /* SUPPORT_PCRE16 */

#ifdef SUPPORT_PCRE32
#undef IMM2_SIZE
#define IMM2_SIZE 1
#undef LINK_SIZE
#define LINK_SIZE 1

static int buffer32_size = 0;
static pcre_uint32 *buffer32 = NULL;
static const pcre_uint32 OP_lengths32[] = { OP_LENGTHS };
#endif  /* SUPPORT_PCRE32 */

/* If we have 8-bit support, default to it; if there is also 16-or 32-bit
support, it can be changed by an option. If there is no 8-bit support, there
must be 16-or 32-bit support, so default it to 1. */

#if defined SUPPORT_PCRE8
static int pcre_mode = PCRE8_MODE;
#elif defined SUPPORT_PCRE16
static int pcre_mode = PCRE16_MODE;
#elif defined SUPPORT_PCRE32
static int pcre_mode = PCRE32_MODE;
#endif

/* JIT study options for -s+n and /S+n where '1' <= n <= '7'. */

static int jit_study_bits[] =
  {
  PCRE_STUDY_JIT_COMPILE,
  PCRE_STUDY_JIT_PARTIAL_SOFT_COMPILE,
  PCRE_STUDY_JIT_COMPILE + PCRE_STUDY_JIT_PARTIAL_SOFT_COMPILE,
  PCRE_STUDY_JIT_PARTIAL_HARD_COMPILE,
  PCRE_STUDY_JIT_COMPILE + PCRE_STUDY_JIT_PARTIAL_HARD_COMPILE,
  PCRE_STUDY_JIT_PARTIAL_SOFT_COMPILE + PCRE_STUDY_JIT_PARTIAL_HARD_COMPILE,
  PCRE_STUDY_JIT_COMPILE + PCRE_STUDY_JIT_PARTIAL_SOFT_COMPILE +
    PCRE_STUDY_JIT_PARTIAL_HARD_COMPILE
};

#define PCRE_STUDY_ALLJIT (PCRE_STUDY_JIT_COMPILE | \
  PCRE_STUDY_JIT_PARTIAL_SOFT_COMPILE | PCRE_STUDY_JIT_PARTIAL_HARD_COMPILE)

/* Textual explanations for runtime error codes */

static const char *errtexts[] = {
  NULL,  /* 0 is no error */
  NULL,  /* NOMATCH is handled specially */
  "NULL argument passed",
  "bad option value",
  "magic number missing",
  "unknown opcode - pattern overwritten?",
  "no more memory",
  NULL,  /* never returned by pcre_exec() or pcre_dfa_exec() */
  "match limit exceeded",
  "callout error code",
  NULL,  /* BADUTF8/16 is handled specially */
  NULL,  /* BADUTF8/16 offset is handled specially */
  NULL,  /* PARTIAL is handled specially */
  "not used - internal error",
  "internal error - pattern overwritten?",
  "bad count value",
  "item unsupported for DFA matching",
  "backreference condition or recursion test not supported for DFA matching",
  "match limit not supported for DFA matching",
  "workspace size exceeded in DFA matching",
  "too much recursion for DFA matching",
  "recursion limit exceeded",
  "not used - internal error",
  "invalid combination of newline options",
  "bad offset value",
  NULL,  /* SHORTUTF8/16 is handled specially */
  "nested recursion at the same subject position",
  "JIT stack limit reached",
  "pattern compiled in wrong mode: 8-bit/16-bit error",
  "pattern compiled with other endianness",
  "invalid data in workspace for DFA restart",
  "bad JIT option",
  "bad length"
};


/*************************************************
*         Alternate character tables             *
*************************************************/

/* By default, the "tables" pointer when calling PCRE is set to NULL, thereby
using the default tables of the library. However, the T option can be used to
select alternate sets of tables, for different kinds of testing. Note also that
the L (locale) option also adjusts the tables. */

/* This is the set of tables distributed as default with PCRE. It recognizes
only ASCII characters. */

static const pcre_uint8 tables0[] = {

/* This table is a lower casing table. */

    0,  1,  2,  3,  4,  5,  6,  7,
    8,  9, 10, 11, 12, 13, 14, 15,
   16, 17, 18, 19, 20, 21, 22, 23,
   24, 25, 26, 27, 28, 29, 30, 31,
   32, 33, 34, 35, 36, 37, 38, 39,
   40, 41, 42, 43, 44, 45, 46, 47,
   48, 49, 50, 51, 52, 53, 54, 55,
   56, 57, 58, 59, 60, 61, 62, 63,
   64, 97, 98, 99,100,101,102,103,
  104,105,106,107,108,109,110,111,
  112,113,114,115,116,117,118,119,
  120,121,122, 91, 92, 93, 94, 95,
   96, 97, 98, 99,100,101,102,103,
  104,105,106,107,108,109,110,111,
  112,113,114,115,116,117,118,119,
  120,121,122,123,124,125,126,127,
  128,129,130,131,132,133,134,135,
  136,137,138,139,140,141,142,143,
  144,145,146,147,148,149,150,151,
  152,153,154,155,156,157,158,159,
  160,161,162,163,164,165,166,167,
  168,169,170,171,172,173,174,175,
  176,177,178,179,180,181,182,183,
  184,185,186,187,188,189,190,191,
  192,193,194,195,196,197,198,199,
  200,201,202,203,204,205,206,207,
  208,209,210,211,212,213,214,215,
  216,217,218,219,220,221,222,223,
  224,225,226,227,228,229,230,231,
  232,233,234,235,236,237,238,239,
  240,241,242,243,244,245,246,247,
  248,249,250,251,252,253,254,255,

/* This table is a case flipping table. */

    0,  1,  2,  3,  4,  5,  6,  7,
    8,  9, 10, 11, 12, 13, 14, 15,
   16, 17, 18, 19, 20, 21, 22, 23,
   24, 25, 26, 27, 28, 29, 30, 31,
   32, 33, 34, 35, 36, 37, 38, 39,
   40, 41, 42, 43, 44, 45, 46, 47,
   48, 49, 50, 51, 52, 53, 54, 55,
   56, 57, 58, 59, 60, 61, 62, 63,
   64, 97, 98, 99,100,101,102,103,
  104,105,106,107,108,109,110,111,
  112,113,114,115,116,117,118,119,
  120,121,122, 91, 92, 93, 94, 95,
   96, 65, 66, 67, 68, 69, 70, 71,
   72, 73, 74, 75, 76, 77, 78, 79,
   80, 81, 82, 83, 84, 85, 86, 87,
   88, 89, 90,123,124,125,126,127,
  128,129,130,131,132,133,134,135,
  136,137,138,139,140,141,142,143,
  144,145,146,147,148,149,150,151,
  152,153,154,155,156,157,158,159,
  160,161,162,163,164,165,166,167,
  168,169,170,171,172,173,174,175,
  176,177,178,179,180,181,182,183,
  184,185,186,187,188,189,190,191,
  192,193,194,195,196,197,198,199,
  200,201,202,203,204,205,206,207,
  208,209,210,211,212,213,214,215,
  216,217,218,219,220,221,222,223,
  224,225,226,227,228,229,230,231,
  232,233,234,235,236,237,238,239,
  240,241,242,243,244,245,246,247,
  248,249,250,251,252,253,254,255,

/* This table contains bit maps for various character classes. Each map is 32
bytes long and the bits run from the least significant end of each byte. The
classes that have their own maps are: space, xdigit, digit, upper, lower, word,
graph, print, punct, and cntrl. Other classes are built from combinations. */

  0x00,0x3e,0x00,0x00,0x01,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

  0x00,0x00,0x00,0x00,0x00,0x00,0xff,0x03,
  0x7e,0x00,0x00,0x00,0x7e,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

  0x00,0x00,0x00,0x00,0x00,0x00,0xff,0x03,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0xfe,0xff,0xff,0x07,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0xfe,0xff,0xff,0x07,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

  0x00,0x00,0x00,0x00,0x00,0x00,0xff,0x03,
  0xfe,0xff,0xff,0x87,0xfe,0xff,0xff,0x07,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

  0x00,0x00,0x00,0x00,0xfe,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x7f,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

  0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x7f,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

  0x00,0x00,0x00,0x00,0xfe,0xff,0x00,0xfc,
  0x01,0x00,0x00,0xf8,0x01,0x00,0x00,0x78,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

  0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/* This table identifies various classes of character by individual bits:
  0x01   white space character
  0x02   letter
  0x04   decimal digit
  0x08   hexadecimal digit
  0x10   alphanumeric or '_'
  0x80   regular expression metacharacter or binary zero
*/

  0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*   0-  7 */
  0x00,0x01,0x01,0x01,0x01,0x01,0x00,0x00, /*   8- 15 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  16- 23 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /*  24- 31 */
  0x01,0x00,0x00,0x00,0x80,0x00,0x00,0x00, /*    - '  */
  0x80,0x80,0x80,0x80,0x00,0x00,0x80,0x00, /*  ( - /  */
  0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c, /*  0 - 7  */
  0x1c,0x1c,0x00,0x00,0x00,0x00,0x00,0x80, /*  8 - ?  */
  0x00,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x12, /*  @ - G  */
  0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12, /*  H - O  */
  0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12, /*  P - W  */
  0x12,0x12,0x12,0x80,0x80,0x00,0x80,0x10, /*  X - _  */
  0x00,0x1a,0x1a,0x1a,0x1a,0x1a,0x1a,0x12, /*  ` - g  */
  0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12, /*  h - o  */
  0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12, /*  p - w  */
  0x12,0x12,0x12,0x80,0x80,0x00,0x00,0x00, /*  x -127 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 128-135 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 136-143 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 144-151 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 152-159 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 160-167 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 168-175 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 176-183 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 184-191 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 192-199 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 200-207 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 208-215 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 216-223 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 224-231 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 232-239 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* 240-247 */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};/* 248-255 */

/* This is a set of tables that came originally from a Windows user. It seems
to be at least an approximation of ISO 8859. In particular, there are
characters greater than 128 that are marked as spaces, letters, etc. */

static const pcre_uint8 tables1[] = {
0,1,2,3,4,5,6,7,
8,9,10,11,12,13,14,15,
16,17,18,19,20,21,22,23,
24,25,26,27,28,29,30,31,
32,33,34,35,36,37,38,39,
40,41,42,43,44,45,46,47,
48,49,50,51,52,53,54,55,
56,57,58,59,60,61,62,63,
64,97,98,99,100,101,102,103,
104,105,106,107,108,109,110,111,
112,113,114,115,116,117,118,119,
120,121,122,91,92,93,94,95,
96,97,98,99,100,101,102,103,
104,105,106,107,108,109,110,111,
112,113,114,115,116,117,118,119,
120,121,122,123,124,125,126,127,
128,129,130,131,132,133,134,135,
136,137,138,139,140,141,142,143,
144,145,146,147,148,149,150,151,
152,153,154,155,156,157,158,159,
160,161,162,163,164,165,166,167,
168,169,170,171,172,173,174,175,
176,177,178,179,180,181,182,183,
184,185,186,187,188,189,190,191,
224,225,226,227,228,229,230,231,
232,233,234,235,236,237,238,239,
240,241,242,243,244,245,246,215,
248,249,250,251,252,253,254,223,
224,225,226,227,228,229,230,231,
232,233,234,235,236,237,238,239,
240,241,242,243,244,245,246,247,
248,249,250,251,252,253,254,255,
0,1,2,3,4,5,6,7,
8,9,10,11,12,13,14,15,
16,17,18,19,20,21,22,23,
24,25,26,27,28,29,30,31,
32,33,34,35,36,37,38,39,
40,41,42,43,44,45,46,47,
48,49,50,51,52,53,54,55,
56,57,58,59,60,61,62,63,
64,97,98,99,100,101,102,103,
104,105,106,107,108,109,110,111,
112,113,114,115,116,117,118,119,
120,121,122,91,92,93,94,95,
96,65,66,67,68,69,70,71,
72,73,74,75,76,77,78,79,
80,81,82,83,84,85,86,87,
88,89,90,123,124,125,126,127,
128,129,130,131,132,133,134,135,
136,137,138,139,140,141,142,143,
144,145,146,147,148,149,150,151,
152,153,154,155,156,157,158,159,
160,161,162,163,164,165,166,167,
168,169,170,171,172,173,174,175,
176,177,178,179,180,181,182,183,
184,185,186,187,188,189,190,191,
224,225,226,227,228,229,230,231,
232,233,234,235,236,237,238,239,
240,241,242,243,244,245,246,215,
248,249,250,251,252,253,254,223,
192,193,194,195,196,197,198,199,
200,201,202,203,204,205,206,207,
208,209,210,211,212,213,214,247,
216,217,218,219,220,221,222,255,
0,62,0,0,1,0,0,0,
0,0,0,0,0,0,0,0,
32,0,0,0,1,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,255,3,
126,0,0,0,126,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,255,3,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,12,2,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
254,255,255,7,0,0,0,0,
0,0,0,0,0,0,0,0,
255,255,127,127,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,254,255,255,7,
0,0,0,0,0,4,32,4,
0,0,0,128,255,255,127,255,
0,0,0,0,0,0,255,3,
254,255,255,135,254,255,255,7,
0,0,0,0,0,4,44,6,
255,255,127,255,255,255,127,255,
0,0,0,0,254,255,255,255,
255,255,255,255,255,255,255,127,
0,0,0,0,254,255,255,255,
255,255,255,255,255,255,255,255,
0,2,0,0,255,255,255,255,
255,255,255,255,255,255,255,127,
0,0,0,0,255,255,255,255,
255,255,255,255,255,255,255,255,
0,0,0,0,254,255,0,252,
1,0,0,248,1,0,0,120,
0,0,0,0,254,255,255,255,
0,0,128,0,0,0,128,0,
255,255,255,255,0,0,0,0,
0,0,0,0,0,0,0,128,
255,255,255,255,0,0,0,0,
0,0,0,0,0,0,0,0,
128,0,0,0,0,0,0,0,
0,1,1,0,1,1,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
1,0,0,0,128,0,0,0,
128,128,128,128,0,0,128,0,
28,28,28,28,28,28,28,28,
28,28,0,0,0,0,0,128,
0,26,26,26,26,26,26,18,
18,18,18,18,18,18,18,18,
18,18,18,18,18,18,18,18,
18,18,18,128,128,0,128,16,
0,26,26,26,26,26,26,18,
18,18,18,18,18,18,18,18,
18,18,18,18,18,18,18,18,
18,18,18,128,128,0,0,0,
0,0,0,0,0,1,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
1,0,0,0,0,0,0,0,
0,0,18,0,0,0,0,0,
0,0,20,20,0,18,0,0,
0,20,18,0,0,0,0,0,
18,18,18,18,18,18,18,18,
18,18,18,18,18,18,18,18,
18,18,18,18,18,18,18,0,
18,18,18,18,18,18,18,18,
18,18,18,18,18,18,18,18,
18,18,18,18,18,18,18,18,
18,18,18,18,18,18,18,0,
18,18,18,18,18,18,18,18
};




#ifndef HAVE_STRERROR
/*************************************************
*     Provide strerror() for non-ANSI libraries  *
*************************************************/

/* Some old-fashioned systems still around (e.g. SunOS4) don't have strerror()
in their libraries, but can provide the same facility by this simple
alternative function. */

extern int   sys_nerr;
extern char *sys_errlist[];

char *
strerror(int n)
{
if (n < 0 || n >= sys_nerr) return "unknown error number";
return sys_errlist[n];
}
#endif /* HAVE_STRERROR */



/*************************************************
*       Print newline configuration              *
*************************************************/

/*
Arguments:
  rc         the return code from PCRE_CONFIG_NEWLINE
  isc        TRUE if called from "-C newline"
Returns:     nothing
*/

static void
print_newline_config(int rc, BOOL isc)
{
const char *s = NULL;
if (!isc) printf("  Newline sequence is ");
switch(rc)
  {
  case CHAR_CR: s = "CR"; break;
  case CHAR_LF: s = "LF"; break;
  case (CHAR_CR<<8 | CHAR_LF): s = "CRLF"; break;
  case -1: s = "ANY"; break;
  case -2: s = "ANYCRLF"; break;

  default:
  printf("a non-standard value: 0x%04x\n", rc);
  return;
  }

printf("%s\n", s);
}



/*************************************************
*         JIT memory callback                    *
*************************************************/

static pcre_jit_stack* jit_callback(void *arg)
{
jit_was_used = TRUE;
return (pcre_jit_stack *)arg;
}


#if !defined NOUTF || defined SUPPORT_PCRE16 || defined SUPPORT_PCRE32
/*************************************************
*            Convert UTF-8 string to value       *
*************************************************/

/* This function takes one or more bytes that represents a UTF-8 character,
and returns the value of the character.

Argument:
  utf8bytes   a pointer to the byte vector
  vptr        a pointer to an int to receive the value

Returns:      >  0 => the number of bytes consumed
              -6 to 0 => malformed UTF-8 character at offset = (-return)
*/

static int
utf82ord(pcre_uint8 *utf8bytes, pcre_uint32 *vptr)
{
pcre_uint32 c = *utf8bytes++;
pcre_uint32 d = c;
int i, j, s;

for (i = -1; i < 6; i++)               /* i is number of additional bytes */
  {
  if ((d & 0x80) == 0) break;
  d <<= 1;
  }

if (i == -1) { *vptr = c; return 1; }  /* ascii character */
if (i == 0 || i == 6) return 0;        /* invalid UTF-8 */

/* i now has a value in the range 1-5 */

s = 6*i;
d = (c & utf8_table3[i]) << s;

for (j = 0; j < i; j++)
  {
  c = *utf8bytes++;
  if ((c & 0xc0) != 0x80) return -(j+1);
  s -= 6;
  d |= (c & 0x3f) << s;
  }

/* Check that encoding was the correct unique one */

for (j = 0; j < utf8_table1_size; j++)
  if (d <= (pcre_uint32)utf8_table1[j]) break;
if (j != i) return -(i+1);

/* Valid value */

*vptr = d;
return i+1;
}
#endif /* NOUTF || SUPPORT_PCRE16 */



#if defined SUPPORT_PCRE8 && !defined NOUTF
/*************************************************
*       Convert character value to UTF-8         *
*************************************************/

/* This function takes an integer value in the range 0 - 0x7fffffff
and encodes it as a UTF-8 character in 0 to 6 bytes.

Arguments:
  cvalue     the character value
  utf8bytes  pointer to buffer for result - at least 6 bytes long

Returns:     number of characters placed in the buffer
*/

static int
ord2utf8(pcre_uint32 cvalue, pcre_uint8 *utf8bytes)
{
register int i, j;
if (cvalue > 0x7fffffffu)
  return -1;
for (i = 0; i < utf8_table1_size; i++)
  if (cvalue <= (pcre_uint32)utf8_table1[i]) break;
utf8bytes += i;
for (j = i; j > 0; j--)
 {
 *utf8bytes-- = 0x80 | (cvalue & 0x3f);
 cvalue >>= 6;
 }
*utf8bytes = utf8_table2[i] | cvalue;
return i + 1;
}
#endif


#ifdef SUPPORT_PCRE16
/*************************************************
*         Convert a string to 16-bit             *
*************************************************/

/* In non-UTF mode, the space needed for a 16-bit string is exactly double the
8-bit size. For a UTF-8 string, the size needed for UTF-16 is no more than
double, because up to 0xffff uses no more than 3 bytes in UTF-8 but possibly 4
in UTF-16. Higher values use 4 bytes in UTF-8 and up to 4 bytes in UTF-16. The
result is always left in buffer16.

Note that this function does not object to surrogate values. This is
deliberate; it makes it possible to construct UTF-16 strings that are invalid,
for the purpose of testing that they are correctly faulted.

Patterns to be converted are either plain ASCII or UTF-8; data lines are always
in UTF-8 so that values greater than 255 can be handled.

Arguments:
  data       TRUE if converting a data line; FALSE for a regex
  p          points to a byte string
  utf        true if UTF-8 (to be converted to UTF-16)
  len        number of bytes in the string (excluding trailing zero)

Returns:     number of 16-bit data items used (excluding trailing zero)
             OR -1 if a UTF-8 string is malformed
             OR -2 if a value > 0x10ffff is encountered
             OR -3 if a value > 0xffff is encountered when not in UTF mode
*/

static int
to16(int data, pcre_uint8 *p, int utf, int len)
{
pcre_uint16 *pp;

if (buffer16_size < 2*len + 2)
  {
  if (buffer16 != NULL) free(buffer16);
  buffer16_size = 2*len + 2;
  buffer16 = (pcre_uint16 *)malloc(buffer16_size);
  if (buffer16 == NULL)
    {
    fprintf(stderr, "pcretest: malloc(%d) failed for buffer16\n", buffer16_size);
    exit(1);
    }
  }

pp = buffer16;

if (!utf && !data)
  {
  while (len-- > 0) *pp++ = *p++;
  }

else
  {
  pcre_uint32 c = 0;
  while (len > 0)
    {
    int chlen = utf82ord(p, &c);
    if (chlen <= 0) return -1;
    if (c > 0x10ffff) return -2;
    p += chlen;
    len -= chlen;
    if (c < 0x10000) *pp++ = c; else
      {
      if (!utf) return -3;
      c -= 0x10000;
      *pp++ = 0xD800 | (c >> 10);
      *pp++ = 0xDC00 | (c & 0x3ff);
      }
    }
  }

*pp = 0;
return pp - buffer16;
}
#endif

#ifdef SUPPORT_PCRE32
/*************************************************
*         Convert a string to 32-bit             *
*************************************************/

/* In non-UTF mode, the space needed for a 32-bit string is exactly four times the
8-bit size. For a UTF-8 string, the size needed for UTF-32 is no more than four
times, because up to 0xffff uses no more than 3 bytes in UTF-8 but possibly 4
in UTF-32. Higher values use 4 bytes in UTF-8 and up to 4 bytes in UTF-32. The
result is always left in buffer32.

Note that this function does not object to surrogate values. This is
deliberate; it makes it possible to construct UTF-32 strings that are invalid,
for the purpose of testing that they are correctly faulted.

Patterns to be converted are either plain ASCII or UTF-8; data lines are always
in UTF-8 so that values greater than 255 can be handled.

Arguments:
  data       TRUE if converting a data line; FALSE for a regex
  p          points to a byte string
  utf        true if UTF-8 (to be converted to UTF-32)
  len        number of bytes in the string (excluding trailing zero)

Returns:     number of 32-bit data items used (excluding trailing zero)
             OR -1 if a UTF-8 string is malformed
             OR -2 if a value > 0x10ffff is encountered
             OR -3 if an ill-formed value is encountered (i.e. a surrogate)
*/

static int
to32(int data, pcre_uint8 *p, int utf, int len)
{
pcre_uint32 *pp;

if (buffer32_size < 4*len + 4)
  {
  if (buffer32 != NULL) free(buffer32);
  buffer32_size = 4*len + 4;
  buffer32 = (pcre_uint32 *)malloc(buffer32_size);
  if (buffer32 == NULL)
    {
    fprintf(stderr, "pcretest: malloc(%d) failed for buffer32\n", buffer32_size);
    exit(1);
    }
  }

pp = buffer32;

if (!utf && !data)
  {
  while (len-- > 0) *pp++ = *p++;
  }

else
  {
  pcre_uint32 c = 0;
  while (len > 0)
    {
    int chlen = utf82ord(p, &c);
    if (chlen <= 0) return -1;
    if (utf)
      {
      if (c > 0x10ffff) return -2;
      if (!data && (c & 0xfffff800u) == 0xd800u) return -3;
      }

    p += chlen;
    len -= chlen;
    *pp++ = c;
    }
  }

*pp = 0;
return pp - buffer32;
}

/* Check that a 32-bit character string is valid UTF-32.

Arguments:
  string       points to the string
  length       length of string, or -1 if the string is zero-terminated

Returns:       TRUE  if the string is a valid UTF-32 string
               FALSE otherwise
*/

#ifdef NEVER   /* Not used */
#ifdef SUPPORT_UTF
static BOOL
valid_utf32(pcre_uint32 *string, int length)
{
register pcre_uint32 *p;
register pcre_uint32 c;

for (p = string; length-- > 0; p++)
  {
  c = *p;
  if (c > 0x10ffffu) return FALSE;                 /* Too big */
  if ((c & 0xfffff800u) == 0xd800u) return FALSE;  /* Surrogate */
  }

return TRUE;
}
#endif /* SUPPORT_UTF */
#endif /* NEVER */
#endif /* SUPPORT_PCRE32 */


/*************************************************
*        Read or extend an input line            *
*************************************************/

/* Input lines are read into buffer, but both patterns and data lines can be
continued over multiple input lines. In addition, if the buffer fills up, we
want to automatically expand it so as to be able to handle extremely large
lines that are needed for certain stress tests. When the input buffer is
expanded, the other two buffers must also be expanded likewise, and the
contents of pbuffer, which are a copy of the input for callouts, must be
preserved (for when expansion happens for a data line). This is not the most
optimal way of handling this, but hey, this is just a test program!

Arguments:
  f            the file to read
  start        where in buffer to start (this *must* be within buffer)
  prompt       for stdin or readline()

Returns:       pointer to the start of new data
               could be a copy of start, or could be moved
               NULL if no data read and EOF reached
*/

static pcre_uint8 *
extend_inputline(FILE *f, pcre_uint8 *start, const char *prompt)
{
pcre_uint8 *here = start;

for (;;)
  {
  size_t rlen = (size_t)(buffer_size - (here - buffer));

  if (rlen > 1000)
    {
    int dlen;

    /* If libreadline or libedit support is required, use readline() to read a
    line if the input is a terminal. Note that readline() removes the trailing
    newline, so we must put it back again, to be compatible with fgets(). */

#if defined(SUPPORT_LIBREADLINE) || defined(SUPPORT_LIBEDIT)
    if (isatty(fileno(f)))
      {
      size_t len;
      char *s = readline(prompt);
      if (s == NULL) return (here == start)? NULL : start;
      len = strlen(s);
      if (len > 0) add_history(s);
      if (len > rlen - 1) len = rlen - 1;
      memcpy(here, s, len);
      here[len] = '\n';
      here[len+1] = 0;
      free(s);
      }
    else
#endif

    /* Read the next line by normal means, prompting if the file is stdin. */

      {
      if (f == stdin) printf("%s", prompt);
      if (fgets((char *)here, rlen,  f) == NULL)
        return (here == start)? NULL : start;
      }

    dlen = (int)strlen((char *)here);
    if (dlen > 0 && here[dlen - 1] == '\n') return start;
    here += dlen;
    }

  else
    {
    int new_buffer_size = 2*buffer_size;
    pcre_uint8 *new_buffer = (pcre_uint8 *)malloc(new_buffer_size);
    pcre_uint8 *new_pbuffer = (pcre_uint8 *)malloc(new_buffer_size);

    if (new_buffer == NULL || new_pbuffer == NULL)
      {
      fprintf(stderr, "pcretest: malloc(%d) failed\n", new_buffer_size);
      exit(1);
      }

    memcpy(new_buffer, buffer, buffer_size);
    memcpy(new_pbuffer, pbuffer, buffer_size);

    buffer_size = new_buffer_size;

    start = new_buffer + (start - buffer);
    here = new_buffer + (here - buffer);

    free(buffer);
    free(pbuffer);

    buffer = new_buffer;
    pbuffer = new_pbuffer;
    }
  }

/* Control never gets here */
}



/*************************************************
*          Read number from string               *
*************************************************/

/* We don't use strtoul() because SunOS4 doesn't have it. Rather than mess
around with conditional compilation, just do the job by hand. It is only used
for unpicking arguments, so just keep it simple.

Arguments:
  str           string to be converted
  endptr        where to put the end pointer

Returns:        the unsigned long
*/

static int
get_value(pcre_uint8 *str, pcre_uint8 **endptr)
{
int result = 0;
while(*str != 0 && isspace(*str)) str++;
while (isdigit(*str)) result = result * 10 + (int)(*str++ - '0');
*endptr = str;
return(result);
}



/*************************************************
*             Print one character                *
*************************************************/

/* Print a single character either literally, or as a hex escape. */

static int pchar(pcre_uint32 c, FILE *f)
{
int n = 0;
if (PRINTOK(c))
  {
  if (f != NULL) fprintf(f, "%c", c);
  return 1;
  }

if (c < 0x100)
  {
  if (use_utf)
    {
    if (f != NULL) fprintf(f, "\\x{%02x}", c);
    return 6;
    }
  else
    {
    if (f != NULL) fprintf(f, "\\x%02x", c);
    return 4;
    }
  }

if (f != NULL) n = fprintf(f, "\\x{%02x}", c);
return n >= 0 ? n : 0;
}



#ifdef SUPPORT_PCRE8
/*************************************************
*         Print 8-bit character string           *
*************************************************/

/* Must handle UTF-8 strings in utf8 mode. Yields number of characters printed.
If handed a NULL file, just counts chars without printing. */

static int pchars(pcre_uint8 *p, int length, FILE *f)
{
pcre_uint32 c = 0;
int yield = 0;

if (length < 0)
  length = strlen((char *)p);

while (length-- > 0)
  {
#if !defined NOUTF
  if (use_utf)
    {
    int rc = utf82ord(p, &c);
    if (rc > 0 && rc <= length + 1)   /* Mustn't run over the end */
      {
      length -= rc - 1;
      p += rc;
      yield += pchar(c, f);
      continue;
      }
    }
#endif
  c = *p++;
  yield += pchar(c, f);
  }

return yield;
}
#endif



#ifdef SUPPORT_PCRE16
/*************************************************
*    Find length of 0-terminated 16-bit string   *
*************************************************/

static int strlen16(PCRE_SPTR16 p)
{
PCRE_SPTR16 pp = p;
while (*pp != 0) pp++;
return (int)(pp - p);
}
#endif  /* SUPPORT_PCRE16 */



#ifdef SUPPORT_PCRE32
/*************************************************
*    Find length of 0-terminated 32-bit string   *
*************************************************/

static int strlen32(PCRE_SPTR32 p)
{
PCRE_SPTR32 pp = p;
while (*pp != 0) pp++;
return (int)(pp - p);
}
#endif  /* SUPPORT_PCRE32 */



#ifdef SUPPORT_PCRE16
/*************************************************
*           Print 16-bit character string        *
*************************************************/

/* Must handle UTF-16 strings in utf mode. Yields number of characters printed.
If handed a NULL file, just counts chars without printing. */

static int pchars16(PCRE_SPTR16 p, int length, FILE *f)
{
int yield = 0;

if (length < 0)
  length = strlen16(p);

while (length-- > 0)
  {
  pcre_uint32 c = *p++ & 0xffff;
#if !defined NOUTF
  if (use_utf && c >= 0xD800 && c < 0xDC00 && length > 0)
    {
    int d = *p & 0xffff;
    if (d >= 0xDC00 && d <= 0xDFFF)
      {
      c = ((c & 0x3ff) << 10) + (d & 0x3ff) + 0x10000;
      length--;
      p++;
      }
    }
#endif
  yield += pchar(c, f);
  }

return yield;
}
#endif  /* SUPPORT_PCRE16 */



#ifdef SUPPORT_PCRE32
/*************************************************
*           Print 32-bit character string        *
*************************************************/

/* Must handle UTF-32 strings in utf mode. Yields number of characters printed.
If handed a NULL file, just counts chars without printing. */

static int pchars32(PCRE_SPTR32 p, int length, BOOL utf, FILE *f)
{
int yield = 0;

(void)(utf);  /* Avoid compiler warning */

if (length < 0)
  length = strlen32(p);

while (length-- > 0)
  {
  pcre_uint32 c = *p++;
  yield += pchar(c, f);
  }

return yield;
}
#endif  /* SUPPORT_PCRE32 */



#ifdef SUPPORT_PCRE8
/*************************************************
*     Read a capture name (8-bit) and check it   *
*************************************************/

static pcre_uint8 *
read_capture_name8(pcre_uint8 *p, pcre_uint8 **pp, pcre *re)
{
pcre_uint8 *npp = *pp;
while (isalnum(*p)) *npp++ = *p++;
*npp++ = 0;
*npp = 0;
if (pcre_get_stringnumber(re, (char *)(*pp)) < 0)
  {
  fprintf(outfile, "no parentheses with name \"");
  PCHARSV(*pp, 0, -1, outfile);
  fprintf(outfile, "\"\n");
  }

*pp = npp;
return p;
}
#endif  /* SUPPORT_PCRE8 */



#ifdef SUPPORT_PCRE16
/*************************************************
*     Read a capture name (16-bit) and check it  *
*************************************************/

/* Note that the text being read is 8-bit. */

static pcre_uint8 *
read_capture_name16(pcre_uint8 *p, pcre_uint16 **pp, pcre *re)
{
pcre_uint16 *npp = *pp;
while (isalnum(*p)) *npp++ = *p++;
*npp++ = 0;
*npp = 0;
if (pcre16_get_stringnumber((pcre16 *)re, (PCRE_SPTR16)(*pp)) < 0)
  {
  fprintf(outfile, "no parentheses with name \"");
  PCHARSV(*pp, 0, -1, outfile);
  fprintf(outfile, "\"\n");
  }
*pp = npp;
return p;
}
#endif  /* SUPPORT_PCRE16 */



#ifdef SUPPORT_PCRE32
/*************************************************
*     Read a capture name (32-bit) and check it  *
*************************************************/

/* Note that the text being read is 8-bit. */

static pcre_uint8 *
read_capture_name32(pcre_uint8 *p, pcre_uint32 **pp, pcre *re)
{
pcre_uint32 *npp = *pp;
while (isalnum(*p)) *npp++ = *p++;
*npp++ = 0;
*npp = 0;
if (pcre32_get_stringnumber((pcre32 *)re, (PCRE_SPTR32)(*pp)) < 0)
  {
  fprintf(outfile, "no parentheses with name \"");
  PCHARSV(*pp, 0, -1, outfile);
  fprintf(outfile, "\"\n");
  }
*pp = npp;
return p;
}
#endif  /* SUPPORT_PCRE32 */



/*************************************************
*            Stack guard function                *
*************************************************/

/* Called from PCRE when set in pcre_stack_guard. We give an error (non-zero)
return when a count overflows. */

static int stack_guard(void)
{
return stack_guard_return;
}

/*************************************************
*              Callout function                  *
*************************************************/

/* Called from PCRE as a result of the (?C) item. We print out where we are in
the match. Yield zero unless more callouts than the fail count, or the callout
data is not zero. */

static int callout(pcre_callout_block *cb)
{
FILE *f = (first_callout | callout_extra)? outfile : NULL;
int i, pre_start, post_start, subject_length;

if (callout_extra)
  {
  fprintf(f, "Callout %d: last capture = %d\n",
    cb->callout_number, cb->capture_last);

  if (cb->offset_vector != NULL)
    {
    for (i = 0; i < cb->capture_top * 2; i += 2)
      {
      if (cb->offset_vector[i] < 0)
        fprintf(f, "%2d: <unset>\n", i/2);
      else
        {
        fprintf(f, "%2d: ", i/2);
        PCHARSV(cb->subject, cb->offset_vector[i],
          cb->offset_vector[i+1] - cb->offset_vector[i], f);
        fprintf(f, "\n");
        }
      }
    }
  }

/* Re-print the subject in canonical form, the first time or if giving full
datails. On subsequent calls in the same match, we use pchars just to find the
printed lengths of the substrings. */

if (f != NULL) fprintf(f, "--->");

PCHARS(pre_start, cb->subject, 0, cb->start_match, f);
PCHARS(post_start, cb->subject, cb->start_match,
  cb->current_position - cb->start_match, f);

PCHARS(subject_length, cb->subject, 0, cb->subject_length, NULL);

PCHARSV(cb->subject, cb->current_position,
  cb->subject_length - cb->current_position, f);

if (f != NULL) fprintf(f, "\n");

/* Always print appropriate indicators, with callout number if not already
shown. For automatic callouts, show the pattern offset. */

if (cb->callout_number == 255)
  {
  fprintf(outfile, "%+3d ", cb->pattern_position);
  if (cb->pattern_position > 99) fprintf(outfile, "\n    ");
  }
else
  {
  if (callout_extra) fprintf(outfile, "    ");
    else fprintf(outfile, "%3d ", cb->callout_number);
  }

for (i = 0; i < pre_start; i++) fprintf(outfile, " ");
fprintf(outfile, "^");

if (post_start > 0)
  {
  for (i = 0; i < post_start - 1; i++) fprintf(outfile, " ");
  fprintf(outfile, "^");
  }

for (i = 0; i < subject_length - pre_start - post_start + 4; i++)
  fprintf(outfile, " ");

fprintf(outfile, "%.*s", (cb->next_item_length == 0)? 1 : cb->next_item_length,
  pbuffer + cb->pattern_position);

fprintf(outfile, "\n");
first_callout = 0;

if (cb->mark != last_callout_mark)
  {
  if (cb->mark == NULL)
    fprintf(outfile, "Latest Mark: <unset>\n");
  else
    {
    fprintf(outfile, "Latest Mark: ");
    PCHARSV(cb->mark, 0, -1, outfile);
    putc('\n', outfile);
    }
  last_callout_mark = cb->mark;
  }

if (cb->callout_data != NULL)
  {
  int callout_data = *((int *)(cb->callout_data));
  if (callout_data != 0)
    {
    fprintf(outfile, "Callout data = %d\n", callout_data);
    return callout_data;
    }
  }

return (cb->callout_number != callout_fail_id)? 0 :
       (++callout_count >= callout_fail_count)? 1 : 0;
}


/*************************************************
*            Local malloc functions              *
*************************************************/

/* Alternative malloc function, to test functionality and save the size of a
compiled re, which is the first store request that pcre_compile() makes. The
show_malloc variable is set only during matching. */

static void *new_malloc(size_t size)
{
void *block = malloc(size);
if (show_malloc)
  fprintf(outfile, "malloc       %3d %p\n", (int)size, block);
return block;
}

static void new_free(void *block)
{
if (show_malloc)
  fprintf(outfile, "free             %p\n", block);
free(block);
}

/* For recursion malloc/free, to test stacking calls */

static void *stack_malloc(size_t size)
{
void *block = malloc(size);
if (show_malloc)
  fprintf(outfile, "stack_malloc %3d %p\n", (int)size, block);
return block;
}

static void stack_free(void *block)
{
if (show_malloc)
  fprintf(outfile, "stack_free       %p\n", block);
free(block);
}


/*************************************************
*          Call pcre_fullinfo()                  *
*************************************************/

/* Get one piece of information from the pcre_fullinfo() function. When only
one of 8-, 16- or 32-bit is supported, pcre_mode should always have the correct
value, but the code is defensive.

Arguments:
  re        compiled regex
  study     study data
  option    PCRE_INFO_xxx option
  ptr       where to put the data

Returns:    0 when OK, < 0 on error
*/

static int
new_info(pcre *re, pcre_extra *study, int option, void *ptr)
{
int rc;

if (pcre_mode == PCRE32_MODE)
#ifdef SUPPORT_PCRE32
  rc = pcre32_fullinfo((pcre32 *)re, (pcre32_extra *)study, option, ptr);
#else
  rc = PCRE_ERROR_BADMODE;
#endif
else if (pcre_mode == PCRE16_MODE)
#ifdef SUPPORT_PCRE16
  rc = pcre16_fullinfo((pcre16 *)re, (pcre16_extra *)study, option, ptr);
#else
  rc = PCRE_ERROR_BADMODE;
#endif
else
#ifdef SUPPORT_PCRE8
  rc = pcre_fullinfo(re, study, option, ptr);
#else
  rc = PCRE_ERROR_BADMODE;
#endif

if (rc < 0 && rc != PCRE_ERROR_UNSET)
  {
  fprintf(outfile, "Error %d from pcre%s_fullinfo(%d)\n", rc,
    pcre_mode == PCRE32_MODE ? "32" : pcre_mode == PCRE16_MODE ? "16" : "", option);
  if (rc == PCRE_ERROR_BADMODE)
    fprintf(outfile, "Running in %d-bit mode but pattern was compiled in "
      "%d-bit mode\n", 8 * CHAR_SIZE,
      8 * (REAL_PCRE_FLAGS(re) & PCRE_MODE_MASK));
  }

return rc;
}



/*************************************************
*             Swap byte functions                *
*************************************************/

/* The following functions swap the bytes of a pcre_uint16 and pcre_uint32
value, respectively.

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
*        Flip bytes in a compiled pattern        *
*************************************************/

/* This function is called if the 'F' option was present on a pattern that is
to be written to a file. We flip the bytes of all the integer fields in the
regex data block and the study block. In 16-bit mode this also flips relevant
bytes in the pattern itself. This is to make it possible to test PCRE's
ability to reload byte-flipped patterns, e.g. those compiled on a different
architecture. */

#if defined SUPPORT_PCRE8 || defined SUPPORT_PCRE16
static void
regexflip8_or_16(pcre *ere, pcre_extra *extra)
{
real_pcre8_or_16 *re = (real_pcre8_or_16 *)ere;
#ifdef SUPPORT_PCRE16
int op;
pcre_uint16 *ptr = (pcre_uint16 *)re + re->name_table_offset;
int length = re->name_count * re->name_entry_size;
#ifdef SUPPORT_UTF
BOOL utf = (re->options & PCRE_UTF16) != 0;
BOOL utf16_char = FALSE;
#endif /* SUPPORT_UTF */
#endif /* SUPPORT_PCRE16 */

/* Always flip the bytes in the main data block and study blocks. */

re->magic_number = REVERSED_MAGIC_NUMBER;
re->size = swap_uint32(re->size);
re->options = swap_uint32(re->options);
re->flags = swap_uint32(re->flags);
re->limit_match = swap_uint32(re->limit_match);
re->limit_recursion = swap_uint32(re->limit_recursion);
re->first_char = swap_uint16(re->first_char);
re->req_char = swap_uint16(re->req_char);
re->max_lookbehind = swap_uint16(re->max_lookbehind);
re->top_bracket = swap_uint16(re->top_bracket);
re->top_backref = swap_uint16(re->top_backref);
re->name_table_offset = swap_uint16(re->name_table_offset);
re->name_entry_size = swap_uint16(re->name_entry_size);
re->name_count = swap_uint16(re->name_count);
re->ref_count = swap_uint16(re->ref_count);

if (extra != NULL && (extra->flags & PCRE_EXTRA_STUDY_DATA) != 0)
  {
  pcre_study_data *rsd = (pcre_study_data *)(extra->study_data);
  rsd->size = swap_uint32(rsd->size);
  rsd->flags = swap_uint32(rsd->flags);
  rsd->minlength = swap_uint32(rsd->minlength);
  }

/* In 8-bit mode, that is all we need to do. In 16-bit mode we must swap bytes
in the name table, if present, and then in the pattern itself. */

#ifdef SUPPORT_PCRE16
if (pcre_mode != PCRE16_MODE) return;

while(TRUE)
  {
  /* Swap previous characters. */
  while (length-- > 0)
    {
    *ptr = swap_uint16(*ptr);
    ptr++;
    }
#ifdef SUPPORT_UTF
  if (utf16_char)
    {
    if ((ptr[-1] & 0xfc00) == 0xd800)
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
  op = *ptr;
  *ptr++ = swap_uint16(op);

  switch (op)
    {
    case OP_END:
    return;

#ifdef SUPPORT_UTF
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
    length = OP_lengths16[op] - 1;
    break;

    case OP_CLASS:
    case OP_NCLASS:
    /* Skip the character bit map. */
    ptr += 32/sizeof(pcre_uint16);
    length = 0;
    break;

    case OP_XCLASS:
    /* LINK_SIZE can be 1 or 2 in 16 bit mode. */
    if (LINK_SIZE > 1)
      length = (int)((((unsigned int)(ptr[0]) << 16) | (unsigned int)(ptr[1]))
        - (1 + LINK_SIZE + 1));
    else
      length = (int)((unsigned int)(ptr[0]) - (1 + LINK_SIZE + 1));

    /* Reverse the size of the XCLASS instance. */
    *ptr = swap_uint16(*ptr);
    ptr++;
    if (LINK_SIZE > 1)
      {
      *ptr = swap_uint16(*ptr);
      ptr++;
      }

    op = *ptr;
    *ptr = swap_uint16(op);
    ptr++;
    if ((op & XCL_MAP) != 0)
      {
      /* Skip the character bit map. */
      ptr += 32/sizeof(pcre_uint16);
      length -= 32/sizeof(pcre_uint16);
      }
    break;
    }
  }
/* Control should never reach here in 16 bit mode. */
#endif /* SUPPORT_PCRE16 */
}
#endif /* SUPPORT_PCRE[8|16] */



#if defined SUPPORT_PCRE32
static void
regexflip_32(pcre *ere, pcre_extra *extra)
{
real_pcre32 *re = (real_pcre32 *)ere;
int op;
pcre_uint32 *ptr = (pcre_uint32 *)re + re->name_table_offset;
int length = re->name_count * re->name_entry_size;

/* Always flip the bytes in the main data block and study blocks. */

re->magic_number = REVERSED_MAGIC_NUMBER;
re->size = swap_uint32(re->size);
re->options = swap_uint32(re->options);
re->flags = swap_uint32(re->flags);
re->limit_match = swap_uint32(re->limit_match);
re->limit_recursion = swap_uint32(re->limit_recursion);
re->first_char = swap_uint32(re->first_char);
re->req_char = swap_uint32(re->req_char);
re->max_lookbehind = swap_uint16(re->max_lookbehind);
re->top_bracket = swap_uint16(re->top_bracket);
re->top_backref = swap_uint16(re->top_backref);
re->name_table_offset = swap_uint16(re->name_table_offset);
re->name_entry_size = swap_uint16(re->name_entry_size);
re->name_count = swap_uint16(re->name_count);
re->ref_count = swap_uint16(re->ref_count);

if (extra != NULL && (extra->flags & PCRE_EXTRA_STUDY_DATA) != 0)
  {
  pcre_study_data *rsd = (pcre_study_data *)(extra->study_data);
  rsd->size = swap_uint32(rsd->size);
  rsd->flags = swap_uint32(rsd->flags);
  rsd->minlength = swap_uint32(rsd->minlength);
  }

/* In 32-bit mode we must swap bytes in the name table, if present, and then in
the pattern itself. */

while(TRUE)
  {
  /* Swap previous characters. */
  while (length-- > 0)
    {
    *ptr = swap_uint32(*ptr);
    ptr++;
    }

  /* Get next opcode. */

  length = 0;
  op = *ptr;
  *ptr++ = swap_uint32(op);

  switch (op)
    {
    case OP_END:
    return;

    default:
    length = OP_lengths32[op] - 1;
    break;

    case OP_CLASS:
    case OP_NCLASS:
    /* Skip the character bit map. */
    ptr += 32/sizeof(pcre_uint32);
    length = 0;
    break;

    case OP_XCLASS:
    /* LINK_SIZE can only be 1 in 32-bit mode. */
    length = (int)((unsigned int)(ptr[0]) - (1 + LINK_SIZE + 1));

    /* Reverse the size of the XCLASS instance. */
    *ptr = swap_uint32(*ptr);
    ptr++;

    op = *ptr;
    *ptr = swap_uint32(op);
    ptr++;
    if ((op & XCL_MAP) != 0)
      {
      /* Skip the character bit map. */
      ptr += 32/sizeof(pcre_uint32);
      length -= 32/sizeof(pcre_uint32);
      }
    break;
    }
  }
/* Control should never reach here in 32 bit mode. */
}

#endif /* SUPPORT_PCRE32 */



static void
regexflip(pcre *ere, pcre_extra *extra)
{
#if defined SUPPORT_PCRE32
  if (REAL_PCRE_FLAGS(ere) & PCRE_MODE32)
    regexflip_32(ere, extra);
#endif
#if defined SUPPORT_PCRE8 || defined SUPPORT_PCRE16
  if (REAL_PCRE_FLAGS(ere) & (PCRE_MODE8 | PCRE_MODE16))
    regexflip8_or_16(ere, extra);
#endif
}



/*************************************************
*        Check match or recursion limit          *
*************************************************/

static int
check_match_limit(pcre *re, pcre_extra *extra, pcre_uint8 *bptr, int len,
  int start_offset, int options, int *use_offsets, int use_size_offsets,
  int flag, unsigned long int *limit, int errnumber, const char *msg)
{
int count;
int min = 0;
int mid = 64;
int max = -1;

extra->flags |= flag;

for (;;)
  {
  *limit = mid;

  PCRE_EXEC(count, re, extra, bptr, len, start_offset, options,
    use_offsets, use_size_offsets);

  if (count == errnumber)
    {
    /* fprintf(outfile, "Testing %s limit = %d\n", msg, mid); */
    min = mid;
    mid = (mid == max - 1)? max : (max > 0)? (min + max)/2 : mid*2;
    }

  else if (count >= 0 || count == PCRE_ERROR_NOMATCH ||
                         count == PCRE_ERROR_PARTIAL)
    {
    if (mid == min + 1)
      {
      fprintf(outfile, "Minimum %s limit = %d\n", msg, mid);
      break;
      }
    /* fprintf(outfile, "Testing %s limit = %d\n", msg, mid); */
    max = mid;
    mid = (min + mid)/2;
    }
  else break;    /* Some other error */
  }

extra->flags &= ~flag;
return count;
}



/*************************************************
*         Case-independent strncmp() function    *
*************************************************/

/*
Arguments:
  s         first string
  t         second string
  n         number of characters to compare

Returns:    < 0, = 0, or > 0, according to the comparison
*/

static int
strncmpic(pcre_uint8 *s, pcre_uint8 *t, int n)
{
while (n--)
  {
  int c = tolower(*s++) - tolower(*t++);
  if (c) return c;
  }
return 0;
}



/*************************************************
*         Check multicharacter option            *
*************************************************/

/* This is used both at compile and run-time to check for <xxx> escapes. Print
a message and return 0 if there is no match.

Arguments:
  p           points after the leading '<'
  f           file for error message
  nl          TRUE to check only for newline settings
  stype       "modifier" or "escape sequence"

Returns:      appropriate PCRE_NEWLINE_xxx flags, or 0
*/

static int
check_mc_option(pcre_uint8 *p, FILE *f, BOOL nl, const char *stype)
{
if (strncmpic(p, (pcre_uint8 *)"cr>", 3) == 0) return PCRE_NEWLINE_CR;
if (strncmpic(p, (pcre_uint8 *)"lf>", 3) == 0) return PCRE_NEWLINE_LF;
if (strncmpic(p, (pcre_uint8 *)"crlf>", 5) == 0) return PCRE_NEWLINE_CRLF;
if (strncmpic(p, (pcre_uint8 *)"anycrlf>", 8) == 0) return PCRE_NEWLINE_ANYCRLF;
if (strncmpic(p, (pcre_uint8 *)"any>", 4) == 0) return PCRE_NEWLINE_ANY;
if (strncmpic(p, (pcre_uint8 *)"bsr_anycrlf>", 12) == 0) return PCRE_BSR_ANYCRLF;
if (strncmpic(p, (pcre_uint8 *)"bsr_unicode>", 12) == 0) return PCRE_BSR_UNICODE;

if (!nl)
  {
  if (strncmpic(p, (pcre_uint8 *)"JS>", 3) == 0) return PCRE_JAVASCRIPT_COMPAT;
  }

fprintf(f, "Unknown %s at: <%s\n", stype, p);
return 0;
}



/*************************************************
*             Usage function                     *
*************************************************/

static void
usage(void)
{
printf("Usage:     pcretest [options] [<input file> [<output file>]]\n\n");
printf("Input and output default to stdin and stdout.\n");
#if defined(SUPPORT_LIBREADLINE) || defined(SUPPORT_LIBEDIT)
printf("If input is a terminal, readline() is used to read from it.\n");
#else
printf("This version of pcretest is not linked with readline().\n");
#endif
printf("\nOptions:\n");
#ifdef SUPPORT_PCRE16
printf("  -16      use the 16-bit library\n");
#endif
#ifdef SUPPORT_PCRE32
printf("  -32      use the 32-bit library\n");
#endif
printf("  -b       show compiled code\n");
printf("  -C       show PCRE compile-time options and exit\n");
printf("  -C arg   show a specific compile-time option and exit\n");
printf("           with its value if numeric (else 0). The arg can be:\n");
printf("     linksize     internal link size [2, 3, 4]\n");
printf("     pcre8        8 bit library support enabled [0, 1]\n");
printf("     pcre16       16 bit library support enabled [0, 1]\n");
printf("     pcre32       32 bit library support enabled [0, 1]\n");
printf("     utf          Unicode Transformation Format supported [0, 1]\n");
printf("     ucp          Unicode Properties supported [0, 1]\n");
printf("     jit          Just-in-time compiler supported [0, 1]\n");
printf("     newline      Newline type [CR, LF, CRLF, ANYCRLF, ANY]\n");
printf("     bsr          \\R type [ANYCRLF, ANY]\n");
printf("  -d       debug: show compiled code and information (-b and -i)\n");
#if !defined NODFA
printf("  -dfa     force DFA matching for all subjects\n");
#endif
printf("  -help    show usage information\n");
printf("  -i       show information about compiled patterns\n"
       "  -M       find MATCH_LIMIT minimum for each subject\n"
       "  -m       output memory used information\n"
       "  -O       set PCRE_NO_AUTO_POSSESS on each pattern\n"
       "  -o <n>   set size of offsets vector to <n>\n");
#if !defined NOPOSIX
printf("  -p       use POSIX interface\n");
#endif
printf("  -q       quiet: do not output PCRE version number at start\n");
printf("  -S <n>   set stack size to <n> megabytes\n");
printf("  -s       force each pattern to be studied at basic level\n"
       "  -s+      force each pattern to be studied, using JIT if available\n"
       "  -s++     ditto, verifying when JIT was actually used\n"
       "  -s+n     force each pattern to be studied, using JIT if available,\n"
       "             where 1 <= n <= 7 selects JIT options\n"
       "  -s++n    ditto, verifying when JIT was actually used\n"
       "  -t       time compilation and execution\n");
printf("  -t <n>   time compilation and execution, repeating <n> times\n");
printf("  -tm      time execution (matching) only\n");
printf("  -tm <n>  time execution (matching) only, repeating <n> times\n");
printf("  -T       same as -t, but show total times at the end\n");
printf("  -TM      same as -tm, but show total time at the end\n");
}



/*************************************************
*                Main Program                    *
*************************************************/

/* Read lines from named file or stdin and write to named file or stdout; lines
consist of a regular expression, in delimiters and optionally followed by
options, followed by a set of test data, terminated by an empty line. */

int main(int argc, char **argv)
{
FILE *infile = stdin;
const char *version;
int options = 0;
int study_options = 0;
int default_find_match_limit = FALSE;
pcre_uint32 default_options = 0;
int op = 1;
int timeit = 0;
int timeitm = 0;
int showtotaltimes = 0;
int showinfo = 0;
int showstore = 0;
int force_study = -1;
int force_study_options = 0;
int quiet = 0;
int size_offsets = 45;
int size_offsets_max;
int *offsets = NULL;
int debug = 0;
int done = 0;
int all_use_dfa = 0;
int verify_jit = 0;
int yield = 0;
int stack_size;
pcre_uint8 *dbuffer = NULL;
pcre_uint8 lockout[24] = { 0 };
size_t dbuffer_size = 1u << 14;
clock_t total_compile_time = 0;
clock_t total_study_time = 0;
clock_t total_match_time = 0;

#if !defined NOPOSIX
int posix = 0;
#endif
#if !defined NODFA
int *dfa_workspace = NULL;
#endif

pcre_jit_stack *jit_stack = NULL;

/* These vectors store, end-to-end, a list of zero-terminated captured
substring names, each list itself being terminated by an empty name. Assume
that 1024 is plenty long enough for the few names we'll be testing. It is
easiest to keep separate 8-, 16- and 32-bit versions, using the 32-bit version
for the actual memory, to ensure alignment. */

pcre_uint32 copynames[1024];
pcre_uint32 getnames[1024];

#ifdef SUPPORT_PCRE32
pcre_uint32 *cn32ptr;
pcre_uint32 *gn32ptr;
#endif

#ifdef SUPPORT_PCRE16
pcre_uint16 *copynames16 = (pcre_uint16 *)copynames;
pcre_uint16 *getnames16 = (pcre_uint16 *)getnames;
pcre_uint16 *cn16ptr;
pcre_uint16 *gn16ptr;
#endif

#ifdef SUPPORT_PCRE8
pcre_uint8 *copynames8 = (pcre_uint8 *)copynames;
pcre_uint8 *getnames8 = (pcre_uint8 *)getnames;
pcre_uint8 *cn8ptr;
pcre_uint8 *gn8ptr;
#endif

/* Get buffers from malloc() so that valgrind will check their misuse when
debugging. They grow automatically when very long lines are read. The 16-
and 32-bit buffers (buffer16, buffer32) are obtained only if needed. */

buffer = (pcre_uint8 *)malloc(buffer_size);
pbuffer = (pcre_uint8 *)malloc(buffer_size);

/* The outfile variable is static so that new_malloc can use it. */

outfile = stdout;

/* The following  _setmode() stuff is some Windows magic that tells its runtime
library to translate CRLF into a single LF character. At least, that's what
I've been told: never having used Windows I take this all on trust. Originally
it set 0x8000, but then I was advised that _O_BINARY was better. */

#if defined(_WIN32) || defined(WIN32)
_setmode( _fileno( stdout ), _O_BINARY );
#endif

/* Get the version number: both pcre_version() and pcre16_version() give the
same answer. We just need to ensure that we call one that is available. */

#if defined SUPPORT_PCRE8
version = pcre_version();
#elif defined SUPPORT_PCRE16
version = pcre16_version();
#elif defined SUPPORT_PCRE32
version = pcre32_version();
#endif

/* Scan options */

while (argc > 1 && argv[op][0] == '-')
  {
  pcre_uint8 *endptr;
  char *arg = argv[op];

  if (strcmp(arg, "-m") == 0) showstore = 1;
  else if (strcmp(arg, "-s") == 0) force_study = 0;

  else if (strncmp(arg, "-s+", 3) == 0)
    {
    arg += 3;
    if (*arg == '+') { arg++; verify_jit = TRUE; }
    force_study = 1;
    if (*arg == 0)
      force_study_options = jit_study_bits[6];
    else if (*arg >= '1' && *arg <= '7')
      force_study_options = jit_study_bits[*arg - '1'];
    else goto BAD_ARG;
    }
  else if (strcmp(arg, "-8") == 0)
    {
#ifdef SUPPORT_PCRE8
    pcre_mode = PCRE8_MODE;
#else
    printf("** This version of PCRE was built without 8-bit support\n");
    exit(1);
#endif
    }
  else if (strcmp(arg, "-16") == 0)
    {
#ifdef SUPPORT_PCRE16
    pcre_mode = PCRE16_MODE;
#else
    printf("** This version of PCRE was built without 16-bit support\n");
    exit(1);
#endif
    }
  else if (strcmp(arg, "-32") == 0)
    {
#ifdef SUPPORT_PCRE32
    pcre_mode = PCRE32_MODE;
#else
    printf("** This version of PCRE was built without 32-bit support\n");
    exit(1);
#endif
    }
  else if (strcmp(arg, "-q") == 0) quiet = 1;
  else if (strcmp(arg, "-b") == 0) debug = 1;
  else if (strcmp(arg, "-i") == 0) showinfo = 1;
  else if (strcmp(arg, "-d") == 0) showinfo = debug = 1;
  else if (strcmp(arg, "-M") == 0) default_find_match_limit = TRUE;
  else if (strcmp(arg, "-O") == 0) default_options |= PCRE_NO_AUTO_POSSESS;
#if !defined NODFA
  else if (strcmp(arg, "-dfa") == 0) all_use_dfa = 1;
#endif
  else if (strcmp(arg, "-o") == 0 && argc > 2 &&
      ((size_offsets = get_value((pcre_uint8 *)argv[op+1], &endptr)),
        *endptr == 0))
    {
    op++;
    argc--;
    }
  else if (strcmp(arg, "-t") == 0 || strcmp(arg, "-tm") == 0 ||
           strcmp(arg, "-T") == 0 || strcmp(arg, "-TM") == 0)
    {
    int temp;
    int both = arg[2] == 0;
    showtotaltimes = arg[1] == 'T';
    if (argc > 2 && (temp = get_value((pcre_uint8 *)argv[op+1], &endptr),
                     *endptr == 0))
      {
      timeitm = temp;
      op++;
      argc--;
      }
    else timeitm = LOOPREPEAT;
    if (both) timeit = timeitm;
    }
  else if (strcmp(arg, "-S") == 0 && argc > 2 &&
      ((stack_size = get_value((pcre_uint8 *)argv[op+1], &endptr)),
        *endptr == 0))
    {
#if defined(_WIN32) || defined(WIN32) || defined(__minix) || defined(NATIVE_ZOS) || defined(__VMS)
    printf("PCRE: -S not supported on this OS\n");
    exit(1);
#else
    int rc;
    struct rlimit rlim;
    getrlimit(RLIMIT_STACK, &rlim);
    rlim.rlim_cur = stack_size * 1024 * 1024;
    rc = setrlimit(RLIMIT_STACK, &rlim);
    if (rc != 0)
      {
    printf("PCRE: setrlimit() failed with error %d\n", rc);
    exit(1);
      }
    op++;
    argc--;
#endif
    }
#if !defined NOPOSIX
  else if (strcmp(arg, "-p") == 0) posix = 1;
#endif
  else if (strcmp(arg, "-C") == 0)
    {
    int rc;
    unsigned long int lrc;

    if (argc > 2)
      {
      if (strcmp(argv[op + 1], "linksize") == 0)
        {
        (void)PCRE_CONFIG(PCRE_CONFIG_LINK_SIZE, &rc);
        printf("%d\n", rc);
        yield = rc;

#ifdef __VMS
        vms_setsymbol("LINKSIZE",0,yield );
#endif
        }
      else if (strcmp(argv[op + 1], "pcre8") == 0)
        {
#ifdef SUPPORT_PCRE8
        printf("1\n");
        yield = 1;
#else
        printf("0\n");
        yield = 0;
#endif
#ifdef __VMS
        vms_setsymbol("PCRE8",0,yield );
#endif
        }
      else if (strcmp(argv[op + 1], "pcre16") == 0)
        {
#ifdef SUPPORT_PCRE16
        printf("1\n");
        yield = 1;
#else
        printf("0\n");
        yield = 0;
#endif
#ifdef __VMS
        vms_setsymbol("PCRE16",0,yield );
#endif
        }
      else if (strcmp(argv[op + 1], "pcre32") == 0)
        {
#ifdef SUPPORT_PCRE32
        printf("1\n");
        yield = 1;
#else
        printf("0\n");
        yield = 0;
#endif
#ifdef __VMS
        vms_setsymbol("PCRE32",0,yield );
#endif
        }
      else if (strcmp(argv[op + 1], "utf") == 0)
        {
#ifdef SUPPORT_PCRE8
        if (pcre_mode == PCRE8_MODE)
          (void)pcre_config(PCRE_CONFIG_UTF8, &rc);
#endif
#ifdef SUPPORT_PCRE16
        if (pcre_mode == PCRE16_MODE)
          (void)pcre16_config(PCRE_CONFIG_UTF16, &rc);
#endif
#ifdef SUPPORT_PCRE32
        if (pcre_mode == PCRE32_MODE)
          (void)pcre32_config(PCRE_CONFIG_UTF32, &rc);
#endif
        printf("%d\n", rc);
        yield = rc;
#ifdef __VMS
        vms_setsymbol("UTF",0,yield );
#endif
        }
      else if (strcmp(argv[op + 1], "ucp") == 0)
        {
        (void)PCRE_CONFIG(PCRE_CONFIG_UNICODE_PROPERTIES, &rc);
        printf("%d\n", rc);
        yield = rc;
        }
      else if (strcmp(argv[op + 1], "jit") == 0)
        {
        (void)PCRE_CONFIG(PCRE_CONFIG_JIT, &rc);
        printf("%d\n", rc);
        yield = rc;
        }
      else if (strcmp(argv[op + 1], "newline") == 0)
        {
        (void)PCRE_CONFIG(PCRE_CONFIG_NEWLINE, &rc);
        print_newline_config(rc, TRUE);
        }
      else if (strcmp(argv[op + 1], "bsr") == 0)
        {
        (void)PCRE_CONFIG(PCRE_CONFIG_BSR, &rc);
        printf("%s\n", rc? "ANYCRLF" : "ANY");
        }
      else if (strcmp(argv[op + 1], "ebcdic") == 0)
        {
#ifdef EBCDIC
        printf("1\n");
        yield = 1;
#else
        printf("0\n");
#endif
        }
      else if (strcmp(argv[op + 1], "ebcdic-nl") == 0)
        {
#ifdef EBCDIC
        printf("0x%02x\n", CHAR_LF);
#else
        printf("0\n");
#endif
        }
      else
        {
        printf("Unknown -C option: %s\n", argv[op + 1]);
        }
      goto EXIT;
      }

    /* No argument for -C: output all configuration information. */

    printf("PCRE version %s\n", version);
    printf("Compiled with\n");

#ifdef EBCDIC
    printf("  EBCDIC code support: LF is 0x%02x\n", CHAR_LF);
#endif

/* At least one of SUPPORT_PCRE8 and SUPPORT_PCRE16 will be set. If both
are set, either both UTFs are supported or both are not supported. */

#ifdef SUPPORT_PCRE8
    printf("  8-bit support\n");
    (void)pcre_config(PCRE_CONFIG_UTF8, &rc);
      printf ("  %sUTF-8 support\n", rc ? "" : "No ");
#endif
#ifdef SUPPORT_PCRE16
    printf("  16-bit support\n");
    (void)pcre16_config(PCRE_CONFIG_UTF16, &rc);
    printf ("  %sUTF-16 support\n", rc ? "" : "No ");
#endif
#ifdef SUPPORT_PCRE32
    printf("  32-bit support\n");
    (void)pcre32_config(PCRE_CONFIG_UTF32, &rc);
    printf ("  %sUTF-32 support\n", rc ? "" : "No ");
#endif

    (void)PCRE_CONFIG(PCRE_CONFIG_UNICODE_PROPERTIES, &rc);
    printf("  %sUnicode properties support\n", rc? "" : "No ");
    (void)PCRE_CONFIG(PCRE_CONFIG_JIT, &rc);
    if (rc)
      {
      const char *arch;
      (void)PCRE_CONFIG(PCRE_CONFIG_JITTARGET, (void *)(&arch));
      printf("  Just-in-time compiler support: %s\n", arch);
      }
    else
      printf("  No just-in-time compiler support\n");
    (void)PCRE_CONFIG(PCRE_CONFIG_NEWLINE, &rc);
    print_newline_config(rc, FALSE);
    (void)PCRE_CONFIG(PCRE_CONFIG_BSR, &rc);
    printf("  \\R matches %s\n", rc? "CR, LF, or CRLF only" :
                                     "all Unicode newlines");
    (void)PCRE_CONFIG(PCRE_CONFIG_LINK_SIZE, &rc);
    printf("  Internal link size = %d\n", rc);
    (void)PCRE_CONFIG(PCRE_CONFIG_POSIX_MALLOC_THRESHOLD, &rc);
    printf("  POSIX malloc threshold = %d\n", rc);
    (void)PCRE_CONFIG(PCRE_CONFIG_PARENS_LIMIT, &lrc);
    printf("  Parentheses nest limit = %ld\n", lrc);
    (void)PCRE_CONFIG(PCRE_CONFIG_MATCH_LIMIT, &lrc);
    printf("  Default match limit = %ld\n", lrc);
    (void)PCRE_CONFIG(PCRE_CONFIG_MATCH_LIMIT_RECURSION, &lrc);
    printf("  Default recursion depth limit = %ld\n", lrc);
    (void)PCRE_CONFIG(PCRE_CONFIG_STACKRECURSE, &rc);
    printf("  Match recursion uses %s", rc? "stack" : "heap");
    if (showstore)
      {
      PCRE_EXEC(stack_size, NULL, NULL, NULL, -999, -999, 0, NULL, 0);
      printf(": %sframe size = %d bytes", rc? "approximate " : "", -stack_size);
      }
    printf("\n");
    goto EXIT;
    }
  else if (strcmp(arg, "-help") == 0 ||
           strcmp(arg, "--help") == 0)
    {
    usage();
    goto EXIT;
    }
  else
    {
    BAD_ARG:
    printf("** Unknown or malformed option %s\n", arg);
    usage();
    yield = 1;
    goto EXIT;
    }
  op++;
  argc--;
  }

/* Get the store for the offsets vector, and remember what it was */

size_offsets_max = size_offsets;
offsets = (int *)malloc(size_offsets_max * sizeof(int));
if (offsets == NULL)
  {
  printf("** Failed to get %d bytes of memory for offsets vector\n",
    (int)(size_offsets_max * sizeof(int)));
  yield = 1;
  goto EXIT;
  }

/* Sort out the input and output files */

if (argc > 1)
  {
  infile = fopen(argv[op], INPUT_MODE);
  if (infile == NULL)
    {
    printf("** Failed to open %s\n", argv[op]);
    yield = 1;
    goto EXIT;
    }
  }

if (argc > 2)
  {
  outfile = fopen(argv[op+1], OUTPUT_MODE);
  if (outfile == NULL)
    {
    printf("** Failed to open %s\n", argv[op+1]);
    yield = 1;
    goto EXIT;
    }
  }

/* Set alternative malloc function */

#ifdef SUPPORT_PCRE8
pcre_malloc = new_malloc;
pcre_free = new_free;
pcre_stack_malloc = stack_malloc;
pcre_stack_free = stack_free;
#endif

#ifdef SUPPORT_PCRE16
pcre16_malloc = new_malloc;
pcre16_free = new_free;
pcre16_stack_malloc = stack_malloc;
pcre16_stack_free = stack_free;
#endif

#ifdef SUPPORT_PCRE32
pcre32_malloc = new_malloc;
pcre32_free = new_free;
pcre32_stack_malloc = stack_malloc;
pcre32_stack_free = stack_free;
#endif

/* Heading line unless quiet */

if (!quiet) fprintf(outfile, "PCRE version %s\n\n", version);

/* Main loop */

while (!done)
  {
  pcre *re = NULL;
  pcre_extra *extra = NULL;

#if !defined NOPOSIX  /* There are still compilers that require no indent */
  regex_t preg = { NULL, 0, 0} ;
  int do_posix = 0;
#endif

  const char *error;
  pcre_uint8 *markptr;
  pcre_uint8 *p, *pp, *ppp;
  pcre_uint8 *to_file = NULL;
  const pcre_uint8 *tables = NULL;
  unsigned long int get_options;
  unsigned long int true_size, true_study_size = 0;
  size_t size;
  int do_allcaps = 0;
  int do_mark = 0;
  int do_study = 0;
  int no_force_study = 0;
  int do_debug = debug;
  int do_G = 0;
  int do_g = 0;
  int do_showinfo = showinfo;
  int do_showrest = 0;
  int do_showcaprest = 0;
  int do_flip = 0;
  int erroroffset, len, delimiter, poffset;

#if !defined NODFA
  int dfa_matched = 0;
#endif

  use_utf = 0;
  debug_lengths = 1;
  SET_PCRE_STACK_GUARD(NULL);

  if (extend_inputline(infile, buffer, "  re> ") == NULL) break;
  if (infile != stdin) fprintf(outfile, "%s", (char *)buffer);
  fflush(outfile);

  p = buffer;
  while (isspace(*p)) p++;
  if (*p == 0) continue;

  /* Handle option lock-out setting */

  if (*p == '<' && p[1] == ' ')
    {
    p += 2;
    while (isspace(*p)) p++;
    if (strncmp((char *)p, "forbid ", 7) == 0)
      {
      p += 7;
      while (isspace(*p)) p++;
      pp = lockout;
      while (!isspace(*p) && pp < lockout + sizeof(lockout) - 1)
        *pp++ = *p++;
      *pp = 0;
      }
    else
      {
      printf("** Unrecognized special command '%s'\n", p);
      yield = 1;
      goto EXIT;
      }
    continue;
    }

  /* See if the pattern is to be loaded pre-compiled from a file. */

  if (*p == '<' && strchr((char *)(p+1), '<') == NULL)
    {
    pcre_uint32 magic;
    pcre_uint8 sbuf[8];
    FILE *f;

    p++;
    if (*p == '!')
      {
      do_debug = TRUE;
      do_showinfo = TRUE;
      p++;
      }

    pp = p + (int)strlen((char *)p);
    while (isspace(pp[-1])) pp--;
    *pp = 0;

    f = fopen((char *)p, "rb");
    if (f == NULL)
      {
      fprintf(outfile, "Failed to open %s: %s\n", p, strerror(errno));
      continue;
      }
    if (fread(sbuf, 1, 8, f) != 8) goto FAIL_READ;

    true_size =
      (sbuf[0] << 24) | (sbuf[1] << 16) | (sbuf[2] << 8) | sbuf[3];
    true_study_size =
      (sbuf[4] << 24) | (sbuf[5] << 16) | (sbuf[6] << 8) | sbuf[7];

    re = (pcre *)new_malloc(true_size);
    if (re == NULL)
      {
      printf("** Failed to get %d bytes of memory for pcre object\n",
        (int)true_size);
      yield = 1;
      goto EXIT;
      }
    if (fread(re, 1, true_size, f) != true_size) goto FAIL_READ;

    magic = REAL_PCRE_MAGIC(re);
    if (magic != MAGIC_NUMBER)
      {
      if (swap_uint32(magic) == MAGIC_NUMBER)
        {
        do_flip = 1;
        }
      else
        {
        fprintf(outfile, "Data in %s is not a compiled PCRE regex\n", p);
        new_free(re);
        fclose(f);
        continue;
        }
      }

    /* We hide the byte-invert info for little and big endian tests. */
    fprintf(outfile, "Compiled pattern%s loaded from %s\n",
      do_flip && (p[-1] == '<') ? " (byte-inverted)" : "", p);

    /* Now see if there is any following study data. */

    if (true_study_size != 0)
      {
      pcre_study_data *psd;

      extra = (pcre_extra *)new_malloc(sizeof(pcre_extra) + true_study_size);
      extra->flags = PCRE_EXTRA_STUDY_DATA;

      psd = (pcre_study_data *)(((char *)extra) + sizeof(pcre_extra));
      extra->study_data = psd;

      if (fread(psd, 1, true_study_size, f) != true_study_size)
        {
        FAIL_READ:
        fprintf(outfile, "Failed to read data from %s\n", p);
        if (extra != NULL)
          {
          PCRE_FREE_STUDY(extra);
          }
        new_free(re);
        fclose(f);
        continue;
        }
      fprintf(outfile, "Study data loaded from %s\n", p);
      do_study = 1;     /* To get the data output if requested */
      }
    else fprintf(outfile, "No study data\n");

    /* Flip the necessary bytes. */
    if (do_flip)
      {
      int rc;
      PCRE_PATTERN_TO_HOST_BYTE_ORDER(rc, re, extra, NULL);
      if (rc == PCRE_ERROR_BADMODE)
        {
        pcre_uint32 flags_in_host_byte_order;
        if (REAL_PCRE_MAGIC(re) == MAGIC_NUMBER)
          flags_in_host_byte_order = REAL_PCRE_FLAGS(re);
        else
          flags_in_host_byte_order = swap_uint32(REAL_PCRE_FLAGS(re));
        /* Simulate the result of the function call below. */
        fprintf(outfile, "Error %d from pcre%s_fullinfo(%d)\n", rc,
          pcre_mode == PCRE32_MODE ? "32" : pcre_mode == PCRE16_MODE ? "16" : "",
          PCRE_INFO_OPTIONS);
        fprintf(outfile, "Running in %d-bit mode but pattern was compiled in "
          "%d-bit mode\n", 8 * CHAR_SIZE, 8 * (flags_in_host_byte_order & PCRE_MODE_MASK));
        new_free(re);
        fclose(f);
        continue;
        }
      }

    /* Need to know if UTF-8 for printing data strings. */

    if (new_info(re, NULL, PCRE_INFO_OPTIONS, &get_options) < 0)
      {
      new_free(re);
      fclose(f);
      continue;
      }
    use_utf = (get_options & PCRE_UTF8) != 0;

    fclose(f);
    goto SHOW_INFO;
    }

  /* In-line pattern (the usual case). Get the delimiter and seek the end of
  the pattern; if it isn't complete, read more. */

  delimiter = *p++;

  if (isalnum(delimiter) || delimiter == '\\')
    {
    fprintf(outfile, "** Delimiter must not be alphanumeric or \\\n");
    goto SKIP_DATA;
    }

  pp = p;
  poffset = (int)(p - buffer);

  for(;;)
    {
    while (*pp != 0)
      {
      if (*pp == '\\' && pp[1] != 0) pp++;
        else if (*pp == delimiter) break;
      pp++;
      }
    if (*pp != 0) break;
    if ((pp = extend_inputline(infile, pp, "    > ")) == NULL)
      {
      fprintf(outfile, "** Unexpected EOF\n");
      done = 1;
      goto CONTINUE;
      }
    if (infile != stdin) fprintf(outfile, "%s", (char *)pp);
    }

  /* The buffer may have moved while being extended; reset the start of data
  pointer to the correct relative point in the buffer. */

  p = buffer + poffset;

  /* If the first character after the delimiter is backslash, make
  the pattern end with backslash. This is purely to provide a way
  of testing for the error message when a pattern ends with backslash. */

  if (pp[1] == '\\') *pp++ = '\\';

  /* Terminate the pattern at the delimiter, and save a copy of the pattern
  for callouts. */

  *pp++ = 0;
  strcpy((char *)pbuffer, (char *)p);

  /* Look for modifiers and options after the final delimiter. */

  options = default_options;
  study_options = force_study_options;
  log_store = showstore;  /* default from command line */

  while (*pp != 0)
    {
    /* Check to see whether this modifier has been locked out for this file.
    This is complicated for the multi-character options that begin with '<'.
    If there is no '>' in the lockout string, all multi-character modifiers are
    locked out. */

    if (strchr((char *)lockout, *pp) != NULL)
      {
      if (*pp == '<' && strchr((char *)lockout, '>') != NULL)
        {
        int x = check_mc_option(pp+1, outfile, FALSE, "modifier");
        if (x == 0) goto SKIP_DATA;

        for (ppp = lockout; *ppp != 0; ppp++)
          {
          if (*ppp == '<')
            {
            int y = check_mc_option(ppp+1, outfile, FALSE, "modifier");
            if (y == 0)
              {
              printf("** Error in modifier forbid data - giving up.\n");
              yield = 1;
              goto EXIT;
              }
            if (x == y)
              {
              ppp = pp;
              while (*ppp != '>') ppp++;
              printf("** The %.*s modifier is locked out - giving up.\n",
                (int)(ppp - pp + 1), pp);
              yield = 1;
              goto EXIT;
              }
            }
          }
        }

      /* The single-character modifiers are straightforward. */

      else
        {
        printf("** The /%c modifier is locked out - giving up.\n", *pp);
        yield = 1;
        goto EXIT;
        }
      }

    /* The modifier is not locked out; handle it. */

    switch (*pp++)
      {
      case 'f': options |= PCRE_FIRSTLINE; break;
      case 'g': do_g = 1; break;
      case 'i': options |= PCRE_CASELESS; break;
      case 'm': options |= PCRE_MULTILINE; break;
      case 's': options |= PCRE_DOTALL; break;
      case 'x': options |= PCRE_EXTENDED; break;

      case '+':
      if (do_showrest) do_showcaprest = 1; else do_showrest = 1;
      break;

      case '=': do_allcaps = 1; break;
      case 'A': options |= PCRE_ANCHORED; break;
      case 'B': do_debug = 1; break;
      case 'C': options |= PCRE_AUTO_CALLOUT; break;
      case 'D': do_debug = do_showinfo = 1; break;
      case 'E': options |= PCRE_DOLLAR_ENDONLY; break;
      case 'F': do_flip = 1; break;
      case 'G': do_G = 1; break;
      case 'I': do_showinfo = 1; break;
      case 'J': options |= PCRE_DUPNAMES; break;
      case 'K': do_mark = 1; break;
      case 'M': log_store = 1; break;
      case 'N': options |= PCRE_NO_AUTO_CAPTURE; break;
      case 'O': options |= PCRE_NO_AUTO_POSSESS; break;

#if !defined NOPOSIX
      case 'P': do_posix = 1; break;
#endif

      case 'Q':
      switch (*pp)
        {
        case '0':
        case '1':
        stack_guard_return = *pp++ - '0';
        break;

        default:
        fprintf(outfile, "** Missing 0 or 1 after /Q\n");
        goto SKIP_DATA;
        }
      SET_PCRE_STACK_GUARD(stack_guard);
      break;

      case 'S':
      do_study = 1;
      for (;;)
        {
        switch (*pp++)
          {
          case 'S':
          do_study = 0;
          no_force_study = 1;
          break;

          case '!':
          study_options |= PCRE_STUDY_EXTRA_NEEDED;
          break;

          case '+':
          if (*pp == '+')
            {
            verify_jit = TRUE;
            pp++;
            }
          if (*pp >= '1' && *pp <= '7')
            study_options |= jit_study_bits[*pp++ - '1'];
          else
            study_options |= jit_study_bits[6];
          break;

          case '-':
          study_options &= ~PCRE_STUDY_ALLJIT;
          break;

          default:
          pp--;
          goto ENDLOOP;
          }
        }
      ENDLOOP:
      break;

      case 'U': options |= PCRE_UNGREEDY; break;
      case 'W': options |= PCRE_UCP; break;
      case 'X': options |= PCRE_EXTRA; break;
      case 'Y': options |= PCRE_NO_START_OPTIMISE; break;
      case 'Z': debug_lengths = 0; break;
      case '8': options |= PCRE_UTF8; use_utf = 1; break;
      case '9': options |= PCRE_NEVER_UTF; break;
      case '?': options |= PCRE_NO_UTF8_CHECK; break;

      case 'T':
      switch (*pp++)
        {
        case '0': tables = tables0; break;
        case '1': tables = tables1; break;

        case '\r':
        case '\n':
        case ' ':
        case 0:
        fprintf(outfile, "** Missing table number after /T\n");
        goto SKIP_DATA;

        default:
        fprintf(outfile, "** Bad table number \"%c\" after /T\n", pp[-1]);
        goto SKIP_DATA;
        }
      break;

      case 'L':
      ppp = pp;
      /* The '\r' test here is so that it works on Windows. */
      /* The '0' test is just in case this is an unterminated line. */
      while (*ppp != 0 && *ppp != '\n' && *ppp != '\r' && *ppp != ' ') ppp++;
      *ppp = 0;
      if (setlocale(LC_CTYPE, (const char *)pp) == NULL)
        {
        fprintf(outfile, "** Failed to set locale \"%s\"\n", pp);
        goto SKIP_DATA;
        }
      locale_set = 1;
      tables = PCRE_MAKETABLES;
      pp = ppp;
      break;

      case '>':
      to_file = pp;
      while (*pp != 0) pp++;
      while (isspace(pp[-1])) pp--;
      *pp = 0;
      break;

      case '<':
        {
        int x = check_mc_option(pp, outfile, FALSE, "modifier");
        if (x == 0) goto SKIP_DATA;
        options |= x;
        while (*pp++ != '>');
        }
      break;

      case '\r':                      /* So that it works in Windows */
      case '\n':
      case ' ':
      break;

      default:
      fprintf(outfile, "** Unknown modifier '%c'\n", pp[-1]);
      goto SKIP_DATA;
      }
    }

  /* Handle compiling via the POSIX interface, which doesn't support the
  timing, showing, or debugging options, nor the ability to pass over
  local character tables. Neither does it have 16-bit support. */

#if !defined NOPOSIX
  if (posix || do_posix)
    {
    int rc;
    int cflags = 0;

    if ((options & PCRE_CASELESS) != 0) cflags |= REG_ICASE;
    if ((options & PCRE_MULTILINE) != 0) cflags |= REG_NEWLINE;
    if ((options & PCRE_DOTALL) != 0) cflags |= REG_DOTALL;
    if ((options & PCRE_NO_AUTO_CAPTURE) != 0) cflags |= REG_NOSUB;
    if ((options & PCRE_UTF8) != 0) cflags |= REG_UTF8;
    if ((options & PCRE_UCP) != 0) cflags |= REG_UCP;
    if ((options & PCRE_UNGREEDY) != 0) cflags |= REG_UNGREEDY;

    rc = regcomp(&preg, (char *)p, cflags);

    /* Compilation failed; go back for another re, skipping to blank line
    if non-interactive. */

    if (rc != 0)
      {
      (void)regerror(rc, &preg, (char *)buffer, buffer_size);
      fprintf(outfile, "Failed: POSIX code %d: %s\n", rc, buffer);
      goto SKIP_DATA;
      }
    }

  /* Handle compiling via the native interface */

  else
#endif  /* !defined NOPOSIX */

    {
    /* In 16- or 32-bit mode, convert the input. */

#ifdef SUPPORT_PCRE16
    if (pcre_mode == PCRE16_MODE)
      {
      switch(to16(FALSE, p, options & PCRE_UTF8, (int)strlen((char *)p)))
        {
        case -1:
        fprintf(outfile, "**Failed: invalid UTF-8 string cannot be "
          "converted to UTF-16\n");
        goto SKIP_DATA;

        case -2:
        fprintf(outfile, "**Failed: character value greater than 0x10ffff "
          "cannot be converted to UTF-16\n");
        goto SKIP_DATA;

        case -3: /* "Impossible error" when to16 is called arg1 FALSE */
        fprintf(outfile, "**Failed: character value greater than 0xffff "
          "cannot be converted to 16-bit in non-UTF mode\n");
        goto SKIP_DATA;

        default:
        break;
        }
      p = (pcre_uint8 *)buffer16;
      }
#endif

#ifdef SUPPORT_PCRE32
    if (pcre_mode == PCRE32_MODE)
      {
      switch(to32(FALSE, p, options & PCRE_UTF32, (int)strlen((char *)p)))
        {
        case -1:
        fprintf(outfile, "**Failed: invalid UTF-8 string cannot be "
          "converted to UTF-32\n");
        goto SKIP_DATA;

        case -2:
        fprintf(outfile, "**Failed: character value greater than 0x10ffff "
          "cannot be converted to UTF-32\n");
        goto SKIP_DATA;

        case -3:
        fprintf(outfile, "**Failed: character value is ill-formed UTF-32\n");
        goto SKIP_DATA;

        default:
        break;
        }
      p = (pcre_uint8 *)buffer32;
      }
#endif

    /* Compile many times when timing */

    if (timeit > 0)
      {
      register int i;
      clock_t time_taken;
      clock_t start_time = clock();
      for (i = 0; i < timeit; i++)
        {
        PCRE_COMPILE(re, p, options, &error, &erroroffset, tables);
        if (re != NULL) free(re);
        }
      total_compile_time += (time_taken = clock() - start_time);
      fprintf(outfile, "Compile time %.4f milliseconds\n",
        (((double)time_taken * 1000.0) / (double)timeit) /
          (double)CLOCKS_PER_SEC);
      }

    PCRE_COMPILE(re, p, options, &error, &erroroffset, tables);

    /* Compilation failed; go back for another re, skipping to blank line
    if non-interactive. */

    if (re == NULL)
      {
      fprintf(outfile, "Failed: %s at offset %d\n", error, erroroffset);
      SKIP_DATA:
      if (infile != stdin)
        {
        for (;;)
          {
          if (extend_inputline(infile, buffer, NULL) == NULL)
            {
            done = 1;
            goto CONTINUE;
            }
          len = (int)strlen((char *)buffer);
          while (len > 0 && isspace(buffer[len-1])) len--;
          if (len == 0) break;
          }
        fprintf(outfile, "\n");
        }
      goto CONTINUE;
      }

    /* Compilation succeeded. It is now possible to set the UTF-8 option from
    within the regex; check for this so that we know how to process the data
    lines. */

    if (new_info(re, NULL, PCRE_INFO_OPTIONS, &get_options) < 0)
      goto SKIP_DATA;
    if ((get_options & PCRE_UTF8) != 0) use_utf = 1;

    /* Extract the size for possible writing before possibly flipping it,
    and remember the store that was got. */

    true_size = REAL_PCRE_SIZE(re);

    /* Output code size information if requested */

    if (log_store)
      {
      int name_count, name_entry_size, real_pcre_size;

      new_info(re, NULL, PCRE_INFO_NAMECOUNT, &name_count);
      new_info(re, NULL, PCRE_INFO_NAMEENTRYSIZE, &name_entry_size);
      real_pcre_size = 0;
#ifdef SUPPORT_PCRE8
      if (REAL_PCRE_FLAGS(re) & PCRE_MODE8)
        real_pcre_size = sizeof(real_pcre);
#endif
#ifdef SUPPORT_PCRE16
      if (REAL_PCRE_FLAGS(re) & PCRE_MODE16)
        real_pcre_size = sizeof(real_pcre16);
#endif
#ifdef SUPPORT_PCRE32
      if (REAL_PCRE_FLAGS(re) & PCRE_MODE32)
        real_pcre_size = sizeof(real_pcre32);
#endif
      new_info(re, NULL, PCRE_INFO_SIZE, &size);
      fprintf(outfile, "Memory allocation (code space): %d\n",
        (int)(size - real_pcre_size - name_count * name_entry_size));
      }

    /* If -s or /S was present, study the regex to generate additional info to
    help with the matching, unless the pattern has the SS option, which
    suppresses the effect of /S (used for a few test patterns where studying is
    never sensible). */

    if (do_study || (force_study >= 0 && !no_force_study))
      {
      if (timeit > 0)
        {
        register int i;
        clock_t time_taken;
        clock_t start_time = clock();
        for (i = 0; i < timeit; i++)
          {
          PCRE_STUDY(extra, re, study_options, &error);
          }
        total_study_time = (time_taken = clock() - start_time);
        if (extra != NULL)
          {
          PCRE_FREE_STUDY(extra);
          }
        fprintf(outfile, "  Study time %.4f milliseconds\n",
          (((double)time_taken * 1000.0) / (double)timeit) /
            (double)CLOCKS_PER_SEC);
        }
      PCRE_STUDY(extra, re, study_options, &error);
      if (error != NULL)
        fprintf(outfile, "Failed to study: %s\n", error);
      else if (extra != NULL)
        {
        true_study_size = ((pcre_study_data *)(extra->study_data))->size;
        if (log_store)
          {
          size_t jitsize;
          if (new_info(re, extra, PCRE_INFO_JITSIZE, &jitsize) == 0 &&
              jitsize != 0)
            fprintf(outfile, "Memory allocation (JIT code): %d\n", (int)jitsize);
          }
        }
      }

    /* If /K was present, we set up for handling MARK data. */

    if (do_mark)
      {
      if (extra == NULL)
        {
        extra = (pcre_extra *)malloc(sizeof(pcre_extra));
        extra->flags = 0;
        }
      extra->mark = &markptr;
      extra->flags |= PCRE_EXTRA_MARK;
      }

    /* Extract and display information from the compiled data if required. */

    SHOW_INFO:

    if (do_debug)
      {
      fprintf(outfile, "------------------------------------------------------------------\n");
      PCRE_PRINTINT(re, outfile, debug_lengths);
      }

    /* We already have the options in get_options (see above) */

    if (do_showinfo)
      {
      unsigned long int all_options;
      pcre_uint32 first_char, need_char;
      pcre_uint32 match_limit, recursion_limit;
      int count, backrefmax, first_char_set, need_char_set, okpartial, jchanged,
        hascrorlf, maxlookbehind, match_empty;
      int nameentrysize, namecount;
      const pcre_uint8 *nametable;

      if (new_info(re, NULL, PCRE_INFO_CAPTURECOUNT, &count) +
          new_info(re, NULL, PCRE_INFO_BACKREFMAX, &backrefmax) +
          new_info(re, NULL, PCRE_INFO_FIRSTCHARACTER, &first_char) +
          new_info(re, NULL, PCRE_INFO_FIRSTCHARACTERFLAGS, &first_char_set) +
          new_info(re, NULL, PCRE_INFO_REQUIREDCHAR, &need_char) +
          new_info(re, NULL, PCRE_INFO_REQUIREDCHARFLAGS, &need_char_set) +
          new_info(re, NULL, PCRE_INFO_NAMEENTRYSIZE, &nameentrysize) +
          new_info(re, NULL, PCRE_INFO_NAMECOUNT, &namecount) +
          new_info(re, NULL, PCRE_INFO_NAMETABLE, (void *)&nametable) +
          new_info(re, NULL, PCRE_INFO_OKPARTIAL, &okpartial) +
          new_info(re, NULL, PCRE_INFO_JCHANGED, &jchanged) +
          new_info(re, NULL, PCRE_INFO_HASCRORLF, &hascrorlf) +
          new_info(re, NULL, PCRE_INFO_MATCH_EMPTY, &match_empty) +
          new_info(re, NULL, PCRE_INFO_MAXLOOKBEHIND, &maxlookbehind)
          != 0)
        goto SKIP_DATA;

      fprintf(outfile, "Capturing subpattern count = %d\n", count);

      if (backrefmax > 0)
        fprintf(outfile, "Max back reference = %d\n", backrefmax);

      if (maxlookbehind > 0)
        fprintf(outfile, "Max lookbehind = %d\n", maxlookbehind);

      if (new_info(re, NULL, PCRE_INFO_MATCHLIMIT, &match_limit) == 0)
        fprintf(outfile, "Match limit = %u\n", match_limit);

      if (new_info(re, NULL, PCRE_INFO_RECURSIONLIMIT, &recursion_limit) == 0)
        fprintf(outfile, "Recursion limit = %u\n", recursion_limit);

      if (namecount > 0)
        {
        fprintf(outfile, "Named capturing subpatterns:\n");
        while (namecount-- > 0)
          {
          int imm2_size = pcre_mode == PCRE8_MODE ? 2 : 1;
          int length = (int)STRLEN(nametable + imm2_size);
          fprintf(outfile, "  ");
          PCHARSV(nametable, imm2_size, length, outfile);
          while (length++ < nameentrysize - imm2_size) putc(' ', outfile);
#ifdef SUPPORT_PCRE32
          if (pcre_mode == PCRE32_MODE)
            fprintf(outfile, "%3d\n", (int)(((PCRE_SPTR32)nametable)[0]));
#endif
#ifdef SUPPORT_PCRE16
          if (pcre_mode == PCRE16_MODE)
            fprintf(outfile, "%3d\n", (int)(((PCRE_SPTR16)nametable)[0]));
#endif
#ifdef SUPPORT_PCRE8
          if (pcre_mode == PCRE8_MODE)
            fprintf(outfile, "%3d\n", ((int)nametable[0] << 8) | (int)nametable[1]);
#endif
          nametable += nameentrysize * CHAR_SIZE;
          }
        }

      if (!okpartial)  fprintf(outfile, "Partial matching not supported\n");
      if (hascrorlf)   fprintf(outfile, "Contains explicit CR or LF match\n");
      if (match_empty) fprintf(outfile, "May match empty string\n");

      all_options = REAL_PCRE_OPTIONS(re);
      if (do_flip) all_options = swap_uint32(all_options);

      if (get_options == 0) fprintf(outfile, "No options\n");
        else fprintf(outfile, "Options:%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
          ((get_options & PCRE_ANCHORED) != 0)? " anchored" : "",
          ((get_options & PCRE_CASELESS) != 0)? " caseless" : "",
          ((get_options & PCRE_EXTENDED) != 0)? " extended" : "",
          ((get_options & PCRE_MULTILINE) != 0)? " multiline" : "",
          ((get_options & PCRE_FIRSTLINE) != 0)? " firstline" : "",
          ((get_options & PCRE_DOTALL) != 0)? " dotall" : "",
          ((get_options & PCRE_BSR_ANYCRLF) != 0)? " bsr_anycrlf" : "",
          ((get_options & PCRE_BSR_UNICODE) != 0)? " bsr_unicode" : "",
          ((get_options & PCRE_DOLLAR_ENDONLY) != 0)? " dollar_endonly" : "",
          ((get_options & PCRE_EXTRA) != 0)? " extra" : "",
          ((get_options & PCRE_UNGREEDY) != 0)? " ungreedy" : "",
          ((get_options & PCRE_NO_AUTO_CAPTURE) != 0)? " no_auto_capture" : "",
          ((get_options & PCRE_NO_AUTO_POSSESS) != 0)? " no_auto_possessify" : "",
          ((get_options & PCRE_UTF8) != 0)? " utf" : "",
          ((get_options & PCRE_UCP) != 0)? " ucp" : "",
          ((get_options & PCRE_NO_UTF8_CHECK) != 0)? " no_utf_check" : "",
          ((get_options & PCRE_NO_START_OPTIMIZE) != 0)? " no_start_optimize" : "",
          ((get_options & PCRE_DUPNAMES) != 0)? " dupnames" : "",
          ((get_options & PCRE_NEVER_UTF) != 0)? " never_utf" : "");

      if (jchanged) fprintf(outfile, "Duplicate name status changes\n");

      switch (get_options & PCRE_NEWLINE_BITS)
        {
        case PCRE_NEWLINE_CR:
        fprintf(outfile, "Forced newline sequence: CR\n");
        break;

        case PCRE_NEWLINE_LF:
        fprintf(outfile, "Forced newline sequence: LF\n");
        break;

        case PCRE_NEWLINE_CRLF:
        fprintf(outfile, "Forced newline sequence: CRLF\n");
        break;

        case PCRE_NEWLINE_ANYCRLF:
        fprintf(outfile, "Forced newline sequence: ANYCRLF\n");
        break;

        case PCRE_NEWLINE_ANY:
        fprintf(outfile, "Forced newline sequence: ANY\n");
        break;

        default:
        break;
        }

      if (first_char_set == 2)
        {
        fprintf(outfile, "First char at start or follows newline\n");
        }
      else if (first_char_set == 1)
        {
        const char *caseless =
          ((REAL_PCRE_FLAGS(re) & PCRE_FCH_CASELESS) == 0)?
          "" : " (caseless)";

        if (PRINTOK(first_char))
          fprintf(outfile, "First char = \'%c\'%s\n", first_char, caseless);
        else
          {
          fprintf(outfile, "First char = ");
          pchar(first_char, outfile);
          fprintf(outfile, "%s\n", caseless);
          }
        }
      else
        {
        fprintf(outfile, "No first char\n");
        }

      if (need_char_set == 0)
        {
        fprintf(outfile, "No need char\n");
        }
      else
        {
        const char *caseless =
          ((REAL_PCRE_FLAGS(re) & PCRE_RCH_CASELESS) == 0)?
          "" : " (caseless)";

        if (PRINTOK(need_char))
          fprintf(outfile, "Need char = \'%c\'%s\n", need_char, caseless);
        else
          {
          fprintf(outfile, "Need char = ");
          pchar(need_char, outfile);
          fprintf(outfile, "%s\n", caseless);
          }
        }

      /* Don't output study size; at present it is in any case a fixed
      value, but it varies, depending on the computer architecture, and
      so messes up the test suite. (And with the /F option, it might be
      flipped.) If study was forced by an external -s, don't show this
      information unless -i or -d was also present. This means that, except
      when auto-callouts are involved, the output from runs with and without
      -s should be identical. */

      if (do_study || (force_study >= 0 && showinfo && !no_force_study))
        {
        if (extra == NULL)
          fprintf(outfile, "Study returned NULL\n");
        else
          {
          pcre_uint8 *start_bits = NULL;
          int minlength;

          if (new_info(re, extra, PCRE_INFO_MINLENGTH, &minlength) == 0)
            fprintf(outfile, "Subject length lower bound = %d\n", minlength);

          if (new_info(re, extra, PCRE_INFO_FIRSTTABLE, &start_bits) == 0)
            {
            if (start_bits == NULL)
              fprintf(outfile, "No starting char list\n");
            else
              {
              int i;
              int c = 24;
              fprintf(outfile, "Starting chars: ");
              for (i = 0; i < 256; i++)
                {
                if ((start_bits[i/8] & (1<<(i&7))) != 0)
                  {
                  if (c > 75)
                    {
                    fprintf(outfile, "\n  ");
                    c = 2;
                    }
                  if (PRINTOK(i) && i != ' ')
                    {
                    fprintf(outfile, "%c ", i);
                    c += 2;
                    }
                  else
                    {
                    fprintf(outfile, "\\x%02x ", i);
                    c += 5;
                    }
                  }
                }
              fprintf(outfile, "\n");
              }
            }
          }

        /* Show this only if the JIT was set by /S, not by -s. */

        if ((study_options & PCRE_STUDY_ALLJIT) != 0 &&
            (force_study_options & PCRE_STUDY_ALLJIT) == 0)
          {
          int jit;
          if (new_info(re, extra, PCRE_INFO_JIT, &jit) == 0)
            {
            if (jit)
              fprintf(outfile, "JIT study was successful\n");
            else
#ifdef SUPPORT_JIT
              fprintf(outfile, "JIT study was not successful\n");
#else
              fprintf(outfile, "JIT support is not available in this version of PCRE\n");
#endif
            }
          }
        }
      }

    /* If the '>' option was present, we write out the regex to a file, and
    that is all. The first 8 bytes of the file are the regex length and then
    the study length, in big-endian order. */

    if (to_file != NULL)
      {
      FILE *f = fopen((char *)to_file, "wb");
      if (f == NULL)
        {
        fprintf(outfile, "Unable to open %s: %s\n", to_file, strerror(errno));
        }
      else
        {
        pcre_uint8 sbuf[8];

        if (do_flip) regexflip(re, extra);
        sbuf[0] = (pcre_uint8)((true_size >> 24) & 255);
        sbuf[1] = (pcre_uint8)((true_size >> 16) & 255);
        sbuf[2] = (pcre_uint8)((true_size >>  8) & 255);
        sbuf[3] = (pcre_uint8)((true_size) & 255);
        sbuf[4] = (pcre_uint8)((true_study_size >> 24) & 255);
        sbuf[5] = (pcre_uint8)((true_study_size >> 16) & 255);
        sbuf[6] = (pcre_uint8)((true_study_size >>  8) & 255);
        sbuf[7] = (pcre_uint8)((true_study_size) & 255);

        if (fwrite(sbuf, 1, 8, f) < 8 ||
            fwrite(re, 1, true_size, f) < true_size)
          {
          fprintf(outfile, "Write error on %s: %s\n", to_file, strerror(errno));
          }
        else
          {
          fprintf(outfile, "Compiled pattern written to %s\n", to_file);

          /* If there is study data, write it. */

          if (extra != NULL)
            {
            if (fwrite(extra->study_data, 1, true_study_size, f) <
                true_study_size)
              {
              fprintf(outfile, "Write error on %s: %s\n", to_file,
                strerror(errno));
              }
            else fprintf(outfile, "Study data written to %s\n", to_file);
            }
          }
        fclose(f);
        }

      new_free(re);
      if (extra != NULL)
        {
        PCRE_FREE_STUDY(extra);
        }
      if (locale_set)
        {
        new_free((void *)tables);
        setlocale(LC_CTYPE, "C");
        locale_set = 0;
        }
      continue;  /* With next regex */
      }
    }        /* End of non-POSIX compile */

  /* Read data lines and test them */

  for (;;)
    {
#ifdef SUPPORT_PCRE8
    pcre_uint8 *q8;
#endif
#ifdef SUPPORT_PCRE16
    pcre_uint16 *q16;
#endif
#ifdef SUPPORT_PCRE32
    pcre_uint32 *q32;
#endif
    pcre_uint8 *bptr;
    int *use_offsets = offsets;
    int use_size_offsets = size_offsets;
    int callout_data = 0;
    int callout_data_set = 0;
    int count;
    pcre_uint32 c;
    int copystrings = 0;
    int find_match_limit = default_find_match_limit;
    int getstrings = 0;
    int getlist = 0;
    int gmatched = 0;
    int start_offset = 0;
    int start_offset_sign = 1;
    int g_notempty = 0;
    int use_dfa = 0;

    *copynames = 0;
    *getnames = 0;

#ifdef SUPPORT_PCRE32
    cn32ptr = copynames;
    gn32ptr = getnames;
#endif
#ifdef SUPPORT_PCRE16
    cn16ptr = copynames16;
    gn16ptr = getnames16;
#endif
#ifdef SUPPORT_PCRE8
    cn8ptr = copynames8;
    gn8ptr = getnames8;
#endif

    SET_PCRE_CALLOUT(callout);
    first_callout = 1;
    last_callout_mark = NULL;
    callout_extra = 0;
    callout_count = 0;
    callout_fail_count = 999999;
    callout_fail_id = -1;
    show_malloc = 0;
    options = 0;

    if (extra != NULL) extra->flags &=
      ~(PCRE_EXTRA_MATCH_LIMIT|PCRE_EXTRA_MATCH_LIMIT_RECURSION);

    len = 0;
    for (;;)
      {
      if (extend_inputline(infile, buffer + len, "data> ") == NULL)
        {
        if (len > 0)    /* Reached EOF without hitting a newline */
          {
          fprintf(outfile, "\n");
          break;
          }
        done = 1;
        goto CONTINUE;
        }
      if (infile != stdin) fprintf(outfile, "%s", (char *)buffer);
      len = (int)strlen((char *)buffer);
      if (buffer[len-1] == '\n') break;
      }

    while (len > 0 && isspace(buffer[len-1])) len--;
    buffer[len] = 0;
    if (len == 0) break;

    p = buffer;
    while (isspace(*p)) p++;

#ifndef NOUTF
    /* Check that the data is well-formed UTF-8 if we're in UTF mode. To create
    invalid input to pcre_exec, you must use \x?? or \x{} sequences. */

    if (use_utf)
      {
      pcre_uint8 *q;
      pcre_uint32 cc;
      int n = 1;

      for (q = p; n > 0 && *q; q += n) n = utf82ord(q, &cc);
      if (n <= 0)
        {
        fprintf(outfile, "**Failed: invalid UTF-8 string cannot be used as input in UTF mode\n");
        goto NEXT_DATA;
        }
      }
#endif

#ifdef SUPPORT_VALGRIND
    /* Mark the dbuffer as addressable but undefined again. */

    if (dbuffer != NULL)
      {
      VALGRIND_MAKE_MEM_UNDEFINED(dbuffer, dbuffer_size * CHAR_SIZE);
      }
#endif

    /* Allocate a buffer to hold the data line; len+1 is an upper bound on
    the number of pcre_uchar units that will be needed. */

    while (dbuffer == NULL || (size_t)len >= dbuffer_size)
      {
      dbuffer_size *= 2;
      dbuffer = (pcre_uint8 *)realloc(dbuffer, dbuffer_size * CHAR_SIZE);
      if (dbuffer == NULL)
        {
        fprintf(stderr, "pcretest: realloc(%d) failed\n", (int)dbuffer_size);
        exit(1);
        }
      }

#ifdef SUPPORT_PCRE8
    q8 = (pcre_uint8 *) dbuffer;
#endif
#ifdef SUPPORT_PCRE16
    q16 = (pcre_uint16 *) dbuffer;
#endif
#ifdef SUPPORT_PCRE32
    q32 = (pcre_uint32 *) dbuffer;
#endif

    while ((c = *p++) != 0)
      {
      int i = 0;
      int n = 0;

      /* In UTF mode, input can be UTF-8, so just copy all non-backslash bytes.
      In non-UTF mode, allow the value of the byte to fall through to later,
      where values greater than 127 are turned into UTF-8 when running in
      16-bit or 32-bit mode. */

      if (c != '\\')
        {
#ifndef NOUTF
        if (use_utf && HASUTF8EXTRALEN(c)) { GETUTF8INC(c, p); }
#endif
        }

      /* Handle backslash escapes */

      else switch ((c = *p++))
        {
        case 'a': c =  CHAR_BEL; break;
        case 'b': c = '\b'; break;
        case 'e': c =  CHAR_ESC; break;
        case 'f': c = '\f'; break;
        case 'n': c = '\n'; break;
        case 'r': c = '\r'; break;
        case 't': c = '\t'; break;
        case 'v': c = '\v'; break;

        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
        c -= '0';
        while (i++ < 2 && isdigit(*p) && *p != '8' && *p != '9')
          c = c * 8 + *p++ - '0';
        break;

        case 'o':
        if (*p == '{')
          {
          pcre_uint8 *pt = p;
          c = 0;
          for (pt++; isdigit(*pt) && *pt != '8' && *pt != '9'; pt++)
            {
            if (++i == 12)
              fprintf(outfile, "** Too many octal digits in \\o{...} item; "
                               "using only the first twelve.\n");
            else c = c * 8 + *pt - '0';
            }
          if (*pt == '}') p = pt + 1;
            else fprintf(outfile, "** Missing } after \\o{ (assumed)\n");
          }
        break;

        case 'x':
        if (*p == '{')
          {
          pcre_uint8 *pt = p;
          c = 0;

          /* We used to have "while (isxdigit(*(++pt)))" here, but it fails
          when isxdigit() is a macro that refers to its argument more than
          once. This is banned by the C Standard, but apparently happens in at
          least one MacOS environment. */

          for (pt++; isxdigit(*pt); pt++)
            {
            if (++i == 9)
              fprintf(outfile, "** Too many hex digits in \\x{...} item; "
                               "using only the first eight.\n");
            else c = c * 16 + tolower(*pt) - ((isdigit(*pt))? '0' : 'a' - 10);
            }
          if (*pt == '}')
            {
            p = pt + 1;
            break;
            }
          /* Not correct form for \x{...}; fall through */
          }

        /* \x without {} always defines just one byte in 8-bit mode. This
        allows UTF-8 characters to be constructed byte by byte, and also allows
        invalid UTF-8 sequences to be made. Just copy the byte in UTF mode.
        Otherwise, pass it down to later code so that it can be turned into
        UTF-8 when running in 16/32-bit mode. */

        c = 0;
        while (i++ < 2 && isxdigit(*p))
          {
          c = c * 16 + tolower(*p) - ((isdigit(*p))? '0' : 'a' - 10);
          p++;
          }
#if !defined NOUTF && defined SUPPORT_PCRE8
        if (use_utf && (pcre_mode == PCRE8_MODE))
          {
          *q8++ = c;
          continue;
          }
#endif
        break;

        case 0:   /* \ followed by EOF allows for an empty line */
        p--;
        continue;

        case '>':
        if (*p == '-')
          {
          start_offset_sign = -1;
          p++;
          }
        while(isdigit(*p)) start_offset = start_offset * 10 + *p++ - '0';
        start_offset *= start_offset_sign;
        continue;

        case 'A':  /* Option setting */
        options |= PCRE_ANCHORED;
        continue;

        case 'B':
        options |= PCRE_NOTBOL;
        continue;

        case 'C':
        if (isdigit(*p))    /* Set copy string */
          {
          while(isdigit(*p)) n = n * 10 + *p++ - '0';
          copystrings |= 1 << n;
          }
        else if (isalnum(*p))
          {
          READ_CAPTURE_NAME(p, &cn8ptr, &cn16ptr, &cn32ptr, re);
          }
        else if (*p == '+')
          {
          callout_extra = 1;
          p++;
          }
        else if (*p == '-')
          {
          SET_PCRE_CALLOUT(NULL);
          p++;
          }
        else if (*p == '!')
          {
          callout_fail_id = 0;
          p++;
          while(isdigit(*p))
            callout_fail_id = callout_fail_id * 10 + *p++ - '0';
          callout_fail_count = 0;
          if (*p == '!')
            {
            p++;
            while(isdigit(*p))
              callout_fail_count = callout_fail_count * 10 + *p++ - '0';
            }
          }
        else if (*p == '*')
          {
          int sign = 1;
          callout_data = 0;
          if (*(++p) == '-') { sign = -1; p++; }
          while(isdigit(*p))
            callout_data = callout_data * 10 + *p++ - '0';
          callout_data *= sign;
          callout_data_set = 1;
          }
        continue;

#if !defined NODFA
        case 'D':
#if !defined NOPOSIX
        if (posix || do_posix)
          printf("** Can't use dfa matching in POSIX mode: \\D ignored\n");
        else
#endif
          use_dfa = 1;
        continue;
#endif

#if !defined NODFA
        case 'F':
        options |= PCRE_DFA_SHORTEST;
        continue;
#endif

        case 'G':
        if (isdigit(*p))
          {
          while(isdigit(*p)) n = n * 10 + *p++ - '0';
          getstrings |= 1 << n;
          }
        else if (isalnum(*p))
          {
          READ_CAPTURE_NAME(p, &gn8ptr, &gn16ptr, &gn32ptr, re);
          }
        continue;

        case 'J':
        while(isdigit(*p)) n = n * 10 + *p++ - '0';
        if (extra != NULL
            && (extra->flags & PCRE_EXTRA_EXECUTABLE_JIT) != 0
            && extra->executable_jit != NULL)
          {
          if (jit_stack != NULL) { PCRE_JIT_STACK_FREE(jit_stack); }
          jit_stack = PCRE_JIT_STACK_ALLOC(1, n * 1024);
          PCRE_ASSIGN_JIT_STACK(extra, jit_callback, jit_stack);
          }
        continue;

        case 'L':
        getlist = 1;
        continue;

        case 'M':
        find_match_limit = 1;
        continue;

        case 'N':
        if ((options & PCRE_NOTEMPTY) != 0)
          options = (options & ~PCRE_NOTEMPTY) | PCRE_NOTEMPTY_ATSTART;
        else
          options |= PCRE_NOTEMPTY;
        continue;

        case 'O':
        while(isdigit(*p)) n = n * 10 + *p++ - '0';
        if (n > size_offsets_max)
          {
          size_offsets_max = n;
          free(offsets);
          use_offsets = offsets = (int *)malloc(size_offsets_max * sizeof(int));
          if (offsets == NULL)
            {
            printf("** Failed to get %d bytes of memory for offsets vector\n",
              (int)(size_offsets_max * sizeof(int)));
            yield = 1;
            goto EXIT;
            }
          }
        use_size_offsets = n;
        if (n == 0) use_offsets = NULL;   /* Ensures it can't write to it */
          else use_offsets = offsets + size_offsets_max - n;  /* To catch overruns */
        continue;

        case 'P':
        options |= ((options & PCRE_PARTIAL_SOFT) == 0)?
          PCRE_PARTIAL_SOFT : PCRE_PARTIAL_HARD;
        continue;

        case 'Q':
        while(isdigit(*p)) n = n * 10 + *p++ - '0';
        if (extra == NULL)
          {
          extra = (pcre_extra *)malloc(sizeof(pcre_extra));
          extra->flags = 0;
          }
        extra->flags |= PCRE_EXTRA_MATCH_LIMIT_RECURSION;
        extra->match_limit_recursion = n;
        continue;

        case 'q':
        while(isdigit(*p)) n = n * 10 + *p++ - '0';
        if (extra == NULL)
          {
          extra = (pcre_extra *)malloc(sizeof(pcre_extra));
          extra->flags = 0;
          }
        extra->flags |= PCRE_EXTRA_MATCH_LIMIT;
        extra->match_limit = n;
        continue;

#if !defined NODFA
        case 'R':
        options |= PCRE_DFA_RESTART;
        continue;
#endif

        case 'S':
        show_malloc = 1;
        continue;

        case 'Y':
        options |= PCRE_NO_START_OPTIMIZE;
        continue;

        case 'Z':
        options |= PCRE_NOTEOL;
        continue;

        case '?':
        options |= PCRE_NO_UTF8_CHECK;
        continue;

        case '<':
          {
          int x = check_mc_option(p, outfile, TRUE, "escape sequence");
          if (x == 0) goto NEXT_DATA;
          options |= x;
          while (*p++ != '>');
          }
        continue;
        }

      /* We now have a character value in c that may be greater than 255.
      In 8-bit mode we convert to UTF-8 if we are in UTF mode. Values greater
      than 127 in UTF mode must have come from \x{...} or octal constructs
      because values from \x.. get this far only in non-UTF mode. */

#ifdef SUPPORT_PCRE8
      if (pcre_mode == PCRE8_MODE)
        {
#ifndef NOUTF
        if (use_utf)
          {
          if (c > 0x7fffffff)
            {
            fprintf(outfile, "** Character \\x{%x} is greater than 0x7fffffff "
              "and so cannot be converted to UTF-8\n", c);
            goto NEXT_DATA;
            }
          q8 += ord2utf8(c, q8);
          }
        else
#endif
          {
          if (c > 0xffu)
            {
            fprintf(outfile, "** Character \\x{%x} is greater than 255 "
              "and UTF-8 mode is not enabled.\n", c);
            fprintf(outfile, "** Truncation will probably give the wrong "
              "result.\n");
            }
          *q8++ = c;
          }
        }
#endif
#ifdef SUPPORT_PCRE16
      if (pcre_mode == PCRE16_MODE)
        {
#ifndef NOUTF
        if (use_utf)
          {
          if (c > 0x10ffffu)
            {
            fprintf(outfile, "** Failed: character \\x{%x} is greater than "
              "0x10ffff and so cannot be converted to UTF-16\n", c);
            goto NEXT_DATA;
            }
          else if (c >= 0x10000u)
            {
            c-= 0x10000u;
            *q16++ = 0xD800 | (c >> 10);
            *q16++ = 0xDC00 | (c & 0x3ff);
            }
          else
            *q16++ = c;
          }
        else
#endif
          {
          if (c > 0xffffu)
            {
            fprintf(outfile, "** Character \\x{%x} is greater than 0xffff "
              "and UTF-16 mode is not enabled.\n", c);
            fprintf(outfile, "** Truncation will probably give the wrong "
              "result.\n");
            }

          *q16++ = c;
          }
        }
#endif
#ifdef SUPPORT_PCRE32
      if (pcre_mode == PCRE32_MODE)
        {
        *q32++ = c;
        }
#endif

      }

    /* Reached end of subject string */

#ifdef SUPPORT_PCRE8
    if (pcre_mode == PCRE8_MODE)
    {
      *q8 = 0;
      len = (int)(q8 - (pcre_uint8 *)dbuffer);
    }
#endif
#ifdef SUPPORT_PCRE16
    if (pcre_mode == PCRE16_MODE)
    {
      *q16 = 0;
      len = (int)(q16 - (pcre_uint16 *)dbuffer);
    }
#endif
#ifdef SUPPORT_PCRE32
    if (pcre_mode == PCRE32_MODE)
    {
      *q32 = 0;
      len = (int)(q32 - (pcre_uint32 *)dbuffer);
    }
#endif

    /* If we're compiling with explicit valgrind support, Mark the data from after
    its end to the end of the buffer as unaddressable, so that a read over the end
    of the buffer will be seen by valgrind, even if it doesn't cause a crash.
    If we're not building with valgrind support, at least move the data to the end
    of the buffer so that it might at least cause a crash.
    If we are using the POSIX interface, we must include the terminating zero. */

    bptr = dbuffer;

#if !defined NOPOSIX
    if (posix || do_posix)
      {
#ifdef SUPPORT_VALGRIND
      VALGRIND_MAKE_MEM_NOACCESS(dbuffer + len + 1, dbuffer_size - (len + 1));
#else
      memmove(bptr + dbuffer_size - len - 1, bptr, len + 1);
      bptr += dbuffer_size - len - 1;
#endif
      }
    else
#endif
      {
#ifdef SUPPORT_VALGRIND
      VALGRIND_MAKE_MEM_NOACCESS(dbuffer + len * CHAR_SIZE, (dbuffer_size - len) * CHAR_SIZE);
#else
      bptr = memmove(bptr + (dbuffer_size - len) * CHAR_SIZE, bptr, len * CHAR_SIZE);
#endif
      }

    if ((all_use_dfa || use_dfa) && find_match_limit)
      {
      printf("**Match limit not relevant for DFA matching: ignored\n");
      find_match_limit = 0;
      }

    /* Handle matching via the POSIX interface, which does not
    support timing or playing with the match limit or callout data. */

#if !defined NOPOSIX
    if (posix || do_posix)
      {
      int rc;
      int eflags = 0;
      regmatch_t *pmatch = NULL;
      if (use_size_offsets > 0)
        pmatch = (regmatch_t *)malloc(sizeof(regmatch_t) * use_size_offsets);
      if ((options & PCRE_NOTBOL) != 0) eflags |= REG_NOTBOL;
      if ((options & PCRE_NOTEOL) != 0) eflags |= REG_NOTEOL;
      if ((options & PCRE_NOTEMPTY) != 0) eflags |= REG_NOTEMPTY;

      rc = regexec(&preg, (const char *)bptr, use_size_offsets, pmatch, eflags);

      if (rc != 0)
        {
        (void)regerror(rc, &preg, (char *)buffer, buffer_size);
        fprintf(outfile, "No match: POSIX code %d: %s\n", rc, buffer);
        }
      else if ((REAL_PCRE_OPTIONS(preg.re_pcre) & PCRE_NO_AUTO_CAPTURE) != 0)
        {
        fprintf(outfile, "Matched with REG_NOSUB\n");
        }
      else
        {
        size_t i;
        for (i = 0; i < (size_t)use_size_offsets; i++)
          {
          if (pmatch[i].rm_so >= 0)
            {
            fprintf(outfile, "%2d: ", (int)i);
            PCHARSV(dbuffer, pmatch[i].rm_so,
              pmatch[i].rm_eo - pmatch[i].rm_so, outfile);
            fprintf(outfile, "\n");
            if (do_showcaprest || (i == 0 && do_showrest))
              {
              fprintf(outfile, "%2d+ ", (int)i);
              PCHARSV(dbuffer, pmatch[i].rm_eo, len - pmatch[i].rm_eo,
                outfile);
              fprintf(outfile, "\n");
              }
            }
          }
        }
      free(pmatch);
      goto NEXT_DATA;
      }

#endif  /* !defined NOPOSIX */

    /* Handle matching via the native interface - repeats for /g and /G */

    /* Ensure that there is a JIT callback if we want to verify that JIT was
    actually used. If jit_stack == NULL, no stack has yet been assigned. */

    if (verify_jit && jit_stack == NULL && extra != NULL)
       { PCRE_ASSIGN_JIT_STACK(extra, jit_callback, jit_stack); }

    for (;; gmatched++)    /* Loop for /g or /G */
      {
      markptr = NULL;
      jit_was_used = FALSE;

      if (timeitm > 0)
        {
        register int i;
        clock_t time_taken;
        clock_t start_time = clock();

#if !defined NODFA
        if (all_use_dfa || use_dfa)
          {
          if ((options & PCRE_DFA_RESTART) != 0)
            {
            fprintf(outfile, "Timing DFA restarts is not supported\n");
            break;
            }
          if (dfa_workspace == NULL)
            dfa_workspace = (int *)malloc(DFA_WS_DIMENSION*sizeof(int));
          for (i = 0; i < timeitm; i++)
            {
            PCRE_DFA_EXEC(count, re, extra, bptr, len, start_offset,
              (options | g_notempty), use_offsets, use_size_offsets,
              dfa_workspace, DFA_WS_DIMENSION);
            }
          }
        else
#endif

        for (i = 0; i < timeitm; i++)
          {
          PCRE_EXEC(count, re, extra, bptr, len, start_offset,
            (options | g_notempty), use_offsets, use_size_offsets);
          }
        total_match_time += (time_taken = clock() - start_time);
        fprintf(outfile, "Execute time %.4f milliseconds\n",
          (((double)time_taken * 1000.0) / (double)timeitm) /
            (double)CLOCKS_PER_SEC);
        }

      /* If find_match_limit is set, we want to do repeated matches with
      varying limits in order to find the minimum value for the match limit and
      for the recursion limit. The match limits are relevant only to the normal
      running of pcre_exec(), so disable the JIT optimization. This makes it
      possible to run the same set of tests with and without JIT externally
      requested. */

      if (find_match_limit)
        {
        if (extra != NULL) { PCRE_FREE_STUDY(extra); }
        extra = (pcre_extra *)malloc(sizeof(pcre_extra));
        extra->flags = 0;

        (void)check_match_limit(re, extra, bptr, len, start_offset,
          options|g_notempty, use_offsets, use_size_offsets,
          PCRE_EXTRA_MATCH_LIMIT, &(extra->match_limit),
          PCRE_ERROR_MATCHLIMIT, "match()");

        count = check_match_limit(re, extra, bptr, len, start_offset,
          options|g_notempty, use_offsets, use_size_offsets,
          PCRE_EXTRA_MATCH_LIMIT_RECURSION, &(extra->match_limit_recursion),
          PCRE_ERROR_RECURSIONLIMIT, "match() recursion");
        }

      /* If callout_data is set, use the interface with additional data */

      else if (callout_data_set)
        {
        if (extra == NULL)
          {
          extra = (pcre_extra *)malloc(sizeof(pcre_extra));
          extra->flags = 0;
          }
        extra->flags |= PCRE_EXTRA_CALLOUT_DATA;
        extra->callout_data = &callout_data;
        PCRE_EXEC(count, re, extra, bptr, len, start_offset,
          options | g_notempty, use_offsets, use_size_offsets);
        extra->flags &= ~PCRE_EXTRA_CALLOUT_DATA;
        }

      /* The normal case is just to do the match once, with the default
      value of match_limit. */

#if !defined NODFA
      else if (all_use_dfa || use_dfa)
        {
        if (dfa_workspace == NULL)
          dfa_workspace = (int *)malloc(DFA_WS_DIMENSION*sizeof(int));
        if (dfa_matched++ == 0)
          dfa_workspace[0] = -1;  /* To catch bad restart */
        PCRE_DFA_EXEC(count, re, extra, bptr, len, start_offset,
          (options | g_notempty), use_offsets, use_size_offsets, dfa_workspace,
          DFA_WS_DIMENSION);
        if (count == 0)
          {
          fprintf(outfile, "Matched, but offsets vector is too small to show all matches\n");
          count = use_size_offsets/2;
          }
        }
#endif

      else
        {
        PCRE_EXEC(count, re, extra, bptr, len, start_offset,
          options | g_notempty, use_offsets, use_size_offsets);
        if (count == 0)
          {
          fprintf(outfile, "Matched, but too many substrings\n");
          /* 2 is a special case; match can be returned */
          count = (use_size_offsets == 2)? 1 : use_size_offsets/3;
          }
        }

      /* Matched */

      if (count >= 0)
        {
        int i, maxcount;
        void *cnptr, *gnptr;

#if !defined NODFA
        if (all_use_dfa || use_dfa) maxcount = use_size_offsets/2; else
#endif
          /* 2 is a special case; match can be returned */
          maxcount = (use_size_offsets == 2)? 1 : use_size_offsets/3;

        /* This is a check against a lunatic return value. */

        if (count > maxcount)
          {
          fprintf(outfile,
            "** PCRE error: returned count %d is too big for offset size %d\n",
            count, use_size_offsets);
          count = use_size_offsets/3;
          if (do_g || do_G)
            {
            fprintf(outfile, "** /%c loop abandoned\n", do_g? 'g' : 'G');
            do_g = do_G = FALSE;        /* Break g/G loop */
            }
          }

        /* do_allcaps requests showing of all captures in the pattern, to check
        unset ones at the end. */

        if (do_allcaps)
          {
          if (new_info(re, NULL, PCRE_INFO_CAPTURECOUNT, &count) < 0)
            goto SKIP_DATA;
          count++;   /* Allow for full match */
          if (count * 2 > use_size_offsets) count = use_size_offsets/2;
          }

        /* Output the captured substrings. Note that, for the matched string,
        the use of \K in an assertion can make the start later than the end. */

        for (i = 0; i < count * 2; i += 2)
          {
          if (use_offsets[i] < 0)
            {
            if (use_offsets[i] != -1)
              fprintf(outfile, "ERROR: bad negative value %d for offset %d\n",
                use_offsets[i], i);
            if (use_offsets[i+1] != -1)
              fprintf(outfile, "ERROR: bad negative value %d for offset %d\n",
                use_offsets[i+1], i+1);
            fprintf(outfile, "%2d: <unset>\n", i/2);
            }
          else
            {
            int start = use_offsets[i];
            int end = use_offsets[i+1];

            if (start > end)
              {
              start = use_offsets[i+1];
              end = use_offsets[i];
              fprintf(outfile, "Start of matched string is beyond its end - "
                "displaying from end to start.\n");
              }

            fprintf(outfile, "%2d: ", i/2);
            PCHARSV(bptr, start, end - start, outfile);
            if (verify_jit && jit_was_used) fprintf(outfile, " (JIT)");
            fprintf(outfile, "\n");

            /* Note: don't use the start/end variables here because we want to
            show the text from what is reported as the end. */

            if (do_showcaprest || (i == 0 && do_showrest))
              {
              fprintf(outfile, "%2d+ ", i/2);
              PCHARSV(bptr, use_offsets[i+1], len - use_offsets[i+1],
                outfile);
              fprintf(outfile, "\n");
              }
            }
          }

        if (markptr != NULL)
          {
          fprintf(outfile, "MK: ");
          PCHARSV(markptr, 0, -1, outfile);
          fprintf(outfile, "\n");
          }

        for (i = 0; i < 32; i++)
          {
          if ((copystrings & (1 << i)) != 0)
            {
            int rc;
            char copybuffer[256];
            PCRE_COPY_SUBSTRING(rc, bptr, use_offsets, count, i,
              copybuffer, sizeof(copybuffer));
            if (rc < 0)
              fprintf(outfile, "copy substring %d failed %d\n", i, rc);
            else
              {
              fprintf(outfile, "%2dC ", i);
              PCHARSV(copybuffer, 0, rc, outfile);
              fprintf(outfile, " (%d)\n", rc);
              }
            }
          }

        cnptr = copynames;
        for (;;)
          {
          int rc;
          char copybuffer[256];

#ifdef SUPPORT_PCRE32
          if (pcre_mode == PCRE32_MODE)
            {
            if (*(pcre_uint32 *)cnptr == 0) break;
            }
#endif
#ifdef SUPPORT_PCRE16
          if (pcre_mode == PCRE16_MODE)
            {
            if (*(pcre_uint16 *)cnptr == 0) break;
            }
#endif
#ifdef SUPPORT_PCRE8
          if (pcre_mode == PCRE8_MODE)
            {
            if (*(pcre_uint8 *)cnptr == 0) break;
            }
#endif

          PCRE_COPY_NAMED_SUBSTRING(rc, re, bptr, use_offsets, count,
            cnptr, copybuffer, sizeof(copybuffer));

          if (rc < 0)
            {
            fprintf(outfile, "copy substring ");
            PCHARSV(cnptr, 0, -1, outfile);
            fprintf(outfile, " failed %d\n", rc);
            }
          else
            {
            fprintf(outfile, "  C ");
            PCHARSV(copybuffer, 0, rc, outfile);
            fprintf(outfile, " (%d) ", rc);
            PCHARSV(cnptr, 0, -1, outfile);
            putc('\n', outfile);
            }

          cnptr = (char *)cnptr + (STRLEN(cnptr) + 1) * CHAR_SIZE;
          }

        for (i = 0; i < 32; i++)
          {
          if ((getstrings & (1 << i)) != 0)
            {
            int rc;
            const char *substring;
            PCRE_GET_SUBSTRING(rc, bptr, use_offsets, count, i, &substring);
            if (rc < 0)
              fprintf(outfile, "get substring %d failed %d\n", i, rc);
            else
              {
              fprintf(outfile, "%2dG ", i);
              PCHARSV(substring, 0, rc, outfile);
              fprintf(outfile, " (%d)\n", rc);
              PCRE_FREE_SUBSTRING(substring);
              }
            }
          }

        gnptr = getnames;
        for (;;)
          {
          int rc;
          const char *substring;

#ifdef SUPPORT_PCRE32
          if (pcre_mode == PCRE32_MODE)
            {
            if (*(pcre_uint32 *)gnptr == 0) break;
            }
#endif
#ifdef SUPPORT_PCRE16
          if (pcre_mode == PCRE16_MODE)
            {
            if (*(pcre_uint16 *)gnptr == 0) break;
            }
#endif
#ifdef SUPPORT_PCRE8
          if (pcre_mode == PCRE8_MODE)
            {
            if (*(pcre_uint8 *)gnptr == 0) break;
            }
#endif

          PCRE_GET_NAMED_SUBSTRING(rc, re, bptr, use_offsets, count,
            gnptr, &substring);
          if (rc < 0)
            {
            fprintf(outfile, "get substring ");
            PCHARSV(gnptr, 0, -1, outfile);
            fprintf(outfile, " failed %d\n", rc);
            }
          else
            {
            fprintf(outfile, "  G ");
            PCHARSV(substring, 0, rc, outfile);
            fprintf(outfile, " (%d) ", rc);
            PCHARSV(gnptr, 0, -1, outfile);
            PCRE_FREE_SUBSTRING(substring);
            putc('\n', outfile);
            }

          gnptr = (char *)gnptr + (STRLEN(gnptr) + 1) * CHAR_SIZE;
          }

        if (getlist)
          {
          int rc;
          const char **stringlist;
          PCRE_GET_SUBSTRING_LIST(rc, bptr, use_offsets, count, &stringlist);
          if (rc < 0)
            fprintf(outfile, "get substring list failed %d\n", rc);
          else
            {
            for (i = 0; i < count; i++)
              {
              fprintf(outfile, "%2dL ", i);
              PCHARSV(stringlist[i], 0, -1, outfile);
              putc('\n', outfile);
              }
            if (stringlist[i] != NULL)
              fprintf(outfile, "string list not terminated by NULL\n");
            PCRE_FREE_SUBSTRING_LIST(stringlist);
            }
          }
        }

      /* There was a partial match. If the bumpalong point is not the same as
      the first inspected character, show the offset explicitly. */

      else if (count == PCRE_ERROR_PARTIAL)
        {
        fprintf(outfile, "Partial match");
        if (use_size_offsets > 2 && use_offsets[0] != use_offsets[2])
          fprintf(outfile, " at offset %d", use_offsets[2]);
        if (markptr != NULL)
          {
          fprintf(outfile, ", mark=");
          PCHARSV(markptr, 0, -1, outfile);
          }
        if (use_size_offsets > 1)
          {
          fprintf(outfile, ": ");
          PCHARSV(bptr, use_offsets[0], use_offsets[1] - use_offsets[0],
            outfile);
          }
        if (verify_jit && jit_was_used) fprintf(outfile, " (JIT)");
        fprintf(outfile, "\n");
        break;  /* Out of the /g loop */
        }

      /* Failed to match. If this is a /g or /G loop and we previously set
      g_notempty after a null match, this is not necessarily the end. We want
      to advance the start offset, and continue. We won't be at the end of the
      string - that was checked before setting g_notempty.

      Complication arises in the case when the newline convention is "any",
      "crlf", or "anycrlf". If the previous match was at the end of a line
      terminated by CRLF, an advance of one character just passes the \r,
      whereas we should prefer the longer newline sequence, as does the code in
      pcre_exec(). Fudge the offset value to achieve this. We check for a
      newline setting in the pattern; if none was set, use PCRE_CONFIG() to
      find the default.

      Otherwise, in the case of UTF-8 matching, the advance must be one
      character, not one byte. */

      else
        {
        if (g_notempty != 0)
          {
          int onechar = 1;
          unsigned int obits = REAL_PCRE_OPTIONS(re);
          use_offsets[0] = start_offset;
          if ((obits & PCRE_NEWLINE_BITS) == 0)
            {
            int d;
            (void)PCRE_CONFIG(PCRE_CONFIG_NEWLINE, &d);
            /* Note that these values are always the ASCII ones, even in
            EBCDIC environments. CR = 13, NL = 10. */
            obits = (d == 13)? PCRE_NEWLINE_CR :
                    (d == 10)? PCRE_NEWLINE_LF :
                    (d == (13<<8 | 10))? PCRE_NEWLINE_CRLF :
                    (d == -2)? PCRE_NEWLINE_ANYCRLF :
                    (d == -1)? PCRE_NEWLINE_ANY : 0;
            }
          if (((obits & PCRE_NEWLINE_BITS) == PCRE_NEWLINE_ANY ||
               (obits & PCRE_NEWLINE_BITS) == PCRE_NEWLINE_CRLF ||
               (obits & PCRE_NEWLINE_BITS) == PCRE_NEWLINE_ANYCRLF)
              &&
              start_offset < len - 1 && (
#ifdef SUPPORT_PCRE8
              (pcre_mode == PCRE8_MODE &&
               bptr[start_offset] == '\r' &&
               bptr[start_offset + 1] == '\n') ||
#endif
#ifdef SUPPORT_PCRE16
              (pcre_mode == PCRE16_MODE &&
               ((PCRE_SPTR16)bptr)[start_offset] == '\r' &&
               ((PCRE_SPTR16)bptr)[start_offset + 1] == '\n') ||
#endif
#ifdef SUPPORT_PCRE32
              (pcre_mode == PCRE32_MODE &&
               ((PCRE_SPTR32)bptr)[start_offset] == '\r' &&
               ((PCRE_SPTR32)bptr)[start_offset + 1] == '\n') ||
#endif
              0))
            onechar++;
          else if (use_utf)
            {
            while (start_offset + onechar < len)
              {
              if ((bptr[start_offset+onechar] & 0xc0) != 0x80) break;
              onechar++;
              }
            }
          use_offsets[1] = start_offset + onechar;
          }
        else
          {
          switch(count)
            {
            case PCRE_ERROR_NOMATCH:
            if (gmatched == 0)
              {
              if (markptr == NULL)
                {
                fprintf(outfile, "No match");
                }
              else
                {
                fprintf(outfile, "No match, mark = ");
                PCHARSV(markptr, 0, -1, outfile);
                }
              if (verify_jit && jit_was_used) fprintf(outfile, " (JIT)");
              putc('\n', outfile);
              }
            break;

            case PCRE_ERROR_BADUTF8:
            case PCRE_ERROR_SHORTUTF8:
            fprintf(outfile, "Error %d (%s UTF-%d string)", count,
              (count == PCRE_ERROR_BADUTF8)? "bad" : "short",
              8 * CHAR_SIZE);
            if (use_size_offsets >= 2)
              fprintf(outfile, " offset=%d reason=%d", use_offsets[0],
                use_offsets[1]);
            fprintf(outfile, "\n");
            break;

            case PCRE_ERROR_BADUTF8_OFFSET:
            fprintf(outfile, "Error %d (bad UTF-%d offset)\n", count,
              8 * CHAR_SIZE);
            break;

            default:
            if (count < 0 &&
                (-count) < (int)(sizeof(errtexts)/sizeof(const char *)))
              fprintf(outfile, "Error %d (%s)\n", count, errtexts[-count]);
            else
              fprintf(outfile, "Error %d (Unexpected value)\n", count);
            break;
            }

          break;  /* Out of the /g loop */
          }
        }

      /* If not /g or /G we are done */

      if (!do_g && !do_G) break;

      if (use_offsets == NULL)
        {
        fprintf(outfile, "Cannot do global matching without an ovector\n");
        break;
        }

      /* If we have matched an empty string, first check to see if we are at
      the end of the subject. If so, the /g loop is over. Otherwise, mimic what
      Perl's /g options does. This turns out to be rather cunning. First we set
      PCRE_NOTEMPTY_ATSTART and PCRE_ANCHORED and try the match again at the
      same point. If this fails (picked up above) we advance to the next
      character. */

      g_notempty = 0;

      if (use_offsets[0] == use_offsets[1])
        {
        if (use_offsets[0] == len) break;
        g_notempty = PCRE_NOTEMPTY_ATSTART | PCRE_ANCHORED;
        }

      /* For /g, update the start offset, leaving the rest alone. There is a
      tricky case when \K is used in a positive lookbehind assertion. This can
      cause the end of the match to be less than or equal to the start offset.
      In this case we restart at one past the start offset. This may return the
      same match if the original start offset was bumped along during the
      match, but eventually the new start offset will hit the actual start
      offset. (In PCRE2 the true start offset is available, and this can be
      done better. It is not worth doing more than making sure we do not loop
      at this stage in the life of PCRE1.) */

      if (do_g)
        {
        if (g_notempty == 0 && use_offsets[1] <= start_offset)
          {
          if (start_offset >= len) break;  /* End of subject */
          start_offset++;
          if (use_utf)
            {
            while (start_offset < len)
              {
              if ((bptr[start_offset] & 0xc0) != 0x80) break;
              start_offset++;
              }
            }
          }
        else start_offset = use_offsets[1];
        }

      /* For /G, update the pointer and length */

      else
        {
        bptr += use_offsets[1] * CHAR_SIZE;
        len -= use_offsets[1];
        }
      }  /* End of loop for /g and /G */

    NEXT_DATA: continue;
    }    /* End of loop for data lines */

  CONTINUE:

#if !defined NOPOSIX
  if ((posix || do_posix) && preg.re_pcre != 0) regfree(&preg);
#endif

  if (re != NULL) new_free(re);
  if (extra != NULL)
    {
    PCRE_FREE_STUDY(extra);
    }
  if (locale_set)
    {
    new_free((void *)tables);
    setlocale(LC_CTYPE, "C");
    locale_set = 0;
    }
  if (jit_stack != NULL)
    {
    PCRE_JIT_STACK_FREE(jit_stack);
    jit_stack = NULL;
    }
  }

if (infile == stdin) fprintf(outfile, "\n");

if (showtotaltimes)
  {
  fprintf(outfile, "--------------------------------------\n");
  if (timeit > 0)
    {
    fprintf(outfile, "Total compile time %.4f milliseconds\n",
      (((double)total_compile_time * 1000.0) / (double)timeit) /
        (double)CLOCKS_PER_SEC);
    fprintf(outfile, "Total study time   %.4f milliseconds\n",
      (((double)total_study_time * 1000.0) / (double)timeit) /
        (double)CLOCKS_PER_SEC);
    }
  fprintf(outfile, "Total execute time %.4f milliseconds\n",
    (((double)total_match_time * 1000.0) / (double)timeitm) /
      (double)CLOCKS_PER_SEC);
  }

EXIT:

if (infile != NULL && infile != stdin) fclose(infile);
if (outfile != NULL && outfile != stdout) fclose(outfile);

free(buffer);
free(dbuffer);
free(pbuffer);
free(offsets);

#ifdef SUPPORT_PCRE16
if (buffer16 != NULL) free(buffer16);
#endif
#ifdef SUPPORT_PCRE32
if (buffer32 != NULL) free(buffer32);
#endif

#if !defined NODFA
if (dfa_workspace != NULL)
  free(dfa_workspace);
#endif

#if defined(__VMS)
  yield = SS$_NORMAL;  /* Return values via DCL symbols */
#endif

return yield;
}

/* End of pcretest.c */

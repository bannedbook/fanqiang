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

/* This header contains definitions that are shared between the different
modules, but which are not relevant to the exported API. This includes some
functions whose names all begin with "_pcre_", "_pcre16_" or "_pcre32_"
depending on the PRIV macro. */

#ifndef PCRE_INTERNAL_H
#define PCRE_INTERNAL_H

/* Define PCRE_DEBUG to get debugging output on stdout. */

#if 0
#define PCRE_DEBUG
#endif

/* PCRE is compiled as an 8 bit library if it is not requested otherwise. */

#if !defined COMPILE_PCRE16 && !defined COMPILE_PCRE32
#define COMPILE_PCRE8
#endif

/* If SUPPORT_UCP is defined, SUPPORT_UTF must also be defined. The
"configure" script ensures this, but not everybody uses "configure". */

#if defined SUPPORT_UCP && !(defined SUPPORT_UTF)
#define SUPPORT_UTF 1
#endif

/* We define SUPPORT_UTF if SUPPORT_UTF8 is enabled for compatibility
reasons with existing code. */

#if defined SUPPORT_UTF8 && !(defined SUPPORT_UTF)
#define SUPPORT_UTF 1
#endif

/* Fixme: SUPPORT_UTF8 should be eventually disappear from the code.
Until then we define it if SUPPORT_UTF is defined. */

#if defined SUPPORT_UTF && !(defined SUPPORT_UTF8)
#define SUPPORT_UTF8 1
#endif

/* We do not support both EBCDIC and UTF-8/16/32 at the same time. The "configure"
script prevents both being selected, but not everybody uses "configure". */

#if defined EBCDIC && defined SUPPORT_UTF
#error The use of both EBCDIC and SUPPORT_UTF is not supported.
#endif

/* Use a macro for debugging printing, 'cause that eliminates the use of #ifdef
inline, and there are *still* stupid compilers about that don't like indented
pre-processor statements, or at least there were when I first wrote this. After
all, it had only been about 10 years then...

It turns out that the Mac Debugging.h header also defines the macro DPRINTF, so
be absolutely sure we get our version. */

#undef DPRINTF
#ifdef PCRE_DEBUG
#define DPRINTF(p) printf p
#else
#define DPRINTF(p) /* Nothing */
#endif


/* Standard C headers plus the external interface definition. The only time
setjmp and stdarg are used is when NO_RECURSE is set. */

#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Valgrind (memcheck) support */

#ifdef SUPPORT_VALGRIND
#include <valgrind/memcheck.h>
#endif

/* When compiling a DLL for Windows, the exported symbols have to be declared
using some MS magic. I found some useful information on this web page:
http://msdn2.microsoft.com/en-us/library/y4h7bcy6(VS.80).aspx. According to the
information there, using __declspec(dllexport) without "extern" we have a
definition; with "extern" we have a declaration. The settings here override the
setting in pcre.h (which is included below); it defines only PCRE_EXP_DECL,
which is all that is needed for applications (they just import the symbols). We
use:

  PCRE_EXP_DECL       for declarations
  PCRE_EXP_DEFN       for definitions of exported functions
  PCRE_EXP_DATA_DEFN  for definitions of exported variables

The reason for the two DEFN macros is that in non-Windows environments, one
does not want to have "extern" before variable definitions because it leads to
compiler warnings. So we distinguish between functions and variables. In
Windows, the two should always be the same.

The reason for wrapping this in #ifndef PCRE_EXP_DECL is so that pcretest,
which is an application, but needs to import this file in order to "peek" at
internals, can #include pcre.h first to get an application's-eye view.

In principle, people compiling for non-Windows, non-Unix-like (i.e. uncommon,
special-purpose environments) might want to stick other stuff in front of
exported symbols. That's why, in the non-Windows case, we set PCRE_EXP_DEFN and
PCRE_EXP_DATA_DEFN only if they are not already set. */

#ifndef PCRE_EXP_DECL
#  ifdef _WIN32
#    ifndef PCRE_STATIC
#      define PCRE_EXP_DECL       extern __declspec(dllexport)
#      define PCRE_EXP_DEFN       __declspec(dllexport)
#      define PCRE_EXP_DATA_DEFN  __declspec(dllexport)
#    else
#      define PCRE_EXP_DECL       extern
#      define PCRE_EXP_DEFN
#      define PCRE_EXP_DATA_DEFN
#    endif
#  else
#    ifdef __cplusplus
#      define PCRE_EXP_DECL       extern "C"
#    else
#      define PCRE_EXP_DECL       extern
#    endif
#    ifndef PCRE_EXP_DEFN
#      define PCRE_EXP_DEFN       PCRE_EXP_DECL
#    endif
#    ifndef PCRE_EXP_DATA_DEFN
#      define PCRE_EXP_DATA_DEFN
#    endif
#  endif
#endif

/* When compiling with the MSVC compiler, it is sometimes necessary to include
a "calling convention" before exported function names. (This is secondhand
information; I know nothing about MSVC myself). For example, something like

  void __cdecl function(....)

might be needed. In order so make this easy, all the exported functions have
PCRE_CALL_CONVENTION just before their names. It is rarely needed; if not
set, we ensure here that it has no effect. */

#ifndef PCRE_CALL_CONVENTION
#define PCRE_CALL_CONVENTION
#endif

/* We need to have types that specify unsigned 8, 16 and 32-bit integers. We
cannot determine these outside the compilation (e.g. by running a program as
part of "configure") because PCRE is often cross-compiled for use on other
systems. Instead we make use of the maximum sizes that are available at
preprocessor time in standard C environments. */

typedef unsigned char pcre_uint8;

#if USHRT_MAX == 65535
typedef unsigned short pcre_uint16;
typedef short pcre_int16;
#define PCRE_UINT16_MAX USHRT_MAX
#define PCRE_INT16_MAX SHRT_MAX
#elif UINT_MAX == 65535
typedef unsigned int pcre_uint16;
typedef int pcre_int16;
#define PCRE_UINT16_MAX UINT_MAX
#define PCRE_INT16_MAX INT_MAX
#else
#error Cannot determine a type for 16-bit integers
#endif

#if UINT_MAX == 4294967295U
typedef unsigned int pcre_uint32;
typedef int pcre_int32;
#define PCRE_UINT32_MAX UINT_MAX
#define PCRE_INT32_MAX INT_MAX
#elif ULONG_MAX == 4294967295UL
typedef unsigned long int pcre_uint32;
typedef long int pcre_int32;
#define PCRE_UINT32_MAX ULONG_MAX
#define PCRE_INT32_MAX LONG_MAX
#else
#error Cannot determine a type for 32-bit integers
#endif

/* When checking for integer overflow in pcre_compile(), we need to handle
large integers. If a 64-bit integer type is available, we can use that.
Otherwise we have to cast to double, which of course requires floating point
arithmetic. Handle this by defining a macro for the appropriate type. If
stdint.h is available, include it; it may define INT64_MAX. Systems that do not
have stdint.h (e.g. Solaris) may have inttypes.h. The macro int64_t may be set
by "configure". */

#if defined HAVE_STDINT_H
#include <stdint.h>
#elif defined HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#if defined INT64_MAX || defined int64_t
#define INT64_OR_DOUBLE int64_t
#else
#define INT64_OR_DOUBLE double
#endif

/* All character handling must be done as unsigned characters. Otherwise there
are problems with top-bit-set characters and functions such as isspace().
However, we leave the interface to the outside world as char * or short *,
because that should make things easier for callers. This character type is
called pcre_uchar.

The IN_UCHARS macro multiply its argument with the byte size of the current
pcre_uchar type. Useful for memcpy and such operations, whose require the
byte size of their input/output buffers.

The MAX_255 macro checks whether its pcre_uchar input is less than 256.

The TABLE_GET macro is designed for accessing elements of tables whose contain
exactly 256 items. When the character is able to contain more than 256
items, some check is needed before accessing these tables.
*/

#if defined COMPILE_PCRE8

typedef unsigned char pcre_uchar;
#define IN_UCHARS(x) (x)
#define MAX_255(c) 1
#define TABLE_GET(c, table, default) ((table)[c])

#elif defined COMPILE_PCRE16

#if USHRT_MAX != 65535
/* This is a warning message. Change PCRE_UCHAR16 to a 16 bit data type in
pcre.h(.in) and disable (comment out) this message. */
#error Warning: PCRE_UCHAR16 is not a 16 bit data type.
#endif

typedef pcre_uint16 pcre_uchar;
#define UCHAR_SHIFT (1)
#define IN_UCHARS(x) ((x) << UCHAR_SHIFT)
#define MAX_255(c) ((c) <= 255u)
#define TABLE_GET(c, table, default) (MAX_255(c)? ((table)[c]):(default))

#elif defined COMPILE_PCRE32

typedef pcre_uint32 pcre_uchar;
#define UCHAR_SHIFT (2)
#define IN_UCHARS(x) ((x) << UCHAR_SHIFT)
#define MAX_255(c) ((c) <= 255u)
#define TABLE_GET(c, table, default) (MAX_255(c)? ((table)[c]):(default))

#else
#error Unsupported compiling mode
#endif /* COMPILE_PCRE[8|16|32] */

/* This is an unsigned int value that no character can ever have. UTF-8
characters only go up to 0x7fffffff (though Unicode doesn't go beyond
0x0010ffff). */

#define NOTACHAR 0xffffffff

/* PCRE is able to support several different kinds of newline (CR, LF, CRLF,
"any" and "anycrlf" at present). The following macros are used to package up
testing for newlines. NLBLOCK, PSSTART, and PSEND are defined in the various
modules to indicate in which datablock the parameters exist, and what the
start/end of string field names are. */

#define NLTYPE_FIXED    0     /* Newline is a fixed length string */
#define NLTYPE_ANY      1     /* Newline is any Unicode line ending */
#define NLTYPE_ANYCRLF  2     /* Newline is CR, LF, or CRLF */

/* This macro checks for a newline at the given position */

#define IS_NEWLINE(p) \
  ((NLBLOCK->nltype != NLTYPE_FIXED)? \
    ((p) < NLBLOCK->PSEND && \
     PRIV(is_newline)((p), NLBLOCK->nltype, NLBLOCK->PSEND, \
       &(NLBLOCK->nllen), utf)) \
    : \
    ((p) <= NLBLOCK->PSEND - NLBLOCK->nllen && \
     UCHAR21TEST(p) == NLBLOCK->nl[0] && \
     (NLBLOCK->nllen == 1 || UCHAR21TEST(p+1) == NLBLOCK->nl[1])       \
    ) \
  )

/* This macro checks for a newline immediately preceding the given position */

#define WAS_NEWLINE(p) \
  ((NLBLOCK->nltype != NLTYPE_FIXED)? \
    ((p) > NLBLOCK->PSSTART && \
     PRIV(was_newline)((p), NLBLOCK->nltype, NLBLOCK->PSSTART, \
       &(NLBLOCK->nllen), utf)) \
    : \
    ((p) >= NLBLOCK->PSSTART + NLBLOCK->nllen && \
     UCHAR21TEST(p - NLBLOCK->nllen) == NLBLOCK->nl[0] &&              \
     (NLBLOCK->nllen == 1 || UCHAR21TEST(p - NLBLOCK->nllen + 1) == NLBLOCK->nl[1]) \
    ) \
  )

/* When PCRE is compiled as a C++ library, the subject pointer can be replaced
with a custom type. This makes it possible, for example, to allow pcre_exec()
to process subject strings that are discontinuous by using a smart pointer
class. It must always be possible to inspect all of the subject string in
pcre_exec() because of the way it backtracks. Two macros are required in the
normal case, for sign-unspecified and unsigned char pointers. The former is
used for the external interface and appears in pcre.h, which is why its name
must begin with PCRE_. */

#ifdef CUSTOM_SUBJECT_PTR
#define PCRE_PUCHAR CUSTOM_SUBJECT_PTR
#else
#define PCRE_PUCHAR const pcre_uchar *
#endif

/* Include the public PCRE header and the definitions of UCP character property
values. */

#include "pcre.h"
#include "ucp.h"

#ifdef COMPILE_PCRE32
/* Assert that the public PCRE_UCHAR32 is a 32-bit type */
typedef int __assert_pcre_uchar32_size[sizeof(PCRE_UCHAR32) == 4 ? 1 : -1];
#endif

/* When compiling for use with the Virtual Pascal compiler, these functions
need to have their names changed. PCRE must be compiled with the -DVPCOMPAT
option on the command line. */

#ifdef VPCOMPAT
#define strlen(s)        _strlen(s)
#define strncmp(s1,s2,m) _strncmp(s1,s2,m)
#define memcmp(s,c,n)    _memcmp(s,c,n)
#define memcpy(d,s,n)    _memcpy(d,s,n)
#define memmove(d,s,n)   _memmove(d,s,n)
#define memset(s,c,n)    _memset(s,c,n)
#else  /* VPCOMPAT */

/* To cope with SunOS4 and other systems that lack memmove() but have bcopy(),
define a macro for memmove() if HAVE_MEMMOVE is false, provided that HAVE_BCOPY
is set. Otherwise, include an emulating function for those systems that have
neither (there some non-Unix environments where this is the case). */

#ifndef HAVE_MEMMOVE
#undef  memmove        /* some systems may have a macro */
#ifdef HAVE_BCOPY
#define memmove(a, b, c) bcopy(b, a, c)
#else  /* HAVE_BCOPY */
static void *
pcre_memmove(void *d, const void *s, size_t n)
{
size_t i;
unsigned char *dest = (unsigned char *)d;
const unsigned char *src = (const unsigned char *)s;
if (dest > src)
  {
  dest += n;
  src += n;
  for (i = 0; i < n; ++i) *(--dest) = *(--src);
  return (void *)dest;
  }
else
  {
  for (i = 0; i < n; ++i) *dest++ = *src++;
  return (void *)(dest - n);
  }
}
#define memmove(a, b, c) pcre_memmove(a, b, c)
#endif   /* not HAVE_BCOPY */
#endif   /* not HAVE_MEMMOVE */
#endif   /* not VPCOMPAT */


/* PCRE keeps offsets in its compiled code as 2-byte quantities (always stored
in big-endian order) by default. These are used, for example, to link from the
start of a subpattern to its alternatives and its end. The use of 2 bytes per
offset limits the size of the compiled regex to around 64K, which is big enough
for almost everybody. However, I received a request for an even bigger limit.
For this reason, and also to make the code easier to maintain, the storing and
loading of offsets from the byte string is now handled by the macros that are
defined here.

The macros are controlled by the value of LINK_SIZE. This defaults to 2 in
the config.h file, but can be overridden by using -D on the command line. This
is automated on Unix systems via the "configure" command. */

#if defined COMPILE_PCRE8

#if LINK_SIZE == 2

#define PUT(a,n,d)   \
  (a[n] = (d) >> 8), \
  (a[(n)+1] = (d) & 255)

#define GET(a,n) \
  (((a)[n] << 8) | (a)[(n)+1])

#define MAX_PATTERN_SIZE (1 << 16)


#elif LINK_SIZE == 3

#define PUT(a,n,d)       \
  (a[n] = (d) >> 16),    \
  (a[(n)+1] = (d) >> 8), \
  (a[(n)+2] = (d) & 255)

#define GET(a,n) \
  (((a)[n] << 16) | ((a)[(n)+1] << 8) | (a)[(n)+2])

#define MAX_PATTERN_SIZE (1 << 24)


#elif LINK_SIZE == 4

#define PUT(a,n,d)        \
  (a[n] = (d) >> 24),     \
  (a[(n)+1] = (d) >> 16), \
  (a[(n)+2] = (d) >> 8),  \
  (a[(n)+3] = (d) & 255)

#define GET(a,n) \
  (((a)[n] << 24) | ((a)[(n)+1] << 16) | ((a)[(n)+2] << 8) | (a)[(n)+3])

/* Keep it positive */
#define MAX_PATTERN_SIZE (1 << 30)

#else
#error LINK_SIZE must be either 2, 3, or 4
#endif

#elif defined COMPILE_PCRE16

#if LINK_SIZE == 2

/* Redefine LINK_SIZE as a multiple of sizeof(pcre_uchar) */
#undef LINK_SIZE
#define LINK_SIZE 1

#define PUT(a,n,d)   \
  (a[n] = (d))

#define GET(a,n) \
  (a[n])

#define MAX_PATTERN_SIZE (1 << 16)

#elif LINK_SIZE == 3 || LINK_SIZE == 4

/* Redefine LINK_SIZE as a multiple of sizeof(pcre_uchar) */
#undef LINK_SIZE
#define LINK_SIZE 2

#define PUT(a,n,d)   \
  (a[n] = (d) >> 16), \
  (a[(n)+1] = (d) & 65535)

#define GET(a,n) \
  (((a)[n] << 16) | (a)[(n)+1])

/* Keep it positive */
#define MAX_PATTERN_SIZE (1 << 30)

#else
#error LINK_SIZE must be either 2, 3, or 4
#endif

#elif defined COMPILE_PCRE32

/* Only supported LINK_SIZE is 4 */
/* Redefine LINK_SIZE as a multiple of sizeof(pcre_uchar) */
#undef LINK_SIZE
#define LINK_SIZE 1

#define PUT(a,n,d)   \
  (a[n] = (d))

#define GET(a,n) \
  (a[n])

/* Keep it positive */
#define MAX_PATTERN_SIZE (1 << 30)

#else
#error Unsupported compiling mode
#endif /* COMPILE_PCRE[8|16|32] */

/* Convenience macro defined in terms of the others */

#define PUTINC(a,n,d)   PUT(a,n,d), a += LINK_SIZE


/* PCRE uses some other 2-byte quantities that do not change when the size of
offsets changes. There are used for repeat counts and for other things such as
capturing parenthesis numbers in back references. */

#if defined COMPILE_PCRE8

#define IMM2_SIZE 2

#define PUT2(a,n,d)   \
  a[n] = (d) >> 8; \
  a[(n)+1] = (d) & 255

/* For reasons that I do not understand, the expression in this GET2 macro is
treated by gcc as a signed expression, even when a is declared as unsigned. It
seems that any kind of arithmetic results in a signed value. */

#define GET2(a,n) \
  (unsigned int)(((a)[n] << 8) | (a)[(n)+1])

#elif defined COMPILE_PCRE16

#define IMM2_SIZE 1

#define PUT2(a,n,d)   \
   a[n] = d

#define GET2(a,n) \
   a[n]

#elif defined COMPILE_PCRE32

#define IMM2_SIZE 1

#define PUT2(a,n,d)   \
   a[n] = d

#define GET2(a,n) \
   a[n]

#else
#error Unsupported compiling mode
#endif /* COMPILE_PCRE[8|16|32] */

#define PUT2INC(a,n,d)  PUT2(a,n,d), a += IMM2_SIZE

/* The maximum length of a MARK name is currently one data unit; it may be
changed in future to be a fixed number of bytes or to depend on LINK_SIZE. */

#if defined COMPILE_PCRE16 || defined COMPILE_PCRE32
#define MAX_MARK ((1u << 16) - 1)
#else
#define MAX_MARK ((1u << 8) - 1)
#endif

/* There is a proposed future special "UTF-21" mode, in which only the lowest
21 bits of a 32-bit character are interpreted as UTF, with the remaining 11
high-order bits available to the application for other uses. In preparation for
the future implementation of this mode, there are macros that load a data item
and, if in this special mode, mask it to 21 bits. These macros all have names
starting with UCHAR21. In all other modes, including the normal 32-bit
library, the macros all have the same simple definitions. When the new mode is
implemented, it is expected that these definitions will be varied appropriately
using #ifdef when compiling the library that supports the special mode. */

#define UCHAR21(eptr)        (*(eptr))
#define UCHAR21TEST(eptr)    (*(eptr))
#define UCHAR21INC(eptr)     (*(eptr)++)
#define UCHAR21INCTEST(eptr) (*(eptr)++)

/* When UTF encoding is being used, a character is no longer just a single
byte in 8-bit mode or a single short in 16-bit mode. The macros for character
handling generate simple sequences when used in the basic mode, and more
complicated ones for UTF characters. GETCHARLENTEST and other macros are not
used when UTF is not supported. To make sure they can never even appear when
UTF support is omitted, we don't even define them. */

#ifndef SUPPORT_UTF

/* #define MAX_VALUE_FOR_SINGLE_CHAR */
/* #define HAS_EXTRALEN(c) */
/* #define GET_EXTRALEN(c) */
/* #define NOT_FIRSTCHAR(c) */
#define GETCHAR(c, eptr) c = *eptr;
#define GETCHARTEST(c, eptr) c = *eptr;
#define GETCHARINC(c, eptr) c = *eptr++;
#define GETCHARINCTEST(c, eptr) c = *eptr++;
#define GETCHARLEN(c, eptr, len) c = *eptr;
/* #define GETCHARLENTEST(c, eptr, len) */
/* #define BACKCHAR(eptr) */
/* #define FORWARDCHAR(eptr) */
/* #define ACROSSCHAR(condition, eptr, action) */

#else   /* SUPPORT_UTF */

/* Tests whether the code point needs extra characters to decode. */

#define HASUTF8EXTRALEN(c) ((c) >= 0xc0)

/* Base macro to pick up the remaining bytes of a UTF-8 character, not
advancing the pointer. */

#define GETUTF8(c, eptr) \
    { \
    if ((c & 0x20) == 0) \
      c = ((c & 0x1f) << 6) | (eptr[1] & 0x3f); \
    else if ((c & 0x10) == 0) \
      c = ((c & 0x0f) << 12) | ((eptr[1] & 0x3f) << 6) | (eptr[2] & 0x3f); \
    else if ((c & 0x08) == 0) \
      c = ((c & 0x07) << 18) | ((eptr[1] & 0x3f) << 12) | \
      ((eptr[2] & 0x3f) << 6) | (eptr[3] & 0x3f); \
    else if ((c & 0x04) == 0) \
      c = ((c & 0x03) << 24) | ((eptr[1] & 0x3f) << 18) | \
          ((eptr[2] & 0x3f) << 12) | ((eptr[3] & 0x3f) << 6) | \
          (eptr[4] & 0x3f); \
    else \
      c = ((c & 0x01) << 30) | ((eptr[1] & 0x3f) << 24) | \
          ((eptr[2] & 0x3f) << 18) | ((eptr[3] & 0x3f) << 12) | \
          ((eptr[4] & 0x3f) << 6) | (eptr[5] & 0x3f); \
    }

/* Base macro to pick up the remaining bytes of a UTF-8 character, advancing
the pointer. */

#define GETUTF8INC(c, eptr) \
    { \
    if ((c & 0x20) == 0) \
      c = ((c & 0x1f) << 6) | (*eptr++ & 0x3f); \
    else if ((c & 0x10) == 0) \
      { \
      c = ((c & 0x0f) << 12) | ((*eptr & 0x3f) << 6) | (eptr[1] & 0x3f); \
      eptr += 2; \
      } \
    else if ((c & 0x08) == 0) \
      { \
      c = ((c & 0x07) << 18) | ((*eptr & 0x3f) << 12) | \
          ((eptr[1] & 0x3f) << 6) | (eptr[2] & 0x3f); \
      eptr += 3; \
      } \
    else if ((c & 0x04) == 0) \
      { \
      c = ((c & 0x03) << 24) | ((*eptr & 0x3f) << 18) | \
          ((eptr[1] & 0x3f) << 12) | ((eptr[2] & 0x3f) << 6) | \
          (eptr[3] & 0x3f); \
      eptr += 4; \
      } \
    else \
      { \
      c = ((c & 0x01) << 30) | ((*eptr & 0x3f) << 24) | \
          ((eptr[1] & 0x3f) << 18) | ((eptr[2] & 0x3f) << 12) | \
          ((eptr[3] & 0x3f) << 6) | (eptr[4] & 0x3f); \
      eptr += 5; \
      } \
    }

#if defined COMPILE_PCRE8

/* These macros were originally written in the form of loops that used data
from the tables whose names start with PRIV(utf8_table). They were rewritten by
a user so as not to use loops, because in some environments this gives a
significant performance advantage, and it seems never to do any harm. */

/* Tells the biggest code point which can be encoded as a single character. */

#define MAX_VALUE_FOR_SINGLE_CHAR 127

/* Tests whether the code point needs extra characters to decode. */

#define HAS_EXTRALEN(c) ((c) >= 0xc0)

/* Returns with the additional number of characters if IS_MULTICHAR(c) is TRUE.
Otherwise it has an undefined behaviour. */

#define GET_EXTRALEN(c) (PRIV(utf8_table4)[(c) & 0x3f])

/* Returns TRUE, if the given character is not the first character
of a UTF sequence. */

#define NOT_FIRSTCHAR(c) (((c) & 0xc0) == 0x80)

/* Get the next UTF-8 character, not advancing the pointer. This is called when
we know we are in UTF-8 mode. */

#define GETCHAR(c, eptr) \
  c = *eptr; \
  if (c >= 0xc0) GETUTF8(c, eptr);

/* Get the next UTF-8 character, testing for UTF-8 mode, and not advancing the
pointer. */

#define GETCHARTEST(c, eptr) \
  c = *eptr; \
  if (utf && c >= 0xc0) GETUTF8(c, eptr);

/* Get the next UTF-8 character, advancing the pointer. This is called when we
know we are in UTF-8 mode. */

#define GETCHARINC(c, eptr) \
  c = *eptr++; \
  if (c >= 0xc0) GETUTF8INC(c, eptr);

/* Get the next character, testing for UTF-8 mode, and advancing the pointer.
This is called when we don't know if we are in UTF-8 mode. */

#define GETCHARINCTEST(c, eptr) \
  c = *eptr++; \
  if (utf && c >= 0xc0) GETUTF8INC(c, eptr);

/* Base macro to pick up the remaining bytes of a UTF-8 character, not
advancing the pointer, incrementing the length. */

#define GETUTF8LEN(c, eptr, len) \
    { \
    if ((c & 0x20) == 0) \
      { \
      c = ((c & 0x1f) << 6) | (eptr[1] & 0x3f); \
      len++; \
      } \
    else if ((c & 0x10)  == 0) \
      { \
      c = ((c & 0x0f) << 12) | ((eptr[1] & 0x3f) << 6) | (eptr[2] & 0x3f); \
      len += 2; \
      } \
    else if ((c & 0x08)  == 0) \
      {\
      c = ((c & 0x07) << 18) | ((eptr[1] & 0x3f) << 12) | \
          ((eptr[2] & 0x3f) << 6) | (eptr[3] & 0x3f); \
      len += 3; \
      } \
    else if ((c & 0x04)  == 0) \
      { \
      c = ((c & 0x03) << 24) | ((eptr[1] & 0x3f) << 18) | \
          ((eptr[2] & 0x3f) << 12) | ((eptr[3] & 0x3f) << 6) | \
          (eptr[4] & 0x3f); \
      len += 4; \
      } \
    else \
      {\
      c = ((c & 0x01) << 30) | ((eptr[1] & 0x3f) << 24) | \
          ((eptr[2] & 0x3f) << 18) | ((eptr[3] & 0x3f) << 12) | \
          ((eptr[4] & 0x3f) << 6) | (eptr[5] & 0x3f); \
      len += 5; \
      } \
    }

/* Get the next UTF-8 character, not advancing the pointer, incrementing length
if there are extra bytes. This is called when we know we are in UTF-8 mode. */

#define GETCHARLEN(c, eptr, len) \
  c = *eptr; \
  if (c >= 0xc0) GETUTF8LEN(c, eptr, len);

/* Get the next UTF-8 character, testing for UTF-8 mode, not advancing the
pointer, incrementing length if there are extra bytes. This is called when we
do not know if we are in UTF-8 mode. */

#define GETCHARLENTEST(c, eptr, len) \
  c = *eptr; \
  if (utf && c >= 0xc0) GETUTF8LEN(c, eptr, len);

/* If the pointer is not at the start of a character, move it back until
it is. This is called only in UTF-8 mode - we don't put a test within the macro
because almost all calls are already within a block of UTF-8 only code. */

#define BACKCHAR(eptr) while((*eptr & 0xc0) == 0x80) eptr--

/* Same as above, just in the other direction. */
#define FORWARDCHAR(eptr) while((*eptr & 0xc0) == 0x80) eptr++

/* Same as above, but it allows a fully customizable form. */
#define ACROSSCHAR(condition, eptr, action) \
  while((condition) && ((eptr) & 0xc0) == 0x80) action

#elif defined COMPILE_PCRE16

/* Tells the biggest code point which can be encoded as a single character. */

#define MAX_VALUE_FOR_SINGLE_CHAR 65535

/* Tests whether the code point needs extra characters to decode. */

#define HAS_EXTRALEN(c) (((c) & 0xfc00) == 0xd800)

/* Returns with the additional number of characters if IS_MULTICHAR(c) is TRUE.
Otherwise it has an undefined behaviour. */

#define GET_EXTRALEN(c) 1

/* Returns TRUE, if the given character is not the first character
of a UTF sequence. */

#define NOT_FIRSTCHAR(c) (((c) & 0xfc00) == 0xdc00)

/* Base macro to pick up the low surrogate of a UTF-16 character, not
advancing the pointer. */

#define GETUTF16(c, eptr) \
   { c = (((c & 0x3ff) << 10) | (eptr[1] & 0x3ff)) + 0x10000; }

/* Get the next UTF-16 character, not advancing the pointer. This is called when
we know we are in UTF-16 mode. */

#define GETCHAR(c, eptr) \
  c = *eptr; \
  if ((c & 0xfc00) == 0xd800) GETUTF16(c, eptr);

/* Get the next UTF-16 character, testing for UTF-16 mode, and not advancing the
pointer. */

#define GETCHARTEST(c, eptr) \
  c = *eptr; \
  if (utf && (c & 0xfc00) == 0xd800) GETUTF16(c, eptr);

/* Base macro to pick up the low surrogate of a UTF-16 character, advancing
the pointer. */

#define GETUTF16INC(c, eptr) \
   { c = (((c & 0x3ff) << 10) | (*eptr++ & 0x3ff)) + 0x10000; }

/* Get the next UTF-16 character, advancing the pointer. This is called when we
know we are in UTF-16 mode. */

#define GETCHARINC(c, eptr) \
  c = *eptr++; \
  if ((c & 0xfc00) == 0xd800) GETUTF16INC(c, eptr);

/* Get the next character, testing for UTF-16 mode, and advancing the pointer.
This is called when we don't know if we are in UTF-16 mode. */

#define GETCHARINCTEST(c, eptr) \
  c = *eptr++; \
  if (utf && (c & 0xfc00) == 0xd800) GETUTF16INC(c, eptr);

/* Base macro to pick up the low surrogate of a UTF-16 character, not
advancing the pointer, incrementing the length. */

#define GETUTF16LEN(c, eptr, len) \
   { c = (((c & 0x3ff) << 10) | (eptr[1] & 0x3ff)) + 0x10000; len++; }

/* Get the next UTF-16 character, not advancing the pointer, incrementing
length if there is a low surrogate. This is called when we know we are in
UTF-16 mode. */

#define GETCHARLEN(c, eptr, len) \
  c = *eptr; \
  if ((c & 0xfc00) == 0xd800) GETUTF16LEN(c, eptr, len);

/* Get the next UTF-816character, testing for UTF-16 mode, not advancing the
pointer, incrementing length if there is a low surrogate. This is called when
we do not know if we are in UTF-16 mode. */

#define GETCHARLENTEST(c, eptr, len) \
  c = *eptr; \
  if (utf && (c & 0xfc00) == 0xd800) GETUTF16LEN(c, eptr, len);

/* If the pointer is not at the start of a character, move it back until
it is. This is called only in UTF-16 mode - we don't put a test within the
macro because almost all calls are already within a block of UTF-16 only
code. */

#define BACKCHAR(eptr) if ((*eptr & 0xfc00) == 0xdc00) eptr--

/* Same as above, just in the other direction. */
#define FORWARDCHAR(eptr) if ((*eptr & 0xfc00) == 0xdc00) eptr++

/* Same as above, but it allows a fully customizable form. */
#define ACROSSCHAR(condition, eptr, action) \
  if ((condition) && ((eptr) & 0xfc00) == 0xdc00) action

#elif defined COMPILE_PCRE32

/* These are trivial for the 32-bit library, since all UTF-32 characters fit
into one pcre_uchar unit. */
#define MAX_VALUE_FOR_SINGLE_CHAR (0x10ffffu)
#define HAS_EXTRALEN(c) (0)
#define GET_EXTRALEN(c) (0)
#define NOT_FIRSTCHAR(c) (0)

/* Get the next UTF-32 character, not advancing the pointer. This is called when
we know we are in UTF-32 mode. */

#define GETCHAR(c, eptr) \
  c = *(eptr);

/* Get the next UTF-32 character, testing for UTF-32 mode, and not advancing the
pointer. */

#define GETCHARTEST(c, eptr) \
  c = *(eptr);

/* Get the next UTF-32 character, advancing the pointer. This is called when we
know we are in UTF-32 mode. */

#define GETCHARINC(c, eptr) \
  c = *((eptr)++);

/* Get the next character, testing for UTF-32 mode, and advancing the pointer.
This is called when we don't know if we are in UTF-32 mode. */

#define GETCHARINCTEST(c, eptr) \
  c = *((eptr)++);

/* Get the next UTF-32 character, not advancing the pointer, not incrementing
length (since all UTF-32 is of length 1). This is called when we know we are in
UTF-32 mode. */

#define GETCHARLEN(c, eptr, len) \
  GETCHAR(c, eptr)

/* Get the next UTF-32character, testing for UTF-32 mode, not advancing the
pointer, not incrementing the length (since all UTF-32 is of length 1).
This is called when we do not know if we are in UTF-32 mode. */

#define GETCHARLENTEST(c, eptr, len) \
  GETCHARTEST(c, eptr)

/* If the pointer is not at the start of a character, move it back until
it is. This is called only in UTF-32 mode - we don't put a test within the
macro because almost all calls are already within a block of UTF-32 only
code.
These are all no-ops since all UTF-32 characters fit into one pcre_uchar. */

#define BACKCHAR(eptr) do { } while (0)

/* Same as above, just in the other direction. */
#define FORWARDCHAR(eptr) do { } while (0)

/* Same as above, but it allows a fully customizable form. */
#define ACROSSCHAR(condition, eptr, action) do { } while (0)

#else
#error Unsupported compiling mode
#endif /* COMPILE_PCRE[8|16|32] */

#endif  /* SUPPORT_UTF */

/* Tests for Unicode horizontal and vertical whitespace characters must check a
number of different values. Using a switch statement for this generates the
fastest code (no loop, no memory access), and there are several places in the
interpreter code where this happens. In order to ensure that all the case lists
remain in step, we use macros so that there is only one place where the lists
are defined.

These values are also required as lists in pcre_compile.c when processing \h,
\H, \v and \V in a character class. The lists are defined in pcre_tables.c, but
macros that define the values are here so that all the definitions are
together. The lists must be in ascending character order, terminated by
NOTACHAR (which is 0xffffffff).

Any changes should ensure that the various macros are kept in step with each
other. NOTE: The values also appear in pcre_jit_compile.c. */

/* ------ ASCII/Unicode environments ------ */

#ifndef EBCDIC

#define HSPACE_LIST \
  CHAR_HT, CHAR_SPACE, CHAR_NBSP, \
  0x1680, 0x180e, 0x2000, 0x2001, 0x2002, 0x2003, 0x2004, 0x2005, \
  0x2006, 0x2007, 0x2008, 0x2009, 0x200A, 0x202f, 0x205f, 0x3000, \
  NOTACHAR

#define HSPACE_MULTIBYTE_CASES \
  case 0x1680:  /* OGHAM SPACE MARK */ \
  case 0x180e:  /* MONGOLIAN VOWEL SEPARATOR */ \
  case 0x2000:  /* EN QUAD */ \
  case 0x2001:  /* EM QUAD */ \
  case 0x2002:  /* EN SPACE */ \
  case 0x2003:  /* EM SPACE */ \
  case 0x2004:  /* THREE-PER-EM SPACE */ \
  case 0x2005:  /* FOUR-PER-EM SPACE */ \
  case 0x2006:  /* SIX-PER-EM SPACE */ \
  case 0x2007:  /* FIGURE SPACE */ \
  case 0x2008:  /* PUNCTUATION SPACE */ \
  case 0x2009:  /* THIN SPACE */ \
  case 0x200A:  /* HAIR SPACE */ \
  case 0x202f:  /* NARROW NO-BREAK SPACE */ \
  case 0x205f:  /* MEDIUM MATHEMATICAL SPACE */ \
  case 0x3000   /* IDEOGRAPHIC SPACE */

#define HSPACE_BYTE_CASES \
  case CHAR_HT: \
  case CHAR_SPACE: \
  case CHAR_NBSP

#define HSPACE_CASES \
  HSPACE_BYTE_CASES: \
  HSPACE_MULTIBYTE_CASES

#define VSPACE_LIST \
  CHAR_LF, CHAR_VT, CHAR_FF, CHAR_CR, CHAR_NEL, 0x2028, 0x2029, NOTACHAR

#define VSPACE_MULTIBYTE_CASES \
  case 0x2028:    /* LINE SEPARATOR */ \
  case 0x2029     /* PARAGRAPH SEPARATOR */

#define VSPACE_BYTE_CASES \
  case CHAR_LF: \
  case CHAR_VT: \
  case CHAR_FF: \
  case CHAR_CR: \
  case CHAR_NEL

#define VSPACE_CASES \
  VSPACE_BYTE_CASES: \
  VSPACE_MULTIBYTE_CASES

/* ------ EBCDIC environments ------ */

#else
#define HSPACE_LIST CHAR_HT, CHAR_SPACE, CHAR_NBSP, NOTACHAR

#define HSPACE_BYTE_CASES \
  case CHAR_HT: \
  case CHAR_SPACE: \
  case CHAR_NBSP

#define HSPACE_CASES HSPACE_BYTE_CASES

#ifdef EBCDIC_NL25
#define VSPACE_LIST \
  CHAR_VT, CHAR_FF, CHAR_CR, CHAR_NEL, CHAR_LF, NOTACHAR
#else
#define VSPACE_LIST \
  CHAR_VT, CHAR_FF, CHAR_CR, CHAR_LF, CHAR_NEL, NOTACHAR
#endif

#define VSPACE_BYTE_CASES \
  case CHAR_LF: \
  case CHAR_VT: \
  case CHAR_FF: \
  case CHAR_CR: \
  case CHAR_NEL

#define VSPACE_CASES VSPACE_BYTE_CASES
#endif  /* EBCDIC */

/* ------ End of whitespace macros ------ */



/* Private flags containing information about the compiled regex. They used to
live at the top end of the options word, but that got almost full, so they were
moved to a 16-bit flags word - which got almost full, so now they are in a
32-bit flags word. From release 8.00, PCRE_NOPARTIAL is unused, as the
restrictions on partial matching have been lifted. It remains for backwards
compatibility. */

#define PCRE_MODE8         0x00000001  /* compiled in 8 bit mode */
#define PCRE_MODE16        0x00000002  /* compiled in 16 bit mode */
#define PCRE_MODE32        0x00000004  /* compiled in 32 bit mode */
#define PCRE_FIRSTSET      0x00000010  /* first_char is set */
#define PCRE_FCH_CASELESS  0x00000020  /* caseless first char */
#define PCRE_REQCHSET      0x00000040  /* req_byte is set */
#define PCRE_RCH_CASELESS  0x00000080  /* caseless requested char */
#define PCRE_STARTLINE     0x00000100  /* start after \n for multiline */
#define PCRE_NOPARTIAL     0x00000200  /* can't use partial with this regex */
#define PCRE_JCHANGED      0x00000400  /* j option used in regex */
#define PCRE_HASCRORLF     0x00000800  /* explicit \r or \n in pattern */
#define PCRE_HASTHEN       0x00001000  /* pattern contains (*THEN) */
#define PCRE_MLSET         0x00002000  /* match limit set by regex */
#define PCRE_RLSET         0x00004000  /* recursion limit set by regex */
#define PCRE_MATCH_EMPTY   0x00008000  /* pattern can match empty string */

#if defined COMPILE_PCRE8
#define PCRE_MODE          PCRE_MODE8
#elif defined COMPILE_PCRE16
#define PCRE_MODE          PCRE_MODE16
#elif defined COMPILE_PCRE32
#define PCRE_MODE          PCRE_MODE32
#endif
#define PCRE_MODE_MASK     (PCRE_MODE8 | PCRE_MODE16 | PCRE_MODE32)

/* Flags for the "extra" block produced by pcre_study(). */

#define PCRE_STUDY_MAPPED  0x0001  /* a map of starting chars exists */
#define PCRE_STUDY_MINLEN  0x0002  /* a minimum length field exists */

/* Masks for identifying the public options that are permitted at compile
time, run time, or study time, respectively. */

#define PCRE_NEWLINE_BITS (PCRE_NEWLINE_CR|PCRE_NEWLINE_LF|PCRE_NEWLINE_ANY| \
                           PCRE_NEWLINE_ANYCRLF)

#define PUBLIC_COMPILE_OPTIONS \
  (PCRE_CASELESS|PCRE_EXTENDED|PCRE_ANCHORED|PCRE_MULTILINE| \
   PCRE_DOTALL|PCRE_DOLLAR_ENDONLY|PCRE_EXTRA|PCRE_UNGREEDY|PCRE_UTF8| \
   PCRE_NO_AUTO_CAPTURE|PCRE_NO_AUTO_POSSESS| \
   PCRE_NO_UTF8_CHECK|PCRE_AUTO_CALLOUT|PCRE_FIRSTLINE| \
   PCRE_DUPNAMES|PCRE_NEWLINE_BITS|PCRE_BSR_ANYCRLF|PCRE_BSR_UNICODE| \
   PCRE_JAVASCRIPT_COMPAT|PCRE_UCP|PCRE_NO_START_OPTIMIZE|PCRE_NEVER_UTF)

#define PUBLIC_EXEC_OPTIONS \
  (PCRE_ANCHORED|PCRE_NOTBOL|PCRE_NOTEOL|PCRE_NOTEMPTY|PCRE_NOTEMPTY_ATSTART| \
   PCRE_NO_UTF8_CHECK|PCRE_PARTIAL_HARD|PCRE_PARTIAL_SOFT|PCRE_NEWLINE_BITS| \
   PCRE_BSR_ANYCRLF|PCRE_BSR_UNICODE|PCRE_NO_START_OPTIMIZE)

#define PUBLIC_DFA_EXEC_OPTIONS \
  (PCRE_ANCHORED|PCRE_NOTBOL|PCRE_NOTEOL|PCRE_NOTEMPTY|PCRE_NOTEMPTY_ATSTART| \
   PCRE_NO_UTF8_CHECK|PCRE_PARTIAL_HARD|PCRE_PARTIAL_SOFT|PCRE_DFA_SHORTEST| \
   PCRE_DFA_RESTART|PCRE_NEWLINE_BITS|PCRE_BSR_ANYCRLF|PCRE_BSR_UNICODE| \
   PCRE_NO_START_OPTIMIZE)

#define PUBLIC_STUDY_OPTIONS \
   (PCRE_STUDY_JIT_COMPILE|PCRE_STUDY_JIT_PARTIAL_SOFT_COMPILE| \
    PCRE_STUDY_JIT_PARTIAL_HARD_COMPILE|PCRE_STUDY_EXTRA_NEEDED)

#define PUBLIC_JIT_EXEC_OPTIONS \
   (PCRE_NO_UTF8_CHECK|PCRE_NOTBOL|PCRE_NOTEOL|PCRE_NOTEMPTY|\
    PCRE_NOTEMPTY_ATSTART|PCRE_PARTIAL_SOFT|PCRE_PARTIAL_HARD)

/* Magic number to provide a small check against being handed junk. */

#define MAGIC_NUMBER  0x50435245UL   /* 'PCRE' */

/* This variable is used to detect a loaded regular expression
in different endianness. */

#define REVERSED_MAGIC_NUMBER  0x45524350UL   /* 'ERCP' */

/* The maximum remaining length of subject we are prepared to search for a
req_byte match. */

#define REQ_BYTE_MAX 1000

/* Miscellaneous definitions. The #ifndef is to pacify compiler warnings in
environments where these macros are defined elsewhere. Unfortunately, there
is no way to do the same for the typedef. */

typedef int BOOL;

#ifndef FALSE
#define FALSE   0
#define TRUE    1
#endif

/* If PCRE is to support UTF-8 on EBCDIC platforms, we cannot use normal
character constants like '*' because the compiler would emit their EBCDIC code,
which is different from their ASCII/UTF-8 code. Instead we define macros for
the characters so that they always use the ASCII/UTF-8 code when UTF-8 support
is enabled. When UTF-8 support is not enabled, the definitions use character
literals. Both character and string versions of each character are needed, and
there are some longer strings as well.

This means that, on EBCDIC platforms, the PCRE library can handle either
EBCDIC, or UTF-8, but not both. To support both in the same compiled library
would need different lookups depending on whether PCRE_UTF8 was set or not.
This would make it impossible to use characters in switch/case statements,
which would reduce performance. For a theoretical use (which nobody has asked
for) in a minority area (EBCDIC platforms), this is not sensible. Any
application that did need both could compile two versions of the library, using
macros to give the functions distinct names. */

#ifndef SUPPORT_UTF

/* UTF-8 support is not enabled; use the platform-dependent character literals
so that PCRE works in both ASCII and EBCDIC environments, but only in non-UTF
mode. Newline characters are problematic in EBCDIC. Though it has CR and LF
characters, a common practice has been to use its NL (0x15) character as the
line terminator in C-like processing environments. However, sometimes the LF
(0x25) character is used instead, according to this Unicode document:

http://unicode.org/standard/reports/tr13/tr13-5.html

PCRE defaults EBCDIC NL to 0x15, but has a build-time option to select 0x25
instead. Whichever is *not* chosen is defined as NEL.

In both ASCII and EBCDIC environments, CHAR_NL and CHAR_LF are synonyms for the
same code point. */

#ifdef EBCDIC

#ifndef EBCDIC_NL25
#define CHAR_NL                     '\x15'
#define CHAR_NEL                    '\x25'
#define STR_NL                      "\x15"
#define STR_NEL                     "\x25"
#else
#define CHAR_NL                     '\x25'
#define CHAR_NEL                    '\x15'
#define STR_NL                      "\x25"
#define STR_NEL                     "\x15"
#endif

#define CHAR_LF                     CHAR_NL
#define STR_LF                      STR_NL

#define CHAR_ESC                    '\047'
#define CHAR_DEL                    '\007'
#define CHAR_NBSP                   '\x41'
#define STR_ESC                     "\047"
#define STR_DEL                     "\007"

#else  /* Not EBCDIC */

/* In ASCII/Unicode, linefeed is '\n' and we equate this to NL for
compatibility. NEL is the Unicode newline character; make sure it is
a positive value. */

#define CHAR_LF                     '\n'
#define CHAR_NL                     CHAR_LF
#define CHAR_NEL                    ((unsigned char)'\x85')
#define CHAR_ESC                    '\033'
#define CHAR_DEL                    '\177'
#define CHAR_NBSP                   ((unsigned char)'\xa0')

#define STR_LF                      "\n"
#define STR_NL                      STR_LF
#define STR_NEL                     "\x85"
#define STR_ESC                     "\033"
#define STR_DEL                     "\177"

#endif  /* EBCDIC */

/* The remaining definitions work in both environments. */

#define CHAR_NULL                   '\0'
#define CHAR_HT                     '\t'
#define CHAR_VT                     '\v'
#define CHAR_FF                     '\f'
#define CHAR_CR                     '\r'
#define CHAR_BS                     '\b'
#define CHAR_BEL                    '\a'

#define CHAR_SPACE                  ' '
#define CHAR_EXCLAMATION_MARK       '!'
#define CHAR_QUOTATION_MARK         '"'
#define CHAR_NUMBER_SIGN            '#'
#define CHAR_DOLLAR_SIGN            '$'
#define CHAR_PERCENT_SIGN           '%'
#define CHAR_AMPERSAND              '&'
#define CHAR_APOSTROPHE             '\''
#define CHAR_LEFT_PARENTHESIS       '('
#define CHAR_RIGHT_PARENTHESIS      ')'
#define CHAR_ASTERISK               '*'
#define CHAR_PLUS                   '+'
#define CHAR_COMMA                  ','
#define CHAR_MINUS                  '-'
#define CHAR_DOT                    '.'
#define CHAR_SLASH                  '/'
#define CHAR_0                      '0'
#define CHAR_1                      '1'
#define CHAR_2                      '2'
#define CHAR_3                      '3'
#define CHAR_4                      '4'
#define CHAR_5                      '5'
#define CHAR_6                      '6'
#define CHAR_7                      '7'
#define CHAR_8                      '8'
#define CHAR_9                      '9'
#define CHAR_COLON                  ':'
#define CHAR_SEMICOLON              ';'
#define CHAR_LESS_THAN_SIGN         '<'
#define CHAR_EQUALS_SIGN            '='
#define CHAR_GREATER_THAN_SIGN      '>'
#define CHAR_QUESTION_MARK          '?'
#define CHAR_COMMERCIAL_AT          '@'
#define CHAR_A                      'A'
#define CHAR_B                      'B'
#define CHAR_C                      'C'
#define CHAR_D                      'D'
#define CHAR_E                      'E'
#define CHAR_F                      'F'
#define CHAR_G                      'G'
#define CHAR_H                      'H'
#define CHAR_I                      'I'
#define CHAR_J                      'J'
#define CHAR_K                      'K'
#define CHAR_L                      'L'
#define CHAR_M                      'M'
#define CHAR_N                      'N'
#define CHAR_O                      'O'
#define CHAR_P                      'P'
#define CHAR_Q                      'Q'
#define CHAR_R                      'R'
#define CHAR_S                      'S'
#define CHAR_T                      'T'
#define CHAR_U                      'U'
#define CHAR_V                      'V'
#define CHAR_W                      'W'
#define CHAR_X                      'X'
#define CHAR_Y                      'Y'
#define CHAR_Z                      'Z'
#define CHAR_LEFT_SQUARE_BRACKET    '['
#define CHAR_BACKSLASH              '\\'
#define CHAR_RIGHT_SQUARE_BRACKET   ']'
#define CHAR_CIRCUMFLEX_ACCENT      '^'
#define CHAR_UNDERSCORE             '_'
#define CHAR_GRAVE_ACCENT           '`'
#define CHAR_a                      'a'
#define CHAR_b                      'b'
#define CHAR_c                      'c'
#define CHAR_d                      'd'
#define CHAR_e                      'e'
#define CHAR_f                      'f'
#define CHAR_g                      'g'
#define CHAR_h                      'h'
#define CHAR_i                      'i'
#define CHAR_j                      'j'
#define CHAR_k                      'k'
#define CHAR_l                      'l'
#define CHAR_m                      'm'
#define CHAR_n                      'n'
#define CHAR_o                      'o'
#define CHAR_p                      'p'
#define CHAR_q                      'q'
#define CHAR_r                      'r'
#define CHAR_s                      's'
#define CHAR_t                      't'
#define CHAR_u                      'u'
#define CHAR_v                      'v'
#define CHAR_w                      'w'
#define CHAR_x                      'x'
#define CHAR_y                      'y'
#define CHAR_z                      'z'
#define CHAR_LEFT_CURLY_BRACKET     '{'
#define CHAR_VERTICAL_LINE          '|'
#define CHAR_RIGHT_CURLY_BRACKET    '}'
#define CHAR_TILDE                  '~'

#define STR_HT                      "\t"
#define STR_VT                      "\v"
#define STR_FF                      "\f"
#define STR_CR                      "\r"
#define STR_BS                      "\b"
#define STR_BEL                     "\a"

#define STR_SPACE                   " "
#define STR_EXCLAMATION_MARK        "!"
#define STR_QUOTATION_MARK          "\""
#define STR_NUMBER_SIGN             "#"
#define STR_DOLLAR_SIGN             "$"
#define STR_PERCENT_SIGN            "%"
#define STR_AMPERSAND               "&"
#define STR_APOSTROPHE              "'"
#define STR_LEFT_PARENTHESIS        "("
#define STR_RIGHT_PARENTHESIS       ")"
#define STR_ASTERISK                "*"
#define STR_PLUS                    "+"
#define STR_COMMA                   ","
#define STR_MINUS                   "-"
#define STR_DOT                     "."
#define STR_SLASH                   "/"
#define STR_0                       "0"
#define STR_1                       "1"
#define STR_2                       "2"
#define STR_3                       "3"
#define STR_4                       "4"
#define STR_5                       "5"
#define STR_6                       "6"
#define STR_7                       "7"
#define STR_8                       "8"
#define STR_9                       "9"
#define STR_COLON                   ":"
#define STR_SEMICOLON               ";"
#define STR_LESS_THAN_SIGN          "<"
#define STR_EQUALS_SIGN             "="
#define STR_GREATER_THAN_SIGN       ">"
#define STR_QUESTION_MARK           "?"
#define STR_COMMERCIAL_AT           "@"
#define STR_A                       "A"
#define STR_B                       "B"
#define STR_C                       "C"
#define STR_D                       "D"
#define STR_E                       "E"
#define STR_F                       "F"
#define STR_G                       "G"
#define STR_H                       "H"
#define STR_I                       "I"
#define STR_J                       "J"
#define STR_K                       "K"
#define STR_L                       "L"
#define STR_M                       "M"
#define STR_N                       "N"
#define STR_O                       "O"
#define STR_P                       "P"
#define STR_Q                       "Q"
#define STR_R                       "R"
#define STR_S                       "S"
#define STR_T                       "T"
#define STR_U                       "U"
#define STR_V                       "V"
#define STR_W                       "W"
#define STR_X                       "X"
#define STR_Y                       "Y"
#define STR_Z                       "Z"
#define STR_LEFT_SQUARE_BRACKET     "["
#define STR_BACKSLASH               "\\"
#define STR_RIGHT_SQUARE_BRACKET    "]"
#define STR_CIRCUMFLEX_ACCENT       "^"
#define STR_UNDERSCORE              "_"
#define STR_GRAVE_ACCENT            "`"
#define STR_a                       "a"
#define STR_b                       "b"
#define STR_c                       "c"
#define STR_d                       "d"
#define STR_e                       "e"
#define STR_f                       "f"
#define STR_g                       "g"
#define STR_h                       "h"
#define STR_i                       "i"
#define STR_j                       "j"
#define STR_k                       "k"
#define STR_l                       "l"
#define STR_m                       "m"
#define STR_n                       "n"
#define STR_o                       "o"
#define STR_p                       "p"
#define STR_q                       "q"
#define STR_r                       "r"
#define STR_s                       "s"
#define STR_t                       "t"
#define STR_u                       "u"
#define STR_v                       "v"
#define STR_w                       "w"
#define STR_x                       "x"
#define STR_y                       "y"
#define STR_z                       "z"
#define STR_LEFT_CURLY_BRACKET      "{"
#define STR_VERTICAL_LINE           "|"
#define STR_RIGHT_CURLY_BRACKET     "}"
#define STR_TILDE                   "~"

#define STRING_ACCEPT0              "ACCEPT\0"
#define STRING_COMMIT0              "COMMIT\0"
#define STRING_F0                   "F\0"
#define STRING_FAIL0                "FAIL\0"
#define STRING_MARK0                "MARK\0"
#define STRING_PRUNE0               "PRUNE\0"
#define STRING_SKIP0                "SKIP\0"
#define STRING_THEN                 "THEN"

#define STRING_alpha0               "alpha\0"
#define STRING_lower0               "lower\0"
#define STRING_upper0               "upper\0"
#define STRING_alnum0               "alnum\0"
#define STRING_ascii0               "ascii\0"
#define STRING_blank0               "blank\0"
#define STRING_cntrl0               "cntrl\0"
#define STRING_digit0               "digit\0"
#define STRING_graph0               "graph\0"
#define STRING_print0               "print\0"
#define STRING_punct0               "punct\0"
#define STRING_space0               "space\0"
#define STRING_word0                "word\0"
#define STRING_xdigit               "xdigit"

#define STRING_DEFINE               "DEFINE"
#define STRING_WEIRD_STARTWORD      "[:<:]]"
#define STRING_WEIRD_ENDWORD        "[:>:]]"

#define STRING_CR_RIGHTPAR              "CR)"
#define STRING_LF_RIGHTPAR              "LF)"
#define STRING_CRLF_RIGHTPAR            "CRLF)"
#define STRING_ANY_RIGHTPAR             "ANY)"
#define STRING_ANYCRLF_RIGHTPAR         "ANYCRLF)"
#define STRING_BSR_ANYCRLF_RIGHTPAR     "BSR_ANYCRLF)"
#define STRING_BSR_UNICODE_RIGHTPAR     "BSR_UNICODE)"
#define STRING_UTF8_RIGHTPAR            "UTF8)"
#define STRING_UTF16_RIGHTPAR           "UTF16)"
#define STRING_UTF32_RIGHTPAR           "UTF32)"
#define STRING_UTF_RIGHTPAR             "UTF)"
#define STRING_UCP_RIGHTPAR             "UCP)"
#define STRING_NO_AUTO_POSSESS_RIGHTPAR "NO_AUTO_POSSESS)"
#define STRING_NO_START_OPT_RIGHTPAR    "NO_START_OPT)"
#define STRING_LIMIT_MATCH_EQ           "LIMIT_MATCH="
#define STRING_LIMIT_RECURSION_EQ       "LIMIT_RECURSION="

#else  /* SUPPORT_UTF */

/* UTF-8 support is enabled; always use UTF-8 (=ASCII) character codes. This
works in both modes non-EBCDIC platforms, and on EBCDIC platforms in UTF-8 mode
only. */

#define CHAR_HT                     '\011'
#define CHAR_VT                     '\013'
#define CHAR_FF                     '\014'
#define CHAR_CR                     '\015'
#define CHAR_LF                     '\012'
#define CHAR_NL                     CHAR_LF
#define CHAR_NEL                    ((unsigned char)'\x85')
#define CHAR_BS                     '\010'
#define CHAR_BEL                    '\007'
#define CHAR_ESC                    '\033'
#define CHAR_DEL                    '\177'

#define CHAR_NULL                   '\0'
#define CHAR_SPACE                  '\040'
#define CHAR_EXCLAMATION_MARK       '\041'
#define CHAR_QUOTATION_MARK         '\042'
#define CHAR_NUMBER_SIGN            '\043'
#define CHAR_DOLLAR_SIGN            '\044'
#define CHAR_PERCENT_SIGN           '\045'
#define CHAR_AMPERSAND              '\046'
#define CHAR_APOSTROPHE             '\047'
#define CHAR_LEFT_PARENTHESIS       '\050'
#define CHAR_RIGHT_PARENTHESIS      '\051'
#define CHAR_ASTERISK               '\052'
#define CHAR_PLUS                   '\053'
#define CHAR_COMMA                  '\054'
#define CHAR_MINUS                  '\055'
#define CHAR_DOT                    '\056'
#define CHAR_SLASH                  '\057'
#define CHAR_0                      '\060'
#define CHAR_1                      '\061'
#define CHAR_2                      '\062'
#define CHAR_3                      '\063'
#define CHAR_4                      '\064'
#define CHAR_5                      '\065'
#define CHAR_6                      '\066'
#define CHAR_7                      '\067'
#define CHAR_8                      '\070'
#define CHAR_9                      '\071'
#define CHAR_COLON                  '\072'
#define CHAR_SEMICOLON              '\073'
#define CHAR_LESS_THAN_SIGN         '\074'
#define CHAR_EQUALS_SIGN            '\075'
#define CHAR_GREATER_THAN_SIGN      '\076'
#define CHAR_QUESTION_MARK          '\077'
#define CHAR_COMMERCIAL_AT          '\100'
#define CHAR_A                      '\101'
#define CHAR_B                      '\102'
#define CHAR_C                      '\103'
#define CHAR_D                      '\104'
#define CHAR_E                      '\105'
#define CHAR_F                      '\106'
#define CHAR_G                      '\107'
#define CHAR_H                      '\110'
#define CHAR_I                      '\111'
#define CHAR_J                      '\112'
#define CHAR_K                      '\113'
#define CHAR_L                      '\114'
#define CHAR_M                      '\115'
#define CHAR_N                      '\116'
#define CHAR_O                      '\117'
#define CHAR_P                      '\120'
#define CHAR_Q                      '\121'
#define CHAR_R                      '\122'
#define CHAR_S                      '\123'
#define CHAR_T                      '\124'
#define CHAR_U                      '\125'
#define CHAR_V                      '\126'
#define CHAR_W                      '\127'
#define CHAR_X                      '\130'
#define CHAR_Y                      '\131'
#define CHAR_Z                      '\132'
#define CHAR_LEFT_SQUARE_BRACKET    '\133'
#define CHAR_BACKSLASH              '\134'
#define CHAR_RIGHT_SQUARE_BRACKET   '\135'
#define CHAR_CIRCUMFLEX_ACCENT      '\136'
#define CHAR_UNDERSCORE             '\137'
#define CHAR_GRAVE_ACCENT           '\140'
#define CHAR_a                      '\141'
#define CHAR_b                      '\142'
#define CHAR_c                      '\143'
#define CHAR_d                      '\144'
#define CHAR_e                      '\145'
#define CHAR_f                      '\146'
#define CHAR_g                      '\147'
#define CHAR_h                      '\150'
#define CHAR_i                      '\151'
#define CHAR_j                      '\152'
#define CHAR_k                      '\153'
#define CHAR_l                      '\154'
#define CHAR_m                      '\155'
#define CHAR_n                      '\156'
#define CHAR_o                      '\157'
#define CHAR_p                      '\160'
#define CHAR_q                      '\161'
#define CHAR_r                      '\162'
#define CHAR_s                      '\163'
#define CHAR_t                      '\164'
#define CHAR_u                      '\165'
#define CHAR_v                      '\166'
#define CHAR_w                      '\167'
#define CHAR_x                      '\170'
#define CHAR_y                      '\171'
#define CHAR_z                      '\172'
#define CHAR_LEFT_CURLY_BRACKET     '\173'
#define CHAR_VERTICAL_LINE          '\174'
#define CHAR_RIGHT_CURLY_BRACKET    '\175'
#define CHAR_TILDE                  '\176'
#define CHAR_NBSP                   ((unsigned char)'\xa0')

#define STR_HT                      "\011"
#define STR_VT                      "\013"
#define STR_FF                      "\014"
#define STR_CR                      "\015"
#define STR_NL                      "\012"
#define STR_BS                      "\010"
#define STR_BEL                     "\007"
#define STR_ESC                     "\033"
#define STR_DEL                     "\177"

#define STR_SPACE                   "\040"
#define STR_EXCLAMATION_MARK        "\041"
#define STR_QUOTATION_MARK          "\042"
#define STR_NUMBER_SIGN             "\043"
#define STR_DOLLAR_SIGN             "\044"
#define STR_PERCENT_SIGN            "\045"
#define STR_AMPERSAND               "\046"
#define STR_APOSTROPHE              "\047"
#define STR_LEFT_PARENTHESIS        "\050"
#define STR_RIGHT_PARENTHESIS       "\051"
#define STR_ASTERISK                "\052"
#define STR_PLUS                    "\053"
#define STR_COMMA                   "\054"
#define STR_MINUS                   "\055"
#define STR_DOT                     "\056"
#define STR_SLASH                   "\057"
#define STR_0                       "\060"
#define STR_1                       "\061"
#define STR_2                       "\062"
#define STR_3                       "\063"
#define STR_4                       "\064"
#define STR_5                       "\065"
#define STR_6                       "\066"
#define STR_7                       "\067"
#define STR_8                       "\070"
#define STR_9                       "\071"
#define STR_COLON                   "\072"
#define STR_SEMICOLON               "\073"
#define STR_LESS_THAN_SIGN          "\074"
#define STR_EQUALS_SIGN             "\075"
#define STR_GREATER_THAN_SIGN       "\076"
#define STR_QUESTION_MARK           "\077"
#define STR_COMMERCIAL_AT           "\100"
#define STR_A                       "\101"
#define STR_B                       "\102"
#define STR_C                       "\103"
#define STR_D                       "\104"
#define STR_E                       "\105"
#define STR_F                       "\106"
#define STR_G                       "\107"
#define STR_H                       "\110"
#define STR_I                       "\111"
#define STR_J                       "\112"
#define STR_K                       "\113"
#define STR_L                       "\114"
#define STR_M                       "\115"
#define STR_N                       "\116"
#define STR_O                       "\117"
#define STR_P                       "\120"
#define STR_Q                       "\121"
#define STR_R                       "\122"
#define STR_S                       "\123"
#define STR_T                       "\124"
#define STR_U                       "\125"
#define STR_V                       "\126"
#define STR_W                       "\127"
#define STR_X                       "\130"
#define STR_Y                       "\131"
#define STR_Z                       "\132"
#define STR_LEFT_SQUARE_BRACKET     "\133"
#define STR_BACKSLASH               "\134"
#define STR_RIGHT_SQUARE_BRACKET    "\135"
#define STR_CIRCUMFLEX_ACCENT       "\136"
#define STR_UNDERSCORE              "\137"
#define STR_GRAVE_ACCENT            "\140"
#define STR_a                       "\141"
#define STR_b                       "\142"
#define STR_c                       "\143"
#define STR_d                       "\144"
#define STR_e                       "\145"
#define STR_f                       "\146"
#define STR_g                       "\147"
#define STR_h                       "\150"
#define STR_i                       "\151"
#define STR_j                       "\152"
#define STR_k                       "\153"
#define STR_l                       "\154"
#define STR_m                       "\155"
#define STR_n                       "\156"
#define STR_o                       "\157"
#define STR_p                       "\160"
#define STR_q                       "\161"
#define STR_r                       "\162"
#define STR_s                       "\163"
#define STR_t                       "\164"
#define STR_u                       "\165"
#define STR_v                       "\166"
#define STR_w                       "\167"
#define STR_x                       "\170"
#define STR_y                       "\171"
#define STR_z                       "\172"
#define STR_LEFT_CURLY_BRACKET      "\173"
#define STR_VERTICAL_LINE           "\174"
#define STR_RIGHT_CURLY_BRACKET     "\175"
#define STR_TILDE                   "\176"

#define STRING_ACCEPT0              STR_A STR_C STR_C STR_E STR_P STR_T "\0"
#define STRING_COMMIT0              STR_C STR_O STR_M STR_M STR_I STR_T "\0"
#define STRING_F0                   STR_F "\0"
#define STRING_FAIL0                STR_F STR_A STR_I STR_L "\0"
#define STRING_MARK0                STR_M STR_A STR_R STR_K "\0"
#define STRING_PRUNE0               STR_P STR_R STR_U STR_N STR_E "\0"
#define STRING_SKIP0                STR_S STR_K STR_I STR_P "\0"
#define STRING_THEN                 STR_T STR_H STR_E STR_N

#define STRING_alpha0               STR_a STR_l STR_p STR_h STR_a "\0"
#define STRING_lower0               STR_l STR_o STR_w STR_e STR_r "\0"
#define STRING_upper0               STR_u STR_p STR_p STR_e STR_r "\0"
#define STRING_alnum0               STR_a STR_l STR_n STR_u STR_m "\0"
#define STRING_ascii0               STR_a STR_s STR_c STR_i STR_i "\0"
#define STRING_blank0               STR_b STR_l STR_a STR_n STR_k "\0"
#define STRING_cntrl0               STR_c STR_n STR_t STR_r STR_l "\0"
#define STRING_digit0               STR_d STR_i STR_g STR_i STR_t "\0"
#define STRING_graph0               STR_g STR_r STR_a STR_p STR_h "\0"
#define STRING_print0               STR_p STR_r STR_i STR_n STR_t "\0"
#define STRING_punct0               STR_p STR_u STR_n STR_c STR_t "\0"
#define STRING_space0               STR_s STR_p STR_a STR_c STR_e "\0"
#define STRING_word0                STR_w STR_o STR_r STR_d       "\0"
#define STRING_xdigit               STR_x STR_d STR_i STR_g STR_i STR_t

#define STRING_DEFINE               STR_D STR_E STR_F STR_I STR_N STR_E
#define STRING_WEIRD_STARTWORD      STR_LEFT_SQUARE_BRACKET STR_COLON STR_LESS_THAN_SIGN STR_COLON STR_RIGHT_SQUARE_BRACKET STR_RIGHT_SQUARE_BRACKET
#define STRING_WEIRD_ENDWORD        STR_LEFT_SQUARE_BRACKET STR_COLON STR_GREATER_THAN_SIGN STR_COLON STR_RIGHT_SQUARE_BRACKET STR_RIGHT_SQUARE_BRACKET

#define STRING_CR_RIGHTPAR              STR_C STR_R STR_RIGHT_PARENTHESIS
#define STRING_LF_RIGHTPAR              STR_L STR_F STR_RIGHT_PARENTHESIS
#define STRING_CRLF_RIGHTPAR            STR_C STR_R STR_L STR_F STR_RIGHT_PARENTHESIS
#define STRING_ANY_RIGHTPAR             STR_A STR_N STR_Y STR_RIGHT_PARENTHESIS
#define STRING_ANYCRLF_RIGHTPAR         STR_A STR_N STR_Y STR_C STR_R STR_L STR_F STR_RIGHT_PARENTHESIS
#define STRING_BSR_ANYCRLF_RIGHTPAR     STR_B STR_S STR_R STR_UNDERSCORE STR_A STR_N STR_Y STR_C STR_R STR_L STR_F STR_RIGHT_PARENTHESIS
#define STRING_BSR_UNICODE_RIGHTPAR     STR_B STR_S STR_R STR_UNDERSCORE STR_U STR_N STR_I STR_C STR_O STR_D STR_E STR_RIGHT_PARENTHESIS
#define STRING_UTF8_RIGHTPAR            STR_U STR_T STR_F STR_8 STR_RIGHT_PARENTHESIS
#define STRING_UTF16_RIGHTPAR           STR_U STR_T STR_F STR_1 STR_6 STR_RIGHT_PARENTHESIS
#define STRING_UTF32_RIGHTPAR           STR_U STR_T STR_F STR_3 STR_2 STR_RIGHT_PARENTHESIS
#define STRING_UTF_RIGHTPAR             STR_U STR_T STR_F STR_RIGHT_PARENTHESIS
#define STRING_UCP_RIGHTPAR             STR_U STR_C STR_P STR_RIGHT_PARENTHESIS
#define STRING_NO_AUTO_POSSESS_RIGHTPAR STR_N STR_O STR_UNDERSCORE STR_A STR_U STR_T STR_O STR_UNDERSCORE STR_P STR_O STR_S STR_S STR_E STR_S STR_S STR_RIGHT_PARENTHESIS
#define STRING_NO_START_OPT_RIGHTPAR    STR_N STR_O STR_UNDERSCORE STR_S STR_T STR_A STR_R STR_T STR_UNDERSCORE STR_O STR_P STR_T STR_RIGHT_PARENTHESIS
#define STRING_LIMIT_MATCH_EQ           STR_L STR_I STR_M STR_I STR_T STR_UNDERSCORE STR_M STR_A STR_T STR_C STR_H STR_EQUALS_SIGN
#define STRING_LIMIT_RECURSION_EQ       STR_L STR_I STR_M STR_I STR_T STR_UNDERSCORE STR_R STR_E STR_C STR_U STR_R STR_S STR_I STR_O STR_N STR_EQUALS_SIGN

#endif  /* SUPPORT_UTF */

/* Escape items that are just an encoding of a particular data value. */

#ifndef ESC_a
#define ESC_a CHAR_BEL
#endif

#ifndef ESC_e
#define ESC_e CHAR_ESC
#endif

#ifndef ESC_f
#define ESC_f CHAR_FF
#endif

#ifndef ESC_n
#define ESC_n CHAR_LF
#endif

#ifndef ESC_r
#define ESC_r CHAR_CR
#endif

/* We can't officially use ESC_t because it is a POSIX reserved identifier
(presumably because of all the others like size_t). */

#ifndef ESC_tee
#define ESC_tee CHAR_HT
#endif

/* Codes for different types of Unicode property */

#define PT_ANY        0    /* Any property - matches all chars */
#define PT_LAMP       1    /* L& - the union of Lu, Ll, Lt */
#define PT_GC         2    /* Specified general characteristic (e.g. L) */
#define PT_PC         3    /* Specified particular characteristic (e.g. Lu) */
#define PT_SC         4    /* Script (e.g. Han) */
#define PT_ALNUM      5    /* Alphanumeric - the union of L and N */
#define PT_SPACE      6    /* Perl space - Z plus 9,10,12,13 */
#define PT_PXSPACE    7    /* POSIX space - Z plus 9,10,11,12,13 */
#define PT_WORD       8    /* Word - L plus N plus underscore */
#define PT_CLIST      9    /* Pseudo-property: match character list */
#define PT_UCNC      10    /* Universal Character nameable character */
#define PT_TABSIZE   11    /* Size of square table for autopossessify tests */

/* The following special properties are used only in XCLASS items, when POSIX
classes are specified and PCRE_UCP is set - in other words, for Unicode
handling of these classes. They are not available via the \p or \P escapes like
those in the above list, and so they do not take part in the autopossessifying
table. */

#define PT_PXGRAPH   11    /* [:graph:] - characters that mark the paper */
#define PT_PXPRINT   12    /* [:print:] - [:graph:] plus non-control spaces */
#define PT_PXPUNCT   13    /* [:punct:] - punctuation characters */

/* Flag bits and data types for the extended class (OP_XCLASS) for classes that
contain characters with values greater than 255. */

#define XCL_NOT       0x01    /* Flag: this is a negative class */
#define XCL_MAP       0x02    /* Flag: a 32-byte map is present */
#define XCL_HASPROP   0x04    /* Flag: property checks are present. */

#define XCL_END       0    /* Marks end of individual items */
#define XCL_SINGLE    1    /* Single item (one multibyte char) follows */
#define XCL_RANGE     2    /* A range (two multibyte chars) follows */
#define XCL_PROP      3    /* Unicode property (2-byte property code follows) */
#define XCL_NOTPROP   4    /* Unicode inverted property (ditto) */

/* These are escaped items that aren't just an encoding of a particular data
value such as \n. They must have non-zero values, as check_escape() returns 0
for a data character.  Also, they must appear in the same order as in the
opcode definitions below, up to ESC_z. There's a dummy for OP_ALLANY because it
corresponds to "." in DOTALL mode rather than an escape sequence. It is also
used for [^] in JavaScript compatibility mode, and for \C in non-utf mode. In
non-DOTALL mode, "." behaves like \N.

The special values ESC_DU, ESC_du, etc. are used instead of ESC_D, ESC_d, etc.
when PCRE_UCP is set and replacement of \d etc by \p sequences is required.
They must be contiguous, and remain in order so that the replacements can be
looked up from a table.

Negative numbers are used to encode a backreference (\1, \2, \3, etc.) in
check_escape(). There are two tests in the code for an escape
greater than ESC_b and less than ESC_Z to detect the types that may be
repeated. These are the types that consume characters. If any new escapes are
put in between that don't consume a character, that code will have to change.
*/

enum { ESC_A = 1, ESC_G, ESC_K, ESC_B, ESC_b, ESC_D, ESC_d, ESC_S, ESC_s,
       ESC_W, ESC_w, ESC_N, ESC_dum, ESC_C, ESC_P, ESC_p, ESC_R, ESC_H,
       ESC_h, ESC_V, ESC_v, ESC_X, ESC_Z, ESC_z,
       ESC_E, ESC_Q, ESC_g, ESC_k,
       ESC_DU, ESC_du, ESC_SU, ESC_su, ESC_WU, ESC_wu };


/********************** Opcode definitions ******************/

/****** NOTE NOTE NOTE ******

Starting from 1 (i.e. after OP_END), the values up to OP_EOD must correspond in
order to the list of escapes immediately above. Furthermore, values up to
OP_DOLLM must not be changed without adjusting the table called autoposstab in
pcre_compile.c

Whenever this list is updated, the two macro definitions that follow must be
updated to match. The possessification table called "opcode_possessify" in
pcre_compile.c must also be updated, and also the tables called "coptable"
and "poptable" in pcre_dfa_exec.c.

****** NOTE NOTE NOTE ******/


/* The values between FIRST_AUTOTAB_OP and LAST_AUTOTAB_RIGHT_OP, inclusive,
are used in a table for deciding whether a repeated character type can be
auto-possessified. */

#define FIRST_AUTOTAB_OP       OP_NOT_DIGIT
#define LAST_AUTOTAB_LEFT_OP   OP_EXTUNI
#define LAST_AUTOTAB_RIGHT_OP  OP_DOLLM

enum {
  OP_END,            /* 0 End of pattern */

  /* Values corresponding to backslashed metacharacters */

  OP_SOD,            /* 1 Start of data: \A */
  OP_SOM,            /* 2 Start of match (subject + offset): \G */
  OP_SET_SOM,        /* 3 Set start of match (\K) */
  OP_NOT_WORD_BOUNDARY,  /*  4 \B */
  OP_WORD_BOUNDARY,      /*  5 \b */
  OP_NOT_DIGIT,          /*  6 \D */
  OP_DIGIT,              /*  7 \d */
  OP_NOT_WHITESPACE,     /*  8 \S */
  OP_WHITESPACE,         /*  9 \s */
  OP_NOT_WORDCHAR,       /* 10 \W */
  OP_WORDCHAR,           /* 11 \w */

  OP_ANY,            /* 12 Match any character except newline (\N) */
  OP_ALLANY,         /* 13 Match any character */
  OP_ANYBYTE,        /* 14 Match any byte (\C); different to OP_ANY for UTF-8 */
  OP_NOTPROP,        /* 15 \P (not Unicode property) */
  OP_PROP,           /* 16 \p (Unicode property) */
  OP_ANYNL,          /* 17 \R (any newline sequence) */
  OP_NOT_HSPACE,     /* 18 \H (not horizontal whitespace) */
  OP_HSPACE,         /* 19 \h (horizontal whitespace) */
  OP_NOT_VSPACE,     /* 20 \V (not vertical whitespace) */
  OP_VSPACE,         /* 21 \v (vertical whitespace) */
  OP_EXTUNI,         /* 22 \X (extended Unicode sequence */
  OP_EODN,           /* 23 End of data or \n at end of data (\Z) */
  OP_EOD,            /* 24 End of data (\z) */

  /* Line end assertions */

  OP_DOLL,           /* 25 End of line - not multiline */
  OP_DOLLM,          /* 26 End of line - multiline */
  OP_CIRC,           /* 27 Start of line - not multiline */
  OP_CIRCM,          /* 28 Start of line - multiline */

  /* Single characters; caseful must precede the caseless ones */

  OP_CHAR,           /* 29 Match one character, casefully */
  OP_CHARI,          /* 30 Match one character, caselessly */
  OP_NOT,            /* 31 Match one character, not the given one, casefully */
  OP_NOTI,           /* 32 Match one character, not the given one, caselessly */

  /* The following sets of 13 opcodes must always be kept in step because
  the offset from the first one is used to generate the others. */

  /* Repeated characters; caseful must precede the caseless ones */

  OP_STAR,           /* 33 The maximizing and minimizing versions of */
  OP_MINSTAR,        /* 34 these six opcodes must come in pairs, with */
  OP_PLUS,           /* 35 the minimizing one second. */
  OP_MINPLUS,        /* 36 */
  OP_QUERY,          /* 37 */
  OP_MINQUERY,       /* 38 */

  OP_UPTO,           /* 39 From 0 to n matches of one character, caseful*/
  OP_MINUPTO,        /* 40 */
  OP_EXACT,          /* 41 Exactly n matches */

  OP_POSSTAR,        /* 42 Possessified star, caseful */
  OP_POSPLUS,        /* 43 Possessified plus, caseful */
  OP_POSQUERY,       /* 44 Posesssified query, caseful */
  OP_POSUPTO,        /* 45 Possessified upto, caseful */

  /* Repeated characters; caseless must follow the caseful ones */

  OP_STARI,          /* 46 */
  OP_MINSTARI,       /* 47 */
  OP_PLUSI,          /* 48 */
  OP_MINPLUSI,       /* 49 */
  OP_QUERYI,         /* 50 */
  OP_MINQUERYI,      /* 51 */

  OP_UPTOI,          /* 52 From 0 to n matches of one character, caseless */
  OP_MINUPTOI,       /* 53 */
  OP_EXACTI,         /* 54 */

  OP_POSSTARI,       /* 55 Possessified star, caseless */
  OP_POSPLUSI,       /* 56 Possessified plus, caseless */
  OP_POSQUERYI,      /* 57 Posesssified query, caseless */
  OP_POSUPTOI,       /* 58 Possessified upto, caseless */

  /* The negated ones must follow the non-negated ones, and match them */
  /* Negated repeated character, caseful; must precede the caseless ones */

  OP_NOTSTAR,        /* 59 The maximizing and minimizing versions of */
  OP_NOTMINSTAR,     /* 60 these six opcodes must come in pairs, with */
  OP_NOTPLUS,        /* 61 the minimizing one second. They must be in */
  OP_NOTMINPLUS,     /* 62 exactly the same order as those above. */
  OP_NOTQUERY,       /* 63 */
  OP_NOTMINQUERY,    /* 64 */

  OP_NOTUPTO,        /* 65 From 0 to n matches, caseful */
  OP_NOTMINUPTO,     /* 66 */
  OP_NOTEXACT,       /* 67 Exactly n matches */

  OP_NOTPOSSTAR,     /* 68 Possessified versions, caseful */
  OP_NOTPOSPLUS,     /* 69 */
  OP_NOTPOSQUERY,    /* 70 */
  OP_NOTPOSUPTO,     /* 71 */

  /* Negated repeated character, caseless; must follow the caseful ones */

  OP_NOTSTARI,       /* 72 */
  OP_NOTMINSTARI,    /* 73 */
  OP_NOTPLUSI,       /* 74 */
  OP_NOTMINPLUSI,    /* 75 */
  OP_NOTQUERYI,      /* 76 */
  OP_NOTMINQUERYI,   /* 77 */

  OP_NOTUPTOI,       /* 78 From 0 to n matches, caseless */
  OP_NOTMINUPTOI,    /* 79 */
  OP_NOTEXACTI,      /* 80 Exactly n matches */

  OP_NOTPOSSTARI,    /* 81 Possessified versions, caseless */
  OP_NOTPOSPLUSI,    /* 82 */
  OP_NOTPOSQUERYI,   /* 83 */
  OP_NOTPOSUPTOI,    /* 84 */

  /* Character types */

  OP_TYPESTAR,       /* 85 The maximizing and minimizing versions of */
  OP_TYPEMINSTAR,    /* 86 these six opcodes must come in pairs, with */
  OP_TYPEPLUS,       /* 87 the minimizing one second. These codes must */
  OP_TYPEMINPLUS,    /* 88 be in exactly the same order as those above. */
  OP_TYPEQUERY,      /* 89 */
  OP_TYPEMINQUERY,   /* 90 */

  OP_TYPEUPTO,       /* 91 From 0 to n matches */
  OP_TYPEMINUPTO,    /* 92 */
  OP_TYPEEXACT,      /* 93 Exactly n matches */

  OP_TYPEPOSSTAR,    /* 94 Possessified versions */
  OP_TYPEPOSPLUS,    /* 95 */
  OP_TYPEPOSQUERY,   /* 96 */
  OP_TYPEPOSUPTO,    /* 97 */

  /* These are used for character classes and back references; only the
  first six are the same as the sets above. */

  OP_CRSTAR,         /* 98 The maximizing and minimizing versions of */
  OP_CRMINSTAR,      /* 99 all these opcodes must come in pairs, with */
  OP_CRPLUS,         /* 100 the minimizing one second. These codes must */
  OP_CRMINPLUS,      /* 101 be in exactly the same order as those above. */
  OP_CRQUERY,        /* 102 */
  OP_CRMINQUERY,     /* 103 */

  OP_CRRANGE,        /* 104 These are different to the three sets above. */
  OP_CRMINRANGE,     /* 105 */

  OP_CRPOSSTAR,      /* 106 Possessified versions */
  OP_CRPOSPLUS,      /* 107 */
  OP_CRPOSQUERY,     /* 108 */
  OP_CRPOSRANGE,     /* 109 */

  /* End of quantifier opcodes */

  OP_CLASS,          /* 110 Match a character class, chars < 256 only */
  OP_NCLASS,         /* 111 Same, but the bitmap was created from a negative
                              class - the difference is relevant only when a
                              character > 255 is encountered. */
  OP_XCLASS,         /* 112 Extended class for handling > 255 chars within the
                              class. This does both positive and negative. */
  OP_REF,            /* 113 Match a back reference, casefully */
  OP_REFI,           /* 114 Match a back reference, caselessly */
  OP_DNREF,          /* 115 Match a duplicate name backref, casefully */
  OP_DNREFI,         /* 116 Match a duplicate name backref, caselessly */
  OP_RECURSE,        /* 117 Match a numbered subpattern (possibly recursive) */
  OP_CALLOUT,        /* 118 Call out to external function if provided */

  OP_ALT,            /* 119 Start of alternation */
  OP_KET,            /* 120 End of group that doesn't have an unbounded repeat */
  OP_KETRMAX,        /* 121 These two must remain together and in this */
  OP_KETRMIN,        /* 122 order. They are for groups the repeat for ever. */
  OP_KETRPOS,        /* 123 Possessive unlimited repeat. */

  /* The assertions must come before BRA, CBRA, ONCE, and COND, and the four
  asserts must remain in order. */

  OP_REVERSE,        /* 124 Move pointer back - used in lookbehind assertions */
  OP_ASSERT,         /* 125 Positive lookahead */
  OP_ASSERT_NOT,     /* 126 Negative lookahead */
  OP_ASSERTBACK,     /* 127 Positive lookbehind */
  OP_ASSERTBACK_NOT, /* 128 Negative lookbehind */

  /* ONCE, ONCE_NC, BRA, BRAPOS, CBRA, CBRAPOS, and COND must come immediately
  after the assertions, with ONCE first, as there's a test for >= ONCE for a
  subpattern that isn't an assertion. The POS versions must immediately follow
  the non-POS versions in each case. */

  OP_ONCE,           /* 129 Atomic group, contains captures */
  OP_ONCE_NC,        /* 130 Atomic group containing no captures */
  OP_BRA,            /* 131 Start of non-capturing bracket */
  OP_BRAPOS,         /* 132 Ditto, with unlimited, possessive repeat */
  OP_CBRA,           /* 133 Start of capturing bracket */
  OP_CBRAPOS,        /* 134 Ditto, with unlimited, possessive repeat */
  OP_COND,           /* 135 Conditional group */

  /* These five must follow the previous five, in the same order. There's a
  check for >= SBRA to distinguish the two sets. */

  OP_SBRA,           /* 136 Start of non-capturing bracket, check empty  */
  OP_SBRAPOS,        /* 137 Ditto, with unlimited, possessive repeat */
  OP_SCBRA,          /* 138 Start of capturing bracket, check empty */
  OP_SCBRAPOS,       /* 139 Ditto, with unlimited, possessive repeat */
  OP_SCOND,          /* 140 Conditional group, check empty */

  /* The next two pairs must (respectively) be kept together. */

  OP_CREF,           /* 141 Used to hold a capture number as condition */
  OP_DNCREF,         /* 142 Used to point to duplicate names as a condition */
  OP_RREF,           /* 143 Used to hold a recursion number as condition */
  OP_DNRREF,         /* 144 Used to point to duplicate names as a condition */
  OP_DEF,            /* 145 The DEFINE condition */

  OP_BRAZERO,        /* 146 These two must remain together and in this */
  OP_BRAMINZERO,     /* 147 order. */
  OP_BRAPOSZERO,     /* 148 */

  /* These are backtracking control verbs */

  OP_MARK,           /* 149 always has an argument */
  OP_PRUNE,          /* 150 */
  OP_PRUNE_ARG,      /* 151 same, but with argument */
  OP_SKIP,           /* 152 */
  OP_SKIP_ARG,       /* 153 same, but with argument */
  OP_THEN,           /* 154 */
  OP_THEN_ARG,       /* 155 same, but with argument */
  OP_COMMIT,         /* 156 */

  /* These are forced failure and success verbs */

  OP_FAIL,           /* 157 */
  OP_ACCEPT,         /* 158 */
  OP_ASSERT_ACCEPT,  /* 159 Used inside assertions */
  OP_CLOSE,          /* 160 Used before OP_ACCEPT to close open captures */

  /* This is used to skip a subpattern with a {0} quantifier */

  OP_SKIPZERO,       /* 161 */

  /* This is not an opcode, but is used to check that tables indexed by opcode
  are the correct length, in order to catch updating errors - there have been
  some in the past. */

  OP_TABLE_LENGTH
};

/* *** NOTE NOTE NOTE *** Whenever the list above is updated, the two macro
definitions that follow must also be updated to match. There are also tables
called "opcode_possessify" in pcre_compile.c and "coptable" and "poptable" in
pcre_dfa_exec.c that must be updated. */


/* This macro defines textual names for all the opcodes. These are used only
for debugging, and some of them are only partial names. The macro is referenced
only in pcre_printint.c, which fills out the full names in many cases (and in
some cases doesn't actually use these names at all). */

#define OP_NAME_LIST \
  "End", "\\A", "\\G", "\\K", "\\B", "\\b", "\\D", "\\d",         \
  "\\S", "\\s", "\\W", "\\w", "Any", "AllAny", "Anybyte",         \
  "notprop", "prop", "\\R", "\\H", "\\h", "\\V", "\\v",           \
  "extuni",  "\\Z", "\\z",                                        \
  "$", "$", "^", "^", "char", "chari", "not", "noti",             \
  "*", "*?", "+", "+?", "?", "??",                                \
  "{", "{", "{",                                                  \
  "*+","++", "?+", "{",                                           \
  "*", "*?", "+", "+?", "?", "??",                                \
  "{", "{", "{",                                                  \
  "*+","++", "?+", "{",                                           \
  "*", "*?", "+", "+?", "?", "??",                                \
  "{", "{", "{",                                                  \
  "*+","++", "?+", "{",                                           \
  "*", "*?", "+", "+?", "?", "??",                                \
  "{", "{", "{",                                                  \
  "*+","++", "?+", "{",                                           \
  "*", "*?", "+", "+?", "?", "??", "{", "{", "{",                 \
  "*+","++", "?+", "{",                                           \
  "*", "*?", "+", "+?", "?", "??", "{", "{",                      \
  "*+","++", "?+", "{",                                           \
  "class", "nclass", "xclass", "Ref", "Refi", "DnRef", "DnRefi",  \
  "Recurse", "Callout",                                           \
  "Alt", "Ket", "KetRmax", "KetRmin", "KetRpos",                  \
  "Reverse", "Assert", "Assert not", "AssertB", "AssertB not",    \
  "Once", "Once_NC",                                              \
  "Bra", "BraPos", "CBra", "CBraPos",                             \
  "Cond",                                                         \
  "SBra", "SBraPos", "SCBra", "SCBraPos",                         \
  "SCond",                                                        \
  "Cond ref", "Cond dnref", "Cond rec", "Cond dnrec", "Cond def", \
  "Brazero", "Braminzero", "Braposzero",                          \
  "*MARK", "*PRUNE", "*PRUNE", "*SKIP", "*SKIP",                  \
  "*THEN", "*THEN", "*COMMIT", "*FAIL",                           \
  "*ACCEPT", "*ASSERT_ACCEPT",                                    \
  "Close", "Skip zero"


/* This macro defines the length of fixed length operations in the compiled
regex. The lengths are used when searching for specific things, and also in the
debugging printing of a compiled regex. We use a macro so that it can be
defined close to the definitions of the opcodes themselves.

As things have been extended, some of these are no longer fixed lenths, but are
minima instead. For example, the length of a single-character repeat may vary
in UTF-8 mode. The code that uses this table must know about such things. */

#define OP_LENGTHS \
  1,                             /* End                                    */ \
  1, 1, 1, 1, 1,                 /* \A, \G, \K, \B, \b                     */ \
  1, 1, 1, 1, 1, 1,              /* \D, \d, \S, \s, \W, \w                 */ \
  1, 1, 1,                       /* Any, AllAny, Anybyte                   */ \
  3, 3,                          /* \P, \p                                 */ \
  1, 1, 1, 1, 1,                 /* \R, \H, \h, \V, \v                     */ \
  1,                             /* \X                                     */ \
  1, 1, 1, 1, 1, 1,              /* \Z, \z, $, $M ^, ^M                    */ \
  2,                             /* Char  - the minimum length             */ \
  2,                             /* Chari  - the minimum length            */ \
  2,                             /* not                                    */ \
  2,                             /* noti                                   */ \
  /* Positive single-char repeats                             ** These are */ \
  2, 2, 2, 2, 2, 2,              /* *, *?, +, +?, ?, ??       ** minima in */ \
  2+IMM2_SIZE, 2+IMM2_SIZE,      /* upto, minupto             ** mode      */ \
  2+IMM2_SIZE,                   /* exact                                  */ \
  2, 2, 2, 2+IMM2_SIZE,          /* *+, ++, ?+, upto+                      */ \
  2, 2, 2, 2, 2, 2,              /* *I, *?I, +I, +?I, ?I, ??I ** UTF-8     */ \
  2+IMM2_SIZE, 2+IMM2_SIZE,      /* upto I, minupto I                      */ \
  2+IMM2_SIZE,                   /* exact I                                */ \
  2, 2, 2, 2+IMM2_SIZE,          /* *+I, ++I, ?+I, upto+I                  */ \
  /* Negative single-char repeats - only for chars < 256                   */ \
  2, 2, 2, 2, 2, 2,              /* NOT *, *?, +, +?, ?, ??                */ \
  2+IMM2_SIZE, 2+IMM2_SIZE,      /* NOT upto, minupto                      */ \
  2+IMM2_SIZE,                   /* NOT exact                              */ \
  2, 2, 2, 2+IMM2_SIZE,          /* Possessive NOT *, +, ?, upto           */ \
  2, 2, 2, 2, 2, 2,              /* NOT *I, *?I, +I, +?I, ?I, ??I          */ \
  2+IMM2_SIZE, 2+IMM2_SIZE,      /* NOT upto I, minupto I                  */ \
  2+IMM2_SIZE,                   /* NOT exact I                            */ \
  2, 2, 2, 2+IMM2_SIZE,          /* Possessive NOT *I, +I, ?I, upto I      */ \
  /* Positive type repeats                                                 */ \
  2, 2, 2, 2, 2, 2,              /* Type *, *?, +, +?, ?, ??               */ \
  2+IMM2_SIZE, 2+IMM2_SIZE,      /* Type upto, minupto                     */ \
  2+IMM2_SIZE,                   /* Type exact                             */ \
  2, 2, 2, 2+IMM2_SIZE,          /* Possessive *+, ++, ?+, upto+           */ \
  /* Character class & ref repeats                                         */ \
  1, 1, 1, 1, 1, 1,              /* *, *?, +, +?, ?, ??                    */ \
  1+2*IMM2_SIZE, 1+2*IMM2_SIZE,  /* CRRANGE, CRMINRANGE                    */ \
  1, 1, 1, 1+2*IMM2_SIZE,        /* Possessive *+, ++, ?+, CRPOSRANGE      */ \
  1+(32/sizeof(pcre_uchar)),     /* CLASS                                  */ \
  1+(32/sizeof(pcre_uchar)),     /* NCLASS                                 */ \
  0,                             /* XCLASS - variable length               */ \
  1+IMM2_SIZE,                   /* REF                                    */ \
  1+IMM2_SIZE,                   /* REFI                                   */ \
  1+2*IMM2_SIZE,                 /* DNREF                                  */ \
  1+2*IMM2_SIZE,                 /* DNREFI                                 */ \
  1+LINK_SIZE,                   /* RECURSE                                */ \
  2+2*LINK_SIZE,                 /* CALLOUT                                */ \
  1+LINK_SIZE,                   /* Alt                                    */ \
  1+LINK_SIZE,                   /* Ket                                    */ \
  1+LINK_SIZE,                   /* KetRmax                                */ \
  1+LINK_SIZE,                   /* KetRmin                                */ \
  1+LINK_SIZE,                   /* KetRpos                                */ \
  1+LINK_SIZE,                   /* Reverse                                */ \
  1+LINK_SIZE,                   /* Assert                                 */ \
  1+LINK_SIZE,                   /* Assert not                             */ \
  1+LINK_SIZE,                   /* Assert behind                          */ \
  1+LINK_SIZE,                   /* Assert behind not                      */ \
  1+LINK_SIZE,                   /* ONCE                                   */ \
  1+LINK_SIZE,                   /* ONCE_NC                                */ \
  1+LINK_SIZE,                   /* BRA                                    */ \
  1+LINK_SIZE,                   /* BRAPOS                                 */ \
  1+LINK_SIZE+IMM2_SIZE,         /* CBRA                                   */ \
  1+LINK_SIZE+IMM2_SIZE,         /* CBRAPOS                                */ \
  1+LINK_SIZE,                   /* COND                                   */ \
  1+LINK_SIZE,                   /* SBRA                                   */ \
  1+LINK_SIZE,                   /* SBRAPOS                                */ \
  1+LINK_SIZE+IMM2_SIZE,         /* SCBRA                                  */ \
  1+LINK_SIZE+IMM2_SIZE,         /* SCBRAPOS                               */ \
  1+LINK_SIZE,                   /* SCOND                                  */ \
  1+IMM2_SIZE, 1+2*IMM2_SIZE,    /* CREF, DNCREF                           */ \
  1+IMM2_SIZE, 1+2*IMM2_SIZE,    /* RREF, DNRREF                           */ \
  1,                             /* DEF                                    */ \
  1, 1, 1,                       /* BRAZERO, BRAMINZERO, BRAPOSZERO        */ \
  3, 1, 3,                       /* MARK, PRUNE, PRUNE_ARG                 */ \
  1, 3,                          /* SKIP, SKIP_ARG                         */ \
  1, 3,                          /* THEN, THEN_ARG                         */ \
  1, 1, 1, 1,                    /* COMMIT, FAIL, ACCEPT, ASSERT_ACCEPT    */ \
  1+IMM2_SIZE, 1                 /* CLOSE, SKIPZERO                        */

/* A magic value for OP_RREF to indicate the "any recursion" condition. */

#define RREF_ANY  0xffff

/* Compile time error code numbers. They are given names so that they can more
easily be tracked. When a new number is added, the table called eint in
pcreposix.c must be updated. */

enum { ERR0,  ERR1,  ERR2,  ERR3,  ERR4,  ERR5,  ERR6,  ERR7,  ERR8,  ERR9,
       ERR10, ERR11, ERR12, ERR13, ERR14, ERR15, ERR16, ERR17, ERR18, ERR19,
       ERR20, ERR21, ERR22, ERR23, ERR24, ERR25, ERR26, ERR27, ERR28, ERR29,
       ERR30, ERR31, ERR32, ERR33, ERR34, ERR35, ERR36, ERR37, ERR38, ERR39,
       ERR40, ERR41, ERR42, ERR43, ERR44, ERR45, ERR46, ERR47, ERR48, ERR49,
       ERR50, ERR51, ERR52, ERR53, ERR54, ERR55, ERR56, ERR57, ERR58, ERR59,
       ERR60, ERR61, ERR62, ERR63, ERR64, ERR65, ERR66, ERR67, ERR68, ERR69,
       ERR70, ERR71, ERR72, ERR73, ERR74, ERR75, ERR76, ERR77, ERR78, ERR79,
       ERR80, ERR81, ERR82, ERR83, ERR84, ERR85, ERR86, ERRCOUNT };

/* JIT compiling modes. The function list is indexed by them. */

enum { JIT_COMPILE, JIT_PARTIAL_SOFT_COMPILE, JIT_PARTIAL_HARD_COMPILE,
       JIT_NUMBER_OF_COMPILE_MODES };

/* The real format of the start of the pcre block; the index of names and the
code vector run on as long as necessary after the end. We store an explicit
offset to the name table so that if a regex is compiled on one host, saved, and
then run on another where the size of pointers is different, all might still
be well.

The size of the structure must be a multiple of 8 bytes. For the case of
compiled-on-4 and run-on-8, we include an extra pointer that is always NULL so
that there are an even number of pointers which therefore are a multiple of 8
bytes.

It is necessary to fork the struct for the 32 bit library, since it needs to
use pcre_uint32 for first_char and req_char. We can't put an ifdef inside the
typedef because pcretest needs access to the struct of the 8-, 16- and 32-bit
variants.

*** WARNING ***
When new fields are added to these structures, remember to adjust the code in
pcre_byte_order.c that is concerned with swapping the byte order of the fields
when a compiled regex is reloaded on a host with different endianness.
*** WARNING ***
There is also similar byte-flipping code in pcretest.c, which is used for
testing the byte-flipping features. It must also be kept in step.
*** WARNING ***
*/

typedef struct real_pcre8_or_16 {
  pcre_uint32 magic_number;
  pcre_uint32 size;               /* Total that was malloced */
  pcre_uint32 options;            /* Public options */
  pcre_uint32 flags;              /* Private flags */
  pcre_uint32 limit_match;        /* Limit set from regex */
  pcre_uint32 limit_recursion;    /* Limit set from regex */
  pcre_uint16 first_char;         /* Starting character */
  pcre_uint16 req_char;           /* This character must be seen */
  pcre_uint16 max_lookbehind;     /* Longest lookbehind (characters) */
  pcre_uint16 top_bracket;        /* Highest numbered group */
  pcre_uint16 top_backref;        /* Highest numbered back reference */
  pcre_uint16 name_table_offset;  /* Offset to name table that follows */
  pcre_uint16 name_entry_size;    /* Size of any name items */
  pcre_uint16 name_count;         /* Number of name items */
  pcre_uint16 ref_count;          /* Reference count */
  pcre_uint16 dummy1;             /* To ensure size is a multiple of 8 */
  pcre_uint16 dummy2;             /* To ensure size is a multiple of 8 */
  pcre_uint16 dummy3;             /* To ensure size is a multiple of 8 */
  const pcre_uint8 *tables;       /* Pointer to tables or NULL for std */
  void             *nullpad;      /* NULL padding */
} real_pcre8_or_16;

typedef struct real_pcre8_or_16 real_pcre;
typedef struct real_pcre8_or_16 real_pcre16;

typedef struct real_pcre32 {
  pcre_uint32 magic_number;
  pcre_uint32 size;               /* Total that was malloced */
  pcre_uint32 options;            /* Public options */
  pcre_uint32 flags;              /* Private flags */
  pcre_uint32 limit_match;        /* Limit set from regex */
  pcre_uint32 limit_recursion;    /* Limit set from regex */
  pcre_uint32 first_char;         /* Starting character */
  pcre_uint32 req_char;           /* This character must be seen */
  pcre_uint16 max_lookbehind;     /* Longest lookbehind (characters) */
  pcre_uint16 top_bracket;        /* Highest numbered group */
  pcre_uint16 top_backref;        /* Highest numbered back reference */
  pcre_uint16 name_table_offset;  /* Offset to name table that follows */
  pcre_uint16 name_entry_size;    /* Size of any name items */
  pcre_uint16 name_count;         /* Number of name items */
  pcre_uint16 ref_count;          /* Reference count */
  pcre_uint16 dummy;              /* To ensure size is a multiple of 8 */
  const pcre_uint8 *tables;       /* Pointer to tables or NULL for std */
  void             *nullpad;      /* NULL padding */
} real_pcre32;

#if defined COMPILE_PCRE8
#define REAL_PCRE real_pcre
#elif defined COMPILE_PCRE16
#define REAL_PCRE real_pcre16
#elif defined COMPILE_PCRE32
#define REAL_PCRE real_pcre32
#endif

/* Assert that the size of REAL_PCRE is divisible by 8 */
typedef int __assert_real_pcre_size_divisible_8[(sizeof(REAL_PCRE) % 8) == 0 ? 1 : -1];

/* Needed in pcretest to access some fields in the real_pcre* structures
 * directly. They're unified for 8/16/32 bits since the structs only differ
 * after these fields; if that ever changes, need to fork those defines into
 * 8/16 and 32 bit versions. */
#define REAL_PCRE_MAGIC(re)     (((REAL_PCRE*)re)->magic_number)
#define REAL_PCRE_SIZE(re)      (((REAL_PCRE*)re)->size)
#define REAL_PCRE_OPTIONS(re)   (((REAL_PCRE*)re)->options)
#define REAL_PCRE_FLAGS(re)     (((REAL_PCRE*)re)->flags)

/* The format of the block used to store data from pcre_study(). The same
remark (see NOTE above) about extending this structure applies. */

typedef struct pcre_study_data {
  pcre_uint32 size;               /* Total that was malloced */
  pcre_uint32 flags;              /* Private flags */
  pcre_uint8 start_bits[32];      /* Starting char bits */
  pcre_uint32 minlength;          /* Minimum subject length */
} pcre_study_data;

/* Structure for building a chain of open capturing subpatterns during
compiling, so that instructions to close them can be compiled when (*ACCEPT) is
encountered. This is also used to identify subpatterns that contain recursive
back references to themselves, so that they can be made atomic. */

typedef struct open_capitem {
  struct open_capitem *next;    /* Chain link */
  pcre_uint16 number;           /* Capture number */
  pcre_uint16 flag;             /* Set TRUE if recursive back ref */
} open_capitem;

/* Structure for building a list of named groups during the first pass of
compiling. */

typedef struct named_group {
  const pcre_uchar  *name;          /* Points to the name in the pattern */
  int                length;        /* Length of the name */
  pcre_uint32        number;        /* Group number */
} named_group;

/* Structure for passing "static" information around between the functions
doing the compiling, so that they are thread-safe. */

typedef struct compile_data {
  const pcre_uint8 *lcc;            /* Points to lower casing table */
  const pcre_uint8 *fcc;            /* Points to case-flipping table */
  const pcre_uint8 *cbits;          /* Points to character type table */
  const pcre_uint8 *ctypes;         /* Points to table of type maps */
  const pcre_uchar *start_workspace;/* The start of working space */
  const pcre_uchar *start_code;     /* The start of the compiled code */
  const pcre_uchar *start_pattern;  /* The start of the pattern */
  const pcre_uchar *end_pattern;    /* The end of the pattern */
  pcre_uchar *hwm;                  /* High watermark of workspace */
  open_capitem *open_caps;          /* Chain of open capture items */
  named_group *named_groups;        /* Points to vector in pre-compile */
  pcre_uchar *name_table;           /* The name/number table */
  int  names_found;                 /* Number of entries so far */
  int  name_entry_size;             /* Size of each entry */
  int  named_group_list_size;       /* Number of entries in the list */
  int  workspace_size;              /* Size of workspace */
  unsigned int bracount;            /* Count of capturing parens as we compile */
  int  final_bracount;              /* Saved value after first pass */
  int  max_lookbehind;              /* Maximum lookbehind (characters) */
  int  top_backref;                 /* Maximum back reference */
  unsigned int backref_map;         /* Bitmap of low back refs */
  unsigned int namedrefcount;       /* Number of backreferences by name */
  int  parens_depth;                /* Depth of nested parentheses */
  int  assert_depth;                /* Depth of nested assertions */
  pcre_uint32 external_options;     /* External (initial) options */
  pcre_uint32 external_flags;       /* External flag bits to be set */
  int  req_varyopt;                 /* "After variable item" flag for reqbyte */
  BOOL had_accept;                  /* (*ACCEPT) encountered */
  BOOL had_pruneorskip;             /* (*PRUNE) or (*SKIP) encountered */
  BOOL check_lookbehind;            /* Lookbehinds need later checking */
  BOOL dupnames;                    /* Duplicate names exist */
  BOOL dupgroups;                   /* Duplicate groups exist: (?| found */
  BOOL iscondassert;                /* Next assert is a condition */
  int  nltype;                      /* Newline type */
  int  nllen;                       /* Newline string length */
  pcre_uchar nl[4];                 /* Newline string when fixed length */
} compile_data;

/* Structure for maintaining a chain of pointers to the currently incomplete
branches, for testing for left recursion while compiling. */

typedef struct branch_chain {
  struct branch_chain *outer;
  pcre_uchar *current_branch;
} branch_chain;

/* Structure for mutual recursion detection. */

typedef struct recurse_check {
  struct recurse_check *prev;
  const pcre_uchar *group;
} recurse_check;

/* Structure for items in a linked list that represents an explicit recursive
call within the pattern; used by pcre_exec(). */

typedef struct recursion_info {
  struct recursion_info *prevrec; /* Previous recursion record (or NULL) */
  unsigned int group_num;         /* Number of group that was called */
  int *offset_save;               /* Pointer to start of saved offsets */
  int saved_max;                  /* Number of saved offsets */
  int saved_capture_last;         /* Last capture number */
  PCRE_PUCHAR subject_position;   /* Position at start of recursion */
} recursion_info;

/* A similar structure for pcre_dfa_exec(). */

typedef struct dfa_recursion_info {
  struct dfa_recursion_info *prevrec;
  int group_num;
  PCRE_PUCHAR subject_position;
} dfa_recursion_info;

/* Structure for building a chain of data for holding the values of the subject
pointer at the start of each subpattern, so as to detect when an empty string
has been matched by a subpattern - to break infinite loops; used by
pcre_exec(). */

typedef struct eptrblock {
  struct eptrblock *epb_prev;
  PCRE_PUCHAR epb_saved_eptr;
} eptrblock;


/* Structure for passing "static" information around between the functions
doing traditional NFA matching, so that they are thread-safe. */

typedef struct match_data {
  unsigned long int match_call_count;      /* As it says */
  unsigned long int match_limit;           /* As it says */
  unsigned long int match_limit_recursion; /* As it says */
  int   *offset_vector;           /* Offset vector */
  int    offset_end;              /* One past the end */
  int    offset_max;              /* The maximum usable for return data */
  int    nltype;                  /* Newline type */
  int    nllen;                   /* Newline string length */
  int    name_count;              /* Number of names in name table */
  int    name_entry_size;         /* Size of entry in names table */
  unsigned int skip_arg_count;    /* For counting SKIP_ARGs */
  unsigned int ignore_skip_arg;   /* For re-run when SKIP arg name not found */
  pcre_uchar *name_table;         /* Table of names */
  pcre_uchar nl[4];               /* Newline string when fixed */
  const  pcre_uint8 *lcc;         /* Points to lower casing table */
  const  pcre_uint8 *fcc;         /* Points to case-flipping table */
  const  pcre_uint8 *ctypes;      /* Points to table of type maps */
  BOOL   notbol;                  /* NOTBOL flag */
  BOOL   noteol;                  /* NOTEOL flag */
  BOOL   utf;                     /* UTF-8 / UTF-16 flag */
  BOOL   jscript_compat;          /* JAVASCRIPT_COMPAT flag */
  BOOL   use_ucp;                 /* PCRE_UCP flag */
  BOOL   endonly;                 /* Dollar not before final \n */
  BOOL   notempty;                /* Empty string match not wanted */
  BOOL   notempty_atstart;        /* Empty string match at start not wanted */
  BOOL   hitend;                  /* Hit the end of the subject at some point */
  BOOL   bsr_anycrlf;             /* \R is just any CRLF, not full Unicode */
  BOOL   hasthen;                 /* Pattern contains (*THEN) */
  const  pcre_uchar *start_code;  /* For use when recursing */
  PCRE_PUCHAR start_subject;      /* Start of the subject string */
  PCRE_PUCHAR end_subject;        /* End of the subject string */
  PCRE_PUCHAR start_match_ptr;    /* Start of matched string */
  PCRE_PUCHAR end_match_ptr;      /* Subject position at end match */
  PCRE_PUCHAR start_used_ptr;     /* Earliest consulted character */
  int    partial;                 /* PARTIAL options */
  int    end_offset_top;          /* Highwater mark at end of match */
  pcre_int32 capture_last;        /* Most recent capture number + overflow flag */
  int    start_offset;            /* The start offset value */
  int    match_function_type;     /* Set for certain special calls of MATCH() */
  eptrblock *eptrchain;           /* Chain of eptrblocks for tail recursions */
  int    eptrn;                   /* Next free eptrblock */
  recursion_info *recursive;      /* Linked list of recursion data */
  void  *callout_data;            /* To pass back to callouts */
  const  pcre_uchar *mark;        /* Mark pointer to pass back on success */
  const  pcre_uchar *nomatch_mark;/* Mark pointer to pass back on failure */
  const  pcre_uchar *once_target; /* Where to back up to for atomic groups */
#ifdef NO_RECURSE
  void  *match_frames_base;       /* For remembering malloc'd frames */
#endif
} match_data;

/* A similar structure is used for the same purpose by the DFA matching
functions. */

typedef struct dfa_match_data {
  const pcre_uchar *start_code;     /* Start of the compiled pattern */
  const pcre_uchar *start_subject ; /* Start of the subject string */
  const pcre_uchar *end_subject;    /* End of subject string */
  const pcre_uchar *start_used_ptr; /* Earliest consulted character */
  const pcre_uint8 *tables;         /* Character tables */
  int   start_offset;               /* The start offset value */
  int   moptions;                   /* Match options */
  int   poptions;                   /* Pattern options */
  int   nltype;                     /* Newline type */
  int   nllen;                      /* Newline string length */
  pcre_uchar nl[4];                 /* Newline string when fixed */
  void *callout_data;               /* To pass back to callouts */
  dfa_recursion_info *recursive;    /* Linked list of recursion data */
} dfa_match_data;

/* Bit definitions for entries in the pcre_ctypes table. */

#define ctype_space   0x01
#define ctype_letter  0x02
#define ctype_digit   0x04
#define ctype_xdigit  0x08
#define ctype_word    0x10   /* alphanumeric or '_' */
#define ctype_meta    0x80   /* regexp meta char or zero (end pattern) */

/* Offsets for the bitmap tables in pcre_cbits. Each table contains a set
of bits for a class map. Some classes are built by combining these tables. */

#define cbit_space     0      /* [:space:] or \s */
#define cbit_xdigit   32      /* [:xdigit:] */
#define cbit_digit    64      /* [:digit:] or \d */
#define cbit_upper    96      /* [:upper:] */
#define cbit_lower   128      /* [:lower:] */
#define cbit_word    160      /* [:word:] or \w */
#define cbit_graph   192      /* [:graph:] */
#define cbit_print   224      /* [:print:] */
#define cbit_punct   256      /* [:punct:] */
#define cbit_cntrl   288      /* [:cntrl:] */
#define cbit_length  320      /* Length of the cbits table */

/* Offsets of the various tables from the base tables pointer, and
total length. */

#define lcc_offset      0
#define fcc_offset    256
#define cbits_offset  512
#define ctypes_offset (cbits_offset + cbit_length)
#define tables_length (ctypes_offset + 256)

/* Internal function and data prefixes. */

#if defined COMPILE_PCRE8
#ifndef PUBL
#define PUBL(name) pcre_##name
#endif
#ifndef PRIV
#define PRIV(name) _pcre_##name
#endif
#elif defined COMPILE_PCRE16
#ifndef PUBL
#define PUBL(name) pcre16_##name
#endif
#ifndef PRIV
#define PRIV(name) _pcre16_##name
#endif
#elif defined COMPILE_PCRE32
#ifndef PUBL
#define PUBL(name) pcre32_##name
#endif
#ifndef PRIV
#define PRIV(name) _pcre32_##name
#endif
#else
#error Unsupported compiling mode
#endif /* COMPILE_PCRE[8|16|32] */

/* Layout of the UCP type table that translates property names into types and
codes. Each entry used to point directly to a name, but to reduce the number of
relocations in shared libraries, it now has an offset into a single string
instead. */

typedef struct {
  pcre_uint16 name_offset;
  pcre_uint16 type;
  pcre_uint16 value;
} ucp_type_table;


/* Internal shared data tables. These are tables that are used by more than one
of the exported public functions. They have to be "external" in the C sense,
but are not part of the PCRE public API. The data for these tables is in the
pcre_tables.c module. */

#ifdef COMPILE_PCRE8
extern const int            PRIV(utf8_table1)[];
extern const int            PRIV(utf8_table1_size);
extern const int            PRIV(utf8_table2)[];
extern const int            PRIV(utf8_table3)[];
extern const pcre_uint8     PRIV(utf8_table4)[];
#endif /* COMPILE_PCRE8 */

extern const char           PRIV(utt_names)[];
extern const ucp_type_table PRIV(utt)[];
extern const int            PRIV(utt_size);

extern const pcre_uint8     PRIV(OP_lengths)[];
extern const pcre_uint8     PRIV(default_tables)[];

extern const pcre_uint32    PRIV(hspace_list)[];
extern const pcre_uint32    PRIV(vspace_list)[];


/* Internal shared functions. These are functions that are used by more than
one of the exported public functions. They have to be "external" in the C
sense, but are not part of the PCRE public API. */

/* String comparison functions. */
#if defined COMPILE_PCRE8

#define STRCMP_UC_UC(str1, str2) \
  strcmp((char *)(str1), (char *)(str2))
#define STRCMP_UC_C8(str1, str2) \
  strcmp((char *)(str1), (str2))
#define STRNCMP_UC_UC(str1, str2, num) \
  strncmp((char *)(str1), (char *)(str2), (num))
#define STRNCMP_UC_C8(str1, str2, num) \
  strncmp((char *)(str1), (str2), (num))
#define STRLEN_UC(str) strlen((const char *)str)

#elif defined COMPILE_PCRE16 || defined COMPILE_PCRE32

extern int               PRIV(strcmp_uc_uc)(const pcre_uchar *,
                           const pcre_uchar *);
extern int               PRIV(strcmp_uc_c8)(const pcre_uchar *,
                           const char *);
extern int               PRIV(strncmp_uc_uc)(const pcre_uchar *,
                           const pcre_uchar *, unsigned int num);
extern int               PRIV(strncmp_uc_c8)(const pcre_uchar *,
                           const char *, unsigned int num);
extern unsigned int      PRIV(strlen_uc)(const pcre_uchar *str);

#define STRCMP_UC_UC(str1, str2) \
  PRIV(strcmp_uc_uc)((str1), (str2))
#define STRCMP_UC_C8(str1, str2) \
  PRIV(strcmp_uc_c8)((str1), (str2))
#define STRNCMP_UC_UC(str1, str2, num) \
  PRIV(strncmp_uc_uc)((str1), (str2), (num))
#define STRNCMP_UC_C8(str1, str2, num) \
  PRIV(strncmp_uc_c8)((str1), (str2), (num))
#define STRLEN_UC(str) PRIV(strlen_uc)(str)

#endif /* COMPILE_PCRE[8|16|32] */

#if defined COMPILE_PCRE8 || defined COMPILE_PCRE16

#define STRCMP_UC_UC_TEST(str1, str2) STRCMP_UC_UC(str1, str2)
#define STRCMP_UC_C8_TEST(str1, str2) STRCMP_UC_C8(str1, str2)

#elif defined COMPILE_PCRE32

extern int               PRIV(strcmp_uc_uc_utf)(const pcre_uchar *,
                           const pcre_uchar *);
extern int               PRIV(strcmp_uc_c8_utf)(const pcre_uchar *,
                           const char *);

#define STRCMP_UC_UC_TEST(str1, str2) \
  (utf ? PRIV(strcmp_uc_uc_utf)((str1), (str2)) : PRIV(strcmp_uc_uc)((str1), (str2)))
#define STRCMP_UC_C8_TEST(str1, str2) \
  (utf ? PRIV(strcmp_uc_c8_utf)((str1), (str2)) : PRIV(strcmp_uc_c8)((str1), (str2)))

#endif /* COMPILE_PCRE[8|16|32] */

extern const pcre_uchar *PRIV(find_bracket)(const pcre_uchar *, BOOL, int);
extern BOOL              PRIV(is_newline)(PCRE_PUCHAR, int, PCRE_PUCHAR,
                           int *, BOOL);
extern unsigned int      PRIV(ord2utf)(pcre_uint32, pcre_uchar *);
extern int               PRIV(valid_utf)(PCRE_PUCHAR, int, int *);
extern BOOL              PRIV(was_newline)(PCRE_PUCHAR, int, PCRE_PUCHAR,
                           int *, BOOL);
extern BOOL              PRIV(xclass)(pcre_uint32, const pcre_uchar *, BOOL);

#ifdef SUPPORT_JIT
extern void              PRIV(jit_compile)(const REAL_PCRE *,
                           PUBL(extra) *, int);
extern int               PRIV(jit_exec)(const PUBL(extra) *,
                           const pcre_uchar *, int, int, int, int *, int);
extern void              PRIV(jit_free)(void *);
extern int               PRIV(jit_get_size)(void *);
extern const char*       PRIV(jit_get_target)(void);
#endif

/* Unicode character database (UCD) */

typedef struct {
  pcre_uint8 script;     /* ucp_Arabic, etc. */
  pcre_uint8 chartype;   /* ucp_Cc, etc. (general categories) */
  pcre_uint8 gbprop;     /* ucp_gbControl, etc. (grapheme break property) */
  pcre_uint8 caseset;    /* offset to multichar other cases or zero */
  pcre_int32 other_case; /* offset to other case, or zero if none */
} ucd_record;

extern const pcre_uint32 PRIV(ucd_caseless_sets)[];
extern const ucd_record  PRIV(ucd_records)[];
extern const pcre_uint8  PRIV(ucd_stage1)[];
extern const pcre_uint16 PRIV(ucd_stage2)[];
extern const pcre_uint32 PRIV(ucp_gentype)[];
extern const pcre_uint32 PRIV(ucp_gbtable)[];
#ifdef SUPPORT_JIT
extern const int         PRIV(ucp_typerange)[];
#endif

#ifdef SUPPORT_UCP
/* UCD access macros */

#define UCD_BLOCK_SIZE 128
#define GET_UCD(ch) (PRIV(ucd_records) + \
        PRIV(ucd_stage2)[PRIV(ucd_stage1)[(int)(ch) / UCD_BLOCK_SIZE] * \
        UCD_BLOCK_SIZE + (int)(ch) % UCD_BLOCK_SIZE])

#define UCD_CHARTYPE(ch)    GET_UCD(ch)->chartype
#define UCD_SCRIPT(ch)      GET_UCD(ch)->script
#define UCD_CATEGORY(ch)    PRIV(ucp_gentype)[UCD_CHARTYPE(ch)]
#define UCD_GRAPHBREAK(ch)  GET_UCD(ch)->gbprop
#define UCD_CASESET(ch)     GET_UCD(ch)->caseset
#define UCD_OTHERCASE(ch)   ((pcre_uint32)((int)ch + (int)(GET_UCD(ch)->other_case)))

#endif /* SUPPORT_UCP */

#endif

/* End of pcre_internal.h */

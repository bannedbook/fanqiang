/*
Copyright (c) 2003-2006 by Juliusz Chroboczek

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef WIN32
#include <sys/param.h>
#endif

#include <limits.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#ifndef _WIN32
#include <unistd.h>
#include <sys/time.h>
#include <dirent.h>
#else
#include "dirent_compat.h"
#endif
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#ifndef WIN32 /*MINGW*/
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <poll.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <signal.h>
#endif

#ifdef __MINGW32_VERSION
#define MINGW
#endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
/* nothing */
#elif defined(__GNUC__)
#define inline __inline
#if  (__GNUC__ >= 3)
#define restrict __restrict
#else
#define restrict /**/
#endif
#else
#define inline /**/
#define restrict /**/
#endif

#if defined(__GNUC__) && (__GNUC__ >= 3)
#define ATTRIBUTE(x) __attribute__(x)
#else
#define ATTRIBUTE(x) /**/
#endif

#if defined __GLIBC__
#define HAVE_TM_GMTOFF
#ifndef __UCLIBC__
#define HAVE_TIMEGM
#define HAVE_FTS
#define HAVE_FFSL
#define HAVE_FFSLL
#endif
#define HAVE_SETENV
#define HAVE_ASPRINTF
#if (__GLIBC__ > 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 2)
#define HAVE_MEMRCHR
#endif
#endif

#if defined(__linux__) && (__GNU_LIBRARY__ == 1)
/* Linux libc 5 */
#define HAVE_TIMEGM
#define HAVE_SETENV
#endif

#ifdef BSD
#define HAVE_TM_GMTOFF
#define HAVE_FTS
#define HAVE_SETENV
#endif

#ifdef __CYGWIN__
#define HAVE_SETENV
#define HAVE_ASPRINTF
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#define HAVE_TZSET

#if _POSIX_VERSION >= 200112L
#define HAVE_SETENV
#endif

#ifndef NO_IPv6

#ifdef __GLIBC__
#if (__GLIBC__ > 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 2)
#define HAVE_IPv6
#endif
#endif

#ifdef __FreeBSD__
#define HAVE_ASPRINTF
#if __FreeBSD_version >= 400000
#define HAVE_IPv6
#define HAVE_TIMEGM
#endif
#if __FreeBSD_version >= 503001
#define HAVE_FFSL
#endif
#endif

#ifdef __NetBSD__
#if __NetBSD_Version__ >= 105000000
#define HAVE_IPv6
#endif
#if __NetBSD_Version__ >= 200000000
#define HAVE_TIMEGM
#define HAVE_ASPRINTF
#endif
#endif

#ifdef __OpenBSD__
#if OpenBSD >= 200311
#define HAVE_IPv6
#endif
#endif

#ifdef __APPLE__
#define HAVE_ASPRINTF
#define HAVE_IPv6
#define HAVE_TIMEGM
#define HAVE_FFSL
#endif

#endif

#if defined(i386) || defined(__mc68020__) || defined(__x86_64__)
#define UNALIGNED_ACCESS
#endif

#ifndef WIN32 /*MINGW*/
#define HAVE_FORK
#ifndef NO_SYSLOG
#define HAVE_SYSLOG
#endif
#define HAVE_READV_WRITEV
#define HAVE_FFS
#define READ(x, y, z) read(x, y, z)
#define WRITE(x, y, z) write(x, y, z)
#define CLOSE(x) close(x)
#else
#ifndef HAVE_REGEX
#define NO_FORBIDDEN
#endif
#ifndef MINGW
#define HAVE_MKGMTIME
#endif
#endif

#ifdef HAVE_READV_WRITEV
#define WRITEV(x, y, z) writev(x, y, z)
#define READV(x, y, z)  readv(x, y, z)
#endif

#ifndef HAVE_FORK
#define NO_REDIRECTOR
#endif

/* This is not going to work if va_list is interesting.  But then, if you
   have a non-trivial implementation of va_list, you should have va_copy. */
#ifndef va_copy
#define va_copy(a, b) do { a = b; } while(0)
#endif

#include "mingw.h"

#include "ftsimport.h"
#include "atom.h"
#include "util.h"
#include "config.h"
#include "event.h"
#include "io.h"
#include "socks.h"
#include "chunk.h"
#include "object.h"
#include "dns.h"
#include "http.h"
#include "client.h"
#include "local.h"
#include "diskcache.h"
#include "server.h"
#include "http_parse.h"
#include "parse_time.h"
#include "forbidden.h"
#include "log.h"
#include "auth.h"
#include "tunnel.h"

extern AtomPtr configFile;
extern int daemonise;
extern AtomPtr pidFile;

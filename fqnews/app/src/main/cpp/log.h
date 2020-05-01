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

#define L_ERROR 0x1
#define L_WARN 0x2
#define L_INFO 0x4
#define L_FORBIDDEN 0x8
#define L_UNCACHEABLE 0x10
#define L_SUPERSEDED 0x20
#define L_VARY 0x40
#define L_TUNNEL 0x80

#define D_SERVER_CONN 0x100
#define D_SERVER_REQ 0x200
#define D_CLIENT_CONN 0x400
#define D_CLIENT_REQ 0x800
#define D_ATOM_REFCOUNT 0x1000
#define D_REFCOUNT 0x2000
#define D_LOCK 0x4000
#define D_OBJECT 0x8000
#define D_OBJECT_DATA 0x10000
#define D_SERVER_OFFSET 0x20000
#define D_CLIENT_DATA 0x40000
#define D_DNS 0x80000
#define D_CHILD 0x100000
#define D_IO 0x200000

#define LOGGING_DEFAULT (L_ERROR | L_WARN | L_INFO)
#define LOGGING_MAX 0xFF

extern int scrubLogs;

void preinitLog(void);
void initLog(void);
void reopenLog(void);
void flushLog(void);
int loggingToStderr(void);

void really_do_log(int type, const char *f, ...)
    ATTRIBUTE ((format (printf, 2, 3)));
void really_do_log_v(int type, const char *f, va_list args)
    ATTRIBUTE ((format (printf, 2, 0)));
void really_do_log_n(int type, const char *s, int n);
void really_do_log_error(int type, int e, const char *f, ...)
    ATTRIBUTE ((format (printf, 3, 4)));
void really_do_log_error_v(int type, int e, const char *f, va_list args)
    ATTRIBUTE ((format (printf, 3, 0)));
const char *scrub(const char *message);

#ifdef __GNUC__
#define DO_BACKTRACE()                  \
  do {                                  \
    int n;                              \
    void *buffer[10];                   \
    n = backtrace(buffer, 5);           \
    fflush(stderr);                     \
    backtrace_symbols_fd(buffer, n, 2); \
 } while(0)
#else
#define DO_BACKTRACE() /* */
#endif

/* These are macros because it's important that they should be
   optimised away. */

#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L

#define do_log(_type, ...)                                           \
    do {                                                             \
        if((_type) & (LOGGING_MAX)) really_do_log((_type), __VA_ARGS__); \
    } while(0)
#define do_log_error(_type, _e, ...)                                 \
    do {                                                             \
        if((_type) & (LOGGING_MAX))                                  \
            really_do_log_error((_type), (_e), __VA_ARGS__);         \
    } while(0)

#elif defined(__GNUC__)

#define do_log(_type, _args...)                                \
    do {                                                       \
        if((_type) & (LOGGING_MAX)) really_do_log((_type), _args); \
    } while(0)
#define do_log_error(_type, _e, _args...)                      \
    do {                                                       \
        if((_type) & (LOGGING_MAX))                            \
            really_do_log_error((_type), (_e), _args);         \
    } while(0)

#else

/* No variadic macros -- let's hope inline works. */

static inline void 
do_log(int type, const char *f, ...)
{
    va_list args;

    va_start(args, f);
    if((type & (LOGGING_MAX)) != 0)
        really_do_log_v(type, f, args);
    va_end(args);
}

static inline void
do_log_error(int type, int e, const char *f, ...)
{
    va_list args;

    va_start(args, f);
    if((type & (LOGGING_MAX)) != 0)
        really_do_log_error_v(type, e, f, args);
    va_end(args);
}    

#endif

#define do_log_n(_type, _s, _n) \
    do { \
        if((_type) & (LOGGING_MAX)) really_do_log_n((_type), (_s), (_n)); \
    } while(0)

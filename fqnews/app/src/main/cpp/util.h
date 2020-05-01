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

/* These are Polipo's error codes.  They need to be positive integers,
   and must not collide with possible errno values.
   Starting at 2^16 should be safe enough. */

#define E0 (1 << 16)
#define E1 (2 << 16)
#define E2 (3 << 16)
#define E3 (4 << 16)
#define EUNKNOWN (E0)
#define EDOSHUTDOWN (E0 + 1)
#define EDOGRACEFUL (E0 + 2)
#define EDOTIMEOUT (E0 + 3)
#define ECLIENTRESET (E0 + 4)
#define ESYNTAX (E0 + 5)
#define EREDIRECTOR (E0 + 6)
#define EDNS_HOST_NOT_FOUND (E1)
#define EDNS_NO_ADDRESS (E1 + 1)
#define EDNS_NO_RECOVERY (E1 + 2)
#define EDNS_TRY_AGAIN (E1 + 3)
#define EDNS_INVALID (E1 + 4)
#define EDNS_UNSUPPORTED (E1 + 5)
#define EDNS_FORMAT (E1 + 6)
#define EDNS_REFUSED (E1 + 7)
#define EDNS_CNAME_LOOP (E1 + 8)
#define ESOCKS_PROTOCOL (E2)
/* These correspond to SOCKS status codes 91 through 93 */
#define ESOCKS_REJECT_FAIL (E2 + 1)
#define ESOCKS_REJECT_IDENTD (E2 + 2)
#define ESOCKS_REJECT_UID_MISMATCH (E2 + 3)
/* (ESOCKS5_BASE + n) corresponds to SOCKS5 status code n (0 to 8) */
#define ESOCKS5_BASE (E3)

typedef struct _IntRange {
    int from;
    int to;
} IntRangeRec, *IntRangePtr;

typedef struct _IntList {
    int length;
    int size;
    IntRangePtr ranges;
} IntListRec, *IntListPtr;

char *strdup_n(const char *restrict buf, int n) ATTRIBUTE ((malloc));
int snnprintf(char *restrict buf, int n, int len, const char *format, ...)
     ATTRIBUTE ((format (printf, 4, 5)));
int snnvprintf(char *restrict buf, int n, int len, const char *format, va_list args)
     ATTRIBUTE ((format (printf, 4, 0)));
int snnprint_n(char *restrict buf, int n, int len, const char *s, int slen);
int strcmp_n(const char *string, const char *buf, int n) ATTRIBUTE ((pure));
int digit(char) ATTRIBUTE ((const));
int letter(char) ATTRIBUTE ((const));
char lwr(char) ATTRIBUTE ((const));
char* lwrcpy(char *restrict dst, const char *restrict src, int n);
int lwrcmp(const char *as, const char *bs, int n) ATTRIBUTE ((pure));
int strcasecmp_n(const char *string, const char *buf, int n)
     ATTRIBUTE ((pure));
int atoi_n(const char *restrict string, int n, int len, int *value_return);
int isWhitespace(const char *string) ATTRIBUTE((pure));
#ifndef HAVE_MEMRCHR
void *memrchr(const void *s, int c, size_t n) ATTRIBUTE ((pure));
#endif
int h2i(char h) ATTRIBUTE ((const));
char i2h(int i) ATTRIBUTE ((const));
int log2_floor(int x) ATTRIBUTE ((const));
int log2_ceil(int x) ATTRIBUTE ((const));
char* vsprintf_a(const char *f, va_list args)
    ATTRIBUTE ((malloc, format (printf, 1, 0)));
char* sprintf_a(const char *f, ...)
    ATTRIBUTE ((malloc, format (printf, 1, 2)));
unsigned int hash(unsigned seed, const void *restrict key, int key_size, 
                  unsigned int hash_size)
     ATTRIBUTE ((pure));
char *pstrerror(int e);
time_t mktime_gmt(struct tm *tm) ATTRIBUTE ((pure));
AtomPtr expandTilde(AtomPtr filename);
void do_daemonise(int noclose);
void writePid(char *pidfile);
int b64cpy(char *restrict dst, const char *restrict src, int n, int fss);
int b64cmp(const char *restrict a, int an, const char *restrict b, int bn)
    ATTRIBUTE ((pure));
IntListPtr makeIntList(int size) ATTRIBUTE ((malloc));
void destroyIntList(IntListPtr list);
int intListMember(int n, IntListPtr list) ATTRIBUTE ((pure));
int intListCons(int from, int to, IntListPtr list);
int physicalMemory(void);

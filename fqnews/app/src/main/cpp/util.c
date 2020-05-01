/*
Copyright (c) 2003-2007 by Juliusz Chroboczek

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

#include "polipo.h"

/* Note that this is different from GNU's strndup(3). */
char *
strdup_n(const char *restrict buf, int n)
{
    char *s;
    s = malloc(n + 1);
    if(s == NULL)
        return NULL;
    memcpy(s, buf, n);
    s[n] = '\0';
    return s;
}

int
snnprintf(char *restrict buf, int n, int len, const char *format, ...)
{
    va_list args;
    int rc;
    va_start(args, format);
    rc = snnvprintf(buf, n, len, format, args);
    va_end(args);
    return rc;
}

int
snnvprintf(char *restrict buf, int n, int len, const char *format, va_list args)
{
    int rc = -1;
    va_list args_copy;

    if(n < 0) return -2;
    if(n < len) {
        va_copy(args_copy, args);
        rc = vsnprintf(buf + n, len - n, format, args_copy);
        va_end(args_copy);
    }
    if(rc >= 0 && n + rc <= len)
        return n + rc;
    else
        return -1;
}

int
snnprint_n(char *restrict buf, int n, int len, const char *s, int slen)
{
    int i = 0;
    if(n < 0) return -2;
    while(i < slen && n < len)
        buf[n++] = s[i++];
    if(n < len)
        return n;
    else
        return -1;
}

int
strcmp_n(const char *string, const char *buf, int n)
{
    int i;
    i = 0;
    while(string[i] != '\0' && i < n) {
        if(string[i] < buf[i])
            return -1;
        else if(string[i] > buf[i])
            return 1;
        i++;
    }
    if(string[i] == '\0' || i == n)
        return 0;
    else if(i == n)
        return 1;
    else
        return -1;
}

int 
letter(char c)
{
    if(c >= 'A' && c <= 'Z') return 1;
    if(c >= 'a' && c <= 'z') return 1;
    return 0;
}

int
digit(char c)
{
    if(c >= '0' && c <= '9')
        return 1;
    return 0;
}

char
lwr(char a)
{
    if(a >= 'A' && a <= 'Z')
        return a | 0x20;
    else
        return a;
}

char *
lwrcpy(char *restrict dst, const char *restrict src, int n)
{
    int i;
    for(i = 0; i < n; i++)
        dst[i] = lwr(src[i]);
    return dst;
}

int
lwrcmp(const char *as, const char *bs, int n)
{
    int i;
    for(i = 0; i < n; i++) {
        char a = lwr(as[i]), b = lwr(bs[i]);
        if(a < b)
            return -1;
        else if(a > b)
            return 1;
    }
    return 0;
}

int
strcasecmp_n(const char *string, const char *buf, int n)
{
    int i;
    i = 0;
    while(string[i] != '\0' && i < n) {
        char a = lwr(string[i]), b = lwr(buf[i]);
        if(a < b)
            return -1;
        else if(a > b)
            return 1;
        i++;
    }
    if(string[i] == '\0' && i == n)
        return 0;
    else if(i == n)
        return 1;
    else
        return -1;
}

int
atoi_n(const char *restrict string, int n, int len, int *value_return)
{
    int i = n;
    int val = 0;

    if(i >= len || !digit(string[i]))
        return -1;

    while(i < len && digit(string[i])) {
        val = val * 10 + (string[i] - '0');
        i++;
    }
    *value_return = val;
    return i;
}

int 
isWhitespace(const char *string)
{
    while(*string != '\0') {
        if(*string == ' ' || *string == '\t')
            string++;
        else
            return 0;
    }
    return 1;
}

#ifndef HAVE_MEMRCHR
void *
memrchr(const void *s, int c, size_t n)
{
    const unsigned char *ss = s;
    unsigned char cc = c;
    size_t i;
    for(i = 1; i <= n; i++)
        if(ss[n - i] == cc)
            return (void*)(ss + n - i);
    return NULL;
}
#endif

int
h2i(char h) 
{
    if(h >= '0' && h <= '9')
        return h - '0';
    else if(h >= 'a' && h <= 'f')
        return h - 'a' + 10;
    else if(h >= 'A' && h <= 'F')
        return h - 'A' + 10;
    else
        return -1;
}
    
char
i2h(int i)
{
    if(i < 0 || i >= 16)
        return '?';
    if(i < 10)
        return i + '0';
    else
        return i - 10 + 'A';
}

/* floor(log2(x)) */
int
log2_floor(int x) 
{
    int i, j;

    assert(x > 0);

    i = 0;
    j = 1;
    while(2 * j <= x) {
        i++;
        j *= 2;
    }
    return i;
}

/* ceil(log2(x)) */
int
log2_ceil(int x) 
{
    int i, j;

    assert(x > 0);

    i = 0;
    j = 1;
    while(j < x) {
        i++;
        j *= 2;
    }
    return i;
}

#ifdef HAVE_ASPRINTF
char *
vsprintf_a(const char *f, va_list args)
{
    char *r;
    int rc;
    va_list args_copy;

    va_copy(args_copy, args);
    rc = vasprintf(&r, f, args_copy);
    va_end(args_copy);
    if(rc < 0)
        return NULL;
    return r;
    
}

#else

char*
vsprintf_a(const char *f, va_list args)
{
    int n, size;
    char buf[64];
    char *string;
    va_list args_copy;

    va_copy(args_copy, args);
    n = vsnprintf(buf, 64, f, args_copy);
    va_end(args_copy);
    if(n >= 0 && n < 64) {
        return strdup_n(buf, n);
    }
    if(n >= 64)
        size = n + 1;
    else
        size = 96;

    while(1) {
        string = malloc(size);
        if(!string)
            return NULL;
        va_copy(args_copy, args);
        n = vsnprintf(string, size, f, args_copy);
        va_end(args_copy);
        if(n >= 0 && n < size) {
            return string;
        } else if(n >= size)
            size = n + 1;
        else
            size = size * 3 / 2;
        free(string);
        if(size > 16 * 1024)
            return NULL;
    }
    /* NOTREACHED */
}
#endif

char*
sprintf_a(const char *f, ...)
{
    char *s;
    va_list args;
    va_start(args, f);
    s = vsprintf_a(f, args);
    va_end(args);
    return s;
}    

unsigned int
hash(unsigned int seed, const void *restrict key, int key_size,
     unsigned int hash_size)
{
    int i;
    unsigned int h;

    h = seed;
    for(i = 0; i < key_size; i++)
        h = (h << 5) + (h >> (hash_size - 5)) +
            ((unsigned char*)key)[i];
    return h & ((1 << hash_size) - 1);
}

char *
pstrerror(int e)
{
    char *s;
    static char buf[20];

    switch(e) {
    case EDOSHUTDOWN: s = "Immediate shutdown requested"; break;
    case EDOGRACEFUL: s = "Graceful shutdown requested"; break;
    case EDOTIMEOUT: s = "Timeout"; break;
    case ECLIENTRESET: s = "Connection reset by client"; break;
    case ESYNTAX: s = "Incorrect syntax"; break;
    case EREDIRECTOR: s = "Redirector error"; break;
    case EDNS_HOST_NOT_FOUND: s = "Host not found"; break;
    case EDNS_NO_ADDRESS: s = "No address"; break;
    case EDNS_NO_RECOVERY: s = "Permanent name server failure"; break;
    case EDNS_TRY_AGAIN: s = "Temporary name server failure"; break;
    case EDNS_INVALID: s = "Invalid reply from name server"; break;
    case EDNS_UNSUPPORTED: s = "Unsupported DNS reply"; break;
    case EDNS_FORMAT: s = "Invalid DNS query"; break;
    case EDNS_REFUSED: s = "DNS query refused by server"; break;
    case EDNS_CNAME_LOOP: s = "DNS CNAME loop"; break;
#ifndef NO_SOCKS
    case ESOCKS_PROTOCOL: s = "SOCKS protocol error"; break;
    case ESOCKS_REJECT_FAIL: s = "SOCKS request rejected or failed"; break;
    case ESOCKS_REJECT_IDENTD: s = "SOCKS request rejected: "
                                   "server couldn't connect to identd";
        break;
    case ESOCKS_REJECT_UID_MISMATCH: s = "SOCKS request rejected: "
                                         "uid mismatch";
        break;
    case ESOCKS5_BASE: s = "SOCKS success"; break;
    case ESOCKS5_BASE + 1: s = "General SOCKS server failure"; break;
    case ESOCKS5_BASE + 2: s = "SOCKS connection not allowed"; break;
    case ESOCKS5_BASE + 3: s = "SOCKS error: network unreachable"; break;
    case ESOCKS5_BASE + 4: s = "SOCKS error: host unreachable"; break;
    case ESOCKS5_BASE + 5: s = "SOCKS error: connection refused"; break;
    case ESOCKS5_BASE + 6: s = "SOCKS error: TTL expired"; break;
    case ESOCKS5_BASE + 7: s = "SOCKS command not supported"; break;
    case ESOCKS5_BASE + 8: s = "SOCKS error: address type not supported";
        break;
#endif
    case EUNKNOWN: s = "Unknown error"; break;
    default: s = NULL; break;
    }
    if(!s) s = strerror(e);
#ifdef WIN32 /*MINGW*/
    if(!s) {
        if(e >= WSABASEERR && e <= WSABASEERR + 2000) {
            /* This should be okay, as long as the caller discards the
               pointer before another error occurs. */
            snprintf(buf, 20, "Winsock error %d", e);
            s = buf;
        }
    }
#endif
    if(!s) {
        snprintf(buf, 20, "Unknown error %d", e);
        s = buf;
    }
    return s;
}

/* Like mktime(3), but UTC rather than local time */
#if defined(HAVE_TIMEGM)
time_t
mktime_gmt(struct tm *tm)
{
    return timegm(tm);
}
#elif defined(HAVE_MKGMTIME)
time_t
mktime_gmt(struct tm *tm)
{
    return _mkgmtime(tm);
}
#elif defined(HAVE_TM_GMTOFF)
time_t
mktime_gmt(struct tm *tm)
{
    time_t t;
    struct tm *ltm;

    t = mktime(tm);
    if(t < 0)
        return -1;
    ltm = localtime(&t);
    if(ltm == NULL)
        return -1;
    return t + ltm->tm_gmtoff;
}
#elif defined(HAVE_TZSET)
#ifdef HAVE_SETENV
/* Taken from the Linux timegm(3) man page. */
time_t
mktime_gmt(struct tm *tm)
{
    time_t t;
    char *tz;

    tz = getenv("TZ");
    setenv("TZ", "GMT", 1);
    tzset();
    t = mktime(tm);
    if(tz)
        setenv("TZ", tz, 1);
    else
        unsetenv("TZ");
    tzset();
    return t;
}
#else
time_t
mktime_gmt(struct tm *tm)
{
    time_t t;
    char *tz;
    static char *old_tz = NULL;

    tz = getenv("TZ");
    putenv("TZ=GMT");
    tzset();
    t = mktime(tm);
    if(old_tz)
        free(old_tz);
    if(tz)
        old_tz = sprintf_a("TZ=%s", tz);
    else
        old_tz = strdup("TZ");  /* XXX - non-portable? */
    if(old_tz)
        putenv(old_tz);
    tzset();
    return t;
}
#endif
#else
#error no mktime_gmt implementation on this platform
#endif


AtomPtr
expandTilde(AtomPtr filename)
{
    char *buf;
    char *home;
    int len;
    AtomPtr ret;

    if(filename == NULL || filename->length < 1 ||
       filename->string[0] != '~' || filename->string[1] != '/')
        return filename;
    
    home = getenv("HOME");
    if(home == NULL) {
        return NULL;
    }
    len = strlen(home);
    buf = malloc(len + 1 + 1 + filename->length - 2);
    if(buf == NULL) {
        do_log(L_ERROR, "Could not allocate buffer.\n");
        return NULL;
    }

    memcpy(buf, home, len);
    if(buf[len - 1] != '/')
        buf[len++] = '/';
    memcpy(buf + len, filename->string + 2, filename->length - 2);
    len += filename->length - 2;
    ret = internAtomN(buf, len);
    free(buf);
    if(ret != NULL)
        releaseAtom(filename);
    return ret;
}

#ifdef HAVE_FORK
void
do_daemonise(int noclose)
{
    int rc;

    fflush(stdout);
    fflush(stderr);

    rc = fork();
    if(rc < 0) {
        do_log_error(L_ERROR, errno, "Couldn't fork");
        exit(1);
    }

    if(rc > 0)
        exit(0);

    if(!noclose) {
        int fd;
        close(0);
        close(1);
        close(2);
        /* Leaving the default file descriptors free is not a good
           idea, as it will cause library functions such as abort to
           thrash the on-disk cache. */
        fd = open("/dev/null", O_RDONLY);
        if(fd > 0) {
            dup2(fd, 0);
            close(fd);
        }
        fd = open("/dev/null", O_WRONLY);
        if(fd >= 0) {
            if(fd != 1)
                dup2(fd, 1);
            if(fd != 2)
                dup2(fd, 2);
            if(fd != 1 && fd != 2)
                close(fd);
        }
    }
    rc = setsid();
    if(rc < 0) {
        do_log_error(L_ERROR, errno, "Couldn't create new session");
        exit(1);
    }
}

#elif defined(WIN32)

void
do_daemonise(int noclose)
{
	do_log(L_INFO, "Detaching console");
	FreeConsole();
}

#else

void
do_daemonise(int noclose)
{
    do_log(L_ERROR, "Cannot daemonise on this platform");
    exit(1);
}
#endif


void
writePid(char *pidfile)
{
    int fd, n, rc;
    char buf[16];

    fd = open(pidfile, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if(fd < 0) {
        do_log_error(L_ERROR, errno, 
                     "Couldn't create pid file %s", pidfile);
        exit(1);
    }
    n = snprintf(buf, 16, "%ld\n", (long)getpid());
    if(n < 0 || n >= 16) {
        close(fd);
        unlink(pidfile);
        do_log(L_ERROR, "Couldn't format pid.\n");
        exit(1);
    }
    rc = write(fd, buf, n);
    if(rc != n) {
        close(fd);
        unlink(pidfile);
        do_log_error(L_ERROR, errno, "Couldn't write pid");
        exit(1);
    }

    close(fd);
    return;
}

static const char b64[64] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* "/" replaced with "-" */
static const char b64fss[64] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";

int
b64cpy(char *restrict dst, const char *restrict src, int n, int fss)
{
    const char *b = fss ? b64fss: b64;
    int i, j;

    j = 0;
    for(i = 0; i < n; i += 3) {
        unsigned char a0, a1, a2;
        a0 = src[i];
        a1 = i < n - 1 ? src[i + 1] : 0;
        a2 = i < n - 2 ? src[i + 2] : 0;
        dst[j++] = b[(a0 >> 2) & 0x3F];
        dst[j++] = b[((a0 << 4) & 0x30) | ((a1 >> 4) & 0x0F)];
        if(i < n - 1)
            dst[j++] = b[((a1 << 2) & 0x3C) | ((a2 >> 6) & 0x03)];
        else
            dst[j++] = '=';
        if(i < n - 2)
            dst[j++] = b[a2 & 0x3F];
        else
            dst[j++] = '=';
    }
    return j;
}

int
b64cmp(const char *restrict a, int an, const char *restrict b, int bn)
{
    char *buf;
    int r;

    if(an % 4 != 0)
        return -1;
    if((bn + 2) / 3 != an / 4)
        return -1;
    buf = malloc(an);
    if(buf == NULL)
        return -1;
    b64cpy(buf, b, bn, 0);
    r = memcmp(buf, a, an);
    free(buf);
    return r;
}

IntListPtr
makeIntList(int size)
{
    IntListPtr list;
    if(size <= 0)
        size = 4;

    list = malloc(sizeof(IntListRec));
    if(list == NULL)
        return NULL;

    list->ranges = malloc(size * sizeof(IntRangeRec));
    if(list->ranges == NULL) {
        free(list);
        return NULL;
    }

    list->length = 0;
    list->size = size;
    return list;
}

void
destroyIntList(IntListPtr list)
{
    free(list->ranges);
    free(list);
}

int
intListMember(int n, IntListPtr list)
{
    int lo = 0, hi = list->length - 1;
    int mid;
    while(hi >= lo) {
        mid = (hi + lo) / 2;
        if(list->ranges[mid].from > n)
            hi = mid - 1;
        else if(list->ranges[mid].to < n)
            lo = mid + 1;
        else
            return 1;
    }
    return 0;
}

static int
deleteRange(IntListPtr list, int i)
{
    assert(list->length > i);
    if(list->length > i + 1)
        memmove(list->ranges + i, list->ranges + i + 1,
                (list->length - i - 1) * sizeof(IntRangeRec));
    list->length--;
    return 1;
}

static int
insertRange(int from, int to, IntListPtr list, int i)
{
    assert(i >= 0 && i <= list->length);
    assert(i == 0 || list->ranges[i - 1].to < from - 1);
    assert(i == list->length || list->ranges[i].from > to + 1);

    if(list->length >= list->size) {
        int newsize = list->size * 2 + 1;
        IntRangePtr newranges = 
            realloc(list->ranges, newsize * sizeof(IntRangeRec));
        if(newranges == NULL)
            return -1;
        list->size = newsize;
        list->ranges = newranges;
    }

    if(i < list->length)
        memmove(list->ranges + i + 1, list->ranges + i,
                list->length - i);
    list->length++;
    list->ranges[i].from = from;
    list->ranges[i].to = to;
    return 1;
}

static int
maybeMergeRanges(IntListPtr list, int i)
{
    int rc;

    while(i > 0 && list->ranges[i].from <= list->ranges[i - 1].to + 1) {
            list->ranges[i - 1].from = 
                MIN(list->ranges[i - 1].from, list->ranges[i].from);
            list->ranges[i - 1].to =
                MAX(list->ranges[i - 1].to, list->ranges[i].to);
            rc = deleteRange(list, i);
            if(rc < 0) return -1;
            i--;
    }

    while(i < list->length - 1 && 
          list->ranges[i].to >= list->ranges[i + 1].from - 1) {
            list->ranges[i + 1].from = 
                MIN(list->ranges[i + 1].from, list->ranges[i].from);
            list->ranges[i - 1].to =
                MAX(list->ranges[i + 1].to, list->ranges[i].to);
            rc = deleteRange(list, i);
            if(rc < 0) return -1;
    }
    return 1;
}

int
intListCons(int from, int to, IntListPtr list)
{
    int i;

    /* Don't bother with the dichotomy. */
    for(i = 0; i < list->length; i++) {
        if(list->ranges[i].to >= from - 1)
            break;
    }

    if(i < list->length && 
       (from >= list->ranges[i].from - 1 || to <= list->ranges[i].to + 1)) {
        if(from <= list->ranges[i].from)
            list->ranges[i].from = from;
        if(to >= list->ranges[i].to)
            list->ranges[i].to = to;
        return maybeMergeRanges(list, i);
    }
    return insertRange(from, to, list, i);
}

/* Return the amount of physical memory on the box, -1 if unknown or
   over two gigs. */
#if defined(__linux__)

#include <sys/sysinfo.h>
int
physicalMemory()
{
    int rc;
    struct sysinfo info;

    rc = sysinfo(&info);
    if(rc < 0)
        return -1;

    if(info.totalram <= 0x7fffffff / info.mem_unit)
        return (int)(info.totalram * info.mem_unit);

    return -1;
}

#elif defined(__FreeBSD__)

#include <sys/sysctl.h>
int
physicalMemory()
{
    unsigned long membytes;
    size_t len;
    int res;

    len = sizeof(membytes);
    res = sysctlbyname("hw.physmem", &membytes, &len, NULL, 0);
    if (res || membytes > INT_MAX)
        return -1;

    return (int)membytes;
}

#else

int
physicalMemory()
{
    return -1;
}
#endif

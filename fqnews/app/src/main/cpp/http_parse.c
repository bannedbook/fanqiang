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

#include "polipo.h"

static int getNextWord(const char *buf, int i, int *x_return, int *y_return);
static int getNextToken(const char *buf, int i, int *x_return, int *y_return);
static int getNextTokenInList(const char *buf, int i, 
                              int *x_return, int *y_return,
                              int *z_return, int *t_return,
                              int *end_return);

static AtomPtr atomConnection, atomProxyConnection, atomContentLength,
    atomHost, atomAcceptRange, atomTE,
    atomReferer, atomProxyAuthenticate, atomProxyAuthorization,
    atomKeepAlive, atomTrailer, atomUpgrade, atomDate, atomExpires,
    atomIfModifiedSince, atomIfUnmodifiedSince, atomIfRange, atomLastModified,
    atomIfMatch, atomIfNoneMatch, atomAge, atomTransferEncoding, 
    atomETag, atomCacheControl, atomPragma, atomContentRange, atomRange,
    atomVia, atomVary, atomExpect, atomAuthorization,
    atomSetCookie, atomCookie, atomCookie2,
    atomXPolipoDate, atomXPolipoAccess, atomXPolipoLocation, 
    atomXPolipoBodyOffset;

AtomPtr atomContentType, atomContentEncoding;

int censorReferer = 0;
int laxHttpParser = 1;

static AtomListPtr censoredHeaders;

void
preinitHttpParser()
{
    CONFIG_VARIABLE_SETTABLE(censorReferer, CONFIG_TRISTATE, configIntSetter,
                             "Censor referer headers.");
    censoredHeaders = makeAtomList(NULL, 0);
    if(censoredHeaders == NULL) {
        do_log(L_ERROR, "Couldn't allocate censored atoms.\n");
        exit(1);
    }
    CONFIG_VARIABLE(censoredHeaders, CONFIG_ATOM_LIST_LOWER,
                    "Headers to censor.");
    CONFIG_VARIABLE_SETTABLE(laxHttpParser, CONFIG_BOOLEAN, configIntSetter,
                             "Ignore unknown HTTP headers.");
}

void
initHttpParser()
{
#define A(name, value) name = internAtom(value); if(!name) goto fail;
    /* These must be in lower-case */
    A(atomConnection, "connection");
    A(atomProxyConnection, "proxy-connection");
    A(atomContentLength, "content-length");
    A(atomHost, "host");
    A(atomAcceptRange, "accept-range");
    A(atomTE, "te");
    A(atomReferer, "referer");
    A(atomProxyAuthenticate, "proxy-authenticate");
    A(atomProxyAuthorization, "proxy-authorization");
    A(atomKeepAlive, "keep-alive");
    A(atomTrailer, "trailer");
    A(atomUpgrade, "upgrade");
    A(atomDate, "date");
    A(atomExpires, "expires");
    A(atomIfModifiedSince, "if-modified-since");
    A(atomIfUnmodifiedSince, "if-unmodified-since");
    A(atomIfRange, "if-range");
    A(atomLastModified, "last-modified");
    A(atomIfMatch, "if-match");
    A(atomIfNoneMatch, "if-none-match");
    A(atomAge, "age");
    A(atomTransferEncoding, "transfer-encoding");
    A(atomETag, "etag");
    A(atomCacheControl, "cache-control");
    A(atomPragma, "pragma");
    A(atomContentRange, "content-range");
    A(atomRange, "range");
    A(atomVia, "via");
    A(atomContentType, "content-type");
    A(atomContentEncoding, "content-encoding");
    A(atomVary, "vary");
    A(atomExpect, "expect");
    A(atomAuthorization, "authorization");
    A(atomSetCookie, "set-cookie");
    A(atomCookie, "cookie");
    A(atomCookie2, "cookie2");
    A(atomXPolipoDate, "x-polipo-date");
    A(atomXPolipoAccess, "x-polipo-access");
    A(atomXPolipoLocation, "x-polipo-location");
    A(atomXPolipoBodyOffset, "x-polipo-body-offset");
#undef A
    return;

 fail:
    do_log(L_ERROR, "Couldn't allocate atom.\n");
    exit(1);
}

static int
getNextWord(const char *restrict buf, int i, int *x_return, int *y_return)
{
    int x, y;
    while(buf[i] == ' ') i++;
    if(buf[i] == '\n' || buf[i] == '\r') return -1;
    x = i;
    while(buf[i] > 32 && buf[i] < 127) i++;
    y = i;

    *x_return = x;
    *y_return = y;

    return 0;
}

static int
skipComment(const char *restrict buf, int i)
{
    assert(buf[i] == '(');

    i++;
    while(1) {
        if(buf[i] == '\\' && buf[i + 1] == ')') i+=2;
        else if(buf[i] == ')') return i + 1;
        else if(buf[i] == '\n') {
            if(buf[i + 1] == ' ' || buf[i + 1] == '\t')
                i += 2;
            else
                return -1;
        } else if(buf[i] == '\r') {
            if(buf[i + 1] != '\n') return -1;
            if(buf[i + 2] == ' ' || buf[i + 2] == '\t')
                i += 3;
            else
                return -1;
        } else {
            i++;
        }
    }
    return i;
}
            

static int
skipWhitespace(const char *restrict buf, int i)
{
    while(1) {
        if(buf[i] == ' ' || buf[i] == '\t')
            i++;
        else  if(buf[i] == '(') {
            i = skipComment(buf, i);
            if(i < 0) return -1;
        } else if(buf[i] == '\n') {
            if(buf[i + 1] == ' ' || buf[i + 1] == '\t')
                i += 2;
            else
                return i;
        } else if(buf[i] == '\r' && buf[i + 1] == '\n') {
            if(buf[i + 2] == ' ' || buf[i + 2] == '\t')
                i += 3;
            else
                return i;
        } else
            return i;
    }
}

static int
getNextToken(const char *restrict buf, int i, int *x_return, int *y_return)
{
    int x, y;
 again:
    while(buf[i] == ' ' || buf[i] == '\t')
        i++;
    if(buf[i] == '(') {
        i++;
        while(buf[i] != ')') {
            if(buf[i] == '\n' || buf[i] == '\r')
                return -1;
            if(buf[i] == '\\' && buf[i + 1] != '\n' && buf[i + 1] != '\r')
                buf += 2;
            else
                buf++;
        }
        goto again;
    }
    if(buf[i] == '\n') {
        if(buf[i + 1] == ' ' || buf[i + 1] == '\t') {
            i += 2;
            goto again;
        } else {
            return -1;
        }
    }
    if(buf[i] == '\r') {
        if(buf[i + 1] == '\n' && (buf[i + 2] == ' ' || buf[i + 2] == '\t')) {
            i += 3;
            goto again;
        } else {
            return -1;
        }
    }
    x = i;
    while(buf[i] > 32 && buf[i] < 127) {
        switch(buf[i]) {
        case '(': case ')': case '<': case '>': case '@':
        case ',': case ';': case ':': case '\\': case '/':
        case '[': case ']': case '?': case '=':
        case '{': case '}': case ' ': case '\t':
            goto out;
        default:
            i++;
        }
    }
 out:
    y = i;

    *x_return = x;
    *y_return = y;

    return y;
}

static int
getNextETag(const char * restrict buf, int i, 
            int *x_return, int *y_return, int *weak_return)
{
    int weak = 0;
    int x, y;
    while(buf[i] == ' ' || buf[i] == '\t')
        i++;
    if(buf[i] == 'W' && buf[i + 1] == '/') {
        weak = 1;
        i += 2;
    }
    if(buf[i] == '"')
        i++;
    else
        return -1;

    x = i;
    while(buf[i] != '"') {
        if(buf[i] == '\r' || buf[i] == '\n')
            return -1;
        i++;
    }
    y = i;
    i++;

    *x_return = x;
    *y_return = y;
    *weak_return = weak;
    return i;
}

static int
getNextTokenInList(const char *restrict buf, int i, 
                   int *x_return, int *y_return,
                   int *z_return, int *t_return,
                   int *end_return)
{
    int j, x, y, z = -1, t = -1, end;
    j = getNextToken(buf, i, &x, &y);
    if(j < 0)
        return -1;
    while(buf[j] == ' ' || buf[j] == '\t')
        j++;

    if(buf[j] == '=') {
        j++;
        while(buf[j] == ' ' || buf[j] == '\t')
            j++;
        z = j;
        while(buf[j] != ',' && buf[j] != '\n' && buf[j] != '\r')
            j++;
    }

    if(buf[j] == '\n' || buf[j] == '\r') {
        if(buf[j] == '\r') {
            if(buf[j + 1] != '\n')
                return -1;
            j += 2;
        } else
            j++;
        end = 1;
        if(buf[j] == ' ' || buf[j] == '\t') {
            while(buf[j] == ' ' || buf[j] == '\t')
                j++;
            end = 0;
        }
    } else if(buf[j] == ',') {
        j++;
        while(buf[j] == ' ' || buf[j] == '\t')
            j++;
        end = 0;
    } else {
        return -1;
    }

    *x_return = x;
    *y_return = y;
    if(z_return)
        *z_return = z;
    if(t_return)
        *t_return = t;
    *end_return = end;
    return j;
}

static inline int
token_compare(const char *buf, int start, int end, const char *s)
{
    return (strcasecmp_n(s, buf + start, end - start) == 0);
}

static int
skipEol(const char *restrict buf, int i)
{
    while(buf[i] == ' ')
        i++;
    if(buf[i] == '\n')
        return i + 1;
    else if(buf[i] == '\r') {
        if(buf[i + 1] == '\n')
            return i + 2;
        else
            return -1;
    } else {
        return -1;
    }
}
    
static int
skipToEol(const char *restrict buf, int i, int *start_return)
{
    while(buf[i] != '\n' && buf[i] != '\r')
        i++;
    if(buf[i] == '\n') {
        *start_return = i;
        return i + 1;
    } else if(buf[i] == '\r') {
        if(buf[i + 1] == '\n') {
            *start_return = i;
            return i + 2;
        } else {
            return -1;
        }
    }
    return -1;
}

static int
getHeaderValue(const char *restrict buf, int start, 
               int *value_start_return, int *value_end_return)
{
    int i, j, k;

    while(buf[start] == ' ' || buf[start] == '\t')
        start++;
    i = start;
 again:
    j = skipToEol(buf, i, &k);
    if(j < 0)
        return -1;
    if(buf[j] == ' ' || buf[j] == '\t') {
        i = j + 1;
        goto again;
    }
    *value_start_return = start;
    *value_end_return = k;
    return j;
}
    
int
httpParseClientFirstLine(const char *restrict buf, int offset,
                         int *method_return,
                         AtomPtr *url_return,
                         int *version_return)
{
    int i = 0;
    int x, y;
    int method;
    AtomPtr url;
    int version = HTTP_UNKNOWN;
    int eol;

    i = offset;
    i = getNextWord(buf, i, &x, &y);
    if(i < 0) return -1;
    if(y == x + 3 && memcmp(buf + x, "GET", 3) == 0)
        method = METHOD_GET;
    else if(y == x + 4 && memcmp(buf + x, "HEAD", 4) == 0)
        method = METHOD_HEAD;
    else if(y == x + 4 && memcmp(buf + x, "POST", 4) == 0)
        method = METHOD_POST;
    else if(y == x + 3 && memcmp(buf + x, "PUT", 3) == 0)
        method = METHOD_PUT;
    else if(y == x + 7 && memcmp(buf + x, "CONNECT", 7) == 0)
        method = METHOD_CONNECT;
    else
        method = METHOD_UNKNOWN;

    i = getNextWord(buf, y + 1, &x, &y);
    if(i < 0) return -1;

    url = internAtomN(buf + x, y - x);

    i = getNextWord(buf, y + 1, &x, &y);
    if(i < 0) {
        releaseAtom(url);
        return -1;
    }

    if(y == x + 8) {
        if(memcmp(buf + x, "HTTP/1.", 7) != 0)
            version = HTTP_UNKNOWN;
        else if(buf[x + 7] == '0')
            version = HTTP_10;
        else if(buf[x + 7] >= '1' && buf[x + 7] <= '9')
            version = HTTP_11;
        else
            version = HTTP_UNKNOWN;
    }

    eol = skipEol(buf, y);
    if(eol < 0) return -1;
        
    *method_return = method;
    if(url_return)
        *url_return = url;
    else
        releaseAtom(url);
    *version_return = version;
    return eol;
}

int
httpParseServerFirstLine(const char *restrict buf, 
                         int *status_return,
                         int *version_return,
                         AtomPtr *message_return)
{
    int i = 0;
    int x, y, eol;
    int status;
    int version = HTTP_UNKNOWN;
    
    i = getNextWord(buf, 0, &x, &y);
    if(i < 0)
        return -1;
    if(y == x + 8 && memcmp(buf + x, "HTTP/1.0", 8) == 0)
        version = HTTP_10;
    else if(y >= x + 8 && memcmp(buf + x, "HTTP/1.", 7) == 0)
        version = HTTP_11;
    else
        version = HTTP_UNKNOWN;

    i = getNextWord(buf, y + 1, &x, &y);
    if(i < 0) return -1;
    if(y == x + 3)
        status = atol(buf + x);
    else return -1;

    i = skipToEol(buf, y, &eol);
    if(i < 0) return -1;
        
    *status_return = status;
    *version_return = version;
    if(message_return) {
        /* Netscape enterprise bug */
        if(eol > y)
            *message_return = internAtomN(buf + y + 1, eol - y - 1);
        else
            *message_return = internAtom("No message");
    }
    return i;
}

static int
parseInt(const char *restrict buf, int start, int *val_return)
{
    int i = start, val = 0;
    if(!digit(buf[i]))
        return -1;
    while(digit(buf[i])) {
        val = val * 10 + (buf[i] - '0');
        i++;
    }
    *val_return = val;
    return i;
}

/* Returned *name_start_return is -1 at end of headers, -2 if the line
   couldn't be parsed. */
static int
parseHeaderLine(const char *restrict buf, int start,
                int *name_start_return, int *name_end_return,
                int *value_start_return, int *value_end_return)
{
    int i;
    int name_start, name_end, value_start, value_end;

    if(buf[start] == '\n') {
        *name_start_return = -1;
        return start + 1;
    }
    if(buf[start] == '\r' && buf[start + 1] == '\n') {
        *name_start_return = -1;
        return start + 2;
    }

    i = getNextToken(buf, start, &name_start, &name_end);
    if(i < 0 || buf[i] != ':')
        goto syntax;
    i++;
    while(buf[i] == ' ' || buf[i] == '\t')
        i++;

    i = getHeaderValue(buf, i, &value_start, &value_end);
    if(i < 0)
        goto syntax;

    *name_start_return = name_start;
    *name_end_return = name_end;
    *value_start_return = value_start;
    *value_end_return = value_end;
    return i;

 syntax:
    i = start;
    while(1) {
        if(buf[i] == '\n') {
            i++;
            break;
        }
        if(buf[i] == '\r' && buf[i + 1] == '\n') {
            i += 2;
            break;
        }
        i++;
    }
    *name_start_return = -2;
    return i;
}

int
findEndOfHeaders(const char *restrict buf, int from, int to, int *body_return) 
{
    int i = from;
    int eol = 0;
    while(i < to) {
        if(buf[i] == '\n') {
            if(eol) {
                *body_return = i + 1;
                return eol;
            }
            eol = i;
            i++;
        } else if(buf[i] == '\r') {
            if(i < to - 1 && buf[i + 1] == '\n') {
                if(eol) {
                    *body_return = eol;
                    return i + 2;
                }
                eol = i;
                i += 2;
            } else {
                eol = 0;
                i++;
            }
        } else {
            eol = 0;
            i++;
        }
    }
    return -1;
}

static int
parseContentRange(const char *restrict buf, int i, 
                  int *from_return, int *to_return, int *full_len_return)
{
    int j;
    int from, to, full_len;

    i = skipWhitespace(buf, i);
    if(i < 0) return -1;
    if(!token_compare(buf, i, i + 5, "bytes")) {
        do_log(L_WARN, "Incorrect Content-Range header -- chugging along.\n");
    } else {
        i += 5;
    }
    i = skipWhitespace(buf, i);
    if(buf[i] == '*') {
        from = 0;
        to = -1;
        i++;
    } else {
        i = parseInt(buf, i, &from);
        if(i < 0) return -1;
        if(buf[i] != '-') return -1;
        i++;
        i = parseInt(buf, i, &to);
        if(i < 0) return -1;
        to = to + 1;
    }
    if(buf[i] != '/')
        return -1;
    i++;
    if(buf[i] == '*')
        full_len = -1;
    else {
        i = parseInt(buf, i, &full_len);
        if(i < 0) return -1;
    }
    j = skipEol(buf, i);
    if(j < 0)
        return -1;

    *from_return = from;
    *to_return = to;
    *full_len_return = full_len;
    return i;
}

static int
parseRange(const char *restrict buf, int i, 
           int *from_return, int *to_return)
{
    int j;
    int from, to;

    i = skipWhitespace(buf, i);
    if(i < 0)
        return -1;
    if(!token_compare(buf, i, i + 6, "bytes="))
        return -1;
    i += 6;
    i = skipWhitespace(buf, i);
    if(buf[i] == '-') {
        from = 0;
    } else {
        i = parseInt(buf, i, &from);
        if(i < 0) return -1;
    }
    if(buf[i] != '-')
        return -1;
    i++;
    j = parseInt(buf, i, &to);
    if(j < 0) 
        to = -1;
    else {
        to = to + 1;
        i = j;
    }
    j = skipEol(buf, i);
    if(j < 0) return -1;
    *from_return = from;
    *to_return = to;
    return i;
}

static void
parseCacheControl(const char *restrict buf, 
                  int token_start, int token_end,
                  int v_start, int v_end, int *age_return)
{
    if(v_start <= 0 || !digit(buf[v_start])) {
        do_log(L_WARN, "Couldn't parse Cache-Control: ");
        do_log_n(L_WARN, buf + token_start,
                 (v_end >= 0 ? v_end : token_end) -
                 token_start);
        do_log(L_WARN, "\n");
    } else
        *age_return = atoi(buf + v_start);
}

static int
urlSameHost(const char *url1, int len1, const char *url2, int len2)
{
    int i;
    if(len1 < 7 || len2 < 7)
        return 0;
    if(memcmp(url1 + 4, "://", 3) != 0 || memcmp(url2 + 4, "://", 3) != 0)
        return 0;

    i = 7;
    while(i < len1 && i < len2 && url1[i] != '/' && url2[i] != '/') {
        if((url1[i] | 0x20) != (url2[i] | 0x20))
            break;
        i++;
    }

    if((i == len1 || url1[i] == '/') && ((i == len2 || url2[i] == '/')))
        return 1;
    return 0;
}

static char *
resize_hbuf(char *hbuf, int *size, char *hbuf_small)
{
    int new_size = 2 * *size;
    char *new_hbuf;

    if(new_size <= *size)
        goto fail;

    if(hbuf == hbuf_small) {
        new_hbuf = malloc(new_size);
        if(new_hbuf == NULL) goto fail;
        memcpy(new_hbuf, hbuf, *size);
    } else {
        new_hbuf = realloc(hbuf, new_size);
        if(new_hbuf == NULL) goto fail;
    }
    *size = new_size;
    return new_hbuf;

 fail:
    if(hbuf != hbuf_small)
        free(hbuf);
    *size = 0;
    return NULL;
}

int
httpParseHeaders(int client, AtomPtr url,
                 const char *buf, int start, HTTPRequestPtr request,
                 AtomPtr *headers_return,
                 int *len_return, CacheControlPtr cache_control_return,
                 HTTPConditionPtr *condition_return, int *te_return,
                 time_t *date_return, time_t *last_modified_return,
                 time_t *expires_return, time_t *polipo_age_return,
                 time_t *polipo_access_return, int *polipo_body_offset_return,
                 int *age_return, char **etag_return, AtomPtr *expect_return,
                 HTTPRangePtr range_return, HTTPRangePtr content_range_return,
                 char **location_return, AtomPtr *via_return,
                 AtomPtr *auth_return)
{
    int local = url ? urlIsLocal(url->string, url->length) : 0;
    char hbuf_small[512];
    char *hbuf = hbuf_small;
    int hbuf_size = 512, hbuf_length = 0;
    int i, j,
        name_start, name_end, value_start, value_end, 
        token_start, token_end, end;
    AtomPtr name = NULL;
    time_t date = -1, last_modified = -1, expires = -1, polipo_age = -1,
        polipo_access = -1, polipo_body_offset = -1;
    int len = -1;
    CacheControlRec cache_control;
    char *endptr;
    int te = TE_IDENTITY;
    int age = -1;
    char *etag = NULL, *ifrange = NULL;
    int persistent = (!request || (request->connection->version != HTTP_10));
    char *location = NULL;
    AtomPtr via = NULL;
    AtomPtr auth = NULL;
    AtomPtr expect = NULL;
    HTTPConditionPtr condition;
    time_t ims = -1, inms = -1;
    char *im = NULL, *inm = NULL;
    AtomListPtr hopToHop = NULL;
    HTTPRangeRec range = {-1, -1, -1}, content_range = {-1, -1, -1};
    int haveCacheControl = 0;
 
#define RESIZE_HBUF() \
    do { \
        hbuf = resize_hbuf(hbuf, &hbuf_size, hbuf_small); \
        if(hbuf == NULL) \
            goto fail; \
    } while(0)

    cache_control.flags = 0;
    cache_control.max_age = -1;
    cache_control.s_maxage = -1;
    cache_control.min_fresh = -1;
    cache_control.max_stale = -1;
    
    i = start;

    while(1) {
        i = parseHeaderLine(buf, i,
                            &name_start, &name_end, &value_start, &value_end);
        if(i < 0) {
            do_log(L_ERROR, "Couldn't find end of header line.\n");
            goto fail;
        }

        if(name_start == -1)
            break;

        if(name_start < 0)
            continue;

        name = internAtomLowerN(buf + name_start, name_end - name_start);

        if(name == atomConnection) {
            j = getNextTokenInList(buf, value_start, 
                                   &token_start, &token_end, NULL, NULL,
                                   &end);
            while(1) {
                if(j < 0) {
                    do_log(L_ERROR, "Couldn't parse Connection: ");
                    do_log_n(L_ERROR, buf + value_start, 
                             value_end - value_start);
                    do_log(L_ERROR, ".\n");
                    goto fail;
                }
                if(token_compare(buf, token_start, token_end, "close")) {
                    persistent = 0;
                } else if(token_compare(buf, token_start, token_end, 
                                        "keep-alive")) {
                    persistent = 1;
                } else {
                    if(hopToHop == NULL)
                        hopToHop = makeAtomList(NULL, 0);
                    if(hopToHop == NULL) {
                        do_log(L_ERROR, "Couldn't allocate atom list.\n");
                        goto fail;
                    }
                    atomListCons(internAtomLowerN(buf + token_start,
                                                  token_end - token_start),
                                 hopToHop);
                }
                if(end)
                    break;
                j = getNextTokenInList(buf, j, 
                                       &token_start, &token_end, NULL, NULL,
                                       &end);
            }
        } else if(name == atomCacheControl)
            haveCacheControl = 1;

        releaseAtom(name);
        name = NULL;
    }
    
    i = start;

    while(1) {
        i = parseHeaderLine(buf, i, 
                            &name_start, &name_end, &value_start, &value_end);
        if(i < 0) {
            do_log(L_ERROR, "Couldn't find end of header line.\n");
            goto fail;
        }

        if(name_start == -1)
            break;

        if(name_start < 0) {
            do_log(L_WARN, "Couldn't parse header line.\n");
            if(laxHttpParser)
                continue;
            else
                goto fail;
        }

        name = internAtomLowerN(buf + name_start, name_end - name_start);
        
        if(name == atomProxyConnection) {
            j = getNextTokenInList(buf, value_start, 
                                   &token_start, &token_end, NULL, NULL,
                                   &end);
            while(1) {
                if(j < 0) {
                    do_log(L_WARN, "Couldn't parse Proxy-Connection:");
                    do_log_n(L_WARN, buf + value_start, 
                             value_end - value_start);
                    do_log(L_WARN, ".\n");
                    persistent = 0;
                    break;
                }
                if(token_compare(buf, token_start, token_end, "close")) {
                    persistent = 0;
                } else if(token_compare(buf, token_start, token_end, 
                                        "keep-alive")) {
                    persistent = 1;
                }
                if(end)
                    break;
                j = getNextTokenInList(buf, j, 
                                       &token_start, &token_end, NULL, NULL,
                                       &end);
            }
        } else if(name == atomContentLength) {
            j = skipWhitespace(buf, value_start);
            if(j < 0) {
                do_log(L_WARN, "Couldn't parse Content-Length: \n");
                do_log_n(L_WARN, buf + value_start, value_end - value_start);
                do_log(L_WARN, ".\n");
                len = -1;
            } else {
                errno = 0;
                len = strtol(buf + value_start, &endptr, 10);
                if(errno == ERANGE || endptr <= buf + value_start) {
                    do_log(L_WARN, "Couldn't parse Content-Length: \n");
                    do_log_n(L_WARN, buf + value_start, 
                             value_end - value_start);
                    do_log(L_WARN, ".\n");
                    len = -1;
                }
            }
        } else if((!local && name == atomProxyAuthorization) ||
                  (local && name == atomAuthorization)) {
            if(auth_return) {
                auth = internAtomN(buf + value_start, value_end - value_start);
                if(auth == NULL) {
                    do_log(L_ERROR, "Couldn't allocate authorization.\n");
                    goto fail;
                }
            }
        } else if(name == atomReferer) {
            int h;
            if(censorReferer == 0 || 
               (censorReferer == 1 && url != NULL &&
                urlSameHost(url->string, url->length,
                            buf + value_start, value_end - value_start))) {
                while(hbuf_length > hbuf_size - 2)
                    RESIZE_HBUF();
                hbuf[hbuf_length++] = '\r';
                hbuf[hbuf_length++] = '\n';
                do {
                    h = snnprint_n(hbuf, hbuf_length, hbuf_size,
                                   buf + name_start, value_end - name_start);
                    if(h < 0) RESIZE_HBUF();
                } while(h < 0);
                hbuf_length = h;
            }
        } else if(name == atomTrailer || name == atomUpgrade) {
            do_log(L_ERROR, "Trailers or upgrade present.\n");
            goto fail;
        } else if(name == atomDate || name == atomExpires ||
                  name == atomIfModifiedSince || 
                  name == atomIfUnmodifiedSince ||
                  name == atomLastModified ||
                  name == atomXPolipoDate || name == atomXPolipoAccess) {
            time_t t;
            j = parse_time(buf, value_start, value_end, &t);
            if(j < 0) {
                if(name != atomExpires) {
                    do_log(L_WARN, "Couldn't parse %s: ", name->string);
                    do_log_n(L_WARN, buf + value_start,
                             value_end - value_start);
                    do_log(L_WARN, "\n");
                }
                t = -1;
            }
            if(name == atomDate) {
                if(t >= 0)
                    date = t;
            } else if(name == atomExpires) {
                if(t >= 0)
                    expires = t;
                else
                    expires = 0;
            } else if(name == atomLastModified)
                last_modified = t;
            else if(name == atomIfModifiedSince)
                ims = t;
            else if(name == atomIfUnmodifiedSince)
                inms = t;
            else if(name == atomXPolipoDate)
                polipo_age = t;
            else if(name == atomXPolipoAccess)
                polipo_access = t;
        } else if(name == atomAge) {
            j = skipWhitespace(buf, value_start);
            if(j < 0) {
                age = -1;
            } else {
                errno = 0;
                age = strtol(buf + value_start, &endptr, 10);
                if(errno == ERANGE || endptr <= buf + value_start)
                    age = -1;
            }
            if(age < 0) {
                do_log(L_WARN, "Couldn't parse age: \n");
                do_log_n(L_WARN, buf + value_start, value_end - value_start);
                do_log(L_WARN, " -- ignored.\n");
            }
        } else if(name == atomXPolipoBodyOffset) {
            j = skipWhitespace(buf, value_start);
            if(j < 0) {
                do_log(L_ERROR, "Couldn't parse body offset.\n");
                goto fail;
            } else {
                errno = 0;
                polipo_body_offset = strtol(buf + value_start, &endptr, 10);
                if(errno == ERANGE || endptr <= buf + value_start) {
                    do_log(L_ERROR, "Couldn't parse body offset.\n");
                    goto fail;
                }
            }
        } else if(name == atomTransferEncoding) {
            if(token_compare(buf, value_start, value_end, "identity"))
                te = TE_IDENTITY;
            else if(token_compare(buf, value_start, value_end, "chunked"))
                te = TE_CHUNKED;
            else
                te = TE_UNKNOWN;
        } else if(name == atomETag ||
                  name == atomIfNoneMatch || name == atomIfMatch ||
                  name == atomIfRange) {
            int x, y;
            int weak;
            char *e;
            j = getNextETag(buf, value_start, &x, &y, &weak);
            if(j < 0) {
                if(buf[value_start] != '\r' && buf[value_start] != '\n')
                    do_log(L_ERROR, "Couldn't parse ETag.\n");
            } else if(weak) {
                do_log(L_WARN, "Server returned weak ETag -- ignored.\n");
            } else {
                e = strdup_n(buf + x, y - x);
                if(e == NULL) goto fail;
                if(name == atomETag) {
                    if(!etag)
                        etag = e;
                    else
                        free(e);
                } else if(name == atomIfNoneMatch) {
                    if(!inm)
                        inm = e;
                    else
                        free(e);
                } else if(name == atomIfMatch) {
                    if(!im)
                        im = e;
                    else
                        free(e);
                } else if(name == atomIfRange) {
                    if(!ifrange)
                        ifrange = e;
                    else
                        free(e);
                } else {
                    abort();
                }
            }
        } else if(name == atomCacheControl) {
            int v_start, v_end;
            j = getNextTokenInList(buf, value_start, 
                                   &token_start, &token_end, 
                                   &v_start, &v_end,
                                   &end);
            while(1) {
                if(j < 0) {
                    do_log(L_WARN, "Couldn't parse Cache-Control.\n");
                    cache_control.flags |= CACHE_NO;
                    break;
                }
                if(token_compare(buf, token_start, token_end, "no-cache")) {
                    cache_control.flags |= CACHE_NO;
                } else if(token_compare(buf, token_start, token_end,
                                        "public")) {
                    cache_control.flags |= CACHE_PUBLIC;
                } else if(token_compare(buf, token_start, token_end, 
                                        "private")) {
                    cache_control.flags |= CACHE_PRIVATE;
                } else if(token_compare(buf, token_start, token_end, 
                                        "no-store")) {
                    cache_control.flags |= CACHE_NO_STORE;
                } else if(token_compare(buf, token_start, token_end, 
                                        "no-transform")) {
                    cache_control.flags |= CACHE_NO_TRANSFORM;
                } else if(token_compare(buf, token_start, token_end,
                                        "must-revalidate") ||
                          token_compare(buf, token_start, token_end,
                                        "must-validate")) { /* losers */
                    cache_control.flags |= CACHE_MUST_REVALIDATE;
                } else if(token_compare(buf, token_start, token_end, 
                                        "proxy-revalidate")) {
                    cache_control.flags |= CACHE_PROXY_REVALIDATE;
                } else if(token_compare(buf, token_start, token_end,
                                        "only-if-cached")) {
                    cache_control.flags |= CACHE_ONLY_IF_CACHED;
                } else if(token_compare(buf, token_start, token_end,
                                        "max-age") ||
                          token_compare(buf, token_start, token_end,
                                        "maxage") || /* losers */
                          token_compare(buf, token_start, token_end,
                                        "s-maxage") ||
                          token_compare(buf, token_start, token_end,
                                        "min-fresh")) {
                    parseCacheControl(buf, token_start, token_end,
                                      v_start, v_end,
                                      &cache_control.max_age);
                } else if(token_compare(buf, token_start, token_end,
                                        "max-stale")) {
                    parseCacheControl(buf, token_start, token_end,
                                      v_start, v_end,
                                      &cache_control.max_stale);
                } else {
                    do_log(L_WARN, "Unsupported Cache-Control directive ");
                    do_log_n(L_WARN, buf + token_start, 
                             (v_end >= 0 ? v_end : token_end) - token_start);
                    do_log(L_WARN, " -- ignored.\n");
                }
                if(end)
                    break;
                j = getNextTokenInList(buf, j, 
                                       &token_start, &token_end,
                                       &v_start, &v_end,
                                       &end);
            }
        } else if(name == atomContentRange) {
            if(!client) {
                j = parseContentRange(buf, value_start, 
                                      &content_range.from, &content_range.to, 
                                      &content_range.full_length);
                if(j < 0) {
                    do_log(L_ERROR, "Couldn't parse Content-Range: ");
                    do_log_n(L_ERROR, buf + value_start, 
                             value_end - value_start);
                    do_log(L_ERROR, "\n");
                    goto fail;
                }
            } else {
                do_log(L_ERROR, "Content-Range from client.\n");
                goto fail;
            }
        } else if(name == atomRange) {
            if(client) {
                j = parseRange(buf, value_start, &range.from, &range.to);
                if(j < 0) {
                    do_log(L_WARN, "Couldn't parse Range -- ignored.\n");
                    range.from = -1;
                    range.to = -1;
                }
            } else {
                do_log(L_WARN, "Range from server -- ignored\n");
            }
        } else if(name == atomXPolipoLocation) {
            if(location_return) {
                location = 
                    strdup_n(buf + value_start, value_end - value_start);
                if(location == NULL) {
                    do_log(L_ERROR, "Couldn't allocate location.\n");
                    goto fail;
                }
            }
        } else if(name == atomVia) {
            if(via_return) {
                AtomPtr new_via, full_via;
                new_via =
                    internAtomN(buf + value_start, value_end - value_start);
                if(new_via == NULL) {
                    do_log(L_ERROR, "Couldn't allocate via.\n");
                    goto fail;
                }
                if(via) {
                    full_via =
                        internAtomF("%s, %s", via->string, new_via->string);
                    releaseAtom(new_via);
                    if(full_via == NULL) {
                        do_log(L_ERROR, "Couldn't allocate via");
                        goto fail;
                    }
                    releaseAtom(via);
                    via = full_via;
                } else {
                    via = new_via;
                }
            }
        } else if(name == atomExpect) {
            if(expect_return) {
                expect = internAtomLowerN(buf + value_start, 
                                          value_end - value_start);
                if(expect == NULL) {
                    do_log(L_ERROR, "Couldn't allocate expect.\n");
                    goto fail;
                }
            }
        } else {
            if(!client && name == atomContentType) {
                if(token_compare(buf, value_start, value_end,
                                 "multipart/byteranges")) {
                    do_log(L_ERROR, 
                           "Server returned multipart/byteranges -- yuck!\n");
                    goto fail;
                }
            } 
            if(name == atomVary) {
                if(!token_compare(buf, value_start, value_end, "host") &&
                   !token_compare(buf, value_start, value_end, "*")) {
                    /* What other vary headers should be ignored? */
                    do_log(L_VARY, "Vary header present (");
                    do_log_n(L_VARY,
                             buf + value_start, value_end - value_start);
                    do_log(L_VARY, ").\n");
                }
                cache_control.flags |= CACHE_VARY;
            } else if(name == atomAuthorization) {
                cache_control.flags |= CACHE_AUTHORIZATION;
            } 

            if(name == atomPragma) {
                /* Pragma is only defined for the client, and the only
                   standard value is no-cache (RFC 1945, 10.12).
                   However, we honour a Pragma: no-cache for both the client
                   and the server when there's no Cache-Control header.  In
                   all cases, we pass the Pragma header to the next hop. */
                if(!haveCacheControl) {
                    j = getNextTokenInList(buf, value_start,
                                           &token_start, &token_end, NULL, NULL,
                                           &end);
                    while(1) {
                        if(j < 0) {
                            do_log(L_WARN, "Couldn't parse Pragma.\n");
                            cache_control.flags |= CACHE_NO;
                            break;
                        }
                        if(token_compare(buf, token_start, token_end,
                                         "no-cache"))
                            cache_control.flags = CACHE_NO;
                        if(end)
                            break;
                        j = getNextTokenInList(buf, j, &token_start, &token_end,
                                               NULL, NULL, &end);
                    }
                }
            }
            if(!client &&
               (name == atomSetCookie || 
                name == atomCookie || name == atomCookie2))
                cache_control.flags |= CACHE_COOKIE;

            if(hbuf) {
                if(name != atomConnection && name != atomHost &&
                   name != atomAcceptRange && name != atomTE &&
                   name != atomProxyAuthenticate &&
                   name != atomKeepAlive &&
                   (!hopToHop || !atomListMember(name, hopToHop)) &&
                   !atomListMember(name, censoredHeaders)) {
                    int h;
                    while(hbuf_length > hbuf_size - 2)
                        RESIZE_HBUF();
                    hbuf[hbuf_length++] = '\r';
                    hbuf[hbuf_length++] = '\n';
                    do {
                        h = snnprint_n(hbuf, hbuf_length, hbuf_size,
                                       buf + name_start, 
                                       value_end - name_start);
                        if(h < 0) RESIZE_HBUF();
                    } while(h < 0);
                    hbuf_length = h;
                }
            }
        }
        releaseAtom(name);
        name = NULL;
    }

    if(headers_return) {
        AtomPtr pheaders = NULL; 
        pheaders = internAtomN(hbuf, hbuf_length);
        if(!pheaders)
            goto fail;
        *headers_return = pheaders;
    }
    if(hbuf != hbuf_small)
        free(hbuf);
    hbuf = NULL;
    hbuf_size = 0;

    if(request)
        if(!persistent)
            request->flags &= ~REQUEST_PERSISTENT;

    if(te != TE_IDENTITY) len = -1;
    if(len_return) *len_return = len;
    if(cache_control_return) *cache_control_return = cache_control;
    if(condition_return) {
        if(ims >= 0 || inms >= 0 || im || inm || ifrange) {
            condition = httpMakeCondition();
            if(condition) {
                condition->ims = ims;
                condition->inms = inms;
                condition->im = im;
                condition->inm = inm;
                condition->ifrange = ifrange;
            } else {
                do_log(L_ERROR, "Couldn't allocate condition.\n");
                if(im) free(im);
                if(inm) free(inm);
            }
        } else {
            condition = NULL;
        }
        *condition_return = condition;
    } else {
        assert(!im && !inm);
    }
            
    if(te_return) *te_return = te;
    if(date_return) *date_return = date;
    if(last_modified_return) *last_modified_return = last_modified;
    if(expires_return) *expires_return = expires;
    if(polipo_age_return) *polipo_age_return = polipo_age;
    if(polipo_access_return) *polipo_access_return = polipo_access;
    if(polipo_body_offset_return)
        *polipo_body_offset_return = polipo_body_offset;
    if(age_return) *age_return = age;
    if(etag_return)
        *etag_return = etag;
    else {
        if(etag) free(etag);
    }
    if(range_return) *range_return = range;
    if(content_range_return) *content_range_return = content_range;
    if(location_return) {
        *location_return = location;
    } else {
        if(location)
            free(location);
    }
    if(via_return)
        *via_return = via;
    else {
        if(via)
            releaseAtom(via);
    }
    if(expect_return)
        *expect_return = expect;
    else {
        if(expect)
            releaseAtom(expect);
    }
    if(auth_return)
        *auth_return = auth;
    else {
        if(auth)
            releaseAtom(auth);
    }
    if(hopToHop) destroyAtomList(hopToHop);
    return i;

 fail:
    if(hbuf && hbuf != hbuf_small) free(hbuf);
    if(name) releaseAtom(name);
    if(etag) free(etag);
    if(location) free(location);
    if(via) releaseAtom(via);
    if(expect) releaseAtom(expect);
    if(auth) releaseAtom(auth);
    if(hopToHop) destroyAtomList(hopToHop);
        
    return -1;
#undef RESIZE_HBUF
}

int
httpFindHeader(AtomPtr header, const char *headers, int hlen,
               int *value_begin_return, int *value_end_return)
{
    int len = header->length;
    int i = 0;

    while(i + len + 1 < hlen) {
        if(headers[i + len] == ':' &&
           lwrcmp(headers + i, header->string, len) == 0) {
            int j = i + len + 1, k;
            while(j < hlen && headers[j] == ' ')
                j++;
            k = j;
            while(k < hlen && headers[k] != '\n' && headers[k] != '\r')
                k++;
            *value_begin_return = j;
            *value_end_return = k;
            return 1;
        } else {
            while(i < hlen && headers[i] != '\n' && headers[i] != '\r')
                i++;
            i++;
            if(i < hlen && headers[i] == '\n')
                i++;
        }
    }
    return 0;
}

int
parseUrl(const char *url, int len,
         int *x_return, int *y_return, int *port_return, int *z_return)
{
    int x, y, z, port = -1, i = 0;

    if(len >= 7 && lwrcmp(url, "http://", 7) == 0) {
        x = 7;
        if(x < len && url[x] == '[') {
            /* RFC 2732 */
            for(i = x + 1; i < len; i++) {
                if(url[i] == ']') {
                    i++;
                    break;
                }
                if((url[i] != ':') && !letter(url[i]) && !digit(url[i]))
                    break;
            }
        } else {
            for(i = x; i < len; i++)
                if(url[i] == ':' || url[i] == '/')
                    break;
        }
        y = i;

        if(i < len && url[i] == ':') {
            int j;
            j = atoi_n(url, i + 1, len, &port);
            if(j < 0) {
                port = 80;
            } else {
                    i = j;
            }
        } else {
            port = 80;
        }
    } else {
        x = -1;
        y = -1;
    }

    z = i;

    *x_return = x;
    *y_return = y;
    *port_return = port;
    *z_return = z;
    return 0;
}

int
urlIsLocal(const char *url, int len)
{
    return (len > 0 && url[0] == '/');
}

int
urlIsSpecial(const char *url, int len)
{
    return (len >= 8 && memcmp(url, "/polipo/", 8) == 0);
}

int
parseChunkSize(const char *restrict buf, int i, int end,
               int *chunk_size_return)
{
    int v, d;
    v = h2i(buf[i]);
    if(v < 0)
        return -1;

    i++;

    while(i < end) {
        d = h2i(buf[i]);
        if(d < 0)
            break;
        v = v * 16 + d;
        i++;
    }

    while(i < end) {
        if(buf[i] == ' ' || buf[i] == '\t')
            i++;
        else
            break;
    }

    if(i >= end - 1)
        return 0;

    if(buf[i] != '\r' || buf[i + 1] != '\n')
        return -1;

    i += 2;

    if(v == 0) {
        if(i >= end - 1)
            return 0;
        if(buf[i] != '\r') {
            do_log(L_ERROR, "Trailers present!\n");
            return -1;
        }
        i++;
        if(buf[i] != '\n')
            return -1;
        i++;
    }

    *chunk_size_return = v;
    return i;
}


int
checkVia(AtomPtr name, AtomPtr via)
{
    int i;
    char *v;
    if(via == NULL || via->length == 0)
        return 1;

    v = via->string;

    i = 0;
    while(i < via->length) {
        while(v[i] == ' ' || v[i] == '\t' || v[i] == ',' ||
              v[i] == '\r' || v[i] == '\n' ||
              digit(v[i]) || v[i] == '.')
            i++;
        if(i + name->length > via->length)
            break;
        if(memcmp(v + i, name->string, name->length) == 0) {
            char c = v[i + name->length];
            if(c == '\0' || c == ' ' || c == '\t' || c == ',' ||
               c == '\r' || c == '\n')
                return 0;
        }
        i++;
        while(letter(v[i]) || digit(v[i]) || v[i] == '.')
            i++;
    }
    return 1;
}

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

typedef struct HTTPRange {
    int from;
    int to;
    int full_length;
} HTTPRangeRec, *HTTPRangePtr;

extern int censorReferer;
extern AtomPtr atomContentType, atomContentEncoding;

void preinitHttpParser(void);
void initHttpParser(void);
int httpParseClientFirstLine(const char *buf, int offset,
                             int *method_return,
                             AtomPtr *url_return,
                             int *version_return);
int httpParseServerFirstLine(const char *buf, 
                             int *status_return,
                             int *version_return,
                             AtomPtr *message_return);

int findEndOfHeaders(const char *buf, int from, int to, int *body_return);

int httpParseHeaders(int, AtomPtr, const char *, int, HTTPRequestPtr,
                     AtomPtr*, int*, CacheControlPtr, 
                     HTTPConditionPtr *, int*,
                     time_t*, time_t*, time_t*, time_t*, time_t*,
                     int*, int*, char**, AtomPtr*,
                     HTTPRangePtr, HTTPRangePtr, char**, AtomPtr*, AtomPtr*);
int httpFindHeader(AtomPtr header, const char *headers, int hlen,
                   int *value_begin_return, int *value_end_return);
int parseUrl(const char *url, int len,
             int *x_return, int *y_return, int *port_return, int *z_return);
int urlIsLocal(const char *url, int len);
int urlIsSpecial(const char *url, int len);
int parseChunkSize(const char *buf, int i, int end, int *chunk_size_return);
int checkVia(AtomPtr, AtomPtr);

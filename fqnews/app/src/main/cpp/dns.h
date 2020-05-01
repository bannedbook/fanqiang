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

extern char *nameServer;
extern int useGethostbyname;

#define DNS_A 0
#define DNS_CNAME 1

typedef struct _GethostbynameRequest {
    AtomPtr name;
    AtomPtr addr;
    AtomPtr error_message;
    int count;
    ObjectPtr object;
    int (*handler)(int, struct _GethostbynameRequest*);
    void *data;
} GethostbynameRequestRec, *GethostbynameRequestPtr;

/* Note that this requires no alignment */
typedef struct _HostAddress {
    char af;                     /* 4 or 6 */
    char data[16];
} HostAddressRec, *HostAddressPtr;

void preinitDns(void);
void initDns(void);
int do_gethostbyname(char *name, int count,
                     int (*handler)(int, GethostbynameRequestPtr), void *data);

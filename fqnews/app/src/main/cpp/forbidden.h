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

extern AtomPtr forbiddenUrl;
extern int forbiddenRedirectCode;

typedef struct _RedirectRequest {
    AtomPtr url;
    struct _RedirectRequest *next;
    int (*handler)(int, AtomPtr, AtomPtr, AtomPtr, void*);
    void *data;
} RedirectRequestRec, *RedirectRequestPtr;

void preinitForbidden(void);
void initForbidden(void);
int urlIsUncachable(char *url, int length);
int urlForbidden(AtomPtr url,
                 int (*handler)(int, AtomPtr, AtomPtr, AtomPtr, void*),
                 void *closure);
void redirectorKill(void);
int redirectorStreamHandler1(int status,
                             FdEventHandlerPtr event,
                             StreamRequestPtr srequest);
int redirectorStreamHandler2(int status,
                             FdEventHandlerPtr event,
                             StreamRequestPtr srequest);
void redirectorTrigger(void);
int 
runRedirector(pid_t *pid_return, int *read_fd_return, int *write_fd_return);

int tunnelIsMatched(char *url, int lurl, char *hostname, int lhost);

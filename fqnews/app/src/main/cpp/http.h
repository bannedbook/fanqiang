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

typedef struct _HTTPCondition {
    time_t ims;
    time_t inms;
    char *im;
    char *inm;
    char *ifrange;
} HTTPConditionRec, *HTTPConditionPtr;

typedef struct _HTTPRequest {
    int flags;
    struct _HTTPConnection *connection;
    ObjectPtr object;
    int method;
    int from;
    int to;
    CacheControlRec cache_control;
    HTTPConditionPtr condition;
    AtomPtr via;
    struct _ConditionHandler *chandler;
    ObjectPtr can_mutate;
    int error_code;
    struct _Atom *error_message;
    struct _Atom *error_headers;
    AtomPtr headers;
    struct timeval time0, time1;
    struct _HTTPRequest *request;
    struct _HTTPRequest *next;
} HTTPRequestRec, *HTTPRequestPtr;

/* request->flags */
/* If not present, drop the connection after this request. */
#define REQUEST_PERSISTENT 1
/* This client-side request has already been requested on the server-side. */
#define REQUEST_REQUESTED 2
/* This request is waiting for continue from the server. */
#define REQUEST_WAIT_CONTINUE 4
/* Force an error unconditionally -- used for client auth failures. */
#define REQUEST_FORCE_ERROR 8
/* This server-side request was pipelined. */
#define REQUEST_PIPELINED 16
/* This client-side request has already switched objects once. */
#define REQUEST_SUPERSEDED 32

typedef struct _HTTPConnection {
    int flags;
    int fd;
    char *buf;
    int len;
    int offset;
    HTTPRequestPtr request;
    HTTPRequestPtr request_last;
    int serviced;
    int version;
    int time;
    TimeEventHandlerPtr timeout;
    int te;
    char *reqbuf;
    int reqlen;
    int reqbegin;
    int reqoffset;
    int bodylen;
    int reqte;
    /* For server connections */
    int chunk_remaining;
    struct _HTTPServer *server;
    int pipelined;
    int connecting;
} HTTPConnectionRec, *HTTPConnectionPtr;

/* connection->flags */
#define CONN_READER 1
#define CONN_WRITER 2
#define CONN_SIDE_READER 4
#define CONN_BIGBUF 8
#define CONN_BIGREQBUF 16

/* request->method */
#define METHOD_UNKNOWN -1
#define METHOD_NONE -1
#define METHOD_GET 0
#define METHOD_HEAD 1
#define METHOD_CONDITIONAL_GET 2
#define METHOD_CONNECT 3
#define METHOD_POST 4
#define METHOD_PUT 5

#define REQUEST_SIDE(request) ((request)->method >= METHOD_POST)

/* server->version */
#define HTTP_10 0
#define HTTP_11 1
#define HTTP_UNKNOWN -1

/* connection->te */
#define TE_IDENTITY 0
#define TE_CHUNKED 1
#define TE_UNKNOWN -1

/* connection->connecting */
#define CONNECTING_DNS 1
#define CONNECTING_CONNECT 2
#define CONNECTING_SOCKS 3

/* the results of a conditional request.  200, 304 and 412. */
#define CONDITION_MATCH 0
#define CONDITION_NOT_MODIFIED 1
#define CONDITION_FAILED 2

extern int disableProxy;
extern AtomPtr proxyName;
extern int proxyPort;
extern int clientTimeout, serverTimeout, serverIdleTimeout;
extern int bigBufferSize;
extern AtomPtr proxyAddress;
extern int proxyOffline;
extern int relaxTransparency;
extern AtomPtr authRealm;
extern AtomPtr authCredentials;
extern AtomPtr parentAuthCredentials;
extern AtomListPtr allowedClients;
extern NetAddressPtr allowedNets;
extern IntListPtr allowedPorts;
extern IntListPtr tunnelAllowedPorts;
extern int expectContinue;
extern AtomPtr atom100Continue;
extern int disableVia;
extern int dontTrustVaryETag;

void preinitHttp(void);
void initHttp(void);

int httpTimeoutHandler(TimeEventHandlerPtr);
int httpSetTimeout(HTTPConnectionPtr connection, int secs);
int httpWriteObjectHeaders(char *buf, int offset, int len, 
                           ObjectPtr object, int from, int to);
int httpPrintCacheControl(char*, int, int, int, CacheControlPtr);
char *httpMessage(int) ATTRIBUTE((pure));
int htmlString(char *buf, int n, int len, char *s, int slen);
void htmlPrint(FILE *out, char *s, int slen);
HTTPConnectionPtr httpMakeConnection(void);
void httpDestroyConnection(HTTPConnectionPtr connection);
void httpConnectionDestroyBuf(HTTPConnectionPtr connection);
void httpConnectionDestroyReqbuf(HTTPConnectionPtr connection);
HTTPRequestPtr httpMakeRequest(void);
void httpDestroyRequest(HTTPRequestPtr request);
void httpQueueRequest(HTTPConnectionPtr, HTTPRequestPtr);
HTTPRequestPtr httpDequeueRequest(HTTPConnectionPtr connection);
int httpConnectionBigify(HTTPConnectionPtr);
int httpConnectionBigifyReqbuf(HTTPConnectionPtr);
int httpConnectionUnbigify(HTTPConnectionPtr);
int httpConnectionUnbigifyReqbuf(HTTPConnectionPtr);
HTTPConditionPtr httpMakeCondition(void);
void httpDestroyCondition(HTTPConditionPtr condition);
int httpCondition(ObjectPtr, HTTPConditionPtr);
int httpWriteErrorHeaders(char *buf, int size, int offset, int do_body,
                          int code, AtomPtr message, int close, AtomPtr,
                          char *url, int url_len, char *etag);
AtomListPtr urlDecode(char*, int);
void httpTweakCachability(ObjectPtr);
int httpHeaderMatch(AtomPtr header, AtomPtr headers1, AtomPtr headers2);

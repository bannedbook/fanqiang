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

extern int serverExpireTime, dontCacheRedirects;

typedef struct _HTTPServer {
    char *name;
    int port;
    int addrindex;
    int isProxy;
    int version;
    int persistent;
    int pipeline;
    int lies;
    int rtt;
    int rate;
    time_t time;
    int numslots;
    int maxslots;
    HTTPConnectionPtr *connection;
    FdEventHandlerPtr *idleHandler;
    HTTPRequestPtr request, request_last;
    struct _HTTPServer *next;
} HTTPServerRec, *HTTPServerPtr;

extern AtomPtr parentHost;
extern int parentPort;

void preinitServer(void);
void initServer(void);

void httpServerAbortHandler(ObjectPtr object);
int httpMakeServerRequest(char *name, int port, ObjectPtr object, 
                          int method, int from, int to,
                          HTTPRequestPtr requestor);
int httpServerQueueRequest(HTTPServerPtr server, HTTPRequestPtr request);
int httpServerTrigger(HTTPServerPtr server);
int httpServerSideRequest(HTTPServerPtr server);
int  httpServerDoSide(HTTPConnectionPtr connection);
int httpServerSideHandler(int status,
                          FdEventHandlerPtr event,
                          StreamRequestPtr srequest);
int httpServerSideHandler2(int status,
                           FdEventHandlerPtr event,
                           StreamRequestPtr srequest);
int httpServerConnectionDnsHandler(int status, 
                                   GethostbynameRequestPtr request);
int httpServerConnectionHandler(int status,
                                FdEventHandlerPtr event,
                                ConnectRequestPtr request);
int httpServerSocksHandler(int status, SocksRequestPtr request);
int httpServerConnectionHandlerCommon(int status,
                                      HTTPConnectionPtr connection);
void httpServerFinish(HTTPConnectionPtr connection, int s, int offset);

void httpServerReply(HTTPConnectionPtr connection, int immediate);
void httpServerAbort(HTTPConnectionPtr connection, int, int, struct _Atom *);
void httpServerAbortRequest(HTTPRequestPtr request, int, int, struct _Atom *);
void httpServerClientReset(HTTPRequestPtr request);
void httpServerUnpipeline(HTTPRequestPtr request);
int
httpServerSendRequest(HTTPConnectionPtr connection);
int
httpServerHandler(int status, 
                    FdEventHandlerPtr event,
                    StreamRequestPtr request);
int
httpServerReplyHandler(int status,
                       FdEventHandlerPtr event, 
                       StreamRequestPtr request);
int
httpServerIndirectHandler(int status,
                          FdEventHandlerPtr event, 
                          StreamRequestPtr request);
int
httpServerDirectHandler(int status,
                        FdEventHandlerPtr event, 
                        StreamRequestPtr request);
int
httpServerDirectHandler2(int status,
                         FdEventHandlerPtr event, 
                         StreamRequestPtr request);
int httpServerRequest(ObjectPtr object, int method, int from, int to,
                      HTTPRequestPtr, void*);
int httpServerHandlerHeaders(int eof,
                             FdEventHandlerPtr event,
                             StreamRequestPtr request, 
                             HTTPConnectionPtr connection);
int httpServerReadData(HTTPConnectionPtr, int);
int connectionAddData(HTTPConnectionPtr connection, int skip);
int 
httpWriteRequest(HTTPConnectionPtr connection, HTTPRequestPtr request, int);

void listServers(FILE*);

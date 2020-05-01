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

static int 
httpAcceptAgain(TimeEventHandlerPtr event)
{
    FdEventHandlerPtr newevent;
    int fd = *(int*)event->data;

    newevent = schedule_accept(fd, httpAccept, NULL);
    if(newevent == NULL) {
        free_chunk_arenas();
        newevent = schedule_accept(fd, httpAccept, NULL);
        if(newevent == NULL) {
            do_log(L_ERROR, "Couldn't schedule accept.\n");
            polipoExit();
        }
    }
    return 1;
}

int
httpAccept(int fd, FdEventHandlerPtr event, AcceptRequestPtr request)
{
    int rc;
    HTTPConnectionPtr connection;
    TimeEventHandlerPtr timeout;

    if(fd < 0) {
        if(-fd == EINTR || -fd == EAGAIN || -fd == EWOULDBLOCK)
            return 0;
        do_log_error(L_ERROR, -fd, "Couldn't establish listening socket");
        if(-fd == EMFILE || -fd == ENOMEM || -fd == ENOBUFS) {
            TimeEventHandlerPtr again = NULL;
            do_log(L_WARN, "Refusing client connections for one second.\n");
            free_chunk_arenas();
            again = scheduleTimeEvent(1, httpAcceptAgain, 
                                      sizeof(request->fd), &request->fd);
            if(!again) {
                do_log(L_ERROR, "Couldn't schedule accept -- sleeping.\n");
                sleep(1);
                again = scheduleTimeEvent(1, httpAcceptAgain, 
                                          sizeof(request->fd), &request->fd);
                if(!again) {
                    do_log(L_ERROR, "Couldn't schedule accept -- aborting.\n");
                    polipoExit();
                }
            }
            return 1;
        } else {
            polipoExit();
            return 1;
        }
    }

    if(allowedNets) {
        if(netAddressMatch(fd, allowedNets) != 1) {
            do_log(L_WARN, "Refusing connection from unauthorised net\n");
            CLOSE(fd);
            return 0;
        }
    }

    rc = setNonblocking(fd, 1);
    if(rc < 0) {
        do_log_error(L_WARN, errno, "Couldn't set non blocking mode");
        CLOSE(fd);
        return 0;
    }
    rc = setNodelay(fd, 1);
    if(rc < 0) 
        do_log_error(L_WARN, errno, "Couldn't disable Nagle's algorithm");

    connection = httpMakeConnection();

    timeout = scheduleTimeEvent(clientTimeout, httpTimeoutHandler,
                                sizeof(connection), &connection);
    if(!timeout) {
        CLOSE(fd);
        free(connection);
        return 0;
    }

    connection->fd = fd;
    connection->timeout = timeout;

    do_log(D_CLIENT_CONN, "Accepted client connection 0x%lx\n",
           (unsigned long)connection);

    connection->flags = CONN_READER;

    do_stream_buf(IO_READ | IO_NOTNOW, connection->fd, 0,
                  &connection->reqbuf, CHUNK_SIZE,
                  httpClientHandler, connection);
    return 0;
}

/* Abort a client connection.  It is only safe to abort the requests
   if we know the connection is closed. */
void
httpClientAbort(HTTPConnectionPtr connection, int closed)
{
    HTTPRequestPtr request = connection->request;

    pokeFdEvent(connection->fd, -EDOSHUTDOWN, POLLOUT);
    if(closed) {
        while(request) {
            if(request->chandler) {
                request->error_code = 500;
                request->error_message = internAtom("Connection finishing");
                abortConditionHandler(request->chandler);
                request->chandler = NULL;
            }
            request = request->next;
        }
    }
}

/* s != 0 specifies that the connection must be shut down.  It is 1 in
   order to linger the connection, 2 to close it straight away. */
void
httpClientFinish(HTTPConnectionPtr connection, int s)
{
    HTTPRequestPtr request = connection->request;

    assert(!(request && request->request 
             && request->request->request != request));

    if(s == 0) {
        if(!request || !(request->flags & REQUEST_PERSISTENT))
            s = 1;
    }

    httpConnectionDestroyBuf(connection);

    connection->flags &= ~CONN_WRITER;

    if(connection->flags & CONN_SIDE_READER) {
        /* We're in POST or PUT and the reader isn't done yet.
           Wait for the read side to close the connection. */
        assert(request && (connection->flags & CONN_READER));
        if(s >= 2) {
            pokeFdEvent(connection->fd, -EDOSHUTDOWN, POLLIN);
        } else {
            pokeFdEvent(connection->fd, -EDOGRACEFUL, POLLIN);
        }
        return;
    }

    if(connection->timeout) 
        cancelTimeEvent(connection->timeout);
    connection->timeout = NULL;

    if(request) {
        HTTPRequestPtr requestee;

        requestee = request->request;
        if(requestee) {
            request->request = NULL;
            requestee->request = NULL;
        }
        if(requestee)
            httpServerClientReset(requestee);
        if(request->chandler) {
            request->error_code = 500;
            request->error_message = internAtom("Connection finishing");
            abortConditionHandler(request->chandler);
            request->chandler = NULL;
        }
            
        if(request->object) {
            if(request->object->requestor == request)
                request->object->requestor = NULL;
            releaseObject(request->object);
            request->object = NULL;
        }
        httpDequeueRequest(connection);
        httpDestroyRequest(request);
        request = NULL;

    }

    connection->len = -1;
    connection->offset = 0;
    connection->te = TE_IDENTITY;

    if(!s) {
        assert(connection->fd > 0);
        connection->serviced++;
        httpSetTimeout(connection, clientTimeout);
        if(!(connection->flags & CONN_READER)) {
            if(connection->reqlen == 0)
                httpConnectionDestroyReqbuf(connection);
            else if((connection->flags & CONN_BIGREQBUF) &&
                    connection->reqlen < CHUNK_SIZE)
                httpConnectionUnbigifyReqbuf(connection);
            connection->flags |= CONN_READER;
            httpSetTimeout(connection, clientTimeout);
            do_stream_buf(IO_READ | IO_NOTNOW |
                          (connection->reqlen ? IO_IMMEDIATE : 0),
                          connection->fd, connection->reqlen,
                          &connection->reqbuf,
                          (connection->flags & CONN_BIGREQBUF) ?
                          bigBufferSize : CHUNK_SIZE,
                          httpClientHandler, connection);
        }
        /* The request has already been validated when it first got
           into the queue */
        if(connection->request) {
            if(connection->request->object != NULL)
                httpClientNoticeRequest(connection->request, 1);
            else
                assert(connection->flags & CONN_READER);
        }
        return;
    }
    
    do_log(D_CLIENT_CONN, "Closing client connection 0x%lx\n",
           (unsigned long)connection);

    if(connection->flags & CONN_READER) {
        httpSetTimeout(connection, 10);
        if(connection->fd < 0) return;
        if(s >= 2) {
            pokeFdEvent(connection->fd, -EDOSHUTDOWN, POLLIN);
        } else {
            pokeFdEvent(connection->fd, -EDOGRACEFUL, POLLIN);
        }
        return;
    }
    while(1) {
        HTTPRequestPtr requestee;
        request = connection->request;
        if(!request)
            break;
        requestee = request->request;
        request->request = NULL;
        if(requestee) {
            requestee->request = NULL;
            httpServerClientReset(requestee);
        }
        if(request->chandler)
            abortConditionHandler(request->chandler);
        request->chandler = NULL;
        if(request->object && request->object->requestor == request)
            request->object->requestor = NULL;
        httpDequeueRequest(connection);
        httpDestroyRequest(request);
    }
    httpConnectionDestroyReqbuf(connection);
    if(connection->timeout)
        cancelTimeEvent(connection->timeout);
    connection->timeout = NULL;
    if(connection->fd >= 0) {
        if(s >= 2)
            CLOSE(connection->fd);
        else
            lingeringClose(connection->fd);
    }
    connection->fd = -1;
    free(connection);
}

/* Extremely baroque implementation of close: we need to synchronise
   between the writer and the reader.  */

static char client_shutdown_buffer[17];

static int httpClientDelayedShutdownHandler(TimeEventHandlerPtr);

static int
httpClientDelayedShutdown(HTTPConnectionPtr connection)
{
    TimeEventHandlerPtr handler;

    assert(connection->flags & CONN_READER);
    handler = scheduleTimeEvent(1, httpClientDelayedShutdownHandler,
                                sizeof(connection), &connection);
    if(!handler) {
        do_log(L_ERROR, 
               "Couldn't schedule delayed shutdown -- freeing memory.");
        free_chunk_arenas();
        handler = scheduleTimeEvent(1, httpClientDelayedShutdownHandler,
                                    sizeof(connection), &connection);
        if(!handler) {
            do_log(L_ERROR, 
                   "Couldn't schedule delayed shutdown -- aborting.\n");
            polipoExit();
        }
    }
    return 1;
}

static int
httpClientShutdownHandler(int status,
                          FdEventHandlerPtr event, StreamRequestPtr request)
{
    HTTPConnectionPtr connection = request->data;

    assert(connection->flags & CONN_READER);

    if(!(connection->flags & CONN_WRITER)) {
        connection->flags &= ~CONN_READER;
        connection->reqlen = 0;
        httpConnectionDestroyReqbuf(connection);
        if(status && status != -EDOGRACEFUL)
            httpClientFinish(connection, 2);
        else
            httpClientFinish(connection, 1);
        return 1;
    }

    httpClientDelayedShutdown(connection);
    return 1;
}

static int 
httpClientDelayedShutdownHandler(TimeEventHandlerPtr event)
{
    HTTPConnectionPtr connection = *(HTTPConnectionPtr*)event->data;
    assert(connection->flags & CONN_READER);

    if(!(connection->flags & CONN_WRITER)) {
        connection->flags &= ~CONN_READER;
        connection->reqlen = 0;
        httpConnectionDestroyReqbuf(connection);
        httpClientFinish(connection, 1);
        return 1;
    }
    do_stream(IO_READ | IO_NOTNOW, connection->fd, 
              0, client_shutdown_buffer, 17, 
              httpClientShutdownHandler, connection);
    return 1;
}

int
httpClientHandler(int status,
                  FdEventHandlerPtr event, StreamRequestPtr request)
{
    HTTPConnectionPtr connection = request->data;
    int i, body;
    int bufsize = 
        (connection->flags & CONN_BIGREQBUF) ? bigBufferSize : CHUNK_SIZE;

    assert(connection->flags & CONN_READER);

    /* There's no point trying to do something with this request if
       the client has shut the connection down -- HTTP doesn't do
       half-open connections. */
    if(status != 0) {
        connection->reqlen = 0;
        httpConnectionDestroyReqbuf(connection);
        if(!(connection->flags & CONN_WRITER)) {
            connection->flags &= ~CONN_READER;
            if(status > 0 || status == -ECONNRESET || status == -EDOSHUTDOWN)
                httpClientFinish(connection, 2);
            else
                httpClientFinish(connection, 1);
            return 1;
        }
        httpClientAbort(connection, status > 0 || status == -ECONNRESET);
        connection->flags &= ~CONN_READER;
        return 1;
    }

    i = findEndOfHeaders(connection->reqbuf, 0, request->offset, &body);
    connection->reqlen = request->offset;

    if(i >= 0) {
        connection->reqbegin = i;
        httpClientHandlerHeaders(event, request, connection);
        return 1;
    }

    if(connection->reqlen >= bufsize) {
        int rc = 0;
        if(!(connection->flags & CONN_BIGREQBUF))
            rc = httpConnectionBigifyReqbuf(connection);
        if((connection->flags & CONN_BIGREQBUF) &&
           connection->reqlen < bigBufferSize) {
            do_stream(IO_READ, connection->fd, connection->reqlen,
                      connection->reqbuf, bigBufferSize,
                      httpClientHandler, connection);
            return 1;
        }
        connection->reqlen = 0;
        httpConnectionDestroyReqbuf(connection);
        if(rc < 0) {
            do_log(L_ERROR, "Couldn't allocate big buffer.\n");
            httpClientNewError(connection, METHOD_UNKNOWN, 0, 400, 
                               internAtom("Couldn't allocate big buffer"));
        } else {
            do_log(L_ERROR, "Couldn't find end of client's headers.\n");
            httpClientNewError(connection, METHOD_UNKNOWN, 0, 400, 
                               internAtom("Couldn't find end of headers"));
        }
        return 1;
    }
    httpSetTimeout(connection, clientTimeout);
    return 0;
}

int
httpClientRawErrorHeaders(HTTPConnectionPtr connection,
                          int code, AtomPtr message,
                          int close, AtomPtr headers)
{
    int fd = connection->fd;
    int n;
    char *url; int url_len;
    char *etag;

    assert(connection->flags & CONN_WRITER);
    assert(code != 0);

    if(close >= 0) {
        if(connection->request)
            close = 
                close || !(connection->request->flags & REQUEST_PERSISTENT);
        else
            close = 1;
    }
    if(connection->request && connection->request->object) {
        url = connection->request->object->key;
        url_len = connection->request->object->key_size;
        etag = connection->request->object->etag;
    } else {
        url = NULL;
        url_len = 0;
        etag = NULL;
    }

    if(connection->buf == NULL) {
        connection->buf = get_chunk();
        if(connection->buf == NULL) {
            httpClientFinish(connection, 1);
            return 1;
        }
    }

    n = httpWriteErrorHeaders(connection->buf, CHUNK_SIZE, 0,
                              connection->request &&
                              connection->request->method != METHOD_HEAD,
                              code, message, close > 0, headers,
                              url, url_len, etag);
    if(n <= 0) {
        shutdown(connection->fd, 1);
        if(close >= 0)
            httpClientFinish(connection, 1);
        return 1;
    }

    httpSetTimeout(connection, clientTimeout);
    do_stream(IO_WRITE, fd, 0, connection->buf, n, 
              close > 0 ? httpErrorStreamHandler :
              close == 0 ? httpErrorNocloseStreamHandler :
              httpErrorNofinishStreamHandler,
              connection);

    return 1;
}

int
httpClientRawError(HTTPConnectionPtr connection, int code, AtomPtr message,
                   int close)
{
    return httpClientRawErrorHeaders(connection, code, message, close, NULL);
}

int
httpClientNoticeErrorHeaders(HTTPRequestPtr request, int code, AtomPtr message,
                             AtomPtr headers)
{
    if(request->error_message)
        releaseAtom(request->error_message);
    if(request->error_headers)
        releaseAtom(request->error_headers);
    request->error_code = code;
    request->error_message = message;
    request->error_headers = headers;
    httpClientNoticeRequest(request, 0);
    return 1;
}

int
httpClientNoticeError(HTTPRequestPtr request, int code, AtomPtr message)
{
    return httpClientNoticeErrorHeaders(request, code, message, NULL);
}

int
httpClientError(HTTPRequestPtr request, int code, AtomPtr message)
{
    if(request->error_message)
        releaseAtom(request->error_message);
    request->error_code = code;
    request->error_message = message;
    if(request->chandler) {
        abortConditionHandler(request->chandler);
        request->chandler = NULL;
    } else if(request->object)
        notifyObject(request->object);
    return 1;
}

/* This may be called from object handlers. */
int
httpClientLeanError(HTTPRequestPtr request, int code, AtomPtr message)
{
    if(request->error_message)
        releaseAtom(request->error_message);
    request->error_code = code;
    request->error_message = message;
    return 1;
}


int
httpClientNewError(HTTPConnectionPtr connection, int method, int persist,
                   int code, AtomPtr message)
{
    HTTPRequestPtr request;
    request = httpMakeRequest();
    if(request == NULL) {
        do_log(L_ERROR, "Couldn't allocate error request.\n");
        httpClientFinish(connection, 1);
        return 1;
    }
    request->method = method;
    if(persist)
        request->flags |= REQUEST_PERSISTENT;
    else
        request->flags &= ~REQUEST_PERSISTENT;
    request->error_code = code;
    request->error_message = message;

    httpQueueRequest(connection, request);
    httpClientNoticeRequest(request, 0);
    return 1;
}
        
int
httpErrorStreamHandler(int status,
                       FdEventHandlerPtr event,
                       StreamRequestPtr srequest)
{
    HTTPConnectionPtr connection = srequest->data;

    if(status == 0 && !streamRequestDone(srequest))
        return 0;

    httpClientFinish(connection, 1);
    return 1;
}

int
httpErrorNocloseStreamHandler(int status,
                              FdEventHandlerPtr event,
                              StreamRequestPtr srequest)
{
    HTTPConnectionPtr connection = srequest->data;

    if(status == 0 && !streamRequestDone(srequest))
        return 0;

    httpClientFinish(connection, 0);
    return 1;
}

int
httpErrorNofinishStreamHandler(int status,
                               FdEventHandlerPtr event,
                               StreamRequestPtr srequest)
{
    if(status == 0 && !streamRequestDone(srequest))
        return 0;

    return 1;
}

int
httpClientHandlerHeaders(FdEventHandlerPtr event, StreamRequestPtr srequest,
                         HTTPConnectionPtr connection)
{
    HTTPRequestPtr request;
    int rc;
    int method, version;
    AtomPtr url = NULL;
    int start;
    int code;
    AtomPtr message;

    start = 0;
    /* Work around clients working around NCSA lossage. */
    if(connection->reqbuf[0] == '\n')
        start = 1;
    else if(connection->reqbuf[0] == '\r' && connection->reqbuf[1] == '\n')
        start = 2;

    httpSetTimeout(connection, -1);
    rc = httpParseClientFirstLine(connection->reqbuf, start,
                                  &method, &url, &version);
    if(rc <= 0) {
        do_log(L_ERROR, "Couldn't parse client's request line\n");
        code = 400;
        message =  internAtom("Error in request line");
        goto fail;
    }

    do_log(D_CLIENT_REQ, "Client request: ");
    do_log_n(D_CLIENT_REQ, connection->reqbuf, rc - 1);
    do_log(D_CLIENT_REQ, "\n");

    if(version != HTTP_10 && version != HTTP_11) {
        do_log(L_ERROR, "Unknown client HTTP version\n");
        code = 400;
        message = internAtom("Error in first request line");
        goto fail;
    }

    if(method == METHOD_UNKNOWN) {
        code = 501;
        message =  internAtom("Method not implemented");
        goto fail;
    }

    request = httpMakeRequest();
    if(request == NULL) {
        do_log(L_ERROR, "Couldn't allocate client request.\n");
        code = 500;
        message = internAtom("Couldn't allocate client request");
        goto fail;
    }

    if(connection->version != HTTP_UNKNOWN && version != connection->version) {
        do_log(L_WARN, "Client version changed!\n");
    }

    connection->version = version;
    request->flags = REQUEST_PERSISTENT;
    request->method = method;
    request->cache_control = no_cache_control;
    httpQueueRequest(connection, request);
    connection->reqbegin = rc;
    return httpClientRequest(request, url);

 fail:
    if(url) releaseAtom(url);
    shutdown(connection->fd, 0);
    connection->reqlen = 0;
    connection->reqbegin = 0;
    httpConnectionDestroyReqbuf(connection);
    connection->flags &= ~CONN_READER;
    httpClientNewError(connection, METHOD_UNKNOWN, 0, code, message);
    return 1;

}

static int
httpClientRequestDelayed(TimeEventHandlerPtr event)
{
    HTTPRequestPtr request = *(HTTPRequestPtr*)event->data;
    AtomPtr url;
    url = internAtomN(request->object->key, request->object->key_size);
    if(url == NULL) {
        do_log(L_ERROR, "Couldn't allocate url.\n");
        abortObject(request->object, 503, internAtom("Couldn't allocate url"));
        return 1;
    }
    httpClientRequest(request, url);
    return 1;
}

int
delayedHttpClientRequest(HTTPRequestPtr request)
{
    TimeEventHandlerPtr event;
    event = scheduleTimeEvent(-1, httpClientRequestDelayed,
                              sizeof(request), &request);
    if(!event)
        return -1;
    return 1;
}

int
httpClientRequest(HTTPRequestPtr request, AtomPtr url)
{
    HTTPConnectionPtr connection = request->connection;
    int i, rc;
    int body_len, body_te;
    AtomPtr headers;
    CacheControlRec cache_control;
    AtomPtr via, expect, auth;
    HTTPConditionPtr condition;
    HTTPRangeRec range;

    assert(!request->chandler);
    assert(connection->reqbuf);

    i = httpParseHeaders(1, url,
                         connection->reqbuf, connection->reqbegin, request,
                         &headers, &body_len, 
                         &cache_control, &condition, &body_te,
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                         &expect, &range, NULL, NULL, &via, &auth);
    if(i < 0) {
        releaseAtom(url);
        do_log(L_ERROR, "Couldn't parse client headers.\n");
        shutdown(connection->fd, 0);
        request->flags &= ~REQUEST_PERSISTENT;
        connection->flags &= ~CONN_READER;
        httpClientNoticeError(request, 503,
                              internAtom("Couldn't parse client headers"));
        return 1;
    }

    connection->reqbegin = i;

    if(body_len < 0) {
        if(request->method == METHOD_GET || request->method == METHOD_HEAD)
            body_len = 0;
    }
    connection->bodylen = body_len;
    connection->reqte = body_te;

    if(authRealm) {
        AtomPtr message = NULL;
        AtomPtr challenge = NULL;
        int code = checkClientAuth(auth, url, &message, &challenge);
        if(auth) {
            releaseAtom(auth);
            auth = NULL;
        }
        if(expect) {
            releaseAtom(expect);
            expect = NULL;
        }
        if(code) {
            request->flags |= REQUEST_FORCE_ERROR;
            httpClientDiscardBody(connection);
            httpClientNoticeErrorHeaders(request, code, message, challenge);
            return 1;
        }
    }

    if(auth) {
        releaseAtom(auth);
        auth = NULL;
    }

    if(expect) {
        if(expect == atom100Continue && REQUEST_SIDE(request)) {
            request->flags |= REQUEST_WAIT_CONTINUE;
        } else {
            httpClientDiscardBody(connection);
            httpClientNoticeError(request, 417,
                                  internAtom("Expectation failed"));
            releaseAtom(expect);
            return 1;
        }
        releaseAtom(expect);
    }

    request->from = range.from < 0 ? 0 : range.from;
    request->to = range.to;
    request->cache_control = cache_control;
    request->via = via;
    request->headers = headers;
    request->condition = condition;
    request->object = NULL;

    if(connection->serviced > 500)
        request->flags &= ~REQUEST_PERSISTENT;

    if(request->method == METHOD_CONNECT) {
        if(connection->flags & CONN_WRITER) {
            /* For now */
            httpClientDiscardBody(connection);
            httpClientNoticeError(request, 500,
                                  internAtom("Pipelined CONNECT "
                                             "not supported"));
            return 1;
        }
        if(connection->flags & CONN_BIGREQBUF) {
            /* For now */
            httpClientDiscardBody(connection);
            httpClientNoticeError(request, 500,
                                  internAtom("CONNECT over big buffer "
                                             "not supported"));
            return 1;
        }
        connection->flags &= ~CONN_READER;
        do_tunnel(connection->fd, connection->reqbuf, 
                  connection->reqbegin, connection->reqlen, url);
        connection->fd = -1;
        connection->reqbuf = NULL;
        connection->reqlen = 0;
        connection->reqbegin = 0;
        httpClientFinish(connection, 2);
        return 1;
    }

    rc = urlForbidden(url, httpClientRequestContinue, request);
    if(rc < 0) {
        do_log(L_ERROR, "Couldn't schedule httpClientRequestContinue.\n");
        httpClientDiscardBody(connection);
        httpClientNoticeError(request, 500,
                              internAtom("Couldn't schedule "
                                         "httpClientRequestContinue"));
        return 1;
    }
    return 1;
}

int
httpClientRequestContinue(int forbidden_code, AtomPtr url,
                          AtomPtr forbidden_message, AtomPtr forbidden_headers,
                          void *closure)
{
    HTTPRequestPtr request = (HTTPRequestPtr)closure;
    HTTPConnectionPtr connection = request->connection;
    RequestFunction requestfn;
    ObjectPtr object = NULL;

    if(forbidden_code < 0) {
        releaseAtom(url);
        httpClientDiscardBody(connection);
        httpClientNoticeError(request, 500, 
                              internAtomError(-forbidden_code,
                                              "Couldn't test for forbidden "
                                              "URL"));
        return 1;
    }

    if(forbidden_code) {
        releaseAtom(url);
        httpClientDiscardBody(connection);
        httpClientNoticeErrorHeaders(request,
                                     forbidden_code, forbidden_message,
                                     forbidden_headers);
        return 1;
    }

    requestfn = 
        urlIsLocal(url->string, url->length) ? 
        httpLocalRequest :
        httpServerRequest;

    if(request->method == METHOD_POST || request->method == METHOD_PUT) {
        do {
            object = findObject(OBJECT_HTTP, url->string, url->length);
            if(object) {
                privatiseObject(object, 0);
                releaseObject(object);
            }
        } while(object);
        request->object = makeObject(OBJECT_HTTP, url->string, url->length,
                                     0, 0, requestfn, NULL);
        if(request->object == NULL) {
            httpClientDiscardBody(connection);
            httpClientNoticeError(request, 503,
                                  internAtom("Couldn't allocate object"));
            return 1;
        }
        if(requestfn == httpLocalRequest)
            request->object->flags |= OBJECT_LOCAL;
        return httpClientSideRequest(request);
    }

    if(request->cache_control.flags & CACHE_AUTHORIZATION) {
        do {
            object = makeObject(OBJECT_HTTP, url->string, url->length, 0, 0,
                                requestfn, NULL);
            if(object && object->flags != OBJECT_INITIAL) {
                if(!(object->cache_control & CACHE_PUBLIC)) {
                    privatiseObject(object, 0);
                    releaseObject(object);
                    object = NULL;
                } else
                    break;
            }
        } while(object == NULL);
        if(object)
            object->flags |= OBJECT_LINEAR;
    } else {
        object = findObject(OBJECT_HTTP, url->string, url->length);
        if(!object)
            object = makeObject(OBJECT_HTTP, url->string, url->length, 1, 1,
                                requestfn, NULL);
    }
    releaseAtom(url);
    url = NULL;

    if(!object) {
        do_log(L_ERROR, "Couldn't allocate object.\n");
        httpClientDiscardBody(connection);
        httpClientNoticeError(request, 503,
                              internAtom("Couldn't allocate object"));
        return 1;
    }

    if(object->request == httpLocalRequest) {
        object->flags |= OBJECT_LOCAL;
    } else {
        if(disableProxy) {
            httpClientDiscardBody(connection);
            httpClientNoticeError(request, 403,
                                  internAtom("Proxying disabled"));
            releaseObject(object);
            return 1;
        }

        if(!checkVia(proxyName, request->via)) {
            httpClientDiscardBody(connection);
            httpClientNoticeError(request, 504, 
                                  internAtom("Proxy loop detected"));
            releaseObject(object);
            return 1;
        }
    }

    request->object = object;

    httpClientDiscardBody(connection);
    httpClientNoticeRequest(request, 0);
    return 1;
}

static int httpClientDelayed(TimeEventHandlerPtr handler);

int
httpClientDiscardBody(HTTPConnectionPtr connection)
{
    TimeEventHandlerPtr handler;

    assert(connection->reqoffset == 0);
    assert(connection->flags & CONN_READER);

    if(connection->reqte != TE_IDENTITY)
        goto fail;

    if(connection->bodylen < 0)
        goto fail;

    if(connection->bodylen < connection->reqlen - connection->reqbegin) {
        connection->reqbegin += connection->bodylen;
        connection->bodylen = 0;
    } else {
        connection->bodylen -= connection->reqlen - connection->reqbegin;
        connection->reqbegin = 0;
        connection->reqlen = 0;
        httpConnectionDestroyReqbuf(connection);
    }
    connection->reqte = TE_UNKNOWN;

    if(connection->bodylen > 0) {
        httpSetTimeout(connection, clientTimeout);
        do_stream_buf(IO_READ | IO_NOTNOW,
                      connection->fd, connection->reqlen,
                      &connection->reqbuf, CHUNK_SIZE,
                      httpClientDiscardHandler, connection);
        return 1;
    }

    if(connection->reqlen > connection->reqbegin) {
        memmove(connection->reqbuf, connection->reqbuf + connection->reqbegin,
                connection->reqlen - connection->reqbegin);
        connection->reqlen -= connection->reqbegin;
        connection->reqbegin = 0;
    } else {
        connection->reqlen = 0;
        connection->reqbegin = 0;
    }

    httpSetTimeout(connection, clientTimeout);
    /* We need to delay in order to make sure the previous request
       gets queued on the server side.  IO_NOTNOW isn't strong enough
       for that due to IO_IMMEDIATE. */
    handler = scheduleTimeEvent(-1, httpClientDelayed,
                                sizeof(connection), &connection);
    if(handler == NULL) {
        do_log(L_ERROR, "Couldn't schedule reading from client.");
        goto fail;
    }
    return 1;

 fail:
    connection->reqlen = 0;
    connection->reqbegin = 0;
    connection->bodylen = 0;
    connection->reqte = TE_UNKNOWN;
    shutdown(connection->fd, 2);
    handler = scheduleTimeEvent(-1, httpClientDelayed,
                                sizeof(connection), &connection);
    if(handler == NULL) {
        do_log(L_ERROR, "Couldn't schedule reading from client.");
        connection->flags &= ~CONN_READER;
    }
    return 1;
}

static int
httpClientDelayed(TimeEventHandlerPtr event)
{
     HTTPConnectionPtr connection = *(HTTPConnectionPtr*)event->data;

     /* IO_NOTNOW is unfortunate, but needed to avoid starvation if a
        client is pipelining a lot of requests. */
     if(connection->reqlen > 0) {
         int bufsize;
         if((connection->flags & CONN_BIGREQBUF) &&
            connection->reqlen < CHUNK_SIZE)
             httpConnectionUnbigifyReqbuf(connection);
         /* Don't read new requests if buffer is big. */
         bufsize = (connection->flags & CONN_BIGREQBUF) ?
             connection->reqlen : CHUNK_SIZE;
         do_stream(IO_READ | IO_IMMEDIATE | IO_NOTNOW,
                   connection->fd, connection->reqlen,
                   connection->reqbuf, bufsize,
                   httpClientHandler, connection);
     } else {
         httpConnectionDestroyReqbuf(connection);
         do_stream_buf(IO_READ | IO_NOTNOW,
                       connection->fd, 0,
                       &connection->reqbuf, CHUNK_SIZE,
                       httpClientHandler, connection);
     }
     return 1;
}

int
httpClientDiscardHandler(int status,
                         FdEventHandlerPtr event, StreamRequestPtr request)
{
    HTTPConnectionPtr connection = request->data;

    assert(connection->flags & CONN_READER);
    if(status) {
        if(status < 0 && status != -EPIPE && status != -ECONNRESET)
            do_log_error(L_ERROR, -status, "Couldn't read from client");
        connection->bodylen = -1;
        return httpClientDiscardBody(connection);
    }

    assert(request->offset > connection->reqlen);
    connection->reqlen = request->offset;

    httpClientDiscardBody(connection);
    return 1;
}

int
httpClientNoticeRequest(HTTPRequestPtr request, int novalidate)
{
    HTTPConnectionPtr connection = request->connection;
    ObjectPtr object = request->object;
    int serveNow = (request == connection->request);
    int validate = 0;
    int conditional = 0;
    int local, haveData;
    int rc;

    assert(!request->chandler);

    if(request->error_code) {
        if((request->flags & REQUEST_FORCE_ERROR) || REQUEST_SIDE(request) ||
           request->object == NULL ||
           (request->object->flags & OBJECT_LOCAL) ||
           (request->object->flags & OBJECT_ABORTED) ||
           (relaxTransparency < 1 && !proxyOffline)) {
            if(serveNow) {
                connection->flags |= CONN_WRITER;
                return httpClientRawErrorHeaders(connection,
                                                 request->error_code, 
                                                 retainAtom(request->
                                                            error_message),
                                                 0, request->error_headers);
            } else {
                return 1;
            }
        }
    }

    if(REQUEST_SIDE(request)) {
        assert(!(request->flags & REQUEST_REQUESTED));
        if(serveNow) {
            assert(!request->chandler);
            request->chandler =
                conditionWait(&request->object->condition, 
                              httpClientGetHandler,
                              sizeof(request), &request);
            if(request->chandler == NULL) {
                do_log(L_ERROR, "Couldn't register condition handler.\n");
                connection->flags |= CONN_WRITER;
                httpClientRawError(connection, 500,
                                   internAtom("Couldn't register "
                                              "condition handler"),
                                   0);
                return 1;
            }
            connection->flags |= CONN_WRITER;
            rc = object->request(request->object,
                                 request->method,
                                 request->from, request->to, 
                                 request,
                                 request->object->request_closure);
        }
        return 1;
    }

    local = urlIsLocal(object->key, object->key_size);
    objectFillFromDisk(object, request->from,
                       request->method == METHOD_HEAD ? 0 : 1);

    /* The spec doesn't strictly forbid 206 for non-200 instances, but doing
       that breaks some client software. */
    if(object->code && object->code != 200) {
        request->from = 0;
        request->to = -1;
    }

    if(request->condition && request->condition->ifrange) {
        if(!object->etag || 
           strcmp(object->etag, request->condition->ifrange) != 0) {
            request->from = 0;
            request->to = -1;
        }
    }

    if(object->flags & OBJECT_DYNAMIC) {
        request->from = 0;
        request->to = -1;
    }

    if(request->method == METHOD_HEAD)
        haveData = !(request->object->flags & OBJECT_INITIAL);
    else
        haveData = 
            (request->object->length >= 0 && 
             request->object->length <= request->from) ||
            (objectHoleSize(request->object, request->from) == 0);

    if(request->flags & REQUEST_REQUESTED)
        validate = 0;
    else if(novalidate || (!local && proxyOffline))
        validate = 0;
    else if(local)
        validate = 
            objectMustRevalidate(request->object, &request->cache_control);
    else if(request->cache_control.flags & CACHE_ONLY_IF_CACHED)
        validate = 0;
    else if((request->object->flags & OBJECT_FAILED) &&
            !(object->flags & OBJECT_INPROGRESS) &&
            !relaxTransparency)
        validate = 1;
    else if(request->method != METHOD_HEAD &&
            !objectHasData(object, request->from, request->to) &&
            !(object->flags & OBJECT_INPROGRESS))
        validate = 1;
    else if(objectMustRevalidate((relaxTransparency <= 1 ? 
                                  request->object : NULL),
                                 &request->cache_control))
        validate = 1;
    else
        validate = 0;

    if(request->cache_control.flags & CACHE_ONLY_IF_CACHED) {
        validate = 0;
        if(!haveData) {
            if(serveNow) {
                connection->flags |= CONN_WRITER;
                return httpClientRawError(connection, 504,
                                          internAtom("Object not in cache"),
                                          0);
            } else
                return 1;
        }
    }

    if(!(request->object->flags & OBJECT_VALIDATING) &&
       ((!validate && haveData) ||
        (request->object->flags & OBJECT_FAILED))) {
        if(serveNow) {
            connection->flags |= CONN_WRITER;
            lockChunk(request->object, request->from / CHUNK_SIZE);
            return httpServeObject(connection);
        } else {
            return 1;
        }
    }

    if((request->flags & REQUEST_REQUESTED) &&
       !(request->object->flags & OBJECT_INPROGRESS)) {
        /* This can happen either because the server side ran out of
           memory, or because it is using HEAD validation.  We mark
           the object to be fetched again. */
        request->flags &= ~REQUEST_REQUESTED;
    }

    if(serveNow) {
        connection->flags |= CONN_WRITER;
        if(!local && proxyOffline)
            return httpClientRawError(connection, 502, 
                                      internAtom("Disconnected operation "
                                                 "and object not in cache"),
                                      0);
        request->chandler =
            conditionWait(&request->object->condition, httpClientGetHandler, 
                          sizeof(request), &request);
        if(request->chandler == NULL) {
            do_log(L_ERROR, "Couldn't register condition handler.\n");
            return httpClientRawError(connection, 503,
                                      internAtom("Couldn't register "
                                                 "condition handler"), 0);
        }
    }

    if(request->object->flags & OBJECT_VALIDATING)
        return 1;

    conditional = (haveData && request->method == METHOD_GET);
    if(!mindlesslyCacheVary && (request->object->cache_control & CACHE_VARY))
        conditional = conditional && (request->object->etag != NULL);

    conditional =
        conditional && !(request->object->cache_control & CACHE_MISMATCH);

    if(!(request->object->flags & OBJECT_INPROGRESS))
        request->object->flags |= OBJECT_VALIDATING;
    rc = request->object->request(request->object,
                                  conditional ? METHOD_CONDITIONAL_GET : 
                                  request->method,
                                  request->from, request->to, request,
                                  request->object->request_closure);
    if(rc < 0) {
        if(request->chandler)
            unregisterConditionHandler(request->chandler);
        request->chandler = NULL;
        request->object->flags &= ~OBJECT_VALIDATING;
        request->object->flags |= OBJECT_FAILED;
        if(request->error_message)
            releaseAtom(request->error_message);
        request->error_code = 503;
        request->error_message = internAtom("Couldn't schedule get");
    }
    return 1;
}

static int
httpClientNoticeRequestDelayed(TimeEventHandlerPtr event)
{
    HTTPRequestPtr request = *(HTTPRequestPtr*)event->data;
    httpClientNoticeRequest(request, 0);
    return 1;
}

int
delayedHttpClientNoticeRequest(HTTPRequestPtr request)
{
    TimeEventHandlerPtr event;
    event = scheduleTimeEvent(-1, httpClientNoticeRequestDelayed,
                              sizeof(request), &request);
    if(!event)
        return -1;
    return 1;
}

int
httpClientContinueDelayed(TimeEventHandlerPtr event)
{
    static char httpContinue[] = "HTTP/1.1 100 Continue\r\n\r\n";
    HTTPConnectionPtr connection = *(HTTPConnectionPtr*)event->data;

    do_stream(IO_WRITE, connection->fd, 0, httpContinue, 25,
              httpErrorNofinishStreamHandler, connection);
    return 1;
}

int
delayedHttpClientContinue(HTTPConnectionPtr connection)
{
    TimeEventHandlerPtr event;
    event = scheduleTimeEvent(-1, httpClientContinueDelayed,
                              sizeof(connection), &connection);
    if(!event)
        return -1;
    return 1;
}

int
httpClientGetHandler(int status, ConditionHandlerPtr chandler)
{
    HTTPRequestPtr request = *(HTTPRequestPtr*)chandler->data;
    HTTPConnectionPtr connection = request->connection;
    ObjectPtr object = request->object;
    int rc;

    assert(request == connection->request);

    if(request->request) {
        assert(request->object->flags & OBJECT_INPROGRESS);
        assert(!request->request->object ||
               request->request->object == request->object);
    }

    if(status < 0) {
        object->flags &= ~OBJECT_VALIDATING; /* for now */
        if(request->request && request->request->request == request)
            httpServerClientReset(request->request);
        lockChunk(object, request->from / CHUNK_SIZE);
        request->chandler = NULL;
        rc = delayedHttpServeObject(connection);
        if(rc < 0) {
            unlockChunk(object, request->from / CHUNK_SIZE);
            do_log(L_ERROR, "Couldn't schedule serving.\n");
            abortObject(object, 503, internAtom("Couldn't schedule serving"));
        }
        return 1;
    }

    if(object->flags & OBJECT_VALIDATING)
        return 0;

    if(request->error_code) {
        lockChunk(object, request->from / CHUNK_SIZE);
        request->chandler = NULL;
        rc = delayedHttpServeObject(connection);
        if(rc < 0) {
            unlockChunk(object, request->from / CHUNK_SIZE);
            do_log(L_ERROR, "Couldn't schedule serving.\n");
            abortObject(object, 503, internAtom("Couldn't schedule serving"));
        }
        return 1;
    }

    if(request->flags & REQUEST_WAIT_CONTINUE) {
        if(request->request && 
           !(request->request->flags & REQUEST_WAIT_CONTINUE)) {
            request->flags &= ~REQUEST_WAIT_CONTINUE;
            delayedHttpClientContinue(connection);
        }
        return 0;
    }

    /* See httpServerHandlerHeaders */
    if((object->flags & OBJECT_SUPERSEDED) &&
       /* Avoid superseding loops. */
       !(request->flags & REQUEST_SUPERSEDED) &&
       request->request && request->request->can_mutate) {
        ObjectPtr new_object = retainObject(request->request->can_mutate);
        if(object->requestor == request) {
            if(new_object->requestor == NULL)
                new_object->requestor = request;
            object->requestor = NULL;
            /* Avoid superseding the same request more than once. */
            request->flags |= REQUEST_SUPERSEDED;
        }
        request->chandler = NULL;
        releaseObject(object);
        request->object = new_object;
        request->request->object = new_object;
        /* We're handling the wrong object now.  It's simpler to
           rebuild the whole data structure from scratch rather than
           trying to compensate. */
        rc = delayedHttpClientNoticeRequest(request);
        if(rc < 0) {
            do_log(L_ERROR, "Couldn't schedule noticing of request.");
            abortObject(object, 500,
                        internAtom("Couldn't schedule "
                                   "noticing of request"));
            /* We're probably out of memory.  What can we do? */
            shutdown(connection->fd, 1);
        }
        return 1;
    }

    if(object->requestor != request && !(object->flags & OBJECT_ABORTED)) {
        /* Make sure we don't serve an object that is stale for us
           unless we're the requestor. */
        if((object->flags & (OBJECT_LINEAR | OBJECT_MUTATING)) ||
           objectMustRevalidate(object, &request->cache_control)) {
           if(object->flags & OBJECT_INPROGRESS)
               return 0;
           rc = delayedHttpClientNoticeRequest(request);
           if(rc < 0) {
               do_log(L_ERROR, "Couldn't schedule noticing of request.");
               abortObject(object, 500,
                           internAtom("Couldn't schedule "
                                      "noticing of request"));
           } else {
               request->chandler = NULL;
               return 1;
           }
        }
    }

    if(object->flags & (OBJECT_INITIAL | OBJECT_VALIDATING)) {
        if(object->flags & (OBJECT_INPROGRESS | OBJECT_VALIDATING)) {
            return 0;
        } else if(object->flags & OBJECT_FAILED) {
            if(request->error_code)
                abortObject(object, 
                            request->error_code, 
                            retainAtom(request->error_message));
            else {
                abortObject(object, 500,
                            internAtom("Error message lost in transit"));
            }
        } else {
            /* The request was pruned by httpServerDiscardRequests */
            if(chandler == request->chandler) {
                int rc;
                request->chandler = NULL;
                rc = delayedHttpClientNoticeRequest(request);
                if(rc < 0)
                    abortObject(object, 500,
                                internAtom("Couldn't allocate "
                                           "delayed notice request"));
                else
                    return 1;
            } else {
                abortObject(object, 500,
                            internAtom("Wrong request pruned -- "
                                       "this shouldn't happen"));
            }
        }
    }

    if(request->object->flags & OBJECT_DYNAMIC) {
        if(objectHoleSize(request->object, 0) == 0) {
            request->from = 0;
            request->to = -1;
        } else {
            /* We really should request again if that is not the case */
        }
    }

    lockChunk(object, request->from / CHUNK_SIZE);
    request->chandler = NULL;
    rc = delayedHttpServeObject(connection);
    if(rc < 0) {
        unlockChunk(object, request->from / CHUNK_SIZE);
        do_log(L_ERROR, "Couldn't schedule serving.\n");
        abortObject(object, 503, internAtom("Couldn't schedule serving"));
    }
    return 1;
}

int
httpClientSideRequest(HTTPRequestPtr request)
{
    HTTPConnectionPtr connection = request->connection;

    if(request->from < 0 || request->to >= 0) {
        httpClientNoticeError(request, 501,
                              internAtom("Partial requests not implemented"));
        httpClientDiscardBody(connection);
        return 1;
    }
    if(connection->reqte != TE_IDENTITY) {
        httpClientNoticeError(request, 501,
                              internAtom("Chunked requests not implemented"));
        httpClientDiscardBody(connection);
        return 1;
    }
    if(connection->bodylen < 0) {
        httpClientNoticeError(request, 502,
                              internAtom("POST or PUT without "
                                         "Content-Length"));
        httpClientDiscardBody(connection);
        return 1;
    }
    if(connection->reqlen < 0) {
        httpClientNoticeError(request, 502,
                              internAtom("Incomplete POST or PUT"));
        httpClientDiscardBody(connection);
        return 1;
    }
        
    return httpClientNoticeRequest(request, 0);
}

int 
httpClientSideHandler(int status,
                      FdEventHandlerPtr event,
                      StreamRequestPtr srequest)
{
    HTTPConnectionPtr connection = srequest->data;
    HTTPRequestPtr request = connection->request;
    HTTPRequestPtr requestee;
    HTTPConnectionPtr server;
    int push;
    int code;
    AtomPtr message = NULL;

    assert(connection->flags & CONN_SIDE_READER);

    if((request->object->flags & OBJECT_ABORTED) || 
       !(request->object->flags & OBJECT_INPROGRESS)) {
        code = request->object->code;
        message = retainAtom(request->object->message);
        goto fail;
    }
        
    if(status < 0) {
        do_log_error(L_ERROR, -status, "Reading from client");
        code = 502;
        message = internAtomError(-status, "Couldn't read from client");
        goto fail;
    }

    requestee = request->request;
    server = requestee->connection;

    push = MIN(srequest->offset - connection->reqlen, 
               connection->bodylen - connection->reqoffset);
    if(push > 0) {
        connection->reqlen += push;
        httpServerDoSide(server);
        return 1;
    }

    if(server->reqoffset >= connection->bodylen) {
        connection->flags &= ~(CONN_READER | CONN_SIDE_READER);
        return 1;
    }

    assert(status);
    do_log(L_ERROR, "Incomplete client request.\n");
    code = 502;
    message = internAtom("Incomplete client request");

 fail:
    request->error_code = code;
    if(request->error_message)
        releaseAtom(request->error_message);
    request->error_message = message;
    if(request->error_headers)
        releaseAtom(request->error_headers);
    request->error_headers = NULL;

    if(request->request) {
        shutdown(request->request->connection->fd, 2);
        pokeFdEvent(request->request->connection->fd, -ESHUTDOWN, POLLOUT);
    }
    notifyObject(request->object);
    connection->flags &= ~CONN_SIDE_READER;
    httpClientDiscardBody(connection);
    return 1;
}

int 
httpServeObject(HTTPConnectionPtr connection)
{
    HTTPRequestPtr request = connection->request;
    ObjectPtr object = request->object;
    int i = request->from / CHUNK_SIZE;
    int j = request->from % CHUNK_SIZE;
    int n, len, rc;
    int bufsize = CHUNK_SIZE;
    int condition_result;

    object->atime = current_time.tv_sec;
    objectMetadataChanged(object, 0);

    httpSetTimeout(connection, -1);

    if((request->error_code && relaxTransparency <= 0) ||
       object->flags & OBJECT_INITIAL) {
        object->flags &= ~OBJECT_FAILED;
        unlockChunk(object, i);
        if(request->error_code)
            return httpClientRawError(connection,
                                      request->error_code, 
                                      retainAtom(request->error_message), 0);
        else
            return httpClientRawError(connection,
                                      500, internAtom("Object vanished."), 0);
    }

    if(!(object->flags & OBJECT_INPROGRESS) && object->code == 0) {
        if(object->flags & OBJECT_INITIAL) {
            unlockChunk(object, i);
            return httpClientRawError(connection, 503,
                                      internAtom("Error message lost"), 0);
                                      
        } else {
            unlockChunk(object, i);
            do_log(L_ERROR, "Internal proxy error: object has no code.\n");
            return httpClientRawError(connection, 500,
                                      internAtom("Internal proxy error: "
                                                 "object has no code"), 0);
        }
    }

    condition_result = httpCondition(object, request->condition);

    if(condition_result == CONDITION_FAILED) {
        unlockChunk(object, i);
        return httpClientRawError(connection, 412,
                                  internAtom("Precondition failed"), 0);
    } else if(condition_result == CONDITION_NOT_MODIFIED) {
        unlockChunk(object, i);
        return httpClientRawError(connection, 304,
                                  internAtom("Not modified"), 0);
    }

    objectFillFromDisk(object, request->from,
                       (request->method == METHOD_HEAD ||
                        condition_result != CONDITION_MATCH) ? 0 : 1);

    if(((object->flags & OBJECT_LINEAR) &&
        (object->requestor != connection->request)) ||
       ((object->flags & OBJECT_SUPERSEDED) &&
        !(object->flags & OBJECT_LINEAR))) {
        if(request->request) {
            request->request->request = NULL;
            request->request = NULL;
            request->object->requestor = NULL;
        }
        object = makeObject(OBJECT_HTTP,
                            object->key, object->key_size, 1, 0,
                            object->request, NULL);
        if(request->object->requestor == request)
            request->object->requestor = NULL;
        unlockChunk(request->object, i);
        releaseObject(request->object);
        request->object = NULL;
        if(object == NULL) {
            do_log(L_ERROR, "Couldn't allocate object.");
            return httpClientRawError(connection, 501,
                                      internAtom("Couldn't allocate object"), 
                                      1);
        }
        if(urlIsLocal(object->key, object->key_size)) {
            object->flags |= OBJECT_LOCAL;
            object->request = httpLocalRequest;
        }
        request->object = object;
        connection->flags &= ~CONN_WRITER;
        return httpClientNoticeRequest(request, 1);
    }

    if(object->flags & OBJECT_ABORTED) {
        unlockChunk(object, i);
        return httpClientNoticeError(request, object->code, 
                                     retainAtom(object->message));
    }

    if(connection->buf == NULL)
        connection->buf = get_chunk();
    if(connection->buf == NULL) {
        unlockChunk(object, i);
        do_log(L_ERROR, "Couldn't allocate client buffer.\n");
        connection->flags &= ~CONN_WRITER;
        httpClientFinish(connection, 1);
        return 1;
    }

    if(object->length >= 0 && request->to >= object->length)
        request->to = object->length;

    if(request->from > 0 || request->to >= 0) {
        if(request->method == METHOD_HEAD) {
            request->to = request->from;
        } else if(request->to < 0) {
            if(object->length >= 0)
                request->to = object->length;
        }
    }

 again:

    connection->len = 0;

    if((request->from <= 0 && request->to < 0) || 
       request->method == METHOD_HEAD) {
        n = snnprintf(connection->buf, 0, bufsize,
                      "HTTP/1.1 %d %s",
                      object->code, atomString(object->message));
    } else {
        if((object->length >= 0 && request->from >= object->length) ||
           (request->to >= 0 && request->from >= request->to)) {
            unlockChunk(object, i);
            return httpClientRawError(connection, 416,
                                      internAtom("Requested range "
                                                 "not satisfiable"),
                                      0);
        } else {
            n = snnprintf(connection->buf, 0, bufsize,
                          "HTTP/1.1 206 Partial content");
        }
    }

    n = httpWriteObjectHeaders(connection->buf, n, bufsize,
                               object, request->from, request->to);
    if(n < 0)
        goto fail;

    if(request->method != METHOD_HEAD && 
       condition_result != CONDITION_NOT_MODIFIED &&
       request->to < 0 && object->length < 0) {
        if(connection->version == HTTP_11) {
            connection->te = TE_CHUNKED;
            n = snnprintf(connection->buf, n, bufsize,
                          "\r\nTransfer-Encoding: chunked");
        } else {
            request->flags &= ~REQUEST_PERSISTENT;
        }
    }
        
    if(object->age < current_time.tv_sec) {
        n = snnprintf(connection->buf, n, bufsize,
                      "\r\nAge: %d",
                      (int)(current_time.tv_sec - object->age));
    }
    n = snnprintf(connection->buf, n, bufsize,
                  "\r\nConnection: %s",
                  (request->flags & REQUEST_PERSISTENT) ? 
                  "keep-alive" : "close");

    if(!(object->flags & OBJECT_LOCAL)) {
        if((object->flags & OBJECT_FAILED) && !proxyOffline) {
            n = snnprintf(connection->buf, n, bufsize,
                          "\r\nWarning: 111 %s:%d Revalidation failed",
                          proxyName->string, proxyPort);
            if(request->error_code)
                n = snnprintf(connection->buf, n, bufsize,
                              " (%d %s)",
                              request->error_code, 
                              atomString(request->error_message));
            object->flags &= ~OBJECT_FAILED;
        } else if(proxyOffline && 
                  objectMustRevalidate(object, &request->cache_control)) {
            n = snnprintf(connection->buf, n, bufsize,
                          "\r\nWarning: 112 %s:%d Disconnected operation",
                          proxyName->string, proxyPort);
        } else if(objectIsStale(object, &request->cache_control)) {
            n = snnprintf(connection->buf, n, bufsize,
                          "\r\nWarning: 110 %s:%d Object is stale",
                          proxyName->string, proxyPort);
        } else if(object->expires < 0 && object->max_age < 0 &&
                  object->age < current_time.tv_sec - 24 * 3600) {
            n = snnprintf(connection->buf, n, bufsize,
                          "\r\nWarning: 113 %s:%d Heuristic expiration",
                          proxyName->string, proxyPort);
        }
    }

    n = snnprintf(connection->buf, n, bufsize, "\r\n\r\n");
    
    if(n < 0)
        goto fail;
    
    connection->offset = request->from;

    if(request->method == METHOD_HEAD || 
       condition_result == CONDITION_NOT_MODIFIED ||
       (object->flags & OBJECT_ABORTED)) {
        len = 0;
    } else {
        if(i < object->numchunks) {
            if(object->chunks[i].size <= j)
                len = 0;
            else
                len = object->chunks[i].size - j;
        } else {
            len = 0;
        }
        if(request->to >= 0)
            len = MIN(len, request->to - request->from);
    }

    connection->offset = request->from;
    httpSetTimeout(connection, clientTimeout);
    do_log(D_CLIENT_DATA, "Serving on 0x%lx for 0x%lx: offset %d len %d\n",
           (unsigned long)connection, (unsigned long)object,
           connection->offset, len);
    do_stream_h(IO_WRITE |
                (connection->te == TE_CHUNKED && len > 0 ? IO_CHUNKED : 0),
                connection->fd, 0, 
                connection->buf, n,
                object->chunks[i].data + j, len,
                httpServeObjectStreamHandler, connection);
    return 1;

 fail:
    rc = 0;
    connection->len = 0;
    if(!(connection->flags & CONN_BIGBUF))
        rc = httpConnectionBigify(connection);
    if(rc > 0) {
        bufsize = bigBufferSize;
        goto again;
    }
    unlockChunk(object, i);
    return httpClientRawError(connection, 500,
                              rc == 0 ?
                              internAtom("No space for headers") :
                              internAtom("Couldn't allocate big buffer"), 0);
}

static int
httpServeObjectDelayed(TimeEventHandlerPtr event)
{
    HTTPConnectionPtr connection = *(HTTPConnectionPtr*)event->data;
    httpServeObject(connection);
    return 1;
}

int
delayedHttpServeObject(HTTPConnectionPtr connection)
{
    TimeEventHandlerPtr event;

    assert(connection->request->object->chunks[connection->request->from / 
                                               CHUNK_SIZE].locked > 0);

    event = scheduleTimeEvent(-1, httpServeObjectDelayed,
                              sizeof(connection), &connection);
    if(!event) return -1;
    return 1;
}

static int
httpServeObjectFinishHandler(int status,
                             FdEventHandlerPtr event,
                             StreamRequestPtr srequest)
{
    HTTPConnectionPtr connection = srequest->data;
    HTTPRequestPtr request = connection->request;

    (void)request;
    assert(!request->chandler);

    if(status == 0 && !streamRequestDone(srequest))
        return 0;

    httpSetTimeout(connection, -1);

    if(status < 0) {
        do_log(L_ERROR, "Couldn't terminate chunked reply\n");
        httpClientFinish(connection, 1);
    } else {
        httpClientFinish(connection, 0);
    }
    return 1;
}

int
httpServeChunk(HTTPConnectionPtr connection)
{
    HTTPRequestPtr request = connection->request;
    ObjectPtr object = request->object;
    int i = connection->offset / CHUNK_SIZE;
    int j = connection->offset - (i * CHUNK_SIZE);
    int to, len, len2, end;
    int rc;

    /* This must be called with chunk i locked. */
    assert(object->chunks[i].locked > 0);

    if(object->flags & OBJECT_ABORTED)
        goto fail;

    if(object->length >= 0 && request->to >= 0)
        to = MIN(request->to, object->length);
    else if(object->length >= 0)
        to = object->length;
    else if(request->to >= 0)
        to = request->to;
    else
        to = -1;

    len = 0;
    if(i < object->numchunks)
        len = object->chunks[i].size - j;

    if(request->method != METHOD_HEAD && 
       len < CHUNK_SIZE && connection->offset + len < to) {
        objectFillFromDisk(object, connection->offset + len, 2);
        len = object->chunks[i].size - j;
    }

    if(to >= 0)
        len = MIN(len, to - connection->offset);

    if(len <= 0) {
        if(to >= 0 && connection->offset >= to) {
            if(request->chandler) {
                unregisterConditionHandler(request->chandler);
                request->chandler = NULL;
            }
            unlockChunk(object, i);
            if(connection->te == TE_CHUNKED) {
                httpSetTimeout(connection, clientTimeout);
                do_stream(IO_WRITE | IO_CHUNKED | IO_END,
                          connection->fd, 0, NULL, 0,
                          httpServeObjectFinishHandler, connection);
            } else {
                httpClientFinish(connection,
                                 !(object->length >= 0 &&
                                   connection->offset >= object->length));
            }
            return 1;
        } else {
            if(!request->chandler) {
                request->chandler =
                    conditionWait(&object->condition, 
                                  httpServeObjectHandler,
                                  sizeof(connection), &connection);
                if(!request->chandler) {
                    do_log(L_ERROR, "Couldn't register condition handler\n");
                    goto fail;
                }
            }
            if(!(object->flags & OBJECT_INPROGRESS)) {
                if(object->flags & OBJECT_SUPERSEDED) {
                    goto fail;
                }
                if(REQUEST_SIDE(request)) goto fail;
                rc = object->request(object, request->method,
                                     connection->offset, -1, request,
                                     object->request_closure);
                if(rc <= 0) goto fail;
            }
            return 1;
        }
    } else {
        /* len > 0 */
        if(request->method != METHOD_HEAD)
            objectFillFromDisk(object, (i + 1) * CHUNK_SIZE, 1);
        if(request->chandler) {
            unregisterConditionHandler(request->chandler);
            request->chandler = NULL;
        }
        len2 = 0;
        if(j + len == CHUNK_SIZE && object->numchunks > i + 1) {
            len2 = object->chunks[i + 1].size;
            if(to >= 0)
                len2 = MIN(len2, to - (i + 1) * CHUNK_SIZE);
        }
        /* Lock early -- httpServerRequest may get_chunk */
        if(len2 > 0)
            lockChunk(object, i + 1);
        if(object->length >= 0 && 
           connection->offset + len + len2 == object->length)
            end = 1;
        else
            end = 0;
        /* Prefetch */
        if(!(object->flags & OBJECT_INPROGRESS) && !REQUEST_SIDE(request)) {
            if(object->chunks[i].size < CHUNK_SIZE &&
               to >= 0 && connection->offset + len + 1 < to)
                object->request(object, request->method,
                                connection->offset + len, -1, request,
                                object->request_closure);
            else if(i + 1 < object->numchunks &&
                    object->chunks[i + 1].size == 0 &&
                    to >= 0 && (i + 1) * CHUNK_SIZE + 1 < to)
                object->request(object, request->method,
                                (i + 1) * CHUNK_SIZE, -1, request,
                                object->request_closure);
        }
        if(len2 == 0) {
            httpSetTimeout(connection, clientTimeout);
            do_log(D_CLIENT_DATA, 
                   "Serving on 0x%lx for 0x%lx: offset %d len %d\n",
                   (unsigned long)connection, (unsigned long)object,
                   connection->offset, len);
            /* IO_NOTNOW in order to give other clients a chance to run. */
            do_stream(IO_WRITE | IO_NOTNOW |
                      (connection->te == TE_CHUNKED ? IO_CHUNKED : 0) |
                      (end ? IO_END : 0),
                      connection->fd, 0, 
                      object->chunks[i].data + j, len,
                      httpServeObjectStreamHandler, connection);
        } else {
            httpSetTimeout(connection, clientTimeout);
            do_log(D_CLIENT_DATA, 
                   "Serving on 0x%lx for 0x%lx: offset %d len %d + %d\n",
                   (unsigned long)connection, (unsigned long)object,
                   connection->offset, len, len2);
            do_stream_2(IO_WRITE | IO_NOTNOW |
                        (connection->te == TE_CHUNKED ? IO_CHUNKED : 0) |
                        (end ? IO_END : 0),
                        connection->fd, 0, 
                        object->chunks[i].data + j, len,
                        object->chunks[i + 1].data, len2,
                        httpServeObjectStreamHandler2, connection);
        }            
        return 1;
    }

    abort();

 fail:
    unlockChunk(object, i);
    if(request->chandler)
        unregisterConditionHandler(request->chandler);
    request->chandler = NULL;
    httpClientFinish(connection, 1);
    return 1;
}

static int
httpServeChunkDelayed(TimeEventHandlerPtr event)
{
    HTTPConnectionPtr connection = *(HTTPConnectionPtr*)event->data;
    httpServeChunk(connection);
    return 1;
}

int
delayedHttpServeChunk(HTTPConnectionPtr connection)
{
    TimeEventHandlerPtr event;
    event = scheduleTimeEvent(-1, httpServeChunkDelayed,
                              sizeof(connection), &connection);
    if(!event) return -1;
    return 1;
}

int
httpServeObjectHandler(int status, ConditionHandlerPtr chandler)
{
    HTTPConnectionPtr connection = *(HTTPConnectionPtr*)chandler->data;
    HTTPRequestPtr request = connection->request;
    int rc;

    if((request->object->flags & OBJECT_ABORTED) || status < 0) {
        shutdown(connection->fd, 1);
        httpSetTimeout(connection, 10);
        /* httpServeChunk will take care of the error. */
    }

    httpSetTimeout(connection, -1);

    request->chandler = NULL;
    rc = delayedHttpServeChunk(connection);
    if(rc < 0) {
        do_log(L_ERROR, "Couldn't schedule serving.\n");
        abortObject(request->object, 503, 
                    internAtom("Couldn't schedule serving"));
    }
    return 1;
}

static int
httpServeObjectStreamHandlerCommon(int kind, int status,
                                   FdEventHandlerPtr event,
                                   StreamRequestPtr srequest)
{
    HTTPConnectionPtr connection = srequest->data;
    HTTPRequestPtr request = connection->request;
    int condition_result = httpCondition(request->object, request->condition);
    int i = connection->offset / CHUNK_SIZE;

    assert(!request->chandler);

    if(status == 0 && !streamRequestDone(srequest)) {
        httpSetTimeout(connection, clientTimeout);
        return 0;
    }

    httpSetTimeout(connection, -1);

    unlockChunk(request->object, i);
    if(kind == 2)
        unlockChunk(request->object, i + 1);

    if(status) {
        if(status < 0) {
            do_log_error(status == -ECONNRESET ? D_IO : L_ERROR, 
                         -status, "Couldn't write to client");
            if(status == -EIO || status == -ESHUTDOWN)
                httpClientFinish(connection, 2);
            else
                httpClientFinish(connection, 1);
        } else {
            do_log(D_IO, "Couldn't write to client: short write.\n");
            httpClientFinish(connection, 2);
        }
        return 1;
    }

    if(srequest->operation & IO_CHUNKED) {
        assert(srequest->offset > 2);
        connection->offset += srequest->offset - 2;
    } else {
        connection->offset += srequest->offset;
    }
    request->flags &= ~REQUEST_REQUESTED;

    if(request->object->flags & OBJECT_ABORTED) {
        httpClientFinish(connection, 1);
        return 1;
    }

    if(connection->request->method == METHOD_HEAD ||
       condition_result == CONDITION_NOT_MODIFIED) {
        httpClientFinish(connection, 0);
        return 1;
    }

    if(srequest->operation & IO_END)
        httpClientFinish(connection, 0);
    else {
        httpConnectionDestroyBuf(connection);
        lockChunk(connection->request->object,
                  connection->offset / CHUNK_SIZE);
        httpServeChunk(connection);
    }
    return 1;
}

int
httpServeObjectStreamHandler(int status,
                             FdEventHandlerPtr event,
                             StreamRequestPtr srequest)
{
    return httpServeObjectStreamHandlerCommon(1, status, event, srequest);
}

int
httpServeObjectStreamHandler2(int status,
                              FdEventHandlerPtr event,
                              StreamRequestPtr srequest)
{
    return httpServeObjectStreamHandlerCommon(2, status, event, srequest);
}

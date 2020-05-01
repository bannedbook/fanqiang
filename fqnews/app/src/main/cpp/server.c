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

int serverExpireTime =  24 * 60 * 60;
int smallRequestTime = 10;
int replyUnpipelineTime = 20;
int replyUnpipelineSize = 1024 * 1024;
int pipelineAdditionalRequests = 1;
int maxPipelineTrain = 10;
AtomPtr parentProxy = NULL;
AtomPtr parentHost = NULL;
int parentPort = -1;
int pmmFirstSize = 0, pmmSize = 0;
int serverSlots = 2;
int serverSlots1 = 4;
int serverMaxSlots = 8;
int dontCacheRedirects = 0;
int maxSideBuffering = 1500;
int maxConnectionAge = 1260;
int maxConnectionRequests = 400;
int alwaysAddNoTransform = 0;

static HTTPServerPtr servers = 0;

static int httpServerContinueConditionHandler(int, ConditionHandlerPtr);
static int initParentProxy(void);
static int parentProxySetter(ConfigVariablePtr var, void *value);
static void httpServerDelayedFinish(HTTPConnectionPtr);
static int allowUnalignedRangeRequests = 0;

void
preinitServer(void)
{
    CONFIG_VARIABLE_SETTABLE(parentProxy, CONFIG_ATOM_LOWER, parentProxySetter,
                    "Parent proxy (host:port).");
    CONFIG_VARIABLE(serverExpireTime, CONFIG_TIME,
                    "Time during which server data is valid.");
    CONFIG_VARIABLE_SETTABLE(smallRequestTime, CONFIG_TIME, configIntSetter,
                             "Estimated time for a small request.");
    CONFIG_VARIABLE_SETTABLE(replyUnpipelineTime, CONFIG_TIME, configIntSetter,
                             "Estimated time for a pipeline break.");
    CONFIG_VARIABLE_SETTABLE(replyUnpipelineSize, CONFIG_INT, configIntSetter,
                    "Size for a pipeline break.");
    CONFIG_VARIABLE_SETTABLE(pipelineAdditionalRequests, CONFIG_TRISTATE,
                             configIntSetter,
                             "Pipeline requests on an active connection.");
    CONFIG_VARIABLE_SETTABLE(maxPipelineTrain, CONFIG_INT,
                             configIntSetter,
                             "Maximum number of requests "
                             "pipelined at a time.");
    CONFIG_VARIABLE(pmmFirstSize, CONFIG_INT,
                    "The size of the first PMM chunk.");
    CONFIG_VARIABLE(pmmSize, CONFIG_INT,
                    "The size of a PMM chunk.");
    CONFIG_VARIABLE(serverSlots, CONFIG_INT,
                    "Maximum number of connections per server.");
    CONFIG_VARIABLE(serverSlots1, CONFIG_INT,
                    "Maximum number of connections per HTTP/1.0 server.");
    CONFIG_VARIABLE(serverMaxSlots, CONFIG_INT,
                    "Maximum number of connections per broken server.");
    CONFIG_VARIABLE(dontCacheRedirects, CONFIG_BOOLEAN,
                    "If true, don't cache redirects.");
    CONFIG_VARIABLE_SETTABLE(allowUnalignedRangeRequests,
                             CONFIG_BOOLEAN, configIntSetter,
                             "Allow unaligned range requests (unreliable).");
    CONFIG_VARIABLE_SETTABLE(maxSideBuffering,
                             CONFIG_INT, configIntSetter,
                             "Maximum buffering for PUT and POST requests.");
    CONFIG_VARIABLE_SETTABLE(maxConnectionAge,
                             CONFIG_TIME, configIntSetter,
                             "Maximum age of a server-side connection.");
    CONFIG_VARIABLE_SETTABLE(maxConnectionRequests,
                             CONFIG_INT, configIntSetter,
                             "Maximum number of requests on a server-side connection.");
    CONFIG_VARIABLE(alwaysAddNoTransform, CONFIG_BOOLEAN,
                    "If true, add a no-transform directive to all requests.");
}

static int
parentProxySetter(ConfigVariablePtr var, void *value)
{
    configAtomSetter(var, value);
    initParentProxy();
    return 1;
}

static void
discardServer(HTTPServerPtr server)
{
    HTTPServerPtr previous;
    assert(!server->request);

    if(server == servers)
        servers = server->next;
    else {
        previous = servers;
        while(previous->next != server)
            previous = previous->next;
        previous->next = server->next;
    }

    if(server->connection)
        free(server->connection);
    if(server->idleHandler)
        free(server->idleHandler);
    if(server->name)
        free(server->name);

    free(server);
}

static int
httpServerIdle(HTTPServerPtr server)
{
    int i;
    if(server->request) 
        return 0;
    for(i = 0; i < server->maxslots; i++)
        if(server->connection[i])
            return 0;
    return 1;
}

static int
expireServersHandler(TimeEventHandlerPtr event)
{
    HTTPServerPtr server, next;
    TimeEventHandlerPtr e;
    server = servers;
    while(server) {
        next = server->next;
        if(httpServerIdle(server) &&
           server->time + serverExpireTime < current_time.tv_sec)
            discardServer(server);
        server = next;
    }
    e = scheduleTimeEvent(serverExpireTime / 60 + 60, 
                          expireServersHandler, 0, NULL);
    if(!e) {
        do_log(L_ERROR, "Couldn't schedule server expiry.\n");
        polipoExit();
    }
    return 1;
}

static int
roundSize(int size)
{
    return (size + CHUNK_SIZE - 1) / CHUNK_SIZE * CHUNK_SIZE;
}

static int
initParentProxy()
{
    AtomPtr host, port_atom;
    int rc, port;

    if(parentHost) {
        releaseAtom(parentHost);
        parentHost = NULL;
    }
    if(parentPort >= 0)
        parentPort = -1;

    if(parentProxy != NULL && parentProxy->length == 0) {
        releaseAtom(parentProxy);
        parentProxy = NULL;
    }

    if(parentProxy == NULL)
        return 1;

    rc = atomSplit(parentProxy, ':', &host, &port_atom);
    if(rc <= 0) {
        do_log(L_ERROR, "Couldn't parse parentProxy.");
        releaseAtom(parentProxy);
        parentProxy = NULL;
        return -1;
    }

    port = atoi(port_atom->string);
    if(port <= 0 || port >= 0x10000) {
        releaseAtom(host);
        releaseAtom(port_atom);
        do_log(L_ERROR, "Couldn't parse parentProxy.");
        releaseAtom(parentProxy);
        parentProxy = NULL;
        return -1;
    }

    parentHost = host;
    parentPort = port;
    return 1;
}

void
initServer(void)
{
    TimeEventHandlerPtr event;
    servers = NULL;

    if(pmmFirstSize || pmmSize) {
        if(pmmSize == 0) pmmSize = pmmFirstSize;
        if(pmmFirstSize == 0) pmmFirstSize = pmmSize;
        pmmSize = roundSize(pmmSize);
        pmmFirstSize = roundSize(pmmFirstSize);
    }

    if(serverMaxSlots < 1)
        serverMaxSlots = 1;
    if(serverSlots < 1)
        serverSlots = 1;
    if(serverSlots > serverMaxSlots)
        serverSlots = serverMaxSlots;
    if(serverSlots1 < serverSlots)
        serverSlots1 = serverSlots;
    if(serverSlots1 > serverMaxSlots)
        serverSlots1 = serverMaxSlots;

    initParentProxy();

    event = scheduleTimeEvent(serverExpireTime / 60 + 60, expireServersHandler,
                              0, NULL);
    if(event == NULL) {
        do_log(L_ERROR, "Couldn't schedule server expiry.\n");
        exit(1);
    }
}

static HTTPServerPtr
getServer(char *name, int port, int proxy)
{
    HTTPServerPtr server;
    int i;

    server = servers;
    while(server) {
        if(strcmp(server->name, name) == 0 && server->port == port &&
           server->isProxy == proxy) {
            if(httpServerIdle(server) &&
               server->time +  serverExpireTime < current_time.tv_sec) {
                discardServer(server);
                server = NULL;
                break;
            } else {
                server->time = current_time.tv_sec;
                return server;
            }
        }
        server = server->next;
    }
    
    server = malloc(sizeof(HTTPServerRec));
    if(server == NULL) {
        do_log(L_ERROR, "Couldn't allocate server.\n");
        return NULL;
    }

    server->connection = malloc(serverMaxSlots * sizeof(HTTPConnectionPtr));
    if(server->connection == NULL) {
        do_log(L_ERROR, "Couldn't allocate server.\n");
        free(server);
        return NULL;
    }

    server->idleHandler = malloc(serverMaxSlots * sizeof(FdEventHandlerPtr));
    if(server->connection == NULL) {
        do_log(L_ERROR, "Couldn't allocate server.\n");
        free(server->connection);
        free(server);
        return NULL;
    }

    server->maxslots = serverMaxSlots;

    server->name = strdup(name);
    if(server->name == NULL) {
        do_log(L_ERROR, "Couldn't allocate server name.\n");
        free(server);
        return NULL;
    }

    server->port = port;
    server->addrindex = 0;
    server->isProxy = proxy;
    server->version = HTTP_UNKNOWN;
    server->persistent = 0;
    server->pipeline = 0;
    server->time = current_time.tv_sec;
    server->rtt = -1;
    server->rate = -1;
    server->numslots = MIN(serverSlots, server->maxslots);
    for(i = 0; i < server->maxslots; i++) {
        server->connection[i] = NULL;
        server->idleHandler[i] = NULL;
    }
    server->request = NULL;
    server->request_last = NULL;
    server->lies = 0;

    server->next = servers;
    servers = server;
    return server;
}

int
httpServerQueueRequest(HTTPServerPtr server, HTTPRequestPtr request)
{
    assert(request->request && request->request->request == request);
    assert(request->connection == NULL);
    if(server->request) {
        server->request_last->next = request;
        server->request_last = request;
    } else {
        server->request_last = request;
        server->request = request;
    }
    return 1;
}

void
httpServerAbort(HTTPConnectionPtr connection, int fail,
                int code, AtomPtr message)
{
    HTTPRequestPtr request = connection->request;
    if(request) {
        if(request->request) {
            httpClientError(request->request, code, retainAtom(message));
        }
        if(fail) {
            request->object->flags |= OBJECT_FAILED;
            if(request->object->flags & OBJECT_INITIAL)
                abortObject(request->object, code, retainAtom(message));
            notifyObject(request->object);
        }
    }
    releaseAtom(message);
    if(!connection->connecting)
        httpServerFinish(connection, 1, 0);
}

void
httpServerAbortRequest(HTTPRequestPtr request, int fail,
                       int code, AtomPtr message)
{
    if(request->connection && request == request->connection->request) {
        httpServerAbort(request->connection, fail, code, message);
    } else {
        HTTPRequestPtr requestor = request->request;
        if(requestor) {
            requestor->request = NULL;
            request->request = NULL;
            httpClientError(requestor, code, retainAtom(message));
        }
        if(fail) {
            request->object->flags |= OBJECT_FAILED;
            if(request->object->flags & OBJECT_INITIAL)
                abortObject(request->object, code, retainAtom(message));
            notifyObject(request->object);
        }
        releaseAtom(message);
    }
}

void 
httpServerClientReset(HTTPRequestPtr request)
{
    if(request->connection && 
       request->connection->fd >= 0 &&
       !request->connection->connecting &&
       request->connection->request == request)
        pokeFdEvent(request->connection->fd, -ECLIENTRESET, POLLIN | POLLOUT);
}


int
httpMakeServerRequest(char *name, int port, ObjectPtr object, 
                  int method, int from, int to, HTTPRequestPtr requestor)
{
    HTTPServerPtr server;
    HTTPRequestPtr request;
    int rc;

    assert(!(object->flags & OBJECT_INPROGRESS));

    if(parentHost) {
        server = getServer(parentHost->string, parentPort, 1);
    } else {
        server = getServer(name, port, 0);
    }
    if(server == NULL) return -1;

    object->flags |= OBJECT_INPROGRESS;
    object->requestor = requestor;

    request = httpMakeRequest();
    if(!request) {
        do_log(L_ERROR, "Couldn't allocate request.\n");
        return -1;
    }

    /* Because we allocate objects in chunks, we cannot have data that
       doesn't start at a chunk boundary. */
    if(from % CHUNK_SIZE != 0) {
        if(allowUnalignedRangeRequests) {
            objectFillFromDisk(object, from / CHUNK_SIZE * CHUNK_SIZE, 1);
            if(objectHoleSize(object, from - 1) != 0)
                from = from / CHUNK_SIZE * CHUNK_SIZE;
        } else {
            from = from / CHUNK_SIZE * CHUNK_SIZE;
        }
    }

    request->object = retainObject(object);
    request->method = method;
    if(method == METHOD_CONDITIONAL_GET) {
        if(server->lies > 0)
            request->method = METHOD_HEAD;
    }
    request->flags =
        REQUEST_PERSISTENT |
        (expectContinue ? (requestor->flags & REQUEST_WAIT_CONTINUE) : 0);
    request->from = from;
    request->to = to;
    request->request = requestor;
    requestor->request = request;
    request->cache_control = requestor->cache_control;
    request->time0 = null_time;
    request->time1 = null_time;

    rc = httpServerQueueRequest(server, request);
    if(rc < 0) {
        do_log(L_ERROR, "Couldn't queue request.\n");
        request->request = NULL;
        requestor->request = NULL;
        object->flags &= ~(OBJECT_INPROGRESS | OBJECT_VALIDATING);
        releaseNotifyObject(object);
        httpDestroyRequest(request);
        return 1;
    }

    if(request->flags & REQUEST_WAIT_CONTINUE) {
        if(server->version == HTTP_10) {
            httpServerAbortRequest(request, 1,
                                   417, internAtom("Expectation failed"));
            return 1;
        }
    } else if(expectContinue >= 2 && server->version == HTTP_11) {
        if(request->method == METHOD_POST || request->method == METHOD_PUT)
            request->flags |= REQUEST_WAIT_CONTINUE;
    }
        
 again:
    rc = httpServerTrigger(server);
    if(rc < 0) {
        /* We must be very short on memory.  If there are any requests
           queued, we abort one and try again.  If there aren't, we
           give up. */
        do_log(L_ERROR, "Couldn't trigger server -- out of memory?\n");
        if(server->request) {
            httpServerAbortRequest(server->request, 1, 503,
                                   internAtom("Couldn't trigger server"));
            goto again;
        }
    }
    return 1;
}

int
httpServerConnection(HTTPServerPtr server)
{
    HTTPConnectionPtr connection;
    int i;

    connection = httpMakeConnection();
    if(connection == NULL) {
        do_log(L_ERROR, "Couldn't allocate server connection.\n");
        return -1;
    }
    connection->server = server;

    for(i = 0; i < server->numslots; i++) {
        if(!server->connection[i]) {
            server->connection[i] = connection;
            break;
        }
    }
    assert(i < server->numslots);
    
    connection->request = NULL;
    connection->request_last = NULL;

    do_log(D_SERVER_CONN, "C... %s:%d.\n",
           scrub(connection->server->name), connection->server->port);
    httpSetTimeout(connection, serverTimeout);
    if(socksParentProxy) {
        connection->connecting = CONNECTING_SOCKS;
        do_socks_connect(server->name, connection->server->port,
                         httpServerSocksHandler, connection);
    } else {
        connection->connecting = CONNECTING_DNS;
        do_gethostbyname(server->name, 0,
                         httpServerConnectionDnsHandler,
                         connection);
    }
    return 1;
}

int
httpServerConnectionDnsHandler(int status, GethostbynameRequestPtr request)
{
    HTTPConnectionPtr connection = request->data;

    httpSetTimeout(connection, -1);

    if(status <= 0) {
        AtomPtr message;
        message = internAtomF("Host %s lookup failed: %s",
                              request->name ?
                              request->name->string : "(unknown)",
                              request->error_message ?
                              request->error_message->string :
                              pstrerror(-status));
        do_log(L_ERROR, "Host %s lookup failed: %s (%d).\n", 
               request->name ?
               scrub(request->name->string) : "(unknown)",
               request->error_message ?
               request->error_message->string :
               pstrerror(-status), -status);
        connection->connecting = 0;
        if(connection->server->request)
            httpServerAbortRequest(connection->server->request, 1, 504,
                                   retainAtom(message));
        httpServerAbort(connection, 1, 502, message);
        return 1;
    }

    if(request->addr->string[0] == DNS_CNAME) {
        if(request->count > 10) {
            AtomPtr message = internAtom("DNS CNAME loop");
            do_log(L_ERROR, "DNS CNAME loop.\n");
            connection->connecting = 0;
            if(connection->server->request)
                httpServerAbortRequest(connection->server->request, 1, 504,
                                       retainAtom(message));
            httpServerAbort(connection, 1, 504, message);
            return 1;
        }
            
        httpSetTimeout(connection, serverTimeout);
        do_gethostbyname(request->addr->string + 1, request->count + 1,
                         httpServerConnectionDnsHandler,
                         connection);
        return 1;
    }

    connection->connecting = CONNECTING_CONNECT;
    httpSetTimeout(connection, serverTimeout);
    do_connect(retainAtom(request->addr), connection->server->addrindex,
               connection->server->port,
               httpServerConnectionHandler, connection);
    return 1;
}

int
httpServerConnectionHandler(int status,
                            FdEventHandlerPtr event,
                            ConnectRequestPtr request)
{
    HTTPConnectionPtr connection = request->data;

    assert(connection->fd < 0);
    if(request->fd >= 0) {
        int rc;
        connection->fd = request->fd;
        connection->server->addrindex = request->index;
        rc = setNodelay(connection->fd, 1);
        if(rc < 0)
            do_log_error(L_WARN, errno, "Couldn't disable Nagle's algorithm");
    }

    return httpServerConnectionHandlerCommon(status, connection);
}

int
httpServerSocksHandler(int status, SocksRequestPtr request)
{
    HTTPConnectionPtr connection = request->data;

    assert(connection->fd < 0);
    if(request->fd >= 0) {
        connection->fd = request->fd;
        connection->server->addrindex = 0;
    }
    return httpServerConnectionHandlerCommon(status, connection);
}

int
httpServerConnectionHandlerCommon(int status, HTTPConnectionPtr connection)
{
    httpSetTimeout(connection, -1);

    if(status < 0) {
        AtomPtr message = 
            internAtomError(-status, "Connect to %s:%d failed",
                            connection->server->name,
                            connection->server->port);
        if(status != -ECLIENTRESET)
            do_log_error(L_ERROR, -status, "Connect to %s:%d failed",
                         scrub(connection->server->name),
                         connection->server->port);
        connection->connecting = 0;
        if(connection->server->request)
            httpServerAbortRequest(connection->server->request,
                                   status != -ECLIENTRESET, 504, 
                                   retainAtom(message));
        httpServerAbort(connection, status != -ECLIENTRESET, 504, message);
        return 1;
    }

    do_log(D_SERVER_CONN, "C    %s:%d.\n",
           scrub(connection->server->name), connection->server->port);

    connection->connecting = 0;
    /* serverTrigger will take care of inserting any timeouts */
    httpServerTrigger(connection->server);
    return 1;
}

int
httpServerIdleHandler(int a, FdEventHandlerPtr event)
{
    HTTPConnectionPtr connection = *(HTTPConnectionPtr*)event->data;
    HTTPServerPtr server = connection->server;
    int i;

    assert(!connection->request);

    do_log(D_SERVER_CONN, "Idle connection to %s:%d died.\n", 
           scrub(connection->server->name), connection->server->port);

    for(i = 0; i < server->maxslots; i++) {
        if(connection == server->connection[i]) {
            server->idleHandler[i] = NULL;
            break;
        }
    }
    assert(i < server->maxslots);

    httpServerAbort(connection, 1, 504, internAtom("Timeout"));
    return 1;
}

/* Discard aborted requests at the head of the queue. */
static void
httpServerDiscardRequests(HTTPServerPtr server)
{
    HTTPRequestPtr request;
    while(server->request && !server->request->request) {
        request = server->request;
        server->request = request->next;
        request->next = NULL;
        if(server->request == NULL)
            server->request_last = NULL;
        request->object->flags &= ~(OBJECT_INPROGRESS | OBJECT_VALIDATING);
        releaseNotifyObject(request->object);
        request->object = NULL;
        httpDestroyRequest(request);
    }
}

static int
pipelineIsSmall(HTTPConnectionPtr connection)
{
    HTTPRequestPtr request = connection->request;

    if(pipelineAdditionalRequests <= 0)
        return 0;
    else if(pipelineAdditionalRequests >= 2)
        return 1;

    if(!request)
        return 1;
    if(request->next || !(request->flags & REQUEST_PERSISTENT))
        return 0;
    if(request->method == METHOD_HEAD || 
       request->method == METHOD_CONDITIONAL_GET)
        return 1;
    if(request->to >= 0 && connection->server->rate > 0 &&
       request->to - request->from < connection->server->rate * 
       smallRequestTime)
        return 1;
    return 0;
}

static int
numRequests(HTTPServerPtr server)
{
    int n = 0;
    HTTPRequestPtr request = server->request;
    while(request) {
        n++;
        request = request->next;
    }
    return n;
}

HTTPConnectionPtr
httpServerGetConnection(HTTPServerPtr server, int *idle_return)
{
    int i, j;
    int connecting = 0, empty = 0, idle = 0;

    j = -1;
    /* Try to find an idle connection */
    for(i = 0; i < server->numslots; i++) {
        if(server->connection[i]) {
            if(!server->connection[i]->connecting) {
                if(!server->connection[i]->request) {
                    if(server->idleHandler[i])
                        unregisterFdEvent(server->idleHandler[i]);
                    server->idleHandler[i] = NULL;
                    if(j < 0) j = i;
                    idle++;
                }
            } else
                connecting++;
        } else
            empty++;
    }

    if(j >= 0) {
        *idle_return = idle;
        return server->connection[j];
    }

    /* If there's an empty slot, schedule connection creation */
    if(empty) {
        /* Don't open a connection if there are already enough in
           progress, except if the server doesn't do persistent
           connections and there's only one in progress. */
        if((connecting == 0 || (server->persistent <= 0 && connecting <= 1)) ||
           connecting < numRequests(server)) {
            httpServerConnection(server);
        }
    }

    /* Find a connection that can accept additional requests */
    if(server->version == HTTP_11 && server->pipeline >= 4) {
        for(i = 0; i < serverSlots; i++) {
            if(server->connection[i] && !server->connection[i]->connecting &&
               pipelineIsSmall(server->connection[i])) {
                if(server->idleHandler[i])
                    unregisterFdEvent(server->idleHandler[i]);
                server->idleHandler[i] = NULL;
                *idle_return = 0;
                return server->connection[i];
            }
        }
    }
    *idle_return = idle;
    return NULL;
}

int
httpServerTrigger(HTTPServerPtr server)
{
    HTTPConnectionPtr connection;
    HTTPRequestPtr request;
    int idle, n, i, rc, numidle;

    while(server->request) {
        httpServerDiscardRequests(server);

        if(!server->request)
            break;

        if(REQUEST_SIDE(server->request)) {
            rc = httpServerSideRequest(server);
            /* If rc is 0, httpServerSideRequest didn't dequeue this
               request.  Go through the scheduling loop again, come
               back later. */
            if(rc <= 0) break;
            continue;
        }
        connection = httpServerGetConnection(server, &numidle);
        if(!connection) break;

        /* If server->pipeline <= 0, we don't do pipelining.  If
           server->pipeline is 1, then we are ready to start probing
           for pipelining on the server; we then send exactly two
           requests in what is hopefully a single packet to check
           whether the server has the nasty habit of discarding its
           input buffers after each request.
           If server->pipeline is 2 or 3, the pipelining probe is in
           progress on this server, and we don't pipeline anything
           until it succeeds.  When server->pipeline >= 4, pipelining
           is believed to work on this server. */
        if(server->version != HTTP_11 || server->pipeline <= 0 ||
           server->pipeline == 2 || server->pipeline == 3) {
            if(connection->pipelined == 0)
                n = 1;
            else
                n = 0;
        } else if(server->pipeline == 1) {
            if(connection->pipelined == 0)
                n = MIN(2, maxPipelineTrain);
            else
                n = 0;
        } else {
            n = maxPipelineTrain;
        }

        /* Don't pipeline if there are more idle connections */
        if(numidle >= 2)
            n = MIN(n, 1);
    
        idle = !connection->pipelined;
        i = 0;
        while(server->request && connection->pipelined < n) {
            httpServerDiscardRequests(server);
            if(!server->request) break;
            request = server->request;
            assert(request->request->request == request);
            rc = httpWriteRequest(connection, request, -1);
            if(rc < 0) {
                if(i == 0)
                    httpServerAbortRequest(request, rc != -ECLIENTRESET, 502,
                                           internAtom("Couldn't "
                                                      "write request"));
                break;
            }
            do_log(D_SERVER_CONN, "W: ");
            do_log_n(D_SERVER_CONN, 
                     request->object->key, request->object->key_size);
            do_log(D_SERVER_CONN, " (%d)\n", request->method);
            if(connection->pipelined > 0)
                request->flags |= REQUEST_PIPELINED;
            request->time0 = current_time;
            i++;
            server->request = request->next;
            request->next = NULL;
            if(server->request == NULL)
                server->request_last = NULL;
            httpQueueRequest(connection, request);
            connection->pipelined++;
        }
        if(server->persistent > 0 && server->pipeline == 1 && i >= 2)
            server->pipeline = 2;

        if(i > 0) httpServerSendRequest(connection);

        if(idle && connection->pipelined > 0)
            httpServerReply(connection, 0);

        if(i == 0) break;
    }

    for(i = 0; i < server->maxslots; i++) {
        if(server->connection[i] &&
           !server->connection[i]->connecting &&
           !server->connection[i]->request) {
            /* Artificially age any fresh connections that aren't used
               straight away; this is necessary for the logic for POST and 
               the logic that determines whether a given request should be 
               restarted. */
            if(server->connection[i]->serviced == 0)
                server->connection[i]->serviced = 1;
            if(!server->idleHandler[i])
                server->idleHandler[i] = 
                    registerFdEvent(server->connection[i]->fd, POLLIN,
                                    httpServerIdleHandler,
                                    sizeof(HTTPConnectionPtr),
                                    &server->connection[i]);
            if(!server->idleHandler[i]) {
                do_log(L_ERROR, "Couldn't register idle handler.\n");
                httpServerFinish(server->connection[i], 1, 0);
            }
            httpSetTimeout(server->connection[i], serverIdleTimeout);
        }
    }

    return 1;
}

int
httpServerSideRequest(HTTPServerPtr server)
{
    HTTPRequestPtr request = server->request;
    HTTPConnectionPtr connection;
    HTTPRequestPtr requestor = request->request;
    HTTPConnectionPtr client = requestor->connection;
    int rc, i, freeslots, idle, connecting;

    assert(REQUEST_SIDE(request));

    connection = NULL;
    freeslots = 0;
    idle = -1;
    connecting = 0;

    /* Find a fresh connection */
    for(i = 0; i < server->numslots; i++) {
        if(!server->connection[i])
            freeslots++;
        else if(!server->connection[i]->connecting) {
            if(!server->connection[i]->request) {
                if(server->connection[i]->serviced == 0) {
                    if(server->idleHandler[i])
                        unregisterFdEvent(server->idleHandler[i]);
                    server->idleHandler[i] = NULL;
                    connection = server->connection[i];
                    break;
                } else {
                    idle = i;
                }
            }
        } else {
            connecting++;
        }
    }

    if(!connection) {
        /* Make sure that a fresh connection will be established at some
           point, then wait until httpServerTrigger calls us again. */
        if(freeslots) {
            httpServerConnection(server);
        } else {
            if(idle >= 0) {
                /* Shutdown a random idle connection */
                pokeFdEvent(server->connection[idle]->fd, 
                            -EDOSHUTDOWN, POLLIN | POLLOUT);
            }
        }
        return 0;
    }

    rc = httpWriteRequest(connection, request, client->bodylen);
    if(rc < 0) {
        do_log(L_ERROR, "Couldn't write POST or PUT request.\n");
        httpServerAbortRequest(request, rc != -ECLIENTRESET, 502,
                               internAtom("Couldn't write request"));
        return 0;
    }
    server->request = request->next;
    request->next = NULL;
    if(server->request == NULL)
        server->request_last = NULL;
    httpQueueRequest(connection, request);
    connection->pipelined = 1;
    request->time0 = current_time;
    connection->reqoffset = 0;
    connection->bodylen = client->bodylen;
    httpServerDoSide(connection);
    return 1;
}

int 
httpServerDoSide(HTTPConnectionPtr connection)
{
    HTTPRequestPtr request = connection->request;
    HTTPRequestPtr requestor = request->request;
    HTTPConnectionPtr client = requestor->connection;
    int len = MIN(client->reqlen - client->reqbegin,
                  connection->bodylen - connection->reqoffset);
    int doflush = 
        len > 0 &&
        (len >= maxSideBuffering ||
         client->reqbegin > 0 ||
         (connection->reqoffset + client->reqlen - client->reqbegin) >=
         connection->bodylen);
    int done = connection->reqoffset >= connection->bodylen;

    assert(connection->bodylen >= 0);

    httpSetTimeout(connection, 60);

    if(connection->reqlen > 0) {
        /* Send the headers, but don't send any part of the body if
           we're in wait_continue. */
        do_stream_2(IO_WRITE,
                    connection->fd, 0,
                    connection->reqbuf, connection->reqlen,
                    client->reqbuf + client->reqbegin, 
                    (request->flags & REQUEST_WAIT_CONTINUE) ? 0 : len,
                    httpServerSideHandler2, connection);
        httpServerReply(connection, 0);
    } else if(request->object->flags & OBJECT_ABORTED) {
        if(connection->reqbuf)
            dispose_chunk(connection->reqbuf);
        connection->reqbuf = NULL;
        connection->reqlen = 0;
        pokeFdEvent(connection->fd, -ESHUTDOWN, POLLIN);
        if(client->flags & CONN_READER) {
            client->flags |= CONN_SIDE_READER;
            do_stream(IO_READ | IO_IMMEDIATE | IO_NOTNOW,
                      client->fd, 0, NULL, 0,
                      httpClientSideHandler, client);
        }
    } else if(!(request->flags & REQUEST_WAIT_CONTINUE) && doflush) {
        /* Make sure there's a reqbuf, as httpServerFinish uses
           it to determine if there's a writer. */
        if(connection->reqbuf == NULL)
            connection->reqbuf = get_chunk();
        assert(connection->reqbuf != NULL);
        do_stream(IO_WRITE,
                  connection->fd, 0,
                  client->reqbuf + client->reqbegin, len,
                  httpServerSideHandler, connection);
    } else {
        if(connection->reqbuf) {
            httpConnectionDestroyReqbuf(connection);
            connection->reqlen = 0;
        }
        if(request->flags & REQUEST_WAIT_CONTINUE) {
            ConditionHandlerPtr chandler;
            do_log(D_SERVER_CONN, "W... %s:%d.\n",
                   scrub(connection->server->name), connection->server->port);
            chandler = 
                conditionWait(&request->object->condition,
                              httpServerContinueConditionHandler,
                              sizeof(connection), &connection);
            if(chandler)
                return 1;
            else
                do_log(L_ERROR, "Couldn't register condition handler.\n");
            /* Fall through -- the client side will clean up. */
        }
        client->flags |= CONN_SIDE_READER;
        do_stream(IO_READ | (done ? IO_IMMEDIATE : 0 ) | IO_NOTNOW,
                  client->fd, client->reqlen,
                  client->reqbuf, CHUNK_SIZE,
                  httpClientSideHandler, client);
    }
    return 1;
}

static int
httpClientDelayedDoSideHandler(TimeEventHandlerPtr event)
{
    HTTPConnectionPtr connection = *(HTTPConnectionPtr*)event->data;
    httpServerDoSide(connection);
    return 1;
}

static int
httpServerDelayedDoSide(HTTPConnectionPtr connection)
{
    TimeEventHandlerPtr handler;
    handler = scheduleTimeEvent(0, httpClientDelayedDoSideHandler,
                                sizeof(connection), &connection);
    if(!handler) {
        do_log(L_ERROR, "Couldn't schedule DoSide -- freeing memory.\n");
        free_chunk_arenas();
        handler = scheduleTimeEvent(0, httpClientDelayedDoSideHandler,
                                    sizeof(connection), &connection);
        do_log(L_ERROR, "Couldn't schedule DoSide.\n");
        /* Somebody will hopefully end up timing out. */
        return 1;
    }
    return 1;
}

static int
httpServerSideHandlerCommon(int kind, int status,
                            FdEventHandlerPtr event,
                            StreamRequestPtr srequest)
{
    HTTPConnectionPtr connection = srequest->data;
    HTTPRequestPtr request = connection->request;
    HTTPRequestPtr requestor = request->request;
    HTTPConnectionPtr client = requestor->connection;
    int bodylen;

    assert(request->object->flags & OBJECT_INPROGRESS);

    if(status) {
        do_log_error(L_ERROR, -status, "Couldn't write to server");
        httpConnectionDestroyReqbuf(connection);
        if(status != -ECLIENTRESET)
            shutdown(connection->fd, 2);
        abortObject(request->object, 502,
                    internAtom("Couldn't write to server"));
        /* Let the read side handle the error */
        httpServerDoSide(connection);
        return 1;
    }

    assert(srequest->offset > 0);

    if(kind == 2) {
        if(srequest->offset < connection->reqlen)
            return 0;
        bodylen = srequest->offset - connection->reqlen;
        connection->reqlen = 0;
        httpConnectionDestroyReqbuf(connection);
    } else {
        bodylen = srequest->offset;
    }


    assert(client->reqbegin + bodylen <= client->reqlen);

    if(client->reqlen > client->reqbegin + bodylen)
        memmove(client->reqbuf, client->reqbuf + client->reqbegin + bodylen,
                client->reqlen - client->reqbegin - bodylen);
    client->reqlen -= bodylen + client->reqbegin;
    client->reqbegin = 0;
    connection->reqoffset += bodylen;
    httpServerDoSide(connection);
    return 1;
}

int
httpServerSideHandler(int status,
                      FdEventHandlerPtr event,
                      StreamRequestPtr srequest)
{
    return httpServerSideHandlerCommon(1, status, event, srequest);
}

int
httpServerSideHandler2(int status,
                       FdEventHandlerPtr event,
                       StreamRequestPtr srequest)
{
    return httpServerSideHandlerCommon(2, status, event, srequest);
}

static int
httpServerContinueConditionHandler(int status, ConditionHandlerPtr chandler)
{
    HTTPConnectionPtr connection = *(HTTPConnectionPtr*)chandler->data;

    if(connection->request->flags & REQUEST_WAIT_CONTINUE)
        return 0;
    httpServerDelayedDoSide(connection);
    return 1;
}

/* s is 0 to keep the connection alive, 1 to shutdown the connection */
void
httpServerFinish(HTTPConnectionPtr connection, int s, int offset)
{
    HTTPServerPtr server = connection->server;
    HTTPRequestPtr request = connection->request;
    int i;

    if(request) {
        assert(connection->pipelined >= 1);
        assert((connection->pipelined > 1) == (request->next != NULL));
    } else {
        assert(connection->pipelined == 0);
    }

    if(!s && (!connection->request ||
                  !(connection->request->flags & REQUEST_PERSISTENT)))
        s = 1;

    if(connection->serviced >= maxConnectionRequests ||
       connection->time < current_time.tv_sec - maxConnectionAge)
        s = 1;

    if(connection->reqbuf) {
        /* As most normal requests go out in a single packet, this is
           extremely unlikely to happen.  As for POST/PUT requests,
           they are not pipelined, so this can only happen if the
           server sent an error reply early. */
        assert(connection->fd >= 0);
        shutdown(connection->fd, 1);
        pokeFdEvent(connection->fd, -EDOSHUTDOWN, POLLOUT);
        httpServerDelayedFinish(connection);
        goto done;
    }

    if(request) {
        /* Update statistics about the server */
        int size = -1, d = -1, rtt = -1, rate = -1;
        if(connection->offset > 0 && request->from >= 0)
            size = connection->offset - request->from;
        if(request->time1.tv_sec != null_time.tv_sec) {
            d = timeval_minus_usec(&current_time, &request->time1);
            if(!(request->flags & REQUEST_PIPELINED) &&
               request->time0.tv_sec != null_time.tv_sec)
                rtt = timeval_minus_usec(&request->time1, &request->time0);
            if(size >= 8192 && d > 50000)
                rate = ((double)size / (double)d) * 1000000.0 + 0.5;
        }
        request->time0 = null_time;
        request->time1 = null_time;

        if(rtt >= 0) {
            if(server->rtt >= 0)
                server->rtt = (3 * server->rtt + rtt + 2) / 4;
            else
                server->rtt = rtt;
        }
        if(rate >= 0) {
            if(server->rate >= 0)
                server->rate = (3 * server->rate + rate + 2) / 4;
            else
                server->rate = rate;
        }

        httpDequeueRequest(connection);
        connection->pipelined--;
        request->object->flags &= ~(OBJECT_INPROGRESS | OBJECT_VALIDATING);
        if(request->request) {
            request->request->request = NULL;
            request->request = NULL;
        }
        releaseNotifyObject(request->object);
        request->object = NULL;
        httpDestroyRequest(request);
    }

    do_log(D_SERVER_CONN, "Done with server %s:%d connection (%d)\n",
           scrub(connection->server->name), connection->server->port, s);

    assert(offset <= connection->len);

    if(!s) {
        if(offset < connection->len) {
            assert(connection->buf != NULL);
            if(!connection->pipelined) {
                do_log(L_WARN, 
                       "Closing connection to %s:%d: "
                       "%d stray bytes of data.\n",
                       scrub(server->name), server->port,
                       connection->len - offset);
                s = 1;
            } else {
                memmove(connection->buf, connection->buf + offset,
                        connection->len - offset);
                connection->len = connection->len - offset;
                if((connection->flags & CONN_BIGBUF) &&
                   connection->len <= CHUNK_SIZE)
                    httpConnectionUnbigify(connection);
            }
        } else {
            connection->len = 0;
        }
    }

    connection->server->time = current_time.tv_sec;
    connection->serviced++;

    if(s) {
        if(connection->timeout)
            cancelTimeEvent(connection->timeout);
        connection->timeout = NULL;
        httpConnectionDestroyBuf(connection);
        if(connection->fd >= 0)
            CLOSE(connection->fd);
        connection->fd = -1;
        server->persistent -= 1;
        if(server->persistent < -5)
            server->numslots = MIN(server->maxslots, serverMaxSlots);
        if(connection->request) {
            HTTPRequestPtr req;
            do_log(D_SERVER_CONN, "Restarting pipeline to %s:%d.\n",
                   scrub(server->name), server->port);
            if(server->pipeline == 2)
                server->pipeline -= 20;
            else
                server->pipeline -= 5;
            req = connection->request;
            while(req) {
                req->connection = NULL;
                req = req->next;
            }
            if(server->request)
                connection->request_last->next = server->request;
            else
                server->request_last = connection->request_last;
            server->request = connection->request;
            connection->request = NULL;
            connection->request_last = NULL;
        }
        /* Make sure we don't get confused into thinking a probe
           is in progress. */
        if(server->pipeline == 2 || server->pipeline == 3)
            server->pipeline = 1;
        for(i = 0; i < server->maxslots; i++)
            if(connection == server->connection[i])
                break;
        assert(i < server->maxslots);
        if(server->idleHandler[i])
            unregisterFdEvent(server->idleHandler[i]);
        server->idleHandler[i] = NULL;
        server->connection[i] = NULL;
        free(connection);
    } else {
        server->persistent += 1;
        if(server->persistent > 0)
            server->numslots = MIN(server->maxslots,
                                   server->version == HTTP_10 ?
                                   serverSlots1 : serverSlots);
        httpSetTimeout(connection, serverTimeout);
        /* See httpServerTrigger */
        if(connection->pipelined ||
           (server->version == HTTP_11 && server->pipeline <= 0) ||
           (server->pipeline == 3)) {
            server->pipeline++;
        }
        if(connection->pipelined) {
            httpServerReply(connection, 1);
        } else {
            httpConnectionDestroyBuf(connection);
        }
    }

 done:
    httpServerTrigger(server);
}

static int
httpServerDelayedFinishHandler(TimeEventHandlerPtr event)
{
    HTTPConnectionPtr connection = *(HTTPConnectionPtr*)event->data;
    httpServerFinish(connection, 1, 0);
    return 1;
}

static void
httpServerDelayedFinish(HTTPConnectionPtr connection)
{
    TimeEventHandlerPtr handler;

    handler = scheduleTimeEvent(1, httpServerDelayedFinishHandler,
                                sizeof(connection), &connection);
    if(!handler) {
        do_log(L_ERROR,
               "Couldn't schedule delayed finish -- freeing memory.");
        free_chunk_arenas();
        handler = scheduleTimeEvent(1, httpServerDelayedFinishHandler,
                                    sizeof(connection), &connection);
        if(!handler) {
            do_log(L_ERROR,
                   "Couldn't schedule delayed finish -- aborting.\n");
            polipoExit();
        }
    }
}

void
httpServerReply(HTTPConnectionPtr connection, int immediate)
{
    assert(connection->pipelined > 0);

    if(connection->request->request == NULL) {
        do_log(L_WARN, "Aborting pipeline on %s:%d.\n",
               scrub(connection->server->name), connection->server->port);
        httpServerFinish(connection, 1, 0);
        return;
    }

    do_log(D_SERVER_CONN, "R: %s (%d)\n",
           scrub(connection->request->object->key),
           connection->request->method);

    if(connection->len == 0)
        httpConnectionDestroyBuf(connection);

    httpSetTimeout(connection, serverTimeout);
    do_stream_buf(IO_READ | (immediate ? IO_IMMEDIATE : 0) | IO_NOTNOW,
                  connection->fd, connection->len,
                  &connection->buf, CHUNK_SIZE,
                  httpServerReplyHandler, connection);
}

int
httpConnectionPipelined(HTTPConnectionPtr connection)
{
    HTTPRequestPtr request = connection->request;
    int i = 0;
    while(request) {
        i++;
        request = request->next;
    }
    return i;
}

void
httpServerUnpipeline(HTTPRequestPtr request)
{
    HTTPConnectionPtr connection = request->connection;
    HTTPServerPtr server = connection->server;

    request->flags &= ~REQUEST_PERSISTENT;
    if(request->next) {
        HTTPRequestPtr req;
        do_log(L_WARN,
               "Restarting pipeline to %s:%d.\n", 
               scrub(connection->server->name), connection->server->port);
        req = request->next;
        while(req) {
            req->connection = NULL;
            req = req->next;
        }
        if(server->request)
            connection->request_last->next = server->request;
        else
            server->request_last = connection->request_last;
        server->request = request->next;
        request->next = NULL;
        connection->request_last = request;
    }
    connection->pipelined = httpConnectionPipelined(connection);
}

void
httpServerRestart(HTTPConnectionPtr connection)
{
    HTTPServerPtr server = connection->server;
    HTTPRequestPtr request = connection->request;

    if(request) {
        HTTPRequestPtr req;
        if(request->next)
            do_log(L_WARN,
                   "Restarting pipeline to %s:%d.\n", 
                   scrub(connection->server->name), connection->server->port);
        req = request;
        while(req) {
            req->connection = NULL;
            req = req->next;
        }
        if(server->request)
            connection->request_last->next = server->request;
        else
            server->request_last = connection->request_last;
        server->request = request;
        connection->request = NULL;
        connection->request_last = NULL;
    }
    connection->pipelined = 0;
    httpServerFinish(connection, 1, 0);
}

int
httpServerRequest(ObjectPtr object, int method, int from, int to, 
                  HTTPRequestPtr requestor, void *closure)
{
    int rc;
    char name[132];
    int port;
    int x, y, z;

    assert(from >= 0 && (to < 0 || to > from));
    assert(closure == NULL);
    assert(!(object->flags & OBJECT_LOCAL));
    assert(object->type == OBJECT_HTTP);

    if(object->flags & OBJECT_INPROGRESS)
        return 1;

    if(requestor->flags & REQUEST_REQUESTED)
        return 0;

    assert(requestor->request == NULL);

    if(proxyOffline)
        return -1;

    rc = parseUrl(object->key, object->key_size, &x, &y, &port, &z);
    
    if(rc < 0 || x < 0 || y < 0 || y - x > 131) {
        do_log(L_ERROR, "Couldn't parse URL %s\n", scrub(object->key));
        abortObject(object, 400, internAtom("Couldn't parse URL"));
        notifyObject(object);
        return 1;
    }

    if(!intListMember(port, allowedPorts)) {
        do_log(L_ERROR, "Attempted connection to port %d.\n", port);
        abortObject(object, 403, internAtom("Forbidden port"));
        notifyObject(object);
        return 1;
    }

    memcpy(name, ((char*)object->key) + x, y - x);
    name[y - x] = '\0';

    requestor->flags |= REQUEST_REQUESTED;
    rc = httpMakeServerRequest(name, port, object, method, from, to,
                               requestor);
                                   
    if(rc < 0) {
        abortObject(object, 
                    503, internAtom("Couldn't schedule server request"));
        notifyObject(object);
        return 1;
    }

    return 1;
}

int
httpWriteRequest(HTTPConnectionPtr connection, HTTPRequestPtr request,
                 int bodylen)
{
    ObjectPtr object = request->object;
    int from = request->from, to = request->to, method = request->method;
    char *url = object->key, *m;
    int url_size = object->key_size;
    int x, y, port, z, location_size;
    char *location;
    int l, n, rc, bufsize;

    assert(method != METHOD_NONE);

    if(request->method == METHOD_GET || 
       request->method == METHOD_CONDITIONAL_GET) {
        if(to >= 0) {
            assert(to >= from);
            if(to == from) {
                do_log(L_ERROR, "Requesting empty segment?\n");
                return -1;
            }
        }

        if(object->flags & OBJECT_DYNAMIC) {
            from = 0;
            to = -1;
        } else {
            objectFillFromDisk(object, from / CHUNK_SIZE * CHUNK_SIZE, 1);
            l = objectHoleSize(request->object, from);
            if(l > 0) {
                if(to <= 0 || to > from + l)
                    to = from + l;
            }

            if(pmmSize && connection->server->pipeline >= 4) {
                if(from == 0)
                    to = to < 0 ? pmmFirstSize : MIN(to, pmmFirstSize);
                else
                    to = to < 0 ? from + pmmSize : MIN(to, from + pmmSize);
            }

            if(from % CHUNK_SIZE != 0)
                if(objectHoleSize(object, from - 1) != 0)
                    from = from / CHUNK_SIZE * CHUNK_SIZE;
        }
    }

    rc = parseUrl(url, url_size, &x, &y, &port, &z);

    if(rc < 0 || x < 0 || y < 0) {
        return -1;
    }

    if(connection->reqbuf == NULL) {
        connection->reqbuf = get_chunk();
        if(connection->reqbuf == NULL)
            return -1;
        connection->reqlen = 0;
    }

    if(method == METHOD_CONDITIONAL_GET &&
       object->last_modified < 0 && object->etag == NULL)
        method = request->method = METHOD_GET;

 again:
    bufsize = 
        (connection->flags & CONN_BIGREQBUF) ? bigBufferSize : CHUNK_SIZE;
    n = connection->reqlen;
    switch(method) {
    case METHOD_GET:
    case METHOD_CONDITIONAL_GET: m = "GET"; break;
    case METHOD_HEAD: m = "HEAD"; break;
    case METHOD_POST: m = "POST"; break;
    case METHOD_PUT: m = "PUT"; break;
    default: abort();
    }
    n = snnprintf(connection->reqbuf, n, bufsize, "%s ", m);

    if(connection->server->isProxy) {
        n = snnprint_n(connection->reqbuf, n, bufsize,
                       url, url_size);
    } else {
        if(url_size - z == 0) {
            location = "/";
            location_size = 1;
        } else {
            location = url + z;
            location_size = url_size - z;
        }
        
        n = snnprint_n(connection->reqbuf, n, bufsize, 
                       location, location_size);
    }
    
    do_log(D_SERVER_REQ, "Server request: ");
    do_log_n(D_SERVER_REQ, url + x, y - x);
    do_log(D_SERVER_REQ, ": ");
    do_log_n(D_SERVER_REQ, connection->reqbuf, n);
    do_log(D_SERVER_REQ, " (method %d from %d to %d, 0x%lx for 0x%lx)\n",
           method, from, to,
           (unsigned long)connection, (unsigned long)object);

    n = snnprintf(connection->reqbuf, n, bufsize, " HTTP/1.1");

    n = snnprintf(connection->reqbuf, n, bufsize, "\r\nHost: ");
    n = snnprint_n(connection->reqbuf, n, bufsize, url + x, y - x);
    if(port != 80)
        n = snnprintf(connection->reqbuf, n, bufsize, ":%d", port);

    if(connection->server->isProxy && parentAuthCredentials) {
        n = buildServerAuthHeaders(connection->reqbuf, n, bufsize,
                                   parentAuthCredentials);
    }

    if(bodylen >= 0)
        n = snnprintf(connection->reqbuf, n, bufsize,
                      "\r\nContent-Length: %d", bodylen);

    if(request->flags & REQUEST_WAIT_CONTINUE)
        n = snnprintf(connection->reqbuf, n, bufsize,
                      "\r\nExpect: 100-continue");

    if(method != METHOD_HEAD && (from > 0 || to >= 0)) {
        if(to >= 0) {
            n = snnprintf(connection->reqbuf, n, bufsize,
                          "\r\nRange: bytes=%d-%d", from, to - 1);
        } else {
            n = snnprintf(connection->reqbuf, n, bufsize,
                          "\r\nRange: bytes=%d-", from);
        }
    }

    if(method == METHOD_GET && object->etag && (from > 0 || to >= 0)) {
        if(request->request && request->request->request == request &&
           request->request->from == 0 && request->request->to == -1 &&
           pmmSize == 0 && pmmFirstSize == 0)
            n = snnprintf(connection->reqbuf, n, bufsize,
                          "\r\nIf-Range: \"%s\"", object->etag);
    }

    if(method == METHOD_CONDITIONAL_GET) {
        if(object->last_modified >= 0) {
            n = snnprintf(connection->reqbuf, n, bufsize,
                          "\r\nIf-Modified-Since: ");
            n = format_time(connection->reqbuf, n, bufsize,
                            object->last_modified);
        }
        if(object->etag) {
            n = snnprintf(connection->reqbuf, n, bufsize,
                          "\r\nIf-None-Match: \"%s\"", object->etag);
        }
    }

    n = httpPrintCacheControl(connection->reqbuf, n, bufsize,
                              alwaysAddNoTransform ? CACHE_NO_TRANSFORM : 0,
			      &request->cache_control);
    if(n < 0)
        goto fail;

    if(request->request && request->request->headers) {
        n = snnprint_n(connection->reqbuf, n, bufsize,
                       request->request->headers->string, 
                       request->request->headers->length);
    }
    if(!disableVia) {
        if(request->request && request->request->via) {
            n = snnprintf(connection->reqbuf, n, bufsize,
                          "\r\nVia: %s, 1.1 %s",
                          request->request->via->string, proxyName->string);
        } else {
            n = snnprintf(connection->reqbuf, n, bufsize,
                          "\r\nVia: 1.1 %s",
                          proxyName->string);
        }
    }

    n = snnprintf(connection->reqbuf, n, bufsize,
                  "\r\nConnection: %s\r\n\r\n",
                  (request->flags & REQUEST_PERSISTENT) ? 
                  "keep-alive" : "close");
    if(n < 0 || n >= bufsize - 1)
        goto fail;
    connection->reqlen = n;
    return n;

 fail:
    rc = 0;
    if(!(connection->flags & CONN_BIGREQBUF))
        rc = httpConnectionBigifyReqbuf(connection);
    if(rc == 1)
        goto again;
    return -1;
}

int
httpServerHandler(int status,
                  FdEventHandlerPtr event,
                  StreamRequestPtr srequest)
{
    HTTPConnectionPtr connection = srequest->data;

    assert(connection->request->object->flags & OBJECT_INPROGRESS);

    if(connection->reqlen == 0) {
        do_log(D_SERVER_REQ, "Writing aborted on 0x%lx\n",
               (unsigned long)connection);
        goto fail;
    }

    if(status == 0 && !streamRequestDone(srequest)) {
        httpSetTimeout(connection, serverTimeout);
        return 0;
    }

    httpConnectionDestroyReqbuf(connection);

    if(status) {
        if(connection->serviced >= 1) {
            httpServerRestart(connection);
            return 1;
        }
        if(status < 0 && status != -ECONNRESET && status != -EPIPE)
            do_log_error(L_ERROR, -status,
                         "Couldn't send request to server");
        goto fail;
    }

    return 1;

 fail:
    httpConnectionDestroyReqbuf(connection);
    shutdown(connection->fd, 2);
    pokeFdEvent(connection->fd, -EDOSHUTDOWN, POLLIN);
    httpSetTimeout(connection, 60);
    return 1;
}

int
httpServerSendRequest(HTTPConnectionPtr connection)
{
    assert(connection->server);

    if(connection->reqlen == 0) {
        do_log(D_SERVER_REQ, 
               "Writing aborted on 0x%lx\n", (unsigned long)connection);
        httpConnectionDestroyReqbuf(connection);
        shutdown(connection->fd, 2);
        pokeFdEvent(connection->fd, -EDOSHUTDOWN, POLLIN | POLLOUT);
        return -1;
    }

    httpSetTimeout(connection, serverTimeout);
    do_stream(IO_WRITE, connection->fd, 0,
              connection->reqbuf, connection->reqlen,
              httpServerHandler, connection);
    return 1;
}

int
httpServerReplyHandler(int status,
                       FdEventHandlerPtr event, 
                       StreamRequestPtr srequest)
{
    HTTPConnectionPtr connection = srequest->data;
    HTTPRequestPtr request = connection->request;
    int i, body;
    int bufsize = 
        (connection->flags & CONN_BIGBUF) ? bigBufferSize : CHUNK_SIZE;

    assert(request->object->flags & OBJECT_INPROGRESS);
    if(status < 0) {
        if(connection->serviced >= 1) {
            httpServerRestart(connection);
            return 1;
        }
        if(status != -ECLIENTRESET)
            do_log_error(L_ERROR, -status, "Read from server failed");
        httpServerAbort(connection, status != -ECLIENTRESET, 502, 
                        internAtomError(-status, "Read from server failed"));
        return 1;
    }

    i = findEndOfHeaders(connection->buf, 0, srequest->offset, &body);
    connection->len = srequest->offset;

    if(i >= 0) {
        request->time1 = current_time;
        return httpServerHandlerHeaders(status, event, srequest, connection);
    }

    if(status) {
        if(connection->serviced >= 1) {
            httpServerRestart(connection);
            return 1;
        }
        if(status < 0) {
            do_log(L_ERROR, 
                   "Error reading server headers: %d\n", -status);
            httpServerAbort(connection, status != -ECLIENTRESET, 502, 
                            internAtomError(-status, 
                                            "Error reading server headers"));
        } else
            httpServerAbort(connection, 1, 502, 
                            internAtom("Server dropped connection"));
        return 1;
    }

    if(connection->len >= bufsize) {
        int rc = 0;
        if(!(connection->flags & CONN_BIGBUF))
            rc = httpConnectionBigify(connection);
        if(rc == 0) {
            do_log(L_ERROR, "Couldn't find end of server's headers.\n");
            httpServerAbort(connection, 1, 502,
                            internAtom("Couldn't find end "
                                       "of server's headers"));
            return 1;
        } else if(rc < 0) {
            do_log(L_ERROR, "Couldn't allocate big buffer.\n");
            httpServerAbort(connection, 1, 500,
                            internAtom("Couldn't allocate big buffer"));
            return 1;
        }
        /* Can't just return 0 -- buf has moved. */
        do_stream(IO_READ,
                  connection->fd, connection->len,
                  connection->buf, bigBufferSize,
                  httpServerReplyHandler, connection);
        return 1;
    }

    return 0;
}

int
httpServerHandlerHeaders(int eof,
                         FdEventHandlerPtr event,
                         StreamRequestPtr srequest, 
                         HTTPConnectionPtr connection)
{
    HTTPRequestPtr request = connection->request;
    ObjectPtr object = request->object;
    int rc;
    int code, version;
    int full_len;
    AtomPtr headers;
    int len;
    int te;
    CacheControlRec cache_control;
    int age = -1;
    time_t date, last_modified, expires;
    struct timeval *init_time;
    char *etag;
    AtomPtr via, new_via;
    int expect_body;
    HTTPRangeRec content_range;
    ObjectPtr new_object = NULL, old_object = NULL;
    int supersede = 0;
    AtomPtr message = NULL;
    int suspectDynamic;
    AtomPtr url = NULL;

    assert(request->object->flags & OBJECT_INPROGRESS);
    assert(eof >= 0);

    httpSetTimeout(connection, -1);

    if(request->flags & REQUEST_WAIT_CONTINUE) {
        do_log(D_SERVER_CONN, "W   %s:%d.\n",
               connection->server->name, connection->server->port);
        request->flags &= ~REQUEST_WAIT_CONTINUE;
    }

    rc = httpParseServerFirstLine(connection->buf, &code, &version, &message);
    if(rc <= 0) {
        do_log(L_ERROR, "Couldn't parse server status line.\n");
        httpServerAbort(connection, 1, 502,
                        internAtom("Couldn't parse server status line"));
        return 1;
    }

    do_log(D_SERVER_REQ, "Server status: ");
    do_log_n(D_SERVER_REQ, connection->buf, 
             connection->buf[rc - 1] == '\r' ? rc - 2 : rc - 2);
    do_log(D_SERVER_REQ, " (0x%lx for 0x%lx)\n",
           (unsigned long)connection, (unsigned long)object);

    if(version != HTTP_10 && version != HTTP_11) {
        do_log(L_ERROR, "Unknown server HTTP version\n");
        httpServerAbort(connection, 1, 502,
                        internAtom("Unknown server HTTP version"));
        releaseAtom(message);
        return 1;
    } 

    connection->version = version;
    connection->server->version = version;
    request->flags |= REQUEST_PERSISTENT;

    url = internAtomN(object->key, object->key_size);    
    rc = httpParseHeaders(0, url, connection->buf, rc, request,
                          &headers, &len, &cache_control, NULL, &te,
                          &date, &last_modified, &expires, NULL, NULL, NULL,
                          &age, &etag, NULL, NULL, &content_range,
                          NULL, &via, NULL);
    if(rc < 0) {
        do_log(L_ERROR, "Couldn't parse server headers\n");
        releaseAtom(url);
        releaseAtom(message);
        httpServerAbort(connection, 1, 502, 
                        internAtom("Couldn't parse server headers"));
        return 1;
    }

    if(date < 0)
        date = current_time.tv_sec;

    if(code == 100) {
        releaseAtom(url);
        releaseAtom(message);
        /* We've already reset wait_continue above, but we must still
           ensure that the writer notices. */
        notifyObject(request->object);
        connection->len -= rc;
        if(connection->len > 0)
            memmove(connection->buf, connection->buf + rc, connection->len);
        httpServerReply(connection, 1);
        return 1;
    }

    if(code == 101) {
        httpServerAbort(connection, 1, 501,
                        internAtom("Upgrade not implemented"));
        goto fail;
    }

    if(via && !checkVia(proxyName, via)) {
        httpServerAbort(connection, 1, 504, internAtom("Proxy loop detected"));
        goto fail;
    }
    full_len = content_range.full_length;

    if(code == 206) {
        if(content_range.from == -1 || content_range.to == -1) {
            do_log(L_ERROR, "Partial content without range.\n");
            httpServerAbort(connection, 1, 502,
                            internAtom("Partial content without range"));
            goto fail;
        }
        if(len >= 0 && len != content_range.to - content_range.from) {
            do_log(L_ERROR, "Inconsistent partial content.\n");
            httpServerAbort(connection, 1, 502,
                            internAtom("Inconsistent partial content"));
            goto fail;
        }
    } else if(code < 400 && 
              (content_range.from >= 0 || content_range.to >= 0 || 
               content_range.full_length >= 0)) {
        do_log(L_WARN, "Range without partial content.\n");
        /* Damn anakata. */
        content_range.from = -1;
        content_range.to = -1;
        content_range.full_length = -1;
    } else if(code != 304 && code != 412) {
        full_len = len;
    }

    if(te != TE_IDENTITY && te != TE_CHUNKED) {
        do_log(L_ERROR, "Unsupported transfer-encoding\n");
        httpServerAbort(connection, 1, 502,
                        internAtom("Unsupported transfer-encoding"));
        goto fail;
    }

    if(code == 304) {
        if(request->method != METHOD_CONDITIONAL_GET) {
            do_log(L_ERROR, "Unexpected \"not changed\" reply from server\n");
            httpServerAbort(connection, 1, 502,
                            internAtom("Unexpected \"not changed\" "
                                       "reply from server"));
            goto fail;
        }
        if(object->etag && !etag) {
            /* RFC 2616 10.3.5.  Violated by some front-end proxies. */
            do_log(L_WARN, "\"Not changed\" reply with no ETag.\n");
        } 
    }

    if(code == 412) {
        if(request->method != METHOD_CONDITIONAL_GET ||
           (!object->etag && !object->last_modified)) {
            do_log(L_ERROR, 
                   "Unexpected \"precondition failed\" reply from server.\n");
            httpServerAbort(connection, 1, 502,
                            internAtom("Unexpected \"precondition failed\" "
                                       "reply from server"));
            goto fail;
        }
    }

    releaseAtom(url);

    /* Okay, we're going to accept this reply. */

    if((code == 200 || code == 206 || code == 304 || code == 412) &&
       (cache_control.flags & (CACHE_NO | CACHE_NO_STORE) ||
        cache_control.max_age == 0 ||
        (cacheIsShared && cache_control.s_maxage == 0) ||
        (expires >= 0 && expires <= object->age))) {
        do_log(L_UNCACHEABLE, "Uncacheable object %s (%d)\n",
               scrub(object->key), cache_control.flags);
    }

    if(request->time0.tv_sec != null_time.tv_sec)
        init_time = &request->time0;
    else
        init_time = &current_time;
    age = MIN(init_time->tv_sec - age, init_time->tv_sec);

    if(request->method == METHOD_HEAD || 
       code < 200 || code == 204 || code == 304)
        expect_body = 0;
    else if(te == TE_IDENTITY)
        expect_body = (len != 0);
    else
        expect_body = 1;

    connection->chunk_remaining = -1;
    connection->te = te;

    old_object = object;

    connection->server->lies--;

    if(object->cache_control & CACHE_MISMATCH)
        supersede = 1;

    if(code == 304 || code == 412) {
        if((object->etag && etag && strcmp(object->etag, etag) != 0) ||
           (object->last_modified >= 0 && last_modified >= 0 &&
            object->last_modified != last_modified)) {
            do_log(L_ERROR, "Inconsistent \"%s\" reply for %s\n",
                   code == 304 ? "not changed":"precondition failed",
                   scrub(object->key));
            object->flags |= OBJECT_DYNAMIC;
            supersede = 1;
        }
    } else if(!(object->flags & OBJECT_INITIAL)) {
        if((object->last_modified < 0 || last_modified < 0) &&
           (!object->etag || !etag))
            supersede = 1;
        else if(object->last_modified != last_modified)
            supersede = 1;
        else if(object->etag || etag) {
            /* We need to be permissive here so as to deal with some
               front-end proxies that discard ETags on partial
               replies but not on full replies. */
            if(etag && object->etag && strcmp(object->etag, etag) != 0)
                supersede = 1;
            else if(!object->etag)
                supersede = 1;
        }

        if(!supersede && (object->cache_control & CACHE_VARY) &&
           dontTrustVaryETag >= 1) {
            /* Check content-type to work around mod_gzip bugs */
            if(!httpHeaderMatch(atomContentType, object->headers, headers) ||
               !httpHeaderMatch(atomContentEncoding, object->headers, headers))
                supersede = 1;
        }

        if(full_len < 0 && te == TE_IDENTITY) {
            /* It's an HTTP/1.0 CGI.  Be afraid. */
            if(expect_body && content_range.from < 0 && content_range.to < 0)
                supersede = 1;
        }

        if(!supersede && object->length >= 0 && full_len >= 0 &&
                object->length != full_len) {
            do_log(L_WARN, "Inconsistent length.\n");
            supersede = 1;
        }

        if(!supersede &&
           ((object->last_modified >= 0 && last_modified >= 0) ||
            (object->etag && etag))) {
            if(request->method == METHOD_CONDITIONAL_GET) {
                do_log(L_WARN, "Server ignored conditional request.\n");
                connection->server->lies += 10;
                /* Drop the connection? */
            }
        }
    } else if(code == 416) {
        do_log(L_ERROR, "Unexpected \"range not satisfiable\" reply\n");
        httpServerAbort(connection, 1, 502,
                        internAtom("Unexpected \"range not satisfiable\" "
                                   "reply"));
        /* The object may be superseded.  Make sure the next request
           won't be partial. */
        abortObject(object, 502, 
                    internAtom("Unexpected \"range not satisfiable\" reply"));
        return 1;
    }

    if(object->flags & OBJECT_INITIAL)
        supersede = 0;

    if(supersede) {
        do_log(L_SUPERSEDED,
               "Superseding object %s (%d %d %d %s -> %d %d %d %s)\n",
               scrub(old_object->key),
               object->code, object->length, (int)object->last_modified,
               object->etag ? object->etag : "(none)",
               code, full_len, (int)last_modified,
               etag ? etag : "(none)");
        privatiseObject(old_object, 0);
        new_object = makeObject(object->type, object->key, 
                                object->key_size, 1, 0, 
                                object->request, NULL);
        if(new_object == NULL) {
            do_log(L_ERROR, "Couldn't allocate object\n");
            httpServerAbort(connection, 1, 500,
                            internAtom("Couldn't allocate object"));
            return 1;
        }
        if(urlIsLocal(new_object->key, new_object->key_size))
            new_object->flags |= OBJECT_LOCAL;
    } else {
        new_object = object;
    }

    suspectDynamic =
        (!etag && last_modified < 0) ||
        (cache_control.flags &
         (CACHE_NO_HIDDEN | CACHE_NO | CACHE_NO_STORE |
          (cacheIsShared ? CACHE_PRIVATE : 0))) ||
        (cache_control.max_age >= 0 && cache_control.max_age <= 2) ||
        (cacheIsShared && 
         cache_control.s_maxage >= 0 && cache_control.s_maxage <= 5) ||
        (old_object->last_modified >= 0 && old_object->expires >= 0 && 
         (old_object->expires - old_object->last_modified <= 1)) ||
        (supersede && (old_object->date - date <= 5));

    if(suspectDynamic)
        new_object->flags |= OBJECT_DYNAMIC;
    else if(!supersede)
        new_object->flags &= ~OBJECT_DYNAMIC;
    else if(old_object->flags & OBJECT_DYNAMIC)
        new_object->flags |= OBJECT_DYNAMIC;

    new_object->age = age;
    new_object->cache_control |= cache_control.flags;
    new_object->max_age = cache_control.max_age;
    new_object->s_maxage = cache_control.s_maxage;
    new_object->flags &= ~OBJECT_FAILED;

    if(date >= 0)
        new_object->date = date;
    if(last_modified >= 0)
        new_object->last_modified = last_modified;
    if(expires >= 0)
        new_object->expires = expires;
    if(new_object->etag == NULL)
        new_object->etag = etag;
    else
        free(etag);

    switch(code) {
    case 200:
    case 300: case 301: case 302: case 303: case 307:
    case 403: case 404: case 405: case 401:
        if(new_object->message) releaseAtom(new_object->message);
        new_object->code = code;
        new_object->message = message;
        break;
    case 206: case 304: case 412:
        if(new_object->code != 200 || !new_object->message) {
            if(new_object->message) releaseAtom(new_object->message);
            new_object->code = 200;
            new_object->message = internAtom("OK");
        }
        releaseAtom(message);
        break;
    default:
        if(new_object->message) releaseAtom(new_object->message);
        new_object->code = code;
        new_object->message = retainAtom(message);
        break;
    }

    httpTweakCachability(new_object);

    if(!via)
        new_via = internAtomF("%s %s",
                              version == HTTP_11 ? "1.1" : "1.0",
                              proxyName->string);
    else
        new_via = internAtomF("%s, %s %s", via->string,
                              version == HTTP_11 ? "1.1" : "1.0",
                              proxyName->string);
    if(new_via == NULL) {
        do_log(L_ERROR, "Couldn't allocate Via.\n");
    } else {
        if(new_object->via) releaseAtom(new_object->via);
        new_object->via = new_via;
    }

    if(new_object->flags & OBJECT_INITIAL) {
        objectPartial(new_object, full_len, headers);
    } else {
        if(new_object->length < 0)
            new_object->length = full_len;
        /* XXX -- RFC 2616 13.5.3 */
        releaseAtom(headers);
    }

    if(supersede) {
        assert(new_object != old_object);
        supersedeObject(old_object);
    }

    if(new_object != old_object) {
        if(new_object->flags & OBJECT_INPROGRESS) {
            /* Make sure we don't fetch this object two times at the
               same time.  Just drop the connection. */
            releaseObject(new_object);
            httpServerFinish(connection, 1, 0);
            return 1;
        }
        old_object->flags &= ~OBJECT_VALIDATING;
        new_object->flags |= OBJECT_INPROGRESS;
        /* Signal the client side to switch to the new object -- see
           httpClientGetHandler.  If it doesn't, we'll give up on this
           request below. */
        new_object->flags |= OBJECT_MUTATING;
        request->can_mutate = new_object;
        notifyObject(old_object);
        request->can_mutate = NULL;
        new_object->flags &= ~OBJECT_MUTATING;
        old_object->flags &= ~OBJECT_INPROGRESS;
        if(request->object == old_object) {
            if(request->request)
                request->request->request = NULL;
            request->request = NULL;
            request->object = new_object;
        } else {
            assert(request->object == new_object);
        }
        releaseNotifyObject(old_object);
        old_object = NULL;
        object = new_object;
    } else {
        objectMetadataChanged(new_object, 0);
    }

    if(object->flags & OBJECT_VALIDATING) {
        object->flags &= ~OBJECT_VALIDATING;
        notifyObject(object);
    }

    if(!expect_body) {
        httpServerFinish(connection, 0, rc);
        return 1;
    }

    if(request->request == NULL) {
        httpServerFinish(connection, 1, 0);
        return 1;
    }

    if(code == 412) {
        /* 412 replies contain a useless body.  For now, we
           drop the connection. */
        httpServerFinish(connection, 1, 0);
        return 1;
    }


    if(request->flags & REQUEST_PERSISTENT) {
        if(request->method != METHOD_HEAD && 
           connection->te == TE_IDENTITY && len < 0) {
            do_log(L_ERROR, "Persistent reply with no Content-Length\n");
            /* That's potentially dangerous, as we could start reading
               arbitrary data into the object.  Unfortunately, some
               servers do that. */
            request->flags &= ~REQUEST_PERSISTENT;
        }
    }

    /* we're getting a body */
    if(content_range.from > 0)
        connection->offset = content_range.from;
    else
        connection->offset = 0;

    if(content_range.to >= 0)
        request->to = content_range.to;

    do_log(D_SERVER_OFFSET, "0x%lx(0x%lx): offset = %d\n",
           (unsigned long)connection, (unsigned long)object,
           connection->offset);

    if(connection->len > rc) {
        rc = connectionAddData(connection, rc);
        if(rc) {
            if(rc < 0) {
                if(rc == -2) {
                    do_log(L_ERROR, "Couldn't parse chunk size.\n");
                    httpServerAbort(connection, 1, 502,
                                    internAtom("Couldn't parse chunk size"));
                } else {
                    do_log(L_ERROR, "Couldn't add data to connection.\n");
                    httpServerAbort(connection, 1, 500, 
                                    internAtom("Couldn't add data "
                                               "to connection"));
                }
                return 1;
            } else {
                if(code != 206) {
                    if(object->length < 0) {
                        object->length = object->size;
                        objectMetadataChanged(object, 0);
                    } else if(object->length != object->size) {
                        httpServerAbort(connection, 1, 500, 
                                        internAtom("Inconsistent "
                                                   "object size"));
                        object->length = -1;
                        return 1;
                    }
                }
                httpServerFinish(connection, 0, 0);
                return 1;
            }
        }
    } else {
        connection->len = 0;
    }

    if(eof) {
        if(connection->te == TE_CHUNKED ||
           (object->length >= 0 && 
            connection->offset < object->length)) {
            do_log(L_ERROR, "Server closed connection.\n");
            httpServerAbort(connection, 1, 502,
                            internAtom("Server closed connection"));
            return 1;
        } else {
            if(code != 206 && eof > 0 && object->length < 0) {
                object->length = object->size;
                objectMetadataChanged(object, 0);
            }
            httpServerFinish(connection, 1, 0);
            return 1;
        }
    } else {
        return httpServerReadData(connection, 1);
    }
    return 0;

 fail:
    releaseAtom(url);
    releaseAtom(message);
    if(headers)
        releaseAtom(headers);
    if(etag)
        free(etag);
    if(via)
        releaseAtom(via);
    return 1;
}

int
httpServerIndirectHandlerCommon(HTTPConnectionPtr connection, int eof)
{
    HTTPRequestPtr request = connection->request;

    assert(eof >= 0);
    assert(request->object->flags & OBJECT_INPROGRESS);

    if(connection->len > 0) {
        int rc;
        rc = connectionAddData(connection, 0);
        if(rc) {
            if(rc < 0) {
                if(rc == -2) {
                    do_log(L_ERROR, "Couldn't parse chunk size.\n");
                    httpServerAbort(connection, 1, 502,
                                    internAtom("Couldn't parse chunk size"));
                } else {
                    do_log(L_ERROR, "Couldn't add data to connection.\n");
                    httpServerAbort(connection, 1, 500,
                                    internAtom("Couldn't add data "
                                               "to connection"));
                }
                return 1;
            } else {
                if(request->to < 0) {
                    if(request->object->length < 0) {
                        request->object->length = request->object->size;
                        objectMetadataChanged(request->object, 0);
                    } else if(request->object->length != 
                              request->object->size) {
                        request->object->length = -1;
                        httpServerAbort(connection, 1, 502,
                                        internAtom("Inconsistent "
                                                   "object size"));
                        return 1;
                    }
                }
                httpServerFinish(connection, 0, 0);
            }
            return 1;
        }
    }

    if(eof && connection->len == 0) {
        if(connection->te == TE_CHUNKED ||
           (request->to >= 0 && connection->offset < request->to)) {
            do_log(L_ERROR, "Server dropped connection.\n");
            httpServerAbort(connection, 1, 502, 
                            internAtom("Server dropped connection"));
            return 1;
        } else {
            if(request->object->length < 0 && eof > 0 &&
               (request->to < 0 || request->to > request->object->size)) {
                request->object->length = request->object->size;
                objectMetadataChanged(request->object, 0);
            }
            httpServerFinish(connection, 1, 0);
            return 1;
        }
    } else {
        return httpServerReadData(connection, 0);
    }
}

int
httpServerIndirectHandler(int status,
                          FdEventHandlerPtr event, 
                          StreamRequestPtr srequest)
{
    HTTPConnectionPtr connection = srequest->data;
    assert(connection->request->object->flags & OBJECT_INPROGRESS);

    httpSetTimeout(connection, -1);
    if(status < 0) {
        if(status != -ECLIENTRESET)
            do_log_error(L_ERROR, -status, "Read from server failed");
        httpServerAbort(connection, status != -ECLIENTRESET, 502,
                        internAtomError(-status, "Read from server failed"));
        return 1;
    }

    connection->len = srequest->offset;

    return httpServerIndirectHandlerCommon(connection, status);
}

int
httpServerReadData(HTTPConnectionPtr connection, int immediate)
{
    HTTPRequestPtr request = connection->request;
    ObjectPtr object = request->object;
    int to = -1;

    assert(object->flags & OBJECT_INPROGRESS);

    if(request->request == NULL) {
        httpServerFinish(connection, 1, 0);
        return 1;
    }

    if(request->to >= 0)
        to = request->to;
    else
        to = object->length;

    if(to >= 0 && to == connection->offset) {
        httpServerFinish(connection, 0, 0);
        return 1;
    }

    if(connection->len == 0 &&
       ((connection->te == TE_IDENTITY && to > connection->offset) ||
        (connection->te == TE_CHUNKED && connection->chunk_remaining > 0))) {
        /* Read directly into the object */
        int i = connection->offset / CHUNK_SIZE;
        int j = connection->offset % CHUNK_SIZE;
        int end, len, more;
        /* See httpServerDirectHandlerCommon if you change this */
        if(connection->te == TE_CHUNKED) {
            len = connection->chunk_remaining;
            /* The logic here is that we want more to just fit the
               chunk header if we're doing a large read, but do a
               large read if we would otherwise do a small one.  The
               magic constant 2000 comes from the assumption that the
               server uses chunks that have a size that are a power of
               two (possibly including the chunk header), and that we
               want a full ethernet packet to fit into our read. */
            more = (len >= 2000 ? 20 : MIN(2048 - len, CHUNK_SIZE));
        } else {
            len = to - connection->offset;
            /* We read more data only when there is a reasonable
               chance of there being another reply coming. */
            more = (connection->pipelined > 1) ? CHUNK_SIZE : 0;
        }
        end = len + connection->offset;

        httpConnectionDestroyBuf(connection);

        /* The order of allocation is important in case we run out of
           memory. */
        lockChunk(object, i);
        if(object->chunks[i].data == NULL)
            object->chunks[i].data = get_chunk();
        if(object->chunks[i].data && object->chunks[i].size >= j) {
            if(len + j > CHUNK_SIZE) {
                lockChunk(object, i + 1);
                if(object->chunks[i + 1].data == NULL)
                    object->chunks[i + 1].data = get_chunk();
                /* Unless we're grabbing all len of data, we do not
                   want to do an indirect read immediately afterwards. */
                if(more && len + j <= 2 * CHUNK_SIZE) {
                    if(!connection->buf)
                        connection->buf = get_chunk(); /* checked below */
                }
                if(object->chunks[i + 1].data) {
                    do_stream_3(IO_READ | IO_NOTNOW, connection->fd, j,
                                object->chunks[i].data, CHUNK_SIZE,
                                object->chunks[i + 1].data,
                                MIN(CHUNK_SIZE,
                                    end - (i + 1) * CHUNK_SIZE),
                                connection->buf, connection->buf ? more : 0,
                                httpServerDirectHandler2, connection);
                    return 1;
                }
                unlockChunk(object, i + 1);
            }
            if(more && len + j <= CHUNK_SIZE) {
                if(!connection->buf)
                    connection->buf = get_chunk();
            }
            do_stream_2(IO_READ | IO_NOTNOW, connection->fd, j,
                        object->chunks[i].data,
                        MIN(CHUNK_SIZE, end - i * CHUNK_SIZE),
                        connection->buf, connection->buf ? more : 0,
                        httpServerDirectHandler, connection);
            return 1;
        } else {
            unlockChunk(object, i);
        }
    }
       
    if(connection->len == 0)
        httpConnectionDestroyBuf(connection);

    httpSetTimeout(connection, serverTimeout);
    do_stream_buf(IO_READ | IO_NOTNOW |
                  ((immediate && connection->len) ? IO_IMMEDIATE : 0),
                  connection->fd, connection->len,
                  &connection->buf,
                  (connection->te == TE_CHUNKED ? 
                   MIN(2048, CHUNK_SIZE) : CHUNK_SIZE),
                  httpServerIndirectHandler, connection);
    return 1;
}

int
httpServerDirectHandlerCommon(int kind, int status,
                              FdEventHandlerPtr event, 
                              StreamRequestPtr srequest)
{
    HTTPConnectionPtr connection = srequest->data;
    HTTPRequestPtr request = connection->request;
    ObjectPtr object = request->object;
    int i = connection->offset / CHUNK_SIZE;
    int to, end, end1;

    assert(request->object->flags & OBJECT_INPROGRESS);

    httpSetTimeout(connection, -1);

    if(status < 0) {
        unlockChunk(object, i);
        if(kind == 2) unlockChunk(object, i + 1);
        if(status != -ECLIENTRESET)
            do_log_error(L_ERROR, -status, "Read from server failed");
        httpServerAbort(connection, status != -ECLIENTRESET, 502,
                        internAtomError(-status, "Read from server failed"));
        return 1;
    }

    /* We have incestuous knowledge of the decisions made in
       httpServerReadData */
    if(request->to >= 0)
        to = request->to;
    else
        to = object->length;
    if(connection->te == TE_CHUNKED)
        end = connection->offset + connection->chunk_remaining;
    else
        end = to;
    /* The amount of data actually read into the object */
    end1 = MIN(end, i * CHUNK_SIZE + MIN(kind * CHUNK_SIZE, srequest->offset));

    assert(end >= 0);
    assert(end1 >= i * CHUNK_SIZE);
    assert(end1 - 2 * CHUNK_SIZE <= i * CHUNK_SIZE);

    object->chunks[i].size = 
        MAX(object->chunks[i].size, MIN(end1 - i * CHUNK_SIZE, CHUNK_SIZE));
    if(kind == 2 && end1 > (i + 1) * CHUNK_SIZE) {
        object->chunks[i + 1].size =
            MAX(object->chunks[i + 1].size, end1 - (i + 1) * CHUNK_SIZE);
    }
    if(connection->te == TE_CHUNKED) {
        connection->chunk_remaining -= (end1 - connection->offset);
        assert(connection->chunk_remaining >= 0);
    }
    connection->offset = end1;
    object->size = MAX(object->size, end1);
    unlockChunk(object, i);
    if(kind == 2) unlockChunk(object, i + 1);

    if(i * CHUNK_SIZE + srequest->offset > end1) {
        connection->len = i * CHUNK_SIZE + srequest->offset - end1;
        return httpServerIndirectHandlerCommon(connection, status);
    } else {
        notifyObject(object);
        if(status) {
            if(connection->te == TE_CHUNKED ||
               (end >= 0 && connection->offset < end)) {
                do_log(L_ERROR, "Server dropped connection.\n");
                httpServerAbort(connection, 1, 502, 
                                internAtom("Server dropped connection"));
            } else
                httpServerFinish(connection, 1, 0);
            return 1;
        } else {
            return httpServerReadData(connection, 0);
        }
    }
}

int
httpServerDirectHandler(int status,
                        FdEventHandlerPtr event, 
                        StreamRequestPtr srequest)
{
    return httpServerDirectHandlerCommon(1, status, event, srequest);
}
    
int
httpServerDirectHandler2(int status,
                         FdEventHandlerPtr event, 
                         StreamRequestPtr srequest)
{
    return httpServerDirectHandlerCommon(2, status, event, srequest);
}

/* Add the data accumulated in connection->buf into the object in
   connection->request.  Returns 0 in the normal case, 1 if the TE is
   self-terminating and we're done, -1 if there was a problem with
   objectAddData, -2 if there was a problem with the data. */
int
connectionAddData(HTTPConnectionPtr connection, int skip)
{
    HTTPRequestPtr request = connection->request;
    ObjectPtr object = request->object;
    int rc;

    if(connection->te == TE_IDENTITY) {
        int len;
        
        len = connection->len - skip;
        if(object->length >= 0) {
            len = MIN(object->length - connection->offset, len);
        }
        if(request->to >= 0)
            len = MIN(request->to - connection->offset, len);
        if(len > 0) {
            rc = objectAddData(object, connection->buf + skip,
                               connection->offset, len);
            if(rc < 0)
                return -1;
            connection->offset += len;
            connection->len -= (len + skip);
            do_log(D_SERVER_OFFSET, "0x%lx(0x%lx): offset = %d\n",
                   (unsigned long)connection, (unsigned long)object,
                   connection->offset);
        }

        if(connection->len > 0 && skip + len > 0) {
            memmove(connection->buf,
                    connection->buf + skip + len, connection->len);
        }

        if((object->length >= 0 && object->length <= connection->offset) ||
           (request->to >= 0 && request->to <= connection->offset)) {
            notifyObject(object);
            return 1;
        } else {
            if(len > 0)
                notifyObject(object);
            return 0;
        }
    } else if(connection->te == TE_CHUNKED) {
        int i = skip, j, size;
        /* connection->chunk_remaining is 0 at the end of a chunk, -1
           after the CR/LF pair ending a chunk, and -2 after we've
           seen a chunk of length 0. */
        if(connection->chunk_remaining > -2) {
            while(1) {
                if(connection->chunk_remaining <= 0) {
                    if(connection->chunk_remaining == 0) {
                        if(connection->len < i + 2)
                            break;
                        if(connection->buf[i] != '\r' ||
                           connection->buf[i + 1] != '\n')
                            return -1;
                        i += 2;
                        connection->chunk_remaining = -1;
                    }
                    if(connection->len < i + 2)
                        break;
                    j = parseChunkSize(connection->buf, i,
                                       connection->len, &size);
                    if(j < 0)
                        return -2;
                    if(j == 0)
                        break;
                    else
                        i = j;
                    if(size == 0) {
                        connection->chunk_remaining = -2;
                        break;
                    } else {
                        connection->chunk_remaining = size;
                    }
                } else {
                    /* connection->chunk_remaining > 0 */
                    size = MIN(connection->chunk_remaining,
                               connection->len - i);
                    if(size <= 0)
                        break;
                    rc = objectAddData(object, connection->buf + i,
                                       connection->offset, size);
                    connection->offset += size;
                    if(rc < 0)
                        return -1;
                    i += size;
                    connection->chunk_remaining -= size;
                    do_log(D_SERVER_OFFSET, "0x%lx(0x%lx): offset = %d\n",
                           (unsigned long)connection, 
                           (unsigned long)object,
                           connection->offset);
                }
            }
        }
        connection->len -= i;
        if(connection->len > 0)
            memmove(connection->buf, connection->buf + i, connection->len);
        if(i > 0 || connection->chunk_remaining == -2)
            notifyObject(object);
        if(connection->chunk_remaining == -2)
            return 1;
        else
            return 0;
    } else {
        abort();
    }
}

void
listServers(FILE *out)
{
    HTTPServerPtr server;
    int i, n, m, entry;

    fprintf(out, "<!DOCTYPE HTML PUBLIC "
            "\"-//W3C//DTD HTML 4.01 Transitional//EN\" "
            "\"http://www.w3.org/TR/html4/loose.dtd\">\n"
            "<html><head>\n"
            "\r\n<title>Known servers</title>\n"
           "</head><body>\n"
            "<h1>Known servers</h1>\n");

    alternatingHttpStyle(out, "servers");
    fprintf(out, "<table id=servers>\n");
    fprintf(out, "<thead><tr><th>Server</th>"
            "<th>Version</th>"
            "<th>Persistent</th>"
            "<th>Pipeline</th>"
            "<th>Connections</th>"
            "<th></th>"
            "<th>rtt</th>"
            "<th>rate</th>"
            "</tr></thead>\n");
    fprintf(out, "<tbody>\n");
    server = servers;
    entry = 0;
    while(server) {
        fprintf(out, "<tr class=\"%s\">", entry % 2 == 0 ? "even" : "odd");
        if(server->port == 80)
            fprintf(out, "<td>%s</td>", server->name);
        else
            fprintf(out, "<td>%s:%d</td>", server->name, server->port);

        if(server->version == HTTP_11)
            fprintf(out, "<td>1.1</td>");
        else if(server->version == HTTP_10)
            fprintf(out, "<td>1.0</td>");
        else
            fprintf(out, "<td>unknown</td>");

        if(server->persistent < 0)
            fprintf(out, "<td>no</td>");
        else if(server->persistent > 0)
            fprintf(out, "<td>yes</td>");
        else
            fprintf(out, "<td>unknown</td>");

        if(server->version != HTTP_11 || server->persistent <= 0)
            fprintf(out, "<td></td>");
        else if(server->pipeline < 0)
            fprintf(out, "<td>no</td>");
        else if(server->pipeline >= 0 && server->pipeline <= 1)
            fprintf(out, "<td>unknown</td>");
        else if(server->pipeline == 2 || server->pipeline == 3)
            fprintf(out, "<td>probing</td>");
        else 
            fprintf(out, "<td>yes</td>");

        n = 0; m = 0;
        for(i = 0; i < server->maxslots; i++)
            if(server->connection[i] && !server->connection[i]->connecting) {
                if(i < server->numslots)
                    n++;
                else
                    m++;
            }
            
        fprintf(out, "<td>%d/%d", n, server->numslots);
        if(m)
            fprintf(out, " + %d</td>", m);
        else
            fprintf(out, "</td>");

        if(server->lies > 0)
            fprintf(out, "<td>(%d lies)</td>", (server->lies + 9) / 10);
        else
            fprintf(out, "<td></td>");

        if(server->rtt > 0)
            fprintf(out, "<td>%.3f</td>", (double)server->rtt / 1000000.0);
        else
            fprintf(out, "<td></td>");
        if(server->rate > 0)
            fprintf(out, "<td>%d</td>", server->rate);
        else
            fprintf(out, "<td></td>");

        fprintf(out, "</tr>\n");
        server = server->next;
        entry++;
    }
    fprintf(out, "</tbody>\n");
    fprintf(out, "</table>\n");
    fprintf(out, "<p><a href=\"/polipo/\">back</a></p>");
    fprintf(out, "</body></html>\n");
}

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

int disableProxy = 0;
AtomPtr proxyName = NULL;
int proxyPort = 8123;

int clientTimeout = 120;
int serverTimeout = 90;
int serverIdleTimeout = 45;

int bigBufferSize = (32 * 1024);

AtomPtr displayName = NULL;

AtomPtr authRealm = NULL;
AtomPtr authCredentials = NULL;

AtomPtr parentAuthCredentials = NULL;

AtomListPtr allowedClients = NULL;
NetAddressPtr allowedNets = NULL;

IntListPtr allowedPorts = NULL;
IntListPtr tunnelAllowedPorts = NULL;
int expectContinue = 1;
int dontTrustVaryETag = 1;

AtomPtr atom100Continue;

int disableVia = 1;

/* 0 means that all failures lead to errors.  1 means that failures to
   connect are reported in a Warning header when stale objects are
   served.  2 means that only missing data is fetched from the net,
   stale data is served without revalidation (browser-side
   Cache-Control directives are still honoured).  3 means that no
   connections are ever attempted. */

int proxyOffline = 0;
int relaxTransparency = 0;
AtomPtr proxyAddress = NULL;

static int timeoutSetter(ConfigVariablePtr var, void *value);

void
preinitHttp()
{
    proxyAddress = internAtom("127.0.0.1");
    CONFIG_VARIABLE_SETTABLE(disableProxy, CONFIG_BOOLEAN, configIntSetter,
                             "Whether to be a web server only.");
    CONFIG_VARIABLE_SETTABLE(proxyOffline, CONFIG_BOOLEAN, configIntSetter,
                             "Avoid contacting remote servers.");
    CONFIG_VARIABLE_SETTABLE(relaxTransparency, CONFIG_TRISTATE, 
                             configIntSetter,
                             "Avoid contacting remote servers.");
    CONFIG_VARIABLE(proxyPort, CONFIG_INT,
                    "The TCP port on which the proxy listens.");
    CONFIG_VARIABLE(proxyAddress, CONFIG_ATOM_LOWER,
                    "The IP address on which the proxy listens.");
    CONFIG_VARIABLE_SETTABLE(proxyName, CONFIG_ATOM_LOWER, configAtomSetter,
                             "The name by which the proxy is known.");
    CONFIG_VARIABLE_SETTABLE(clientTimeout, CONFIG_TIME, 
                             timeoutSetter, "Client-side timeout.");
    CONFIG_VARIABLE_SETTABLE(serverTimeout, CONFIG_TIME,
                             timeoutSetter, "Server-side timeout.");
    CONFIG_VARIABLE_SETTABLE(serverIdleTimeout, CONFIG_TIME,
                             timeoutSetter, "Server-side idle timeout.");
    CONFIG_VARIABLE(authRealm, CONFIG_ATOM,
                    "Authentication realm.");
    CONFIG_VARIABLE(displayName, CONFIG_ATOM,
                    "Server name displayed on error pages.");
    CONFIG_VARIABLE(authCredentials, CONFIG_PASSWORD,
                    "username:password.");
    CONFIG_VARIABLE(parentAuthCredentials, CONFIG_PASSWORD,
                    "username:password.");
    CONFIG_VARIABLE(allowedClients, CONFIG_ATOM_LIST_LOWER,
                    "Networks from which clients are allowed to connect.");
    CONFIG_VARIABLE(tunnelAllowedPorts, CONFIG_INT_LIST,
                    "Ports to which tunnelled connections are allowed.");
    CONFIG_VARIABLE(allowedPorts, CONFIG_INT_LIST,
                    "Ports to which connections are allowed.");
    CONFIG_VARIABLE(expectContinue, CONFIG_TRISTATE,
                    "Send Expect-Continue to servers.");
    CONFIG_VARIABLE(bigBufferSize, CONFIG_INT,
                    "Size of big buffers (max size of headers).");
    CONFIG_VARIABLE_SETTABLE(disableVia, CONFIG_BOOLEAN, configIntSetter,
                             "Don't use Via headers.");
    CONFIG_VARIABLE(dontTrustVaryETag, CONFIG_TRISTATE,
                    "Whether to trust the ETag when there's Vary.");
    preinitHttpParser();
}

static int
timeoutSetter(ConfigVariablePtr var, void *value)
{
    configIntSetter(var, value);
    if(clientTimeout <= serverTimeout)
        clientTimeout = serverTimeout + 1;
    return 1;
}

void
initHttp()
{
    char *buf = NULL;
    int namelen;
    int n;
    struct hostent *host;

    initHttpParser();

    atom100Continue = internAtom("100-continue");

    if(clientTimeout <= serverTimeout) {
        clientTimeout = serverTimeout + 1;
        do_log(L_WARN, "Value of clientTimeout too small -- setting to %d.\n",
               clientTimeout);
    }

    if(displayName == NULL)
        displayName = internAtom("Polipo");

    if(authCredentials != NULL && authRealm == NULL)
        authRealm = internAtom("Polipo");

    if(allowedClients) {
        allowedNets = parseNetAddress(allowedClients);
        if(allowedNets == NULL)
            exit(1);
    }

    if(allowedPorts == NULL) {
        allowedPorts = makeIntList(0);
        if(allowedPorts == NULL) {
            do_log(L_ERROR, "Couldn't allocate allowedPorts.\n");
            exit(1);
        }
        intListCons(80, 100, allowedPorts);
        intListCons(1024, 0xFFFF, allowedPorts);
    }

    if(tunnelAllowedPorts == NULL) {
        tunnelAllowedPorts = makeIntList(0);
        if(tunnelAllowedPorts == NULL) {
            do_log(L_ERROR, "Couldn't allocate tunnelAllowedPorts.\n");
            exit(1);
        }
        intListCons(22, 22, tunnelAllowedPorts);   /* ssh */
        intListCons(80, 80, tunnelAllowedPorts);   /* HTTP */
        intListCons(109, 110, tunnelAllowedPorts); /* POP 2 and 3*/
        intListCons(143, 143, tunnelAllowedPorts); /* IMAP 2/4 */
        intListCons(443, 443, tunnelAllowedPorts); /* HTTP/SSL */
        intListCons(873, 873, tunnelAllowedPorts); /* rsync */
        intListCons(993, 993, tunnelAllowedPorts); /* IMAP/SSL */
        intListCons(995, 995, tunnelAllowedPorts); /* POP/SSL */
        intListCons(2401, 2401, tunnelAllowedPorts); /* CVS */
        intListCons(5222, 5223, tunnelAllowedPorts); /* Jabber */
        intListCons(9418, 9418, tunnelAllowedPorts); /* Git */
    }

    if(proxyName)
        return;

    buf = get_chunk();
    if(buf == NULL) {
        do_log(L_ERROR, "Couldn't allocate chunk for host name.\n");
        goto fail;
    }

    n = gethostname(buf, CHUNK_SIZE);
    if(n != 0) {
        do_log_error(L_WARN, errno, "Gethostname");
        strcpy(buf, "polipo");
        goto success;
    }
    /* gethostname doesn't necessarily NUL-terminate on overflow */
    buf[CHUNK_SIZE - 1] = '\0';

    if(strcmp(buf, "(none)") == 0 ||
       strcmp(buf, "localhost") == 0 ||
       strcmp(buf, "localhost.localdomain") == 0) {
        do_log(L_WARN, "Couldn't determine host name -- using ``polipo''.\n");
        strcpy(buf, "polipo");
        goto success;
    }

    if(strchr(buf, '.') != NULL)
        goto success;

    host = gethostbyname(buf);
    if(host == NULL) {
        goto success;
    }

    if(host->h_addrtype != AF_INET)
        goto success;

    host = gethostbyaddr(host->h_addr_list[0], host->h_length,  AF_INET);

    if(!host || !host->h_name || strcmp(host->h_name, "localhost") == 0 ||
       strcmp(host->h_name, "localhost.localdomain") == 0)
        goto success;

    namelen = strlen(host->h_name);
    if(namelen >= CHUNK_SIZE) {
        do_log(L_ERROR, "Host name too long.\n");
        goto success;
    }

    memcpy(buf, host->h_name, namelen + 1);

 success:
    proxyName = internAtom(buf);
    if(proxyName == NULL) {
        do_log(L_ERROR, "Couldn't allocate proxy name.\n");
        goto fail;
    }
    dispose_chunk(buf);
    return;

 fail:
    if(buf)
        dispose_chunk(buf);
    exit(1);
    return;
}

int
httpSetTimeout(HTTPConnectionPtr connection, int secs)
{
    TimeEventHandlerPtr new;

    if(connection->timeout)
        cancelTimeEvent(connection->timeout);
    connection->timeout = NULL;

    if(secs > 0) {
        new = scheduleTimeEvent(secs, httpTimeoutHandler,
                                sizeof(connection), &connection);
        if(!new) {
            do_log(L_ERROR, "Couldn't schedule timeout for connection 0x%lx\n",
                   (unsigned long)connection);
            return -1;
        }
    } else {
        new = NULL;
    }

    connection->timeout = new;
    return 1;
}

int 
httpTimeoutHandler(TimeEventHandlerPtr event)
{
    HTTPConnectionPtr connection = *(HTTPConnectionPtr*)event->data;

    if(connection->fd >= 0) {
        int rc;
        rc = shutdown(connection->fd, 2);
        if(rc < 0 && errno != ENOTCONN)
                do_log_error(L_ERROR, errno, "Timeout: shutdown failed");
        pokeFdEvent(connection->fd, -EDOTIMEOUT, POLLIN | POLLOUT);
    }
    connection->timeout = NULL;
    return 1;
}

int
httpWriteObjectHeaders(char *buf, int offset, int len,
                       ObjectPtr object, int from, int to)
{
    int n = offset;
    CacheControlRec cache_control;

    cache_control.flags = object->cache_control;
    cache_control.max_age = object->max_age;
    cache_control.s_maxage = object->s_maxage;
    cache_control.max_stale = -1;
    cache_control.min_fresh = -1;

    if(from <= 0 && to < 0) {
        if(object->length >= 0) {
            n = snnprintf(buf, n, len,
                          "\r\nContent-Length: %d", object->length);
        }
    } else {
        if(to >= 0) {
            n = snnprintf(buf, n, len,
                          "\r\nContent-Length: %d", to - from);
        }
    }

    if(from > 0 || to > 0) {
        if(object->length >= 0) {
            if(from >= to) {
                n = snnprintf(buf, n, len,
                              "\r\nContent-Range: bytes */%d",
                              object->length);
            } else {
                n = snnprintf(buf, n, len,
                              "\r\nContent-Range: bytes %d-%d/%d",
                              from, to - 1, 
                              object->length);
            }
        } else {
            if(to >= 0) {
                n = snnprintf(buf, n, len,
                              "\r\nContent-Range: bytes %d-/*",
                              from);
            } else {
                n = snnprintf(buf, n, len,
                              "\r\nContent-Range: bytes %d-%d/*",
                              from, to);
            }
        }
    }
        
    if(object->etag) {
        n = snnprintf(buf, n, len, "\r\nETag: \"%s\"", object->etag);
    }
    if((object->flags & OBJECT_LOCAL) || object->date >= 0) {
        n = snnprintf(buf, n, len, "\r\nDate: ");
        n = format_time(buf, n, len, 
                        (object->flags & OBJECT_LOCAL) ?
                        current_time.tv_sec : object->date);
        if(n < 0)
            goto fail;
    }

    if(object->last_modified >= 0) {
        n = snnprintf(buf, n, len, "\r\nLast-Modified: ");
        n = format_time(buf, n, len, object->last_modified);
        if(n < 0)
            goto fail;
    }

    if(object->expires >= 0) {
        n = snnprintf(buf, n, len, "\r\nExpires: ");
        n = format_time(buf, n, len, object->expires);
        if(n < 0)
            goto fail;
    }

    n = httpPrintCacheControl(buf, n, len,
                              object->cache_control, &cache_control);
    if(n < 0)
        goto fail;

    if(!disableVia && object->via)
        n = snnprintf(buf, n, len, "\r\nVia: %s", object->via->string);

    if(object->headers)
        n = snnprint_n(buf, n, len, object->headers->string,
                       object->headers->length);

    if(n < len)
        return n;
    else
        return -1;

 fail:
    return -1;
}

static int
cachePrintSeparator(char *buf, int offset, int len,
                    int subsequent)
{
    int n = offset;
    if(subsequent)
        n = snnprintf(buf, offset, len, ", ");
    else
        n = snnprintf(buf, offset, len, "\r\nCache-Control: ");
    return n;
}

int
httpPrintCacheControl(char *buf, int offset, int len,
                      int flags, CacheControlPtr cache_control)
{
    int n = offset;
    int sub = 0;

#define PRINT_SEP() \
    do {\
        n = cachePrintSeparator(buf, n, len, sub); \
        sub = 1; \
    } while(0)

    if(cache_control)
        flags |= cache_control->flags;

    if(flags & CACHE_NO) {
        PRINT_SEP();
        n = snnprintf(buf, n, len, "no-cache");
    }
    if(flags & CACHE_PUBLIC) {
        PRINT_SEP();
        n = snnprintf(buf, n, len, "public");
    }
    if(flags & CACHE_PRIVATE) {
        PRINT_SEP();
        n = snnprintf(buf, n, len, "private");
    }
    if(flags & CACHE_NO_STORE) {
        PRINT_SEP();
        n = snnprintf(buf, n, len, "no-store");
    }
    if(flags & CACHE_NO_TRANSFORM) {
        PRINT_SEP();
        n = snnprintf(buf, n, len, "no-transform");
    }
    if(flags & CACHE_MUST_REVALIDATE) {
        PRINT_SEP();
        n = snnprintf(buf, n, len, "must-revalidate");
    }
    if(flags & CACHE_PROXY_REVALIDATE) {
        PRINT_SEP();
        n = snnprintf(buf, n, len, "proxy-revalidate");
    }
    if(flags & CACHE_ONLY_IF_CACHED) {
        PRINT_SEP();
        n = snnprintf(buf, n, len, "only-if-cached");
    }
    if(cache_control) {
        if(cache_control->max_age >= 0) {
            PRINT_SEP();
            n = snnprintf(buf, n, len, "max-age=%d",
                          cache_control->max_age);
        }
        if(cache_control->s_maxage >= 0) {
            PRINT_SEP();
            n = snnprintf(buf, n, len, "s-maxage=%d", 
                          cache_control->s_maxage);
        }
        if(cache_control->min_fresh > 0) {
            PRINT_SEP();
            n = snnprintf(buf, n, len, "min-fresh=%d",
                          cache_control->min_fresh);
        }
        if(cache_control->max_stale > 0) {
            PRINT_SEP();
            n = snnprintf(buf, n, len, "max-stale=%d",
                          cache_control->min_fresh);
        }
    }
    return n;
#undef PRINT_SEP
}

char *
httpMessage(int code)
{
    switch(code) {
    case 200:
        return "Okay";
    case 206:
        return "Partial content";
    case 300:
        return "Multiple choices";
    case 301:
        return "Moved permanently";
    case 302:
        return "Found";
    case 303:
        return "See other";
    case 304:
        return "Not changed";
    case 307:
        return "Temporary redirect";
    case 401:
        return "Authentication Required";
    case 403:
        return "Forbidden";
    case 404:
        return "Not found";
    case 405:
        return "Method not allowed";
    case 407:
        return "Proxy authentication required";
    default:
        return "Unknown error code";
    }
}

int
htmlString(char *buf, int n, int len, char *s, int slen)
{
    int i = 0;
    while(i < slen && n + 5 < len) {
        switch(s[i]) {
        case '&':
            buf[n++] = '&'; buf[n++] = 'a'; buf[n++] = 'm'; buf[n++] = 'p';
            buf[n++] = ';';
            break;
        case '<':
            buf[n++] = '&'; buf[n++] = 'l'; buf[n++] = 't'; buf[n++] = ';';
            break;
        case '>':
            buf[n++] = '&'; buf[n++] = 'g'; buf[n++] = 't'; buf[n++] = ';';
            break;
        case '"':
            buf[n++] = '&'; buf[n++] = 'q'; buf[n++] = 'u'; buf[n++] = 'o';
            buf[n++] = 't'; buf[n++] = ';';
            break;
        case '\0':
            break;
        default:
            buf[n++] = s[i];
        }
        i++;
    }
    return n;
}

void
htmlPrint(FILE *out, char *s, int slen)
{
    int i;
    for(i = 0; i < slen; i++) {
        switch(s[i]) {
        case '&':
            fputs("&amp;", out);
            break;
        case '<':
            fputs("&lt;", out);
            break;
        case '>':
            fputs("&gt;", out);
            break;
        default:
            fputc(s[i], out);
        }
    }
}

HTTPConnectionPtr
httpMakeConnection()
{
    HTTPConnectionPtr connection;
    connection = malloc(sizeof(HTTPConnectionRec));
    if(connection == NULL)
        return NULL;
    connection->flags = 0;
    connection->fd = -1;
    connection->buf = NULL;
    connection->len = 0;
    connection->offset = 0;
    connection->request = NULL;
    connection->request_last = NULL;
    connection->serviced = 0;
    connection->version = HTTP_UNKNOWN;
    connection->time = current_time.tv_sec;
    connection->timeout = NULL;
    connection->te = TE_IDENTITY;
    connection->reqbuf = NULL;
    connection->reqlen = 0;
    connection->reqbegin = 0;
    connection->reqoffset = 0;
    connection->bodylen = -1;
    connection->reqte = TE_IDENTITY;
    connection->chunk_remaining = 0;
    connection->server = NULL;
    connection->pipelined = 0;
    connection->connecting = 0;
    connection->server = NULL;
    return connection;
}

void
httpDestroyConnection(HTTPConnectionPtr connection)
{
    assert(connection->flags == 0);
    httpConnectionDestroyBuf(connection);
    assert(!connection->request);
    assert(!connection->request_last);
    httpConnectionDestroyReqbuf(connection);
    assert(!connection->timeout);
    assert(!connection->server);
    free(connection);
}

void
httpConnectionDestroyBuf(HTTPConnectionPtr connection)
{
    if(connection->buf) {
        if(connection->flags & CONN_BIGBUF)
            free(connection->buf);
        else
            dispose_chunk(connection->buf);
    }
    connection->flags &= ~CONN_BIGBUF;
    connection->buf = NULL;
}

void
httpConnectionDestroyReqbuf(HTTPConnectionPtr connection)
{
    if(connection->reqbuf) {
        if(connection->flags & CONN_BIGREQBUF)
            free(connection->reqbuf);
        else
            dispose_chunk(connection->reqbuf);
    }
    connection->flags &= ~CONN_BIGREQBUF;
    connection->reqbuf = NULL;
}

HTTPRequestPtr 
httpMakeRequest()
{
    HTTPRequestPtr request;
    request = malloc(sizeof(HTTPRequestRec));
    if(request == NULL)
        return NULL;
    request->flags = 0;
    request->connection = NULL;
    request->object = NULL;
    request->method = METHOD_UNKNOWN;
    request->from = 0;
    request->to = -1;
    request->cache_control = no_cache_control;
    request->condition = NULL;
    request->via = NULL;
    request->chandler = NULL;
    request->can_mutate = NULL;
    request->error_code = 0;
    request->error_message = NULL;
    request->error_headers = NULL;
    request->headers = NULL;
    request->time0 = null_time;
    request->time1 = null_time;
    request->request = NULL;
    request->next = NULL;
    return request;
}

void
httpDestroyRequest(HTTPRequestPtr request)
{
    if(request->object)
        releaseObject(request->object);
    if(request->condition)
        httpDestroyCondition(request->condition);
    releaseAtom(request->via);
    assert(request->chandler == NULL);
    releaseAtom(request->error_message);
    releaseAtom(request->headers);
    releaseAtom(request->error_headers);
    assert(request->request == NULL);
    assert(request->next == NULL);
    free(request);
}

void
httpQueueRequest(HTTPConnectionPtr connection, HTTPRequestPtr request)
{
    assert(request->next == NULL && request->connection == NULL);
    request->connection = connection;
    if(connection->request_last) {
        assert(connection->request);
        connection->request_last->next = request;
        connection->request_last = request;
    } else {
        assert(!connection->request_last);
        connection->request = request;
        connection->request_last = request;
    }
}

HTTPRequestPtr
httpDequeueRequest(HTTPConnectionPtr connection)
{
    HTTPRequestPtr request = connection->request;
    if(request) {
        assert(connection->request_last);
        connection->request = request->next;
        if(!connection->request) connection->request_last = NULL;
        request->next = NULL;
    }
    return request;
}

int
httpConnectionBigify(HTTPConnectionPtr connection)
{
    char *bigbuf;
    assert(!(connection->flags & CONN_BIGBUF));

    if(bigBufferSize <= CHUNK_SIZE)
        return 0;

    bigbuf = malloc(bigBufferSize);
    if(bigbuf == NULL)
        return -1;
    if(connection->len > 0)
        memcpy(bigbuf, connection->buf, connection->len);
    if(connection->buf)
        dispose_chunk(connection->buf);
    connection->buf = bigbuf;
    connection->flags |= CONN_BIGBUF;
    return 1;
}

int
httpConnectionBigifyReqbuf(HTTPConnectionPtr connection)
{
    char *bigbuf;
    assert(!(connection->flags & CONN_BIGREQBUF));

    if(bigBufferSize <= CHUNK_SIZE)
        return 0;

    bigbuf = malloc(bigBufferSize);
    if(bigbuf == NULL)
        return -1;
    if(connection->reqlen > 0)
        memcpy(bigbuf, connection->reqbuf, connection->reqlen);
    if(connection->reqbuf)
        dispose_chunk(connection->reqbuf);
    connection->reqbuf = bigbuf;
    connection->flags |= CONN_BIGREQBUF;
    return 1;
}

int
httpConnectionUnbigify(HTTPConnectionPtr connection)
{
    char *buf;
    assert(connection->flags & CONN_BIGBUF);
    assert(connection->len < CHUNK_SIZE);

    buf = get_chunk();
    if(buf == NULL)
        return -1;
    if(connection->len > 0)
        memcpy(buf, connection->buf, connection->len);
    free(connection->buf);
    connection->buf = buf;
    connection->flags &= ~CONN_BIGBUF;
    return 1;
}

int
httpConnectionUnbigifyReqbuf(HTTPConnectionPtr connection)
{
    char *buf;
    assert(connection->flags & CONN_BIGREQBUF);
    assert(connection->reqlen < CHUNK_SIZE);

    buf = get_chunk();
    if(buf == NULL)
        return -1;
    if(connection->reqlen > 0)
        memcpy(buf, connection->reqbuf, connection->reqlen);
    free(connection->reqbuf);
    connection->reqbuf = buf;
    connection->flags &= ~CONN_BIGREQBUF;
    return 1;
}

HTTPConditionPtr 
httpMakeCondition()
{
    HTTPConditionPtr condition;
    condition = malloc(sizeof(HTTPConditionRec));
    if(condition == NULL)
        return NULL;
    condition->ims = -1;
    condition->inms = -1;
    condition->im = NULL;
    condition->inm = NULL;
    condition->ifrange = NULL;
    return condition;
}

void
httpDestroyCondition(HTTPConditionPtr condition)
{
    if(condition->inm)
        free(condition->inm);
    if(condition->im)
        free(condition->im);
    if(condition->ifrange)
        free(condition->ifrange);
    free(condition);
}
        
int
httpCondition(ObjectPtr object, HTTPConditionPtr condition)
{
    int rc = CONDITION_MATCH;

    assert(!(object->flags & OBJECT_INITIAL));

    if(!condition) return CONDITION_MATCH;

    if(condition->ims >= 0) {
        if(object->last_modified < 0 ||
           condition->ims < object->last_modified)
            return rc;
        else
            rc = CONDITION_NOT_MODIFIED;
    }

    if(condition->inms >= 0) {
        if(object->last_modified < 0 || 
           condition->inms >= object->last_modified)
            return rc;
        else
            rc = CONDITION_FAILED;
    }

    if(condition->inm) {
        if(!object->etag || strcmp(object->etag, condition->inm) != 0)
            return rc;
        else
            rc = CONDITION_NOT_MODIFIED;
    }

    if(condition->im) {
        if(!object->etag || strcmp(object->etag, condition->im) != 0)
            rc = CONDITION_FAILED;
        else
            return rc;
    }

    return rc;
}

int
httpWriteErrorHeaders(char *buf, int size, int offset, int do_body,
                      int code, AtomPtr message, int close, AtomPtr headers,
                      char *url, int url_len, char *etag)
{
    int n, m, i;
    char *body;
    char htmlMessage[100];
    char timeStr[100];

    assert(code != 0);

    i = htmlString(htmlMessage, 0, 100, message->string, message->length);
    if(i < 0)
        strcpy(htmlMessage, "(Couldn't format message)");
    else
        htmlMessage[MIN(i, 99)] = '\0';

    if(code != 304) {
        body = get_chunk();
        if(!body) {
            do_log(L_ERROR, "Couldn't allocate body buffer.\n");
            return -1;
        }
        m = snnprintf(body, 0, CHUNK_SIZE,
                      "<!DOCTYPE HTML PUBLIC "
                      "\"-//W3C//DTD HTML 4.01 Transitional//EN\" "
                      "\"http://www.w3.org/TR/html4/loose.dtd\">"
                      "\n<html><head>"
                      "\n<title>Proxy %s: %3d %s.</title>"
                      "\n</head><body>"
                      "\n<h1>%3d %s</h1>"
                      "\n<p>The following %s",
                      code >= 400 ? "error" : "result",
                      code, htmlMessage,
                      code, htmlMessage,
                      code >= 400 ? 
                      "error occurred" :
                      "status was returned");
        if(url_len > 0) {
            m = snnprintf(body, m, CHUNK_SIZE,
                          " while trying to access <strong>");
            m = htmlString(body, m, CHUNK_SIZE, url, url_len);
            m = snnprintf(body, m, CHUNK_SIZE, "</strong>");
        }

        {
            /* On BSD systems, tv_sec is a long. */
            const time_t ct = current_time.tv_sec;
                                             /*Mon, 24 Sep 2004 17:46:35 GMT*/
            strftime(timeStr, sizeof(timeStr), "%a, %d %b %Y %H:%M:%S %Z",
                     localtime(&ct));
        }
        
        m = snnprintf(body, m, CHUNK_SIZE,
                      ":<br><br>"
                      "\n<strong>%3d %s</strong></p>"
                      "\n<hr>Generated %s by %s on <em>%s:%d</em>."
                      "\n</body></html>\r\n",
                      code, htmlMessage,
                      timeStr, displayName->string, proxyName->string, proxyPort);
        if(m <= 0 || m >= CHUNK_SIZE) {
            do_log(L_ERROR, "Couldn't write error body.\n");
            dispose_chunk(body);
            return -1;
        }
    } else {
        body = NULL;
        m = 0;
    }

    n = snnprintf(buf, 0, size,
                 "HTTP/1.1 %3d %s"
                 "\r\nConnection: %s"
                 "\r\nDate: ",
                  code, atomString(message),
                  close ? "close" : "keep-alive");
    n = format_time(buf, n, size, current_time.tv_sec);
    if(code != 304) {
        n = snnprintf(buf, n, size,
                      "\r\nContent-Type: text/html"
                      "\r\nContent-Length: %d", m);
    } else {
        if(etag)
            n = snnprintf(buf, n, size, "\r\nETag: \"%s\"", etag);
    }

    if(code != 304 && code != 412) {
        n = snnprintf(buf, n, size,
                      "\r\nExpires: 0"
                      "\r\nCache-Control: no-cache"
                      "\r\nPragma: no-cache");
    }

    if(headers)
        n = snnprint_n(buf, n, size,
                      headers->string, headers->length);

    n = snnprintf(buf, n, size, "\r\n\r\n");

    if(n < 0 || n >= size) {
        do_log(L_ERROR, "Couldn't write error.\n");
        dispose_chunk(body);
        return -1;
    }

    if(code != 304 && do_body) {
        if(m > 0) memcpy(buf + n, body, m);
        n += m;
    }

    if(body)
        dispose_chunk(body);

    return n;
}

AtomListPtr
urlDecode(char *buf, int n)
{
    char mybuf[500];
    int i, j = 0;
    AtomListPtr list;
    AtomPtr atom;

    list = makeAtomList(NULL, 0);
    if(list == NULL)
        return NULL;

    i = 0;
    while(i < n) {
        if(buf[i] == '%') {
            int a, b;
            if(i + 3 > n)
                goto fail;
            a = h2i(buf[i + 1]);
            b = h2i(buf[i + 2]);
            if(a < 0 || b < 0)
                goto fail;
            mybuf[j++] = (char)((a << 4) | b);
            i += 3;
            if(j >= 500) goto fail;
        } else if(buf[i] == '&') {
            atom = internAtomN(mybuf, j);
            if(atom == NULL)
                goto fail;
            atomListCons(atom, list);
            j = 0;
            i++;
        } else {
            mybuf[j++] = buf[i++];
            if(j >= 500) goto fail;
        }
    }

    atom = internAtomN(mybuf, j);
    if(atom == NULL)
        goto fail;
    atomListCons(atom, list);
    return list;

 fail:
    destroyAtomList(list);
    return NULL;
}

void
httpTweakCachability(ObjectPtr object)
{
    int code = object->code;

    if((object->cache_control & CACHE_AUTHORIZATION) &&
       !(object->cache_control & CACHE_PUBLIC)) {
        object->cache_control |= CACHE_NO_HIDDEN;
        object->flags |= OBJECT_LINEAR;
    }

    /* This is not required by RFC 2616 -- but see RFC 3143 2.1.1.  We
       manically avoid caching replies that we don't know how to
       handle, even if Expires or Cache-Control says otherwise.  As to
       known uncacheable replies, we obey Cache-Control and default to
       allowing sharing but not caching. */
    if(code != 200 && code != 206 && 
       code != 300 && code != 301 && code != 302 && code != 303 &&
       code != 304 && code != 307 &&
       code != 403 && code != 404 && code != 405 && code != 416) {
        object->cache_control |= (CACHE_NO_HIDDEN | CACHE_MISMATCH);
        object->flags |= OBJECT_LINEAR;
    } else if(code != 200 && code != 206 &&
              code != 300 && code != 301 && code != 304 &&
              code != 410) {
        if(object->expires < 0 && !(object->cache_control & CACHE_PUBLIC)) {
            object->cache_control |= CACHE_NO_HIDDEN;
        }
    } else if(dontCacheRedirects && (code == 301 || code == 302)) {
        object->cache_control |= CACHE_NO_HIDDEN;
    }

    if(urlIsUncachable(object->key, object->key_size)) {
        object->cache_control |= CACHE_NO_HIDDEN;
    }

    if((object->cache_control & CACHE_NO_STORE) != 0) {
        object->cache_control |= CACHE_NO_HIDDEN;
    }

    if(object->cache_control & CACHE_VARY) {
        if(!object->etag || dontTrustVaryETag >= 2) {
            object->cache_control |= CACHE_MISMATCH;
        }
    }
}

int
httpHeaderMatch(AtomPtr header, AtomPtr headers1, AtomPtr headers2)
{
    int rc1, b1, e1, rc2, b2, e2;

    /* Short cut if both sets of headers are identical */
    if(headers1 == headers2)
        return 1;

    rc1 = httpFindHeader(header, headers1->string, headers1->length,
                         &b1, &e1);
    rc2 = httpFindHeader(header, headers2->string, headers2->length,
                         &b2, &e2);

    if(rc1 == 0 && rc2 == 0)
        return 1;

    if(rc1 == 0 || rc2 == 0)
        return 0;

    if(e1 - b1 != e2 - b2)
        return 0;

    if(memcmp(headers1->string + b1, headers2->string + b2, e1 - b1) != 0)
        return 0;

    return 1;
}

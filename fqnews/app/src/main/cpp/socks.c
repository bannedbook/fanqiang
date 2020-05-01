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

#ifdef NO_SOCKS

AtomPtr socksParentProxy = NULL;

void
preinitSocks()
{
    return;
}

void
initSocks()
{
    return;
}

int
do_socks_connect(char *name, int port,
                 int (*handler)(int, SocksRequestPtr),
                 void *data)
{
    SocksRequestRec request;
    request.name = internAtomLowerN(name, strlen(name));
    request.port = port;
    request.handler = handler;
    request.buf = NULL;
    request.data = data;

    handler(-ENOSYS, &request);
    releaseAtom(request.name);
    return 1;
}

#else

AtomPtr socksParentProxy = NULL;
AtomPtr socksProxyHost = NULL;
int socksProxyPort = -1;
AtomPtr socksProxyAddress = NULL;
int socksProxyAddressIndex = -1;
AtomPtr socksUserName = NULL;
AtomPtr socksProxyType = NULL;
AtomPtr aSocks4a, aSocks5;

static int socksParentProxySetter(ConfigVariablePtr, void*);
static int socksProxyTypeSetter(ConfigVariablePtr, void*);
static int do_socks_connect_common(SocksRequestPtr);
static int socksDnsHandler(int, GethostbynameRequestPtr);
static int socksConnectHandler(int, FdEventHandlerPtr, ConnectRequestPtr);
static int socksWriteHandler(int, FdEventHandlerPtr, StreamRequestPtr);
static int socksReadHandler(int, FdEventHandlerPtr, StreamRequestPtr);
static int socks5ReadHandler(int, FdEventHandlerPtr, StreamRequestPtr);
static int socks5WriteHandler(int, FdEventHandlerPtr, StreamRequestPtr);
static int socks5ReadHandler2(int, FdEventHandlerPtr, StreamRequestPtr);

void
preinitSocks()
{
    aSocks4a = internAtom("socks4a");
    aSocks5 = internAtom("socks5");
    socksProxyType = retainAtom(aSocks5);
    socksUserName = internAtom("");
    CONFIG_VARIABLE_SETTABLE(socksParentProxy, CONFIG_ATOM_LOWER,
                             socksParentProxySetter,
                             "SOCKS parent proxy (host:port)");
    CONFIG_VARIABLE_SETTABLE(socksUserName, CONFIG_ATOM,
                             configAtomSetter,
                             "SOCKS4a user name");
    CONFIG_VARIABLE_SETTABLE(socksProxyType, CONFIG_ATOM_LOWER,
                             socksProxyTypeSetter,
                             "One of socks4a or socks5");
}

static int
socksParentProxySetter(ConfigVariablePtr var, void *value)
{
    configAtomSetter(var, value);
    initSocks();
    return 1;
}

static int
socksProxyTypeSetter(ConfigVariablePtr var, void *value)
{
    if(*var->value.a != aSocks4a && *var->value.a != aSocks5) {
        do_log(L_ERROR, "Unknown socksProxyType %s\n", (*var->value.a)->string);
        return -1;
    }

    return configAtomSetter(var, value);
}

void
initSocks()
{
    int port = -1;
    AtomPtr host = NULL, port_atom;
    int rc;

    if(socksParentProxy != NULL && socksParentProxy->length == 0) {
        releaseAtom(socksParentProxy);
        socksParentProxy = NULL;
    }

    if(socksParentProxy) {
        rc = atomSplit(socksParentProxy, ':', &host, &port_atom);
        if(rc <= 0) {
            do_log(L_ERROR, "Couldn't parse socksParentProxy");
            exit(1);
        }
        port = atoi(port_atom->string);
        releaseAtom(port_atom);
    }

    if(socksProxyHost)
        releaseAtom(socksProxyHost);
    socksProxyHost = host;
    socksProxyPort = port;
    if(socksProxyAddress)
        releaseAtom(socksProxyAddress);
    socksProxyAddress = NULL;
    socksProxyAddressIndex = -1;

    if(socksProxyType != aSocks4a && socksProxyType != aSocks5) {
        do_log(L_ERROR, "Unknown socksProxyType %s\n", socksProxyType->string);
        exit(1);
    }
}

static void
destroySocksRequest(SocksRequestPtr request)
{
    releaseAtom(request->name);
    if(request->buf)
        free(request->buf);
    free(request);
}

int
do_socks_connect(char *name, int port,
                 int (*handler)(int, SocksRequestPtr),
                 void *data)
{
    SocksRequestPtr request = malloc(sizeof(SocksRequestRec));
    SocksRequestRec request_nomem;
    if(request == NULL)
        goto nomem;

    request->name = internAtomLowerN(name, strlen(name));
    if(request->name == NULL) {
        free(request);
        goto nomem;
    }

    request->port = port;
    request->fd = -1;
    request->handler = handler;
    request->buf = NULL;
    request->data = data;

    if(socksProxyAddress == NULL) {
        do_gethostbyname(socksProxyHost->string, 0,
                         socksDnsHandler,
                         request);
        return 1;
    }

    return do_socks_connect_common(request);

 nomem:
    request_nomem.name = internAtomLowerN(name, strlen(name));
    request_nomem.port = port;
    request_nomem.handler = handler;
    request_nomem.buf = NULL;
    request_nomem.data = data;

    handler(-ENOMEM, &request_nomem);
    releaseAtom(request_nomem.name);
    return 1;
}

static int
do_socks_connect_common(SocksRequestPtr request)
{
    assert(socksProxyAddressIndex >= 0);

    do_connect(retainAtom(socksProxyAddress),
               socksProxyAddressIndex,
               socksProxyPort,
               socksConnectHandler, request);
    return 1;
}

static int
socksDnsHandler(int status, GethostbynameRequestPtr grequest)
{
    SocksRequestPtr request = grequest->data;
    if(status <= 0) {
        request->handler(status, request);
        destroySocksRequest(request);
        return 1;
    }

    if(grequest->addr->string[0] == DNS_CNAME) {
        if(grequest->count > 10) {
            do_log(L_ERROR, "DNS CNAME loop.\n");
            request->handler(-EDNS_CNAME_LOOP, request);
            destroySocksRequest(request);
            return 1;
        }
        do_gethostbyname(grequest->addr->string + 1, grequest->count + 1,
                         socksDnsHandler, request);
        return 1;
    }


    socksProxyAddress = retainAtom(grequest->addr);
    socksProxyAddressIndex = 0;

    do_socks_connect_common(request);
    return 1;
}

static int
socksConnectHandler(int status,
                    FdEventHandlerPtr event,
                    ConnectRequestPtr crequest)
{
    SocksRequestPtr request = crequest->data;
    int rc;

    if(status < 0) {
        request->handler(status, request);
        destroySocksRequest(request);
        return 1;
    }

    assert(request->fd < 0);
    request->fd = crequest->fd;
    socksProxyAddressIndex = crequest->index;

    rc = setNodelay(request->fd, 1);
    if(rc < 0)
        do_log_error(L_WARN, errno, "Couldn't disable Nagle's algorithm");

    if(socksProxyType == aSocks4a) {
        request->buf = malloc(8 +
                              socksUserName->length + 1 +
                              request->name->length + 1);
        if(request->buf == NULL) {
            CLOSE(request->fd);
            request->fd = -1;
            request->handler(-ENOMEM, request);
            destroySocksRequest(request);
            return 1;
        }

        request->buf[0] = 4;        /* VN */
        request->buf[1] = 1;        /* CD = REQUEST */
        request->buf[2] = (request->port >> 8) & 0xFF;
        request->buf[3] = request->port & 0xFF;
        request->buf[4] = request->buf[5] = request->buf[6] = 0;
        request->buf[7] = 3;

        memcpy(request->buf + 8, socksUserName->string, socksUserName->length);
        request->buf[8 + socksUserName->length] = '\0';

        memcpy(request->buf + 8 + socksUserName->length + 1,
               request->name->string, request->name->length);
        request->buf[8 + socksUserName->length + 1 + request->name->length] =
            '\0';

        do_stream(IO_WRITE, request->fd, 0, request->buf,
                  8 + socksUserName->length + 1 + request->name->length + 1,
                  socksWriteHandler, request);
    } else if(socksProxyType == aSocks5) {
        request->buf = malloc(8); /* 8 needed for the subsequent read */
        if(request->buf == NULL) {
            CLOSE(request->fd);
            request->fd = -1;
            request->handler(-ENOMEM, request);
            destroySocksRequest(request);
            return 1;
        }

        request->buf[0] = 5;             /* ver */
        request->buf[1] = 1;             /* nmethods */
        request->buf[2] = 0;             /* no authentication required */
        do_stream(IO_WRITE, request->fd, 0, request->buf, 3,
                  socksWriteHandler, request);
    } else {
        request->handler(-EUNKNOWN, request);
    }
    return 1;
}

static int
socksWriteHandler(int status,
                  FdEventHandlerPtr event,
                  StreamRequestPtr srequest)
{
    SocksRequestPtr request = srequest->data;

    if(status < 0)
        goto error;

    if(!streamRequestDone(srequest)) {
        if(status) {
            status = -ESOCKS_PROTOCOL;
            goto error;
        }
        return 0;
    }

    do_stream(IO_READ | IO_NOTNOW, request->fd, 0, request->buf, 8,
              socksProxyType == aSocks5 ?
              socks5ReadHandler : socksReadHandler,
              request);
    return 1;

 error:
    CLOSE(request->fd);
    request->fd = -1;
    request->handler(status, request);
    destroySocksRequest(request);
    return 1;
}

static int
socksReadHandler(int status,
                 FdEventHandlerPtr event,
                 StreamRequestPtr srequest)
{
    SocksRequestPtr request = srequest->data;

    if(status < 0)
        goto error;

    if(srequest->offset < 8) {
        if(status) {
            status = -ESOCKS_PROTOCOL;
            goto error;
        }
        return 0;
    }

    if(request->buf[0] != 0 || request->buf[1] != 90) {
        if(request->buf[1] >= 91 && request->buf[1] <= 93)
            status = -(ESOCKS_PROTOCOL + request->buf[1] - 90);
        else
            status = -ESOCKS_PROTOCOL;
        goto error;
    }

    request->handler(1, request);
    destroySocksRequest(request);
    return 1;

 error:
    CLOSE(request->fd);
    request->fd = -1;
    request->handler(status, request);
    destroySocksRequest(request);
    return 1;
}

static int
socks5ReadHandler(int status,
                  FdEventHandlerPtr event,
                  StreamRequestPtr srequest)
{
    SocksRequestPtr request = srequest->data;

    if(status < 0)
        goto error;

    if(srequest->offset < 2) {
        if(status) {
            status = -ESOCKS_PROTOCOL;
            goto error;
        }
        return 0;
    }

    if(request->buf[0] != 5 || request->buf[1] != 0) {
        status = -ESOCKS_PROTOCOL;
        goto error;
    }

    free(request->buf);
    request->buf = malloc(5 + request->name->length + 2);
    if(request->buf == NULL) {
        status = -ENOMEM;
        goto error;
    }

    request->buf[0] = 5;        /* ver */
    request->buf[1] = 1;        /* cmd */
    request->buf[2] = 0;        /* rsv */
    request->buf[3] = 3;        /* atyp */
    request->buf[4] = request->name->length;
    memcpy(request->buf + 5, request->name->string, request->name->length);
    request->buf[5 + request->name->length] = (request->port >> 8) & 0xFF;
    request->buf[5 + request->name->length + 1] = request->port & 0xFF;

    do_stream(IO_WRITE, request->fd, 0,
              request->buf, 5 + request->name->length + 2,
              socks5WriteHandler, request);
    return 1;

 error:
    CLOSE(request->fd);
    request->fd = -1;
    request->handler(status, request);
    destroySocksRequest(request);
    return 1;
}

static int
socks5WriteHandler(int status,
                   FdEventHandlerPtr event,
                   StreamRequestPtr srequest)
{
    SocksRequestPtr request = srequest->data;
    
    if(status < 0)
        goto error;

    if(!streamRequestDone(srequest)) {
        if(status) {
            status = -ESOCKS_PROTOCOL;
            goto error;
        }
        return 0;
    }

    do_stream(IO_READ | IO_NOTNOW, request->fd, 0, request->buf, 10,
              socks5ReadHandler2, request);
    return 1;

 error:
    request->handler(status, request);
    destroySocksRequest(request);
    return 1;
}

static int
socks5ReadHandler2(int status,
                   FdEventHandlerPtr event,
                   StreamRequestPtr srequest)
{
    SocksRequestPtr request = srequest->data;

    if(status < 0)
        goto error;

    if(srequest->offset < 4) {
        if(status) {
            status = -ESOCKS_PROTOCOL;
            goto error;
        }
        return 0;
    }

    if(request->buf[0] != 5) {
        status = -ESOCKS_PROTOCOL;
        goto error;
    }

    if(request->buf[1] != 0) {
        status = -(ESOCKS5_BASE + request->buf[1]);
        goto error;
    }

    if(request->buf[3] != 1) {
        status = -ESOCKS_PROTOCOL;
        goto error;
    }

    if(srequest->offset < 10)
        return 0;

    request->handler(1, request);
    destroySocksRequest(request);
    return 1;

 error:
    CLOSE(request->fd);
    request->fd = -1;
    request->handler(status, request);
    destroySocksRequest(request);
    return 1;
}

#endif

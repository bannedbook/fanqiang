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

#ifdef HAVE_IPv6
#ifdef IPV6_PREFER_TEMPADDR
#define HAVE_IPV6_PREFER_TEMPADDR 1
#endif
#endif

#ifdef HAVE_IPV6_PREFER_TEMPADDR
int useTemporarySourceAddress = 1;
#endif

void
preinitIo()
{
#ifdef HAVE_IPV6_PREFER_TEMPADDR
    CONFIG_VARIABLE_SETTABLE(useTemporarySourceAddress, CONFIG_TRISTATE,
                             configIntSetter,
                             "Prefer IPv6 temporary source address.");
#endif

#ifdef HAVE_WINSOCK
    /* Load the winsock dll */
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);
    int err = WSAStartup( wVersionRequested, &wsaData );
    if (err != 0) {
        do_log_error(L_ERROR, err, "Couldn't load winsock dll");
        exit(-1);
    }
#endif
    return;
}

void
initIo()
{
    return;
}

FdEventHandlerPtr
do_stream(int operation, int fd, int offset, char *buf, int len,
          int (*handler)(int, FdEventHandlerPtr, StreamRequestPtr),
          void *data)
{
    assert(len > offset || (operation & (IO_END | IO_IMMEDIATE)));
    return schedule_stream(operation, fd, offset, 
                           NULL, 0, buf, len, NULL, 0, NULL, 0, NULL,
                           handler, data);
}

FdEventHandlerPtr
do_stream_2(int operation, int fd, int offset, 
            char *buf, int len, char *buf2, int len2,
            int (*handler)(int, FdEventHandlerPtr, StreamRequestPtr),
            void *data)
{
    assert(len + len2 > offset || (operation & (IO_END | IO_IMMEDIATE)));
    return schedule_stream(operation, fd, offset,
                           NULL, 0, buf, len, buf2, len2, NULL, 0, NULL,
                           handler, data);
}

FdEventHandlerPtr
do_stream_3(int operation, int fd, int offset, 
            char *buf, int len, char *buf2, int len2, char *buf3, int len3,
            int (*handler)(int, FdEventHandlerPtr, StreamRequestPtr),
            void *data)
{
    assert(len + len2 > offset || (operation & (IO_END | IO_IMMEDIATE)));
    return schedule_stream(operation, fd, offset,
                           NULL, 0, buf, len, buf2, len2, buf3, len3, NULL,
                           handler, data);
}

FdEventHandlerPtr
do_stream_h(int operation, int fd, int offset,
            char *header, int hlen, char *buf, int len,
            int (*handler)(int, FdEventHandlerPtr, StreamRequestPtr),
            void *data)
{
    assert(hlen + len > offset || (operation & (IO_END | IO_IMMEDIATE)));
    return schedule_stream(operation, fd, offset, 
                           header, hlen, buf, len, NULL, 0, NULL, 0, NULL,
                           handler, data);
}

FdEventHandlerPtr
do_stream_buf(int operation, int fd, int offset, char **buf_location, int len,
              int (*handler)(int, FdEventHandlerPtr, StreamRequestPtr),
              void *data)
{
    assert((len > offset || (operation & (IO_END | IO_IMMEDIATE)))
           && len <= CHUNK_SIZE);
    return schedule_stream(operation, fd, offset,
                           NULL, 0, *buf_location, len, 
                           NULL, 0, NULL, 0, buf_location,
                           handler, data);
}

static int
chunkHeaderLen(int i)
{
    if(i <= 0)
        return 0;
    if(i < 0x10)
        return 3;
    else if(i < 0x100)
        return 4;
    else if(i < 0x1000)
        return 5;
    else if(i < 0x10000)
        return 6;
    else
        abort();
}

static int
chunkHeader(char *buf, int buflen, int i)
{
    int n;
    if(i <= 0)
        return 0;
    n = snprintf(buf, buflen, "%x\r\n", i);
    return n;
}


FdEventHandlerPtr
schedule_stream(int operation, int fd, int offset,
                char *header, int hlen,
                char *buf, int len, char *buf2, int len2, char *buf3, int len3,
                char **buf_location,
                int (*handler)(int, FdEventHandlerPtr, StreamRequestPtr),
                void *data)
{
    StreamRequestRec request;
    FdEventHandlerPtr event;
    int done;

    request.operation = operation;
    request.fd = fd;
    if(len3) {
        assert(hlen == 0 && buf_location == NULL);
        request.u.b.len3 = len3;
        request.u.b.buf3 = buf3;
        request.operation |= IO_BUF3;
    } else if(buf_location) {
        assert(hlen == 0);
        request.u.l.buf_location = buf_location;
        request.operation |= IO_BUF_LOCATION;
    } else {
        request.u.h.hlen = hlen;
        request.u.h.header = header;
    }
    request.buf = buf;
    request.len = len;
    request.buf2 = buf2;
    request.len2 = len2;
    if((operation & IO_CHUNKED) || 
       (!(request.operation & (IO_BUF3 | IO_BUF_LOCATION)) && hlen > 0)) {
        assert(offset == 0);
        request.offset = -hlen;
        if(operation & IO_CHUNKED)
            request.offset += -chunkHeaderLen(len + len2);
    } else {
        request.offset = offset;
    }
    request.handler = handler;
    request.data = data;
    event = makeFdEvent(fd, 
                        (operation & IO_MASK) == IO_WRITE ?
                        POLLOUT : POLLIN, 
                        do_scheduled_stream, 
                        sizeof(StreamRequestRec), &request);
    if(!event) {
        done = (*handler)(-ENOMEM, NULL, &request);
        assert(done);
        return NULL;
    }

    if(!(operation & IO_NOTNOW)) {
        done = event->handler(0, event);
        if(done) {
            free(event);
            return NULL;
        }
    } 

    if(operation & IO_IMMEDIATE) {
        assert(hlen == 0 && !(operation & IO_CHUNKED));
        done = (*handler)(0, event, &request);
        if(done) {
            free(event);
            return NULL;
        }
    }
    event = registerFdEventHelper(event);
    return event;
}

static const char *endChunkTrailer = "\r\n0\r\n\r\n";

int
do_scheduled_stream(int status, FdEventHandlerPtr event)
{
    StreamRequestPtr request = (StreamRequestPtr)&event->data;
    int rc, done, i;
    struct iovec iov[6];
    int chunk_header_len;
    char chunk_header[10];
    int len12 = request->len + request->len2;
    int len123 = 
        request->len + request->len2 + 
        ((request->operation & IO_BUF3) ? request->u.b.len3 : 0);

    if(status) {
        done = request->handler(status, event, request);
        return done;
    }

    i = 0;

    if(request->offset < 0) {
        assert((request->operation & (IO_MASK | IO_BUF3 | IO_BUF_LOCATION)) == 
               IO_WRITE);
        if(request->operation & IO_CHUNKED) {
            chunk_header_len = chunkHeaderLen(len123);
        } else {
            chunk_header_len = 0;
        }

        if(request->offset < -chunk_header_len) {
            assert(request->offset >= -(request->u.h.hlen + chunk_header_len));
            iov[i].iov_base = request->u.h.header;
            iov[i].iov_len = -request->offset - chunk_header_len;
            i++;
        }

        if(chunk_header_len > 0) {
            chunkHeader(chunk_header, 10, len123);
            if(request->offset < -chunk_header_len) {
                iov[i].iov_base = chunk_header;
                iov[i].iov_len = chunk_header_len;
            } else {
                iov[i].iov_base = chunk_header + 
                    chunk_header_len + request->offset;
                iov[i].iov_len = -request->offset;
            }
            i++;
        }
    }

    if(request->len > 0) {
        if(request->buf == NULL && 
           (request->operation & IO_BUF_LOCATION)) {
            assert(*request->u.l.buf_location == NULL);
            request->buf = *request->u.l.buf_location = get_chunk();
            if(request->buf == NULL) {
                done = request->handler(-ENOMEM, event, request);
                return done;
            }
        }
        if(request->offset <= 0) {
            iov[i].iov_base = request->buf;
            iov[i].iov_len = request->len;
            i++;
        } else if(request->offset < request->len) {
            iov[i].iov_base = request->buf + request->offset;
            iov[i].iov_len = request->len - request->offset;
            i++;
        }
    }

    if(request->len2 > 0) {
        if(request->offset <= request->len) {
            iov[i].iov_base = request->buf2;
            iov[i].iov_len = request->len2;
            i++;
        } else if(request->offset < request->len + request->len2) {
            iov[i].iov_base = request->buf2 + request->offset - request->len;
            iov[i].iov_len = request->len2 - request->offset + request->len;
            i++;
        }
    }

    if((request->operation & IO_BUF3) && request->u.b.len3 > 0) {
        if(request->offset <= len12) {
            iov[i].iov_base = request->u.b.buf3;
            iov[i].iov_len = request->u.b.len3;
            i++;
        } else if(request->offset < len12 + request->u.b.len3) {
            iov[i].iov_base = request->u.b.buf3 + request->offset - len12;
            iov[i].iov_len = request->u.b.len3 - request->offset + len12;
            i++;
        }
    }

    if((request->operation & IO_CHUNKED)) {
        int l;
        const char *trailer;
        if(request->operation & IO_END) {
            if(len123 == 0) {
                trailer = endChunkTrailer + 2;
                l = 5;
            } else {
                trailer = endChunkTrailer;
                l = 7;
            }
        } else {
            trailer = endChunkTrailer;
            l = 2;
        }

        if(request->offset <= len123) {
            iov[i].iov_base = (char*)trailer;
            iov[i].iov_len = l;
            i++;
        } else if(request->offset < len123 + l) {
            iov[i].iov_base = 
                (char*)endChunkTrailer + request->offset - len123;
            iov[i].iov_len = l - request->offset + len123;
            i++;
        }
    }

    assert(i > 0);

    if((request->operation & IO_MASK) == IO_WRITE) {
        if(i > 1) 
            rc = WRITEV(request->fd, iov, i);
        else
            rc = WRITE(request->fd, iov[0].iov_base, iov[0].iov_len);
    } else {
        if(i > 1) 
            rc = READV(request->fd, iov, i);
        else
            rc = READ(request->fd, iov[0].iov_base, iov[0].iov_len);
    }

    if(rc > 0) {
        request->offset += rc;
        if(request->offset < 0) return 0;
        done = request->handler(0, event, request);
        return done;
    } else if(rc == 0 || errno == EPIPE) {
        done = request->handler(1, event, request);
    } else if(errno == EAGAIN || errno == EINTR) {
        return 0;
    } else if(errno == EFAULT || errno == EBADF) {
        abort();
    } else {
        done = request->handler(-errno, event, request);
    }
    assert(done);
    return done;
}

int
streamRequestDone(StreamRequestPtr request)
{
    int len123 = 
        request->len + request->len2 +
        ((request->operation & IO_BUF3) ? request->u.b.len3 : 0);

    if(request->offset < 0)
        return 0;
    else if(request->offset < len123)
        return 0;
    else if(request->operation & IO_CHUNKED) {
        if(request->operation & IO_END) {
            if(request->offset < len123 + (len123 ? 7 : 5))
                return 0;
        } else {
            if(request->offset < len123 + 2)
                return 0;
        }
    }

    return 1;
}

static int
serverSocket(int af)
{
    int fd, rc;
    if(af == 4) {
        fd = socket(PF_INET, SOCK_STREAM, 0);
    } else if(af == 6) {
#ifdef HAVE_IPv6
        fd = socket(PF_INET6, SOCK_STREAM, 0);
#else
        fd = -1;
        errno = EAFNOSUPPORT;
#endif
    } else {
        abort();
    }

    if(fd >= 0) {
        rc = setNonblocking(fd, 1);
        if(rc < 0) {
            int errno_save = errno;
            CLOSE(fd);
            errno = errno_save;
            return -1;
        }
#ifdef HAVE_IPV6_PREFER_TEMPADDR
	if (af == 6 && useTemporarySourceAddress != 1) {
            int value;
            value = (useTemporarySourceAddress == 2) ? 1 : 0;
            rc = setsockopt(fd, IPPROTO_IPV6, IPV6_PREFER_TEMPADDR,
                            &value, sizeof(value));
            if (rc < 0) {
                /* no error, warning only */
                do_log_error(L_WARN, errno, "Couldn't set IPV6CTL_USETEMPADDR");
            }
	}

#endif
    }
    return fd;
}

FdEventHandlerPtr
do_connect(AtomPtr addr, int index, int port,
           int (*handler)(int, FdEventHandlerPtr, ConnectRequestPtr),
           void *data)
{
    ConnectRequestRec request;
    FdEventHandlerPtr event;
    int done, fd, af;

    assert(addr->length > 0 && addr->string[0] == DNS_A);
    assert(addr->length % sizeof(HostAddressRec) == 1);

    if(index >= (addr->length - 1)/ sizeof(HostAddressRec))
        index = 0;

    request.firstindex = index;
    request.port = port;
    request.handler = handler;
    request.data = data;
 again:
    af = addr->string[1 + index * sizeof(HostAddressRec)];
    fd = serverSocket(af);

    request.fd = fd;
    request.af = af;
    request.addr = addr;
    request.index = index;

    if(fd < 0) {
        int n = (addr->length - 1) / sizeof(HostAddressRec);
        if(errno == EAFNOSUPPORT || errno == EPROTONOSUPPORT) {
            if((index + 1) % n != request.firstindex) {
                index = (index + 1) % n;
                goto again;
            }
        }
        do_log_error(L_ERROR, errno, "Couldn't create socket");
        done = (*handler)(-errno, NULL, &request);
        assert(done);
        return NULL;
    }

    /* POLLIN is apparently needed on Windows */
    event = registerFdEvent(fd, POLLIN | POLLOUT,
                            do_scheduled_connect,
                            sizeof(ConnectRequestRec), &request);
    if(event == NULL) {
        done = (*handler)(-ENOMEM, NULL, &request);
        assert(done);
        return NULL;
    }

    done = event->handler(0, event);
    if(done) {
        unregisterFdEvent(event);
        return NULL;
    }
    return event;
}

int
do_scheduled_connect(int status, FdEventHandlerPtr event)
{
    ConnectRequestPtr request = (ConnectRequestPtr)&event->data;
    AtomPtr addr = request->addr;
    int done;
    int rc;
    HostAddressPtr host;
    struct sockaddr_in servaddr;
#ifdef HAVE_IPv6
    struct sockaddr_in6 servaddr6;
#endif

    assert(addr->length > 0 && addr->string[0] == DNS_A);
    assert(addr->length % sizeof(HostAddressRec) == 1);
    assert(request->index < (addr->length - 1) / sizeof(HostAddressRec));

    if(status) {
        done = request->handler(status, event, request);
        if(done) {
            releaseAtom(addr);
            request->addr = NULL;
            return 1;
        }
        return 0;
    }

 again:
    host = (HostAddressPtr)&addr->string[1 + 
                                         request->index * 
                                         sizeof(HostAddressRec)];
    if(host->af != request->af) {
        int newfd;
        /* Ouch.  Our socket has a different protocol than the host
           address. */
        CLOSE(request->fd);
        newfd = serverSocket(host->af);
        if(newfd < 0) {
            if(errno == EAFNOSUPPORT || errno == EPROTONOSUPPORT) {
                int n = request->addr->length / sizeof(HostAddressRec);
                if((request->index + 1) % n != request->firstindex) {
                    request->index = (request->index + 1) % n;
                    goto again;
                }
            }
            request->fd = -1;
            done = request->handler(-errno, event, request);
            assert(done);
            return 1;
        }
        if(newfd != request->fd) {
            request->fd = dup2(newfd, request->fd);
            CLOSE(newfd);
            if(request->fd < 0) {
                done = request->handler(-errno, event, request);
                assert(done);
                return 1;
            }
        }
        request->af = host->af;
    }
    switch(host->af) {
    case 4:
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(request->port);
        memcpy(&servaddr.sin_addr, &host->data, sizeof(struct in_addr));
        rc = connect(request->fd,
                     (struct sockaddr*)&servaddr, sizeof(servaddr));
        break;
    case 6:
#ifdef HAVE_IPv6
        memset(&servaddr6, 0, sizeof(servaddr6));
        servaddr6.sin6_family = AF_INET6;
        servaddr6.sin6_port = htons(request->port);
        memcpy(&servaddr6.sin6_addr, &host->data, sizeof(struct in6_addr));
        rc = connect(request->fd,
                     (struct sockaddr*)&servaddr6, sizeof(servaddr6));
#else
        rc = -1;
        errno = EAFNOSUPPORT;
#endif
        break;
    default:
        abort();
    }
        
    if(rc >= 0 || errno == EISCONN) {
        done = request->handler(1, event, request);
        assert(done);
        releaseAtom(request->addr);
        request->addr = NULL;
        return 1;
    }

    if(errno == EINPROGRESS || errno == EINTR) {
        return 0;
    } else if(errno == EFAULT || errno == EBADF) {
        abort();
    } else {
        int n = request->addr->length / sizeof(HostAddressRec);
        if((request->index + 1) % n != request->firstindex) {
            request->index = (request->index + 1) % n;
            goto again;
        }
        done = request->handler(-errno, event, request);
        assert(done);
        releaseAtom(request->addr);
        request->addr = NULL;
        return 1;
    }
}

FdEventHandlerPtr
do_accept(int fd,
          int (*handler)(int, FdEventHandlerPtr, AcceptRequestPtr),
          void *data)
{
    FdEventHandlerPtr event;
    int done;

    event = schedule_accept(fd, handler, data);
    if(event == NULL) {
        done = (*handler)(-ENOMEM, NULL, NULL);
        assert(done);
    }

    /* But don't invoke it now - this will delay accept if under load. */
    return event;
}

FdEventHandlerPtr
schedule_accept(int fd,
                int (*handler)(int, FdEventHandlerPtr, AcceptRequestPtr),
                void *data)
{
    FdEventHandlerPtr event;
    AcceptRequestRec request;
    int done;

    request.fd = fd;
    request.handler = handler;
    request.data = data;
    event = registerFdEvent(fd, POLLIN, 
                            do_scheduled_accept, sizeof(request), &request);
    if(!event) {
        done = (*handler)(-ENOMEM, NULL, NULL);
        assert(done);
    }
    return event;
}

int
do_scheduled_accept(int status, FdEventHandlerPtr event)
{
    AcceptRequestPtr request = (AcceptRequestPtr)&event->data;
    int rc, done;
    unsigned len;
    struct sockaddr_in addr;

    if(status) {
        done = request->handler(status, event, request);
        if(done) return done;
    }

    len = sizeof(struct sockaddr_in);

    rc = accept(request->fd, (struct sockaddr*)&addr, &len);

    if(rc >= 0)
        done = request->handler(rc, event, request);
    else
        done = request->handler(-errno, event, request);
    return done;
}

FdEventHandlerPtr
create_listener(char *address, int port,
                int (*handler)(int, FdEventHandlerPtr, AcceptRequestPtr),
                void *data)
{
    do_log(L_INFO, "create_listener start ...");
    int fd, rc;
    int one = 1;
    int done;
    struct sockaddr_in addr;
#ifdef HAVE_IPv6
    int inet6 = 1;
    struct sockaddr_in6 addr6;
#else
    int inet6 = 0;
#endif

    if(inet6 && address) {
        struct in_addr buf;
        rc = inet_aton(address, &buf);
        if(rc == 1)
            inet6 = 0;
    }
    fd = -1;
    errno = EAFNOSUPPORT;

#ifdef HAVE_IPv6
    if(inet6) {
        fd = socket(PF_INET6, SOCK_STREAM, 0);
    }
#endif

    if(fd < 0 && (errno == EPROTONOSUPPORT || errno == EAFNOSUPPORT)) {
        inet6 = 0;
        fd = socket(PF_INET, SOCK_STREAM, 0);
    }

    if(fd < 0) {
        done = (*handler)(-errno, NULL, NULL);
        assert(done);
        return NULL;
    }

#ifndef WIN32
    /* on WIN32 SO_REUSEADDR allows two sockets bind to the same port */
    rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one));
    if(rc < 0) do_log_error(L_WARN, errno, "Couldn't set SO_REUSEADDR");
#endif

    if(inet6) {
#ifdef HAVE_IPv6
        rc = setV6only(fd, 0);
        if(rc < 0)
            /* Reportedly OpenBSD returns an error for that.  So only
               log it as a debugging message. */
            do_log_error(D_CLIENT_CONN, errno, "Couldn't reset IPV6_V6ONLY");

        memset(&addr6, 0, sizeof(addr6));
        rc = inet_pton(AF_INET6, address, &addr6.sin6_addr);
        if(rc != 1) {
            done = (*handler)(rc == 0 ? -ESYNTAX : -errno, NULL, NULL);
            assert(done);
            return NULL;
        }
        addr6.sin6_family = AF_INET6;
        addr6.sin6_port = htons(port);
        rc = bind(fd, (struct sockaddr*)&addr6, sizeof(addr6));
#else
        rc = -1;
        errno = EAFNOSUPPORT;
#endif
    } else {
        memset(&addr, 0, sizeof(addr));
        rc = inet_aton(address, &addr.sin_addr);
        if(rc != 1) {
            done = (*handler)(rc == 0 ? -ESYNTAX : -errno, NULL, NULL);
            assert(done);
            return NULL;
        }
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        rc = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    }

    if(rc < 0) {
        do_log_error(L_ERROR, errno, "Couldn't bind");
        CLOSE(fd);
        done = (*handler)(-errno, NULL, NULL);
        assert(done);
        return NULL;
    }

    rc = setNonblocking(fd, 1);
    if(rc < 0) {
        do_log_error(L_ERROR, errno, "Couldn't set non blocking mode");
        CLOSE(fd);
        done = (*handler)(-errno, NULL, NULL);
        assert(done);
        return NULL;
    }
        
    rc = listen(fd, 1024);
    if(rc < 0) {
        do_log_error(L_ERROR, errno, "Couldn't listen");
        CLOSE(fd);
        done = (*handler)(-errno, NULL, NULL);
        assert(done);
        return NULL;
    }

    do_log(L_INFO, "Established listening socket on port %d.\n", port);

    return schedule_accept(fd, handler, data);
}

#ifndef SOL_TCP
/* BSD */
#define SOL_TCP IPPROTO_TCP
#endif

int
setNonblocking(int fd, int nonblocking)
{
#ifdef WIN32 /*MINGW*/
    return win32_setnonblocking(fd, nonblocking);
#else
    int rc;
    rc = fcntl(fd, F_GETFL, 0);
    if(rc < 0)
        return -1;

    rc = fcntl(fd, F_SETFL, nonblocking?(rc | O_NONBLOCK):(rc & ~O_NONBLOCK));
    if(rc < 0)
        return -1;

    return 0;
#endif
}

int
setNodelay(int fd, int nodelay)
{
    int val = nodelay ? 1 : 0;
    int rc;
    rc = setsockopt(fd, SOL_TCP, TCP_NODELAY, (char *)&val, sizeof(val));
    if(rc < 0)
        return -1;
    return 0;
}

#ifdef IPV6_V6ONLY
int
setV6only(int fd, int v6only)
{
    int val = v6only ? 1 : 0;
    int rc;
    rc = setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&val, sizeof(val));
    if(rc < 0)
        return -1;
    return 0;
}
#else
int
setV6only(int fd, int v6only)
{
    return 0;
}
#endif

typedef struct _LingeringClose {
    int fd;
    FdEventHandlerPtr handler;
    TimeEventHandlerPtr timeout;
} LingeringCloseRec, *LingeringClosePtr;

static int
lingeringCloseTimeoutHandler(TimeEventHandlerPtr event)
{
    LingeringClosePtr l = *(LingeringClosePtr*)event->data;
    assert(l->timeout == event);
    l->timeout = NULL;
    if(l->handler)
        pokeFdEvent(l->fd, -ESHUTDOWN, POLLIN | POLLOUT);
    else {
        CLOSE(l->fd);
        free(l);
    }
    return 1;
}

static int
lingeringCloseHandler(int status, FdEventHandlerPtr event)
{
    LingeringClosePtr l = *(LingeringClosePtr*)event->data;
    char buf[17];
    int rc;

    assert(l->handler == event);

    l->handler = NULL;
    if(status && status != -EDOGRACEFUL)
        goto done;

    rc = READ(l->fd, &buf, 17);
    if(rc == 0 || (rc < 0 && errno != EAGAIN && errno != EINTR))
        goto done;

    /* The client is still sending data.  Ignore it in order to let
       TCP's flow control do its work.  The timeout will close the
       connection. */
    return 1;

 done:
    if(l->timeout) {
        cancelTimeEvent(l->timeout);
        l->timeout = NULL;
    }
    CLOSE(l->fd);
    free(l);
    return 1;
}

int
lingeringClose(int fd)
{
    int rc;
    LingeringClosePtr l;

    rc = shutdown(fd, 1);
    if(rc < 0) {
        if(errno != ENOTCONN) {
            do_log_error(L_ERROR, errno, "Shutdown failed");
        } else if(errno == EFAULT || errno == EBADF) {
            abort();
        }
        CLOSE(fd);
        return 1;
    }

    l = malloc(sizeof(LingeringCloseRec));
    if(l == NULL)
        goto fail;
    l->fd = fd;
    l->handler = NULL;
    l->timeout = NULL;

    l->timeout = scheduleTimeEvent(10, lingeringCloseTimeoutHandler,
                                   sizeof(LingeringClosePtr), &l);
    if(l->timeout == NULL) {
        free(l);
        goto fail;
    }

    l->handler = registerFdEvent(fd, POLLIN,
                                 lingeringCloseHandler,
                                 sizeof(LingeringClosePtr), &l);
    if(l->handler == NULL) {
        do_log(L_ERROR, "Couldn't schedule lingering close handler.\n");
        /* But don't close -- the timeout will do its work. */
    }
    return 1;

 fail:
    do_log(L_ERROR, "Couldn't schedule lingering close.\n");
    CLOSE(fd);
    return 1;
}

NetAddressPtr
parseNetAddress(AtomListPtr list)
{
    NetAddressPtr nl;
    int i, rc, rc6;
    char buf[100];
    struct in_addr ina;
#ifdef HAVE_IPv6
    struct in6_addr ina6;
#endif

    nl = malloc((list->length + 1) * sizeof(NetAddressRec));
    if(nl == NULL) {
        do_log(L_ERROR, "Couldn't allocate network list.\n");
        return NULL;
    }

    for(i = 0; i < list->length; i++) {
        int prefix;
        char *s = list->list[i]->string, *p;
        int n = list->list[i]->length;
        char *suffix;

        while(*s == ' ' || *s == '\t') {
            s++;
            n--;
        }

        if(n >= 100) {
            do_log(L_ERROR, "Network name too long.\n");
            goto fail;
        }
        p = memchr(s, '/', n);
        if(p) {
            memcpy(buf, s, p - s);
            buf[p - s] = '\0';
            prefix = strtol(p + 1, &suffix, 10);
        } else {
            char *s1, *s2;
            prefix = -1;
            strcpy(buf, s);
            s1 = strchr(s, ' ');
            s2 = strchr(s, '\t');
            if(s1 == NULL) suffix = s2;
            else if(s2 == NULL) suffix = s1;
            else if(s1 < s2) suffix = s1;
            else suffix = s2;
            if(suffix == NULL)
                suffix = s + n;
        }

        if(!isWhitespace(suffix)) {
            do_log(L_ERROR, "Couldn't parse network %s.\n", buf);
            goto fail;
        }

        rc = 0; rc6 = 0;
        rc = inet_aton(buf, &ina);
#ifdef HAVE_IPv6
        if(rc == 0) {
            rc6 = inet_pton(AF_INET6, buf, &ina6);
        }
#endif
        if(rc == 0 && rc6 == 0) {
            do_log(L_ERROR, "Couldn't parse network %s.\n", buf);
            goto fail;
        }
        nl[i].prefix = prefix;
        if(rc) {
            nl[i].af = 4;
            memcpy(nl[i].data, &ina, 4);
        } else {
#ifdef HAVE_IPv6
            nl[i].af = 6;
            memcpy(nl[i].data, &ina6, 16);
#else
            abort();
#endif
        }
    }
    nl[i].af = 0;
    return nl;

 fail:
    free(nl);
    return NULL;
}

/* Returns 1 if the first n bits of a and b are equal */
static int
bitmatch(const unsigned char *a, const unsigned char *b, int n)
{
    if(n >= 8) {
        if(memcmp(a, b, n / 8) != 0)
            return 0;
    }

    if(n % 8 != 0) {
        int mask = (~0) << (8 - n % 8);
        if((a[n / 8] & mask) != (b[n / 8] & mask))
            return 0;
    }

    return 1;
}

/* Returns 1 if the address in data is in list */
static int
match(int af, unsigned char *data, NetAddressPtr list)
{
    int i;
#ifdef HAVE_IPv6
    static const unsigned char v6mapped[] =
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF };
#endif

    i = 0;
    while(list[i].af != 0) {
        if(af == 4 && list[i].af == 4) {
            if(bitmatch(data, list[i].data,
                        list[i].prefix >= 0 ? list[i].prefix : 32))
                return 1;
#ifdef HAVE_IPv6
        } else if(af == 6 && list[i].af == 6) {
            if(bitmatch(data, list[i].data,
                        list[i].prefix >= 0 ? list[i].prefix : 128))
                return 1;
        } else if(af == 6 && list[i].af == 4) {
            if(bitmatch(data, v6mapped, 96)) {
                if(bitmatch(data + 12, list[i].data,
                            list[i].prefix >= 0 ? list[i].prefix : 32))
                    return 1;
            }
        } else if(af == 4 && list[i].af == 6) {
            if(bitmatch(list[i].data, v6mapped, 96)) {
                if(bitmatch(data, list[i].data + 12,
                            list[i].prefix >= 96 ?
                            list[i].prefix - 96 : 32))
                    return 1;
            }
#endif
        } else {
            abort();
        }
        i++;
    }
    return 0;
}

int
netAddressMatch(int fd, NetAddressPtr list)
{
    int rc;
    unsigned int len;
    struct sockaddr_in sain;
#ifdef HAVE_IPv6
    struct sockaddr_in6 sain6;
#endif

    len = sizeof(sain);
    rc = getpeername(fd, (struct sockaddr*)&sain, &len);
    if(rc < 0) {
        do_log_error(L_ERROR, errno, "Couldn't get peer name");
        return -1;
    }

    if(sain.sin_family == AF_INET) {
        return match(4, (unsigned char*)&sain.sin_addr, list);
#ifdef HAVE_IPv6
    } else if(sain.sin_family == AF_INET6) {
        len = sizeof(sain6);
        rc = getpeername(fd, (struct sockaddr*)&sain6, &len);
        if(rc < 0) {
            do_log_error(L_ERROR, errno, "Couldn't get peer name");
            return -1;
        }
        if(sain6.sin6_family != AF_INET6) {
            do_log(L_ERROR, "Inconsistent peer name");
            return -1;
        }
        return match(6, (unsigned char*)&sain6.sin6_addr, list);
#endif
    } else {
        do_log(L_ERROR, "Unknown address family %d\n", sain.sin_family);
        return -1;
    }
    return 0;
}


        
        

    

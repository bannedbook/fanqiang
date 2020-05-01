/*
Copyright (c) 2006 by Dan Kennedy.
Copyright (c) 2006 by Juliusz Chroboczek.

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

#ifndef WIN32 /*MINGW*/

static int dummy ATTRIBUTE((unused));

#else

#undef poll
#undef socket
#undef connect
#undef accept
#undef shutdown
#undef getpeername
#undef sleep
#undef inet_aton
#undef gettimeofday
#undef stat

/* Windows needs this header file for the implementation of inet_aton() */
#include <ctype.h>

/* _snprintf and friends have broken NULL termination semantics */
int win32_snprintf(char* dest, size_t count, const char* format, ...)
{
    int r;
    va_list args;
    va_start(args, format);
    r = _vsnprintf(dest, count, format, args);
    va_end(args);
    if (count > 0) {
        dest[count-1] = '\0';
    }
    return r;
}

/* 
 * Check whether "cp" is a valid ascii representation of an Internet address
 * and convert to a binary address.  Returns 1 if the address is valid, 0 if
 * not.  This replaces inet_addr, the return value from which cannot
 * distinguish between failure and a local broadcast address.
 *
 * This implementation of the standard inet_aton() function was copied 
 * (with trivial modifications) from the OpenBSD project.
 */
int
win32_inet_aton(const char *cp, struct in_addr *addr)
{
    register unsigned int val;
    register int base, n;
    register char c;
    unsigned int parts[4];
    register unsigned int *pp = parts;

    assert(sizeof(val) == 4);

    c = *cp;
    while(1) {
        /*
         * Collect number up to ``.''.
         * Values are specified as for C:
         * 0x=hex, 0=octal, isdigit=decimal.
         */
        if(!isdigit(c))
            return (0);
        val = 0; base = 10;
        if(c == '0') {
            c = *++cp;
            if(c == 'x' || c == 'X')
                base = 16, c = *++cp;
            else
                base = 8;
        }
        while(1) {
            if(isascii(c) && isdigit(c)) {
                val = (val * base) + (c - '0');
                c = *++cp;
            } else if(base == 16 && isascii(c) && isxdigit(c)) {
                val = (val << 4) |
                    (c + 10 - (islower(c) ? 'a' : 'A'));
                c = *++cp;
            } else
                break;
        }
        if(c == '.') {
            /*
             * Internet format:
             *    a.b.c.d
             *    a.b.c    (with c treated as 16 bits)
             *    a.b    (with b treated as 24 bits)
             */
            if(pp >= parts + 3)
                return (0);
            *pp++ = val;
            c = *++cp;
        } else
            break;
    }
    /*
     * Check for trailing characters.
     */
    if(c != '\0' && (!isascii(c) || !isspace(c)))
        return (0);
    /*
     * Concoct the address according to
     * the number of parts specified.
     */
    n = pp - parts + 1;
    switch(n) {

    case 0:
        return (0);        /* initial nondigit */

    case 1:                /* a -- 32 bits */
        break;

    case 2:                /* a.b -- 8.24 bits */
        if((val > 0xffffff) || (parts[0] > 0xff))
            return (0);
        val |= parts[0] << 24;
        break;

    case 3:                /* a.b.c -- 8.8.16 bits */
        if((val > 0xffff) || (parts[0] > 0xff) || (parts[1] > 0xff))
            return (0);
        val |= (parts[0] << 24) | (parts[1] << 16);
        break;

    case 4:                /* a.b.c.d -- 8.8.8.8 bits */
        if((val > 0xff) || (parts[0] > 0xff) ||
           (parts[1] > 0xff) || (parts[2] > 0xff))
            return (0);
        val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
        break;
    }
    if(addr)
        addr->s_addr = htonl(val);
    return (1);
}

unsigned int
win32_sleep(unsigned int seconds)
{
    Sleep(seconds * 1000);
    return 0;
}

int
win32_gettimeofday(struct timeval *tv, char *tz)
{
    const long long EPOCHFILETIME = (116444736000000000LL);
    FILETIME        ft;
    LARGE_INTEGER   li;
    long long        t;

    /* This implementation doesn't support the timezone parameter. That's Ok,
     * as at present polipo always passed NULL as the second arg. We
     * also need to make sure that we have at least 8 bytes of space to
     * do the math in - otherwise there will be overflow errors.
     */
    assert(tz == NULL);
    assert(sizeof(t) == 8);

    if(tv) {
        GetSystemTimeAsFileTime(&ft);
        li.LowPart  = ft.dwLowDateTime;
        li.HighPart = ft.dwHighDateTime;
        t  = li.QuadPart;       /* In 100-nanosecond intervals */
        t -= EPOCHFILETIME;     /* Offset to the Epoch time */
        t /= 10;                /* In microseconds */
        tv->tv_sec  = (long)(t / 1000000);
        tv->tv_usec = (long)(t % 1000000);
    }
    return 0;
}

int win32_poll(struct pollfd *fds, unsigned int nfds, int timo)
{
    struct timeval timeout, *toptr;
    fd_set ifds, ofds, efds, *ip, *op;
    int i, rc;

    /* Set up the file-descriptor sets in ifds, ofds and efds. */
    FD_ZERO(&ifds);
    FD_ZERO(&ofds);
    FD_ZERO(&efds);
    for (i = 0, op = ip = 0; i < nfds; ++i) {
	fds[i].revents = 0;
	if(fds[i].events & (POLLIN|POLLPRI)) {
		ip = &ifds;
		FD_SET(fds[i].fd, ip);
	}
	if(fds[i].events & POLLOUT) {
		op = &ofds;
		FD_SET(fds[i].fd, op);
	}
	FD_SET(fds[i].fd, &efds);
    } 

    /* Set up the timeval structure for the timeout parameter */
    if(timo < 0) {
	toptr = 0;
    } else {
	toptr = &timeout;
	timeout.tv_sec = timo / 1000;
	timeout.tv_usec = (timo - timeout.tv_sec * 1000) * 1000;
    }

#ifdef DEBUG_POLL
    printf("Entering select() sec=%ld usec=%ld ip=%lx op=%lx\n",
           (long)timeout.tv_sec, (long)timeout.tv_usec, (long)ip, (long)op);
#endif
    rc = select(0, ip, op, &efds, toptr);
#ifdef DEBUG_POLL
    printf("Exiting select rc=%d\n", rc);
#endif

    if(rc <= 0)
	return rc;

    if(rc > 0) {
        for (i = 0; i < nfds; ++i) {
            int fd = fds[i].fd;
    	if(fds[i].events & (POLLIN|POLLPRI) && FD_ISSET(fd, &ifds))
    		fds[i].revents |= POLLIN;
    	if(fds[i].events & POLLOUT && FD_ISSET(fd, &ofds))
    		fds[i].revents |= POLLOUT;
    	if(FD_ISSET(fd, &efds))
    		/* Some error was detected ... should be some way to know. */
    		fds[i].revents |= POLLHUP;
#ifdef DEBUG_POLL
        printf("%d %d %d revent = %x\n", 
                FD_ISSET(fd, &ifds), FD_ISSET(fd, &ofds), FD_ISSET(fd, &efds), 
                fds[i].revents
        );
#endif
        }
    }
    return rc;
}

int win32_close_socket(SOCKET fd) {
    int rc;

    rc = closesocket(fd);
    return 0;
}

static void
set_errno(int winsock_err)
{
    switch(winsock_err) {
        case WSAEWOULDBLOCK:
            errno = EAGAIN;
            break;
        default:
            errno = winsock_err;
            break;
    }
}

int win32_write_socket(SOCKET fd, void *buf, int n)
{
    int rc = send(fd, buf, n, 0);
    if(rc == SOCKET_ERROR) {
        set_errno(WSAGetLastError());
    }
    return rc;
}

int win32_read_socket(SOCKET fd, void *buf, int n)
{
    int rc = recv(fd, buf, n, 0);
    if(rc == SOCKET_ERROR) {
        set_errno(WSAGetLastError());
    }
    return rc;
}


/*
 * Set the "non-blocking" flag on socket fd to the value specified by
 * the second argument (i.e. if the nonblocking argument is non-zero, the
 * socket is set to non-blocking mode). Zero is returned if the operation
 * is successful, other -1.
 */
int
win32_setnonblocking(SOCKET fd, int nonblocking)
{
    int rc;

    unsigned long mode = 1;
    rc = ioctlsocket(fd, FIONBIO, &mode);
    if(rc != 0) {
        set_errno(WSAGetLastError());
    }
    return (rc == 0 ? 0 : -1);
}

/*
 * A wrapper around the socket() function. The purpose of this wrapper
 * is to ensure that the global errno symbol is set if an error occurs,
 * even if we are using winsock.
 */
SOCKET
win32_socket(int domain, int type, int protocol)
{
    SOCKET fd = socket(domain, type, protocol);
    if(fd == INVALID_SOCKET) {
        set_errno(WSAGetLastError());
    }
    return fd;
}

static void
set_connect_errno(int winsock_err)
{
    switch(winsock_err) {
        case WSAEINVAL:
        case WSAEALREADY:
        case WSAEWOULDBLOCK:
            errno = EINPROGRESS;
            break;
        default:
            errno = winsock_err;
            break;
    }
}

/*
 * A wrapper around the connect() function. The purpose of this wrapper
 * is to ensure that the global errno symbol is set if an error occurs,
 * even if we are using winsock.
 */
int
win32_connect(SOCKET fd, struct sockaddr *addr, socklen_t addr_len)
{
    int rc = connect(fd, addr, addr_len);
    assert(rc == 0 || rc == SOCKET_ERROR);
    if(rc == SOCKET_ERROR) {
        set_connect_errno(WSAGetLastError());
    }
    return rc;
}

/*
 * A wrapper around the accept() function. The purpose of this wrapper
 * is to ensure that the global errno symbol is set if an error occurs,
 * even if we are using winsock.
 */
SOCKET
win32_accept(SOCKET fd, struct sockaddr *addr, socklen_t *addr_len)
{
    SOCKET newfd = accept(fd, addr, addr_len);
    if(newfd == INVALID_SOCKET) {
        set_errno(WSAGetLastError());
        newfd = -1;
    }
    return newfd;
}

/*
 * A wrapper around the shutdown() function. The purpose of this wrapper
 * is to ensure that the global errno symbol is set if an error occurs,
 * even if we are using winsock.
 */
int
win32_shutdown(SOCKET fd, int mode)
{
    int rc = shutdown(fd, mode);
    assert(rc == 0 || rc == SOCKET_ERROR);
    if(rc == SOCKET_ERROR) {
        set_errno(WSAGetLastError());
    }
    return rc;
}

/*
 * A wrapper around the getpeername() function. The purpose of this wrapper
 * is to ensure that the global errno symbol is set if an error occurs,
 * even if we are using winsock.
 */
int
win32_getpeername(SOCKET fd, struct sockaddr *name, socklen_t *namelen)
{
    int rc = getpeername(fd, name, namelen);
    assert(rc == 0 || rc == SOCKET_ERROR);
    if(rc == SOCKET_ERROR) {
        set_errno(WSAGetLastError());
    }
    return rc;
}

/* Stat doesn't work on directories if the name ends in a slash. */

int
win32_stat(const char *filename, struct stat *ss)
{
    int len, rc, saved_errno;
    char *noslash;

    len = strlen(filename);
    if(len <= 1 || filename[len - 1] != '/')
        return stat(filename, ss);

    noslash = malloc(len);
    if(noslash == NULL)
        return -1;

    memcpy(noslash, filename, len - 1);
    noslash[len - 1] = '\0';

    rc = stat(noslash, ss);
    saved_errno = errno;
    free(noslash);
    errno = saved_errno;
    return rc;
}
#endif /* #ifdef WIN32 MINGW */

#ifndef HAVE_READV_WRITEV

int
polipo_writev(int fd, const struct iovec *vector, int count)
{
    int rc;                     /* Return Code */
    if(count == 1) {
        rc = WRITE(fd, vector->iov_base, vector->iov_len);
    } else {
        int n = 0;              /* Total bytes to write */
        char *buf = 0;          /* Buffer to copy to before writing */
        int i;                  /* Counter var for looping over vector[] */
        int offset = 0;        /* Offset for copying to buf */

        /* Figure out the required buffer size */
        for(i = 0; i < count; i++) {
            n += vector[i].iov_len;
        }

        /* Allocate the buffer. If the allocation fails, bail out */
        buf = malloc(n);
        if(!buf) {
            errno = ENOMEM;
            return -1;
        }

        /* Copy the contents of the vector array to the buffer */
        for(i = 0; i < count; i++) {
            memcpy(&buf[offset], vector[i].iov_base, vector[i].iov_len);
            offset += vector[i].iov_len;
        }
        assert(offset == n);

        /* Write the entire buffer to the socket and free the allocation */
        rc = WRITE(fd, buf, n);
        free(buf);
    }
    return rc;
}

int
polipo_readv(int fd, const struct iovec *vector, int count)
{
    int ret = 0;                     /* Return value */
    int i;
    for(i = 0; i < count; i++) {
        int n = vector[i].iov_len;
        int rc = READ(fd, vector[i].iov_base, n);
        if(rc == n) {
            ret += rc;
        } else {
            if(rc < 0) {
                ret = (ret == 0 ? rc : ret);
            } else {
                ret += rc;
            }
            break;
        }
    }
    return ret;
}
#endif

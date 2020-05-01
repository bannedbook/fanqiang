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

/* 
 * Polipo was originally designed to run on Unix-like systems. This
 * header file (and it's accompanying implementation file mingw.c) contain
 * code that allows polipo to run on Microsoft Windows too. 
 *
 * The target MS windows compiler is Mingw (MINimal Gnu for Windows). The
 * code in this file probably get's us pretty close to MSVC also, but
 * this has not been tested. To build polipo for Mingw, define the MINGW
 * symbol. For Unix or Unix-like systems, leave it undefined.
 */

#ifdef WIN32 /*MINGW*/

/* Unfortunately, there's no hiding it. */
#define HAVE_WINSOCK 1

#ifndef WINVER
#define WINVER 0x0501
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0410
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0700
#endif

/* At time of writing, a fair bit of stuff doesn't work under Mingw.
 * Hopefully they will be fixed later (especially the disk-cache).
 */
#define NO_IPv6 1

#define S_IROTH S_IREAD

/* Pull in winsock2.h for (almost) berkeley sockets. */
#include <winsock2.h>

// here we smash the errno defines so we can smash the socket functions later to smash errno. yay!
#ifdef ENOTCONN
  #undef ENOTCONN
#endif
#ifdef EWOULDBLOCK
  #undef EWOULDBLOCK
#endif
#ifdef ENOBUFS
  #undef ENOBUFS
#endif
#ifdef ECONNRESET
  #undef ECONNRESET
#endif
#ifdef EAFNOSUPPORT
  #undef EAFNOSUPPORT
#endif
#ifdef EPROTONOSUPPORT
  #undef EPROTONOSUPPORT
#endif
#ifdef EINPROGRESS
  #undef EINPROGRESS
#endif
#ifdef EISCONN
  #undef EISCONN
#endif
#define ENOTCONN        WSAENOTCONN
#define EWOULDBLOCK     WSAEWOULDBLOCK
#define ENOBUFS         WSAENOBUFS
#define ECONNRESET      WSAECONNRESET
#define ESHUTDOWN       WSAESHUTDOWN
#define EAFNOSUPPORT    WSAEAFNOSUPPORT
#define EPROTONOSUPPORT WSAEPROTONOSUPPORT
#define EINPROGRESS     WSAEINPROGRESS
#define EISCONN         WSAEISCONN


#include <direct.h>
#include <io.h>
#include <process.h>

/* winsock doesn't feature poll(), so there is a version implemented
 * in terms of select() in mingw.c. The following definitions
 * are copied from linux man pages. A poll() macro is defined to
 * call the version in mingw.c.
 */
#define POLLIN      0x0001    /* There is data to read */
#define POLLPRI     0x0002    /* There is urgent data to read */
#define POLLOUT     0x0004    /* Writing now will not block */
#define POLLERR     0x0008    /* Error condition */
#define POLLHUP     0x0010    /* Hung up */
#define POLLNVAL    0x0020    /* Invalid request: fd not open */
struct pollfd {
    SOCKET fd;        /* file descriptor */
    short events;     /* requested events */
    short revents;    /* returned events */
};
#define poll(x, y, z)        win32_poll(x, y, z)

/* These wrappers do nothing special except set the global errno variable if
 * an error occurs (winsock doesn't do this by default). They set errno
 * to unix-like values (i.e. WSAEWOULDBLOCK is mapped to EAGAIN), so code
 * outside of this file "shouldn't" have to worry about winsock specific error
 * handling.
 */
#define socket(x, y, z)      win32_socket(x, y, z)
#define connect(x, y, z)     win32_connect(x, y, z)
#define accept(x, y, z)      win32_accept(x, y, z)
#define shutdown(x, y)       win32_shutdown(x, y)
#define getpeername(x, y, z) win32_getpeername(x, y, z)

/* Wrapper macros to call misc. functions mingw is missing */
#define sleep(x)             win32_sleep(x)
#define inet_aton(x, y)      win32_inet_aton(x, y)
#define gettimeofday(x, y)   win32_gettimeofday(x, y)
#define stat(x, y)           win32_stat(x, y)
#define snprintf             win32_snprintf

#define mkdir(x, y) mkdir(x)
#define getcwd _getcwd
#define getpid _getpid
#ifndef S_ISDIR
#define S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(mode)  (((mode) & S_IFMT) == S_IFREG)
#endif

/* Winsock uses int instead of the usual socklen_t */
typedef int socklen_t;

typedef int pid_t;

/* Function prototypes for functions in mingw.c */
unsigned int win32_sleep(unsigned int);
int     win32_inet_aton(const char *, struct in_addr *);
int     win32_gettimeofday(struct timeval *, char *);
int     win32_poll(struct pollfd *, unsigned int, int);
SOCKET  win32_socket(int, int, int);
int     win32_connect(SOCKET, struct sockaddr*, socklen_t);
SOCKET  win32_accept(SOCKET, struct sockaddr*, socklen_t *);
int     win32_shutdown(SOCKET, int);
int     win32_getpeername(SOCKET, struct sockaddr*, socklen_t *);
int     win32_snprintf(char* dest, size_t count, const char* format, ...);

/* Three socket specific macros */
#define READ(x, y, z)  win32_read_socket(x, y, z)
#define WRITE(x, y, z) win32_write_socket(x, y, z)
#define CLOSE(x)       win32_close_socket(x)

int win32_read_socket(SOCKET, void *, int);
int win32_write_socket(SOCKET, void *, int);
int win32_close_socket(SOCKET);

int win32_setnonblocking(SOCKET, int);
int win32_stat(const char*, struct stat*);
#endif

#ifndef HAVE_READV_WRITEV
/*
 * The HAVE_READV_WRITEV symbol should be defined if the system features
 * the vector IO functions readv() and writev() and those functions may
 * be legally used with sockets.
 */
struct iovec {
    void *iov_base;   /* Starting address */
    size_t iov_len;   /* Number of bytes */
};
#define WRITEV(x, y, z) polipo_writev(x, y, z)
#define READV(x, y, z)  polipo_readv(x, y, z)
int polipo_readv(int fd, const struct iovec *vector, int count);
int polipo_writev(int fd, const struct iovec *vector, int count);
#endif

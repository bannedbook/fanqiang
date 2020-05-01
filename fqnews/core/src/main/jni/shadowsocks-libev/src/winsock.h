/*
 * winsock.h - Windows socket compatibility layer
 *
 * Copyright (C) 2013 - 2019, Max Lv <max.c.lv@gmail.com>
 *
 * This file is part of the shadowsocks-libev.
 *
 * shadowsocks-libev is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * shadowsocks-libev is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with shadowsocks-libev; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef _WINSOCK_H
#define _WINSOCK_H

#ifdef __MINGW32__

// Target NT6
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#if defined(_WIN32_WINNT) && _WIN32_WINNT < 0x0600
#undef _WIN32_WINNT
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

// Winsock headers
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>

// Override POSIX error number
#ifdef errno
#undef errno
#endif
#define errno WSAGetLastError()

#ifdef EWOULDBLOCK
#undef EWOULDBLOCK
#endif
#define EWOULDBLOCK WSAEWOULDBLOCK

#ifdef CONNECT_IN_PROGRESS
#undef CONNECT_IN_PROGRESS
#endif
#define CONNECT_IN_PROGRESS WSAEWOULDBLOCK

#ifdef EOPNOTSUPP
#undef EOPNOTSUPP
#endif
#define EOPNOTSUPP WSAEOPNOTSUPP

#ifdef EPROTONOSUPPORT
#undef EPROTONOSUPPORT
#endif
#define EPROTONOSUPPORT WSAEPROTONOSUPPORT

#ifdef ENOPROTOOPT
#undef ENOPROTOOPT
#endif
#define ENOPROTOOPT WSAENOPROTOOPT

// Check if ConnectEx supported in header
#ifdef WSAID_CONNECTEX
// Hardcode TCP fast open option
#ifndef TCP_FASTOPEN
#define TCP_FASTOPEN 15
#endif
// Enable TFO support
#define TCP_FASTOPEN_WINSOCK 1
#endif

// Override close function
#define close(fd) closesocket(fd)

// Override MinGW functions
#define setsockopt(a, b, c, d, e) setsockopt(a, b, c, (const char *)(d), e)
#define inet_ntop(a, b, c, d) inet_ntop(a, (void *)(b), c, d)

// Override Windows built-in functions
#ifdef ERROR
#undef ERROR
#endif
#define ERROR(s) ss_error(s)

#ifdef gai_strerror
#undef gai_strerror
#endif
#define gai_strerror(e) ss_gai_strerror(e)
char *ss_gai_strerror(int ecode);

// Missing Unix functions
#define sleep(x) Sleep((x) * 1000)
#define bzero(s, n) memset(s, 0, n)
#define strndup(s, n) ss_strndup(s, n)

// Winsock compatibility functions
int setnonblocking(SOCKET socket);
void winsock_init(void);
void winsock_cleanup(void);
#ifdef TCP_FASTOPEN_WINSOCK
LPFN_CONNECTEX winsock_getconnectex(void);
int winsock_dummybind(SOCKET fd, struct sockaddr *sa);
#endif

#endif // __MINGW32__

#endif // _WINSOCK_H

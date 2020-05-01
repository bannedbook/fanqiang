/*
 * winsock.c - Windows socket compatibility layer
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

#ifdef __MINGW32__

#include "winsock.h"
#include "utils.h"

#ifndef ENABLE_QUICK_EDIT
#define ENABLE_QUICK_EDIT 0x0040
#endif

#ifndef STD_INPUT_HANDLE
#define STD_INPUT_HANDLE ((DWORD)-10)
#endif

#ifndef ENABLE_EXTENDED_FLAGS
#define ENABLE_EXTENDED_FLAGS 0x0080
#endif

#ifndef STD_OUTPUT_HANDLE
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#endif

static void
disable_quick_edit(void)
{
    DWORD mode     = 0;
    HANDLE console = GetStdHandle(STD_INPUT_HANDLE);

    // Get current console mode
    if (console == NULL ||
        console == INVALID_HANDLE_VALUE ||
        !GetConsoleMode(console, &mode)) {
        return;
    }

    // Clear the quick edit bit in the mode flags
    mode &= ~ENABLE_QUICK_EDIT;
    mode |= ENABLE_EXTENDED_FLAGS;
    SetConsoleMode(console, mode);
}

void
winsock_init(void)
{
    int ret;
    WSADATA wsa_data;
    ret = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (ret != 0) {
        FATAL("Failed to initialize winsock");
    }
    // Disable quick edit mode to prevent stuck
    disable_quick_edit();
}

void
winsock_cleanup(void)
{
    WSACleanup();
}

int
setnonblocking(SOCKET socket)
{
    u_long arg = 1;
    return ioctlsocket(socket, FIONBIO, &arg);
}

void
ss_error(const char *s)
{
    char *msg = NULL;
    DWORD err = WSAGetLastError();
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&msg, 0, NULL);
    if (msg != NULL) {
        // Remove trailing newline character
        ssize_t len = strlen(msg) - 1;
        if (len >= 0 && msg[len] == '\n') {
            msg[len] = '\0';
        }
        LOGE("%s: [%ld] %s", s, err, msg);
        LocalFree(msg);
    }
}

char *
ss_gai_strerror(int ecode)
{
    static TCHAR buff[GAI_STRERROR_BUFFER_SIZE + 1];
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_MAX_WIDTH_MASK,
        NULL, ecode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)buff, GAI_STRERROR_BUFFER_SIZE, NULL);
    return (char *)buff;
}

static BOOL
get_conattr(HANDLE console, WORD *out_attr)
{
    static BOOL done       = FALSE;
    static WORD saved_attr = 0;
    if (!done) {
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (GetConsoleScreenBufferInfo(console, &info)) {
            saved_attr = info.wAttributes;
            done       = TRUE;
        }
    }
    if (out_attr != NULL) {
        *out_attr = saved_attr;
    }
    return done;
}

static BOOL
set_concolor(WORD color, BOOL reset)
{
    static HANDLE console = NULL;
    if (console == NULL) {
        console = GetStdHandle(STD_OUTPUT_HANDLE);
    }
    if (console == NULL ||
        console == INVALID_HANDLE_VALUE) {
        // If no console is available, we will not try again
        console = INVALID_HANDLE_VALUE;
        return FALSE;
    }
    WORD attr;
    if (!get_conattr(console, &attr)) {
        return FALSE;
    }
    if (!reset) {
        // Only override foreground color without changing background
        attr &= ~(FOREGROUND_RED | FOREGROUND_GREEN |
                  FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        attr |= (color | FOREGROUND_INTENSITY);
    }
    return SetConsoleTextAttribute(console, attr);
}

void
ss_color_info(void)
{
    set_concolor(FOREGROUND_GREEN, FALSE);
}

void
ss_color_error(void)
{
    set_concolor(FOREGROUND_RED | FOREGROUND_BLUE, FALSE);
}

void
ss_color_reset(void)
{
    set_concolor(0, TRUE);
}

#ifdef TCP_FASTOPEN_WINSOCK
LPFN_CONNECTEX
winsock_getconnectex(void)
{
    static LPFN_CONNECTEX pConnectEx = NULL;
    if (pConnectEx != NULL) {
        return pConnectEx;
    }

    // Dummy socket for WSAIoctl
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        ERROR("socket");
        return NULL;
    }

    // Load ConnectEx function
    GUID guid = WSAID_CONNECTEX;
    DWORD numBytes;
    int ret = -1;
    ret = WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,
                   (void *)&guid, sizeof(guid),
                   (void *)&pConnectEx, sizeof(pConnectEx),
                   &numBytes, NULL, NULL);
    if (ret != 0) {
        ERROR("WSAIoctl");
        closesocket(s);
        return NULL;
    }
    closesocket(s);
    return pConnectEx;
}

int
winsock_dummybind(SOCKET fd, struct sockaddr *sa)
{
    struct sockaddr_storage ss;
    memset(&ss, 0, sizeof(ss));
    if (sa->sa_family == AF_INET) {
        struct sockaddr_in *sin = (struct sockaddr_in *)&ss;
        sin->sin_family      = AF_INET;
        sin->sin_addr.s_addr = INADDR_ANY;
    } else if (sa->sa_family == AF_INET6) {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&ss;
        sin6->sin6_family = AF_INET6;
        sin6->sin6_addr   = in6addr_any;
    } else {
        return -1;
    }
    if (bind(fd, (struct sockaddr *)&ss, sizeof(ss)) < 0 &&
        WSAGetLastError() != WSAEINVAL) {
        return -1;
    }
    return 0;
}

#endif

#endif // __MINGW32__

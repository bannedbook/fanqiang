/*
Copyright (c) 2004-2006 by Juliusz Chroboczek

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

typedef struct _CircularBuffer {
    int head;
    int tail;
    char *buf;
} CircularBufferRec, *CircularBufferPtr;

#define TUNNEL_READER1 1
#define TUNNEL_WRITER1 2
#define TUNNEL_EOF1 4
#define TUNNEL_EPIPE1 8
#define TUNNEL_READER2 16
#define TUNNEL_WRITER2 32
#define TUNNEL_EOF2 64
#define TUNNEL_EPIPE2 128

typedef struct _Tunnel {
    AtomPtr hostname;
    int port;
    int flags;
    int fd1;
    CircularBufferRec buf1;
    int fd2;
    CircularBufferRec buf2;
} TunnelRec, *TunnelPtr;

void do_tunnel(int fd, char *buf, int offset, int len, AtomPtr url);

void listTunnels(FILE *out);

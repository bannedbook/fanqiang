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

/* request->operation */
#define IO_READ 0
#define IO_WRITE 1
#define IO_MASK 0xFF
/* Do not initiate operation now -- wait for the poll loop. */
#define IO_NOTNOW 0x100
/* Call the progress handler once if no data arrives immediately. */
#define IO_IMMEDIATE 0x200
/* Emit a chunk length before every write operation */
#define IO_CHUNKED 0x400
/* Emit a zero-length chunk at the end if chunked */
#define IO_END 0x800

/* Internal -- header is really buf3 */
#define IO_BUF3 0x1000
/* Internal -- header is really buf_location */
#define IO_BUF_LOCATION 0x2000

typedef struct _StreamRequest {
    short operation;
    short fd;
    int offset;
    int len;
    int len2;
    union {
        struct {
            int hlen;
            char *header;
        } h;
        struct {
            int len3;
            char *buf3;
        } b;
        struct {
            char **buf_location;
        } l;
    } u;
    char *buf;
    char *buf2;
    int (*handler)(int, FdEventHandlerPtr, struct _StreamRequest*);
    void *data;
} StreamRequestRec, *StreamRequestPtr;

typedef struct _ConnectRequest {
    int fd;
    int af;
    struct _Atom *addr;
    int firstindex;
    int index;
    int port;
    int (*handler)(int, FdEventHandlerPtr, struct _ConnectRequest*);
    void *data;
} ConnectRequestRec, *ConnectRequestPtr;

typedef struct _AcceptRequest {
    int fd;
    int (*handler)(int, FdEventHandlerPtr, struct _AcceptRequest*);
    void *data;
} AcceptRequestRec, *AcceptRequestPtr;

void preinitIo();
void initIo();

FdEventHandlerPtr
do_stream(int operation, int fd, int offset, char *buf, int len,
          int (*handler)(int, FdEventHandlerPtr, StreamRequestPtr),
          void *data);

FdEventHandlerPtr
do_stream_h(int operation, int fd, int offset, 
            char *header, int hlen, char *buf, int len,
            int (*handler)(int, FdEventHandlerPtr, StreamRequestPtr),
            void *data);

FdEventHandlerPtr
do_stream_2(int operation, int fd, int offset, 
            char *buf, int len, char *buf2, int len2,
            int (*handler)(int, FdEventHandlerPtr, StreamRequestPtr),
            void *data);

FdEventHandlerPtr
do_stream_3(int operation, int fd, int offset, 
            char *buf, int len, char *buf2, int len2, char *buf3, int len3,
            int (*handler)(int, FdEventHandlerPtr, StreamRequestPtr),
            void *data);

FdEventHandlerPtr
do_stream_buf(int operation, int fd, int offset, char **buf_location, int len,
              int (*handler)(int, FdEventHandlerPtr, StreamRequestPtr),
              void *data);

FdEventHandlerPtr
schedule_stream(int operation, int fd, int offset,
                char *header, int hlen,
                char *buf, int len, char *buf2, int len2, char *buf3, int len3,
                char **buf_location,
                int (*handler)(int, FdEventHandlerPtr, StreamRequestPtr),
                void *data);

int do_scheduled_stream(int, FdEventHandlerPtr);
int streamRequestDone(StreamRequestPtr);

FdEventHandlerPtr
do_connect(struct _Atom *addr, int index, int port,
           int (*handler)(int, FdEventHandlerPtr, ConnectRequestPtr),
           void *data);

int do_scheduled_connect(int, FdEventHandlerPtr event);

FdEventHandlerPtr
do_accept(int fd,
          int (*handler)(int, FdEventHandlerPtr, AcceptRequestPtr),
          void* data);

FdEventHandlerPtr 
schedule_accept(int fd,
                int (*handler)(int, FdEventHandlerPtr, AcceptRequestPtr),
                void* data);

int do_scheduled_accept(int, FdEventHandlerPtr event);

FdEventHandlerPtr
create_listener(char *address, int port,
                int (*handler)(int, FdEventHandlerPtr, AcceptRequestPtr),
                void *data);
int setNonblocking(int fd, int nonblocking);
int setNodelay(int fd, int nodelay);
int setV6only(int fd, int v6only);
int lingeringClose(int fd);

typedef struct _NetAddress {
    int prefix;
    int af;
    unsigned char data[16];
} NetAddressRec, *NetAddressPtr;

NetAddressPtr parseNetAddress(AtomListPtr list);
int netAddressMatch(int fd, NetAddressPtr list) ATTRIBUTE ((pure));


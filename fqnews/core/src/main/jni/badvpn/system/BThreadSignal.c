/**
 * @file BThreadSignal.c
 * @author Ambroz Bizjak <ambrop7@gmail.com>
 * 
 * @section LICENSE
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <unistd.h>

#include <misc/debug.h>
#include <misc/nonblocking.h>
#include <base/BLog.h>

#include "BThreadSignal.h"

#include <generated/blog_channel_BThreadSignal.h>

static void bfd_handler (void *user, int events)
{
    BThreadSignal *o = user;
    DebugObject_Access(&o->d_obj);
    
    char byte;
    ssize_t res = read(o->pipe[0], &byte, sizeof(byte));
    
    if (res < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return;
    }
    
    if (res < 0) {
        BLog(BLOG_ERROR, "read failed");
        return;
    }
    
    if (res != sizeof(byte)) {
        BLog(BLOG_ERROR, "bad read");
        return;
    }
    
    o->handler(o);
}

int BThreadSignal_Init (BThreadSignal *o, BReactor *reactor, BThreadSignal_handler handler)
{
    o->reactor = reactor;
    o->handler = handler;
    
    if (pipe(o->pipe) < 0) {
        BLog(BLOG_ERROR, "pipe failed");
        goto fail0;
    }
    
    if (!badvpn_set_nonblocking(o->pipe[0]) || !badvpn_set_nonblocking(o->pipe[1])) {
        BLog(BLOG_ERROR, "badvpn_set_nonblocking failed");
        goto fail1;
    }
    
    BFileDescriptor_Init(&o->bfd, o->pipe[0], bfd_handler, o);
    
    if (!BReactor_AddFileDescriptor(o->reactor, &o->bfd)) {
        BLog(BLOG_ERROR, "BReactor_AddFileDescriptor failed");
        goto fail1;
    }
    
    BReactor_SetFileDescriptorEvents(o->reactor, &o->bfd, BREACTOR_READ);
    
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail1:
    if (close(o->pipe[1]) < 0) {
        BLog(BLOG_ERROR, "close failed");
    }
    if (close(o->pipe[0]) < 0) {
        BLog(BLOG_ERROR, "close failed");
    }
fail0:
    return 0;
}

void BThreadSignal_Free (BThreadSignal *o)
{
    DebugObject_Free(&o->d_obj);
    
    BReactor_RemoveFileDescriptor(o->reactor, &o->bfd);
    
    if (close(o->pipe[1]) < 0) {
        BLog(BLOG_ERROR, "close failed");
    }
    if (close(o->pipe[0]) < 0) {
        BLog(BLOG_ERROR, "close failed");
    }
}

int BThreadSignal_Thread_Signal (BThreadSignal *o)
{
    DebugObject_Access(&o->d_obj);
    
    char byte = 0;
    ssize_t res = write(o->pipe[1], &byte, sizeof(byte));
    
    if (res < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return 0;
    }
    
    if (res < 0) {
        BLog(BLOG_ERROR, "write failed");
    } else if (res != sizeof(byte)) {
        BLog(BLOG_ERROR, "bad write");
    }
    
    return 1;
}

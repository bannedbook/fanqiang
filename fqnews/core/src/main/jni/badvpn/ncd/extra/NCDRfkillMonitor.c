/**
 * @file NCDRfkillMonitor.c
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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <misc/debug.h>
#include <misc/nonblocking.h>
#include <base/BLog.h>

#include "NCDRfkillMonitor.h"

#include <generated/blog_channel_NCDRfkillMonitor.h>

#define RFKILL_DEVICE_NODE "/dev/rfkill"

static void rfkill_fd_handler (NCDRfkillMonitor *o, int events);

void rfkill_fd_handler (NCDRfkillMonitor *o, int events)
{
    DebugObject_Access(&o->d_obj);
    
    // read from netlink fd
    struct rfkill_event event;
    int len = read(o->rfkill_fd, &event, sizeof(event));
    if (len < 0) {
        BLog(BLOG_ERROR, "read failed");
        return;
    }
    if (len != sizeof(event)) {
        BLog(BLOG_ERROR, "read returned wrong length");
        return;
    }
    
    // call handler
    o->handler(o->user, event);
    return;
}

int NCDRfkillMonitor_Init (NCDRfkillMonitor *o, BReactor *reactor, NCDRfkillMonitor_handler handler, void *user)
{
    // init arguments
    o->reactor = reactor;
    o->handler = handler;
    o->user = user;
    
    // open rfkill
    if ((o->rfkill_fd = open(RFKILL_DEVICE_NODE, O_RDONLY)) < 0) {
        BLog(BLOG_ERROR, "open failed");
        goto fail0;
    }
    
    // set fd non-blocking
    if (!badvpn_set_nonblocking(o->rfkill_fd)) {
        BLog(BLOG_ERROR, "badvpn_set_nonblocking failed");
        goto fail1;
    }
    
    // init BFileDescriptor
    BFileDescriptor_Init(&o->bfd, o->rfkill_fd, (BFileDescriptor_handler)rfkill_fd_handler, o);
    if (!BReactor_AddFileDescriptor(o->reactor, &o->bfd)) {
        BLog(BLOG_ERROR, "BReactor_AddFileDescriptor failed");
        goto fail1;
    }
    BReactor_SetFileDescriptorEvents(o->reactor, &o->bfd, BREACTOR_READ);
    
    DebugObject_Init(&o->d_obj);
    return 1;
    
fail1:
    if (close(o->rfkill_fd) < 0) {
        BLog(BLOG_ERROR, "close failed");
    }
fail0:
    return 0;
}

void NCDRfkillMonitor_Free (NCDRfkillMonitor *o)
{
    DebugObject_Free(&o->d_obj);
    
    // free BFileDescriptor
    BReactor_RemoveFileDescriptor(o->reactor, &o->bfd);
    
    // close rfkill
    if (close(o->rfkill_fd) < 0) {
        BLog(BLOG_ERROR, "close failed");
    }
}

/**
 * @file BUnixSignal.c
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

#include <inttypes.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#ifdef BADVPN_USE_SIGNALFD
#include <sys/signalfd.h>
#endif

#include <misc/balloc.h>
#include <misc/nonblocking.h>
#include <base/BLog.h>

#include <system/BUnixSignal.h>

#include <generated/blog_channel_BUnixSignal.h>

#define BUNIXSIGNAL_MAX_SIGNALS 64

#ifdef BADVPN_USE_SIGNALFD

static void signalfd_handler (BUnixSignal *o, int events)
{
    DebugObject_Access(&o->d_obj);
    
    // read a signal
    struct signalfd_siginfo siginfo;
    int bytes = read(o->signalfd_fd, &siginfo, sizeof(siginfo));
    if (bytes < 0) {
        int error = errno;
        if (error == EAGAIN || error == EWOULDBLOCK) {
            return;
        }
        BLog(BLOG_ERROR, "read failed (%d)", error);
        return;
    }
    ASSERT_FORCE(bytes == sizeof(siginfo))
    
    // check signal
    if (siginfo.ssi_signo > INT_MAX) {
        BLog(BLOG_ERROR, "read returned out of int range signo (%"PRIu32")", siginfo.ssi_signo);
        return;
    }
    int signo = siginfo.ssi_signo;
    if (sigismember(&o->signals, signo) <= 0) {
        BLog(BLOG_ERROR, "read returned wrong signo (%d)", signo);
        return;
    }
    
    BLog(BLOG_DEBUG, "dispatching signal %d", signo);
    
    // call handler
    o->handler(o->user, signo);
    return;
}

#endif

#ifdef BADVPN_USE_KEVENT

static void kevent_handler (struct BUnixSignal_kevent_entry *entry, u_int fflags, intptr_t data)
{
    BUnixSignal *o = entry->parent;
    DebugObject_Access(&o->d_obj);
    
    // call signal
    o->handler(o->user, entry->signo);
    return;
}

#endif

#ifdef BADVPN_USE_SELFPIPE

struct BUnixSignal_selfpipe_entry *bunixsignal_selfpipe_entries[BUNIXSIGNAL_MAX_SIGNALS];

static void free_selfpipe_entry (struct BUnixSignal_selfpipe_entry *entry)
{
    BUnixSignal *o = entry->parent;
    
    // uninstall signal handler
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_DFL;
    sigemptyset(&act.sa_mask);
    ASSERT_FORCE(sigaction(entry->signo, &act, NULL) == 0)
    
    // free BFileDescriptor
    BReactor_RemoveFileDescriptor(o->reactor, &entry->pipe_read_bfd);
    
    // close pipe
    ASSERT_FORCE(close(entry->pipefds[0]) == 0)
    ASSERT_FORCE(close(entry->pipefds[1]) == 0)
}

static void pipe_read_fd_handler (struct BUnixSignal_selfpipe_entry *entry, int events)
{
    BUnixSignal *o = entry->parent;
    DebugObject_Access(&o->d_obj);
    
    // read a byte
    uint8_t b;
    if (read(entry->pipefds[0], &b, sizeof(b)) < 0) {
        int error = errno;
        if (error == EAGAIN || error == EWOULDBLOCK) {
            return;
        }
        BLog(BLOG_ERROR, "read failed (%d)", error);
        return;
    }
    
    // call handler
    o->handler(o->user, entry->signo);
    return;
}

static void signal_handler (int signo)
{
    ASSERT(signo >= 0)
    ASSERT(signo < BUNIXSIGNAL_MAX_SIGNALS)
    
    struct BUnixSignal_selfpipe_entry *entry = bunixsignal_selfpipe_entries[signo];
    
    uint8_t b = 0;
    write(entry->pipefds[1], &b, sizeof(b));
}

#endif

int BUnixSignal_Init (BUnixSignal *o, BReactor *reactor, sigset_t signals, BUnixSignal_handler handler, void *user)
{
    // init arguments
    o->reactor = reactor;
    o->signals = signals;
    o->handler = handler;
    o->user = user;
    
    #ifdef BADVPN_USE_SIGNALFD
    
    // init signalfd fd
    if ((o->signalfd_fd = signalfd(-1, &o->signals, 0)) < 0) {
        BLog(BLOG_ERROR, "signalfd failed");
        goto fail0;
    }
    
    // set non-blocking
    if (fcntl(o->signalfd_fd, F_SETFL, O_NONBLOCK) < 0) {
        BLog(BLOG_ERROR, "cannot set non-blocking");
        goto fail1;
    }
    
    // init signalfd BFileDescriptor
    BFileDescriptor_Init(&o->signalfd_bfd, o->signalfd_fd, (BFileDescriptor_handler)signalfd_handler, o);
    if (!BReactor_AddFileDescriptor(o->reactor, &o->signalfd_bfd)) {
        BLog(BLOG_ERROR, "BReactor_AddFileDescriptor failed");
        goto fail1;
    }
    BReactor_SetFileDescriptorEvents(o->reactor, &o->signalfd_bfd, BREACTOR_READ);
    
    // block signals
    if (pthread_sigmask(SIG_BLOCK, &o->signals, 0) != 0) {
        BLog(BLOG_ERROR, "pthread_sigmask block failed");
        goto fail2;
    }
    
    #endif
    
    #ifdef BADVPN_USE_KEVENT
    
    // count signals
    int num_signals = 0;
    for (int i = 0; i < BUNIXSIGNAL_MAX_SIGNALS; i++) {
        if (!sigismember(&o->signals, i)) {
            continue;
        }
        num_signals++;
    }
    
    // allocate array
    if (!(o->entries = BAllocArray(num_signals, sizeof(o->entries[0])))) {
        BLog(BLOG_ERROR, "BAllocArray failed");
        goto fail0;
    }
    
    // init kevents
    o->num_entries = 0;
    for (int i = 0; i < BUNIXSIGNAL_MAX_SIGNALS; i++) {
        if (!sigismember(&o->signals, i)) {
            continue;
        }
        struct BUnixSignal_kevent_entry *entry = &o->entries[o->num_entries];
        entry->parent = o;
        entry->signo = i;
        if (!BReactorKEvent_Init(&entry->kevent, o->reactor, (BReactorKEvent_handler)kevent_handler, entry, entry->signo, EVFILT_SIGNAL, 0, 0)) {
            BLog(BLOG_ERROR, "BReactorKEvent_Init failed");
            goto fail2;
        }
        o->num_entries++;
    }
    
    // block signals
    if (pthread_sigmask(SIG_BLOCK, &o->signals, 0) != 0) {
        BLog(BLOG_ERROR, "pthread_sigmask block failed");
        goto fail2;
    }
    
    #endif
    
    #ifdef BADVPN_USE_SELFPIPE
    
    // count signals
    int num_signals = 0;
    for (int i = 1; i < BUNIXSIGNAL_MAX_SIGNALS; i++) {
        if (!sigismember(&o->signals, i)) {
            continue;
        }
        num_signals++;
    }
    
    // allocate array
    if (!(o->entries = BAllocArray(num_signals, sizeof(o->entries[0])))) {
        BLog(BLOG_ERROR, "BAllocArray failed");
        goto fail0;
    }
    
    // init entries
    o->num_entries = 0;
    for (int i = 1; i < BUNIXSIGNAL_MAX_SIGNALS; i++) {
        if (!sigismember(&o->signals, i)) {
            continue;
        }
        
        struct BUnixSignal_selfpipe_entry *entry = &o->entries[o->num_entries];
        entry->parent = o;
        entry->signo = i;
        
        // init pipe
        if (pipe(entry->pipefds) < 0) {
            BLog(BLOG_ERROR, "pipe failed");
            goto loop_fail0;
        }
        
        // set pipe ends non-blocking
        if (!badvpn_set_nonblocking(entry->pipefds[0]) || !badvpn_set_nonblocking(entry->pipefds[1])) {
            BLog(BLOG_ERROR, "set nonblocking failed");
            goto loop_fail1;
        }
        
        // init read end BFileDescriptor
        BFileDescriptor_Init(&entry->pipe_read_bfd, entry->pipefds[0], (BFileDescriptor_handler)pipe_read_fd_handler, entry);
        if (!BReactor_AddFileDescriptor(o->reactor, &entry->pipe_read_bfd)) {
            BLog(BLOG_ERROR, "BReactor_AddFileDescriptor failed");
            goto loop_fail1;
        }
        BReactor_SetFileDescriptorEvents(o->reactor, &entry->pipe_read_bfd, BREACTOR_READ);
        
        // set global entry pointer
        bunixsignal_selfpipe_entries[entry->signo] = entry;
        
        // install signal handler
        struct sigaction act;
        memset(&act, 0, sizeof(act));
        act.sa_handler = signal_handler;
        sigemptyset(&act.sa_mask);
        if (sigaction(entry->signo, &act, NULL) < 0) {
            BLog(BLOG_ERROR, "sigaction failed");
            goto loop_fail2;
        }
        
        o->num_entries++;
        
        continue;
        
    loop_fail2:
        BReactor_RemoveFileDescriptor(o->reactor, &entry->pipe_read_bfd);
    loop_fail1:
        ASSERT_FORCE(close(entry->pipefds[0]) == 0)
        ASSERT_FORCE(close(entry->pipefds[1]) == 0)
    loop_fail0:
        goto fail2;
    }
    
    #endif
    
    DebugObject_Init(&o->d_obj);
    
    return 1;
    
    #ifdef BADVPN_USE_SIGNALFD
fail2:
    BReactor_RemoveFileDescriptor(o->reactor, &o->signalfd_bfd);
fail1:
    ASSERT_FORCE(close(o->signalfd_fd) == 0)
    #endif
    
    #ifdef BADVPN_USE_KEVENT
fail2:
    while (o->num_entries > 0) {
        BReactorKEvent_Free(&o->entries[o->num_entries - 1].kevent);
        o->num_entries--;
    }
    BFree(o->entries);
    #endif
    
    #ifdef BADVPN_USE_SELFPIPE
fail2:
    while (o->num_entries > 0) {
        free_selfpipe_entry(&o->entries[o->num_entries - 1]);
        o->num_entries--;
    }
    BFree(o->entries);
    #endif
    
fail0:
    return 0;
}

void BUnixSignal_Free (BUnixSignal *o, int unblock)
{
    ASSERT(unblock == 0 || unblock == 1)
    DebugObject_Free(&o->d_obj);
    
    #ifdef BADVPN_USE_SIGNALFD
    
    if (unblock) {
        // unblock signals
        ASSERT_FORCE(pthread_sigmask(SIG_UNBLOCK, &o->signals, 0) == 0)
    }
    
    // free signalfd BFileDescriptor
    BReactor_RemoveFileDescriptor(o->reactor, &o->signalfd_bfd);
    
    // free signalfd fd
    ASSERT_FORCE(close(o->signalfd_fd) == 0)
    
    #endif
    
    #ifdef BADVPN_USE_KEVENT
    
    if (unblock) {
        // unblock signals
        ASSERT_FORCE(pthread_sigmask(SIG_UNBLOCK, &o->signals, 0) == 0)
    }
    
    // free kevents
    while (o->num_entries > 0) {
        BReactorKEvent_Free(&o->entries[o->num_entries - 1].kevent);
        o->num_entries--;
    }
    
    // free array
    BFree(o->entries);
    
    #endif
    
    #ifdef BADVPN_USE_SELFPIPE
    
    if (!unblock) {
        // block signals
        if (pthread_sigmask(SIG_BLOCK, &o->signals, 0) != 0) {
            BLog(BLOG_ERROR, "pthread_sigmask block failed");
        }
    }
    
    // free entries
    while (o->num_entries > 0) {
        free_selfpipe_entry(&o->entries[o->num_entries - 1]);
        o->num_entries--;
    }
    
    // free array
    BFree(o->entries);
    
    #endif
}

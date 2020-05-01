/**
 * @file BUnixSignal.h
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
 * 
 * @section DESCRIPTION
 * 
 * Object for catching unix signals.
 */

#ifndef BADVPN_SYSTEM_BUNIXSIGNAL_H
#define BADVPN_SYSTEM_BUNIXSIGNAL_H

#if (defined(BADVPN_USE_SIGNALFD) + defined(BADVPN_USE_KEVENT) + defined(BADVPN_USE_SELFPIPE)) != 1
#error Unknown signal backend or too many signal backends
#endif

#include <unistd.h>
#include <signal.h>

#include <misc/debug.h>
#include <system/BReactor.h>
#include <base/DebugObject.h>

struct BUnixSignal_s;

/**
 * Handler function called when a signal is received.
 * 
 * @param user as in {@link BUnixSignal_Init}
 * @param signo signal number. Will be one of the signals provided to {@link signals}.
 */
typedef void (*BUnixSignal_handler) (void *user, int signo);

#ifdef BADVPN_USE_KEVENT
struct BUnixSignal_kevent_entry {
    struct BUnixSignal_s *parent;
    int signo;
    BReactorKEvent kevent;
};
#endif

#ifdef BADVPN_USE_SELFPIPE
struct BUnixSignal_selfpipe_entry {
    struct BUnixSignal_s *parent;
    int signo;
    int pipefds[2];
    BFileDescriptor pipe_read_bfd;
};
#endif

/**
 * Object for catching unix signals.
 */
typedef struct BUnixSignal_s {
    BReactor *reactor;
    sigset_t signals;
    BUnixSignal_handler handler;
    void *user;
    
    #ifdef BADVPN_USE_SIGNALFD
    int signalfd_fd;
    BFileDescriptor signalfd_bfd;
    #endif
    
    #ifdef BADVPN_USE_KEVENT
    struct BUnixSignal_kevent_entry *entries;
    int num_entries;
    #endif
    
    #ifdef BADVPN_USE_SELFPIPE
    struct BUnixSignal_selfpipe_entry *entries;
    int num_entries;
    #endif
    
    DebugObject d_obj;
} BUnixSignal;

/**
 * Initializes the object.
 * {@link BLog_Init} must have been done.
 * 
 * WARNING: for every signal number there should be at most one {@link BUnixSignal}
 * object handling it (or anything else that could interfere).
 * 
 * This blocks the signal using pthread_sigmask() and sets up signalfd() for receiving
 * signals.
 *
 * @param o the object
 * @param reactor reactor we live in
 * @param signals signals to handle. See man 3 sigsetops.
 * @param handler handler function to call when a signal is received
 * @param user value passed to callback function
 * @return 1 on success, 0 on failure
 */
int BUnixSignal_Init (BUnixSignal *o, BReactor *reactor, sigset_t signals, BUnixSignal_handler handler, void *user) WARN_UNUSED;

/**
 * Frees the object.
 * 
 * @param o the object
 * @param unblock whether to unblock the signals using pthread_sigmask(). Not unblocking it
 *                can be used while the program is exiting gracefully to prevent the
 *                signals from being handled handled according to its default disposition
 *                after this function is called. Must be 0 or 1.
 */
void BUnixSignal_Free (BUnixSignal *o, int unblock);

#endif

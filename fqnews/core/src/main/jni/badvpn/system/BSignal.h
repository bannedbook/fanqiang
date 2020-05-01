/**
 * @file BSignal.h
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
 * A global object for catching program termination requests.
 */

#ifndef BADVPN_SYSTEM_BSIGNAL_H
#define BADVPN_SYSTEM_BSIGNAL_H

#include <misc/debug.h>
#include <system/BReactor.h>

typedef void (*BSignal_handler) (void *user);

/**
 * Initializes signal handling.
 * The object is created in not capturing state.
 * {@link BLog_Init} must have been done.
 * 
 * WARNING: make sure this won't interfere with other components:
 *   - On Linux, this uses {@link BUnixSignal} to catch SIGTERM, SIGINT
 *     and SIGHUP.
 *   - on Windows, this sets up a handler with SetConsoleCtrlHandler.
 *
 * @param reactor {@link BReactor} from which the handler will be called
 * @param handler callback function invoked from the reactor
 * @param user value passed to callback function
 * @return 1 on success, 0 on failure
 */
int BSignal_Init (BReactor *reactor, BSignal_handler handler, void *user) WARN_UNUSED;

/**
 * Finishes signal handling.
 * {@link BSignal_Init} must not be called again.
 */
void BSignal_Finish (void);

#endif

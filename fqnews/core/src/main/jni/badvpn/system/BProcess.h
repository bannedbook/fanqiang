/**
 * @file BProcess.h
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

#ifndef BADVPN_BPROCESS_H
#define BADVPN_BPROCESS_H

#include <stdint.h>
#include <unistd.h>

#include <misc/debug.h>
#include <misc/debugerror.h>
#include <structure/LinkedList1.h>
#include <base/DebugObject.h>
#include <system/BUnixSignal.h>
#include <base/BPending.h>

/**
 * Manages child processes.
 * There may be at most one process manager at any given time. This restriction is not
 * enforced, however.
 */
typedef struct {
    BReactor *reactor;
    BUnixSignal signal;
    LinkedList1 processes;
    BPending wait_job;
    DebugObject d_obj;
} BProcessManager;

/**
 * Handler called when the process terminates.
 * The process object must be freed from the job context of this handler.
 * {@link BProcess_Terminate} or {@link BProcess_Kill} must not be called
 * after this handler is called.
 * 
 * @param user as in {@link BProcess_InitWithFds} or {@link BProcess_Init}
 * @param normally whether the child process terminated normally (0 or 1)
 * @param normally_exit_status if the child process terminated normally, its exit
 *                             status; otherwise undefined
 */
typedef void (*BProcess_handler) (void *user, int normally, uint8_t normally_exit_status);

/**
 * Represents a child process.
 */
typedef struct {
    BProcessManager *m;
    BProcess_handler handler;
    void *user;
    pid_t pid;
    LinkedList1Node list_node; // node in BProcessManager.processes
    DebugObject d_obj;
    DebugError d_err;
} BProcess;

/**
 * Initializes the process manager.
 * There may be at most one process manager at any given time. This restriction is not
 * enforced, however.
 * 
 * @param o the object
 * @param reactor reactor we live in
 * @return 1 on success, 0 on failure
 */
int BProcessManager_Init (BProcessManager *o, BReactor *reactor) WARN_UNUSED;

/**
 * Frees the process manager.
 * There must be no {@link BProcess} objects using this process manager.
 * 
 * @param o the object
 */
void BProcessManager_Free (BProcessManager *o);

struct BProcess_params {
    const char *username;
    const int *fds;
    const int *fds_map;
    int do_setsid;
};

/**
 * Initializes the process.
 * 'file', 'argv', 'username', 'fds' and 'fds_map' arguments are only used during this
 * function call.
 * If no file descriptor is mapped to a standard stream (file descriptors 0, 1, 2),
 * then /dev/null will be opened in the child for that standard stream.
 * 
 * @param o the object
 * @param m process manager
 * @param handler handler called when the process terminates
 * @param user argument to handler
 * @param file path to executable file
 * @param argv arguments array, including the zeroth argument, terminated with a NULL pointer
 * @param params.username user account to run the program as, or NULL to not switch user
 * @param params.fds array of file descriptors in the parent to map to file descriptors in the child,
 *            terminated with -1
 * @param params.fds_map array of file descriptors in the child that file descriptors in 'fds' will
 *                be mapped to, in the same order. Must contain the same number of file descriptors
 *                as the 'fds' argument, and does not have to be terminated with -1.
 * @param params.do_setsid if set to non-zero, will make the child call setsid() before exec'ing.
 *                         Failure of setsid() will be ignored.
 * @return 1 on success, 0 on failure
 */
int BProcess_Init2 (BProcess *o, BProcessManager *m, BProcess_handler handler, void *user, const char *file, char *const argv[], struct BProcess_params params) WARN_UNUSED;

/**
 * Initializes the process.
 * 'file', 'argv', 'username', 'fds' and 'fds_map' arguments are only used during this
 * function call.
 * If no file descriptor is mapped to a standard stream (file descriptors 0, 1, 2),
 * then /dev/null will be opened in the child for that standard stream.
 * 
 * @param o the object
 * @param m process manager
 * @param handler handler called when the process terminates
 * @param user argument to handler
 * @param file path to executable file
 * @param argv arguments array, including the zeroth argument, terminated with a NULL pointer
 * @param username user account to run the program as, or NULL to not switch user
 * @param fds array of file descriptors in the parent to map to file descriptors in the child,
 *            terminated with -1
 * @param fds_map array of file descriptors in the child that file descriptors in 'fds' will
 *                be mapped to, in the same order. Must contain the same number of file descriptors
 *                as the 'fds' argument, and does not have to be terminated with -1.
 * @return 1 on success, 0 on failure
 */
int BProcess_InitWithFds (BProcess *o, BProcessManager *m, BProcess_handler handler, void *user, const char *file, char *const argv[], const char *username, const int *fds, const int *fds_map) WARN_UNUSED;

/**
 * Initializes the process.
 * Like {@link BProcess_InitWithFds}, but without file descriptor mapping.
 * 'file', 'argv' and 'username' arguments are only used during this function call.
 * 
 * @param o the object
 * @param m process manager
 * @param handler handler called when the process terminates
 * @param user argument to handler
 * @param file path to executable file
 * @param argv arguments array, including the zeroth argument, terminated with a NULL pointer
 * @param username user account to run the program as, or NULL to not switch user
 * @return 1 on success, 0 on failure
 */
int BProcess_Init (BProcess *o, BProcessManager *m, BProcess_handler handler, void *user, const char *file, char *const argv[], const char *username) WARN_UNUSED;

/**
 * Frees the process.
 * This does not do anything with the actual child process; it only prevents the user to wait
 * for its termination. If the process terminates while a process manager is running, it will still
 * be waited for (and will not become a zombie).
 * 
 * @param o the object
 */
void BProcess_Free (BProcess *o);

/**
 * Sends the process the SIGTERM signal.
 * Success of this action does NOT mean that the child has terminated.
 * 
 * @param o the object
 * @return 1 on success, 0 on failure
 */
int BProcess_Terminate (BProcess *o);

/**
 * Sends the process the SIGKILL signal.
 * Success of this action does NOT mean that the child has terminated.
 * 
 * @param o the object
 * @return 1 on success, 0 on failure
 */
int BProcess_Kill (BProcess *o);

#endif

/**
 * @file BProcess.c
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

#include <stddef.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <grp.h>
#include <pwd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <misc/offset.h>
#include <misc/open_standard_streams.h>
#include <base/BLog.h>

#include "BProcess.h"

#include <generated/blog_channel_BProcess.h>

#define INITIAL_NUM_GROUPS 24

static void call_handler (BProcess *o, int normally, uint8_t normally_exit_status)
{
    DEBUGERROR(&o->d_err, o->handler(o->user, normally, normally_exit_status))
}

static BProcess * find_process (BProcessManager *o, pid_t pid)
{
    for (LinkedList1Node *node = LinkedList1_GetFirst(&o->processes); node; node = LinkedList1Node_Next(node)) {
        BProcess *p = UPPER_OBJECT(node, BProcess, list_node);
        if (p->pid == pid) {
            return p;
        }
    }
    
    return NULL;
}

static void work_signals (BProcessManager *o)
{
    // read exit status with waitpid()
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid <= 0) {
        return;
    }
    
    // schedule next waitpid
    BPending_Set(&o->wait_job);
    
    // find process
    BProcess *p = find_process(o, pid);
    if (!p) {
        BLog(BLOG_DEBUG, "unknown child %p");
    }
    
    if (WIFEXITED(status)) {
        uint8_t exit_status = WEXITSTATUS(status);
        
        BLog(BLOG_INFO, "child %"PRIiMAX" exited with status %"PRIu8, (intmax_t)pid, exit_status);
        
        if (p) {
            call_handler(p, 1, exit_status);
            return;
        }
    }
    else if (WIFSIGNALED(status)) {
        int signo = WTERMSIG(status);
        
        BLog(BLOG_INFO, "child %"PRIiMAX" exited with signal %d", (intmax_t)pid, signo);
        
        if (p) {
            call_handler(p, 0, 0);
            return;
        }
    }
    else {
        BLog(BLOG_ERROR, "unknown wait status type for pid %"PRIiMAX" (%d)", (intmax_t)pid, status);
    }
}

static void wait_job_handler (BProcessManager *o)
{
    DebugObject_Access(&o->d_obj);
    
    work_signals(o);
    return;
}

static void signal_handler (BProcessManager *o, int signo)
{
    ASSERT(signo == SIGCHLD)
    DebugObject_Access(&o->d_obj);
    
    work_signals(o);
    return;
}

int BProcessManager_Init (BProcessManager *o, BReactor *reactor)
{
    // init arguments
    o->reactor = reactor;
    
    // init signal handling
    sigset_t sset;
    ASSERT_FORCE(sigemptyset(&sset) == 0)
    ASSERT_FORCE(sigaddset(&sset, SIGCHLD) == 0)
    if (!BUnixSignal_Init(&o->signal, o->reactor, sset, (BUnixSignal_handler)signal_handler, o)) {
        BLog(BLOG_ERROR, "BUnixSignal_Init failed");
        goto fail0;
    }
    
    // init processes list
    LinkedList1_Init(&o->processes);
    
    // init wait job
    BPending_Init(&o->wait_job, BReactor_PendingGroup(o->reactor), (BPending_handler)wait_job_handler, o);
    
    DebugObject_Init(&o->d_obj);
    
    return 1;
    
fail0:
    return 0;
}

void BProcessManager_Free (BProcessManager *o)
{
    ASSERT(LinkedList1_IsEmpty(&o->processes))
    DebugObject_Free(&o->d_obj);
    
    // free wait job
    BPending_Free(&o->wait_job);
    
    // free signal handling
    BUnixSignal_Free(&o->signal, 1);
}

static int fds_contains (const int *fds, int fd, size_t *pos)
{
    for (size_t i = 0; fds[i] >= 0; i++) {
        if (fds[i] == fd) {
            if (pos) {
                *pos = i;
            }
            
            return 1;
        }
    }
    
    return 0;
}

int BProcess_Init2 (BProcess *o, BProcessManager *m, BProcess_handler handler, void *user, const char *file, char *const argv[], struct BProcess_params params)
{
    int res = 0;
    
    // init arguments
    o->m = m;
    o->handler = handler;
    o->user = user;
    
    // count fds
    size_t num_fds;
    for (num_fds = 0; params.fds[num_fds] >= 0; num_fds++);
    
    // We retrieve all the needed info and allocate all needed memory
    // before the fork, because doing these things in the child after
    // the fork may be unsafe.
    
    // Get the max FD number.
    int max_fd = sysconf(_SC_OPEN_MAX);
    if (max_fd < 0) {
        BLog(BLOG_ERROR, "sysconf(_SC_OPEN_MAX)");
        goto fail0;
    }
    
    char *pwnam_buf = NULL;
    struct passwd pwd;
    gid_t groups_static[INITIAL_NUM_GROUPS];
    gid_t *groups = groups_static;
    int num_groups = INITIAL_NUM_GROUPS;    
    
    if (params.username) {
        // Get the max getpwnam_r buffer size.
        long pwnam_bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
        if (pwnam_bufsize < 0) {
            pwnam_bufsize = 16384;
        }
        
        // Allocate memory for getpwnam_r.
        pwnam_buf = malloc(pwnam_bufsize);
        if (!pwnam_buf) {
            BLog(BLOG_ERROR, "malloc failed");
            goto fail0;
        }
        
        // Get information about the user.
        struct passwd *getpwnam_res;
        getpwnam_r(params.username, &pwd, pwnam_buf, pwnam_bufsize, &getpwnam_res);
        if (!getpwnam_res) {
            BLog(BLOG_ERROR, "getpwnam_r failed");
            goto fail1;
        }
        
        // Retrieve the group list.
        // Start with a small auto-allocated list and only if it turns out
        // to be too small resort to malloc.
        while (1) {
            int groups_ret = getgrouplist(params.username, pwd.pw_gid, groups, &num_groups);
            if ((groups_ret < 0) ? (num_groups <= 0) : (num_groups != groups_ret)) {
                BLog(BLOG_ERROR, "getgrouplist behaved inconsistently");
                goto fail2;
            }
            
            if (groups_ret >= 0) {
                break;
            }
            
            if (groups != groups_static) {
                free(groups);
            }
            groups = malloc(num_groups * sizeof(gid_t));
            if (!groups) {
                BLog(BLOG_ERROR, "malloc failed");
                goto fail2;
            }
        }
    }

    // Make a copy of the file descriptors array.
    int *fds2 = malloc((num_fds + 1) * sizeof(fds2[0]));
    if (!fds2) {
        BLog(BLOG_ERROR, "malloc failed");
        goto fail2;
    }
    memcpy(fds2, params.fds, (num_fds + 1) * sizeof(fds2[0]));
    
    // block signals
    // needed to prevent parent's signal handlers from being called
    // in the child
    sigset_t sset_all;
    sigfillset(&sset_all);
    sigset_t sset_old;
    if (pthread_sigmask(SIG_SETMASK, &sset_all, &sset_old) != 0) {
        BLog(BLOG_ERROR, "pthread_sigmask failed");
        goto fail3;
    }
    
    // fork
    pid_t pid = fork();
    
    if (pid == 0) {
        // this is child
        
        // restore signal dispositions
        for (int i = 1; i < NSIG; i++) {
            struct sigaction sa;
            memset(&sa, 0, sizeof(sa));
            sa.sa_handler = SIG_DFL;
            sa.sa_flags = 0;
            sigaction(i, &sa, NULL);
        }
        
        // unblock signals
        sigset_t sset_none;
        sigemptyset(&sset_none);
        if (pthread_sigmask(SIG_SETMASK, &sset_none, NULL) != 0) {
            abort();
        }
        
        // close file descriptors, except the given fds
        for (int i = 0; i < max_fd; i++) {
            if (!fds_contains(fds2, i, NULL)) {
                close(i);
            }
        }
        
        // map fds to requested fd numbers
        while (*fds2 >= 0) {
            // resolve possible conflict
            size_t cpos;
            if (fds_contains(fds2 + 1, *params.fds_map, &cpos)) {
                // dup() the fd to a new number; the old one will be closed
                // in the following dup2()
                if ((fds2[1 + cpos] = dup(fds2[1 + cpos])) < 0) {
                    abort();
                }
            }
            
            if (*fds2 != *params.fds_map) {
                // dup fd
                if (dup2(*fds2, *params.fds_map) < 0) {
                    abort();
                }
                
                // close original fd
                close(*fds2);
            }
            
            fds2++;
            params.fds_map++;
        }
        
        // make sure standard streams are open
        open_standard_streams();
        
        // make session leader if requested
        if (params.do_setsid) {
            setsid();
        }
        
        // assume identity of username, if requested
        if (params.username) {
            if (setgroups(num_groups, groups) < 0) {
                abort();
            }
            
            if (setgid(pwd.pw_gid) < 0) {
                abort();
            }
            
            if (setuid(pwd.pw_uid) < 0) {
                abort();
            }
        }
        
        // do the exec
        execv(file, argv);
        
        // if we're still here, something went wrong
        abort();
    }
    
    // restore original signal mask
    ASSERT_FORCE(pthread_sigmask(SIG_SETMASK, &sset_old, NULL) == 0)
    
    if (pid < 0) {
        BLog(BLOG_ERROR, "fork failed");
        goto fail3;
    }
    
    // remember pid
    o->pid = pid;
    
    // add to processes list
    LinkedList1_Append(&o->m->processes, &o->list_node);
    
    DebugObject_Init(&o->d_obj);
    DebugError_Init(&o->d_err, BReactor_PendingGroup(m->reactor));
    
    // Returning success, but cleanup first.
    res = 1;
    
fail3:
    free(fds2);
fail2:
    if (groups != groups_static) {
        free(groups);
    }
fail1:
    free(pwnam_buf);
fail0:
    return res;
}

int BProcess_InitWithFds (BProcess *o, BProcessManager *m, BProcess_handler handler, void *user, const char *file, char *const argv[], const char *username, const int *fds, const int *fds_map)
{
    struct BProcess_params params;
    params.username = username;
    params.fds = fds;
    params.fds_map = fds_map;
    params.do_setsid = 0;
    
    return BProcess_Init2(o, m, handler, user, file, argv, params);
}

int BProcess_Init (BProcess *o, BProcessManager *m, BProcess_handler handler, void *user, const char *file, char *const argv[], const char *username)
{
    int fds[] = {-1};
    
    return BProcess_InitWithFds(o, m, handler, user, file, argv, username, fds, NULL);
}

void BProcess_Free (BProcess *o)
{
    DebugError_Free(&o->d_err);
    DebugObject_Free(&o->d_obj);
    
    // remove from processes list
    LinkedList1_Remove(&o->m->processes, &o->list_node);
}

int BProcess_Terminate (BProcess *o)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    
    ASSERT(o->pid > 0)
    
    if (kill(o->pid, SIGTERM) < 0) {
        BLog(BLOG_ERROR, "kill(%"PRIiMAX", SIGTERM) failed", (intmax_t)o->pid);
        return 0;
    }
    
    return 1;
}

int BProcess_Kill (BProcess *o)
{
    DebugObject_Access(&o->d_obj);
    DebugError_AssertNoError(&o->d_err);
    
    ASSERT(o->pid > 0)
    
    if (kill(o->pid, SIGKILL) < 0) {
        BLog(BLOG_ERROR, "kill(%"PRIiMAX", SIGKILL) failed", (intmax_t)o->pid);
        return 0;
    }
    
    return 1;
}

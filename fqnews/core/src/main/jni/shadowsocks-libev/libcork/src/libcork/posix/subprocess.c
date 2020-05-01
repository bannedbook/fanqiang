/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012-2014, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>

#include "libcork/core.h"
#include "libcork/ds.h"
#include "libcork/os/subprocess.h"
#include "libcork/threads/basics.h"
#include "libcork/helpers/errors.h"
#include "libcork/helpers/posix.h"


#if !defined(CORK_DEBUG_SUBPROCESS)
#define CORK_DEBUG_SUBPROCESS  0
#endif

#if CORK_DEBUG_SUBPROCESS
#include <stdio.h>
#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG(...) /* no debug messages */
#endif


/*-----------------------------------------------------------------------
 * Subprocess groups
 */

#define BUF_SIZE  4096

struct cork_subprocess_group {
    cork_array(struct cork_subprocess *)  subprocesses;
};

struct cork_subprocess_group *
cork_subprocess_group_new(void)
{
    struct cork_subprocess_group  *group =
        cork_new(struct cork_subprocess_group);
    cork_pointer_array_init
        (&group->subprocesses, (cork_free_f) cork_subprocess_free);
    return group;
}

void
cork_subprocess_group_free(struct cork_subprocess_group *group)
{
    cork_array_done(&group->subprocesses);
    cork_delete(struct cork_subprocess_group, group);
}

void
cork_subprocess_group_add(struct cork_subprocess_group *group,
                          struct cork_subprocess *sub)
{
    cork_array_append(&group->subprocesses, sub);
}


/*-----------------------------------------------------------------------
 * Pipes (parent reads)
 */

struct cork_read_pipe {
    struct cork_stream_consumer  *consumer;
    int  fds[2];
    bool  first;
};

static void
cork_read_pipe_init(struct cork_read_pipe *p, struct cork_stream_consumer *consumer)
{
    p->consumer = consumer;
    p->fds[0] = -1;
    p->fds[1] = -1;
}

static int
cork_read_pipe_close_read(struct cork_read_pipe *p)
{
    if (p->fds[0] != -1) {
        DEBUG("Closing read pipe %d\n", p->fds[0]);
        rii_check_posix(close(p->fds[0]));
        p->fds[0] = -1;
    }
    return 0;
}

static int
cork_read_pipe_close_write(struct cork_read_pipe *p)
{
    if (p->fds[1] != -1) {
        DEBUG("Closing write pipe %d\n", p->fds[1]);
        rii_check_posix(close(p->fds[1]));
        p->fds[1] = -1;
    }
    return 0;
}

static void
cork_read_pipe_close(struct cork_read_pipe *p)
{
    cork_read_pipe_close_read(p);
    cork_read_pipe_close_write(p);
}

static void
cork_read_pipe_done(struct cork_read_pipe *p)
{
    cork_read_pipe_close(p);
}

static int
cork_read_pipe_open(struct cork_read_pipe *p)
{
    if (p->consumer != NULL) {
        int  flags;

        /* We want the read end of the pipe to be non-blocking. */
        DEBUG("[read] Opening pipe\n");
        rii_check_posix(pipe(p->fds));
        DEBUG("[read]   Got read=%d write=%d\n", p->fds[0], p->fds[1]);
        DEBUG("[read]   Setting non-blocking flag on read pipe\n");
        ei_check_posix(flags = fcntl(p->fds[0], F_GETFD));
        flags |= O_NONBLOCK;
        ei_check_posix(fcntl(p->fds[0], F_SETFD, flags));
    }

    p->first = true;
    return 0;

error:
    cork_read_pipe_close(p);
    return -1;
}

static int
cork_read_pipe_dup(struct cork_read_pipe *p, int fd)
{
    if (p->fds[1] != -1) {
        rii_check_posix(dup2(p->fds[1], fd));
    }
    return 0;
}

static int
cork_read_pipe_read(struct cork_read_pipe *p, char *buf, bool *progress)
{
    if (p->fds[0] == -1) {
        return 0;
    }

    do {
        DEBUG("[read] Reading from pipe %d\n", p->fds[0]);
        ssize_t  bytes_read = read(p->fds[0], buf, BUF_SIZE);
        if (bytes_read == -1) {
            if (errno == EAGAIN) {
                /* We've exhausted all of the data currently available. */
                DEBUG("[read]   No more bytes without blocking\n");
                return 0;
            } else if (errno == EINTR) {
                /* Interrupted by a signal; return so that our wait loop can
                 * catch that. */
                DEBUG("[read]   Interrupted by signal\n");
                return 0;
            } else {
                /* An actual error */
                cork_system_error_set();
                DEBUG("[read]   Error: %s\n", cork_error_message());
                return -1;
            }
        } else if (bytes_read == 0) {
            DEBUG("[read]   End of stream\n");
            *progress = true;
            rii_check(cork_stream_consumer_eof(p->consumer));
            rii_check_posix(close(p->fds[0]));
            p->fds[0] = -1;
            return 0;
        } else {
            DEBUG("[read]   Got %zd bytes\n", bytes_read);
            *progress = true;
            rii_check(cork_stream_consumer_data
                      (p->consumer, buf, bytes_read, p->first));
            p->first = false;
        }
    } while (true);
}

static bool
cork_read_pipe_is_finished(struct cork_read_pipe *p)
{
    return p->fds[0] == -1;
}


/*-----------------------------------------------------------------------
 * Pipes (parent writes)
 */

struct cork_write_pipe {
    struct cork_stream_consumer  consumer;
    int  fds[2];
};

static int
cork_write_pipe_close_read(struct cork_write_pipe *p)
{
    if (p->fds[0] != -1) {
        DEBUG("[write] Closing read pipe %d\n", p->fds[0]);
        rii_check_posix(close(p->fds[0]));
        p->fds[0] = -1;
    }
    return 0;
}

static int
cork_write_pipe_close_write(struct cork_write_pipe *p)
{
    if (p->fds[1] != -1) {
        DEBUG("[write] Closing write pipe %d\n", p->fds[1]);
        rii_check_posix(close(p->fds[1]));
        p->fds[1] = -1;
    }
    return 0;
}

static int
cork_write_pipe__data(struct cork_stream_consumer *consumer,
                      const void *buf, size_t size, bool is_first_chunk)
{
    struct cork_write_pipe  *p =
        cork_container_of(consumer, struct cork_write_pipe, consumer);
    rii_check_posix(write(p->fds[1], buf, size));
    return 0;
}

static int
cork_write_pipe__eof(struct cork_stream_consumer *consumer)
{
    struct cork_write_pipe  *p =
        cork_container_of(consumer, struct cork_write_pipe, consumer);
    return cork_write_pipe_close_write(p);
}

static void
cork_write_pipe__free(struct cork_stream_consumer *consumer)
{
}

static void
cork_write_pipe_init(struct cork_write_pipe *p)
{
    p->consumer.data = cork_write_pipe__data;
    p->consumer.eof = cork_write_pipe__eof;
    p->consumer.free = cork_write_pipe__free;
    p->fds[0] = -1;
    p->fds[1] = -1;
}

static void
cork_write_pipe_close(struct cork_write_pipe *p)
{
    cork_write_pipe_close_read(p);
    cork_write_pipe_close_write(p);
}

static void
cork_write_pipe_done(struct cork_write_pipe *p)
{
    cork_write_pipe_close(p);
}

static int
cork_write_pipe_open(struct cork_write_pipe *p)
{
    DEBUG("[write] Opening writer pipe\n");
    rii_check_posix(pipe(p->fds));
    DEBUG("[write]   Got read=%d write=%d\n", p->fds[0], p->fds[1]);
    return 0;
}

static int
cork_write_pipe_dup(struct cork_write_pipe *p, int fd)
{
    if (p->fds[0] != -1) {
        rii_check_posix(dup2(p->fds[0], fd));
    }
    return 0;
}


/*-----------------------------------------------------------------------
 * Subprocesses
 */

struct cork_subprocess {
    pid_t  pid;
    struct cork_write_pipe  stdin_pipe;
    struct cork_read_pipe  stdout_pipe;
    struct cork_read_pipe  stderr_pipe;
    void  *user_data;
    cork_free_f  free_user_data;
    cork_run_f  run;
    int  *exit_code;
    char  buf[BUF_SIZE];
};

struct cork_subprocess *
cork_subprocess_new(void *user_data, cork_free_f free_user_data,
                    cork_run_f run,
                    struct cork_stream_consumer *stdout_consumer,
                    struct cork_stream_consumer *stderr_consumer,
                    int *exit_code)
{
    struct cork_subprocess  *self = cork_new(struct cork_subprocess);
    cork_write_pipe_init(&self->stdin_pipe);
    cork_read_pipe_init(&self->stdout_pipe, stdout_consumer);
    cork_read_pipe_init(&self->stderr_pipe, stderr_consumer);
    self->pid = 0;
    self->user_data = user_data;
    self->free_user_data = free_user_data;
    self->run = run;
    self->exit_code = exit_code;
    return self;
}

void
cork_subprocess_free(struct cork_subprocess *self)
{
    cork_free_user_data(self);
    cork_write_pipe_done(&self->stdin_pipe);
    cork_read_pipe_done(&self->stdout_pipe);
    cork_read_pipe_done(&self->stderr_pipe);
    cork_delete(struct cork_subprocess, self);
}

struct cork_stream_consumer *
cork_subprocess_stdin(struct cork_subprocess *self)
{
    return &self->stdin_pipe.consumer;
}


/*-----------------------------------------------------------------------
 * Executing another program
 */

static int
cork_exec__run(void *vself)
{
    struct cork_exec  *exec = vself;
    return cork_exec_run(exec);
}

static void
cork_exec__free(void *vself)
{
    struct cork_exec  *exec = vself;
    cork_exec_free(exec);
}

struct cork_subprocess *
cork_subprocess_new_exec(struct cork_exec *exec,
                         struct cork_stream_consumer *out,
                         struct cork_stream_consumer *err,
                         int *exit_code)
{
    return cork_subprocess_new
        (exec, cork_exec__free,
         cork_exec__run,
         out, err, exit_code);
}


/*-----------------------------------------------------------------------
 * Running subprocesses
 */

int
cork_subprocess_start(struct cork_subprocess *self)
{
    pid_t  pid;

    /* Create the stdout and stderr pipes. */
    if (cork_write_pipe_open(&self->stdin_pipe) == -1) {
        return -1;
    }
    if (cork_read_pipe_open(&self->stdout_pipe) == -1) {
        cork_write_pipe_close(&self->stdin_pipe);
        return -1;
    }
    if (cork_read_pipe_open(&self->stderr_pipe) == -1) {
        cork_write_pipe_close(&self->stdin_pipe);
        cork_read_pipe_close(&self->stdout_pipe);
        return -1;
    }

    /* Fork the child process. */
    DEBUG("Forking child process\n");
    pid = fork();
    if (pid == 0) {
        /* Child process */
        int  rc;

        /* Close the parent's end of the pipes */
        DEBUG("[child] ");
        cork_write_pipe_close_write(&self->stdin_pipe);
        DEBUG("[child] ");
        cork_read_pipe_close_read(&self->stdout_pipe);
        DEBUG("[child] ");
        cork_read_pipe_close_read(&self->stderr_pipe);

        /* Bind the stdout and stderr pipes */
        if (cork_write_pipe_dup(&self->stdin_pipe, STDIN_FILENO) == -1) {
            _exit(EXIT_FAILURE);
        }
        if (cork_read_pipe_dup(&self->stdout_pipe, STDOUT_FILENO) == -1) {
            _exit(EXIT_FAILURE);
        }
        if (cork_read_pipe_dup(&self->stderr_pipe, STDERR_FILENO) == -1) {
            _exit(EXIT_FAILURE);
        }

        /* Run the subprocess */
        rc = self->run(self->user_data);
        if (CORK_LIKELY(rc == 0)) {
            _exit(EXIT_SUCCESS);
        } else {
            fprintf(stderr, "%s\n", cork_error_message());
            _exit(EXIT_FAILURE);
        }
    } else if (pid < 0) {
        /* Error forking */
        cork_system_error_set();
        return -1;
    } else {
        /* Parent process */
        DEBUG("  Child PID=%d\n", (int) pid);
        self->pid = pid;
        cork_write_pipe_close_read(&self->stdin_pipe);
        cork_read_pipe_close_write(&self->stdout_pipe);
        cork_read_pipe_close_write(&self->stderr_pipe);
        return 0;
    }
}

#if defined(__APPLE__)
#include <pthread.h>
#define THREAD_YIELD   pthread_yield_np
#elif defined(__linux__) || defined(BSD) || defined(__FreeBSD_kernel__) || defined(__GNU__) || defined(__CYGWIN__)
#include <sched.h>
#define THREAD_YIELD   sched_yield
#else
#error "Unknown thread yield implementation"
#endif

static void
cork_subprocess_yield(unsigned int *spin_count)
{
    /* Adapted from
     * http://www.1024cores.net/home/lock-free-algorithms/tricks/spinning */

    if (*spin_count < 10) {
        /* Spin-wait */
        cork_pause();
    } else if (*spin_count < 20) {
        /* A more intense spin-wait */
        int  i;
        for (i = 0; i < 50; i++) {
            cork_pause();
        }
    } else if (*spin_count < 22) {
        THREAD_YIELD();
    } else if (*spin_count < 24) {
        usleep(0);
    } else if (*spin_count < 50) {
        usleep(1);
    } else if (*spin_count < 75) {
        usleep((*spin_count - 49) * 1000);
    } else {
        usleep(25000);
    }

    (*spin_count)++;
}


static int
cork_subprocess_reap(struct cork_subprocess *self, int flags, bool *progress)
{
    int  pid;
    int  status;
    rii_check_posix(pid = waitpid(self->pid, &status, flags));
    if (pid == self->pid) {
        *progress = true;
        self->pid = 0;
        if (self->exit_code != NULL) {
            *self->exit_code = WEXITSTATUS(status);
        }
    }
    return 0;
}

int
cork_subprocess_abort(struct cork_subprocess *self)
{
    if (self->pid > 0) {
        CORK_ATTR_UNUSED bool  progress = false;
        DEBUG("Terminating child process %d\n", (int) self->pid);
        kill(self->pid, SIGTERM);
        unsigned int  spin_count = 0;
        int exitcode = cork_subprocess_reap(self, WNOHANG, &progress);
        while (!progress && spin_count < 50) {
            cork_subprocess_yield(&spin_count);
            exitcode = cork_subprocess_reap(self, WNOHANG, &progress);
        }
        if (progress) {
            return exitcode;
        } else {
            kill(self->pid, SIGKILL);
            return 0;
        }
    } else {
        return 0;
    }
}

bool
cork_subprocess_is_finished(struct cork_subprocess *self)
{
    return (self->pid == 0)
        && cork_read_pipe_is_finished(&self->stdout_pipe)
        && cork_read_pipe_is_finished(&self->stderr_pipe);
}

static int
cork_subprocess_drain_(struct cork_subprocess *self, bool *progress)
{
    rii_check(cork_read_pipe_read(&self->stdout_pipe, self->buf, progress));
    rii_check(cork_read_pipe_read(&self->stderr_pipe, self->buf, progress));
    if (self->pid > 0) {
        return cork_subprocess_reap(self, WNOHANG, progress);
    } else {
        return 0;
    }
}

bool
cork_subprocess_drain(struct cork_subprocess *self)
{
    bool  progress;
    cork_subprocess_drain_(self, &progress);
    return progress;
}

int
cork_subprocess_wait(struct cork_subprocess *self)
{
    unsigned int  spin_count = 0;
    bool  progress;
    while (!cork_subprocess_is_finished(self)) {
        progress = false;
        rii_check(cork_subprocess_drain_(self, &progress));
        if (!progress) {
            cork_subprocess_yield(&spin_count);
        }
    }
    return 0;
}


/*-----------------------------------------------------------------------
 * Running subprocess groups
 */

static int
cork_subprocess_group_terminate(struct cork_subprocess_group *group)
{
    size_t  i;
    for (i = 0; i < cork_array_size(&group->subprocesses); i++) {
        struct cork_subprocess  *sub = cork_array_at(&group->subprocesses, i);
        rii_check(cork_subprocess_abort(sub));
    }
    return 0;
}

int
cork_subprocess_group_start(struct cork_subprocess_group *group)
{
    size_t  i;
    DEBUG("Starting subprocess group\n");
    /* Start each subprocess. */
    for (i = 0; i < cork_array_size(&group->subprocesses); i++) {
        struct cork_subprocess  *sub = cork_array_at(&group->subprocesses, i);
        ei_check(cork_subprocess_start(sub));
    }
    return 0;

error:
    cork_subprocess_group_terminate(group);
    return -1;
}


int
cork_subprocess_group_abort(struct cork_subprocess_group *group)
{
    DEBUG("Aborting subprocess group\n");
    return cork_subprocess_group_terminate(group);
}


bool
cork_subprocess_group_is_finished(struct cork_subprocess_group *group)
{
    size_t  i;
    for (i = 0; i < cork_array_size(&group->subprocesses); i++) {
        struct cork_subprocess  *sub = cork_array_at(&group->subprocesses, i);
        bool  sub_finished = cork_subprocess_is_finished(sub);
        if (!sub_finished) {
            return false;
        }
    }
    return true;
}

static int
cork_subprocess_group_drain_(struct cork_subprocess_group *group,
                             bool *progress)
{
    size_t  i;
    for (i = 0; i < cork_array_size(&group->subprocesses); i++) {
        struct cork_subprocess  *sub = cork_array_at(&group->subprocesses, i);
        rii_check(cork_subprocess_drain_(sub, progress));
    }
    return 0;
}

bool
cork_subprocess_group_drain(struct cork_subprocess_group *group)
{
    bool  progress = false;
    cork_subprocess_group_drain_(group, &progress);
    return progress;
}

int
cork_subprocess_group_wait(struct cork_subprocess_group *group)
{
    unsigned int  spin_count = 0;
    bool  progress;
    DEBUG("Waiting for subprocess group to finish\n");
    while (!cork_subprocess_group_is_finished(group)) {
        progress = false;
        rii_check(cork_subprocess_group_drain_(group, &progress));
        if (!progress) {
            cork_subprocess_yield(&spin_count);
        }
    }
    return 0;
}

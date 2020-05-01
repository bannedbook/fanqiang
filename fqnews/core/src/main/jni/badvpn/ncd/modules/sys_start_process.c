/**
 * @file sys_start_process.c
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
 * Synopsis:
 *   sys.start_process(list command, string mode [, map options])
 * 
 * Options:
 *   "keep_stdout":"true" - Start the program with the same stdout as the NCD process.
 *     Must not be present if the process is being opened for reading.
 *   "keep_stderr":"true" - Start the program with the same stderr as the NCD process.
 *   "do_setsid":"true" - Call setsid() in the child before exec. This is needed to
 *     start the 'agetty' program.
 *   "username":username_string - Start the process under the permissions of the
 *     specified user. 
 *   "term_on_deinit":"false" - do not send SIGTERM to the process when this statement
 *     is requested to terminate
 *   "deinit_kill_time":milliseconds - how long to wait for the process to terminate
 *     after this statement is requested to terminate until we send SIGKILL. If this option
 *     is not present or is "never", SIGKILL will not be sent. If this option is empty, the
 *     process will be sent SIGKILL immediately when the statement is requested to terminate.
 * 
 * Variables:
 *   is_error - "true" if there was an error starting the process, "false" if the process
 *              has been started successfully
 * 
 * Synopsis:
 *   sys.start_process::wait()
 * 
 * Variables:
 *   exit_status - the exit code if the process terminated normally, -1 if it terminated
 *                 with a signal
 * 
 * Synopsis:
 *   sys.start_process::terminate()
 *   sys.start_process::kill()
 * 
 * Synopsis:
 *   sys.start_process::read_pipe()
 * 
 * Description:
 *   Creates a read interface to the process's standard output. Data is read using the
 *   read() method on this object. Read errors are reported implicitly by this statement
 *   going down and the 'is_error' variable changing to "true".
 *   When read_pipe() is initialized for a process, it takes ownership of the read pipe
 *   to the process. When read_pipe() is requested to terminate, it will close the pipe.
 *   Attempting to initialize read_pipe() on a process which was not started with 'r'
 *   in the mode argument, or where another read_pipe() object has already taken ownership
 *   of the read pipe, will result in throwing an error to the interpreter.
 * 
 * Variables:
 *   string is_error - "true" if there was a read error, "false" if not
 * 
 * Synopsis:
 *   sys.start_process::read_pipe::read()
 * 
 * Description:
 *   Reads some data. If a read error occurs, it is reported implicitly via the
 *   read_pipe() object going down. If end of file is reached, this and any future read()
 *   operations will indicate that via the 'not_eof' variable. It is guaranteed that after
 *   EOF is reached, the read_pipe() object will not go down to report any errors.
 *   WARNING: if a read() is requested to terminate before it has completed, the
 *            read_pipe() will become unusable and any read() invocation after that will
 *            throw an error to the interpreter.
 * 
 * Variables:
 *   string (empty) - data that was read, or an empty string on EOF
 *   string not_eof - "true" is EOF was not reached, "false" if it was
 * 
 * Synopsis:
 *   sys.start_process::write_pipe()
 * 
 * Description:
 *   Creates a write interface to the process's standard input. Data is written using the
 *   write() method on this object. Write errors are reported implicitly by this statement
 *   going down and the ''is_error variable changing to "true".
 *   When write_pipe() is initialized for a process, it takes ownership of the write pipe
 *   to the process. When write_pipe() is requested to terminate, it will close the pipe
 *   (unless the close() has been used).
 *   Attempting to initialize write_pipe() on a process which was not started with 'w'
 *   in the mode argument, or where another write_pipe() object has already taken ownership
 *   of the write pope, will result in throwing an error to the interpreter.
 * 
 * Variables:
 *   string is_error - "true" if there was a write error, "false" if not
 * 
 * Synopsis:
 *   sys.start_process::write_pipe::write(string data)
 * 
 * Description:
 *   Writes the given data. If a write error occurs, it is reported implicitly via the
 *   write_pipe() object going down.
 *   WARNING: if a write() is requested to terminate before it has completed, the
 *            write_pipe() will become unusable and any write() or close() invocation after
 *            that will throw an error to the interpreter.
 * 
 * Synopsis:
 *   sys.start_process::write_pipe::close(string data)
 * 
 * Description:
 *   Closes the write pipe. This will make whatever is reading the other end of the pipe
 *   encounter EOF after it has read any pending data. It is guaranteed that after the
 *   pipe is closed, the write_pipe() object will not go down to report any errors.
 *   After close() is performed, any further write() or close() calls are disallowed and
 *   will throw errors to the interpreter.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <limits.h>
#include <unistd.h>

#include <misc/offset.h>
#include <structure/LinkedList0.h>
#include <system/BProcess.h>
#include <system/BConnection.h>
#include <ncd/extra/NCDBuf.h>
#include <ncd/extra/build_cmdline.h>
#include <ncd/extra/NCDBProcessOpts.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_sys_start_process.h>

#define READ_BUF_SIZE 8192

#define PROCESS_STATE_ERROR 1
#define PROCESS_STATE_RUNNING 2
#define PROCESS_STATE_TERMINATED 3
#define PROCESS_STATE_DYING 4

#define READER_STATE_RUNNING 1
#define READER_STATE_EOF 2
#define READER_STATE_ERROR 3
#define READER_STATE_ABORTED 4

#define WRITER_STATE_RUNNING 1
#define WRITER_STATE_CLOSED 2
#define WRITER_STATE_ERROR 3
#define WRITER_STATE_ABORTED 4

struct process_instance {
    NCDModuleInst *i;
    BProcess process;
    BSmallTimer kill_timer;
    LinkedList0 waits_list;
    btime_t deinit_kill_time;
    int term_on_deinit;
    int read_fd;
    int write_fd;
    int exit_status;
    int state;
};

struct wait_instance {
    NCDModuleInst *i;
    struct process_instance *pinst;
    LinkedList0Node waits_list_node;
    int exit_status;
};

struct read_pipe_instance {
    NCDModuleInst *i;
    int state;
    int read_fd;
    BConnection connection;
    NCDBufStore store;
    struct read_instance *read_inst;
};

struct read_instance {
    NCDModuleInst *i;
    struct read_pipe_instance *read_pipe_inst;
    NCDBuf *buf;
    size_t read_size;
};

struct write_pipe_instance {
    NCDModuleInst *i;
    int state;
    int write_fd;
    BConnection connection;
    struct write_instance *write_inst;
};

struct write_instance {
    NCDModuleInst *i;
    struct write_pipe_instance *write_pipe_inst;
    MemRef data;
};

static int parse_mode (NCDModuleInst *i, NCDValRef mode_arg, int *out_read, int *out_write)
{
    if (!NCDVal_IsString(mode_arg)) {
        ModuleLog(i, BLOG_ERROR, "mode argument must be a string");
        return 0;
    }
    
    *out_read = 0;
    *out_write = 0;
    
    MEMREF_LOOP_CHARS(NCDVal_StringMemRef(mode_arg), char_pos, ch, {
        if (ch == 'r') {
            *out_read = 1;
        }
        else if (ch == 'w') {
            *out_write = 1;
        }
        else {
            ModuleLog(i, BLOG_ERROR, "invalid character in mode argument");
            return 0;
        }
    })
    
    return 1;
}

static void process_free (struct process_instance *o)
{
    // close write fd
    if (o->write_fd != -1) {
        if (close(o->write_fd) < 0) {
            ModuleLog(o->i, BLOG_ERROR, "close failed");
        }
    }
    
    // close read fd
    if (o->read_fd != -1) {
        if (close(o->read_fd) < 0) {
            ModuleLog(o->i, BLOG_ERROR, "close failed");
        }
    }
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void process_handler (void *vo, int normally, uint8_t normally_exit_status)
{
    struct process_instance *o = vo;
    ASSERT(o->state == PROCESS_STATE_RUNNING || o->state == PROCESS_STATE_DYING)
    
    ModuleLog(o->i, BLOG_INFO, "process terminated");
    
    // free kill timer
    BReactor_RemoveSmallTimer(o->i->params->iparams->reactor, &o->kill_timer);
    
    // free process
    BProcess_Free(&o->process);
    
    // remember exit code
    o->exit_status = (!normally ? -1 : normally_exit_status);
    
    // finish waits
    LinkedList0Node *ln;
    while ((ln = LinkedList0_GetFirst(&o->waits_list))) {
        struct wait_instance *winst = UPPER_OBJECT(ln, struct wait_instance, waits_list_node);
        ASSERT(winst->pinst == o)
        LinkedList0_Remove(&o->waits_list, &winst->waits_list_node);
        winst->pinst = NULL;
        winst->exit_status = o->exit_status;
        NCDModuleInst_Backend_Up(winst->i);
    }
    
    // if we have been requested to die, then die now
    if (o->state == PROCESS_STATE_DYING) {
        process_free(o);
        return;
    }
    
    // set state
    o->state = PROCESS_STATE_TERMINATED;
}

static void process_kill_timer_handler (BSmallTimer *kill_timer)
{
    struct process_instance *o = UPPER_OBJECT(kill_timer, struct process_instance, kill_timer);
    ASSERT(o->state == PROCESS_STATE_DYING)
    
    ModuleLog(o->i, BLOG_INFO, "killing process after timeout");
    BProcess_Kill(&o->process);
}

static int opts_func_unknown (void *user, NCDValRef key, NCDValRef val)
{
    struct process_instance *o = user;
    
    if (NCDVal_IsString(key) && NCDVal_StringEquals(key, "term_on_deinit")) {
        if (!ncd_read_boolean(val, &o->term_on_deinit)) {
            ModuleLog(o->i, BLOG_ERROR, "term_on_deinit: bad value");
            return 0;
        }
        return 1;
    }
    
    if (NCDVal_IsString(key) && NCDVal_StringEquals(key, "deinit_kill_time")) {
        if (NCDVal_IsString(val) && NCDVal_StringEquals(val, "never")) {
            o->deinit_kill_time = -2;
        }
        else if (NCDVal_IsString(val) && NCDVal_StringEqualsId(val, NCD_STRING_EMPTY)) {
            o->deinit_kill_time = -1;
        }
        else if (!ncd_read_time(val, &o->deinit_kill_time)) {
            ModuleLog(o->i, BLOG_ERROR, "wrong value for deinit_kill_time option");
            return 0;
        }
        return 1;
    }
    
    return 0;
}

static void process_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct process_instance *o = vo;
    o->i = i;
    NCDModuleInst_Backend_PassMemToMethods(i);
    
    // check arguments
    NCDValRef command_arg;
    NCDValRef mode_arg;
    NCDValRef options_arg = NCDVal_NewInvalid();
    if (!NCDVal_ListRead(params->args, 2, &command_arg, &mode_arg) &&
        !NCDVal_ListRead(params->args, 3, &command_arg, &mode_arg, &options_arg)
    ) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    // parse mode
    int is_read;
    int is_write;
    if (!parse_mode(i, mode_arg, &is_read, &is_write)) {
        goto fail0;
    }
    
    // parse options
    NCDBProcessOpts opts;
    int keep_stdout;
    int keep_stderr;
    o->deinit_kill_time = -2;
    o->term_on_deinit = 1;
    if (!NCDBProcessOpts_Init2(&opts, options_arg, opts_func_unknown, o, i, BLOG_CURRENT_CHANNEL, &keep_stdout, &keep_stderr)) {
        goto fail0;
    }
    
    // keep-stdout option and read mode are not compatible
    if (keep_stdout && is_read) {
        ModuleLog(i, BLOG_ERROR, "keep-stdout and read mode are not compatible");
        goto fail1;
    }
    
    // prepare for creating pipes
    int fds[4];
    int fds_map[3];
    int start_num_fds = opts.nfds;
    int num_fds = start_num_fds;
    memcpy(fds, opts.fds, num_fds * sizeof(int));
    memcpy(fds_map, opts.fds_map, num_fds * sizeof(int));
    int read_fd = -1;
    int write_fd = -1;
    
    // create read pipe
    if (is_read) {
        int pipefd[2];
        if (pipe(pipefd) < 0) {
            ModuleLog(i, BLOG_ERROR, "pipe failed");
            goto error1;
        }
        read_fd = pipefd[0];
        fds[num_fds] = pipefd[1];
        fds_map[num_fds++] = STDOUT_FILENO;
    }
    
    // create write pipe
    if (is_write) {
        int pipefd[2];
        if (pipe(pipefd) < 0) {
            ModuleLog(i, BLOG_ERROR, "pipe failed");
            goto error1;
        }
        write_fd = pipefd[1];
        fds[num_fds] = pipefd[0];
        fds_map[num_fds++] = STDIN_FILENO;
    }
    
    // terminate fds array
    fds[num_fds] = -1;
    
    // build process parameters struct
    struct BProcess_params p_params = {};
    p_params.fds = fds;
    p_params.fds_map = fds_map;
    p_params.do_setsid = opts.do_setsid;
    p_params.username = opts.username;
    
    // build command line
    char *exec;
    CmdLine cl;
    if (!ncd_build_cmdline(i, BLOG_CURRENT_CHANNEL, command_arg, &exec, &cl)) {
        goto error1;
    }
    
    // start process
    int res = BProcess_Init2(&o->process, i->params->iparams->manager, process_handler, o, exec, CmdLine_Get(&cl), p_params);
    CmdLine_Free(&cl);
    free(exec);
    if (!res) {
        ModuleLog(i, BLOG_ERROR, "BProcess_Init failed");
        goto error1;
    }
    
    // init kill timer
    BSmallTimer_Init(&o->kill_timer, process_kill_timer_handler);
    
    // close child fds
    while (num_fds-- > start_num_fds) {
        if (close(fds[num_fds]) < 0) {
            ModuleLog(i, BLOG_ERROR, "close failed");
        }
    }
    
    // free opts
    NCDBProcessOpts_Free(&opts);
    
    // init waits list
    LinkedList0_Init(&o->waits_list);
    
    // remember our fds
    o->read_fd = read_fd;
    o->write_fd = write_fd;
    
    // set state
    o->state = PROCESS_STATE_RUNNING;
    
    // go up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail1:
    NCDBProcessOpts_Free(&opts);
fail0:
    NCDModuleInst_Backend_DeadError(i);
    return;
    
error1:
    if (write_fd != -1) {
        if (close(write_fd) < 0) {
            ModuleLog(i, BLOG_ERROR, "close failed");
        }
    }
    if (read_fd != -1) {
        if (close(read_fd) < 0) {
            ModuleLog(i, BLOG_ERROR, "close failed");
        }
    }
    while (num_fds-- > start_num_fds) {
        if (close(fds[num_fds]) < 0) {
            ModuleLog(i, BLOG_ERROR, "close failed");
        }
    }
    NCDBProcessOpts_Free(&opts);
    
    o->read_fd = -1;
    o->write_fd = -1;
    o->state = PROCESS_STATE_ERROR;
    NCDModuleInst_Backend_Up(i);
}

static void process_func_die (void *vo)
{
    struct process_instance *o = vo;
    ASSERT(o->state != PROCESS_STATE_DYING)
    
    // if process is not running, die immediately
    if (o->state != PROCESS_STATE_RUNNING) {
        process_free(o);
        return;
    }
    
    if (o->term_on_deinit) {
        ModuleLog(o->i, BLOG_INFO, "terminating process");
        
        // send termination signal
        BProcess_Terminate(&o->process);
    } else {
        ModuleLog(o->i, BLOG_INFO, "not terminating process as requested");
    }
    
    if (o->deinit_kill_time == -1) {
        // user wants SIGKILL immediately
        ModuleLog(o->i, BLOG_INFO, "killing process immediately");
        BProcess_Kill(&o->process);
    } else if (o->deinit_kill_time >= 0) {
        // user wants SIGKILL after some time
        BReactor_SetSmallTimer(o->i->params->iparams->reactor, &o->kill_timer, BTIMER_SET_RELATIVE, o->deinit_kill_time);
    }
    
    // set state
    o->state = PROCESS_STATE_DYING;
}

static int process_func_getvar (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct process_instance *o = vo;
    
    if (name == NCD_STRING_IS_ERROR) {
        int is_error = (o->state == PROCESS_STATE_ERROR);
        *out = ncd_make_boolean(mem, is_error);
        return 1;
    }
    
    return 0;
}

static void wait_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct wait_instance *o = vo;
    o->i = i;
    
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    struct process_instance *pinst = params->method_user;
    
    if (pinst->state == PROCESS_STATE_ERROR) {
        ModuleLog(i, BLOG_ERROR, "wait() is disallowed after the process has failed to start");
        goto fail0;
    }
    
    if (pinst->state == PROCESS_STATE_TERMINATED) {
        // not waiting, set no pinst
        o->pinst = NULL;
        
        // remember exit code
        o->exit_status = pinst->exit_status;
        
        // go up
        NCDModuleInst_Backend_Up(i);
    } else {
        // waitint, set pinst
        o->pinst = pinst;
        
        // insert to waits list
        LinkedList0_Prepend(&pinst->waits_list, &o->waits_list_node);
    }
    
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void wait_func_die (void *vo)
{
    struct wait_instance *o = vo;
    
    // remove from waits list
    if (o->pinst) {
        LinkedList0_Remove(&o->pinst->waits_list, &o->waits_list_node);
    }
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int wait_func_getvar (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct wait_instance *o = vo;
    ASSERT(!o->pinst)
    
    if (name == NCD_STRING_EXIT_STATUS) {
        if (o->exit_status == -1) {
            *out = NCDVal_NewString(mem, "-1");
        } else {
            *out = ncd_make_uintmax(mem, o->exit_status);
        }
        return 1;
    }
    
    return 0;
}

static void terminate_kill_new_common (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params, int is_kill)
{
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    struct process_instance *pinst = params->method_user;
    
    if (pinst->state == PROCESS_STATE_ERROR) {
        ModuleLog(i, BLOG_ERROR, "terminate()/kill() is disallowed after the process has failed to start");
        goto fail0;
    }
    
    if (pinst->state != PROCESS_STATE_TERMINATED) {
        if (is_kill) {
            BProcess_Kill(&pinst->process);
        } else {
            BProcess_Terminate(&pinst->process);
        }
    }
    
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void terminate_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    terminate_kill_new_common(vo, i, params, 0);
}

static void kill_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    terminate_kill_new_common(vo, i, params, 1);
}

static void read_pipe_free_connection (struct read_pipe_instance *o)
{
    // disconnect read instance
    if (o->read_inst) {
        ASSERT(o->read_inst->read_pipe_inst == o)
        o->read_inst->read_pipe_inst = NULL;
    }
    
    // free store
    NCDBufStore_Free(&o->store);
    
    // free connection read interface
    BConnection_RecvAsync_Free(&o->connection);
    
    // free connection
    BConnection_Free(&o->connection);
    
    // close fd
    if (close(o->read_fd) < 0) {
        ModuleLog(o->i, BLOG_ERROR, "close failed");
    }
}

static void read_pipe_abort (struct read_pipe_instance *o)
{
    ASSERT(o->state == READER_STATE_RUNNING)
    
    // release connection resources
    read_pipe_free_connection(o);
    
    // set state
    o->state = READER_STATE_ABORTED;
}

static void read_pipe_connection_handler (void *vo, int event)
{
    struct read_pipe_instance *o = vo;
    ASSERT(o->state == READER_STATE_RUNNING)
    
    if (event == BCONNECTION_EVENT_RECVCLOSED) {
        // if we have read operation, make it finish with eof
        if (o->read_inst) {
            ASSERT(o->read_inst->read_pipe_inst == o)
            ASSERT(o->read_inst->buf)
            o->read_inst->read_pipe_inst = NULL;
            o->read_inst->read_size = 0;
            NCDModuleInst_Backend_Up(o->read_inst->i);
            o->read_inst = NULL;
        }
        
        // free connection resources
        read_pipe_free_connection(o);
        
        // set state closed
        o->state = READER_STATE_EOF;
        return;
    }
    
    ModuleLog(o->i, BLOG_ERROR, "read pipe error");
    
    // free connection resources
    read_pipe_free_connection(o);
    
    // set state error
    o->state = READER_STATE_ERROR;
    
    // backtrack
    NCDModuleInst_Backend_DownUp(o->i);
}

static void read_pipe_recv_handler_done (void *vo, int data_len)
{
    struct read_pipe_instance *o = vo;
    ASSERT(o->state == READER_STATE_RUNNING)
    ASSERT(o->read_inst)
    ASSERT(o->read_inst->read_pipe_inst == o)
    ASSERT(o->read_inst->buf)
    ASSERT(data_len > 0)
    ASSERT(data_len <= NCDBufStore_BufSize(&o->store))
    
    // finish read operation
    o->read_inst->read_pipe_inst = NULL;
    o->read_inst->read_size = data_len;
    NCDModuleInst_Backend_Up(o->read_inst->i);
    o->read_inst = NULL;
}

static void read_pipe_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct read_pipe_instance *o = vo;
    o->i = i;
    NCDModuleInst_Backend_PassMemToMethods(i);
    
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    struct process_instance *pinst = params->method_user;
    
    if (pinst->read_fd == -1) {
        ModuleLog(i, BLOG_ERROR, "process did not start successfully, was not opened for reading or a read_pipe was already created");
        goto fail0;
    }
    
    // init connection
    if (!BConnection_Init(&o->connection, BConnection_source_pipe(pinst->read_fd, 0), i->params->iparams->reactor, o, read_pipe_connection_handler)) {
        ModuleLog(i, BLOG_ERROR, "BConnection_Init failed");
        goto fail0;
    }
    
    // init connection read interface
    BConnection_RecvAsync_Init(&o->connection);
    
    // set recv done callback
    StreamRecvInterface_Receiver_Init(BConnection_RecvAsync_GetIf(&o->connection), read_pipe_recv_handler_done, o);
    
    // init store
    NCDBufStore_Init(&o->store, READ_BUF_SIZE);
    
    // set variables
    o->state = READER_STATE_RUNNING;
    o->read_fd = pinst->read_fd;
    o->read_inst = NULL;
    
    // steal read fd from process instance
    pinst->read_fd = -1;
    
    // go up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void read_pipe_func_die (void *vo)
{
    struct read_pipe_instance *o = vo;
    
    // free connection resources
    if (o->state == READER_STATE_RUNNING) {
        read_pipe_free_connection(o);
    }
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int read_pipe_func_getvar (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct read_pipe_instance *o = vo;
    
    if (name == NCD_STRING_IS_ERROR) {
        int is_error = (o->state == READER_STATE_ERROR);
        *out = ncd_make_boolean(mem, is_error);
        return 1;
    }
    
    return 0;
}

static void read_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct read_instance *o = vo;
    o->i = i;
    
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    struct read_pipe_instance *read_pipe_inst = params->method_user;
    
    // check if a read error has already occured
    if (read_pipe_inst->state == READER_STATE_ERROR) {
        ModuleLog(i, BLOG_ERROR, "read() is disallowed after a read error has occured");
        goto fail0;
    }
    
    // check if the read_pipe has been aborted
    if (read_pipe_inst->state == READER_STATE_ABORTED) {
        ModuleLog(i, BLOG_ERROR, "read() is disallowed after a read() has been aborted");
        goto fail0;
    }
    
    // if EOF has already been encountered, complete the read immediately
    if (read_pipe_inst->state == READER_STATE_EOF) {
        o->buf = NULL;
        o->read_pipe_inst = NULL;
        o->read_size = 0;
        NCDModuleInst_Backend_Up(i);
        return;
    }
    
    ASSERT(read_pipe_inst->state == READER_STATE_RUNNING)
    
    // check if there's already a read in progress
    if (read_pipe_inst->read_inst) {
        ModuleLog(i, BLOG_ERROR, "read() is disallowed while another read() is in progress");
        goto fail0;
    }
    
    // get buffer
    o->buf = NCDBufStore_GetBuf(&read_pipe_inst->store);
    if (!o->buf) {
        ModuleLog(i, BLOG_ERROR, "NCDBufStore_GetBuf failed");
        goto fail0;
    }
    
    // set read_pipe
    o->read_pipe_inst = read_pipe_inst;
    
    // register read in read_pipe
    read_pipe_inst->read_inst = o;
    
    // receive
    size_t buf_size = NCDBufStore_BufSize(&read_pipe_inst->store);
    int to_read = (buf_size > INT_MAX ? INT_MAX : buf_size);
    StreamRecvInterface_Receiver_Recv(BConnection_RecvAsync_GetIf(&read_pipe_inst->connection), (uint8_t *)NCDBuf_Data(o->buf), to_read);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void read_func_die (void *vo)
{
    struct read_instance *o = vo;
    
    // if we're receiving, abort read_pipe
    if (o->read_pipe_inst) {
        ASSERT(o->read_pipe_inst->state == READER_STATE_RUNNING)
        ASSERT(o->read_pipe_inst->read_inst == o)
        ASSERT(o->buf)
        read_pipe_abort(o->read_pipe_inst);
    }
    
    // release buffer
    if (o->buf) {
        BRefTarget_Deref(NCDBuf_RefTarget(o->buf));
    }
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int read_func_getvar (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct read_instance *o = vo;
    ASSERT(!o->read_pipe_inst)
    ASSERT(!(o->read_size > 0) || o->buf)
    
    if (name == NCD_STRING_EMPTY) {
        if (o->read_size > 0) {
            *out = NCDVal_NewExternalString(mem, NCDBuf_Data(o->buf), o->read_size, NCDBuf_RefTarget(o->buf));
        } else {
            *out = NCDVal_NewIdString(mem, NCD_STRING_EMPTY);
        }
        return 1;
    }
    
    if (name == NCD_STRING_NOT_EOF) {
        int not_eof = (o->read_size > 0);
        *out = ncd_make_boolean(mem, not_eof);
        return 1;
    }
    
    return 0;
}

static void write_pipe_free_connection (struct write_pipe_instance *o)
{
    // disconnect write instance
    if (o->write_inst) {
        ASSERT(o->write_inst->write_pipe_inst == o)
        o->write_inst->write_pipe_inst = NULL;
    }
    
    // free connection send interface
    BConnection_SendAsync_Free(&o->connection);
    
    // free connection
    BConnection_Free(&o->connection);
    
    // close fd
    if (close(o->write_fd) < 0) {
        ModuleLog(o->i, BLOG_ERROR, "close failed");
    }
}

static void write_pipe_abort (struct write_pipe_instance *o)
{
    ASSERT(o->state == WRITER_STATE_RUNNING)
    
    // release connection resources
    write_pipe_free_connection(o);
    
    // set state
    o->state = WRITER_STATE_ABORTED;
}

static void write_pipe_close (struct write_pipe_instance *o)
{
    ASSERT(o->state == WRITER_STATE_RUNNING)
    
    // release connection resources
    write_pipe_free_connection(o);
    
    // set state
    o->state = WRITER_STATE_CLOSED;
}

static void write_pipe_connection_handler (void *vo, int event)
{
    struct write_pipe_instance *o = vo;
    ASSERT(o->state == WRITER_STATE_RUNNING)
    
    ModuleLog(o->i, BLOG_ERROR, "write pipe error");
    
    // free connection resources
    write_pipe_free_connection(o);
    
    // set state error
    o->state = WRITER_STATE_ERROR;
    
    // backtrack
    NCDModuleInst_Backend_DownUp(o->i);
}

static void write_pipe_send_handler_done (void *vo, int data_len)
{
    struct write_pipe_instance *o = vo;
    ASSERT(o->state == WRITER_STATE_RUNNING)
    ASSERT(o->write_inst)
    ASSERT(o->write_inst->write_pipe_inst == o)
    ASSERT(data_len > 0)
    ASSERT(data_len <= o->write_inst->data.len)
    
    struct write_instance *wr = o->write_inst;
    
    // update write progress
    wr->data = MemRef_SubFrom(wr->data, data_len);
    
    // if there is more data, start another write operation
    if (wr->data.len > 0) {
        size_t to_send = (wr->data.len > INT_MAX) ? INT_MAX : wr->data.len;
        StreamPassInterface_Sender_Send(BConnection_SendAsync_GetIf(&o->connection), (uint8_t *)wr->data.ptr, to_send);
        return;
    }
    
    // finish write operation
    wr->write_pipe_inst = NULL;
    NCDModuleInst_Backend_Up(wr->i);
    o->write_inst = NULL;
}

static void write_pipe_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct write_pipe_instance *o = vo;
    o->i = i;
    NCDModuleInst_Backend_PassMemToMethods(i);
    
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    struct process_instance *pinst = params->method_user;
    
    if (pinst->write_fd == -1) {
        ModuleLog(i, BLOG_ERROR, "process did not start successfully, was not opened for writing or a write_pipe was already created");
        goto fail0;
    }
    
    // init connection
    if (!BConnection_Init(&o->connection, BConnection_source_pipe(pinst->write_fd, 0), i->params->iparams->reactor, o, write_pipe_connection_handler)) {
        ModuleLog(i, BLOG_ERROR, "BConnection_Init failed");
        goto fail0;
    }
    
    // init connection send interface
    BConnection_SendAsync_Init(&o->connection);
    
    // set send done callback
    StreamPassInterface_Sender_Init(BConnection_SendAsync_GetIf(&o->connection), write_pipe_send_handler_done, o);
    
    // set variables
    o->state = WRITER_STATE_RUNNING;
    o->write_fd = pinst->write_fd;
    o->write_inst = NULL;
    
    // steal write fd from process instance
    pinst->write_fd = -1;
    
    // go up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void write_pipe_func_die (void *vo)
{
    struct write_pipe_instance *o = vo;
    
    // free connection resources
    if (o->state == WRITER_STATE_RUNNING) {
        write_pipe_free_connection(o);
    }
    
    NCDModuleInst_Backend_Dead(o->i);
}

static int write_pipe_func_getvar (void *vo, NCD_string_id_t name, NCDValMem *mem, NCDValRef *out)
{
    struct write_pipe_instance *o = vo;
    
    if (name == NCD_STRING_IS_ERROR) {
        int is_error = (o->state == WRITER_STATE_ERROR);
        *out = ncd_make_boolean(mem, is_error);
        return 1;
    }
    
    return 0;
}

static void write_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    struct write_instance *o = vo;
    o->i = i;
    
    NCDValRef data_arg;
    if (!NCDVal_ListRead(params->args, 1, &data_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    if (!NCDVal_IsString(data_arg)) {
        ModuleLog(i, BLOG_ERROR, "wrong type");
        goto fail0;
    }
    
    struct write_pipe_instance *write_pipe_inst = params->method_user;
    
    // check if a write error has already occured
    if (write_pipe_inst->state == WRITER_STATE_ERROR) {
        ModuleLog(i, BLOG_ERROR, "write() is disallowed after a write error has occured");
        goto fail0;
    }
    
    // check if the write_pipe has been aborted
    if (write_pipe_inst->state == WRITER_STATE_ABORTED) {
        ModuleLog(i, BLOG_ERROR, "write() is disallowed after a write() has been aborted");
        goto fail0;
    }
    
    // check if the write_pipe has been aborted
    if (write_pipe_inst->state == WRITER_STATE_CLOSED) {
        ModuleLog(i, BLOG_ERROR, "write() is disallowed after close() has been called");
        goto fail0;
    }
    
    ASSERT(write_pipe_inst->state == WRITER_STATE_RUNNING)
    
    // check if there's already a write in progress
    if (write_pipe_inst->write_inst) {
        ModuleLog(i, BLOG_ERROR, "write() is disallowed while another write() is in progress");
        goto fail0;
    }
    
    // initialize write progress state
    o->data = NCDVal_StringMemRef(data_arg);
    
    // if there's nothing to send, go up immediately
    if (o->data.len == 0) {
        o->write_pipe_inst = NULL;
        NCDModuleInst_Backend_Up(i);
        return;
    }
    
    // set write_pipe
    o->write_pipe_inst = write_pipe_inst;
    
    // register write in write_pipe
    write_pipe_inst->write_inst = o;
    
    // start send operation
    size_t to_send = (o->data.len > INT_MAX) ? INT_MAX : o->data.len;
    StreamPassInterface_Sender_Send(BConnection_SendAsync_GetIf(&write_pipe_inst->connection), (uint8_t *)o->data.ptr, to_send);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static void write_func_die (void *vo)
{
    struct write_instance *o = vo;
    
    // if we're sending, abort write_pipe
    if (o->write_pipe_inst) {
        ASSERT(o->write_pipe_inst->state == WRITER_STATE_RUNNING)
        ASSERT(o->write_pipe_inst->write_inst == o)
        write_pipe_abort(o->write_pipe_inst);
    }
    
    NCDModuleInst_Backend_Dead(o->i);
}

static void close_func_new (void *vo, NCDModuleInst *i, const struct NCDModuleInst_new_params *params)
{
    if (!NCDVal_ListRead(params->args, 0)) {
        ModuleLog(i, BLOG_ERROR, "wrong arity");
        goto fail0;
    }
    
    struct write_pipe_instance *write_pipe_inst = params->method_user;
    
    // check if a write error has already occured
    if (write_pipe_inst->state == WRITER_STATE_ERROR) {
        ModuleLog(i, BLOG_ERROR, "close() is disallowed after a write error has occured");
        goto fail0;
    }
    
    // check if the write_pipe has been aborted
    if (write_pipe_inst->state == WRITER_STATE_ABORTED) {
        ModuleLog(i, BLOG_ERROR, "close() is disallowed after a write() has been aborted");
        goto fail0;
    }
    
    // check if the write_pipe has been closed
    if (write_pipe_inst->state == WRITER_STATE_CLOSED) {
        ModuleLog(i, BLOG_ERROR, "close() is disallowed after close() has been called");
        goto fail0;
    }
    
    // close
    write_pipe_close(write_pipe_inst);
    
    // go up
    NCDModuleInst_Backend_Up(i);
    return;
    
fail0:
    NCDModuleInst_Backend_DeadError(i);
}

static struct NCDModule modules[] = {
    {
        .type = "sys.start_process",
        .func_new2 = process_func_new,
        .func_die = process_func_die,
        .func_getvar2 = process_func_getvar,
        .alloc_size = sizeof(struct process_instance)
    }, {
        .type = "sys.start_process::wait",
        .func_new2 = wait_func_new,
        .func_die = wait_func_die,
        .func_getvar2 = wait_func_getvar,
        .alloc_size = sizeof(struct wait_instance)
    }, {
        .type = "sys.start_process::terminate",
        .func_new2 = terminate_func_new
    }, {
        .type = "sys.start_process::kill",
        .func_new2 = kill_func_new
    }, {
        .type = "sys.start_process::read_pipe",
        .func_new2 = read_pipe_func_new,
        .func_die = read_pipe_func_die,
        .func_getvar2 = read_pipe_func_getvar,
        .alloc_size = sizeof(struct read_pipe_instance)
    }, {
        .type = "sys.start_process::read_pipe::read",
        .func_new2 = read_func_new,
        .func_die = read_func_die,
        .func_getvar2 = read_func_getvar,
        .alloc_size = sizeof(struct read_instance)
    }, {
        .type = "sys.start_process::write_pipe",
        .func_new2 = write_pipe_func_new,
        .func_die = write_pipe_func_die,
        .func_getvar2 = write_pipe_func_getvar,
        .alloc_size = sizeof(struct write_pipe_instance)
    }, {
        .type = "sys.start_process::write_pipe::write",
        .func_new2 = write_func_new,
        .func_die = write_func_die,
        .alloc_size = sizeof(struct write_instance)
    }, {
        .type = "sys.start_process::write_pipe::close",
        .func_new2 = close_func_new
    }, {
        .type = NULL
    }
};

const struct NCDModuleGroup ncdmodule_sys_start_process = {
    .modules = modules
};

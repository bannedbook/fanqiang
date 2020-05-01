/**
 * @file command_template.c
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

#include <misc/debug.h>

#include <ncd/modules/command_template.h>

#define STATE_ADDING_LOCK 1
#define STATE_ADDING 2
#define STATE_ADDING_NEED_DELETE 3
#define STATE_DONE 4
#define STATE_DELETING_LOCK 5
#define STATE_DELETING 6

static void lock_handler (command_template_instance *o);
static void process_handler (command_template_instance *o, int normally, uint8_t normally_exit_status);
static void free_template (command_template_instance *o, int is_error);

static void lock_handler (command_template_instance *o)
{
    ASSERT(o->state == STATE_ADDING_LOCK || o->state == STATE_DELETING_LOCK)
    ASSERT(!(o->state == STATE_ADDING_LOCK) || o->do_exec)
    ASSERT(!(o->state == STATE_DELETING_LOCK) || o->undo_exec)
    
    if (o->state == STATE_ADDING_LOCK) {
        // start process
        if (!BProcess_Init(&o->process, o->i->params->iparams->manager, (BProcess_handler)process_handler, o, o->do_exec, CmdLine_Get(&o->do_cmdline), NULL)) {
            NCDModuleInst_Backend_Log(o->i, o->blog_channel, BLOG_ERROR, "BProcess_Init failed");
            free_template(o, 1);
            return;
        }
        
        // set state
        o->state = STATE_ADDING;
    } else {
        // start process
        if (!BProcess_Init(&o->process, o->i->params->iparams->manager, (BProcess_handler)process_handler, o, o->undo_exec, CmdLine_Get(&o->undo_cmdline), NULL)) {
            NCDModuleInst_Backend_Log(o->i, o->blog_channel, BLOG_ERROR, "BProcess_Init failed");
            free_template(o, 1);
            return;
        }
        
        // set state
        o->state = STATE_DELETING;
    }
}

static void process_handler (command_template_instance *o, int normally, uint8_t normally_exit_status)
{
    ASSERT(o->state == STATE_ADDING || o->state == STATE_ADDING_NEED_DELETE || o->state == STATE_DELETING)
    
    // release lock
    BEventLockJob_Release(&o->elock_job);
    
    // free process
    BProcess_Free(&o->process);
    
    if (!normally || normally_exit_status != 0) {
        NCDModuleInst_Backend_Log(o->i, o->blog_channel, BLOG_ERROR, "command failed");
        
        free_template(o, 1);
        return;
    }
    
    switch (o->state) {
        case STATE_ADDING: {
            // set state
            o->state = STATE_DONE;
            
            // signal up
            NCDModuleInst_Backend_Up(o->i);
        } break;
        
        case STATE_ADDING_NEED_DELETE: {
            if (o->undo_exec) {
                // wait for lock
                BEventLockJob_Wait(&o->elock_job);
                
                // set state
                o->state = STATE_DELETING_LOCK;
            } else {
                free_template(o, 0);
                return;
            }
        } break;
        
        case STATE_DELETING: {
            // finish
            free_template(o, 0);
            return;
        } break;
    }
}

void command_template_new (command_template_instance *o, NCDModuleInst *i, const struct NCDModuleInst_new_params *params, command_template_build_cmdline build_cmdline, command_template_free_func free_func, void *user, int blog_channel, BEventLock *elock)
{
    // init arguments
    o->i = i;
    o->free_func = free_func;
    o->user = user;
    o->blog_channel = blog_channel;
    
    // build do command
    if (!build_cmdline(o->i, params->args, 0, &o->do_exec, &o->do_cmdline)) {
        NCDModuleInst_Backend_Log(o->i, o->blog_channel, BLOG_ERROR, "build_cmdline do callback failed");
        goto fail0;
    }
    
    // build undo command
    if (!build_cmdline(o->i, params->args, 1, &o->undo_exec, &o->undo_cmdline)) {
        NCDModuleInst_Backend_Log(o->i, o->blog_channel, BLOG_ERROR, "build_cmdline undo callback failed");
        goto fail1;
    }
    
    // init lock job
    BEventLockJob_Init(&o->elock_job, elock, (BEventLock_handler)lock_handler, o);
    
    if (o->do_exec) {
        // wait for lock
        BEventLockJob_Wait(&o->elock_job);
        
        // set state
        o->state = STATE_ADDING_LOCK;
    } else {
        // set state
        o->state = STATE_DONE;
        
        // signal up
        NCDModuleInst_Backend_Up(o->i);
    }
    
    return;
    
fail1:
    if (o->do_exec) {
        free(o->do_exec);
        CmdLine_Free(&o->do_cmdline);
    }
fail0:
    o->free_func(o->user, 1);
}

static void free_template (command_template_instance *o, int is_error)
{
    // free lock job
    BEventLockJob_Free(&o->elock_job);
    
    // free undo command
    if (o->undo_exec) {
        free(o->undo_exec);
        CmdLine_Free(&o->undo_cmdline);
    }
    
    // free do command
    if (o->do_exec) {
        free(o->do_exec);
        CmdLine_Free(&o->do_cmdline);
    }
    
    // call free function
    o->free_func(o->user, is_error);
}

void command_template_die (command_template_instance *o)
{
    ASSERT(o->state == STATE_ADDING_LOCK || o->state == STATE_ADDING || o->state == STATE_DONE)
    
    switch (o->state) {
        case STATE_ADDING_LOCK: {
            free_template(o, 0);
            return;
        } break;
        
        case STATE_ADDING: {
            // set state
            o->state = STATE_ADDING_NEED_DELETE;
        } break;
        
        case STATE_DONE: {
            if (o->undo_exec) {
                // wait for lock
                BEventLockJob_Wait(&o->elock_job);
                
                // set state
                o->state = STATE_DELETING_LOCK;
            } else {
                free_template(o, 0);
                return;
            }
        } break;
    }
}
